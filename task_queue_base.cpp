#include "task_queue_base.h"
#include <thread>

namespace vi {

namespace {

thread_local TaskQueueBase* _current = nullptr;

}  // namespace

TaskQueueBase* TaskQueueBase::current() {
    return _current;
}

TaskQueueBase::CurrentTaskQueueSetter::CurrentTaskQueueSetter(TaskQueueBase* taskQueue)
    : _previous(_current) {
    _current = taskQueue;
}

TaskQueueBase::CurrentTaskQueueSetter::~CurrentTaskQueueSetter() {
    _current = _previous;
}

}
