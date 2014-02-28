#pragma once
#include <functional>
#include <atomic>

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

    struct TaskId
    {
        typedef int32_t Offset;
        inline TaskId(Offset _offset, int32_t _generation) : offset(_offset), generation(_generation) {}

        Offset offset;
        int32_t generation;
    };

    struct InputStream
    {
        inline InputStream(void* _data, uint32_t _stride) : data(_data), elementStride(_stride){}

        void* data;
        uint32_t elementStride;
    };

    struct OutputStream
    {
        inline OutputStream(void* _data, uint32_t _stride) : data(_data), elementStride(_stride) {}

        void* data;
        uint32_t elementStride;
    };


    struct TaskData
    {
        struct StreamingData
        {
            uint32_t elementCount;
            void* inputStreams[3];
            void* outputStreams[3];
        };

        union TaskSpecificData
        {
            StreamingData streamingData;
        };

        void* kernelData;
        TaskSpecificData specificData;
    };

    typedef std::function<void(const TaskData&)> Kernel;
    class Freelist;

    struct Task
    {
        static const TaskId::Offset NO_PARENT = -1;

        Task()
        {
            openTasks = 1;
            parent = Task::NO_PARENT;
        }
        Freelist* unusedFreelistAlias;
        std::atomic<int> openTasks;
        int32_t generation;
        TaskId::Offset parent;
        Kernel kernel;
        TaskData taskData;
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
            uint32_t elementCount, uint32_t _elementsPerTask);

        TaskId addStreamingTask(Kernel _kernel, void *_kernelData,
            InputStream _is0, OutputStream _os0,
            InputStream _is1, OutputStream _os1,
            uint32_t _elementCount, uint32_t _elementsPerTask);

        TaskId addStreamingTask(Kernel _kernel, void *_kernelData,
            InputStream _is0, OutputStream _os0,
            InputStream _is1, OutputStream _os1,
            InputStream _is2, OutputStream _os2,
            uint32_t _elementCount, uint32_t _elementsPerTask);

        void addChild(const TaskId& _parent, const TaskId& _child);
        void runTask(const TaskId& _id);

        void wait(const TaskId& _taskId);
        void waitWithoutHelping(const TaskId& _taskId);

        void workOnTask(Task* _task);

    private:
        void helpWithWork();
        void finishTask(Task* task);
        bool canExecuteTask(Task* _task);

        int determineNumberOfTasks(uint32_t _elementCount, uint32_t _elementsPerTask);

    private:
        Pimpl *_impl;
    };
}