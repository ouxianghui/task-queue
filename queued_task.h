#pragma once

#include <type_traits>
#include <memory>

namespace vi {

// Base interface for asynchronously executed tasks.
// The interface basically consists of a single function, run(), that executes
// on the target queue.  For more details see the run() method and TaskQueue.
class QueuedTask {
public:
    virtual ~QueuedTask() = default;

    // Main routine that will run when the task is executed on the desired queue.
    // The task should return |true| to indicate that it should be deleted or
    // |false| to indicate that the queue should consider ownership of the task
    // having been transferred.  Returning |false| can be useful if a task has
    // re-posted itself to a different queue or is otherwise being re-used.
    virtual bool run() = 0;
};

// Simple implementation of QueuedTask for use with rtc::Bind and lambdas.
template <typename Closure>
class ClosureTask : public QueuedTask {
public:
    explicit ClosureTask(Closure&& closure)
        : closure_(std::forward<Closure>(closure)) {}

private:
    bool run() override {
        closure_();
        return true;
    }

    typename std::decay<Closure>::type closure_;
};

template <typename Closure>
std::unique_ptr<QueuedTask> ToQueuedTask(Closure&& closure) {
    return std::make_unique<ClosureTask<Closure>>(std::forward<Closure>(closure));
}

// Extends ClosureTask to also allow specifying cleanup code.
// This is useful when using lambdas if guaranteeing cleanup, even if a task
// was dropped (queue is too full), is required.
template <typename Closure, typename Cleanup>
class ClosureTaskWithCleanup : public ClosureTask<Closure> {
public:
    ClosureTaskWithCleanup(Closure&& closure, Cleanup&& cleanup)
        : ClosureTask<Closure>(std::forward<Closure>(closure))
        , cleanup_(std::forward<Cleanup>(cleanup)) {}
    ~ClosureTaskWithCleanup() override { cleanup_(); }

private:
    typename std::decay<Cleanup>::type cleanup_;
};

}
