/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGKEY.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  KeyTreeWnd TreeView routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regkey.h"
#include "regvalue.h"
#include "regresid.h"

#define MAX_KEYNAME_TEMPLATE_ID         100

#define SELCHANGE_TIMER_ID              1

#define REFRESH_DPA_GROW                16

VOID
PASCAL
RegEdit_OnKeyTreeDelete(
    HWND hWnd,
    HTREEITEM hTreeItem
    );

VOID
PASCAL
RegEdit_OnKeyTreeRename(
    HWND hWnd,
    HTREEITEM hTreeItem
    );

int
WINAPI
DPACompareKeyNames(
    LPVOID lpString1,
    LPVOID lpString2,
    LPARAM lParam
    );

HTREEITEM
PASCAL
KeyTree_InsertItem(
    HWND hKeyTreeWnd,
    HTREEITEM hParent,
    HTREEITEM hInsertAfter,
    LPCTSTR lpText,
    UINT fHasKids,
    LPARAM lParam
    );

BOOL
PASCAL
DoesKeyHaveKids(
    HKEY hKey,
    LPTSTR lpKeyName
    );

VOID
PASCAL
KeyTree_EditLabel(
    HWND hKeyTreeWnd,
    HTREEITEM hTreeItem
    );

BOOL
PASCAL
KeyTree_CanDeleteOrRenameItem(
    HWND hWnd,
    HTREEITEM hTreeItem
    );

/*******************************************************************************
*
*  RegEdit_OnNewKey
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnNewKey(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    TCHAR KeyName[MAXKEYNAME];
    HKEY hRootKey;
    HKEY hKey;
    UINT ErrorStringID;
    BOOL fNewKeyIsOnlyChild;
    UINT NewKeyNameID;
    HKEY hNewKey;
    HTREEITEM hNewTreeItem;
    TV_ITEM TVItem;

    hRootKey = KeyTree_BuildKeyPath(g_RegEditData.hKeyTreeWnd, hTreeItem,
        KeyName, BKP_TOSUBKEY);

    if(RegOpenKeyEx(hRootKey,KeyName,0,KEY_CREATE_SUB_KEY,&hKey) != ERROR_SUCCESS) {

        //
        //  Get the text of the selected tree item so that we can display
        //  a more meaningful error message.
        //

        TVItem.mask = TVIF_TEXT;
        TVItem.hItem = hTreeItem;
        TVItem.pszText = (LPTSTR) KeyName;
        TVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

        TreeView_GetItem(g_RegEditData.hKeyTreeWnd, &TVItem);

        ErrorStringID = IDS_NEWKEYPARENTOPENFAILED;
        goto error_ShowDialog;

    }

    TVItem.mask = TVIF_STATE | TVIF_CHILDREN;
    TVItem.hItem = hTreeItem;
    TreeView_GetItem(g_RegEditData.hKeyTreeWnd, &TVItem);

    fNewKeyIsOnlyChild = FALSE;

    if (TVItem.cChildren == FALSE) {

        //
        //  The selected key doesn't have any subkeys, so we can't do an expand
        //  on it just yet.  We'll just set a flag and later tag it with a
        //  plus/minus icon and expand it.
        //

        fNewKeyIsOnlyChild = TRUE;

    }

    else if (!(TVItem.state & TVIS_EXPANDED)) {

        //
        //  The selected key isn't expanded.  Do it now so that we can do an
        //  in-place edit and don't reenumerate the "New Key #xxx" after we do
        //  the RegCreateKey.
        //

        TreeView_Expand(g_RegEditData.hKeyTreeWnd, hTreeItem, TVE_EXPAND);

    }

    //
    //  Loop through the registry trying to find a valid temporary name until
    //  the user renames the key.
    //

    NewKeyNameID = 1;

    while (NewKeyNameID < MAX_KEYNAME_TEMPLATE_ID) {

        wsprintf(KeyName, g_RegEditData.pNewKeyTemplate, NewKeyNameID);

        if(RegOpenKeyEx(hKey,KeyName,0,0,&hNewKey) == ERROR_FILE_NOT_FOUND) {

            if (RegCreateKey(hKey, KeyName, &hNewKey) == ERROR_SUCCESS)
                break;

            else {

                ErrorStringID = IDS_NEWKEYCANNOTCREATE;
                goto error_CloseKey;

            }

        }

        NewKeyNameID++;

    }

    RegCloseKey(hKey);

    if (NewKeyNameID == MAX_KEYNAME_TEMPLATE_ID) {

        ErrorStringID = IDS_NEWKEYNOUNIQUE;
        goto error_ShowDialog;

    }

    RegCloseKey(hNewKey);

    if (fNewKeyIsOnlyChild) {

        TVItem.mask = TVIF_CHILDREN;
        TVItem.cChildren = TRUE;
        TreeView_SetItem(g_RegEditData.hKeyTreeWnd, &TVItem);

        TreeView_Expand(g_RegEditData.hKeyTreeWnd, hTreeItem, TVE_EXPAND);

        //  BUGBUG:  It is possible for our new item _not_ to be the only child
        //  if our view is out of date!
        hNewTreeItem = TreeView_GetChild(g_RegEditData.hKeyTreeWnd, hTreeItem);

    }

    else {

        hNewTreeItem = KeyTree_InsertItem(g_RegEditData.hKeyTreeWnd, hTreeItem,
            TVI_LAST, KeyName, FALSE, 0);

    }

    TreeView_SelectItem(g_RegEditData.hKeyTreeWnd, hNewTreeItem);
    KeyTree_EditLabel(g_RegEditData.hKeyTreeWnd, hNewTreeItem);

    return;

error_CloseKey:
    RegCloseKey(hKey);

    //  BUGBUG:  For any errors that may crop up, we may need to turn off the
    //  child flag.

error_ShowDialog:
    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_NEWKEYERRORTITLE), MB_ICONERROR | MB_OK,
        (LPTSTR) KeyName);

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeItemExpanding
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpNMTreeView, TreeView notification data.
*
*******************************************************************************/

LRESULT
PASCAL
RegEdit_OnKeyTreeItemExpanding(
    HWND hWnd,
    LPNM_TREEVIEW lpNMTreeView
    )
{

    HWND hKeyTreeWnd;
    HTREEITEM hExpandingTreeItem;
    TCHAR KeyName[MAXKEYNAME];
    TV_ITEM TVItem;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;
    hExpandingTreeItem = lpNMTreeView-> itemNew.hItem;

    //
    //  Check if we're to expand the given tree item for the first time.  If so,
    //  delve into the registry to get all of the key's subkeys.
    //

    if (lpNMTreeView-> action & TVE_EXPAND && !(lpNMTreeView-> itemNew.state &
        TVIS_EXPANDEDONCE)) {

        if (TreeView_GetChild(hKeyTreeWnd, hExpandingTreeItem) != NULL)
            return FALSE;

        RegEdit_SetWaitCursor(TRUE);

        if (!KeyTree_ExpandBranch(hKeyTreeWnd, hExpandingTreeItem)) {

            //
            //  Get the text of the selected tree item so that we can display
            //  a more meaningful error message.
            //

            TVItem.mask = TVIF_TEXT;
            TVItem.hItem = hExpandingTreeItem;
            TVItem.pszText = (LPTSTR) KeyName;
            TVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

            TreeView_GetItem(hKeyTreeWnd, &TVItem);

            InternalMessageBox(g_hInstance, hWnd,
                MAKEINTRESOURCE(IDS_OPENKEYCANNOTOPEN),
                MAKEINTRESOURCE(IDS_OPENKEYERRORTITLE), MB_ICONERROR | MB_OK,
                (LPTSTR) KeyName);

        }

	RegEdit_SetWaitCursor(FALSE);

    }

    return FALSE;

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeSelChanged
*
*  DESCRIPTION:
*     Depending on how the user has selected the new item in the KeyTreeWnd,
*     we call to the real worker routine, RegEdit_KeyTreeSelChanged, or delay
*     the call for several milliseconds.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpNMTreeView, TreeView notification data.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeSelChanged(
    HWND hWnd,
    LPNM_TREEVIEW lpNMTreeView
    )
{

    UINT TimerDelay;

    //
    //  We delay the actual update of the selection and thus of the
    //  ValueListWnd for several milliseconds.  This avoids unnecessary flashing
    //  as the user scrolls through the tree.  (This behavior is directly taken
    //	from the Explorer.)
    //

    switch (g_RegEditData.SelChangeTimerState) {

        case SCTS_TIMERSET:
            KillTimer(hWnd, SELCHANGE_TIMER_ID);
            //  FALL THROUGH

        case SCTS_TIMERCLEAR:
#ifdef WINNT
        //
        // This behavior is extremely annoying so I am changing it.
        //
	    TimerDelay = 1;
#else
	    TimerDelay = (lpNMTreeView != NULL && lpNMTreeView-> action ==
		TVC_BYMOUSE) ? (1) : (GetDoubleClickTime() * 3 / 2);
#endif
	    SetTimer(hWnd, SELCHANGE_TIMER_ID, TimerDelay, NULL);
            g_RegEditData.SelChangeTimerState = SCTS_TIMERSET;
            break;

        //
        //  We want to punt the first selection change notification that comes
        //  through.
        //

        case SCTS_INITIALIZING:
            RegEdit_KeyTreeSelChanged(hWnd);
            break;

    }

}

/*******************************************************************************
*
*  RegEdit_OnSelChangedTimer
*
*  DESCRIPTION:
*     Called several milliseconds after a keyboard operation has selected a new
*     item in the KeyTreeWnd.  Act as if a new selection has just been made in
*     the KeyTreeWnd.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnSelChangedTimer(
    HWND hWnd
    )
{

    KillTimer(hWnd, SELCHANGE_TIMER_ID);
    g_RegEditData.SelChangeTimerState = SCTS_TIMERCLEAR;

    RegEdit_KeyTreeSelChanged(hWnd);

}

/*******************************************************************************
*
*  RegEdit_KeyTreeSelChanged
*
*  DESCRIPTION:
*     Called after a new item has been selected in the KeyTreeWnd.  Opens a
*     registry key to the new branch and notifies the ValueListWnd to update
*     itself.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_KeyTreeSelChanged(
    HWND hWnd
    )
{

    HWND hKeyTreeWnd;
    HTREEITEM hSelectedTreeItem;
    RECT ItemRect;
    RECT ClientRect;
    RECT FromRect;
    RECT ToRect;
    HKEY hRootKey;
    TCHAR KeyName[MAXKEYNAME];
    TV_ITEM TVItem;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;
    hSelectedTreeItem = TreeView_GetSelection(hKeyTreeWnd);

    if (g_RegEditData.SelChangeTimerState != SCTS_INITIALIZING) {

        //
        //  Draw an animation that shows the "expansion" of the newly selected
        //  tree item to the ListView.
        //

        TreeView_GetItemRect(hKeyTreeWnd, hSelectedTreeItem, &ItemRect, TRUE);
        GetClientRect(hKeyTreeWnd, &ClientRect);
        IntersectRect(&FromRect, &ClientRect, &ItemRect);
        MapWindowPoints(hKeyTreeWnd, hWnd, (LPPOINT) &FromRect, 2);

        GetWindowRect(g_RegEditData.hValueListWnd, &ToRect);
        MapWindowPoints(NULL, hWnd, (LPPOINT) &ToRect, 2);

        DrawAnimatedRects(hWnd, IDANI_OPEN, &FromRect, &ToRect);

    }

    //
    //  Close the previously selected item's key handle, if appropriate.
    //

    if (g_RegEditData.hCurrentSelectionKey != NULL) {

        RegCloseKey(g_RegEditData.hCurrentSelectionKey);
        g_RegEditData.hCurrentSelectionKey = NULL;

    }

    RegEdit_UpdateStatusBar();

    //
    //  Simple case-- we're changing to one of the top-level labels, such as
    //  "My Computer" or a network computer name.  Right now, nothing is
    //  displayed in the ListView, so just empty it and return.
    //

    if (TreeView_GetParent(hKeyTreeWnd, hSelectedTreeItem) != NULL) {

        //
        //  Build a registry path to the selected tree item and open a registry
        //  key.
        //

        hRootKey = KeyTree_BuildKeyPath(hKeyTreeWnd, hSelectedTreeItem,
            KeyName, BKP_TOSUBKEY);

        if(RegOpenKeyEx(hRootKey,KeyName, 0, MAXIMUM_ALLOWED,
            &g_RegEditData.hCurrentSelectionKey) != ERROR_SUCCESS) {

            //
            //  Get the text of the selected tree item so that we can display
            //  a more meaningful error message.
            //

            TVItem.mask = TVIF_TEXT;
            TVItem.hItem = hSelectedTreeItem;
            TVItem.pszText = (LPTSTR) KeyName;
            TVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

            TreeView_GetItem(hKeyTreeWnd, &TVItem);

            InternalMessageBox(g_hInstance, hWnd,
                MAKEINTRESOURCE(IDS_OPENKEYCANNOTOPEN),
                MAKEINTRESOURCE(IDS_OPENKEYERRORTITLE), MB_ICONERROR | MB_OK,
                (LPTSTR) KeyName);

        }

    }

    RegEdit_OnValueListRefresh(hWnd);

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeBeginLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpTVDispInfo,
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnKeyTreeBeginLabelEdit(
    HWND hWnd,
    TV_DISPINFO FAR* lpTVDispInfo
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
*  RegEdit_OnKeyTreeEndLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpTVDispInfo,
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnKeyTreeEndLabelEdit(
    HWND hWnd,
    TV_DISPINFO FAR* lpTVDispInfo
    )
{

    HWND hKeyTreeWnd;
    HKEY hRootKey;
    TCHAR SourceKeyName[MAXKEYNAME];
    TCHAR DestinationKeyName[MAXKEYNAME];
    HKEY hSourceKey;
    HKEY hDestinationKey;
    LPTSTR lpEndOfParentKey;
    UINT ErrorStringID;
    TV_ITEM TVItem;

    //
    //  We can reenable our keyboard accelerators now that the edit control no
    //  longer needs to "see" them.
    //

    g_fDisableAccelerators = FALSE;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    //
    //  Check to see if the user cancelled the edit.  If so, we don't care so
    //  just return.
    //

    if (lpTVDispInfo-> item.pszText == NULL)
        return FALSE;

    //
    //  Attempt to open the key to be renamed.  This may or may not be the same
    //  key that is already open.
    //

    hRootKey = KeyTree_BuildKeyPath(hKeyTreeWnd, lpTVDispInfo-> item.hItem,
        SourceKeyName, BKP_TOSUBKEY);

    if(RegOpenKeyEx(hRootKey,SourceKeyName,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSourceKey) != ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEKEYOTHERERROR;
        goto error_ShowDialog;

    }

    //
    //  Take the full path name of the key (relative to a predefined root key)
    //  and replace the old key name with the new.  Make sure that this key
    //  doesn't exceed our internal buffers.
    //

    lstrcpy(DestinationKeyName, SourceKeyName);

    if ((lpEndOfParentKey = StrRChr(DestinationKeyName, NULL, TEXT('\\'))) != NULL)
        lpEndOfParentKey++;

    else
        lpEndOfParentKey = DestinationKeyName;

    *lpEndOfParentKey = 0;

    if (lstrlen(DestinationKeyName) + lstrlen(lpTVDispInfo-> item.pszText) >=
        MAXKEYNAME) {

        ErrorStringID = IDS_RENAMEKEYTOOLONG;
        goto error_CloseSourceKey;

    }

    lstrcpy(lpEndOfParentKey, lpTVDispInfo-> item.pszText);

    //
    //  Make sure there are no backslashes in the name.
    //

    if (StrChr(lpEndOfParentKey, TEXT('\\')) != NULL) {

        ErrorStringID = IDS_RENAMEKEYBADCHARS;
        goto error_CloseSourceKey;

    }

    //
    //  Make sure there the name isn't empty
    //

    if (DestinationKeyName[0] == 0) {
        ErrorStringID = IDS_RENAMEKEYEMPTY;
        goto error_CloseSourceKey;
    }

    //
    //  Make sure that the destination doesn't already exist.
    //
    if(RegOpenKeyEx(hRootKey,DestinationKeyName,0,KEY_QUERY_VALUE,&hDestinationKey) ==
        ERROR_SUCCESS) {

        RegCloseKey(hDestinationKey);

        ErrorStringID = IDS_RENAMEKEYEXISTS;
        goto error_CloseSourceKey;

    }

    //
    //  Create the destination key and do the copy.
    //

    if (RegCreateKey(hRootKey, DestinationKeyName, &hDestinationKey) !=
        ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEKEYOTHERERROR;
        goto error_CloseSourceKey;

    }

    //  BUGBUG:  Check this return (when it gets one!)
    CopyRegistry(hSourceKey, hDestinationKey);

    RegCloseKey(hSourceKey);

    //
    //  Check to see if we're renaming the currently selected key.  If so, toss
    //  our cached key handle and change to our source key.
    //

    if (TreeView_GetSelection(hKeyTreeWnd) == lpTVDispInfo-> item.hItem) {

        RegCloseKey(g_RegEditData.hCurrentSelectionKey);

        g_RegEditData.hCurrentSelectionKey = hDestinationKey;

        //
        //  We can't just call RegEdit_UpdateStatusBar here... the tree item
        //  won't be updated until we return TRUE from this message.  So we must
        //  post a message to tell ourselves to do the update later on.
        //

        PostMessage(hWnd, REM_UPDATESTATUSBAR, 0, 0);

    }

    else
        RegCloseKey(hDestinationKey);

    if (RegDeleteKeyRecursive(hRootKey, SourceKeyName) != ERROR_SUCCESS) {

        ErrorStringID = IDS_RENAMEKEYOTHERERROR;
        goto error_CloseSourceKey;

    }

    return TRUE;

error_CloseSourceKey:
    RegCloseKey(hSourceKey);

error_ShowDialog:
    TVItem.hItem = lpTVDispInfo-> item.hItem;
    TVItem.mask = TVIF_TEXT;
    TVItem.pszText = SourceKeyName;
    TVItem.cchTextMax = sizeof(SourceKeyName)/sizeof(TCHAR);

    TreeView_GetItem(hKeyTreeWnd, &TVItem);

    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_RENAMEKEYERRORTITLE), MB_ICONERROR | MB_OK,
        (LPTSTR) SourceKeyName);

    return FALSE;

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeCommand
*
*  DESCRIPTION:
*     Handles the selection of a menu item by the user intended for the
*     KeyTree child window.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     MenuCommand, identifier of menu command.
*     hTreeItem,
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeCommand(
    HWND hWnd,
    int MenuCommand,
    HTREEITEM hTreeItem
    )
{

    HWND hKeyTreeWnd;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    //
    //  Assume that the we mean the current selection if we're to dispatch a
    //  command that requires a tree item.  This is necessary because the tree
    //  control will let you activate the context menu of one tree item while
    //  another one is really the selected tree item.
    //

    if (hTreeItem == NULL)
        hTreeItem = TreeView_GetSelection(hKeyTreeWnd);

    switch (MenuCommand) {

        case ID_CONTEXTMENU:
            RegEdit_OnKeyTreeContextMenu(hWnd, TRUE);
            break;

        case ID_TOGGLE:
            TreeView_Expand(hKeyTreeWnd, hTreeItem, TVE_TOGGLE);
            break;

        case ID_DELETE:
            RegEdit_OnKeyTreeDelete(hWnd, hTreeItem);
            break;

        case ID_RENAME:
            RegEdit_OnKeyTreeRename(hWnd, hTreeItem);
            break;

        case ID_DISCONNECT:
            RegEdit_OnKeyTreeDisconnect(hWnd, hTreeItem);
            break;

        case ID_COPYKEYNAME:
            RegEdit_OnCopyKeyName(hWnd, hTreeItem);
            break;

        case ID_NEWKEY:
            RegEdit_OnNewKey(hWnd, hTreeItem);
            break;

        case ID_NEWSTRINGVALUE:
        case ID_NEWBINARYVALUE:
            if (hTreeItem != TreeView_GetSelection(hKeyTreeWnd)) {

                //
                //  Force the selection to occur now, so that we're dealing
                //  with the right open key.
                //

                TreeView_SelectItem(hKeyTreeWnd, hTreeItem);
                RegEdit_OnSelChangedTimer(hWnd);

            }
            //  FALL THROUGH

        default:
            //
            //  Check to see if this menu command should be handled by the main
            //  window's command handler.
            //

            if (MenuCommand >= ID_FIRSTMAINMENUITEM && MenuCommand <=
                ID_LASTMAINMENUITEM)
                RegEdit_OnCommand(hWnd, MenuCommand, NULL, 0);
            break;

    }

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeContextMenu
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeContextMenu(
    HWND hWnd,
    BOOL fByAccelerator
    )
{

    HWND hKeyTreeWnd;
    DWORD MessagePos;
    POINT MessagePoint;
    TV_HITTESTINFO TVHitTestInfo;
    UINT MenuID;
    HMENU hContextMenu;
    HMENU hContextPopupMenu;
    TV_ITEM TVItem;
    int MenuCommand;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    //
    //  If fByAcclerator is TRUE, then the user hit Shift-F10 to bring up the
    //  context menu.  Following the Cabinet's convention, this menu is
    //  placed at (0,0) of the KeyTree client area.
    //

    if (fByAccelerator) {

        MessagePoint.x = 0;
        MessagePoint.y = 0;

        ClientToScreen(hKeyTreeWnd, &MessagePoint);

        TVItem.hItem = TreeView_GetSelection(hKeyTreeWnd);

    }

    else {

        MessagePos = GetMessagePos();

        MessagePoint.x = GET_X_LPARAM(MessagePos);
        MessagePoint.y = GET_Y_LPARAM(MessagePos);

        TVHitTestInfo.pt = MessagePoint;
        ScreenToClient(hKeyTreeWnd, &TVHitTestInfo.pt);
        TVItem.hItem = TreeView_HitTest(hKeyTreeWnd, &TVHitTestInfo);

    }

    //
    //  Determine which context menu to use and load it up.
    //

    if (TVItem.hItem == NULL)
        return;     //  No context menu for now

    else {

        if (TreeView_GetParent(hKeyTreeWnd, TVItem.hItem) == NULL)
            MenuID = IDM_COMPUTER_CONTEXT;

        else
            MenuID = IDM_KEY_CONTEXT;

    }

    if ((hContextMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(MenuID))) == NULL)
        return;

    hContextPopupMenu = GetSubMenu(hContextMenu, 0);

    TVItem.mask = TVIF_STATE | TVIF_CHILDREN;
    TreeView_GetItem(hKeyTreeWnd, &TVItem);

    if (TVItem.state & TVIS_EXPANDED)
        ModifyMenu(hContextPopupMenu, ID_TOGGLE, MF_BYCOMMAND | MF_STRING,
            ID_TOGGLE, g_RegEditData.pCollapse);

    if (MenuID == IDM_COMPUTER_CONTEXT) {

        if (g_RegEditData.fHaveNetwork) {

            if (TreeView_GetPrevSibling(hKeyTreeWnd, TVItem.hItem) == NULL)
                EnableMenuItem(hContextPopupMenu, ID_DISCONNECT, MF_GRAYED |
                    MF_BYCOMMAND);

        }

        else {

            DeleteMenu(hContextPopupMenu, ID_DISCONNECT, MF_BYCOMMAND);
            DeleteMenu(hContextPopupMenu, ID_NETSEPARATOR, MF_BYCOMMAND);

        }

    }

    else {

        RegEdit_SetKeyTreeEditMenuItems(hContextPopupMenu, TVItem.hItem);

        if (TVItem.cChildren == 0)
            EnableMenuItem(hContextPopupMenu, ID_TOGGLE, MF_GRAYED |
                MF_BYCOMMAND);

    }

    SetMenuDefaultItem(hContextPopupMenu, ID_TOGGLE, MF_BYCOMMAND);

    MenuCommand = TrackPopupMenuEx(hContextPopupMenu, TPM_RETURNCMD |
        TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, MessagePoint.x,
        MessagePoint.y, hWnd, NULL);

    DestroyMenu(hContextMenu);

    RegEdit_OnKeyTreeCommand(hWnd, MenuCommand, TVItem.hItem);

}

/*******************************************************************************
*
*  RegEdit_SetKeyTreeEditMenuItems
*
*  DESCRIPTION:
*     Shared routine between the main menu and the context menu to setup the
*     edit menu items.
*
*  PARAMETERS:
*     hPopupMenu, handle of popup menu to modify.
*     hTreeItem, handle of selected tree item.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_SetKeyTreeEditMenuItems(
    HMENU hPopupMenu,
    HTREEITEM hSelectedTreeItem
    )
{

    UINT EnableFlags;

    EnableFlags = KeyTree_CanDeleteOrRenameItem(g_RegEditData.hKeyTreeWnd,
        hSelectedTreeItem) ? (MF_ENABLED | MF_BYCOMMAND) :
        (MF_GRAYED | MF_BYCOMMAND);

    EnableMenuItem(hPopupMenu, ID_DELETE, EnableFlags);
    EnableMenuItem(hPopupMenu, ID_RENAME, EnableFlags);

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeDelete
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeDelete(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    HWND hKeyTreeWnd;
    HKEY hRootKey;
    TCHAR KeyName[MAXKEYNAME];
    HTREEITEM hParentTreeItem;
    TV_ITEM TVItem;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    if (!KeyTree_CanDeleteOrRenameItem(hKeyTreeWnd, hTreeItem))
        return;

    if (InternalMessageBox(g_hInstance, hWnd,
        MAKEINTRESOURCE(IDS_CONFIRMDELKEYTEXT),
        MAKEINTRESOURCE(IDS_CONFIRMDELKEYTITLE), MB_ICONWARNING | MB_YESNO) !=
        IDYES)
        return;

    if (hTreeItem == TreeView_GetSelection(hKeyTreeWnd)) {

        if (g_RegEditData.hCurrentSelectionKey != NULL) {

            RegCloseKey(g_RegEditData.hCurrentSelectionKey);
            g_RegEditData.hCurrentSelectionKey = NULL;

        }

    }

    hRootKey = KeyTree_BuildKeyPath(hKeyTreeWnd, hTreeItem, KeyName,
        BKP_TOSUBKEY);

    if (RegDeleteKeyRecursive(hRootKey, KeyName) == ERROR_SUCCESS) {

        SetWindowRedraw(hKeyTreeWnd, FALSE);

        hParentTreeItem = TreeView_GetParent(hKeyTreeWnd, hTreeItem);

        TreeView_DeleteItem(hKeyTreeWnd, hTreeItem);

        //
        //  See if the key that we just deleted was the last child of its
        //  parent.  If so, remove the expand/collapse button.
        //

        if (TreeView_GetChild(hKeyTreeWnd, hParentTreeItem) == NULL) {

            TVItem.mask = TVIF_CHILDREN | TVIF_STATE;
            TVItem.hItem = hParentTreeItem;
            TVItem.cChildren = 0;
            TVItem.state = 0;
            TVItem.stateMask = TVIS_EXPANDED | TVIS_EXPANDEDONCE;
            TreeView_SetItem(hKeyTreeWnd, &TVItem);

        }

        //
        //  Make sure we can see the selected tree item now since it may be
        //  currently off-screen.
        //

        TreeView_EnsureVisible(hKeyTreeWnd, TreeView_GetSelection(hKeyTreeWnd));

        SetWindowRedraw(hKeyTreeWnd, TRUE);

        UpdateWindow(hKeyTreeWnd);

    }

    else {

        TVItem.hItem = hTreeItem;
        TVItem.mask = TVIF_TEXT;
        TVItem.pszText = KeyName;
        TVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

        TreeView_GetItem(hKeyTreeWnd, &TVItem);

        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(IDS_DELETEKEYDELETEFAILED),
            MAKEINTRESOURCE(IDS_DELETEKEYERRORTITLE), MB_ICONERROR | MB_OK,
            KeyName);

    }

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeRename
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeRename(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    if (KeyTree_CanDeleteOrRenameItem(g_RegEditData.hKeyTreeWnd, hTreeItem))
        KeyTree_EditLabel(g_RegEditData.hKeyTreeWnd, hTreeItem);

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeRefresh
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeRefresh(
    HWND hWnd
    )
{

    HDPA hDPA;
    HWND hKeyTreeWnd;
    HTREEITEM hPrevSelectedTreeItem;
    TV_ITEM EnumTVItem;
    TV_ITEM CurrentTVItem;
    HKEY hRootKey;
    TCHAR KeyName[MAXKEYNAME];
    int MaximumSubKeyLength;
    int Index;
    HKEY hEnumKey;
    int CompareResult;
    LPTSTR lpDPAKeyName;
    HTREEITEM hTempTreeItem;

    if ((hDPA = DPA_CreateEx(REFRESH_DPA_GROW, GetProcessHeap())) == NULL)
        return;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    RegEdit_SetWaitCursor(TRUE);
    SetWindowRedraw(hKeyTreeWnd, FALSE);

    hPrevSelectedTreeItem = TreeView_GetSelection(hKeyTreeWnd);

    EnumTVItem.mask = TVIF_TEXT;
    EnumTVItem.pszText = KeyName;
    EnumTVItem.cchTextMax = sizeof(KeyName)/sizeof(TCHAR);

    CurrentTVItem.mask = TVIF_STATE | TVIF_CHILDREN;
    CurrentTVItem.stateMask = 0;
    CurrentTVItem.hItem = TreeView_GetRoot(hKeyTreeWnd);

    while (TRUE) {

        TreeView_GetItem(hKeyTreeWnd, &CurrentTVItem);
        hRootKey = KeyTree_BuildKeyPath(hKeyTreeWnd, CurrentTVItem.hItem,
            KeyName, BKP_TOSUBKEY);

        if (CurrentTVItem.state & TVIS_EXPANDED) {

            //
            //  If this isn't a top-level label (and it won't be if hRootKey is
            //  not NULL), then compare the actual contents of the registry
            //  against what we're showing.
            //
            if(hRootKey && RegOpenKeyEx(hRootKey,KeyName,0,KEY_ENUMERATE_SUB_KEYS,&hEnumKey) ==
                ERROR_SUCCESS) {

                //
                //  As a result of adding new keys and renaming existing ones,
                //  the children of this item may be out of order.  For the
                //  following algorithm to work correctly, we must now sort
                //  these keys.
                //

                TreeView_SortChildren(hKeyTreeWnd, CurrentTVItem.hItem, FALSE);

                //
                //  Build a sorted dynamic array of strings that represent the
                //  keys actually in the registry at this time.
                //

                MaximumSubKeyLength = MAXKEYNAME - (lstrlen(KeyName) + 1);
                Index = 0;

                while (RegEnumKey(hEnumKey, Index, KeyName,
                    MaximumSubKeyLength) == ERROR_SUCCESS) {

                    lpDPAKeyName = NULL;
                    Str_SetPtr(&lpDPAKeyName, KeyName);
                    DPA_InsertPtr(hDPA, Index++, lpDPAKeyName);

                }

                RegCloseKey(hEnumKey);
                DPA_Sort(hDPA, DPACompareKeyNames, 0);

                //
                //  Does this key have subkeys anymore?  If not, then we need
                //  to reset it's child flag and remove all of it's children.
                //

                if (Index == 0) {

                    DPA_DeleteAllPtrs(hDPA);

                    TreeView_Expand(hKeyTreeWnd, CurrentTVItem.hItem,
                        TVE_COLLAPSE | TVE_COLLAPSERESET);

                    CurrentTVItem.cChildren = 0;
                    goto SetCurrentTreeItem;

                }

                //
                //  Merge the keys that we found during our above enumeration
                //  with the keys that our key tree lists.  Add and remove
                //  elements from the tree as appropriate.
                //

                lpDPAKeyName = DPA_FastGetPtr(hDPA, --Index);

                EnumTVItem.hItem = TreeView_GetChild(hKeyTreeWnd,
                    CurrentTVItem.hItem);
                if (EnumTVItem.hItem)
                    TreeView_GetItem(hKeyTreeWnd, &EnumTVItem);

                while (Index >= 0 && EnumTVItem.hItem != NULL) {

                    CompareResult = lstrcmpi(KeyName, lpDPAKeyName);

                    if (CompareResult == 0) {

                        EnumTVItem.hItem = TreeView_GetNextSibling(hKeyTreeWnd,
                            EnumTVItem.hItem);
                        if (EnumTVItem.hItem)
                            TreeView_GetItem(hKeyTreeWnd, &EnumTVItem);

                        goto GetNextDPAPointer;

                    }

                    else if (CompareResult > 0) {

                        KeyTree_InsertItem(hKeyTreeWnd, CurrentTVItem.hItem,
                            TVI_SORT, lpDPAKeyName, DoesKeyHaveKids(hEnumKey,
                            lpDPAKeyName), 0);

GetNextDPAPointer:
                        Str_SetPtr(&lpDPAKeyName, NULL);

                        if (--Index >= 0)
                            lpDPAKeyName = DPA_FastGetPtr(hDPA, Index);

                    }

                    else {

                        hTempTreeItem = TreeView_GetNextSibling(hKeyTreeWnd,
                            EnumTVItem.hItem);
                        TreeView_DeleteItem(hKeyTreeWnd, EnumTVItem.hItem);
                        EnumTVItem.hItem = hTempTreeItem;
                        if (EnumTVItem.hItem)
                            TreeView_GetItem(hKeyTreeWnd, &EnumTVItem);

                    }

                }

                //
                //  Once we drop to here, we may have extra items in the key
                //  tree or in the dynamic array.  Process them accordingly.
                //

                if (Index >= 0) {

                    while (TRUE) {

                        KeyTree_InsertItem(hKeyTreeWnd, CurrentTVItem.hItem,
                            TVI_SORT, lpDPAKeyName, DoesKeyHaveKids(hEnumKey,
                            lpDPAKeyName), 0);

                        Str_SetPtr(&lpDPAKeyName, NULL);

                        if (--Index < 0)
                            break;

                        lpDPAKeyName = DPA_FastGetPtr(hDPA, Index);

                    }

                }

                else {

                    while (EnumTVItem.hItem != NULL) {

                        hTempTreeItem = TreeView_GetNextSibling(hKeyTreeWnd,
                            EnumTVItem.hItem);
                        TreeView_DeleteItem(hKeyTreeWnd, EnumTVItem.hItem);
                        EnumTVItem.hItem = hTempTreeItem;

                    }

                }

                DPA_DeleteAllPtrs(hDPA);

            }

            CurrentTVItem.hItem = TreeView_GetChild(hKeyTreeWnd,
                CurrentTVItem.hItem);

        }

        else {

            //
            //  If this isn't a top-level label (and it won't be if hRootKey is
            //  not NULL), then re-check if this key has any children.
            //

            if (hRootKey != NULL) {

                TreeView_Expand(hKeyTreeWnd, CurrentTVItem.hItem, TVE_COLLAPSE |
                    TVE_COLLAPSERESET);

                CurrentTVItem.cChildren = DoesKeyHaveKids(hRootKey, KeyName);

SetCurrentTreeItem:
                TreeView_SetItem(hKeyTreeWnd, &CurrentTVItem);

            }

            //
            //  Because we're at the "bottom" of the TreeView, we now need to
            //  walk to the siblings of this tree item.  And if no siblings
            //  exist, we walk back to the parent and check again for siblings.
            //

            while (TRUE) {

                if ((hTempTreeItem = TreeView_GetNextSibling(hKeyTreeWnd,
                    CurrentTVItem.hItem)) != NULL) {

                    CurrentTVItem.hItem = hTempTreeItem;
                    break;

                }

                if ((CurrentTVItem.hItem = TreeView_GetParent(hKeyTreeWnd,
                    CurrentTVItem.hItem)) == NULL) {

                    //
                    //  We've now walked over all of the tree items, so do any
                    //  cleanup here and exit.
                    //

                    DPA_Destroy(hDPA);

                    SetWindowRedraw(hKeyTreeWnd, TRUE);

                    //
                    //  The selection may have changed as a result of having
                    //  the focus on an nonexistent key.
                    //

                    if (TreeView_GetSelection(hKeyTreeWnd) !=
                        hPrevSelectedTreeItem)
                        RegEdit_OnKeyTreeSelChanged(hWnd, NULL);
                    else
                        RegEdit_OnValueListRefresh(hWnd);

                    RegEdit_SetWaitCursor(FALSE);

                    return;

                }

            }

        }

    }

}

/*******************************************************************************
*
*  DPACompareKeyNames
*
*  DESCRIPTION:
*     Callback comparision routine for refresh's DPA_Sort call.  Simply returns
*     the result of lstrcmpi.
*
*  PARAMETERS:
*     lpString1,
*     lpString2,
*     lParam, ignored optional data.
*
*******************************************************************************/

int
WINAPI
DPACompareKeyNames(
    LPVOID lpString1,
    LPVOID lpString2,
    LPARAM lParam
    )
{

    return lstrcmpi((LPTSTR) lpString2, (LPTSTR) lpString1);

}

/*******************************************************************************
*
*  RegEdit_OnKeyTreeDisconnect
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnKeyTreeDisconnect(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    HWND hKeyTreeWnd;
    TV_ITEM TVItem;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    //
    //  Disconnect all of the root registry handles that we've opened.
    //

    TVItem.mask = TVIF_PARAM;
    TVItem.hItem = TreeView_GetChild(hKeyTreeWnd, hTreeItem);

    while (TVItem.hItem != NULL) {

        TreeView_GetItem(hKeyTreeWnd, &TVItem);

        RegCloseKey((HKEY) TVItem.lParam);

        TVItem.hItem = TreeView_GetNextSibling(hKeyTreeWnd, TVItem.hItem);

    }

    TreeView_DeleteItem(hKeyTreeWnd, hTreeItem);

}

/*******************************************************************************
*
*  RegEdit_UpdateStatusBar
*
*  DESCRIPTION:
*     Show the full registry path in the status bar, for lack of anything
*     better to do with it.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
PASCAL
RegEdit_UpdateStatusBar(
    VOID
    )
{

    HWND hKeyTreeWnd;
    TCHAR KeyName[MAXKEYNAME*2];

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;
    KeyTree_BuildKeyPath(hKeyTreeWnd, TreeView_GetSelection(hKeyTreeWnd),
        KeyName, BKP_TOCOMPUTER);
    SetWindowText(g_RegEditData.hStatusBarWnd, KeyName);

}

/*******************************************************************************
*
*  RegEdit_OnCopyKeyName
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCopyKeyName(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    TCHAR KeyName[MAXKEYNAME*2];
    UINT KeyNameLength;
    HANDLE hClipboardData;
    LPTSTR lpClipboardData;

    KeyTree_BuildKeyPath(g_RegEditData.hKeyTreeWnd, hTreeItem, KeyName,
        BKP_TOSYMBOLICROOT);
    KeyNameLength = (lstrlen(KeyName) + 1) * sizeof(TCHAR);

    if (OpenClipboard(hWnd)) {

        if ((hClipboardData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
            KeyNameLength)) != NULL) {

            lpClipboardData = (LPTSTR) GlobalLock(hClipboardData);
            CopyMemory(lpClipboardData, KeyName, KeyNameLength);
            GlobalUnlock(hClipboardData);

            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hClipboardData);

        }

        CloseClipboard();

    }

}

/*******************************************************************************
*
*  KeyTree_BuildKeyPath
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hTreeViewWnd, handle of KeyTree window.
*     hTreeItem, handle of tree item to begin building from.
*     lpKeyPath, buffer to store path in.
*     fIncludeSymbolicRootName, TRUE if root key's name should be included
*        (e.g., HKEY_LOCAL_MACHINE), else FALSE.
*
*******************************************************************************/

HKEY
PASCAL
KeyTree_BuildKeyPath(
    HWND hTreeViewWnd,
    HTREEITEM hTreeItem,
    LPTSTR lpKeyPath,
    UINT ToFlags
    )
{

    TV_ITEM TVItem;
    TCHAR SubKeyName[MAXKEYNAME*2];

    *lpKeyPath = '\0';

    TVItem.mask = TVIF_TEXT | TVIF_PARAM;
    TVItem.hItem = hTreeItem;
    TVItem.pszText = (LPTSTR) SubKeyName;
    TVItem.cchTextMax = sizeof(SubKeyName)/sizeof(TCHAR);

    while (TRUE) {

        TreeView_GetItem(hTreeViewWnd, &TVItem);

        if (TVItem.lParam != 0 && !(ToFlags & BKP_TOSYMBOLICROOT))
            break;

        if (*lpKeyPath != '\0') {

            lstrcat(SubKeyName, TEXT("\\"));
            lstrcat(SubKeyName, lpKeyPath);

        }

        lstrcpy(lpKeyPath, SubKeyName);

        if (TVItem.lParam != 0 && (ToFlags & BKP_TOCOMPUTER) != BKP_TOCOMPUTER)
            break;

        TVItem.hItem = TreeView_GetParent(hTreeViewWnd, TVItem.hItem);

        if (TVItem.hItem == NULL) {

            if ((ToFlags & BKP_TOCOMPUTER) != BKP_TOCOMPUTER)
                *lpKeyPath = '\0';

            break;

        }

    }

    return ((HKEY) TVItem.lParam);

}

/*******************************************************************************
*
*  KeyTree_InsertItem
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HTREEITEM
PASCAL
KeyTree_InsertItem(
    HWND hKeyTreeWnd,
    HTREEITEM hParent,
    HTREEITEM hInsertAfter,
    LPCTSTR lpText,
    UINT fHasKids,
    LPARAM lParam
    )
{

    TV_INSERTSTRUCT TVInsertStruct;

    TVInsertStruct.hParent = hParent;
    TVInsertStruct.hInsertAfter = hInsertAfter;
    TVInsertStruct.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
        TVIF_PARAM | TVIF_CHILDREN;
    //  TVInsertStruct.item.hItem = NULL;
    //  TVInsertStruct.item.state = 0;
    //  TVInsertStruct.item.stateMask = 0;
    TVInsertStruct.item.pszText = (LPTSTR) lpText;
    //  TVInsertStruct.item.cchTextMax = lstrlen(lpText);
    TVInsertStruct.item.iImage = IMAGEINDEX(IDI_FOLDER);
    TVInsertStruct.item.iSelectedImage = IMAGEINDEX(IDI_FOLDEROPEN);
    TVInsertStruct.item.cChildren = fHasKids;
    TVInsertStruct.item.lParam = lParam;

    return TreeView_InsertItem(hKeyTreeWnd, &TVInsertStruct);

}

/*******************************************************************************
*
*  KeyTree_ExpandBranch
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hTreeViewWnd, handle of KeyTree window.
*     hTreeItem, handle of tree item to edit.
*
*******************************************************************************/

BOOL
PASCAL
KeyTree_ExpandBranch(
    HWND hKeyTreeWnd,
    HTREEITEM hExpandingTreeItem
    )
{

    TCHAR KeyName[MAXKEYNAME];
    HKEY hRootKey;
    HKEY hEnumKey;
    int Index;
    int MaximumSubKeyLength;

    //
    //  Nothing special needs to be done with a top-level label such as "My
    //  Computer" or a network computer name.  It's children are already filled
    //  in and are always valid.
    //

    if (TreeView_GetParent(hKeyTreeWnd, hExpandingTreeItem) == NULL)
        return TRUE;

    hRootKey = KeyTree_BuildKeyPath(hKeyTreeWnd, hExpandingTreeItem,
        KeyName, FALSE);

    if(RegOpenKeyEx(hRootKey,KeyName,0,KEY_ENUMERATE_SUB_KEYS,&hEnumKey) != ERROR_SUCCESS)
        return FALSE;

    MaximumSubKeyLength = MAXKEYNAME - (lstrlen(KeyName) + 1);
    Index = 0;

    while (RegEnumKey(hEnumKey, Index++, KeyName, MaximumSubKeyLength) ==
        ERROR_SUCCESS) {

        KeyTree_InsertItem(hKeyTreeWnd, hExpandingTreeItem, TVI_FIRST,
            KeyName, DoesKeyHaveKids(hEnumKey, KeyName), 0);

    }

    RegCloseKey(hEnumKey);

    //
    //  Sort the subkeys _after_ inserting all the items.  The above insert
    //  used to specify TVI_SORT, but on NT, expanding a key with several
    //  subkeys (e.g., HKEY_CLASSES_ROOT) would take several seconds!
    //

    TreeView_SortChildren(hKeyTreeWnd, hExpandingTreeItem, FALSE);


    return TRUE;

}

/*******************************************************************************
*
*  DoesKeyHaveKids
*
*  DESCRIPTION:
*     Checks if the given key path has any subkeys or not.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
DoesKeyHaveKids(
    HKEY hKey,
    LPTSTR lpKeyName
    )
{

    BOOL fHasKids;
    HKEY hCheckChildrenKey;
    DWORD cSubKeys;

    fHasKids = FALSE;

    if(RegOpenKeyEx(hKey,lpKeyName,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,
        &hCheckChildrenKey) == ERROR_SUCCESS) {

        if (RegQueryInfoKey(hCheckChildrenKey, NULL, NULL, NULL, &cSubKeys,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS &&
            cSubKeys > 0)
            fHasKids = TRUE;

        RegCloseKey(hCheckChildrenKey);

    }

    return fHasKids;

}

/*******************************************************************************
*
*  KeyTree_EditLabel
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hTreeViewWnd, handle of KeyTree window.
*     hTreeItem, handle of tree item to edit.
*
*******************************************************************************/

VOID
PASCAL
KeyTree_EditLabel(
    HWND hKeyTreeWnd,
    HTREEITEM hTreeItem
    )
{

    g_RegEditData.fAllowLabelEdits = TRUE;

    TreeView_EditLabel(hKeyTreeWnd, hTreeItem);

    g_RegEditData.fAllowLabelEdits = FALSE;

}

/*******************************************************************************
*
*  KeyTree_CanDeleteOrRenameItem
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hTreeViewWnd, handle of KeyTree window.
*     hTreeItem, handle of tree item to check.
*
*******************************************************************************/

BOOL
PASCAL
KeyTree_CanDeleteOrRenameItem(
    HWND hWnd,
    HTREEITEM hTreeItem
    )
{

    TV_ITEM TVItem;

    //
    //  Check if the selected tree item is null.  This will occur when viewing
    //  the Edit popup from the main menu with no selection made.
    //

    if (hTreeItem != NULL) {

        //
        //  Check if this tree item has any reference data indicating that it
        //  is a predefined root.  Predefined roots cannot be renamed or
        //  deleted.
        //

        TVItem.hItem = hTreeItem;
        TVItem.mask = TVIF_PARAM;
        TreeView_GetItem(hWnd, &TVItem);

        if ((HKEY) TVItem.lParam == NULL) {

            //
            //  Check that this isn't a top-level item such as "My Computer" or
            //  a remote registry connection.
            //

            if (TreeView_GetParent(hWnd, hTreeItem) != NULL)
                return TRUE;

        }

    }

    return FALSE;

}
