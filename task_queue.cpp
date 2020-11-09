#include "task_queue.h"
#include "task_queue_base.h"
#include "task_queue_std.h"

namespace vi {

TaskQueue::TaskQueue(std::unique_ptr<TaskQueueBase, TaskQueueDeleter> taskQueue)
    : impl_(taskQueue.release()) {}

TaskQueue::~TaskQueue() {
    // There might running task that tries to rescheduler itself to the TaskQueue
    // and not yet aware TaskQueue destructor is called.
    // Calling back to TaskQueue::PostTask need impl_ pointer still be valid, so
    // do not invalidate impl_ pointer until Delete returns.
    impl_->deleteThis();
}

bool TaskQueue::isCurrent() const {
    return impl_->isCurrent();
}

void TaskQueue::postTask(std::unique_ptr<QueuedTask> task) {
    return impl_->postTask(std::move(task));
}

void TaskQueue::postDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds) {
    return impl_->postDelayedTask(std::move(task), milliseconds);
}

std::unique_ptr<TaskQueue> TaskQueue::create(std::string_view name) {
    return std::make_unique<TaskQueue>(std::unique_ptr<TaskQueueBase, TaskQueueDeleter>(new TaskQueueSTD(name)));
}

}
