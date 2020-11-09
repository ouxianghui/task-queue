#pragma once

#include <string.h>
#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <utility>
#include <thread>
#include <string_view>
#include "queued_task.h"
#include "event.h"
#include "task_queue_base.h"

namespace vi {

class TaskQueueSTD final : public TaskQueueBase {
public:
    TaskQueueSTD(std::string_view queueName);
    ~TaskQueueSTD() override = default;

    void deleteThis() override;

    void postTask(std::unique_ptr<QueuedTask> task) override;

    void postDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds) override;

    const std::string& name() const override;

private:
    using OrderId = uint64_t;

    struct DelayedEntryTimeout {
        int64_t next_fire_at_ms_{};
        OrderId order_{};

        bool operator<(const DelayedEntryTimeout& o) const {
            return std::tie(next_fire_at_ms_, order_) < std::tie(o.next_fire_at_ms_, o.order_);
        }
    };

    struct NextTask {
        bool final_task_{false};
        std::unique_ptr<QueuedTask> run_task_;
        int64_t sleep_time_ms_{};
    };

    NextTask getNextTask();

    void processTasks();

    void notifyWake();

    static int64_t milliseconds();

private:
    // Indicates if the thread has started.
    vi::Event started_;

    // Indicates if the thread has stopped.
    vi::Event stopped_;

    // Signaled whenever a new task is pending.
    vi::Event flag_notify_;

    // Contains the active worker thread assigned to processing
    // tasks (including delayed tasks).
    std::thread thread_;

    std::mutex pending_mutex_;

    // Indicates if the worker thread needs to shutdown now.
    bool thread_should_quit_ {false};

    // Holds the next order to use for the next task to be
    // put into one of the pending queues.
    OrderId thread_posting_order_ {};

    // The list of all pending tasks that need to be processed in the
    // FIFO queue ordering on the worker thread.
    std::queue<std::pair<OrderId, std::unique_ptr<QueuedTask>>> pending_queue_;

    // The list of all pending tasks that need to be processed at a future
    // time based upon a delay. On the off change the delayed task should
    // happen at exactly the same time interval as another task then the
    // task is processed based on FIFO ordering. std::priority_queue was
    // considered but rejected due to its inability to extract the
    // std::unique_ptr out of the queue without the presence of a hack.
    std::map<DelayedEntryTimeout, std::unique_ptr<QueuedTask>> delayed_queue_;

    std::string name_;

};

}

