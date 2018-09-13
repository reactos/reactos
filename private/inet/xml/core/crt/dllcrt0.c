/***
*dllcrt0.c - C runtime initialization routine for a DLL with linked-in C R-T
*
* Copyright (c) 1989 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This the startup routine for a DLL which is linked with its own
*       C run-time code.  It is similar to the routine _mainCRTStartup()
*       in the file CRT0.C, except that there is no main() in a DLL.
*
*******************************************************************************/
#define _CRTBLD

#ifndef UNIX
#define NOGDI
#endif
#include <windows.h>
#include <w4warn.h>

CRITICAL_SECTION s_cs;

/**
 * External initialization
 */
EXTERN_C BOOL Runtime_init();
EXTERN_C void Runtime_exit();

extern void __cdecl _cinit();          /* crt0dat.c */
extern void __cdecl _cexit();

extern int _C_Termination_Done; /* termination done flag */

/*
 * flag set iff _CRTDLL_INIT was called with DLL_PROCESS_ATTACH
 */
static int __proc_attached = 0;

/*
 * User routine DllMain is called on all notifications
 */

extern BOOL WINAPI DllMain(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        ) ;

/* _pRawDllMain MUST be a common variable, not extern nor initialized! */

BOOL (WINAPI *_pRawDllMain)(HANDLE, DWORD, LPVOID);


/***
*BOOL WINAPI _CRT_INIT(hDllHandle, dwReason, lpreserved) -
*       C Run-Time initialization for a DLL linked with a C run-time library.
*
*Purpose:
*       This routine does the C run-time initialization or termination.
*       For the multi-threaded run-time library, it also cleans up the
*       multi-threading locks on DLL termination.
*
*Entry:
*
*Exit:
*
*NOTES:
*       This routine must be the entry point for the DLL.
*
*******************************************************************************/

extern HANDLE g_hProcessHeap;

CRITICAL_SECTION g_csHeap;

#ifdef PROFILE
#    include "pure.h"
#endif
#if SMALLBLOCKHEAP
int    __cdecl _set_sbh_threshold(size_t);
#endif

BOOL WINAPI _CRT_INIT(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
#ifdef PROFILE
    QuantifyStopRecordingData();
#endif
    /*
     * Start-up code only gets executed when the process is initialized
     */

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        /*
         * increment flag to indicate process attach notification
         * has been received
         */
        __proc_attached++;

        InitializeCriticalSection(&s_cs);

        //Do initialization here so that we can alloc memory in atexit
        g_hProcessHeap = GetProcessHeap();

        InitializeCriticalSection(&g_csHeap);

#if SMALLBLOCKHEAP
        _set_sbh_threshold(512 + 16);
#endif
        _cinit();               /* do C data initialize */

        if (!Runtime_init())
            return FALSE;
    }

    else if ( dwReason == DLL_PROCESS_DETACH )
    {
        Runtime_exit();

        if ( __proc_attached > 0 )
        {
            __proc_attached--;

            /*
             * Any basic clean-up code that goes here must be duplicated
             * below in _DllMainCRTStartup for the case where the user's
             * DllMain() routine fails on a Process Attach notification.
             * This does not include calling user C++ destructors, etc.
             */

            if ( _C_Termination_Done == FALSE )
            {
                _cexit();
                DeleteCriticalSection(&s_cs);

                DeleteCriticalSection(&g_csHeap);
            }

        }
        else
            /* no prior process attach, just return */
            return FALSE;
    }

    return TRUE ;
}

/***
*BOOL WINAPI _DllMainCRTStartup(hDllHandle, dwReason, lpreserved) -
*       C Run-Time initialization for a DLL linked with a C run-time library.
*
*Purpose:
*       This routine does the C run-time initialization or termination
*       and then calls the user code notification handler "DllMain".
*       For the multi-threaded run-time library, it also cleans up the
*       multi-threading locks on DLL termination.
*
*Entry:
*
*Exit:
*
*NOTES:
*       This routine is the preferred entry point. _CRT_INIT may also be
*       used, or the user may supply his/her own entry and call _CRT_INIT
*       from within it, but this is not the preferred method.
*
*******************************************************************************/

BOOL WINAPI _DllMainCRTStartup(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
    BOOL retcode = TRUE;

    /*
     * If this is a process attach notification, increment the process
     * attached flag. If this is a process detach notification, check
     * that there has been a prior process attach notification.
     */
    if ( dwReason == DLL_PROCESS_ATTACH )
        __proc_attached++;
    else if ( dwReason == DLL_PROCESS_DETACH ) {
        if ( __proc_attached > 0 )
            __proc_attached--;
        else
            /*
             * no prior process attach notification. just return
             * without doing anything.
             */
            return FALSE;
    }

    if ( dwReason == DLL_PROCESS_ATTACH || dwReason == DLL_THREAD_ATTACH )
    {
        if ( _pRawDllMain )
            retcode = (*_pRawDllMain)(hDllHandle, dwReason, lpreserved);

        if ( retcode )
            retcode = _CRT_INIT(hDllHandle, dwReason, lpreserved);
    }

    if ( retcode )
        retcode = DllMain(hDllHandle, dwReason, lpreserved);

    if ( dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH )
    {
        if ( _CRT_INIT(hDllHandle, dwReason, lpreserved) == FALSE )
            retcode = FALSE ;

        if ( retcode && _pRawDllMain )
            retcode = (*_pRawDllMain)(hDllHandle, dwReason, lpreserved);
    }

    return retcode ;
}



