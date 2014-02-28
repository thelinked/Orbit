#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <memory>
#include <atomic>

#include "TaskCore.hpp"
#include "TaskPool.hpp"
#include "LockingQueue.hpp"

namespace orbit
{
    enum ThreadType
    {
        MAIN,
        TASK0,
        TASK1 = TASK0 + 1,
        TASK2 = TASK0 + 2,
        TASK3 = TASK0 + 3,
        TASK4 = TASK0 + 4,
        TASK5 = TASK0 + 5,
        TASK6 = TASK0 + 6,
        TASK7 = TASK0 + 7,
    };

#ifdef _WIN32
    extern __declspec(thread) ThreadType threadType;
#else
    extern __thread ThreadType threadType;
#endif
    const char* ThreadName();

    class TaskQueue
    {
    public:
        /// Tries to queue a task.
        void queueTask(Task* task);

        /// Waits the calling thread until a task becomes available in the queue.
        Task* waitUntilTaskIsAvailable();

        /// Tries to get a task from the queue, returns \c nullptr if no task is currently available.
        Task* getAvailableTask(void);

    private:
        LockingQueue<Task*> queue;
    };

    class Scheduler;
    class ThreadPool
    {
    public:
        ThreadPool(Scheduler &_scheduler, TaskQueue &_queue);
        ThreadPool(const ThreadPool &) = delete;

        void initialise(uint8_t _numberOfThreads);
        void initialiseThread();

        void work();
        void shutdown();

    private:
        TaskQueue &queue;
        Scheduler &scheduler;

        std::atomic<int> threadNumber;
        std::atomic<bool> shouldRun;
        std::vector<std::thread> threads;
    };


    class Scheduler
    {
    public:
        class Pimpl;

        Scheduler();
        ~Scheduler();
        void initialise(uint8_t _cores);

        TaskId addTask(void *_kernelData, Kernel _kernel);
        TaskId addAndRunTask(void *_kernelData, Kernel _kernel);
        TaskId addEmptyTask();

        TaskId addStreamingTask(Kernel _kernel, void *_kernelData,
            InputStream _is0, OutputStream _os0, 
            size_t elementCount, size_t _elementsPerTask);

        TaskId addStreamingTask(Kernel _kernel, void *_kernelData,
            InputStream _is0, OutputStream _os0, 
            InputStream _is1, OutputStream _os1,
            size_t _elementCount, size_t _elementsPerTask);

        TaskId addStreamingTask(Kernel _kernel, void *_kernelData,
            InputStream _is0, OutputStream _os0, 
            InputStream _is1, OutputStream _os1,
            InputStream _is2, OutputStream _os2, 
            size_t _elementCount, size_t _elementsPerTask);

        void addChild(const TaskId& _parent, const TaskId& _child);
        void runTask(const TaskId& _id);

        void wait(const TaskId& _taskId);
        void waitWithoutHelping(const TaskId& _taskId);

        void workOnTask(Task* _task);

    private:
        void helpWithWork();
        void finishTask(Task* task);
        bool canExecuteTask(Task* _task);

        size_t determineNumberOfTasks(size_t _elementCount, size_t _elementsPerTask);
    private:
        Pimpl *impl;
    };
}