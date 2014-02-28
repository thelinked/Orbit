#pragma once
#include <functional>
#include <atomic>

namespace orbit
{
    const char* ThreadName();

    struct TaskId
    {
        typedef size_t Offset;
        inline TaskId(Offset _offset, size_t _generation) : offset(_offset), generation(_generation) {}

        Offset offset;
        size_t generation;
    };

    struct InputStream
    {
        inline InputStream(void* _data, size_t _stride) : data(_data), elementStride(_stride){}

        void* data;
        size_t elementStride;
    };

    struct OutputStream
    {
        inline OutputStream(void* _data, size_t _stride) : data(_data), elementStride(_stride) {}

        void* data;
        size_t elementStride;
    };


    struct TaskData
    {
        struct StreamingData
        {
            size_t elementCount;
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
    class Task;
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

        int determineNumberOfTasks(uint32_t _elementCount, uint32_t _elementsPerTask);

    private:
        Pimpl *impl;
    };
}