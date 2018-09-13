//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       aclpriv.h
//
//--------------------------------------------------------------------------

#ifndef _ACLUI_
#define _ACLUI_

#ifndef UNICODE
#error "No ANSI support yet"
#endif

// For test building NT4
//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//BOOL WINAPI ConvertSidToStringSid(PSID pSid, LPTSTR *ppszStringSid);

extern "C"
{
    #include <nt.h>         // SE_TAKE_OWNERSHIP_PRIVILEGE, etc
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <seopaque.h>   // RtlObjectAceSid, etc.
}

#define INC_OLE2
#include <windows.h>
#include <commctrl.h>
#include "resource.h"   // resource ID's
#include "idh.h"        // help ID's

#ifndef RC_INVOKED

#include <windowsx.h>
#include <atlconv.h>    // ANSI/Unicode conversion support
#include <shlobj.h>
#include <aclui.h>
#include <comctrlp.h>   // DPA/DSA
#if(_WIN32_WINNT >= 0x0500)
#include <objsel.h>     // DS Object Picker
#else
typedef IUnknown IDsObjectPicker;  // dummy placeholder
#endif
#include <common.h>
#include "misc.h"
#include "pagebase.h"
#include "chklist.h"
#include "ace.h"
#include "cstrings.h"
#include "sidcache.h"

// These are here for NT4 SP4 builds (comctrlp.h added these for NT5)
#ifndef DA_LAST
#define DA_LAST     (0x7FFFFFFF)
#endif
#ifndef DPA_AppendPtr
#define DPA_AppendPtr(hdpa, pitem)  DPA_InsertPtr(hdpa, DA_LAST, pitem)
#endif
#ifndef DSA_AppendItem
#define DSA_AppendItem(hdsa, pitem) DSA_InsertItem(hdsa, DA_LAST, pitem)
#endif

extern HINSTANCE hModule;
extern HINSTANCE g_hGetUserLib;
extern UINT UM_SIDLOOKUPCOMPLETE;
extern UINT g_cfDsSelectionList;
extern UINT g_cfSidInfoList;

// Magic debug flags
#define TRACE_PERMPAGE      0x00000001
#define TRACE_PRINCIPAL     0x00000002
#define TRACE_SI            0x00000004
#define TRACE_PERMSET       0x00000008
#define TRACE_ACELIST       0x00000010
#define TRACE_ACEEDIT       0x00000020
#define TRACE_OWNER         0x00000040
#define TRACE_MISC          0x00000080
#define TRACE_CHECKLIST     0x00000100
#define TRACE_SIDCACHE      0x00000200
#define TRACE_ALWAYS        0xffffffff          // use with caution

#define MAX_COLUMN_CHARS    100

#define COLUMN_ALLOW    1
#define COLUMN_DENY     2

#define ACE_INHERIT_ALL     (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

BOOL
ACLUIAPI
EditSecurityEx(HWND hwndOwner,
               LPSECURITYINFO psi,
               UINT nStartPage);

BOOL
EditACEEntry(HWND hwndOwner,
             LPSECURITYINFO psi,
             PACE pAce,
             SI_PAGE_TYPE siType,
             LPCTSTR pszObjectName,
             BOOL bReadOnly,
             DWORD *pdwResult,
             HDPA *phEntries,
             HDPA *phPropertyEntries,
             UINT nStartPage = 0);

// EditACEEntry result values. Set if something was edited on the
// corresponding page, otherwise clear.
#define EAE_NEW_OBJECT_ACE      0x0001
#define EAE_NEW_PROPERTY_ACE    0x0002

LPARAM
GetSelectedItemData(HWND hList, int *pIndex);

int
ConfirmAclProtect(HWND hwndParent, BOOL bDacl);

HPROPSHEETPAGE
CreateOwnerPage(LPSECURITYINFO psi, SI_OBJECT_INFO *psiObjectInfo);

STDMETHODIMP
_InitCheckList(HWND           hwndList,
               LPSECURITYINFO psi,
               const GUID *   pguidObjectType,
               DWORD          dwFlags,
               HINSTANCE      hInstance,
               DWORD          dwType,
               PSI_ACCESS *   ppDefaultAccess);

void
HandleListClick(PNM_CHECKLIST pnmc, SI_PAGE_TYPE siType, BOOL bContainerFlags);

UINT
GetAcesFromCheckList(HWND hChkList,
                     PSID pSid,
                     BOOL fPerm,
                     BOOL fAceFlagsProvided,
                     UCHAR uAceFlagsNew,
                     const GUID *pInheritGUID,
                     HDPA hEntries);

#endif // RC_INVOKED
#endif // _ACLUI_
