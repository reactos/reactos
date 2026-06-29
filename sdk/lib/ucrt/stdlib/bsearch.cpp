//
// bsearch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines bsearch(), which performs a binary search over an array.
//
#include <corecrt_internal.h>
#include <search.h>

#ifdef _M_CEE
    #define __fileDECL __clrcall
#else
    #define __fileDECL __cdecl
#endif

/***
*char *bsearch() - do a binary search on an array
*
*Purpose:
*   Does a binary search of a sorted array for a key.
*
*Entry:
*   const char *key    - key to search for
*   const char *base   - base of sorted array to search
*   unsigned int num   - number of elements in array
*   unsigned int width - number of bytes per element
*   int (*compare)()   - pointer to function that compares two array
*           elements, returning neg when #1 < #2, pos when #1 > #2, and
*           0 when they are equal. Function is passed pointers to two
*           array elements.
*
*Exit:
*   if key is found:
*           returns pointer to occurrence of key in array
*   if key is not found:
*           returns nullptr
*
*Exceptions:
*   Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

#ifdef __USE_CONTEXT
    #define __COMPARE(context, p1, p2) (*compare)(context, p1, p2)
#else
    #define __COMPARE(context, p1, p2) (*compare)(p1, p2)
#endif

#ifndef _M_CEE
extern "C"
#endif

_CRT_SECURITYSAFECRITICAL_ATTRIBUTE
#ifdef __USE_CONTEXT
void* __fileDECL bsearch_s(
    void const* const key,
    void const* const base,
    size_t            num,
    size_t      const width,
    int (__fileDECL* const compare)(void*, void const*, void const*),
    void*       const context
    )
#else // __USE_CONTEXT
void* __fileDECL bsearch(
    void const* const key,
    void const* const base,
    size_t            num,
    size_t      const width,
    int (__fileDECL* const compare)(void const*, void const*)
    )
#endif // __USE_CONTEXT
{
    _VALIDATE_RETURN(base != nullptr || num == 0, EINVAL, nullptr);
    _VALIDATE_RETURN(width > 0, EINVAL, nullptr);
    _VALIDATE_RETURN(compare != nullptr, EINVAL, nullptr);

    char const* lo = reinterpret_cast<char const*>(base);
    char const* hi = reinterpret_cast<char const*>(base) + (num - 1) * width;

    // Reentrancy diligence: Save (and unset) global-state mode to the stack before making callout to 'compare'
    __crt_state_management::scoped_global_state_reset saved_state;

    // We allow a nullptr key here because it breaks some older code and because
    // we do not dereference this ourselves so we can't be sure that it's a
    // problem for the comparison function

    while (lo <= hi)
    {
        size_t const half = num / 2;
        if (half != 0)
        {
            char const* const mid = lo + (num & 1 ? half : (half - 1)) * width;

            int const result = __COMPARE(context, key, mid);
            if (result == 0)
            {
                return const_cast<void*>(static_cast<void const*>(mid));
            }
            else if (result < 0)
            {
                hi = mid - width;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo = mid + width;
                num = half;
            }
        }
        else if (num != 0)
        {
            return __COMPARE(context, key, lo)
                ? nullptr
                : const_cast<void*>(static_cast<void const*>(lo));
        }
        else
        {
            break;
        }
    }

    return nullptr;
}

#undef __COMPARE
