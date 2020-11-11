#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

namespace vi {

class TaskQueue;

class TaskQueueManager {
public:
    static std::unique_ptr<TaskQueueManager>& instance();

    ~TaskQueueManager();

    void create(const std::vector<std::string>& nameList);

    TaskQueue* queue(const std::string& name);

    bool hasQueue(const std::string& name);

private:
    void clear();

    bool exist(const std::string& name);

private:
    TaskQueueManager();

    TaskQueueManager(TaskQueueManager&&) = delete;

    TaskQueueManager(const TaskQueueManager&) = delete;

    TaskQueueManager& operator=(const TaskQueueManager&) = delete;

private:
    std::mutex m_mutex;

    std::unordered_map<std::string, std::unique_ptr<TaskQueue>> m_queueMap;

};

}

#define TQMgr vi::TaskQueueManager::instance()

#define TQ(name) TQMgr->queue(name)
