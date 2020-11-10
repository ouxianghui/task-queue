#pragma once

#include <mutex>
#include <condition_variable>

namespace vi {

class Event {
public:
    static const int kForever = -1;

    Event();
    Event(bool manual_reset, bool initially_signaled);
    ~Event();

    void set();
    void reset();

    // Waits for the event to become signaled, but logs a warning if it takes more
    // than `warn_after_ms` milliseconds, and gives up completely if it takes more
    // than `give_up_after_ms` milliseconds. (If `warn_after_ms >=
    // give_up_after_ms`, no warning will be logged.) Either or both may be
    // `kForever`, which means wait indefinitely.
    //
    // Returns true if the event was signaled, false if there was a timeout or
    // some other error.
    bool wait(int give_up_after_ms, int warn_after_ms);

    // Waits with the given timeout and a reasonable default warning timeout.
    bool wait(int give_up_after_ms) {
        return wait(give_up_after_ms, give_up_after_ms == kForever ? 3000 : kForever);
    }

private:
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

private:
    std::mutex event_mutex_;
    std::condition_variable event_cond_;
    const bool is_manual_reset_;
    bool event_status_;
};

}
