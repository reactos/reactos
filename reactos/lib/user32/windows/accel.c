/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: accel.c,v 1.7 2003/07/09 00:09:47 hyperion Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Accelerator tables
 * PROGRAMMER:      KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *      09/05/2001  CSH  Created
 *      08/07/2003  KJK  Fully implemented
 */

/* INCLUDES ******************************************************************/
#include <string.h>
#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* RT_ACCELERATOR resources are arrays of RES_ACCEL structures */
typedef struct _RES_ACCEL
{
 WORD fVirt;
 WORD key;
 DWORD cmd;
}
RES_ACCEL;

/* ACCELERATOR TABLES CACHE */
/* Cache entry */
typedef struct _USER_ACCEL_CACHE_ENTRY
{
 struct _USER_ACCEL_CACHE_ENTRY * Next;
 ULONG_PTR Usage; /* how many times the table has been loaded */
 HACCEL Object;   /* handle to the NtUser accelerator table object */
 HGLOBAL Data;    /* base address of the resource data */
}
U32_ACCEL_CACHE_ENTRY;

/* Lock guarding the cache */
CRITICAL_SECTION U32AccelCacheLock;

/* Cache */
U32_ACCEL_CACHE_ENTRY * U32AccelCache = NULL;

U32_ACCEL_CACHE_ENTRY ** WINAPI U32AccelCacheFind(HANDLE, HGLOBAL);
void WINAPI U32AccelCacheAdd(HACCEL, HGLOBAL);

/* Look up a handle or resource address in the cache */
U32_ACCEL_CACHE_ENTRY ** WINAPI U32AccelCacheFind(HANDLE Object, HGLOBAL Data)
{
 /*
  to avoid using a double-link list and still allow elements to be removed,
  return a pointer to the list link that points to the desired entry
 */
 U32_ACCEL_CACHE_ENTRY ** ppEntry = &U32AccelCache;
 
 for(; *ppEntry; ppEntry = &((*ppEntry)->Next))
  if((*ppEntry)->Object == Object || (*ppEntry)->Data == Data) break;

 return ppEntry;
}

/* Allocate an entry and insert it into the cache */
void WINAPI U32AccelCacheAdd(HACCEL Object, HGLOBAL Data)
{
 U32_ACCEL_CACHE_ENTRY * pEntry =
  LocalAlloc(LMEM_FIXED, sizeof(U32_ACCEL_CACHE_ENTRY));

 /* failed to allocate an entry - not critical */
 if(pEntry == NULL) return;

 /* initialize the entry */
 pEntry->Usage = 1;
 pEntry->Object = Object;
 pEntry->Data = Data;

 /* insert the entry into the cache */
 pEntry->Next = U32AccelCache;
 U32AccelCache = pEntry;
}

/* Create an accelerator table from a loaded resource */
HACCEL WINAPI U32LoadAccelerators(HINSTANCE hInstance, HRSRC hTableRes)
{
 HGLOBAL hAccTableData;
 HACCEL hAccTable = NULL;
 U32_ACCEL_CACHE_ENTRY * pEntry;
 RES_ACCEL * pAccTableResData;
 RES_ACCEL * p;
 SIZE_T i = 0;
 SIZE_T j = 0;
 ACCEL * pAccTableData;

 /* load the accelerator table */
 hAccTableData = LoadResource(hInstance, hTableRes);

 /* failure */
 if(hAccTableData == NULL) return NULL;

 RtlEnterCriticalSection(&U32AccelCacheLock);

 /* see if this accelerator table has already been loaded */
 pEntry = *U32AccelCacheFind(NULL, hAccTableData);

 /* accelerator table already loaded */
 if(pEntry)
 {
  /* increment the reference count */
  ++ pEntry->Usage;

  /* return the existing object */
  hAccTable = pEntry->Object;

  /* success */
  goto l_Leave;
 }

 /* count the number of entries in the table */
 p = pAccTableResData = (RES_ACCEL *)hAccTableData;

 while(1)
 {
  /* FIXME??? unknown flag 0x60 stops the scan */
  if(p->fVirt & 0x60) break;

  ++ i;
  ++ p;

  /* flag 0x80 marks the last entry of the table */
  if(p->fVirt & 0x80) break;
 }

 /* allocate the buffer for the table to be passed to Win32K */
 pAccTableData = LocalAlloc(LMEM_FIXED, i * sizeof(ACCEL));

 /* failure */
 if(pAccTableData == NULL) goto l_Leave;

 /* copy the table */
 for(j = 0; j < i; ++ j)
 {
  pAccTableData[j].fVirt = pAccTableResData[j].fVirt;
  pAccTableData[j].key = pAccTableResData[j].key;
  pAccTableData[j].cmd = pAccTableResData[j].cmd;
 }

 /* create a new accelerator table object */
 hAccTable = NtUserCreateAcceleratorTable(pAccTableData, i);

 /* free the buffer */
 LocalFree(pAccTableData);
 
 /* failure */
 if(hAccTable == NULL) goto l_Leave;

 /* success - cache the object */
 U32AccelCacheAdd(hAccTable, pAccTableResData);

l_Leave:
 RtlLeaveCriticalSection(&U32AccelCacheLock);
 return hAccTable;
}

/* Checks if a message can be translated through an accelerator table */
BOOL WINAPI U32IsValidAccelMessage(UINT uMsg)
{
 switch(uMsg)
 {
  case WM_KEYDOWN:
  case WM_CHAR:
  case WM_SYSKEYDOWN:
  case WM_SYSCHAR:
   return TRUE;

  default:
   return FALSE;
 }
}

/* WIN32 FUNCTIONS ***********************************************************/

/*
 Dereference the specified accelerator table, removing it from the cache and
 deleting the associated NtUser object as appropriate
*/
BOOL WINAPI DestroyAcceleratorTable(HACCEL hAccel)
{
 U32_ACCEL_CACHE_ENTRY ** ppEntry;
 ULONG_PTR nUsage = 0;

 RtlEnterCriticalSection(&U32AccelCacheLock);

 /* see if this accelerator table has been cached */
 ppEntry = U32AccelCacheFind(hAccel, NULL);

 /* accelerator table cached */
 if(*ppEntry)
 {
  U32_ACCEL_CACHE_ENTRY * pEntry = *ppEntry;

  /* decrement the reference count */
  nUsage = pEntry->Usage= pEntry->Usage - 1;

  /* reference count now zero: destroy the cache entry */
  if(nUsage == 0)
  {
   /* unlink the cache entry */
   *ppEntry = pEntry->Next;

   /* free the cache entry */
   LocalFree(pEntry);
  }
 }

 RtlLeaveCriticalSection(&U32AccelCacheLock);

 if(nUsage > 0) return FALSE;

 /* destroy the object */
 return NtUserDestroyAcceleratorTable(hAccel);
}

/* Create an accelerator table from a named resource */
HACCEL WINAPI LoadAcceleratorsW(HINSTANCE hInstance, LPCWSTR lpTableName)
{
 return U32LoadAccelerators
 (
  hInstance, 
  FindResourceExW(hInstance, MAKEINTRESOURCEW(RT_ACCELERATOR), lpTableName, 0)
 );
}
 
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName)
{
 return U32LoadAccelerators
 (
  hInstance, 
  FindResourceExA(hInstance, MAKEINTRESOURCEA(RT_ACCELERATOR), lpTableName, 0)
 );
}

/* Translate a key press into a WM_COMMAND message */
int WINAPI TranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
 if(!U32IsValidAccelMessage(lpMsg->message)) return 0;

 return NtUserTranslateAccelerator(hWnd, hAccTable, lpMsg);
}

/* NTUSER STUBS */
int WINAPI CopyAcceleratorTableW
(
 HACCEL hAccelSrc,
 LPACCEL lpAccelDst,
 int cAccelEntries
)
{
 return NtUserCopyAcceleratorTable(hAccelSrc, lpAccelDst, cAccelEntries);
}

HACCEL WINAPI CreateAcceleratorTableW(LPACCEL lpaccl, int cEntries)
{
 return NtUserCreateAcceleratorTable(lpaccl, cEntries);
}

/* ANSI STUBS */
int WINAPI CopyAcceleratorTableA
(
 HACCEL hAccelSrc,
 LPACCEL lpAccelDst,
 int cAccelEntries
)
{
 int i;

 cAccelEntries = CopyAcceleratorTableW(hAccelSrc, lpAccelDst, cAccelEntries);
 
 if(cAccelEntries == 0) return 0;

 for(i = 0; i < cAccelEntries; ++ i)
  if(!(lpAccelDst[i].fVirt & FVIRTKEY))
  {
   NTSTATUS nErrCode = RtlUnicodeToMultiByteN
   (
    (PCHAR)&lpAccelDst[i].key,
    sizeof(lpAccelDst[i].key),
    NULL,
    (PWCHAR)&lpAccelDst[i].key,
    sizeof(lpAccelDst[i].key)
   );

   if(!NT_SUCCESS(nErrCode)) lpAccelDst[i].key = 0;
  }

 return cAccelEntries;
}

HACCEL WINAPI CreateAcceleratorTableA(LPACCEL lpaccl, int cEntries)
{
 int i;

 for(i = 0; i < cEntries; ++ i)
  if(!lpaccl[i].fVirt)
  {
   NTSTATUS nErrCode = RtlMultiByteToUnicodeN
   (
    (PWCHAR)&lpaccl[i].key,
    sizeof(lpaccl[i].key),
    NULL,
    (PCHAR)&lpaccl[i].key,
    sizeof(lpaccl[i].key)
   );

   if(!NT_SUCCESS(nErrCode)) lpaccl[i].key = -1;
  }

 return CreateAcceleratorTableW(lpaccl, cEntries);
}

int WINAPI TranslateAcceleratorA(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
 MSG mCopy = *lpMsg;
 CHAR cChar;
 WCHAR wChar;

 if(!U32IsValidAccelMessage(lpMsg->message)) return 0;

 NTSTATUS nErrCode =
  RtlMultiByteToUnicodeN(&wChar, sizeof(wChar), NULL, &cChar, sizeof(cChar));

 if(!nErrCode)
 {
  SetLastError(RtlNtStatusToDosError(nErrCode));
  return 0;
 }

 return TranslateAcceleratorW(hWnd, hAccTable, &mCopy);
}

/* EOF */
