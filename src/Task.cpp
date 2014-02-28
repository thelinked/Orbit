#include "Task.hpp"

namespace orbit
{
#ifdef _WIN32
        __declspec(thread) ThreadType threadType;
#else
        __thread ThreadType threadType;
#endif
    const char* ThreadName()
    {
        switch (threadType)
        {
        case ThreadType::MAIN: return "Main";
        case ThreadType::TASK0: return "Task0";
        case ThreadType::TASK1: return "Task1";
        case ThreadType::TASK2: return "Task2";
        case ThreadType::TASK3: return "Task3";
        case ThreadType::TASK4: return "Task4";
        case ThreadType::TASK5: return "Task5";
        case ThreadType::TASK6: return "Task6";
        case ThreadType::TASK7: return "Task7";
        default: return "Unknown";
        }
    }

    ThreadPool::ThreadPool(Scheduler &_scheduler, TaskQueue &_queue) : scheduler(_scheduler), queue(_queue)
    {
        threadNumber = 0;
        shouldRun.store(true);
    }

    void ThreadPool::initialise(uint8_t _numberOfThreads)
    {
        threads.reserve(_numberOfThreads);
        auto work = std::bind(&ThreadPool::initialiseThread, this);
        for (uint8_t i(0); i != _numberOfThreads; ++i)
        {
            threads.push_back(std::thread(work));
        }
    }

    void ThreadPool::initialiseThread()
    {
        int number = threadNumber++;
        ::orbit::threadType = static_cast<ThreadType>(ThreadType::TASK0 + number);

        work();
    }
    void ThreadPool::work()
    {
        while (shouldRun.load())
        {
            Task* task = queue.waitUntilTaskIsAvailable();
            if (task)
            {
                scheduler.workOnTask(task);
            }
        }
    }

    void ThreadPool::shutdown()
    {
        shouldRun.store(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        for (auto &thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void TaskQueue::queueTask(Task* task)
    {
        queue.push(task);
    }

    Task* TaskQueue::waitUntilTaskIsAvailable()
    {
        Task* task;
        return queue.tryWaitAndPop(task, 1000) ? task : nullptr;
    }

    Task* TaskQueue::getAvailableTask()
    {
        Task* task;
        return queue.tryPop(task) ? task : nullptr;
    }


    class Scheduler::Pimpl
    {
    public:
        Pimpl(Scheduler &_scheduler) : threads(_scheduler, queue)
        {
        }

        ThreadPool threads;
        TaskPool taskPool;
        TaskQueue queue;
        Kernel emptyKernal;
    };

    Scheduler::Scheduler()
    {
        impl = new Pimpl(*this);
    }
    Scheduler::~Scheduler()
    {
        impl->threads.shutdown();
    }

    void Scheduler::initialise(uint8_t _cores)
    {
        threadType = MAIN;
        impl->threads.initialise(_cores);
    }

    TaskId Scheduler::addTask(void *_kernelData, Kernel _kernel)
    {
        Task* task = impl->taskPool.obtainTask();
        task->kernel = _kernel;
        task->taskData.kernelData = _kernelData;

        return TaskId(impl->taskPool.getTaskOffset(task), task->generation);
    }

    TaskId Scheduler::addAndRunTask(void *_kernelData, Kernel _kernel)
    {
        Task* task = impl->taskPool.obtainTask();
        task->kernel = _kernel;
        task->taskData.kernelData = _kernelData;

        impl->queue.queueTask(task);

        return TaskId(impl->taskPool.getTaskOffset(task), task->generation);
    }

    TaskId Scheduler::addEmptyTask()
    {
        Task* root = impl->taskPool.obtainTask();
        root->kernel = impl->emptyKernal;

        return TaskId(impl->taskPool.getTaskOffset(root), root->generation);
    }

    TaskId Scheduler::addStreamingTask(Kernel _kernel, void *_kernelData,
        InputStream _is0, OutputStream _os0,
        size_t _elementCount, size_t _elementsPerTask)
    {
        const size_t N = determineNumberOfTasks(_elementCount, _elementsPerTask);

        // add a root task used for synchronisation
        Task* root = impl->taskPool.obtainTask();
        root->kernel = impl->emptyKernal;
        root->openTasks = (_elementCount % N == 0) ? N + 1 : N + 2;
        root->parent = Task::NO_PARENT;

        TaskId::Offset rootOffset = impl->taskPool.getTaskOffset(root);
        {
            // split the task into several subtasks, according to the size of the input/output streams
            const size_t perElementCount = _elementCount / N;
            for (int i = 0; i < N; ++i)
            {
                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = perElementCount;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + i*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + i*_os0.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }

            if (_elementCount % N != 0)
            {
                const size_t handled = perElementCount * N;

                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = _elementCount - handled;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + N*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + N*_os0.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }
        }

        return TaskId(rootOffset, root->generation);
    }

    TaskId Scheduler::addStreamingTask(Kernel _kernel, void *_kernelData,
        InputStream _is0, OutputStream _os0,
        InputStream _is1, OutputStream _os1,
        size_t _elementCount, size_t _elementsPerTask)
    {
        const size_t N = determineNumberOfTasks(_elementCount, _elementsPerTask);

        // add a root task used for synchronisation
        Task* root = impl->taskPool.obtainTask();
        root->kernel = impl->emptyKernal;
        root->openTasks = (_elementCount % N == 0) ? N + 1 : N + 2;
        root->parent = Task::NO_PARENT;

        TaskId::Offset rootOffset = impl->taskPool.getTaskOffset(root);
        {
            // split the task into several subtasks, according to the size of the input/output streams
            const size_t perElementCount = _elementCount / N;
            for (int i = 0; i < N; ++i)
            {
                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = perElementCount;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + i*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + i*_os0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[1] =
                    static_cast<char*>(_is1.data) + i*_is1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[1] =
                    static_cast<char*>(_os1.data) + i*_os1.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }

            if (_elementCount % N != 0)
            {
                const size_t handled = perElementCount * N;

                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = _elementCount - handled;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + N*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + N*_os0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[1] =
                    static_cast<char*>(_is1.data) + N*_is1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[1] =
                    static_cast<char*>(_os1.data) + N*_os1.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }
        }

        return TaskId(rootOffset, root->generation);
    }

    TaskId Scheduler::addStreamingTask(Kernel _kernel, void *_kernelData,
        InputStream _is0, OutputStream _os0,
        InputStream _is1, OutputStream _os1,
        InputStream _is2, OutputStream _os2,
        size_t _elementCount, size_t _elementsPerTask)
    {
        const size_t N = determineNumberOfTasks(_elementCount, _elementsPerTask);

        // add a root task used for synchronisation
        Task* root = impl->taskPool.obtainTask();
        root->kernel = impl->emptyKernal;
        root->openTasks = (_elementCount % N == 0) ? N + 1 : N + 2;
        root->parent = Task::NO_PARENT;

        TaskId::Offset rootOffset = impl->taskPool.getTaskOffset(root);
        {
            // split the task into several subtasks, according to the size of the input/output streams
            const size_t perElementCount = _elementCount / N;
            for (int i = 0; i < N; ++i)
            {
                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = perElementCount;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + i*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + i*_os0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[1] =
                    static_cast<char*>(_is1.data) + i*_is1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[1] =
                    static_cast<char*>(_os1.data) + i*_os1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[2] =
                    static_cast<char*>(_is2.data) + i*_is2.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[2] =
                    static_cast<char*>(_os2.data) + i*_os2.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }

            if (_elementCount % N != 0)
            {
                const size_t handled = perElementCount * N;

                Task* task = impl->taskPool.obtainTask();
                task->kernel = _kernel;
                task->parent = rootOffset;
                task->taskData.kernelData = _kernelData;
                task->taskData.specificData.streamingData.elementCount = _elementCount - handled;

                task->taskData.specificData.streamingData.inputStreams[0] =
                    static_cast<char*>(_is0.data) + N*_is0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[0] =
                    static_cast<char*>(_os0.data) + N*_os0.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[1] =
                    static_cast<char*>(_is1.data) + N*_is1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[1] =
                    static_cast<char*>(_os1.data) + N*_os1.elementStride*perElementCount;

                task->taskData.specificData.streamingData.inputStreams[2] =
                    static_cast<char*>(_is2.data) + N*_is2.elementStride*perElementCount;

                task->taskData.specificData.streamingData.outputStreams[2] =
                    static_cast<char*>(_os2.data) + N*_os2.elementStride*perElementCount;

                impl->queue.queueTask(task);
            }
        }

        return TaskId(rootOffset, root->generation);
    }

    void Scheduler::addChild(const TaskId& _parent, const TaskId& _child)
    {
        Task* parentTask = impl->taskPool.getTask(_parent.offset);
        parentTask->openTasks++;

        Task* childTask = impl->taskPool.getTask(_child.offset);
        childTask->parent = _parent.offset;
    }

    void Scheduler::runTask(const TaskId& _id)
    {
        Task* task = impl->taskPool.getTask(_id.offset);
        impl->queue.queueTask(task);
    }

    void Scheduler::wait(const TaskId& _taskId)
    {
        // wait until the task and all its children have completed
        while (!impl->taskPool.isTaskFinished(_taskId))
        {
            helpWithWork();
        }
    }

    void Scheduler::waitWithoutHelping(const TaskId& _taskId)
    {
        while (!impl->taskPool.isTaskFinished(_taskId))
        {
            std::this_thread::yield();
        }
    }

    void Scheduler::helpWithWork(void)
    {
        Task* task = impl->queue.getAvailableTask();
        if (task)
        {
            workOnTask(task);
        }
        else
        {
            std::this_thread::yield();
        }
    }

    void Scheduler::workOnTask(Task* _task)
    {
        while (!canExecuteTask(_task))
        {
            // the task cannot be executed at this time, work on another item
            helpWithWork();
        }

        // execute the kernel and finish the task
        if (_task->kernel)
        {
            (_task->kernel)(_task->taskData);
        }

        finishTask(_task);
    }

    void Scheduler::finishTask(Task* task)
    {
        const size_t openTasks = (--task->openTasks);

        if (openTasks == 0)
        {
            if (task->parent != Task::NO_PARENT)
            {
                // tell our parent that we're finished
                Task* parent = impl->taskPool.getTask(task->parent);
                finishTask(parent);
            }

            // this task has finished completely, remove it
            impl->taskPool.returnTask(task);
        }
    }

    size_t Scheduler::determineNumberOfTasks(size_t _elementCount, size_t _elementsPerTask)
    {
        size_t numberOftasks = _elementCount / _elementsPerTask;

        return numberOftasks != 0 ? numberOftasks : 1;
    }

    bool Scheduler::canExecuteTask(Task* _task)
    {
        if (_task->parent == Task::NO_PARENT)
        {
            return true;
        }

        return _task->openTasks == 1;
    }
}