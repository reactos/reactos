/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * Clang C++ global allocation operators for the ReactOS C++ runtime.
 */

#include <new>
#include <stdlib.h>

static new_handler CurrentNewHandler;

new_handler
set_new_handler(new_handler Handler) throw()
{
    new_handler PreviousHandler = CurrentNewHandler;

    CurrentNewHandler = Handler;
    return PreviousHandler;
}

void*
operator new(std::size_t Size)
{
    if (Size == 0)
        Size = 1;

    for (;;)
    {
        void *Pointer = malloc(Size);

        if (Pointer != NULL)
            return Pointer;

        if (CurrentNewHandler == NULL)
            throw std::bad_alloc();

        CurrentNewHandler();
    }
}

void*
operator new[](std::size_t Size)
{
    return operator new(Size);
}

void*
operator new(std::size_t Size, const std::nothrow_t&) throw()
{
    try
    {
        return operator new(Size);
    }
    catch (std::bad_alloc)
    {
        return NULL;
    }
}

void*
operator new[](std::size_t Size, const std::nothrow_t&) throw()
{
    try
    {
        return operator new[](Size);
    }
    catch (std::bad_alloc)
    {
        return NULL;
    }
}

void
operator delete(void *Pointer) throw()
{
    free(Pointer);
}

void
operator delete[](void *Pointer) throw()
{
    operator delete(Pointer);
}

void
operator delete(void *Pointer, std::size_t) throw()
{
    operator delete(Pointer);
}

void
operator delete[](void *Pointer, std::size_t) throw()
{
    operator delete[](Pointer);
}

void
operator delete(void *Pointer, const std::nothrow_t&) throw()
{
    operator delete(Pointer);
}

void
operator delete[](void *Pointer, const std::nothrow_t&) throw()
{
    operator delete[](Pointer);
}
