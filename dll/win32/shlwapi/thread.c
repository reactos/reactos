/*
 * SHLWAPI thread and MT synchronisation functions
 *
 * Copyright 2002 Juergen Schmied
 * Copyright 2002 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_USER
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Get a function pointer from a DLL handle */
#define GET_FUNC(func, module, name, fail) \
  do { \
    if (!func) { \
      if (!SHLWAPI_h##module && !(SHLWAPI_h##module = LoadLibraryA(#module ".dll"))) return fail; \
      if (!(func = (void*)GetProcAddress(SHLWAPI_h##module, name))) return fail; \
    } \
  } while (0)

/* DLL handles for late bound calls */
extern HMODULE SHLWAPI_hshell32;

/* Function pointers for GET_FUNC macro; these need to be global because of gcc bug */
static HRESULT (WINAPI *pSHGetInstanceExplorer)(IUnknown**);

extern DWORD SHLWAPI_ThreadRef_index;  /* Initialised in shlwapi_main.c */

DWORD WINAPI SHStringFromGUIDA(REFGUID,LPSTR,INT);

/**************************************************************************
 *      _CreateAllAccessSecurityAttributes       [SHLWAPI.356]
 *
 * Initialise security attributes from a security descriptor.
 *
 * PARAMS
 *  lpAttr [O] Security attributes
 *  lpSec  [I] Security descriptor
 *
 * RETURNS
 *  Success: lpAttr, initialised using lpSec.
 *  Failure: NULL, if any parameters are invalid.
 *
 * NOTES
 *  This function always returns NULL if the underlying OS version
 *  Wine is impersonating does not use security descriptors (i.e. anything
 *  before Windows NT).
 */
LPSECURITY_ATTRIBUTES WINAPI _CreateAllAccessSecurityAttributes(
	LPSECURITY_ATTRIBUTES lpAttr,
	PSECURITY_DESCRIPTOR lpSec,
        DWORD p3)
{
  /* This function is used within SHLWAPI only to create security attributes
   * for shell semaphores. */

  TRACE("(%p,%p,%08lx)\n", lpAttr, lpSec, p3);

  if (!(GetVersion() & 0x80000000))  /* NT */
  {
    if (!lpSec || !lpAttr)
      return NULL;

    if (InitializeSecurityDescriptor(lpSec, 1))
    {
      if (SetSecurityDescriptorDacl(lpSec, TRUE, NULL, FALSE))
      {
         lpAttr->nLength = sizeof(SECURITY_ATTRIBUTES);
         lpAttr->lpSecurityDescriptor = lpSec;
         lpAttr->bInheritHandle = FALSE;
         return lpAttr;
      }
    }
  }
  return NULL;
}

/*************************************************************************
 *      _SHGetInstanceExplorer	[SHLWAPI.@]
 *
 * Get an interface to the shell explorer.
 *
 * PARAMS
 *  lppUnknown [O] Destination for explorers IUnknown interface.
 *
 * RETURNS
 *  Success: S_OK. lppUnknown contains the explorer interface.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI _SHGetInstanceExplorer(IUnknown **lppUnknown)
{
  /* This function is used within SHLWAPI only to hold the IE reference
   * for threads created with the CTF_PROCESS_REF flag set. */

  GET_FUNC(pSHGetInstanceExplorer, shell32, "SHGetInstanceExplorer", E_FAIL);
  return pSHGetInstanceExplorer(lppUnknown);
}

/* Internal thread information structure */
typedef struct tagSHLWAPI_THREAD_INFO
{
  LPTHREAD_START_ROUTINE pfnThreadProc; /* Thread start */
  LPTHREAD_START_ROUTINE pfnCallback;   /* Thread initialisation */
  PVOID     pData;                      /* Application specific data */
  BOOL      bInitCom;                   /* Initialise COM for the thread? */
  HANDLE    hEvent;                     /* Signal for creator to continue */
  IUnknown *refThread;                  /* Reference to thread creator */
  IUnknown *refIE;                      /* Reference to the IE process */
} SHLWAPI_THREAD_INFO, *LPSHLWAPI_THREAD_INFO;


/*************************************************************************
 * SHGetThreadRef	[SHLWAPI.@]
 *
 * Get a per-thread object reference set by SHSetThreadRef().
 *
 * PARAMS
 *   lppUnknown [O] Destination to receive object reference
 *
 * RETURNS
 *   Success: S_OK. lppUnknown is set to the object reference.
 *   Failure: E_NOINTERFACE, if an error occurs or lppUnknown is NULL.
 */
HRESULT WINAPI SHGetThreadRef(IUnknown **lppUnknown)
{
  TRACE("(%p)\n", lppUnknown);

  if (!lppUnknown || SHLWAPI_ThreadRef_index == TLS_OUT_OF_INDEXES)
    return E_NOINTERFACE;

  *lppUnknown = (IUnknown*)TlsGetValue(SHLWAPI_ThreadRef_index);
  if (!*lppUnknown)
    return E_NOINTERFACE;

  /* Add a reference. Caller will Release() us when finished */
  IUnknown_AddRef(*lppUnknown);
  return S_OK;
}

/*************************************************************************
 * SHSetThreadRef	[SHLWAPI.@]
 *
 * Store a per-thread object reference.
 *
 * PARAMS
 *   lpUnknown [I] Object reference to store
 *
 * RETURNS
 *   Success: S_OK. lpUnknown is stored and can be retrieved by SHGetThreadRef()
 *   Failure: E_NOINTERFACE, if an error occurs or lpUnknown is NULL.
 */
HRESULT WINAPI SHSetThreadRef(IUnknown *lpUnknown)
{
  TRACE("(%p)\n", lpUnknown);

  if (!lpUnknown || SHLWAPI_ThreadRef_index  == 0xffffffff)
    return E_NOINTERFACE;

  TlsSetValue(SHLWAPI_ThreadRef_index, lpUnknown);
  return S_OK;
}

/*************************************************************************
 * SHReleaseThreadRef	[SHLWAPI.@]
 *
 * Release a per-thread object reference.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *   Success: S_OK. The threads object reference is released.
 *   Failure: An HRESULT error code.
 */
HRESULT WINAPI SHReleaseThreadRef()
{
  FIXME("() - stub!\n");
  return S_OK;
}

/*************************************************************************
 * SHLWAPI_ThreadWrapper
 *
 * Internal wrapper for executing user thread functions from SHCreateThread.
 */
static DWORD WINAPI SHLWAPI_ThreadWrapper(PVOID pTi)
{
  SHLWAPI_THREAD_INFO ti;
  HRESULT hCom = E_FAIL;
  DWORD dwRet;

  TRACE("(%p)\n", pTi);

  /* We are now executing in the context of the newly created thread.
   * So we copy the data passed to us (it is on the stack of the function
   * that called us, which is waiting for us to signal an event before
   * returning). */
  memcpy(&ti, pTi, sizeof(SHLWAPI_THREAD_INFO));

  /* Initialise COM for the thread, if desired */
  if (ti.bInitCom)
  {
    hCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);

    if (FAILED(hCom))
      hCom = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE);
  }

  /* Execute the callback function before returning */
  if (ti.pfnCallback)
    ti.pfnCallback(ti.pData);

  /* Signal the thread that created us; it can return now */
  SetEvent(ti.hEvent);

  /* Execute the callers start code */
  dwRet = ti.pfnThreadProc(ti.pData);

  /* Release references to the caller and IE process, if held */
  if (ti.refThread)
    IUnknown_Release(ti.refThread);

  if (ti.refIE)
    IUnknown_Release(ti.refIE);

  if (SUCCEEDED(hCom))
    CoUninitialize();

  /* Return the users thread return value */
  return dwRet;
}

/*************************************************************************
 *      SHCreateThread	[SHLWAPI.16]
 *
 * Create a new thread.
 *
 * PARAMS
 *   pfnThreadProc [I] Function to execute in new thread
 *   pData         [I] Application specific data passed to pfnThreadProc
 *   dwFlags       [I] CTF_ flags from "shlwapi.h"
 *   pfnCallback   [I] Function to execute before pfnThreadProc
 *
 * RETURNS
 *   Success: TRUE. pfnThreadProc was executed.
 *   Failure: FALSE. pfnThreadProc was not executed.
 *
 * NOTES
 *   If the thread cannot be created, pfnCallback is NULL, and dwFlags
 *   has bit CTF_INSIST set, pfnThreadProc will be executed synchronously.
 */
BOOL WINAPI SHCreateThread(LPTHREAD_START_ROUTINE pfnThreadProc, VOID *pData,
                           DWORD dwFlags, LPTHREAD_START_ROUTINE pfnCallback)
{
  SHLWAPI_THREAD_INFO ti;
  BOOL bCalled = FALSE;

  TRACE("(%p,%p,0x%lX,%p)\n", pfnThreadProc, pData, dwFlags, pfnCallback);

  /* Set up data to pass to the new thread (On our stack) */
  ti.pfnThreadProc = pfnThreadProc;
  ti.pfnCallback = pfnCallback;
  ti.pData = pData;
  ti.bInitCom = dwFlags & CTF_COINIT ? TRUE : FALSE;
  ti.hEvent = CreateEventW(NULL,FALSE,FALSE,NULL);

  /* Hold references to the current thread and IE process, if desired */
  if(dwFlags & CTF_THREAD_REF)
    SHGetThreadRef(&ti.refThread);
  else
    ti.refThread = NULL;

  if(dwFlags & CTF_PROCESS_REF)
    _SHGetInstanceExplorer(&ti.refIE);
  else
    ti.refIE = NULL;

  /* Create the thread */
  if(ti.hEvent)
  {
    DWORD dwRetVal;
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, SHLWAPI_ThreadWrapper, &ti, 0, &dwRetVal);

    if(hThread)
    {
      /* Wait for the thread to signal us to continue */
      WaitForSingleObject(ti.hEvent, INFINITE);
      CloseHandle(hThread);
      bCalled = TRUE;
    }
    CloseHandle(ti.hEvent);
  }

  if (!bCalled)
  {
    if (!ti.pfnCallback && dwFlags & CTF_INSIST)
    {
      /* Couldn't call, call synchronously */
      pfnThreadProc(pData);
      bCalled = TRUE;
    }
    else
    {
      /* Free references, since thread hasn't run to do so */
      if(ti.refThread)
        IUnknown_Release(ti.refThread);

      if(ti.refIE)
        IUnknown_Release(ti.refIE);
    }
  }
  return bCalled;
}

/*************************************************************************
 *      _SHGlobalCounterGetValue	[SHLWAPI.223]
 *
 * Get the current count of a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The current count of the semaphore.
 */
LONG WINAPI _SHGlobalCounterGetValue(HANDLE hSem)
{
  LONG dwOldCount = 0;

  TRACE("(%p)\n", hSem);
  ReleaseSemaphore(hSem, 1, &dwOldCount); /* +1 */
  WaitForSingleObject(hSem, 0);           /* -1 */
  return dwOldCount;
}

/*************************************************************************
 *      _SHGlobalCounterIncrement	[SHLWAPI.224]
 *
 * Claim a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The new count of the semaphore.
 */
LONG WINAPI _SHGlobalCounterIncrement(HANDLE hSem)
{
  LONG dwOldCount = 0;

  TRACE("(%p)\n", hSem);
  ReleaseSemaphore(hSem, 1, &dwOldCount);
  return dwOldCount + 1;
}

/*************************************************************************
 *      _SHGlobalCounterDecrement	[SHLWAPI.424]
 *
 * Release a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The new count of the semaphore.
 */
DWORD WINAPI _SHGlobalCounterDecrement(HANDLE hSem)
{
  DWORD dwOldCount = 0;

  TRACE("(%p)\n", hSem);

  dwOldCount = _SHGlobalCounterGetValue(hSem);
  WaitForSingleObject(hSem, 0);
  return dwOldCount - 1;
}

/*************************************************************************
 *      _SHGlobalCounterCreateNamedW	[SHLWAPI.423]
 *
 * Unicode version of _SHGlobalCounterCreateNamedA.
 */
HANDLE WINAPI _SHGlobalCounterCreateNamedW(LPCWSTR lpszName, DWORD iInitial)
{
  static const WCHAR szPrefix[] = { 's', 'h', 'e', 'l', 'l', '.', '\0' };
  const int iPrefixLen = 6;
  WCHAR szBuff[MAX_PATH];
  const int iBuffLen = sizeof(szBuff)/sizeof(WCHAR);
  SECURITY_DESCRIPTOR sd;
  SECURITY_ATTRIBUTES sAttr, *pSecAttr;
  HANDLE hRet;

  TRACE("(%s,%ld)\n", debugstr_w(lpszName), iInitial);

  /* Create Semaphore name */
  memcpy(szBuff, szPrefix, (iPrefixLen + 1) * sizeof(WCHAR));
  if (lpszName)
    StrCpyNW(szBuff + iPrefixLen, lpszName, iBuffLen - iPrefixLen);

  /* Initialise security attributes */
  pSecAttr = _CreateAllAccessSecurityAttributes(&sAttr, &sd, 0);

  if (!(hRet = CreateSemaphoreW(pSecAttr , iInitial, MAXLONG, szBuff)))
    hRet = OpenSemaphoreW(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE, 0, szBuff);
  return hRet;
}

/*************************************************************************
 *      _SHGlobalCounterCreateNamedA	[SHLWAPI.422]
 *
 * Create a semaphore.
 *
 * PARAMS
 *  lpszName [I] Name of semaphore
 *  iInitial [I] Initial count for semaphore
 *
 * RETURNS
 *  A new semaphore handle.
 */
HANDLE WINAPI _SHGlobalCounterCreateNamedA(LPCSTR lpszName, DWORD iInitial)
{
  WCHAR szBuff[MAX_PATH];

  TRACE("(%s,%ld)\n", debugstr_a(lpszName), iInitial);

  if (lpszName)
    MultiByteToWideChar(0, 0, lpszName, -1, szBuff, MAX_PATH);
  return _SHGlobalCounterCreateNamedW(lpszName ? szBuff : NULL, iInitial);
}

/*************************************************************************
 *      _SHGlobalCounterCreate	[SHLWAPI.222]
 *
 * Create a semaphore using the name of a GUID.
 *
 * PARAMS
 *  guid [I] GUID to use as semaphore name
 *
 * RETURNS
 *  A handle to the semaphore.
 *
 * NOTES
 *  The initial count of the semaphore is set to 0.
 */
HANDLE WINAPI _SHGlobalCounterCreate (REFGUID guid)
{
  char szName[40];

  TRACE("(%s)\n", debugstr_guid(guid));

  /* Create a named semaphore using the GUID string */
  SHStringFromGUIDA(guid, szName, sizeof(szName) - 1);
  return _SHGlobalCounterCreateNamedA(szName, 0);
}
