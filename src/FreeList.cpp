#include "FreeList.hpp"
namespace orbit
{
    Freelist::Freelist(void* _start, void* _end, size_t _stride) : next(nullptr)
    {
        union
        {
            void* as_void;
            char* as_char;
            Self* as_self;
        };

        as_void = _start;
        next = as_self;

        const size_t numElements = (static_cast<char*>(_end) -as_char) / _stride;

        as_char += _stride;

        // initialize the free list - make every m_next at each element point to the next element in the list
        Self* runner = next;
        for (size_t i = 1; i < numElements; ++i)
        {
            runner->next = as_self;
            runner = runner->next;

            as_char += _stride;
        }

        runner->next = nullptr;
    }

    void* Freelist::Obtain()
    {
        // obtain one element from the head of the free list
        Self* head = next;
        next = head->next;
        return head;
    }

    void Freelist::Return(void* _ptr)
    {
        // put the returned element at the head of the free list
        Self* head = static_cast<Self*>(_ptr);
        head->next = next;
        next = head;
    }
}