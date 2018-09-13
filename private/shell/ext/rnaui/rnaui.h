//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnaui.h
//  Content:    This file contains the declaration for RnaUI
//  History:
//      Tue 23-Feb-1993 14:08:25  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef _RNAUI_H_
#define _RNAUI_H_


//****************************************************************************
// Global Include File
//****************************************************************************

#define _INC_OLE
#define STRICT

#include <windows.h>        // also includes windowsx.h
#include <shellapi.h>       // for registration functions
#include <port32.h>
#include <windowsx.h>
#include <ole2.h>

#undef RtlMoveMemory
__declspec(dllimport) VOID __stdcall
RtlMoveMemory (
  VOID UNALIGNED *Destination,
  CONST VOID UNALIGNED *Source,
  DWORD Length
  );

// shlobj.h can be found in \dev\inc
#include <shlobj.h>         // Shell OLE interfaces
#define USE_MONIKER
// shellp.h can be found in \win\core\shell\inc
#include <shsemip.h>         // 32-bit Shell UI stuff
#include <shellp.h>         // *INTERNAL* shell include file (SatoNa gave us permission)

#include <shlwapi.h>

#include <string.h>
#include <rna.h>            // RNA Session API
#include <rnaspi.h>         // Communication with RNA applet
#include <rnap.h>           // Communication with RNA applet

#include "rcids.h"          // Resource declaration
#include "cstrings.h"       // String constants

#define PUBLIC          FAR PASCAL
#define CPUBLIC         FAR _cdecl
#define PRIVATE         NEAR PASCAL

#include "utils.h"          // Common macros
#include "strings.h"        // String macros
#include "err.h"            // Error/Debug support
#include "subobj.h"         // Subobject space
#include "mem.h"            // Shared memory support


//****************************************************************************
// Constant Definitions
//****************************************************************************

#define MAXNAME         32      // Maximum name length
#define MAXMESSAGE      128     // Maximum resource string message
#define MAXSTRINGLEN    256     // Maximum output string length
#define MAXINTLEN       7       // Maximum interger string length
#define MAXLONGLEN      11      // Maximum long string length
#define MAXMSGLEN       512     // Maximum message length
#define MAXSUBOBJLEN    (2*MAXPATHLEN)

//****************************************************************************
// Macros
//****************************************************************************

#define lmemcpy(dest, src, cb)          CopyMemory(dest, src, (DWORD)cb)
#define lmemzero(dest, cb)              ZeroMemory(dest, (DWORD)cb)
#define lmemfill(dest, cb, ch)          FillMemory(dest, (DWORD)cb, ch)

//#define lstrcpyA(dest, src)             strcpy(dest, src)
//#define lstrcpynA(dest, src, cb)        {strncpy(dest, src, cb); dest[cb-1]='\0';}
//#define lstrlenA(dest)                  strlen(dest)
//#define lstrcmpiA(dest, src)            stricmp(dest, src)
//#define lstrcmpA(dest, src)             strcmp(dest, src)
//#define lstrcatA(dest, src)             strcat(dest, src)

// Task allocator macros
#define OLEAddRef(pmalloc)              ((pmalloc)->lpVtbl->AddRef((pmalloc)))
#define OLERelease(pmalloc)             ((pmalloc)->lpVtbl->Release((pmalloc)))
#define OLEAlloc(pmalloc, cb)           ((pmalloc)->lpVtbl->Alloc((pmalloc), (cb)))
#define OLEFree(pmalloc, pv)            ((pmalloc)->lpVtbl->Free((pmalloc), (pv)))
#define OLERealloc(pmalloc, pv, cb)     ((pmalloc)->lpVtbl->Realloc((pmalloc), (pv), (cb)))

//****************************************************************************
// Constants
//****************************************************************************

#define DEVICE_NULL     szNullDevice
#define DEVICE_MODEM    szModemDevice

#define CLIENT_WIZ      0x00000002
#define INTRO_WIZ       0x00000080

//****************************************************************************
// Global Parameters
//****************************************************************************

#ifdef DEBUG

extern UINT g_uBreakFlags;         // Controls when to int 3
extern UINT g_uTraceFlags;         // Controls what trace messages are spewed
extern UINT g_uDumpFlags;          // Controls what structs get dumped

#endif

extern  HINSTANCE   ghInstance;
extern  int         g_cRef;
extern LPITEMIDLIST g_pidlRemote;


//---------------------------------------------------------------------------
// Critical section stuff
//---------------------------------------------------------------------------

// Notes:
//  1. Never "return" from the critical section.
//  2. Never "SendMessage" or "Yield" from the critical section.
//  3. Never call USER API which may yield.
//  4. Always make the critical section as small as possible.
//

#ifdef DEBUG

void PUBLIC RNA_EnterExclusive(void);
void PUBLIC RNA_LeaveExclusive(void);
extern BOOL g_bExclusive;

#define ENTEREXCLUSIVE()    RNA_EnterExclusive();
#define LEAVEEXCLUSIVE()    RNA_LeaveExclusive();
#define ASSERTEXCLUSIVE()   ASSERT(g_bExclusive)

#else

extern CRITICAL_SECTION g_csRNA;

#define ENTEREXCLUSIVE() EnterCriticalSection(&g_csRNA);
#define LEAVEEXCLUSIVE() LeaveCriticalSection(&g_csRNA);
#define ASSERTEXCLUSIVE()

#endif

//****************************************************************************
// .ini file routines
//****************************************************************************

BOOL PUBLIC ProcessIniFile(void);
BOOL FAR PASCAL RunWizard (HWND hwnd, DWORD dwType);

#endif  //_RNAUI_H_
