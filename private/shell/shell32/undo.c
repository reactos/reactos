//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1994
//
// File: undo.c
//
// History:
//  There is no history.  This file doesn't exist
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include <shellp.h>

#pragma data_seg(DATASEG_SHARED)

// use a linked list because we're going to be pulling things off the top
// and bottom all the time.
HDPA s_hdpaUndo = NULL;
BOOL s_fUndoSuspended = FALSE;

#pragma data_seg()

#define MAX_UNDO  10

void NukeUndoAtom(LPUNDOATOM lpua)
{
    lpua->Release( lpua );
    Free( lpua );
}

void SuspendUndo(BOOL f)
{
    if (f)
        s_fUndoSuspended++;
    else
        s_fUndoSuspended--;
    
    ASSERT(s_fUndoSuspended >= 0);
    // sanity check
    if (s_fUndoSuspended < 0)
        s_fUndoSuspended = 0;
}


void AddUndoAtom(LPUNDOATOM lpua)
{
    int i;

    ENTERCRITICAL;
    ASSERT(lpua);
    if (!s_hdpaUndo) {
        s_hdpaUndo = DPA_Create(MAX_UNDO + 1);
    }

    if (s_hdpaUndo) {
        i = DPA_AppendPtr(s_hdpaUndo, lpua);
        if (i != -1) {
            if (i >= MAX_UNDO) {
                lpua = DPA_FastGetPtr(s_hdpaUndo, 0);
                NukeUndoAtom(lpua);
                DPA_DeletePtr(s_hdpaUndo, 0);
            }
        }
    }
    LEAVECRITICAL;
}

LPUNDOATOM _PeekUndoAtom(LPINT lpi)
{
    int i = -1;
    LPUNDOATOM lpua = NULL;

    if (s_hdpaUndo) {
        i = DPA_GetPtrCount(s_hdpaUndo) - 1;
        if (i >= 0) {
            lpua = DPA_FastGetPtr(s_hdpaUndo, i);

        }
    }
    if (lpi)
        *lpi = i;
    return lpua;
}

void EnumUndoAtoms(int (CALLBACK* lpfn)(LPUNDOATOM lpua, LPARAM lParam), LPARAM lParam)
{
    int i;

    if (!s_hdpaUndo) {
        return;
    }

    ENTERCRITICAL;
    for (i = DPA_GetPtrCount(s_hdpaUndo) - 1; i >= 0; i--) {
        LPUNDOATOM lpua;
        int iRet;
        lpua = DPA_FastGetPtr(s_hdpaUndo, i);
        iRet = lpfn(lpua, lParam);

        if (iRet &  EUA_DELETE) {
            DPA_DeletePtr(s_hdpaUndo, i);
            NukeUndoAtom(lpua);
        }

        if (iRet & EUA_ABORT) {
            break;
        }
    }

    LEAVECRITICAL;
}

#define DoUndoAtom(lpua) ((lpua)->Invoke((lpua)))

void Undo(HWND hwnd)
{
    int i;
    LPUNDOATOM lpua;
    DECLAREWAITCURSOR;

    if (!IsUndoAvailable()) {
        MessageBeep(0);
        return;
    }
    
    SetWaitCursor();

    ENTERCRITICAL;
    ASSERT(s_hdpaUndo);
    lpua = _PeekUndoAtom(&i);
    if (lpua)
        DPA_DeletePtr(s_hdpaUndo, i);
    LEAVECRITICAL;

    if (lpua) {
        lpua->hwnd = hwnd;
        DoUndoAtom(lpua);
    }
    ResetWaitCursor();
}

BOOL IsUndoAvailable()
{
    return s_hdpaUndo && !s_fUndoSuspended &&
        DPA_GetPtrCount(s_hdpaUndo);
}

#define _GetUndoText(lpua, buffer, type) (lpua)->GetText((lpua), buffer, type)

void GetUndoText(LPUNDOATOM lpua, LPTSTR lpszBuffer, UINT cchBuffer, int type)
{
    TCHAR szTemp[MAX_PATH * 2 + 80];
    TCHAR lpszFormat[MAX_PATH];     // (MAX_PATH is overkill, oh well...)

    ASSERT(cchBuffer > 0);
    lpszBuffer[0] = 0;  // assume failure
    szTemp[0] = 0;

    _GetUndoText(lpua, szTemp, type);

    if (LoadString(HINST_THISDLL, (type == UNDO_MENUTEXT)  ? IDS_UNDOACCEL : IDS_UNDO, lpszFormat, ARRAYSIZE(lpszFormat)) != 0)
    {
        if (type == UNDO_STATUSTEXT)
        {
            // Status text shouldn't have ampersand or tab
            ASSERT(StrChr(lpszFormat, TEXT('&')) == NULL);
            ASSERT(StrChr(lpszFormat, TEXT('\t')) == NULL);
        }
        wnsprintf(lpszBuffer, cchBuffer, lpszFormat, szTemp);
    }
}
// BUGBUG, clean up on PROCESS_DETACH
