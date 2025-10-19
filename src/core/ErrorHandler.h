// ============================================================================
// ErrorHandler.h - Comprehensive error handling and monitoring system
// CueForge Qt6 - Professional show control software
// ============================================================================

#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QTimer>

namespace CueForge {

    enum class ErrorSeverity {
        Info,
        Warning,
        Error,
        Critical
    };

    struct ErrorEntry {
        QString id;
        ErrorSeverity severity;
        QString message;
        QString source;
        QDateTime timestamp;
        QString contextData;
        bool resolved;

        ErrorEntry()
            : severity(ErrorSeverity::Info)
            , resolved(false)
        {
        }
    };

    struct HealthMetrics {
        double cpuUsage;
        qint64 memoryUsage;
        int errorCount24h;
        int warningCount24h;
        bool audioSystemHealthy;
        bool fileSystemHealthy;
        QDateTime lastCheck;

        HealthMetrics()
            : cpuUsage(0.0)
            , memoryUsage(0)
            , errorCount24h(0)
            , warningCount24h(0)
            , audioSystemHealthy(true)
            , fileSystemHealthy(true)
        {
        }
    };

    class ErrorHandler : public QObject
    {
        Q_OBJECT
            Q_PROPERTY(int errorCount READ errorCount NOTIFY errorCountChanged)
            Q_PROPERTY(int warningCount READ warningCount NOTIFY warningCountChanged)
            Q_PROPERTY(bool systemHealthy READ isSystemHealthy NOTIFY systemHealthChanged)

    public:
        explicit ErrorHandler(QObject* parent = nullptr);
        ~ErrorHandler() override;

        // Error Reporting
        QString reportError(ErrorSeverity severity,
            const QString& message,
            const QString& source,
            const QString& contextData = QString());

        QString reportInfo(const QString& message, const QString& source = "System");
        QString reportWarning(const QString& message, const QString& source = "System");
        QString reportError(const QString& message, const QString& source = "System");
        QString reportCritical(const QString& message, const QString& source = "System");

        void resolveError(const QString& errorId);
        void clearResolvedErrors();
        void clearAllErrors();

        // Error Access
        QList<ErrorEntry> errors() const { return errors_; }
        QList<ErrorEntry> unresolvedErrors() const;
        QList<ErrorEntry> errorsBySeverity(ErrorSeverity severity) const;
        ErrorEntry getError(const QString& errorId) const;

        int errorCount() const;
        int warningCount() const;
        int criticalErrorCount() const;

        // Health Monitoring
        HealthMetrics healthMetrics() const { return healthMetrics_; }
        bool isSystemHealthy() const;
        void startHealthMonitoring(int intervalMs = 5000);
        void stopHealthMonitoring();
        void checkSystemHealth();

        // Recovery System
        bool attemptRecovery(const QString& errorId);
        void setAutoRecoveryEnabled(bool enabled);
        bool isAutoRecoveryEnabled() const { return autoRecoveryEnabled_; }

        // Error History
        QList<ErrorEntry> errorsInRange(const QDateTime& start, const QDateTime& end) const;
        QList<ErrorEntry> recentErrors(int hours = 24) const;
        bool exportErrorLog(const QString& filePath) const;

        // Logging Control
        void setMaxErrorHistory(int maxEntries);
        void setLoggingEnabled(bool enabled);
        bool isLoggingEnabled() const { return loggingEnabled_; }

    signals:
        void errorOccurred(const ErrorEntry& error);
        void warningOccurred(const ErrorEntry& error);
        void criticalErrorOccurred(const ErrorEntry& error);
        void errorResolved(const QString& errorId);

        void errorCountChanged(int count);
        void warningCountChanged(int count);

        void systemHealthChanged(bool healthy);
        void healthMetricsUpdated(const HealthMetrics& metrics);

        void recoveryAttempted(const QString& errorId, bool success);

    private slots:
        void onHealthCheckTimer();

    private:
        void updateHealthMetrics();
        void pruneOldErrors();
        QString generateErrorId() const;
        void emitAppropriateSignal(const ErrorEntry& error);

        QList<ErrorEntry> errors_;
        int maxErrorHistory_;

        QTimer* healthCheckTimer_;
        HealthMetrics healthMetrics_;

        bool loggingEnabled_;
        bool autoRecoveryEnabled_;
        bool monitoringActive_;
    };

    QString severityToString(ErrorSeverity severity);
    ErrorSeverity stringToSeverity(const QString& str);

} // namespace CueForge

Q_DECLARE_METATYPE(CueForge::ErrorSeverity)
Q_DECLARE_METATYPE(CueForge::ErrorEntry)
Q_DECLARE_METATYPE(CueForge::HealthMetrics)