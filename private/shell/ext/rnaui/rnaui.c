//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnaui.c
//  Content:    This file contains the moudle initialization.
//  History:
//      Tue 30-Nov-1993 07:42:02  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"

//****************************************************************************
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//****************************************************************************

#define INITGUID
#include <initguid.h>
#include <coguid.h>
#include <oleguid.h>
#include <shlguid.h>
#include <shguidp.h>       // Remote CLSID

//****************************************************************************
// Constants
//****************************************************************************

//****************************************************************************
// Global Parameters
//****************************************************************************

HINSTANCE ghInstance      = NULL;

// we put the dll reference count and process count in the shared data
//
#pragma data_seg("SHAREDATA")

static int  s_cProcesses  = 0;
int         g_cRef        = 0;

CRITICAL_SECTION g_csRNA = { 0 };

#ifdef DEBUG
BOOL g_bExclusive = FALSE;
#endif

#pragma data_seg()

#ifdef DEBUG

UINT g_uBreakFlags = 0;         // Controls when to int 3
UINT g_uTraceFlags = 0;         // Controls what trace messages are spewed
UINT g_uDumpFlags = 0;          // Controls what structs get dumped

#endif



#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Enter a critical section
Returns: --
Cond:    --
*/
void PUBLIC RNA_EnterExclusive(void)
    {
    EnterCriticalSection(&g_csRNA);
    g_bExclusive = TRUE;
    }

/*----------------------------------------------------------
Purpose: Leave a critical section
Returns: --
Cond:    --
*/
void PUBLIC RNA_LeaveExclusive(void)
    {
    g_bExclusive = FALSE;
    LeaveCriticalSection(&g_csRNA);
    }
#endif



//****************************************************************************
// BOOL _Processattach (HINSTANCE)
//
// This function is called when a process is attached to the DLL
//
// History:
//  Mon 06-Sep-1993 09:20:10  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL _ProcessAttach(HINSTANCE hDll)
    {
    // It's okay to use a critical section in Chicago because (unlike
    //  NT) they work across processes.

    ReinitializeCriticalSection(&g_csRNA);
    ASSERT(0 != *((LPDWORD)&g_csRNA));

    ENTEREXCLUSIVE()
        {
        ghInstance = hDll;

        if (0 == s_cProcesses++)
            {
            // Do first-time stuff here
            }

#ifdef DEBUG

        // We do this simply to load the debug .ini flags
        ProcessIniFile();

        TRACE_MSG(TF_GENERAL, "Process Attach [%d] (hDll = %lx)", s_cProcesses, hDll);
        DEBUG_BREAK(BF_ONPROCESSATT);

#endif
        }
    LEAVEEXCLUSIVE()

    return TRUE;
    }

//****************************************************************************
// BOOL _ProcessDetach (HINSTANCE)
//
// This function is called when a process is detached from the DLL
//
// History:
//  Mon 06-Sep-1993 09:20:10  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL _ProcessDetach(HINSTANCE hDll)
    {
    ENTEREXCLUSIVE()
        {
        ASSERT(hDll == ghInstance);

#ifdef DEBUG

        TRACE_MSG(TF_GENERAL, "Process Detach [%d] (hDll = %lx)",
            s_cProcesses-1, hDll);

        DEBUG_BREAK(BF_ONPROCESSDET);

#endif

        --s_cProcesses;

        if (s_cProcesses == 0)
            {
            if (g_pidlRemote != NULL)
                {
                ILGlobalFree(g_pidlRemote);
                g_pidlRemote = NULL;
                };
            };
        }
    LEAVEEXCLUSIVE()

    if (0 == s_cProcesses)
        {
        // This use of g_csRNA is unprotected
        DeleteCriticalSection(&g_csRNA);
        }

    return TRUE;
    }

//****************************************************************************
// BOOL APIENTRY LibMain (HINSTANCE, DWORD, LPVOID)
//
// This function is called when the DLL is loaded
//
// History:
//  Mon 06-Sep-1993 09:20:10  -by-  Viroon  Touranachun [viroont]
// Ported from Shell.
//****************************************************************************

BOOL APIENTRY LibMain(HANDLE hDll, DWORD dwReason,  LPVOID lpReserved)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
        _ProcessAttach(hDll);
        break;

    case DLL_PROCESS_DETACH:
        _ProcessDetach(hDll);
        break;

    case DLL_THREAD_ATTACH:
#ifdef DEBUG

        // We do this simply to load the debug .ini flags
        //
        ProcessIniFile();

        TRACE_MSG(TF_GENERAL, "Thread Attach [%d] (hDll = %lx)",
                  s_cProcesses, hDll);

        DEBUG_BREAK(BF_ONTHREADATT);

#endif
        break;

    case DLL_THREAD_DETACH:

#ifdef DEBUG

        TRACE_MSG(TF_GENERAL, "Thread Detach [%d] (hDll = %lx)",
            s_cProcesses, hDll);

        DEBUG_BREAK(BF_ONTHREADDET);

#endif
        break;

    default:
        break;

  } // end switch()

  return TRUE;
}

//****************************************************************************
// STDAPI DllCanUnLoadNow()
//
// This function is called by shell
//
// History:
//  Tue 23-Feb-1993 14:12:21  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDAPI DllCanUnloadNow(void)
    {
    HRESULT hr;

    ENTEREXCLUSIVE()
        {
        if (0 == g_cRef)
            {
            DEBUG_CODE( TRACE_MSG(TF_GENERAL, "DllCanUnloadNow says OK (Ref=%d)",
                g_cRef); )

            hr = ResultFromScode(S_OK);
            }
        else
            {
            DEBUG_CODE( TRACE_MSG(TF_GENERAL, "DllCanUnloadNow says FALSE (Ref=%d)",
                g_cRef); )

            hr = ResultFromScode(S_FALSE);
            }
        }
    LEAVEEXCLUSIVE()

    return hr;
    }
