#pragma once

#include <stdint.h>

#include <memory>
#include <string_view>
#include "queued_task.h"


namespace vi {
// Implements a task queue that asynchronously executes tasks in a way that
// guarantees that they're executed in FIFO order and that tasks never overlap.
// Tasks may always execute on the same worker thread and they may not.
// To DCHECK that tasks are executing on a known task queue, use IsCurrent().
//
// Here are some usage examples:
//
//   1) Asynchronously running a lambda:
//
//     class MyClass {
//       ...
//       TaskQueue queue_("MyQueue");
//     };
//
//     void MyClass::StartWork() {
//       queue_.PostTask([]() { Work(); });
//     ...
//
//   2) Posting a custom task on a timer.  The task posts itself again after
//      every running:
//
//     class TimerTask : public QueuedTask {
//      public:
//       TimerTask() {}
//      private:
//       bool Run() override {
//         ++count_;
//         TaskQueueBase::Current()->PostDelayedTask(
//             absl::WrapUnique(this), 1000);
//         // Ownership has been transferred to the next occurance,
//         // so return false to prevent from being deleted now.
//         return false;
//       }
//       int count_ = 0;
//     };
//     ...
//     queue_.PostDelayedTask(std::make_unique<TimerTask>(), 1000);
//
// For more examples, see task_queue_unittests.cc.
//
// A note on destruction:
//

class TaskQueueBase;
class TaskQueueDeleter;

// When a TaskQueue is deleted, pending tasks will not be executed but they will
// be deleted.  The deletion of tasks may happen asynchronously after the
// TaskQueue itself has been deleted or it may happen synchronously while the
// TaskQueue instance is being deleted.  This may vary from one OS to the next
// so assumptions about lifetimes of pending tasks should not be made.
class TaskQueue {
public:
    explicit TaskQueue(std::unique_ptr<TaskQueueBase, TaskQueueDeleter> taskQueue);
    ~TaskQueue();

    static std::unique_ptr<TaskQueue> create(std::string_view name);

    // Used for DCHECKing the current queue.
    bool isCurrent() const;

    // Returns non-owning pointer to the task queue implementation.
    TaskQueueBase* get() { return impl_; }

    // TODO(tommi): For better debuggability, implement RTC_FROM_HERE.

    // Ownership of the task is passed to PostTask.
    void postTask(std::unique_ptr<QueuedTask> task);

    // Schedules a task to execute a specified number of milliseconds from when
    // the call is made. The precision should be considered as "best effort"
    // and in some cases, such as on Windows when all high precision timers have
    // been used up, can be off by as much as 15 millseconds (although 8 would be
    // more likely). This can be mitigated by limiting the use of delayed tasks.
    void postDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t milliseconds);


    // std::enable_if is used here to make sure that calls to PostTask() with
    // std::unique_ptr<SomeClassDerivedFromQueuedTask> would not end up being
    // caught by this template.
    template <class Closure, typename std::enable_if<!std::is_convertible<Closure, std::unique_ptr<QueuedTask>>::value>::type* = nullptr>
    void postTask(Closure&& closure) {
        postTask(ToQueuedTask(std::forward<Closure>(closure)));
    }

    // See documentation above for performance expectations.
    template <class Closure, typename std::enable_if<!std::is_convertible<Closure, std::unique_ptr<QueuedTask>>::value>::type* = nullptr>
    void postDelayedTask(Closure&& closure, uint32_t milliseconds) {
        postDelayedTask(ToQueuedTask(std::forward<Closure>(closure)),  milliseconds);
    }


private:
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue(const TaskQueue&) = delete;

private:
    TaskQueueBase* const impl_;

};

}
