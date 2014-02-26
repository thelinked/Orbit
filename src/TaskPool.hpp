#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <memory>
#include <atomic>

#include "FreeList.hpp"
#include "TaskCore.hpp"
#include "LockingQueue.hpp"

namespace orbit
{
    class TaskPool
    {
    public:
        TaskPool();
        ~TaskPool();

        Task* obtainTask();
        void returnTask(Task* _task);

        TaskId::Offset getTaskOffset(Task* _task);
        Task* getTask(TaskId::Offset _taskOffset);

        bool isTaskFinished(const TaskId& _taskId);

    private:
        const unsigned int TASK_POOL_SIZE = sizeof(Task) * configuration::MAX_TASK_COUNT;
        char taskPoolMemory[sizeof(Task) * configuration::MAX_TASK_COUNT];
        std::mutex guard;

        Freelist futureTaskPool;
        int32_t generation;
    };
}