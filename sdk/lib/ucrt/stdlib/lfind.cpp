//
// lfind.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _lfind(), which performs a linear search over an array.
//
#include <corecrt_internal.h>
#include <search.h>

#ifdef _M_CEE
    #define __fileDECL __clrcall
#else
    #define __fileDECL __cdecl
#endif



#ifdef __USE_CONTEXT
    #define __COMPARE(context, p1, p2) (*compare)(context, p1, p2)
#else
    #define __COMPARE(context, p1, p2) (*compare)(p1, p2)
#endif

// Performs a linear search over the array, looking for the value 'key' in an
// array of 'num' elements of 'width' bytes in size.  Returns a pointer to the
// matching element if found.  Otherwise, returns nullptr.
//
// Parameters:
//  * key:   The key for which to search
//  * base:  A pointer to the initial element of the array to be searched
//  * num:   The number of elements in the array.
//  * width: The size of each element, in bytes.
//  * comp:  Pointer to a function returning analog of strcmp for strings, but
//           supplied by the caller for comparing the array elements.  It
//           accepts two pointers to elements; returns negative if 1 < 2;
//           zero if 1 == 2, and positive if 1 > 2.
#ifndef _M_CEE
extern "C"
#endif
#ifdef __USE_CONTEXT
void* __fileDECL _lfind_s(
    void const*   const key,
    void const*   const base,
    unsigned int* const num,
    size_t        const width,
    int (__fileDECL* const compare)(void*, void const*, void const*),
    void*         const context
    )
#else // __USE_CONTEXT
void* __fileDECL _lfind(
    void const*   const key,
    void const*   const base,
    unsigned int* const num,
    unsigned int  const width,
    int (__fileDECL* const compare)(void const*, void const*)
    )
#endif // __USE_CONTEXT
{
    _VALIDATE_RETURN(key != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(num != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(base != nullptr || *num == 0, EINVAL, nullptr);
    _VALIDATE_RETURN(width > 0, EINVAL, nullptr);
    _VALIDATE_RETURN(compare != nullptr, EINVAL, nullptr);

    char const* const first = static_cast<char const*>(base);
    char const* const last  = first + *num * width;

    for (char const* p = first; p != last; p += width)
    {
        if (__COMPARE(context, key, const_cast<char*>(p)) == 0)
        {
            return const_cast<char*>(p);
        }
    }

    return nullptr;
}

#undef __COMPARE
