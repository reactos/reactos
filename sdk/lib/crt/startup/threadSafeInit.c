/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     CC0-1.0 (https://spdx.org/licenses/CC0-1.0)
 * PURPOSE:     Thread safe initialization support routines for MSVC and Clang-CL
 * COPYRIGHT:   Copyright 2019 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <intrin.h>

int _stdcall SwitchToThread(void);

unsigned int _tls_array;
unsigned int _tls_index;
long _Init_global_epoch;
long _Init_thread_epoch;

/*
    This function tries to acquire a lock on the initialization for the static
    variable by changing the value saved in *ptss to -1. If *ptss is 0, the
    variable was not initialized yet and the function tries to set it to -1.
    If that succeeds, the function will return. If the value is already -1,
    another thread is in the process of doing the initialization and we
    wait for it. If it is any other value the initialization is complete.
    After returning the compiler generated code will check the value:
    if it is -1 it will continue with the initialization, otherwise the
    initialization must be complete and will be skipped.
*/
void
_Init_thread_header(volatile int* ptss)
{
    while (1)
    {
        /* Try to acquire the first initialization lock */
        int oldTss = _InterlockedCompareExchange((long*)ptss, -1, 0);
        if (oldTss == -1)
        {
            /* Busy, wait for the other thread to do the initialization */
            SwitchToThread();
            continue;
        }

        /* Either we acquired the lock and the caller will do the initializaion
           or the initialization is complete and the caller will skip it */
        break;
    }
}

void
_Init_thread_footer(volatile int* ptss)
{
    /* Initialization is complete */
    *ptss = _InterlockedIncrement(&_Init_global_epoch);
}

void
_Init_thread_abort(volatile int* ptss)
{
    /* Abort the initialization */
    _InterlockedAnd((volatile long*)ptss, 0);
}
