/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGVALUE.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  ValueListWnd ListView routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regvalue.h"
#include "regstred.h"
#include "regbined.h"
#include "regdwded.h"
#include "regresid.h"

#define MAX_VALUENAME_TEMPLATE_ID       100

//  Maximum number of bytes that will be shown in the ListView.  If the user
//  wants to see more, then they can use the edit dialogs.
#define SIZE_DATATEXT                   196

//  Allow room in a SIZE_DATATEXT buffer for one null and possibly
//  the ellipsis.
#define MAXIMUM_STRINGDATATEXT          192
const TCHAR s_StringDataFormatSpec[] = TEXT("%.192s");

//  Allow room for multiple three character pairs, one null, and possibly the
//  ellipsis.
#define MAXIMUM_BINARYDATABYTES         64
const TCHAR s_BinaryDataFormatSpec[] = TEXT("%02x ");

const TCHAR s_Ellipsis[] = TEXT("...");

const LPCTSTR s_TypeNames[] = { TEXT("REG_NONE"),
                                TEXT("REG_SZ"),
                                TEXT("REG_EXPAND_SZ"),
                                TEXT("REG_BINARY"),
                                TEXT("REG_DWORD"),
                                TEXT("REG_DWORD_LITTLE_ENDIAN"),
                                TEXT("REG_LINK"),
                                TEXT("REG_MULTI_SZ"),
                                TEXT("REG_RESOURCE_LIST"),
                                TEXT("REG_FULL_RESOURCE_DESCRIPTOR"),
                                TEXT("REG_RESOURCE_REQUIREMENTS_LIST"),
                                TEXT("REG_QWORD")
                              };

#define MAX_KNOWN_TYPE REG_QWORD

VOID
PASCAL
RegEdit_OnValueListDelete(
    HWND hWnd
    );

VOID
PASCAL
RegEdit_OnValueListRename(
    HWND hWnd
    );

VOID
PASCAL
ValueList_EditLabel(
    HWND hValueListWnd,
    int ListIndex
    );

/*******************************************************************************
*
*  RegEdit_OnNewValue
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnNewValue(
    HWND hWnd,
    DWORD Type
    )
{

    UINT NewValueNameID;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD Ignore;
    DWORD cbValueData;
    LV_ITEM LVItem;
    int ListIndex;
    UINT ErrorStringID;

    if (g_RegEditData.hCurrentSelectionKey == NULL)
        return;

    //
    //  Loop through the registry trying to find a valid temporary name until
    //  the user renames the key.
    //

    NewValueNameID = 1;

    while (NewValueNameID < MAX_VALUENAME_TEMPLATE_ID) {

        wsprintf(ValueName, g_RegEditData.pNewValueTemplate, NewValueNameID);

        if (RegQueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName,
            NULL, &Ignore, NULL, &Ignore) != ERROR_SUCCESS) {

            //
            //  For strings, we need to have at least one byte to represent the
            //  null.  For binary data, it's okay to have zero-length data.
            //

            switch (Type) {

                case REG_SZ:
                    ((PTSTR)g_ValueDataBuffer)[0] = 0;
                    cbValueData = 1;
                    break;

                case REG_DWORD:
                    ((LPDWORD) g_ValueDataBuffer)[0] = 0;
                    cbValueData = sizeof(DWORD);
                    break;

                case REG_BINARY:
                    cbValueData = 0;
                    break;

            }

            if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, 0,
                Type, g_ValueDataBuffer, cbValueData) == ERROR_SUCCESS)
                break;

            else {

                ErrorStringID = IDS_NEWVALUECANNOTCREATE;
                goto error_ShowDialog;

            }

        }

        NewValueNameID++;

    }

    if (NewValueNameID == MAX_VALUENAME_TEMPLATE_ID) {

        ErrorStringID = IDS_NEWVALUENOUNIQUE;
        goto error_ShowDialog;

    }

    LVItem.mask = LVIF_TEXT | LVIF_IMAGE;
    LVItem.pszText = ValueName;
    LVItem.iItem = ListView_GetItemCount(g_RegEditData.hValueListWnd);
    LVItem.iSubItem = 0;
    LVItem.iImage = IsRegStringType(Type) ? IMAGEINDEX(IDI_STRING) :
        IMAGEINDEX(IDI_BINARY);

    if ((ListIndex = ListView_InsertItem(g_RegEditData.hValueListWnd,
        &LVItem)) != -1) {

        ValueList_SetItemDataText(g_RegEditData.hValueListWnd, ListIndex,
            g_ValueDataBuffer, cbValueData, Type);

        ValueList_EditLabel(g_RegEditData.hValueListWnd, ListIndex);

    }

    return;

error_ShowDialog:
    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_NEWVALUEERRORTITLE), MB_ICONERROR | MB_OK);

}

/*******************************************************************************
*
*  RegEdit_OnValueListBeginLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpLVDispInfo,
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnValueListBeginLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    )
{

    //
    //  B#7933:  We don't want the user to hurt themselves by making it too easy
    //  to rename keys and values.  Only allow renames via the menus.
    //

    //
    //  We don't get any information on the source of this editing action, so
    //  we must maintain a flag that tells us whether or not this is "good".
    //

    if (!g_RegEditData.fAllowLabelEdits)
        return TRUE;

    //
    //  All other labels are fair game.  We need to disable our keyboard
    //  accelerators so that the edit control can "see" them.
    //

    g_fDisableAccelerators = TRUE;

    return FALSE;

}

/*******************************************************************************
*
*  RegEdit_OnValueListEndLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnValueListEndLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    )
{

    HWND hValueListWnd;
    DWORD cbValueData;
    DWORD Ignore;
    DWORD Type;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    UINT ErrorStringID;

    //
    //  We can reenable our keyboard accelerators now that the edit control no
    //  longer needs to "see" them.
    //

    g_fDisableAccelerators = FALSE;

    hValueListWnd = g_RegEditData.hValueListWnd;

    //
    //  Check to see if the user cancelled the edit.  If so, we don't care so
    //  just return.
    //

    if (lpLVDispInfo-> item.pszText == NULL)
        return TRUE;

    ListView_GetItemText(hValueListWnd, lpLVDispInfo-> item.iItem, 0,
        ValueName, sizeof(ValueName)/sizeof(TCHAR));

    //
    //  Check to see if the new value name is empty
    //
    if (lpLVDispInfo->item.pszText[0] == 0) {
        ErrorStringID = IDS_RENAMEVALEMPTY;
        goto error_ShowDialog;
    }

    if (RegQueryValueEx(g_RegEditData.hCurrentSelectionKey, lpLVDispInfo->
        item.pszText, NULL, &Ignore, NULL, &Ignore) != ERROR_FILE_NOT_FOUND) {

        ErrorStringID = IDS_RENAMEVALEXISTS;
        goto error_ShowDialog;

    }

    cbValueData = sizeof(g_ValueDataBuffer);

    if (RegQueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, NULL,
        &Type, g_ValueDataBuffer, &cbValueData) != ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEVALOTHERERROR;
        goto error_ShowDialog;

    }

    if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, lpLVDispInfo->
        item.pszText, 0, Type, g_ValueDataBuffer, cbValueData) !=
        ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEVALOTHERERROR;
        goto error_ShowDialog;

    }

    if (RegDeleteValue(g_RegEditData.hCurrentSelectionKey, ValueName) !=
        ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEVALOTHERERROR;
        goto error_ShowDialog;

    }

    return TRUE;

error_ShowDialog:
    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_RENAMEVALERRORTITLE), MB_ICONERROR | MB_OK,
        (LPTSTR) ValueName);

    return FALSE;

}

/*******************************************************************************
*
*  RegEdit_OnValueListCommand
*
*  DESCRIPTION:
*     Handles the selection of a menu item by the user intended for the
*     ValueList child window.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     MenuCommand, identifier of menu command.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListCommand(
    HWND hWnd,
    int MenuCommand
    )
{

    //
    //  Check to see if this menu command should be handled by the main window's
    //  command handler.
    //

    if (MenuCommand >= ID_FIRSTMAINMENUITEM && MenuCommand <=
        ID_LASTMAINMENUITEM)
        RegEdit_OnCommand(hWnd, MenuCommand, NULL, 0);

    else {

        switch (MenuCommand) {

            case ID_CONTEXTMENU:
                RegEdit_OnValueListContextMenu(hWnd, TRUE);
                break;

            case ID_MODIFY:
                RegEdit_OnValueListModify(hWnd);
                break;

            case ID_DELETE:
                RegEdit_OnValueListDelete(hWnd);
                break;

            case ID_RENAME:
                RegEdit_OnValueListRename(hWnd);
                break;

        }

    }

}

/*******************************************************************************
*
*  RegEdit_OnValueListContextMenu
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListContextMenu(
    HWND hWnd,
    BOOL fByAccelerator
    )
{

    HWND hValueListWnd;
    DWORD MessagePos;
    POINT MessagePoint;
    LV_HITTESTINFO LVHitTestInfo;
    int ListIndex;
    UINT MenuID;
    HMENU hContextMenu;
    HMENU hContextPopupMenu;
    int MenuCommand;

    hValueListWnd = g_RegEditData.hValueListWnd;

    //
    //  If fByAcclerator is TRUE, then the user hit Shift-F10 to bring up the
    //  context menu.  Following the Cabinet's convention, this menu is
    //  placed at (0,0) of the ListView client area.
    //

    if (fByAccelerator) {

        MessagePoint.x = 0;
        MessagePoint.y = 0;

        ClientToScreen(hValueListWnd, &MessagePoint);

        ListIndex = ListView_GetNextItem(hValueListWnd, -1, LVNI_SELECTED);

    }

    else {

        MessagePos = GetMessagePos();
        MessagePoint.x = GET_X_LPARAM(MessagePos);
        MessagePoint.y = GET_Y_LPARAM(MessagePos);

        LVHitTestInfo.pt = MessagePoint;
        ScreenToClient(hValueListWnd, &LVHitTestInfo.pt);
        ListIndex = ListView_HitTest(hValueListWnd, &LVHitTestInfo);

    }

    MenuID = (ListIndex != -1) ? IDM_VALUE_CONTEXT :
        IDM_VALUELIST_NOITEM_CONTEXT;

    if ((hContextMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(MenuID))) == NULL)
        return;

    hContextPopupMenu = GetSubMenu(hContextMenu, 0);

    if (ListIndex != -1) {

        RegEdit_SetValueListEditMenuItems(hContextMenu, ListIndex);

        SetMenuDefaultItem(hContextPopupMenu, ID_MODIFY, MF_BYCOMMAND);

    }

        //  BUGBUG:  Fix constant
    else
        RegEdit_SetNewObjectEditMenuItems(GetSubMenu(hContextPopupMenu, 0));

    MenuCommand = TrackPopupMenuEx(hContextPopupMenu, TPM_RETURNCMD |
        TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, MessagePoint.x,
        MessagePoint.y, hWnd, NULL);

    DestroyMenu(hContextMenu);

    RegEdit_OnValueListCommand(hWnd, MenuCommand);

}

/*******************************************************************************
*
*  RegEdit_SetValueListEditMenuItems
*
*  DESCRIPTION:
*     Shared routine between the main menu and the context menu to setup the
*     edit menu items.
*
*  PARAMETERS:
*     hPopupMenu, handle of popup menu to modify.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_SetValueListEditMenuItems(
    HMENU hPopupMenu,
    int SelectedListIndex
    )
{

    UINT SelectedCount;
    UINT EnableFlags;

    SelectedCount = ListView_GetSelectedCount(g_RegEditData.hValueListWnd);

    //
    //  The edit option is only enabled when a single item is selected.  Note
    //  that this item is not in the main menu, but this should work fine.
    //

    if (SelectedCount == 1)
        EnableFlags = MF_ENABLED | MF_BYCOMMAND;
    else
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_MODIFY, EnableFlags);

    //
    //  The rename option is also only enabled when a single item is selected
    //  and that item cannot be the default item.  EnableFlags is already
    //  disabled if the SelectedCount is not one from above.
    //

    if (SelectedListIndex == 0)
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_RENAME, EnableFlags);

    //
    //  The delete option is only enabled when multiple items are selected.
    //

    if (SelectedCount > 0)
        EnableFlags = MF_ENABLED | MF_BYCOMMAND;
    else
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_DELETE, EnableFlags);

}

/*******************************************************************************
*
*  RegEdit_OnValueListModify
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListModify(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    UINT SelectedCount;
    int ListIndex;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    EDITVALUEPARAM EditValueParam;
    DWORD Type;
    LONG RegError;
    UINT TemplateID;
    DLGPROC lpDlgProc;
    UINT ErrorStringID;

    hValueListWnd = g_RegEditData.hValueListWnd;

    //
    //  Verify that we only have one item selected for editing and "notify" the
    //  user if this is not the case.  Don't beep if the user is just double-
    //  clicking on the background.
    //

    SelectedCount = ListView_GetSelectedCount(hValueListWnd);

    if (SelectedCount != 1) {

        if (SelectedCount > 0)
            MessageBeep(0);

        return;

    }

    //
    //  Determine which item we are to edit.
    //

    ListIndex = ListView_GetNextItem(hValueListWnd, -1, LVNI_SELECTED);

    ListView_GetItemText(hValueListWnd, ListIndex, 0, ValueName,
        sizeof(ValueName)/sizeof(TCHAR));

    //
    //  If this is the default value, zap the first byte to null so that we
    //  can do a RegQueryValueEx.  Note that below we replace this "zapped"
    //  byte.
    //

    if (ListIndex == 0)
        ValueName[0] = '\0';

    EditValueParam.pValueName = ValueName;
    EditValueParam.pValueData = g_ValueDataBuffer;
    EditValueParam.cbValueData = sizeof(g_ValueDataBuffer);

    // RegQueryValueEx on an empty string value is supposed to return the
    // size of the terminating NULL.  But the W version returns 1 instead 
    // of 2.  So we don't have a properly terminated Unicode string and
    // we can read past it into garbage.  Defend against this problem by
    // making sure the first WCHAR is nulled out

    ((TCHAR*)g_ValueDataBuffer)[0] = '\0';

    RegError = RegQueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName,
        NULL, &Type, g_ValueDataBuffer, &EditValueParam.cbValueData);

    if (RegError != ERROR_SUCCESS) {

        //
        //  If this is the default value, then the value may not really exist
        //  in the registry, so ignore ERROR_FILE_NOT_FOUND.  We always display
        //  the default value in the ListView regardless of its existence.
        //

        if (ListIndex != 0 || RegError != ERROR_FILE_NOT_FOUND) {

            ErrorStringID = IDS_EDITVALCANNOTREAD;
            goto error_ShowDialog;

        }

        //
        //  If the default value didn't exist, then Type isn't necessarily
        //  valid.  Here, we make sure it's a string which by definition must be
        //  true.
        //

        if (ListIndex == 0) {
            Type = REG_SZ;
            ((TCHAR*)g_ValueDataBuffer)[0] = 0;
        }

    }

    //
    //  This is kinda funky-- if this is the default value, then we zapped the
    //  first byte of "(Default Value") up above to do the RegQueryValueEx.
    //  Here we replace the byte just to avoid a string copy...
    //

    if (ListIndex == 0)
        ValueName[0] = g_RegEditData.pDefaultValue[0];

    switch (Type) {

        case REG_SZ:
        case REG_EXPAND_SZ:
            TemplateID = IDD_EDITSTRINGVALUE;
            lpDlgProc = EditStringValueDlgProc;
            break;

        case REG_DWORD:
            if (EditValueParam.cbValueData == sizeof(DWORD)) {

                TemplateID = IDD_EDITDWORDVALUE;
                lpDlgProc = EditDwordValueDlgProc;
                break;

            }
            //  FALL THROUGH

        case REG_BINARY:
        default:
            TemplateID = IDD_EDITBINARYVALUE;
            lpDlgProc = EditBinaryValueDlgProc;
            break;

    }

    if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(TemplateID), hWnd,
        lpDlgProc, (LPARAM) (LPEDITVALUEPARAM) &EditValueParam) == IDOK) {

        if (ListIndex == 0)
            ValueName[0] = 0;

        if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, 0,
            Type, EditValueParam.pValueData, EditValueParam.cbValueData) !=
            ERROR_SUCCESS) {

            ErrorStringID = IDS_EDITVALCANNOTWRITE;
            goto error_ShowDialog;

        }

        ValueList_SetItemDataText(hValueListWnd, ListIndex,
            EditValueParam.pValueData, EditValueParam.cbValueData, Type);

    }

    return;

error_ShowDialog:
    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_EDITVALERRORTITLE), MB_ICONERROR | MB_OK,
        (LPTSTR) ValueName);

}

/*******************************************************************************
*
*  RegEdit_OnValueListDelete
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListDelete(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    UINT ConfirmTextStringID;
    BOOL fErrorDeleting;
    int ListStartIndex;
    int ListIndex;
    TCHAR ValueName[MAXVALUENAME_LENGTH];

    hValueListWnd = g_RegEditData.hValueListWnd;

    ConfirmTextStringID =  (ListView_GetSelectedCount(hValueListWnd) == 1) ?
        IDS_CONFIRMDELVALTEXT : IDS_CONFIRMDELVALMULTITEXT;

    if (InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ConfirmTextStringID),
        MAKEINTRESOURCE(IDS_CONFIRMDELVALTITLE),  MB_ICONWARNING | MB_YESNO) !=
        IDYES)
        return;

    SetWindowRedraw(hValueListWnd, FALSE);

    fErrorDeleting = FALSE;
    ListStartIndex = -1;

    while ((ListIndex = ListView_GetNextItem(hValueListWnd, ListStartIndex,
        LVNI_SELECTED)) != -1) {

        if (ListIndex != 0) {

            ListView_GetItemText(hValueListWnd, ListIndex, 0, ValueName,
                sizeof(ValueName)/sizeof(TCHAR));

        }

        else
            ValueName[0] = 0;

        if (RegDeleteValue(g_RegEditData.hCurrentSelectionKey, ValueName) ==
            ERROR_SUCCESS) {

            if (ListIndex != 0)
                ListView_DeleteItem(hValueListWnd, ListIndex);

            else {

                ValueList_SetItemDataText(hValueListWnd, 0, NULL, 0, REG_SZ);

                ListStartIndex = 0;

            }

        }

        else {

            fErrorDeleting = TRUE;

            ListStartIndex = ListIndex;

        }

    }

    SetWindowRedraw(hValueListWnd, TRUE);

    if (fErrorDeleting)
        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(IDS_DELETEVALDELETEFAILED),
            MAKEINTRESOURCE(IDS_DELETEVALERRORTITLE), MB_ICONERROR | MB_OK);

}

/*******************************************************************************
*
*  RegEdit_OnValueListRename
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListRename(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    int ListIndex;

    hValueListWnd = g_RegEditData.hValueListWnd;

    if (ListView_GetSelectedCount(hValueListWnd) == 1 && (ListIndex =
        ListView_GetNextItem(hValueListWnd, -1, LVNI_SELECTED)) != 0)
        ValueList_EditLabel(g_RegEditData.hValueListWnd, ListIndex);

}

/*******************************************************************************
*
*  RegEdit_OnValueListRefresh
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListRefresh(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    LV_ITEM LVItem;
    DWORD cbValueName;
    DWORD cbValueData;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD Type;
    LONG PrevStyle;
    DWORD EnumIndex;
    BOOL fInsertedDefaultValue;
    int ListIndex;

    hValueListWnd = g_RegEditData.hValueListWnd;

    RegEdit_SetWaitCursor(TRUE);
    SetWindowRedraw(hValueListWnd, FALSE);

    ListView_DeleteAllItems(hValueListWnd);

    if (g_RegEditData.hCurrentSelectionKey != NULL) {

        LVItem.mask = LVIF_TEXT | LVIF_IMAGE;
        LVItem.pszText = ValueName;
        LVItem.iSubItem = 0;

        PrevStyle = SetWindowLong(hValueListWnd, GWL_STYLE,
            GetWindowLong(hValueListWnd, GWL_STYLE) | LVS_SORTASCENDING);

        EnumIndex = 0;
        fInsertedDefaultValue = FALSE;

        while (TRUE) {

            cbValueName = sizeof(ValueName);
            cbValueData = sizeof(g_ValueDataBuffer);

            // RegQueryValueEx on an empty string value is supposed to return the
            // size of the terminating NULL.  But the W version returns 1 instead 
            // of 2.  So we don't have a properly terminated Unicode string and
            // we can read past it into garbage.  Defend against this problem by
            // making sure the first WCHAR is nulled out

            ((TCHAR*)g_ValueDataBuffer)[0] = '\0';

            if (RegEnumValue(g_RegEditData.hCurrentSelectionKey, EnumIndex++,
                ValueName, &cbValueName, NULL, &Type, g_ValueDataBuffer,
                &cbValueData) != ERROR_SUCCESS)
                break;

            if (cbValueName == 0)
                fInsertedDefaultValue = TRUE;

            LVItem.iImage = IsRegStringType(Type) ? IMAGEINDEX(IDI_STRING) :
                IMAGEINDEX(IDI_BINARY);

            ListIndex = ListView_InsertItem(hValueListWnd, &LVItem);

            ValueList_SetItemDataText(hValueListWnd, ListIndex,
                g_ValueDataBuffer, cbValueData, Type);

        }

        SetWindowLong(hValueListWnd, GWL_STYLE, PrevStyle);

        LVItem.iItem = 0;
        LVItem.pszText = g_RegEditData.pDefaultValue;
        LVItem.iImage = IMAGEINDEX(IDI_STRING);

        if (fInsertedDefaultValue) {

            LVItem.mask = LVIF_TEXT;
            ListView_SetItem(hValueListWnd, &LVItem);

        }

        else {

            ListView_InsertItem(hValueListWnd, &LVItem);
            ValueList_SetItemDataText(hValueListWnd, 0, NULL, 0, REG_SZ);

        }

        ListView_SetItemState(hValueListWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);

    }

    SetWindowRedraw(hValueListWnd, TRUE);
    RegEdit_SetWaitCursor(FALSE);

}

/*******************************************************************************
*
*  ValueList_SetItemDataText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hValueListWnd, handle of ValueList window.
*     ListIndex, index into ValueList window.
*     pValueData, pointer to buffer containing data.
*     cbValueData, size of the above buffer.
*     Type, type of data this buffer contains (REG_* definition).
*
*******************************************************************************/

VOID
PASCAL
ValueList_SetItemDataText(
    HWND hValueListWnd,
    int ListIndex,
    PBYTE pValueData,
    DWORD cbValueData,
    DWORD Type
    )
{

    BOOL fMustDeleteString;
    TCHAR DataText[SIZE_DATATEXT];
    int BytesToWrite;
    PTSTR pString;

    fMustDeleteString = FALSE;

    //
    //  When pValueData is NULL, then that's a special indicator to us that this
    //  is the default value and it's value is undefined.
    //

    if (pValueData == NULL)
        pString = g_RegEditData.pValueNotSet;

    else if ((Type == REG_SZ) || (Type == REG_EXPAND_SZ)) {

        wsprintf(DataText, s_StringDataFormatSpec, (LPTSTR) pValueData);

        if ((cbValueData/sizeof(TCHAR)) > MAXIMUM_STRINGDATATEXT + 1)           //  for null
            lstrcat(DataText, s_Ellipsis);

        pString = DataText;

    }

    else if (Type == REG_DWORD) {

        //  BUGBUG:  Check for invalid cbValueData!
        if (cbValueData == sizeof(DWORD))
            pString = LoadDynamicString(IDS_DWORDDATAFORMATSPEC,
                ((LPDWORD) g_ValueDataBuffer)[0]);
        else
            pString = LoadDynamicString(IDS_INVALIDDWORDDATA);

        fMustDeleteString = TRUE;

    }
 
    else if (Type == REG_MULTI_SZ) {

        int CharsAvailableInBuffer;
        int ComponentLength;
        PTCHAR Start;

        ZeroMemory(DataText,sizeof(DataText));
        CharsAvailableInBuffer = MAXIMUM_STRINGDATATEXT+1;
        Start = DataText;
        for(pString=(PTSTR)pValueData; *pString; pString+=ComponentLength+1) {

            ComponentLength = lstrlen(pString);

            //
            // Quirky behavior of lstrcpyn is exactly what we need here.
            //
            if(CharsAvailableInBuffer > 0) {
                lstrcpyn(Start,pString,CharsAvailableInBuffer);
                Start += ComponentLength;
            }

            CharsAvailableInBuffer -= ComponentLength;

            if(CharsAvailableInBuffer > 0) {
                lstrcpyn(Start,TEXT(" "),CharsAvailableInBuffer);
                Start += 1;
            }

            CharsAvailableInBuffer -= 1;
        }

        if(CharsAvailableInBuffer < 0) {
            lstrcpy(DataText+MAXIMUM_STRINGDATATEXT,s_Ellipsis);
        }

        pString = DataText;
    }
 
    else {

        if (cbValueData == 0)
            pString = g_RegEditData.pEmptyBinary;

        else {

            BytesToWrite = min(cbValueData, MAXIMUM_BINARYDATABYTES);

            pString = DataText;

            while (BytesToWrite--)
                pString += wsprintf(pString, s_BinaryDataFormatSpec,
                    (BYTE) *pValueData++);

            *(--pString) = 0;

            if (cbValueData > MAXIMUM_BINARYDATABYTES)
                lstrcpy(pString, s_Ellipsis);

            pString = DataText;

        }

    }

    if(Type <= MAX_KNOWN_TYPE) {
        ListView_SetItemText(hValueListWnd, ListIndex, 1, (LPTSTR)s_TypeNames[Type]);
    } else {
        TCHAR TypeString[24];

        wsprintf(TypeString,TEXT("0x%x"),Type);
        ListView_SetItemText(hValueListWnd, ListIndex, 1, TypeString);
    }

    ListView_SetItemText(hValueListWnd, ListIndex, 2, pString);

    if (fMustDeleteString)
        DeleteDynamicString(pString);

}

/*******************************************************************************
*
*  ValueList_EditLabel
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hValueListWnd, handle of ValueList window.
*     ListIndex, index of item to edit.
*
*******************************************************************************/

VOID
PASCAL
ValueList_EditLabel(
    HWND hValueListWnd,
    int ListIndex
    )
{

    g_RegEditData.fAllowLabelEdits = TRUE;

    //
    //  We have to set the focus to the ListView or else ListView_EditLabel will
    //  return FALSE.  While we're at it, clear the selected state of all the
    //  items to eliminate some flicker when we move the focus back to this
    //  pane.
    //

    if (hValueListWnd != g_RegEditData.hFocusWnd) {

        ListView_SetItemState(hValueListWnd, -1, 0, LVIS_SELECTED |
            LVIS_FOCUSED);

        SetFocus(hValueListWnd);

    }

    ListView_EditLabel(hValueListWnd, ListIndex);

    g_RegEditData.fAllowLabelEdits = FALSE;

}
