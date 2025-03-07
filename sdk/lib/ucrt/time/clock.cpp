//
// clock.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The clock() function, which calculates the amount of elapsed time since the
// process started execution.
//
#include <corecrt_internal_time.h>
#include <sys/timeb.h>
#include <sys/types.h>



// The source frequency of the performance counter and the counter value at
// process startup, in the source frequency.
static long long source_frequency;
static long long start_count;



// Scales a 64-bit counter from the QueryPerformanceCounter frequency to the
// clock() frequency defined by CLOCKS_PER_SEC.  This is the same algorithm as
// is used internally by QueryPerformanceCounter to deal with frequency changes.
static long long scale_count(long long count)
{
    // Convert the count to seconds and multiply this by the destination frequency:
    long long scaled_count = (count / source_frequency) * CLOCKS_PER_SEC;

    // To reduce error introduced by scaling using integer division, separately
    // handle the remainder from the above division by multiplying the left-over
    // counter by the destination frequency, then diviting by the input frequency:
    count %= source_frequency;

    scaled_count += (count * CLOCKS_PER_SEC) / source_frequency;

    return scaled_count;
}



// This function initializes the global start_count variable when the CRT is
// initialized.  This initializer always runs in the CRT DLL; it only runs in
// the static CRT if this object file is linked into the module, which will only
// happen if some user code calls clock().
extern "C" int __cdecl __acrt_initialize_clock()
{
    LARGE_INTEGER local_frequency;
    LARGE_INTEGER local_start_count;
    if (!QueryPerformanceFrequency(&local_frequency) ||
        !QueryPerformanceCounter(&local_start_count) ||
        local_frequency.QuadPart == 0)
    {
        source_frequency = -1;
        start_count      = -1;
        return 0;
    }

    source_frequency = local_frequency.QuadPart;
    start_count      = local_start_count.QuadPart;

    return 0;
}

_CRT_LINKER_FORCE_INCLUDE(__acrt_clock_initializer);

// Calculates and returns the amount of elapsed time since the process started
// execution.  During CRT initialization, the 'start_count' global variable is
// initialized to the current tick count; subsequent calls to clock() get the
// new tick count and subtract the 'start_count' from it.
//
// The return value is the number of CLK_TCKs that have elapsed (milliseconds).
// On failure, -1 is returned.
extern "C" clock_t __cdecl clock()
{
    if (start_count == -1)
        return -1;

    LARGE_INTEGER current_count;
    if (!QueryPerformanceCounter(&current_count))
        return -1;

    long long const result = current_count.QuadPart - start_count;
    if (result < 0)
        return -1;

    long long const scaled_result = scale_count(result);

    // Per C11 7.27.2.1 ("The clock function")/3, "If the processor time used...
    // cannot be represented, the function returns the value (clock_t)(-1)."
    if (scaled_result > LONG_MAX)
        return -1;

    return static_cast<clock_t>(scaled_result);
}
