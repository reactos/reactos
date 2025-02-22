/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig/treeview.h
 * PURPOSE:     Tree-View helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "comctl32supp.h"


//
// Should be present in commctrl.h
// defined for Windows Vista+
//

#if (_WIN32_WINNT >= 0x0600)

#define TVS_EX_MULTISELECT          0x0002
#define TVS_EX_DOUBLEBUFFER         0x0004
#define TVS_EX_NOINDENTSTATE        0x0008
#define TVS_EX_RICHTOOLTIP          0x0010
#define TVS_EX_AUTOHSCROLL          0x0020
#define TVS_EX_FADEINOUTEXPANDOS    0x0040
#define TVS_EX_PARTIALCHECKBOXES    0x0080
#define TVS_EX_EXCLUSIONCHECKBOXES  0x0100
#define TVS_EX_DIMMEDCHECKBOXES     0x0200
#define TVS_EX_DRAWIMAGEASYNC       0x0400

#endif


#if (_WIN32_WINNT >= 0x0501)

#define TVM_SETEXTENDEDSTYLE      (TV_FIRST + 44)
#define TreeView_SetExtendedStyle(hwnd, dw, mask) \
    (DWORD)SNDMSG((hwnd), TVM_SETEXTENDEDSTYLE, mask, dw)

#define TVM_GETEXTENDEDSTYLE      (TV_FIRST + 45)
#define TreeView_GetExtendedStyle(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETEXTENDEDSTYLE, 0, 0)

#endif


void TreeView_Set3StateCheck(HWND hTree);

void TreeView_Cleanup(HWND hTree);

HTREEITEM
InsertItem(HWND hTree,
           LPCWSTR szName,
           HTREEITEM hParent,
           HTREEITEM hInsertAfter);

UINT TreeView_GetRealSubtreeState(HWND hTree, HTREEITEM htiSubtreeItem);
void TreeView_PropagateStateOfItemToParent(HWND hTree, HTREEITEM htiItem);
void TreeView_DownItem(HWND hTree, HTREEITEM htiItemToDown);
void TreeView_UpItem(HWND hTree, HTREEITEM htiItemToUp);
HTREEITEM TreeView_GetFirst(HWND hTree);
HTREEITEM TreeView_GetLastFromItem(HWND hTree, HTREEITEM hItem);
HTREEITEM TreeView_GetLast(HWND hTree);
HTREEITEM TreeView_GetPrev(HWND hTree, HTREEITEM hItem);
HTREEITEM TreeView_GetNext(HWND hTree, HTREEITEM hItem);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TREEVIEW_H__

/* EOF */
