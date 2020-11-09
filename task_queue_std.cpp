#include "task_queue_std.h"

namespace vi {

TaskQueueSTD::TaskQueueSTD(std::string_view queueName)
    : started_(/*manual_reset=*/false, /*initially_signaled=*/false)
    , stopped_(/*manual_reset=*/false, /*initially_signaled=*/false)
    , flag_notify_(/*manual_reset=*/false, /*initially_signaled=*/false)
    , name_(queueName) {

    thread_ = std::thread(&TaskQueueSTD::run, this);

    started_.wait(vi::Event::kForever);
}

void TaskQueueSTD::deleteThis() {
    //RTC_DCHECK(!isCurrent());
    assert(isCurrent() == false);

    {
        std::unique_lock<std::mutex> lock(pending_mutex_);
        thread_should_quit_ = true;
    }

    notifyWake();

    stopped_.wait(vi::Event::kForever);

    if (thread_.joinable()) {
        thread_.join();
    }
    delete this;
}

void TaskQueueSTD::postTask(std::unique_ptr<QueuedTask> task) {
    {
        std::unique_lock<std::mutex> lock(pending_mutex_);
        OrderId order = thread_posting_order_++;

        pending_queue_.push(std::pair<OrderId, std::unique_ptr<QueuedTask>>(order, std::move(task)));
    }

    notifyWake();
}

void TaskQueueSTD::postDelayedTask(std::unique_ptr<QueuedTask> task, uint32_t ms) {
    auto fire_at = milliseconds() + ms;

    DelayedEntryTimeout delay;
    delay.next_fire_at_ms_ = fire_at;

    {
        std::unique_lock<std::mutex> lock(pending_mutex_);
        delay.order_ = ++thread_posting_order_;
        delayed_queue_[delay] = std::move(task);
    }

    notifyWake();
}

TaskQueueSTD::NextTask TaskQueueSTD::getNextTask() {
    NextTask result{};

    auto tick = milliseconds();

    std::unique_lock<std::mutex> lock(pending_mutex_);

    if (thread_should_quit_) {
        result.final_task_ = true;
        return result;
    }

    if (delayed_queue_.size() > 0) {
        auto delayed_entry = delayed_queue_.begin();
        const auto& delay_info = delayed_entry->first;
        auto& delay_run = delayed_entry->second;
        if (tick >= delay_info.next_fire_at_ms_) {
            if (pending_queue_.size() > 0) {
                auto& entry = pending_queue_.front();
                auto& entry_order = entry.first;
                auto& entry_run = entry.second;
                if (entry_order < delay_info.order_) {
                    result.run_task_ = std::move(entry_run);
                    pending_queue_.pop();
                    return result;
                }
            }

            result.run_task_ = std::move(delay_run);
            delayed_queue_.erase(delayed_entry);
            return result;
        }

        result.sleep_time_ms_ = delay_info.next_fire_at_ms_ - tick;
    }

    if (pending_queue_.size() > 0) {
        auto& entry = pending_queue_.front();
        result.run_task_ = std::move(entry.second);
        pending_queue_.pop();
    }

    return result;
}

// static
void TaskQueueSTD::run(void* context) {
    TaskQueueSTD* me = static_cast<TaskQueueSTD*>(context);
    CurrentTaskQueueSetter setCurrent(me);
    me->processTasks();
}

void TaskQueueSTD::processTasks() {
    started_.set();

    while (true) {
        auto task = getNextTask();

        if (task.final_task_) {
            break;
        }

        if (task.run_task_) {
            // process entry immediately then try again
            QueuedTask* release_ptr = task.run_task_.release();
            if (release_ptr->run()) {
                delete release_ptr;
            }
            // attempt to sleep again
            continue;
        }

        if (0 == task.sleep_time_ms_) {
            flag_notify_.wait(vi::Event::kForever);
        }
        else {
            flag_notify_.wait(task.sleep_time_ms_);
        }
    }

    stopped_.set();
}

void TaskQueueSTD::notifyWake() {
    // The queue holds pending tasks to complete. Either tasks are to be
    // executed immediately or tasks are to be run at some future delayed time.
    // For immediate tasks the task queue's thread is busy running the task and
    // the thread will not be waiting on the flag_notify_ event. If no immediate
    // tasks are available but a delayed task is pending then the thread will be
    // waiting on flag_notify_ with a delayed time-out of the nearest timed task
    // to run. If no immediate or pending tasks are available, the thread will
    // wait on flag_notify_ until signaled that a task has been added (or the
    // thread to be told to shutdown).

    // In all cases, when a new immediate task, delayed task, or request to
    // shutdown the thread is added the flag_notify_ is signaled after. If the
    // thread was waiting then the thread will wake up immediately and re-assess
    // what task needs to be run next (i.e. run a task now, wait for the nearest
    // timed delayed task, or shutdown the thread). If the thread was not waiting
    // then the thread will remained signaled to wake up the next time any
    // attempt to wait on the flag_notify_ event occurs.

    // Any immediate or delayed pending task (or request to shutdown the thread)
    // must always be added to the queue prior to signaling flag_notify_ to wake
    // up the possibly sleeping thread. This prevents a race condition where the
    // thread is notified to wake up but the task queue's thread finds nothing to
    // do so it waits once again to be signaled where such a signal may never
    // happen.
    flag_notify_.set();
}

int64_t TaskQueueSTD::milliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

const std::string& TaskQueueSTD::name() const {
    return name_;
}

}
