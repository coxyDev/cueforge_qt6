// ============================================================================
// ErrorHandler.cpp - Error handling system implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "ErrorHandler.h"
#include <QUuid>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace CueForge {

    QString severityToString(ErrorSeverity severity)
    {
        switch (severity) {
        case ErrorSeverity::Info:     return "Info";
        case ErrorSeverity::Warning:  return "Warning";
        case ErrorSeverity::Error:    return "Error";
        case ErrorSeverity::Critical: return "Critical";
        default:                      return "Unknown";
        }
    }

    ErrorSeverity stringToSeverity(const QString& str)
    {
        QString lower = str.toLower();
        if (lower == "info") return ErrorSeverity::Info;
        if (lower == "warning") return ErrorSeverity::Warning;
        if (lower == "error") return ErrorSeverity::Error;
        if (lower == "critical") return ErrorSeverity::Critical;
        return ErrorSeverity::Info;
    }

    ErrorHandler::ErrorHandler(QObject* parent)
        : QObject(parent)
        , maxErrorHistory_(1000)
        , healthCheckTimer_(new QTimer(this))
        , loggingEnabled_(true)
        , autoRecoveryEnabled_(false)
        , monitoringActive_(false)
    {
        connect(healthCheckTimer_, &QTimer::timeout, this, &ErrorHandler::onHealthCheckTimer);
        qDebug() << "ErrorHandler initialized";
    }

    ErrorHandler::~ErrorHandler()
    {
        stopHealthMonitoring();
    }

    QString ErrorHandler::reportError(ErrorSeverity severity,
        const QString& message,
        const QString& source,
        const QString& contextData)
    {
        if (!loggingEnabled_) {
            return QString();
        }

        ErrorEntry entry;
        entry.id = generateErrorId();
        entry.severity = severity;
        entry.message = message;
        entry.source = source;
        entry.timestamp = QDateTime::currentDateTime();
        entry.contextData = contextData;
        entry.resolved = false;

        errors_.append(entry);

        if (errors_.size() > maxErrorHistory_) {
            pruneOldErrors();
        }

        emitAppropriateSignal(entry);

        qDebug() << "[" << severityToString(severity) << "]"
            << source << ":" << message;

        if (autoRecoveryEnabled_ && severity == ErrorSeverity::Critical) {
            attemptRecovery(entry.id);
        }

        return entry.id;
    }

    QString ErrorHandler::reportInfo(const QString& message, const QString& source)
    {
        return reportError(ErrorSeverity::Info, message, source);
    }

    QString ErrorHandler::reportWarning(const QString& message, const QString& source)
    {
        return reportError(ErrorSeverity::Warning, message, source);
    }

    QString ErrorHandler::reportError(const QString& message, const QString& source)
    {
        return reportError(ErrorSeverity::Error, message, source);
    }

    QString ErrorHandler::reportCritical(const QString& message, const QString& source)
    {
        return reportError(ErrorSeverity::Critical, message, source);
    }

    void ErrorHandler::resolveError(const QString& errorId)
    {
        for (ErrorEntry& entry : errors_) {
            if (entry.id == errorId && !entry.resolved) {
                entry.resolved = true;
                emit errorResolved(errorId);
                qDebug() << "Error resolved:" << errorId;
                break;
            }
        }
    }

    void ErrorHandler::clearResolvedErrors()
    {
        errors_.erase(std::remove_if(errors_.begin(), errors_.end(),
            [](const ErrorEntry& e) { return e.resolved; }), errors_.end());
    }

    void ErrorHandler::clearAllErrors()
    {
        errors_.clear();
        emit errorCountChanged(0);
        emit warningCountChanged(0);
    }

    QList<ErrorEntry> ErrorHandler::unresolvedErrors() const
    {
        QList<ErrorEntry> unresolved;
        for (const ErrorEntry& entry : errors_) {
            if (!entry.resolved) {
                unresolved.append(entry);
            }
        }
        return unresolved;
    }

    QList<ErrorEntry> ErrorHandler::errorsBySeverity(ErrorSeverity severity) const
    {
        QList<ErrorEntry> filtered;
        for (const ErrorEntry& entry : errors_) {
            if (entry.severity == severity) {
                filtered.append(entry);
            }
        }
        return filtered;
    }

    ErrorEntry ErrorHandler::getError(const QString& errorId) const
    {
        for (const ErrorEntry& entry : errors_) {
            if (entry.id == errorId) {
                return entry;
            }
        }
        return ErrorEntry();
    }

    int ErrorHandler::errorCount() const
    {
        int count = 0;
        for (const ErrorEntry& entry : errors_) {
            if (entry.severity == ErrorSeverity::Error ||
                entry.severity == ErrorSeverity::Critical) {
                if (!entry.resolved) {
                    count++;
                }
            }
        }
        return count;
    }

    int ErrorHandler::warningCount() const
    {
        int count = 0;
        for (const ErrorEntry& entry : errors_) {
            if (entry.severity == ErrorSeverity::Warning && !entry.resolved) {
                count++;
            }
        }
        return count;
    }

    int ErrorHandler::criticalErrorCount() const
    {
        int count = 0;
        for (const ErrorEntry& entry : errors_) {
            if (entry.severity == ErrorSeverity::Critical && !entry.resolved) {
                count++;
            }
        }
        return count;
    }

    bool ErrorHandler::isSystemHealthy() const
    {
        return criticalErrorCount() == 0 &&
            healthMetrics_.audioSystemHealthy &&
            healthMetrics_.fileSystemHealthy &&
            healthMetrics_.errorCount24h < 50;
    }

    void ErrorHandler::startHealthMonitoring(int intervalMs)
    {
        if (monitoringActive_) {
            return;
        }

        healthCheckTimer_->setInterval(intervalMs);
        healthCheckTimer_->start();
        monitoringActive_ = true;

        checkSystemHealth();

        qDebug() << "Health monitoring started with interval:" << intervalMs << "ms";
    }

    void ErrorHandler::stopHealthMonitoring()
    {
        if (monitoringActive_) {
            healthCheckTimer_->stop();
            monitoringActive_ = false;
            qDebug() << "Health monitoring stopped";
        }
    }

    void ErrorHandler::checkSystemHealth()
    {
        updateHealthMetrics();

        bool wasHealthy = isSystemHealthy();
        emit healthMetricsUpdated(healthMetrics_);

        bool nowHealthy = isSystemHealthy();
        if (wasHealthy != nowHealthy) {
            emit systemHealthChanged(nowHealthy);
        }
    }

    void ErrorHandler::updateHealthMetrics()
    {
        healthMetrics_.lastCheck = QDateTime::currentDateTime();

        QList<ErrorEntry> recent = recentErrors(24);
        healthMetrics_.errorCount24h = 0;
        healthMetrics_.warningCount24h = 0;

        for (const ErrorEntry& entry : recent) {
            if (entry.severity == ErrorSeverity::Error ||
                entry.severity == ErrorSeverity::Critical) {
                healthMetrics_.errorCount24h++;
            }
            else if (entry.severity == ErrorSeverity::Warning) {
                healthMetrics_.warningCount24h++;
            }
        }

        healthMetrics_.cpuUsage = 0.0;
        healthMetrics_.memoryUsage = 0;

        healthMetrics_.audioSystemHealthy = (criticalErrorCount() == 0);
        healthMetrics_.fileSystemHealthy = true;
    }

    bool ErrorHandler::attemptRecovery(const QString& errorId)
    {
        ErrorEntry error = getError(errorId);
        if (error.id.isEmpty()) {
            return false;
        }

        qDebug() << "Attempting recovery for error:" << errorId;

        bool success = false;

        if (error.source.contains("Audio", Qt::CaseInsensitive)) {
            qDebug() << "Attempting audio system recovery...";
            success = true;
        }

        if (success) {
            resolveError(errorId);
        }

        emit recoveryAttempted(errorId, success);
        return success;
    }

    void ErrorHandler::setAutoRecoveryEnabled(bool enabled)
    {
        autoRecoveryEnabled_ = enabled;
        qDebug() << "Auto-recovery" << (enabled ? "enabled" : "disabled");
    }

    QList<ErrorEntry> ErrorHandler::errorsInRange(const QDateTime& start, const QDateTime& end) const
    {
        QList<ErrorEntry> filtered;
        for (const ErrorEntry& entry : errors_) {
            if (entry.timestamp >= start && entry.timestamp <= end) {
                filtered.append(entry);
            }
        }
        return filtered;
    }

    QList<ErrorEntry> ErrorHandler::recentErrors(int hours) const
    {
        QDateTime cutoff = QDateTime::currentDateTime().addSecs(-hours * 3600);
        QList<ErrorEntry> recent;

        for (const ErrorEntry& entry : errors_) {
            if (entry.timestamp >= cutoff) {
                recent.append(entry);
            }
        }
        return recent;
    }

    bool ErrorHandler::exportErrorLog(const QString& filePath) const
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open error log file for writing:" << filePath;
            return false;
        }

        QTextStream out(&file);
        out << "CueForge Error Log\n";
        out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        out << "Total Errors: " << errors_.size() << "\n\n";

        for (const ErrorEntry& entry : errors_) {
            out << "----------------------------------------\n";
            out << "ID: " << entry.id << "\n";
            out << "Timestamp: " << entry.timestamp.toString(Qt::ISODate) << "\n";
            out << "Severity: " << severityToString(entry.severity) << "\n";
            out << "Source: " << entry.source << "\n";
            out << "Message: " << entry.message << "\n";
            out << "Resolved: " << (entry.resolved ? "Yes" : "No") << "\n";
            if (!entry.contextData.isEmpty()) {
                out << "Context: " << entry.contextData << "\n";
            }
            out << "\n";
        }

        file.close();
        qDebug() << "Error log exported to:" << filePath;
        return true;
    }

    void ErrorHandler::setMaxErrorHistory(int maxEntries)
    {
        maxErrorHistory_ = qMax(10, maxEntries);
        if (errors_.size() > maxErrorHistory_) {
            pruneOldErrors();
        }
    }

    void ErrorHandler::setLoggingEnabled(bool enabled)
    {
        loggingEnabled_ = enabled;
        qDebug() << "Error logging" << (enabled ? "enabled" : "disabled");
    }

    void ErrorHandler::pruneOldErrors()
    {
        while (errors_.size() > maxErrorHistory_) {
            errors_.removeFirst();
        }
    }

    QString ErrorHandler::generateErrorId() const
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }

    void ErrorHandler::emitAppropriateSignal(const ErrorEntry& error)
    {
        emit errorOccurred(error);

        switch (error.severity) {
        case ErrorSeverity::Warning:
            emit warningOccurred(error);
            emit warningCountChanged(warningCount());
            break;
        case ErrorSeverity::Error:
            emit errorCountChanged(errorCount());
            break;
        case ErrorSeverity::Critical:
            emit criticalErrorOccurred(error);
            emit errorCountChanged(errorCount());
            break;
        default:
            break;
        }
    }

    void ErrorHandler::onHealthCheckTimer()
    {
        checkSystemHealth();
    }

} // namespace CueForge