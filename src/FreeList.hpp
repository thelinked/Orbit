#pragma once

namespace orbit
{
    class Freelist
    {
        typedef Freelist Self;

    public:
        Freelist(void* _start, void* _end, size_t stride);
        void* Obtain();
        void Return(void* _ptr);

    private:
        Self* next;
    };
}