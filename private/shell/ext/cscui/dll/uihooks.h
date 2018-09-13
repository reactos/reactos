//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       uihooks.h
//
//--------------------------------------------------------------------------

#ifndef _UIHOOKS_
#define _UIHOOKS_

#define CSCH_NotifyOffline          1
#define CSCH_NotifyAvailable        2
#define CSCH_Pin                    3
#define CSCH_Unpin                  4
#define CSCH_DeleteConflict         5
#define CSCH_UpdateConflict         6
#define CSCH_SyncShare              7

#ifdef CSCUI_HOOKS

//
// Use CSCUI_NOTIFYHOOK to send self-host notifications. The
// macro should be used like this.
//
// CSCUI_NOTIFYHOOK((<CSCH_* value>,<fmt string>,arg,arg,arg));
//
// CSCUI_NOTIFYHOOK((CSCH_SyncShare, TEXT("Syncing share %1"), pszShareName));
//
// Entire macro arg set must be enclosed in separate set of parens.
//

#define CSCUI_NOTIFYHOOK(x) CSCUIExt_NotifyHook x

void CSCUIExt_NotifyHook(DWORD csch, LPCTSTR psz, ...);
void CSCUIExt_CleanupHooks();

#else   // !CSCUI_HOOKS

#define CSCUI_NOTIFYHOOK(x)

inline void CSCUIExt_NotifyHook(DWORD csch, LPCTSTR psz, ...) {}
#define CSCUIExt_CleanupHooks()

#endif  // !CSCUI_HOOKS

#endif  // _UIHOOKS_
