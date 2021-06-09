#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <QAbstractEventDispatcher>
#include <QSocketNotifier>

class QSocketNotifier;

struct uv_loop_s;
struct uv_async_s;
struct uv_poll_s;
struct uv_timer_s;
struct uv_handle_s;

namespace i9ms
{
    class LibUvEventDispatcher final : public QAbstractEventDispatcher
    {
        Q_OBJECT
        Q_DISABLE_COPY(LibUvEventDispatcher)

    public:
        // ctor
        explicit LibUvEventDispatcher(QObject* parent = nullptr);

        // dtor
        ~LibUvEventDispatcher();

        // returns raw libuv loop
        uv_loop_s* rawLoop() const;

        // QAbstractEventDispatcher interface -->

        void interrupt() override;
        bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

        void registerSocketNotifier(QSocketNotifier* notifier) override;
        void unregisterSocketNotifier(QSocketNotifier* notifier) override;

        void enableSocketNotifier(QSocketNotifier* notifier);
        void disableSocketNotifier(QSocketNotifier* notifier);

        void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject* object) override;
        bool unregisterTimer(int timerId) override;
        bool unregisterTimers(QObject* object) override;

        QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject* object) const override;
        int remainingTime(int timerId) override;
        void wakeUp() override;

#if defined(Q_OS_WIN)
        // windows specific members - not implemented -->

        bool registerEventNotifier(QWinEventNotifier* notifier) override;
        void unregisterEventNotifier(QWinEventNotifier* notifier) override;

        // <-- windows specific members - not implemented
#endif

        // obsolete members - not implemented -->

        void flush() override;
        bool hasPendingEvents() override;

        // <-- obsolete members - not implemented

        // <-- QAbstractEventDispatcher interface

        // poller struct data
        struct PollerData {
            explicit PollerData(int socketDescriptor, uv_poll_s* uvPollData, int events, LibUvEventDispatcher* context)
                : events(events)
                , socketDescriptor(socketDescriptor)
                , uvPollData(uvPollData)
                , context(context)
            {
            }

            int events;
            int socketDescriptor;
            uv_poll_s* uvPollData;
            QSocketNotifier* readNotifier{ nullptr };
            QSocketNotifier* writeNotifier{ nullptr };
            LibUvEventDispatcher* context;
        };

    private:
        // morphs qt event type to libuv's
        int qtouv(QSocketNotifier::Type qtEventType) const;

        // timer data struct
        struct TimerData {
            TimerData() = delete;

            explicit TimerData(int timerID, int interval, QObject* qobject, Qt::TimerType timerType, uv_timer_s* timer, LibUvEventDispatcher* context)
                : timerID(timerID)
                , interval(interval)
                , qobject(qobject)
                , timerType(timerType)
                , timer(timer)
                , context(context)
            {
            }

            int timerID;
            uint64_t lastFired{ 0 };
            int interval;
            QObject* qobject;
            Qt::TimerType timerType;
            uv_timer_s* timer;
            LibUvEventDispatcher* context;
        };

        // libuv's socket notifier callback
        static void socketCallback(uv_poll_s* w, int status, int events);

        // libuv's timer callback
        static void timerCallback(uv_timer_s* w);

        // libuv's timer delete callback
        static void timerDeleteCallback(uv_handle_s* w);

        // libuv's loop
        uv_loop_s* m_uvLoop{ nullptr };

        // libuv' async handle
        uv_async_s* m_wakeupHandle{ nullptr };

        // socket pollers
        std::unordered_map<int, std::unique_ptr<PollerData>> m_socketPollers;

        // timers
        std::unordered_map<int, TimerData*> m_timers;
        std::unordered_map<QObject*, std::unordered_set<TimerData*>> m_objectTimerIndex;

        // true if interrupt was requested
        std::atomic<bool> m_interrupt{ false };

        // amount of processed callbacks in single processEvents() call
        std::uint64_t m_processedCallbacks{ 0 };
    };
}   // namespace i9ms

Q_DECLARE_METATYPE(i9ms::LibUvEventDispatcher::PollerData*);
