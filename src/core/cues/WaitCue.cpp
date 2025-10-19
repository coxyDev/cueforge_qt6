// ============================================================================
// WaitCue.cpp - Timing wait/delay cue implementation
// CueForge Qt6 - Professional show control software
// ============================================================================

#include "WaitCue.h"
#include <QDebug>

namespace CueForge {

    WaitCue::WaitCue(QObject* parent)
        : Cue(CueType::Wait, parent)
        , timer_(new QTimer(this))
        , remainingTime_(0.0)
        , pauseTime_(0)
    {
        setName("Wait");
        setColor(QColor(255, 200, 100));
        setDuration(5.0);

        timer_->setInterval(100);
        connect(timer_, &QTimer::timeout, this, &WaitCue::onTimerTick);
    }

    void WaitCue::setWaitDuration(double seconds)
    {
        setDuration(seconds);
        remainingTime_ = seconds;
    }

    double WaitCue::elapsedTime() const
    {
        if (status() == CueStatus::Running) {
            return (elapsed_.elapsed() / 1000.0);
        }
        return 0.0;
    }

    bool WaitCue::execute()
    {
        if (duration() <= 0.0) {
            emit warning("Wait cue has zero duration");
            return false;
        }

        setStatus(CueStatus::Running);
        remainingTime_ = duration();

        elapsed_.start();
        timer_->start();

        QTimer::singleShot(static_cast<int>(duration() * 1000), this, &WaitCue::onWaitFinished);

        qDebug() << "WaitCue execute:" << name() << "duration:" << duration();

        return true;
    }

    void WaitCue::stop(double fadeTime)
    {
        Q_UNUSED(fadeTime);

        timer_->stop();
        remainingTime_ = 0.0;
        setStatus(CueStatus::Loaded);

        qDebug() << "WaitCue stopped:" << name();
    }

    void WaitCue::pause()
    {
        if (status() == CueStatus::Running) {
            timer_->stop();
            pauseTime_ = elapsed_.elapsed();
            setStatus(CueStatus::Paused);

            qDebug() << "WaitCue paused:" << name() << "at" << (pauseTime_ / 1000.0) << "seconds";
        }
    }

    void WaitCue::resume()
    {
        if (status() == CueStatus::Paused) {
            timer_->start();
            setStatus(CueStatus::Running);

            qDebug() << "WaitCue resumed:" << name();
        }
    }

    QJsonObject WaitCue::toJson() const
    {
        QJsonObject json = Cue::toJson();
        return json;
    }

    void WaitCue::fromJson(const QJsonObject& json)
    {
        Cue::fromJson(json);
        remainingTime_ = duration();
    }

    std::unique_ptr<Cue> WaitCue::clone() const
    {
        auto cloned = std::make_unique<WaitCue>();

        cloned->setNumber(number());
        cloned->setName(name() + " (copy)");
        cloned->setDuration(duration());
        cloned->setPreWait(preWait());
        cloned->setPostWait(postWait());
        cloned->setContinueMode(continueMode());
        cloned->setColor(color());
        cloned->setNotes(notes());

        return cloned;
    }

    void WaitCue::onTimerTick()
    {
        if (status() == CueStatus::Running) {
            remainingTime_ = duration() - elapsedTime();

            double progress = elapsedTime() / duration();
            emit executionProgress(qBound(0.0, progress, 1.0));
        }
    }

    void WaitCue::onWaitFinished()
    {
        if (status() == CueStatus::Running) {
            timer_->stop();
            remainingTime_ = 0.0;
            setStatus(CueStatus::Finished);
            emit executionFinished();

            qDebug() << "WaitCue finished:" << name();
        }
    }

} // namespace CueForge