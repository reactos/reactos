//
// qsort.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines qsort(), a routine for sorting arrays.
//
#include <corecrt_internal.h>
#include <search.h>


/* Temporarily define optimization macros (to be removed by the build team: RsmqblCompiler alias) */
#if !defined(BEGIN_PRAGMA_OPTIMIZE_DISABLE)
#define BEGIN_PRAGMA_OPTIMIZE_DISABLE(flags, bug, reason) \
    __pragma(optimize(flags, off))
#define BEGIN_PRAGMA_OPTIMIZE_ENABLE(flags, bug, reason) \
    __pragma(optimize(flags, on))
#define END_PRAGMA_OPTIMIZE() \
    __pragma(optimize("", on))
#endif


// Always compile this module for speed, not size
BEGIN_PRAGMA_OPTIMIZE_ENABLE("t", MSFT:4499497, "This file is performance-critical and should always be optimized for speed")



#ifdef _M_CEE
    #define __fileDECL __clrcall
#else
    #define __fileDECL __cdecl
#endif



#ifdef __USE_CONTEXT
    #define __COMPARE(context, p1, p2)                comp(context, p1, p2)
    #define __SHORTSORT(lo, hi, width, comp, context) shortsort_s(lo, hi, width, comp, context);
#else
    #define __COMPARE(context, p1, p2)                comp(p1, p2)
    #define __SHORTSORT(lo, hi, width, comp, context) shortsort(lo, hi, width, comp);
#endif



// Swaps the objects of size 'width' that are pointed to by 'a' and 'b'
#ifndef _QSORT_SWAP_DEFINED
#define _QSORT_SWAP_DEFINED
_CRT_SECURITYSAFECRITICAL_ATTRIBUTE
static void __fileDECL swap(_Inout_updates_(width) char* a, _Inout_updates_(width) char* b, size_t width)
{
    if (a != b)
    {
        // Do the swap one character at a time to avoid potential alignment
        // problems:
        while (width--)
        {
            char const tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}
#endif // _QSORT_SWAP_DEFINED



// An insertion sort for sorting short arrays.  Sorts the sub-array of elements
// between lo and hi (inclusive).  Assumes lo < hi.  lo and hi are pointers to
// the first and last elements in the range to be sorted (note:  hi does not
// point one-past-the-end).  The comp is a comparer with the same behavior as
// specified for qsort.
_CRT_SECURITYSAFECRITICAL_ATTRIBUTE
#ifdef __USE_CONTEXT
static void __fileDECL shortsort_s(
    _Inout_updates_(hi - lo + 1) char*        lo,
    _Inout_updates_(width)       char*        hi,
    size_t const width,
    int (__fileDECL* comp)(void*, void const*, void const*),
    void*  const context
    )
#else // __USE_CONTEXT
static void __fileDECL shortsort(
    _Inout_updates_(hi - lo + 1) char*        lo,
    _Inout_updates_(width)       char*        hi,
    size_t const width,
    int (__fileDECL* comp)(void const*, void const*)
    )
#endif // __USE_CONTEXT
{
    // Note: in assertions below, i and j are alway inside original bound of
    // array to sort.

    // Reentrancy diligence: Save (and unset) global-state mode to the stack before making callout to 'compare'
    __crt_state_management::scoped_global_state_reset saved_state;

    while (hi > lo)
    {
        // A[i] <= A[j] for i <= j, j > hi
        char* max = lo;
        for (char* p = lo+width; p <= hi; p += width)
        {
            // A[i] <= A[max] for lo <= i < p
            if (__COMPARE(context, p, max) > 0)
            {
                max = p;
            }
            // A[i] <= A[max] for lo <= i <= p
        }

        // A[i] <= A[max] for lo <= i <= hi

        swap(max, hi, width);

        // A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi

        hi -= width;

        // A[i] <= A[j] for i <= j, j > hi, loop top condition established
    }

    // A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
    // so array is sorted
}



// This macro defines the cutoff between using QuickSort and insertion sort for
// arrays; arrays with lengths shorter or equal to the below value use insertion
// sort.
#define CUTOFF 8 // Testing shows that this is a good value.

// Note:  The theoretical number of stack entries required is no more than 1 +
// log2(num).  But we switch to insertion sort for CUTOFF elements or less, so
// we really only need 1 + log2(num) - log(CUTOFF) stack entries.  For a CUTOFF
// of 8, that means we need no more than 30 stack entries for 32-bit platforms
// and 62 for 64-bit platforms.
#define STKSIZ (8 * sizeof(void*) - 2)



// QuickSort function for sorting arrays.  The array is sorted in place.
// Parameters:
//  * base:  Pointer to the initial element of the array
//  * num:   Number of elements in the array
//  * width: Width of each element in the array, in bytes
//  * comp:  Pointer to a function returning analog of strcmp for strings, but
//           supplied by the caller for comparing the array elements.  It
//           accepts two pointers to elements; returns negative if 1 < 2;
//           zero if 1 == 2, and positive if 1 > 2.
#ifndef _M_CEE
extern "C"
#endif
_CRT_SECURITYSAFECRITICAL_ATTRIBUTE
#ifdef __USE_CONTEXT
void __fileDECL qsort_s(
    void*  const base,
    size_t const num,
    size_t const width,
    int (__fileDECL* const comp)(void*, void const*, void const*),
    void*  const context
    )
#else // __USE_CONTEXT
void __fileDECL qsort(
    void*  const base,
    size_t const num,
    size_t const width,
    int (__fileDECL* const comp)(void const*, void const*)
    )
#endif // __USE_CONTEXT
{
    _VALIDATE_RETURN_VOID(base != nullptr || num == 0, EINVAL);
    _VALIDATE_RETURN_VOID(width > 0, EINVAL);
    _VALIDATE_RETURN_VOID(comp != nullptr, EINVAL);

    // A stack for saving the sub-arrays yet to be processed:
    char* lostk[STKSIZ];
    char* histk[STKSIZ];
    int stkptr = 0;

    if (num < 2)
        return; // Nothing to do:

    // The ends of the sub-array currently being sorted (note that 'hi' points
    // to the last element, not one-past-the-end):
    char* lo = static_cast<char*>(base);
    char* hi = static_cast<char*>(base) + width * (num-1);

    // This entry point is for pseudo-recursion calling: setting
    // lo and hi and jumping to here is like recursion, but stkptr is
    // preserved, locals aren't, so we preserve stuff on the stack.
recurse:

    // The number of elements in the sub-array currently being sorted:
    size_t const size = (hi - lo) / width + 1;

    // Below a certain size, it is faster to use a O(n^2) sorting method:
    if (size <= CUTOFF)
    {
        __SHORTSORT(lo, hi, width, comp, context);
    }
    else
    {
        // First we pick a partitioning element.  The efficiency of the
        // algorithm demands that we find one that is approximately the median
        // of the values, but also that we select one fast.  We choose the
        // median of the first, middle, and last elements, to avoid bad
        // performance in the face of already sorted data, or data that is made
        // up of multiple sorted runs appended together.  Testing shows that a
        // median-of-three algorithm provides better performance than simply
        // picking the middle element for the latter case.

        // Find the middle element:
        char* mid = lo + (size / 2) * width;

        // Sort the first, middle, last elements into order:
        if (__COMPARE(context, lo, mid) > 0)
            swap(lo, mid, width);

        if (__COMPARE(context, lo, hi) > 0)
            swap(lo, hi, width);

        if (__COMPARE(context, mid, hi) > 0)
            swap(mid, hi, width);

        // We now wish to partition the array into three pieces, one consisting
        // of elements <= partition element, one of elements equal to the
        // partition element, and one of elements > than it.  This is done
        // below; comments indicate conditions established at every step.

        char* loguy = lo;
        char* higuy = hi;

        // Note that higuy decreases and loguy increases on every iteration,
        // so loop must terminate.
        for (;;)
        {
            // lo <= loguy < hi, lo < higuy <= hi,
            // A[i] <= A[mid] for lo <= i <= loguy,
            // A[i] > A[mid] for higuy <= i < hi,
            // A[hi] >= A[mid]

            // The doubled loop is to avoid calling comp(mid,mid), since some
            // existing comparison funcs don't work when passed the same
            // value for both pointers.

            if (mid > loguy)
            {
                do
                {
                    loguy += width;
                }
                while (loguy < mid && __COMPARE(context, loguy, mid) <= 0);
            }
            if (mid <= loguy)
            {
                do
                {
                    loguy += width;
                }
                while (loguy <= hi && __COMPARE(context, loguy, mid) <= 0);
            }

            // lo < loguy <= hi+1, A[i] <= A[mid] for lo <= i < loguy,
            // either loguy > hi or A[loguy] > A[mid]

            do
            {
                higuy -= width;
            }
            while (higuy > mid && __COMPARE(context, higuy, mid) > 0);

            // lo <= higuy < hi, A[i] > A[mid] for higuy < i < hi,
            // either higuy == lo or A[higuy] <= A[mid]

            if (higuy < loguy)
                break;

            // if loguy > hi or higuy == lo, then we would have exited, so
            // A[loguy] > A[mid], A[higuy] <= A[mid],
            // loguy <= hi, higuy > lo

            swap(loguy, higuy, width);

            // If the partition element was moved, follow it.  Only need
            // to check for mid == higuy, since before the swap,
            // A[loguy] > A[mid] implies loguy != mid.

            if (mid == higuy)
                mid = loguy;

            // A[loguy] <= A[mid], A[higuy] > A[mid]; so condition at top
            // of loop is re-established
        }

        //     A[i] <= A[mid] for lo <= i < loguy,
        //     A[i] > A[mid] for higuy < i < hi,
        //     A[hi] >= A[mid]
        //     higuy < loguy
        // implying:
        //     higuy == loguy-1
        //     or higuy == hi - 1, loguy == hi + 1, A[hi] == A[mid]

        // Find adjacent elements equal to the partition element.  The
        // doubled loop is to avoid calling comp(mid,mid), since some
        // existing comparison funcs don't work when passed the same value
        // for both pointers.

        higuy += width;
        if (mid < higuy)
        {
            do
            {
                higuy -= width;
            }
            while (higuy > mid && __COMPARE(context, higuy, mid) == 0);
        }
        if (mid >= higuy)
        {
            do
            {
                higuy -= width;
            }
            while (higuy > lo && __COMPARE(context, higuy, mid) == 0);
        }

        // OK, now we have the following:
        //    higuy < loguy
        //    lo <= higuy <= hi
        //    A[i]  <= A[mid] for lo <= i <= higuy
        //    A[i]  == A[mid] for higuy < i < loguy
        //    A[i]  >  A[mid] for loguy <= i < hi
        //    A[hi] >= A[mid] */

        // We've finished the partition, now we want to sort the subarrays
        // [lo, higuy] and [loguy, hi].
        // We do the smaller one first to minimize stack usage.
        // We only sort arrays of length 2 or more.

        if (higuy - lo >= hi - loguy)
        {
            if (lo < higuy)
            {
                // Save the big recursion for later:
                lostk[stkptr] = lo;
                histk[stkptr] = higuy;
                ++stkptr;
            }

            if (loguy < hi)
            {
                // Do the small recursion:
                lo = loguy;
                goto recurse;
            }
        }
        else
        {
            if (loguy < hi)
            {
                // Save the big recursion for later:
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;
            }

            if (lo < higuy)
            {
                // Do the small recursion:
                hi = higuy;
                goto recurse;
            }
        }
    }

    // We have sorted the array, except for any pending sorts on the stack.
    // Check if there are any, and sort them:
    --stkptr;
    if (stkptr >= 0)
    {
        // Pop sub-array from the stack:
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;
    }
    else
    {
        // Otherwise, all sub-arrays have been sorted:
        return;
    }
}

#undef __COMPARE
#undef __SHORTSORT

END_PRAGMA_OPTIMIZE()
