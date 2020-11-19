#include "event.h"
#include <optional>

#ifdef _WIN32
#include <windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

namespace vi {

Event::Event() : Event(false, false) {

}

Event::Event(bool manual_reset, bool initially_signaled)
    : is_manual_reset_(manual_reset), event_status_(initially_signaled) {

}

Event::~Event() {
}

void Event::set() {
    std::unique_lock<std::mutex>(event_mutex_);
    event_status_ = true;
    event_cond_.notify_all();
}

void Event::reset() {
    std::unique_lock<std::mutex>(event_mutex_);
    event_status_ = false;
}

namespace {

#ifdef _WIN32
static void gettimeofday(struct timeval *tv, void *ignore)
{
    struct timeb tb;

    ftime(&tb);
    tv->tv_sec = (long)tb.time;
    tv->tv_usec = tb.millitm * 1000;
}
#endif

timespec getTimespec(const int milliseconds_from_now) {
    timespec ts;

    // Get the current time.
#if USE_CLOCK_GETTIME
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    timeval tv;
    gettimeofday(&tv, nullptr);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif

    // Add the specified number of milliseconds to it.
    ts.tv_sec += (milliseconds_from_now / 1000);
    ts.tv_nsec += (milliseconds_from_now % 1000) * 1000000;

    // Normalize.
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    return ts;
}

}  // namespace

bool Event::wait(const int give_up_after_ms, const int warn_after_ms) {
    // Instant when we'll log a warning message (because we've been waiting so
    // long it might be a bug), but not yet give up waiting. nullopt if we
    // shouldn't log a warning.
    const std::optional<timespec> warn_ts = warn_after_ms == kForever ||
            (give_up_after_ms != kForever && warn_after_ms > give_up_after_ms)
            ? std::nullopt
            : std::make_optional(getTimespec(warn_after_ms));

    // Instant when we'll stop waiting and return an error. nullopt if we should
    // never give up.
    const std::optional<timespec> give_up_ts =
            give_up_after_ms == kForever
            ? std::nullopt
            : std::make_optional(getTimespec(give_up_after_ms));

    //ScopedYieldPolicy::YieldExecution();

    std::unique_lock<std::mutex> lock(event_mutex_);

    // Wait for `event_cond_` to trigger and `event_status_` to be set, with the
    // given timeout (or without a timeout if none is given).
    const auto wait = [&](const std::optional<timespec> timeout_ts) {
        std::cv_status status = std::cv_status::no_timeout;
        while (!event_status_ && status == std::cv_status::no_timeout) {
            if (timeout_ts == std::nullopt) {
                event_cond_.wait(lock);
            } else {
                using namespace std::chrono;
                system_clock::time_point tp{duration_cast<system_clock::duration>(seconds{timeout_ts->tv_sec} + nanoseconds{(timeout_ts->tv_nsec)})};
                status = event_cond_.wait_until(lock, tp);
            }
        }
        return status;
    };

    std::cv_status error;
    if (warn_ts == std::nullopt) {
        error = wait(give_up_ts);
    } else {
        error = wait(warn_ts);
        if (error == std::cv_status::timeout) {
            error = wait(give_up_ts);
        }
    }

    // NOTE(liulk): Exactly one thread will auto-reset this event. All
    // the other threads will think it's unsignaled.  This seems to be
    // consistent with auto-reset events in WEBRTC_WIN
    if (error == std::cv_status::no_timeout && !is_manual_reset_) {
        event_status_ = false;
    }

    return (error == std::cv_status::no_timeout);
}

}
