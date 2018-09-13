/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dllmain.cpp

Abstract:
    This module contains the DllMain entry point for winsock2 dll to
    control the global init and shutdown of the DLL.

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

[Environment:]

[Notes:]

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h

--*/

#include "precomp.h"
#pragma warning(disable: 4001)      /* Single-line comment */

#if defined(DEBUG_TRACING)
#include "dthook.h"
#endif // defined(DEBUG_TRACING)

DWORD gdwTlsIndex = TLS_OUT_OF_INDEXES;
HINSTANCE gDllHandle = NULL;


BOOL WINAPI DllMain(
    IN HINSTANCE hinstDll,
    IN DWORD fdwReason,
    LPVOID lpvReserved
    )
{
    switch (fdwReason) {

    case DLL_PROCESS_ATTACH:
        // DLL is attaching to the address
        // space of the current process.

        // Save DLL handle
        gDllHandle = hinstDll;

        gdwTlsIndex = TlsAlloc();
        if (gdwTlsIndex==TLS_OUT_OF_INDEXES) {
            return FALSE;
        }

        // Use private heap on MP machines to
        // avoid lock contention with other DLLs
        {
            SYSTEM_INFO sysInfo;
            GetSystemInfo (&sysInfo);

            if (sysInfo.dwNumberOfProcessors>1) {
                gHeap = HeapCreate (0, 0, 0);
                if (gHeap==NULL) {
                    gHeap = GetProcessHeap ();
                }
            }
            else
                gHeap = GetProcessHeap ();
        }


        {
            BOOLEAN startup = FALSE
#ifdef RASAUTODIAL
                    , autodial = FALSE
#endif
#if defined(DEBUG_TRACING)
                    , dthook = FALSE
#endif // defined(DEBUG_TRACING)
                    ;

                    

            __try {
                CreateStartupSynchronization();
                startup = TRUE;
#ifdef RASAUTODIAL
                InitializeAutodial();
                autodial = TRUE;
#endif // RASAUTODIAL

#if defined(DEBUG_TRACING)
                DTHookInitialize();
                dthook = TRUE;
#endif // defined(DEBUG_TRACING)

            }
            __except (WS2_EXCEPTION_FILTER ()) {
                goto cleanup;
            }
            if (!SockAsyncGlobalInitialize())
                goto cleanup;
            break;

        cleanup:
#if defined(DEBUG_TRACING)
            if (dthook) {
                DTHookShutdown();
            }
#endif // defined(DEBUG_TRACING)
#ifdef RASAUTODIAL
            if (autodial) {
                UninitializeAutodial();
            }
#endif // RASAUTODIAL
            if (startup) {
                DestroyStartupSynchronization();
            }

            gDllHandle = NULL;

            return FALSE;
        }

   case DLL_THREAD_ATTACH:
        // A new thread is being created in the current process.
        break;

   case DLL_THREAD_DETACH:
        // A thread is exiting cleanly.
        DTHREAD::DestroyCurrentThread();
        break;

   case DLL_PROCESS_DETACH:
        //
        // Check if we were initialized.
        //
        if (gDllHandle==NULL)
            break;

        // The calling process is detaching
        // the DLL from its address space.
        //
        // Note that lpvReserved will be NULL if the detach is due to
        // a FreeLibrary() call, and non-NULL if the detach is due to
        // process cleanup.
        //

        if( lpvReserved == NULL ) {
            PDPROCESS  CurrentProcess;

            // A thread is exiting cleanly (if we do not get a separate 
            // DLL_THREAD_DETACH).
            DTHREAD::DestroyCurrentThread();

            CurrentProcess = DPROCESS::GetCurrentDProcess();
            if (CurrentProcess!=NULL) {
                delete CurrentProcess;
            }

            DTHREAD::DThreadClassCleanup();
            DSOCKET::DSocketClassCleanup();
            SockAsyncGlobalTerminate();
            DestroyStartupSynchronization();
            if ((gHeap!=NULL) && (gHeap!=GetProcessHeap ())) {
                HeapDestroy (gHeap);
            }
            TlsFree (gdwTlsIndex);
        }

#if defined(DEBUG_TRACING)
        DTHookShutdown();
#endif // defined(DEBUG_TRACING)

#ifdef RASAUTODIAL
        UninitializeAutodial();
#endif // RASAUTODIAL

        //
        // Set the function prolog pointer to point to Prolog_Detached just
        // in case some lame-ass DLL trys to invoke one of our entrypoints
        // *after* we've been detached...
        //

        PrologPointer = &Prolog_Detached;

        gDllHandle = NULL;
        break;
    }


    return(TRUE);
}

