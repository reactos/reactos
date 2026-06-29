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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_USER
#include "shlwapi.h"
#include "shlobj.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern DWORD SHLWAPI_ThreadRef_index;  /* Initialised in shlwapi_main.c */

INT WINAPI SHStringFromGUIDA(REFGUID,LPSTR,INT);

/**************************************************************************
 *      CreateAllAccessSecurityAttributes       [SHLWAPI.356]
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
LPSECURITY_ATTRIBUTES WINAPI CreateAllAccessSecurityAttributes(
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
    return SHGetInstanceExplorer(lppUnknown);
}

/*************************************************************************
 *      SHGlobalCounterGetValue	[SHLWAPI.223]
 *
 * Get the current count of a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The current count of the semaphore.
 */
LONG WINAPI SHGlobalCounterGetValue(HANDLE hSem)
{
  LONG dwOldCount = 0;

  TRACE("(%p)\n", hSem);
  ReleaseSemaphore(hSem, 1, &dwOldCount); /* +1 */
  WaitForSingleObject(hSem, 0);           /* -1 */
  return dwOldCount;
}

/*************************************************************************
 *      SHGlobalCounterIncrement	[SHLWAPI.224]
 *
 * Claim a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The new count of the semaphore.
 */
LONG WINAPI SHGlobalCounterIncrement(HANDLE hSem)
{
  LONG dwOldCount = 0;

  TRACE("(%p)\n", hSem);
  ReleaseSemaphore(hSem, 1, &dwOldCount);
  return dwOldCount + 1;
}

/*************************************************************************
 *      SHGlobalCounterDecrement	[SHLWAPI.424]
 *
 * Release a semaphore.
 *
 * PARAMS
 *  hSem [I] Semaphore handle
 *
 * RETURNS
 *  The new count of the semaphore.
 */
DWORD WINAPI SHGlobalCounterDecrement(HANDLE hSem)
{
  DWORD dwOldCount = 0;

  TRACE("(%p)\n", hSem);

  dwOldCount = SHGlobalCounterGetValue(hSem);
  WaitForSingleObject(hSem, 0);
  return dwOldCount - 1;
}

/*************************************************************************
 *      SHGlobalCounterCreateNamedW	[SHLWAPI.423]
 *
 * Unicode version of SHGlobalCounterCreateNamedA.
 */
HANDLE WINAPI SHGlobalCounterCreateNamedW(LPCWSTR lpszName, DWORD iInitial)
{
  static const WCHAR szPrefix[] = { 's', 'h', 'e', 'l', 'l', '.', '\0' };
  const int iPrefixLen = 6;
  WCHAR szBuff[MAX_PATH];
  SECURITY_DESCRIPTOR sd;
  SECURITY_ATTRIBUTES sAttr, *pSecAttr;
  HANDLE hRet;

  TRACE("(%s,%ld)\n", debugstr_w(lpszName), iInitial);

  /* Create Semaphore name */
  memcpy(szBuff, szPrefix, (iPrefixLen + 1) * sizeof(WCHAR));
  if (lpszName)
    StrCpyNW(szBuff + iPrefixLen, lpszName, ARRAY_SIZE(szBuff) - iPrefixLen);

  /* Initialise security attributes */
  pSecAttr = CreateAllAccessSecurityAttributes(&sAttr, &sd, 0);

  if (!(hRet = CreateSemaphoreW(pSecAttr , iInitial, MAXLONG, szBuff)))
    hRet = OpenSemaphoreW(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE, 0, szBuff);
  return hRet;
}

/*************************************************************************
 *      SHGlobalCounterCreateNamedA	[SHLWAPI.422]
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
HANDLE WINAPI SHGlobalCounterCreateNamedA(LPCSTR lpszName, DWORD iInitial)
{
  WCHAR szBuff[MAX_PATH];

  TRACE("(%s,%ld)\n", debugstr_a(lpszName), iInitial);

  if (lpszName)
    MultiByteToWideChar(CP_ACP, 0, lpszName, -1, szBuff, MAX_PATH);
  return SHGlobalCounterCreateNamedW(lpszName ? szBuff : NULL, iInitial);
}

/*************************************************************************
 *      SHGlobalCounterCreate	[SHLWAPI.222]
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
HANDLE WINAPI SHGlobalCounterCreate (REFGUID guid)
{
  char szName[40];

  TRACE("(%s)\n", debugstr_guid(guid));

  /* Create a named semaphore using the GUID string */
  SHStringFromGUIDA(guid, szName, sizeof(szName) - 1);
  return SHGlobalCounterCreateNamedA(szName, 0);
}
