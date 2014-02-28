#pragma once
#include <functional>
#include <memory>
#include <atomic>
#include "FreeList.hpp"

namespace orbit
{
    namespace configuration
    {
        static const unsigned int MAX_WORKER_THREAD_COUNT = 16;
        static const unsigned int MAX_TASK_COUNT = 4096;
    }
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

    struct Task
    {
        static const TaskId::Offset NO_PARENT = -1;

        Task()
        {
            openTasks = 1;
            parent = Task::NO_PARENT;
        }
        Freelist* unusedFreelistAlias;
        std::atomic<size_t> openTasks;
        int32_t generation;
        TaskId::Offset parent;
        Kernel kernel;
        TaskData taskData;
    };
}