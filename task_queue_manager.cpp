#include "task_queue_manager.h"
#include "task_queue.h"

namespace vi {

std::unique_ptr<TaskQueueManager>& TaskQueueManager::instance()
{
    static std::unique_ptr<TaskQueueManager> _instance = nullptr;
    static std::once_flag ocf;
    std::call_once(ocf, [](){
        _instance.reset(new TaskQueueManager());
    });
    return _instance;
}

TaskQueueManager::TaskQueueManager()
{

}

TaskQueueManager::~TaskQueueManager()
{
    clear();
}

void TaskQueueManager::create(const std::vector<std::string>& nameList)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    for (const auto& name : nameList) {
        if (!exist(name)) {
            m_queueMap[name] = TaskQueue::create(name);
        }
    }
}

void TaskQueueManager::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queueMap.clear();
}

bool TaskQueueManager::exist(const std::string& name)
{
    return (m_queueMap.find(name) != m_queueMap.end());
}

bool TaskQueueManager::hasQueue(const std::string& name)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return exist(name);
}

TaskQueue* TaskQueueManager::queue(const std::string& name)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return exist(name) ? m_queueMap[name].get() : nullptr;
}

}
