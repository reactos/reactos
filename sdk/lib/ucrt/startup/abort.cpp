/***
*abort.c - abort a program by raising SIGABRT
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines abort() - print a message and raise SIGABRT.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <signal.h>
#include <stdlib.h>

#ifdef _DEBUG
    #define _INIT_ABORT_BEHAVIOR _WRITE_ABORT_MSG
#else
    #define _INIT_ABORT_BEHAVIOR _CALL_REPORTFAULT
#endif

extern "C" { unsigned int __abort_behavior = _INIT_ABORT_BEHAVIOR; }

/***
*void abort() - abort the current program by raising SIGABRT
*
*Purpose:
*   print out an abort message and raise the SIGABRT signal.  If the user
*   hasn't defined an abort handler routine, terminate the program
*   with exit status of 3 without cleaning up.
*
*   Multi-thread version does not raise SIGABRT -- this isn't supported
*   under multi-thread.
*
*Entry:
*   None.
*
*Exit:
*   Does not return.
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl abort()
{
    #ifdef _DEBUG
    if (__abort_behavior & _WRITE_ABORT_MSG)
    {
        __acrt_report_runtime_error(L"abort() has been called");
    }
    #endif


    /* Check if the user installed a handler for SIGABRT.
     * We need to read the user handler atomically in the case
     * another thread is aborting while we change the signal
     * handler.
     */
    __crt_signal_handler_t const sigabrt_action = __acrt_get_sigabrt_handler();
    if (sigabrt_action != SIG_DFL)
    {
        raise(SIGABRT);
    }

    /* If there is no user handler for SIGABRT or if the user
     * handler returns, then exit from the program anyway
     */

    if (__abort_behavior & _CALL_REPORTFAULT)
    {
        #if defined _M_ARM || defined _M_ARM64 || defined _M_ARM64EC || defined _UCRT_ENCLAVE_BUILD
        __fastfail(FAST_FAIL_FATAL_APP_EXIT);
        #else
        if (IsProcessorFeaturePresent(PF_FASTFAIL_AVAILABLE))
            __fastfail(FAST_FAIL_FATAL_APP_EXIT);

        __acrt_call_reportfault(_CRT_DEBUGGER_ABORT, STATUS_FATAL_APP_EXIT, EXCEPTION_NONCONTINUABLE);
        #endif
    }


    /* If we don't want to call ReportFault, then we call _exit(3), which is the
     * same as invoking the default handler for SIGABRT
     */


    _exit(3);
}

/***
*unsigned int _set_abort_behavior(unsigned int, unsigned int) - set the behavior on abort
*
*Purpose:
*
*Entry:
*   unsigned int flags - the flags we want to set
*   unsigned int mask - mask the flag values
*
*Exit:
*   Return the old behavior flags
*
*Exceptions:
*   None
*
*******************************************************************************/

extern "C" unsigned int __cdecl _set_abort_behavior(unsigned int flags, unsigned int mask)
{
    unsigned int oldflags = __abort_behavior;
    __abort_behavior = oldflags & (~mask) | flags & mask;
    return oldflags;
}
