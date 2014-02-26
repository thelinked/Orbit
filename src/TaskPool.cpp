#include "TaskPool.hpp"

namespace orbit
{
    TaskPool::TaskPool() : futureTaskPool(taskPoolMemory, taskPoolMemory + TASK_POOL_SIZE, sizeof(Task)), generation(0) {}
    TaskPool::~TaskPool() {}

    Task* TaskPool::obtainTask()
    {
        void* memory = nullptr;
        {
            std::lock_guard<std::mutex> lock(guard);
            memory = futureTaskPool.Obtain();
        }

        Task* task = new(memory) Task();

        // the generation is a unique ID which allows us to distinguish between proper tasks and deleted ones
        task->generation = ++generation;
        return task;
    }

    void TaskPool::returnTask(Task* _task)
    {
        _task->generation = ++generation;
        //delete task;
        {
            std::lock_guard<std::mutex> lock(guard);
            futureTaskPool.Return(_task);
        }
    }

    TaskId::Offset TaskPool::getTaskOffset(Task* _task)
    {
        return _task - reinterpret_cast<Task*>(taskPoolMemory);
    }

    Task* TaskPool::getTask(TaskId::Offset _taskOffset)
    {
        return reinterpret_cast<Task*>(taskPoolMemory) + _taskOffset;
    }

    bool TaskPool::isTaskFinished(const TaskId& _taskId)
    {
        Task* task = getTask(_taskId.offset);
        if (task->generation != _taskId.generation)
        {
            // task is from an older generation and has been recycled again, so it's been finished already
            return true;
        }
        else
        {
            if (task->openTasks == 0)
            {
                return true;
            }
        }
        return false;
    }
}