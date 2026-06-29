//
// lsearch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _lsearch(), which performs a linear search over an array, appending
// the key to the end of the array if it is not found.
//
#include <corecrt_internal.h>
#include <memory.h>
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
// matching element if found.  Otherwise, adds a new element to the end of the
// array, copied from 'key'.
//
// Parameters:
//  * key:   The key for which to search
//  * base:  A pointer to the initial element of the array to be searched
//  * num:   A pointer to an integer containing the number of elements in the
//           array.  If a new element is appended, this function increments the
//           pointed-to value.
//  * width: The size of each element, in bytes.
//  * comp:  Pointer to a function returning analog of strcmp for strings, but
//           supplied by the caller for comparing the array elements.  It
//           accepts two pointers to elements; returns negative if 1 < 2;
//           zero if 1 == 2, and positive if 1 > 2.
#ifndef _M_CEE
extern "C"
#endif
#ifdef __USE_CONTEXT
void* __fileDECL _lsearch_s(
    void const*   const key,
    void*         const base,
    unsigned int* const num,
    size_t        const width,
    int (__fileDECL* compare)(void*, void const*, void const*),
    void*         const context
    )
#else // __USE_CONTEXT
void* __fileDECL _lsearch(
    const void*   const key,
    void*         const base,
    unsigned int* const num,
    unsigned int  const width,
    int (__fileDECL* compare)(void const*, void const*)
    )
#endif // __USE_CONTEXT
{
    _VALIDATE_RETURN(key != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(num != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(base != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(width > 0, EINVAL, nullptr);
    _VALIDATE_RETURN(compare != nullptr, EINVAL, nullptr);

    char* const first = static_cast<char*>(base);
    char* const last  = first + *num * width;

    // Reentrancy diligence: Save (and unset) global-state mode to the stack before making callout to 'compare'
    __crt_state_management::scoped_global_state_reset saved_state;

    for (char* p = first; p != last; p += width)
    {
        if (__COMPARE(context, key, p) == 0)
        {
            return p;
        }
    }

    memcpy(last, key, width);
    ++(*num);
    return last;
}

#undef __COMPARE
