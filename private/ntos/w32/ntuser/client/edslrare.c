/****************************************************************************\
* edslRare.c - SL Edit controls Routines Called rarely are to be
* put in a seperate segment _EDSLRare. This file contains
* these routines.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Single-Line Support Routines called Rarely
*
* Created: 02-08-89 sankar
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SLCreate
*
* Creates the edit control for the window hwnd by allocating memory
* as required from the application's heap. Notifies parent if no memory
* error (after cleaning up if needed). Returns TRUE if no error else return s
* -1.
*
* History:
\***************************************************************************/

LONG SLCreate(
    PED ped,
    LPCREATESTRUCT lpCreateStruct) //!!! CREATESTRUCT AorW and in other routines
{
    LPSTR lpWindowText;
    LONG windowStyle = ped->pwnd->style;

    /*
     * Do the standard creation stuff
     */
    if (!ECCreate(ped, windowStyle))
        return (-1);

    /*
     * Single lines always have no undo and 1 line
     */
    ped->cLines = 1;
    ped->undoType = UNDO_NONE;

    /*
     * Check if this edit control is part of a combobox and get a pointer to the
     * combobox structure.
     */
    if (windowStyle & ES_COMBOBOX)
        ped->listboxHwnd = GetDlgItem(lpCreateStruct->hwndParent, CBLISTBOXID);

    /*
     * Set the default font to be the system font.
     */
    ECSetFont(ped, NULL, FALSE);

    /*
     * Set the window text if needed. Return false if we can't set the text
     * SLSetText notifies the parent in case there is a no memory error.
     */
    if ((ULONG_PTR)lpCreateStruct->lpszName > gHighestUserAddress)
        lpWindowText = REBASEPTR(ped->pwnd, (PVOID)lpCreateStruct->lpszName);
    else
        lpWindowText = (LPSTR)lpCreateStruct->lpszName;

    if ((lpWindowText != NULL)
            && !IsEmptyString(lpWindowText, ped->fAnsi)
            && !ECSetText(ped, lpWindowText)) {
        return (-1);
    }

    if (windowStyle & ES_PASSWORD)
        ECSetPasswordChar(ped, (UINT)'*');

    return TRUE;
}

/***************************************************************************\
* SLUndoHandler AorW
*
* Handles UNDO for single line edit controls.
*
* History:
\***************************************************************************/

BOOL SLUndo(
    PED ped)
{
    PBYTE hDeletedText = ped->hDeletedText;
    BOOL fDelete = (BOOL)(ped->undoType & UNDO_DELETE);
    ICH cchDeleted = ped->cchDeleted;
    ICH ichDeleted = ped->ichDeleted;
    BOOL fUpdate = FALSE;

    if (ped->undoType == UNDO_NONE) {

        /*
         * No undo...
         */
        return FALSE;
    }

    ped->hDeletedText = NULL;
    ped->cchDeleted = 0;
    ped->ichDeleted = (ICH)-1;
    ped->undoType &= ~UNDO_DELETE;

    if (ped->undoType == UNDO_INSERT) {
        ped->undoType = UNDO_NONE;

        /*
         * Set the selection to the inserted text
         */
        SLSetSelection(ped, ped->ichInsStart, ped->ichInsEnd);
        ped->ichInsStart = ped->ichInsEnd = (ICH)-1;

#ifdef NEVER

        /*
         * Now send a backspace to deleted and save it in the undo buffer...
         */
        SLCharHandler(pped, VK_BACK);
        fUpdate = TRUE;
#else

        /*
         * Delete the selected text and save it in undo buff.
         * Call ECDeleteText() instead of sending a VK_BACK message
         * which results in an EN_UPDATE notification send even before
         * we insert the deleted chars. This results in Bug #6610.
         * Fix for Bug #6610 -- SANKAR -- 04/19/91 --
         */
        if (ECDeleteText(ped)) {

            /*
             * Text was deleted -- flag for update and clear selection
             */
            fUpdate = TRUE;
            SLSetSelection(ped, ichDeleted, ichDeleted);
        }
#endif
    }

    if (fDelete) {
        HWND hwndSave = ped->hwnd; // Used for validation.

        /*
         * Insert deleted chars. Set the selection to the inserted text.
         */
        SLSetSelection(ped, ichDeleted, ichDeleted);
        SLInsertText(ped, hDeletedText, cchDeleted);
        UserGlobalFree(hDeletedText);
        if (!IsWindow(hwndSave))
            return FALSE;
        SLSetSelection(ped, ichDeleted, ichDeleted + cchDeleted);
        fUpdate = TRUE;
    }

    if (fUpdate) {
        /*
         * If we have something to update, send EN_UPDATE before and
         * EN_CHANGE after the actual update.
         * A part of the fix for Bug #6610 -- SANKAR -- 04/19/91 --
         */
        ECNotifyParent(ped, EN_UPDATE);

        if (FChildVisible(ped->hwnd)) {
// JimA changed this to ECInvalidateClient(ped, FALSE) Nov 1994
//            GetClientRect(ped->hwnd, &rcEdit);
//            if (ped->fBorder && rcEdit.right - rcEdit.left && rcEdit.bottom - rcEdit.top) {
//
//                /*
//                 * Don't invalidate the border so that we avoid flicker
//                 */
//                InflateRect(&rcEdit, -1, -1);
//            }
//            NtUserInvalidateRect(ped->hwnd, &rcEdit, FALSE);
            ECInvalidateClient(ped, FALSE);
        }

        ECNotifyParent(ped, EN_CHANGE);

        if (FWINABLE()) {
            NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        }
    }

    return TRUE;
}
