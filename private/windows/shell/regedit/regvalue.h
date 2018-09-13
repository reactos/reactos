/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGVALUE.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  ValueListWnd ListView routines for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGVALUE
#define _INC_REGVALUE

VOID
PASCAL
RegEdit_OnNewValue(
    HWND hWnd,
    DWORD Type
    );

VOID
PASCAL
RegEdit_OnValueListCommand(
    HWND hWnd,
    int MenuCommand
    );

VOID
PASCAL
RegEdit_OnValueListBeginDrag(
    HWND hWnd,
    NM_LISTVIEW FAR* lpNMListView
    );

BOOL
PASCAL
RegEdit_OnValueListBeginLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    );

BOOL
PASCAL
RegEdit_OnValueListEndLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    );

VOID
PASCAL
RegEdit_OnValueListCommand(
    HWND hWnd,
    int MenuCommand
    );

VOID
PASCAL
RegEdit_OnValueListContextMenu(
    HWND hWnd,
    BOOL fByAccelerator
    );

VOID
PASCAL
RegEdit_SetValueListEditMenuItems(
    HMENU hPopupMenu,
    int SelectedListIndex
    );

VOID
PASCAL
RegEdit_OnValueListModify(
    HWND hWnd
    );

VOID
PASCAL
RegEdit_OnValueListRefresh(
    HWND hWnd
    );

VOID
PASCAL
ValueList_SetItemDataText(
    HWND hValueListWnd,
    int ListIndex,
    PBYTE pValueData,
    DWORD cbValueData,
    DWORD Type
    );

#endif // _INC_REGVALUE
