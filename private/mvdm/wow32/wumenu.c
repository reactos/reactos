/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMENU.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wumenu.c);


/*++
    BOOL AppendMenu(<hMenu>, <wFlags>, <wIDNewItem>, <lpNewItem>)
    HMENU <hMenu>;
    WORD <wFlags>;
    WORD <wIDNewItem>;
    LPSTR <lpNewItem>;

    The %AppendMenu% function appends a new item to the end of a menu. The
    application can specify the state of the menu item by setting values in the
    <wFlags> parameter.

    <hMenu>
        Identifies the menu to be changed.

    <wFlags>
        Specifies information about the state of the new menu item when
        it is added to the menu. It consists of one or more values listed in the
        following Comments section.

    <wIDNewItem>
        Specifies either the command ID of the new menu item or, if
        <wFlags> is set to MF_POPUP, the menu handle of the pop-up menu.

    <lpNewItem>
        Specifies the content of the new menu item. The interpretation
        of the <lpNewItem> parameter depends upon the setting of the <wFlags>
        parameter.

    MF_STRING
        Contains a long pointer to a null-terminated string.

    MF_BITMAP
        Contains a bitmap handle in its low-order word.

    MF_OWNERDRAW
        Contains an application-supplied 32-bit value which the application can
        use to maintain additional data associated with the menu item. This
        32-bit value is available to the application in the %itemData% member of
        the structure pointed to by the <lParam> parameter of the WM_MEASUREITEM
        and WM_DRAWITEM messages sent when the menu item is initially displayed
        or is changed.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%.

    Each of the following groups list flags that are mutually exclusive and
    cannot be used together:

    o   MF_BYCOMMAND and MF_BYPOSITION

    o   MF_DISABLED, MF_ENABLED, and MF_GRAYED

    o   MF_BITMAP, MF_STRING, and MF_OWNERDRAW

    o   MF_MENUBARBREAK and MF_MENUBREAK

    o   MF_CHECKED and MF_UNCHECKED

    .cmt
    16-Sep-1990 [ralphw]

    Some of the above flags aren't documented as valid values for the wFlags
    parameter. If they are valid, they should be documented, otherwise, they
    should be removed from the list.
    .endcmt

    The following list describes the flags which may be set in the <wFlags>
    parameter:

    MF_BITMAP
        Uses a bitmap as the item. The low-order word of the lpNewItem parameter
        contains the handle of the bitmap.

    MF_CHECKED
        Places a checkmark next to the item. If the application has supplied
        checkmark bitmaps (see %SetMenuItemBitmaps%), setting this flag displays
        the checkmark on bitmap next to the menu item.

    MF_DISABLED
        Disables the menu item so that it cannot be selected, but does not gray
        it.

    MF_ENABLED
        Enables the menu item so that it can be selected and restores it from
        its grayed state.

    MF_GRAYED
        Disables the menu item so that it cannot be selected and grays it.

    MF_MENUBARBREAK
        Same as MF_MENUBREAK except that for pop-up menus, separates the new
        column from the old column with a vertical line.

    MF_MENUBREAK
        Places the item on a new line for static menu-bar items. For pop-up
        menus, places the item in a new column, with no dividing line between
        the columns.

    MF_OWNERDRAW
        Specifies that the item is an owner-draw item. The window that owns the
        menu receives a WM_MEASUREITEM message when the menu is displayed for
        the first time to retrieve the height and width of the menu item. The
        WM_DRAWITEM message is then sent whenever the owner must update the
        visual appearance of the menu item. This option is not valid for a
        top-level menu item.

    MF_POPUP
        Specifies that the menu item has a pop-up menu associated with it. The
        <wIDNewItem> parameter specifies a handle to a pop-up menu to be
        associated with the item. This is used for adding either a top-level
        pop-up menu or adding a hierarchical pop-up menu to a pop-up menu item.

    MF_SEPARATOR
        Draws a horizontal dividing line. Can only be used in a pop-up menu.
        This line cannot be grayed, disabled, or highlighted. The <lpNewItem>
        and <wIDNewItem> parameters are ignored.

    MF_STRING
        Specifies that the menu item is a character string; the <lpNewItem>
        parameter points to the string for the menu item.

    MF_UNCHECKED
        Does not place a checkmark next to the item (default). If the
        application has supplied checkmark bitmaps (see %SetMenuItemBitmaps%),
        setting this flag displays the checkmark off bitmap next to the menu
        item.
--*/

ULONG FASTCALL WU32AppendMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz4;
    register PAPPENDMENU16 parg16;
    UINT wIDNewItem;

    GETARGPTR(pFrame, sizeof(APPENDMENU16), parg16);

    // USER has some internal bitmap identifiers for menu item bitmaps so
    // it doesn't have to store multiple copies of these bitmaps when they
    // appear in multiple menus.  WinProj does its own MDI and uses these
    // bitmaps.

    //  #define MENUHBM_CHILDCLOSE   (UINT)1
    //  #define MENUHBM_RESTORE      (UINT)2
    //  #define MENUHBM_MINIMIZE     (UINT)3
    //  #define MENUHBM_MAX          (UINT)4

    if (parg16->f2 & MF_BITMAP) {
        if (LOW(parg16->f4) >= 4)
            psz4 = (PSZ)HBITMAP32(LOW(parg16->f4));
        else
            psz4 = (PSZ)WORD32(parg16->f4);
    }
    else if (parg16->f2 & MF_OWNERDRAW)
        psz4 = (PSZ)DWORD32(parg16->f4);
    else
        GETPSZPTR(parg16->f4, psz4);

    wIDNewItem = (UINT) WORD32(parg16->f3);

    if (parg16->f2 & MF_POPUP)
        wIDNewItem = (UINT) HMENU32(parg16->f3);


    ul = GETBOOL16(AppendMenu(HMENU32(parg16->f1),
                              WORD32(parg16->f2),
                              wIDNewItem,
                              psz4));

#ifdef	FE_SB
    // For AutherWare Star ( APP BUG ) MAKKBUG:3203
    if (CURRENTPTD()->dwWOWCompatFlags2 & WOWCF_ASWHELPER) {                        	if (parg16->f2 & MF_POPUP) {
	    HWND hWnd;

	    if (!(hWnd=GetActiveWindow())) {
        	hWnd = GetForegroundWindow();
	    }
	    SetMenu(hWnd , HMENU32(parg16->f1));
	}
    }
#endif // FE_SB

    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++
    BOOL ChangeMenu(<hMenu>,<wIDChangeItem>,<lpNewItem>,<wIDNewItem>,<wFlags>)
    HMENU <hMenu>;
    WORD <wIDChangeItem>;
    LPSTR <lpNewItem>;
    WORD <wIDNewItem>;
    WORD <wFlags>;

    The %ChangeMenu% function appends, inserts, deletes, or modifies a menu
    item in the menu given by the <hMenu> parameter.  The <wIDChangeItem>,
    <lpNewItem>, <wIDNewItem> and <wFlags> parameters define which item to
    change and how to change it.

    <hMenu>
        Identifies the menu to be changed.

    <wIDChangeItem>
        Specifies the item to be changed.  If MF_BYPOSITION is specified,
        <wIDChangeItem> gives the position of the menu item to be
        changed (the first item is at position zero).  If MF_BYCOMMAND is
        specified instead, <wIDChangeItem> specifies the menu-item ID.
        The menu-item ID can specify any popup-menu item (that is, an item
        in a popup menu associated with an item of <hMenu>).  If neither
        flag is given, the default is MF_BYCOMMAND.  When MF_INSERT is used,
        <wIDChangeItem> identifies the item before which the new item is to
        be inserted.  When MF_APPEND is used, <wIDChangeItem> is NULL.

    <lpNewItem>
        Specifies the content of the new menu item. The interpretation
        of the <lpNewItem> parameter depends upon the setting of the <wFlags>
        parameter.  The <lpNewItem> parameter is never a handle to a menu.
        The MF_POPUP flag applies to the <wIDNewItem> parameter only.

        MF_BITMAP
            <lpNewItem> contains a bitmap handle in its low-order word.

        MF_STRING
            <lpNewItem> contains a long pointer to a null-terminated string.

        The default is MF_STRING.  A NULL value for <lpNewItem> creates a
        horizontal break (the same effect as using the MF_SEPARATOR flag).

        Note that MF_OWNERDRAW is not allowed on ChangeMenu;  it conflicts with
        the ChangeMenu command bit MF_APPEND.

    <wIDNewItem>
        Specifies either the command ID of the new menu item or, if
        <wFlags> is set to MF_POPUP, the menu handle of the pop-up menu.
        It is never a menu-item position.

    <wFlags>
        Specifies information about the state of the new menu item when it
        is added to the menu.  It consists of one or more values listed in
        the following Comments section.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%.

    Each of the following groups list flags that are mutually exclusive and
    cannot be used together:

    o   MF_APPEND, MF_CHANGE, MF_DELETE, MF_INSERT and MF_REMOVE

    o   MF_BYCOMMAND and MF_BYPOSITION

    o   MF_DISABLED, MF_ENABLED, and MF_GRAYED

    o   MF_BITMAP, MF_POPUP and MF_STRING

    o   MF_MENUBARBREAK and MF_MENUBREAK

    o   MF_CHECKED and MF_UNCHECKED

    .cmt
    16-Sep-1990 [ralphw]

    Some of the above flags aren't documented as valid values for the wFlags
    parameter. If they are valid, they should be documented, otherwise, they
    should be removed from the list.
    .endcmt

    The following list describes the flags which may be set in the <wFlags>
    parameter:

    MF_APPEND
        Appends the new item to the end of the menu.

    MF_BITMAP
        Uses a bitmap as the item. The low-order word of the lpNewItem parameter
        contains the handle of the bitmap.

    MF_BYCOMMAND
        Specifies that the <wIDChangeItem> parameter gives the menu-item ID
        number (default).

    MF_BYPOSITION
        Specifies that the <wIDChangeItem> parameter gives the position of
        the menu item to be changed.

    MF_CHANGE
        Changes or replaces the specified item.

    MF_CHECKED
        Places a checkmark next to the item. If the application has supplied
        checkmark bitmaps (see %SetMenuItemBitmaps%), setting this flag displays
        the checkmark on bitmap next to the menu item.

    MF_DELETE
        Deletes the item.

    MF_DISABLED
        Disables the menu item so that it cannot be selected, but does not gray
        it.

    MF_ENABLED
        Enables the menu item so that it can be selected and restores it from
        its grayed state.

    MF_GRAYED
        Disables the menu item so that it cannot be selected and grays it.

    MF_INSERT
        Inserts a new item, just before the specified item.

    MF_MENUBARBREAK
        Same as MF_MENUBREAK except that for pop-up menus, separates the new
        column from the old column with a vertical line.

    MF_MENUBREAK
        Places the item on a new line for static menu-bar items. For pop-up
        menus, places the item in a new column, with no dividing line between
        the columns.

    MF_POPUP
        Specifies that the menu item has a pop-up menu associated with it. The
        <wIDNewItem> parameter specifies a handle to a pop-up menu to be
        associated with the item. This is used for adding either a top-level
        pop-up menu or adding a hierarchical pop-up menu to a pop-up menu item.

    MF_REMOVE
        Removes the item but does not delete it.

    MF_SEPARATOR
        Draws a horizontal dividing line. Can only be used in a pop-up menu.
        This line cannot be grayed, disabled, or highlighted. The <lpNewItem>
        and <wIDNewItem> parameters are ignored.

    MF_STRING
        Specifies that the menu item is a character string; the <lpNewItem>
        parameter points to the string for the menu item.

    MF_UNCHECKED
        Does not place a checkmark next to the item (default). If the
        application has supplied checkmark bitmaps (see %SetMenuItemBitmaps%),
        setting this flag displays the checkmark off bitmap next to the menu
        item.
--*/

ULONG FASTCALL WU32ChangeMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz3;
    DWORD dw4;

    register PCHANGEMENU16 parg16;

    GETARGPTR(pFrame, sizeof(CHANGEMENU16), parg16);

    if (parg16->f5 & MF_BITMAP) {
        if (LOW(parg16->f3) >= 4)
            psz3 = (PSZ)HBITMAP32(LOW(parg16->f3));
        else
            psz3 = (PSZ)WORD32(parg16->f3);
    }
    else
        GETPSZPTR(parg16->f3, psz3);

    dw4 = WORD32(parg16->f4);
    if (WORD32(parg16->f5) & MF_POPUP)
    dw4 = (DWORD)HMENU32(parg16->f4);

    ul = GETBOOL16(ChangeMenu(
      HMENU32(parg16->f1),
      WORD32(parg16->f2),
      psz3,
      dw4,
      WORD32(parg16->f5)
    ));

    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL CheckMenuItem(<hMenu>, <wIDCheckItem>, <wCheck>)
    HMENU <hMenu>;
    WORD <wIDCheckItem>;
    WORD <wCheck>;

    The %CheckMenuItem% function places checkmarks next to or removes checkmarks
    from menu items in the pop-up menu specified by the <hMenu> parameter. The
    <wIDCheckItem> parameter specifies the item to be modified.

    <hMenu>
        Identifies the menu.

    <wIDCheckItem>
        Specifies the menu item to be checked.

    <wCheck>
        Specifies how to check the menu item and how to determine the item's
        position in the menu. The <wCheck> parameter can be a combination of the
        MF_CHECKED or MF_UNCHECKED with MF_BYPOSITION or MF_BYCOMMAND flags.
        These flags can be combined by using the bitwise OR operator. They have
        the following meanings:

    MF_BYCOMMAND
        Specifies that the <wIDCheckItem> parameter gives the menu-item ID
        (MF_BYCOMMAND is the default).

    MF_BYPOSITION
        Specifies that the <wIDCheckItem > parameter gives the position of the
        menu item (the first item is at position zero).

    MF_CHECKED
        Adds checkmark.

    MF_UNCHECKED
        Removes checkmark.

    The return value specifies the previous state of the item. It is either
    MF_CHECKED or MF_UNCHECKED. The return value is -1 if the menu item does not
    exist.

    The <wIDCheckItem> parameter may identify a pop-up menu item as well as a
    menu item. No special steps are required to check a pop-up menu item.

    Top-level menu items cannot be checked.

    A pop-up menu item should be checked by position since it does not have a
    menu-item identifier associated with it.
--*/

ULONG FASTCALL WU32CheckMenuItem(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCHECKMENUITEM16 parg16;

    GETARGPTR(pFrame, sizeof(CHECKMENUITEM16), parg16);

    ul = GETBOOL16(CheckMenuItem(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HMENU CreateMenu(VOID)

    The %CreateMenu% function creates a menu. The menu is initially empty, but
    can be filled with menu items by using the %AppendMenu% or %InsertMenu%
    function.

    This function has no parameters.

    The return value identifies the newly created menu. It is NULL if the menu
    cannot be created.
--*/

ULONG FASTCALL WU32CreateMenu(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHMENU16(CreateMenu());

    RETURN(ul);
}


/*++
    HMENU CreatePopupMenu(VOID)

    The %CreatePopupMenu% function creates and returns a handle to an empty
    pop-up menu.

    An application adds items to the pop-up menu by calling %InsertMenu% and
    %AppendMenu%. The application can add the pop-up menu to an existing menu or
    pop-up menu, or it may display and track selections on the pop-up menu by
    calling %TrackPopupMenu%.

    This function has no parameters.

    The return value identifies the newly created menu. It is NULL if the menu
    cannot be created.
--*/

ULONG FASTCALL WU32CreatePopupMenu(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHMENU16(CreatePopupMenu());

    RETURN(ul);
}


/*++
    BOOL DeleteMenu(<hMenu>, <nPosition>, <wFlags>)
    HMENU <hMenu>;
    WORD <nPosition>;
    WORD <wFlags>;

    The %DeleteMenu% function deletes an item from the menu identified by the
    <hMenu> parameter; if the menu item has an associated pop-up menu,
    %DeleteMenu% destroys the handle by the pop-up menu and frees the memory
    used by the pop-up menu.

    <hMenu>
        Identifies the menu to be changed.

    <nPosition>
        Specifies the menu item which is to be deleted, as determined by the
        <wFlags> parameter:

    MF_BYPOSITION
        Specifies the position of the menu item; the first item in the menu is
        at position 0.

    MF_BYCOMMAND
        Specifies the command ID of the existing menu item.

    <wFlags>
        Specifies how the <nPosition> parameter is interpreted. It may be
        set to either MF_BYCOMMAND or MF_BYPOSITION.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%.
--*/
ULONG FASTCALL WU32DeleteMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDELETEMENU16 parg16;
    PSZ pszModName;

    GETARGPTR(pFrame, sizeof(DELETEMENU16), parg16);

    // Nasty Hack to fix MSVC 1.5, they remove the close item
    // from their system menu, so we prevent them from doing
    // this and act like nothing happened ( ie return TRUE ).
    // From Win 95, Bug#8154, [t-arthb]

    if ((parg16->f2 == 6) && (parg16->f3 & MF_BYPOSITION)) {
        pszModName = ((PTDB)SEGPTR(CURRENTPTD()->htask16,0))->TDB_ModName;
        if (!WOW32_strncmp(pszModName, "MSVC", 4)) {
            FREEARGPTR(parg16);
            RETURN(GETBOOL16(TRUE));
        }
    }

    ul = GETBOOL16(DeleteMenu(HMENU32(parg16->f1),
                              WORD32(parg16->f2),
                              WORD32(parg16->f3)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL DestroyMenu(<hMenu>)
    HMENU <hMenu>;

    The %DestroyMenu% function destroys the menu specified by the <hMenu>
    parameter and frees any memory that the menu occupied.

    <hMenu>
        Identifies the menu to be destroyed.

    The return value specifies whether or not the specified menu is destroyed.
    It is TRUE if the menu is destroyed. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32DestroyMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDESTROYMENU16 parg16;

    GETARGPTR(pFrame, sizeof(DESTROYMENU16), parg16);

    ul = GETBOOL16(DestroyMenu(HMENU32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void DrawMenuBar(<hwnd>)
    HWND <hwnd>;

    The %DrawMenuBar% function redraws the menu bar. If a menu bar is changed
    <after> Windows has created the window, this function should be called to
    draw the changed menu bar.

    <hwnd>
        Identifies the window whose menu needs redrawing.

    This function does not return a value.
--*/

ULONG FASTCALL WU32DrawMenuBar(PVDMFRAME pFrame)
{
    register PDRAWMENUBAR16 parg16;

    GETARGPTR(pFrame, sizeof(DRAWMENUBAR16), parg16);

    DrawMenuBar(HWND32(parg16->f1));

    FREEARGPTR(parg16);
    RETURN(TRUE);
}


/*++
    DWORD GetMenuCheckMarkDimensions(VOID)

    The %GetMenuCheckMarkDimensions% function returns the dimensions of the
    default checkmark bitmap. Windows displays this bitmap next to checked menu
    items. Before calling the %SetMenuItemBitmaps% function to replace the
    default checkmark, an application should call the
    %GetMenuCheckMarkDimensions% function to determine the correct size for the
    bitmaps.

    This function has no parameters.

    The return value specifies the height and width of the default checkmark
    bitmap. The high-order word contains the height in pixels and the low-order
    word contains the width.
--*/

ULONG FASTCALL WU32GetMenuCheckMarkDimensions(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETLONG16(GetMenuCheckMarkDimensions());

    RETURN(ul);
}


/*++
    int GetMenuString(<hMenu>, <wIDItem>, <lpString>, <nMaxCount>, <wFlag>)
    HMENU <hMenu>;
    WORD <wIDItem>;
    LPSTR <lpString>;
    int <nMaxCount>;
    WORD <wFlag>;

    The %GetMenuString% function copies the label of the specified menu item
    into the <lpString> parameter.

    <hMenu>
        Identifies the menu.

    <wIDItem>
        Specifies the integer identifier of the menu item (from the
        resource file) or the offset of the menu item in the menu, depending on
        the value of the <wFlag> parameter.

    <lpString>
        Points to the buffer that is to receive the label.

    <nMaxCount>
        Specifies the maximum length of the label to be copied. If the
        label is longer than the maximum specified in <nMaxCount>, the extra
        characters are truncated.

    <wFlag>
        Specifies the nature of the <wID> parameter. If <wFlags> contains
        MF_BYPOSITION, <wId> specifies a (zero-based) relative position; if the
        <wFlags> parameter contains MF_BYCOMMAND, <wId> specifies the item ID.

    The return value specifies the actual number of bytes copied to the buffer.

    The <nMaxCount> parameter should be one larger than the number of characters
    in the label to accommodate the null character that terminates a string.
--*/

#define GMS32_LIMIT 2000
ULONG FASTCALL WU32GetMenuString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz3;
    register PGETMENUSTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GETMENUSTRING16), parg16);
    ALLOCVDMPTR(parg16->f3, parg16->f4, psz3);

    // limit nMaxCount to a reasonable amount so it does not fail in client
    // server.  Some wow apps passed in -1.
    ul = GETINT16(GetMenuString(
        HMENU32(parg16->f1),
        WORD32(parg16->f2),
        psz3,
        (WORD32(parg16->f4) > GMS32_LIMIT) ? GMS32_LIMIT : WORD32(parg16->f4),
        WORD32(parg16->f5)
        ));

    FLUSHVDMPTR(parg16->f3, strlen(psz3)+1, psz3);
    FREEVDMPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HMENU GetSystemMenu(<hwnd>, <bRevert>)
    HWND <hwnd>;
    BOOL <bRevert>;

    The %GetSystemMenu% function allows the application to access the System
    menu for copying and modification.

    <hwnd>
        Identifies the window that will own a copy of the System menu.

    <bRevert>
        Specifies the action to be taken.

        If <bRevert> is FALSE, the %GetSystemMenu% returns a handle to a copy of
        the System menu currently in use. This copy is initially identical to
        the System menu, but can be modified.

        If <bRevert> is TRUE, the %GetSystemMenu% function destroys the possibly
        modified copy of the System menu (if there is one) that belongs to the
        specified window and returns a handle to the original, unmodified
        version of the System menu.

    The return value identifies the System menu if <bRevert> is TRUE and the
    System menu has been modified. If <bRevert> is TRUE and the System menu
    has <not> been modified, the return value is NULL. If <bRevert> is FALSE, the
    return value identifies a copy of the System menu.

    Any window that does not use the %GetSystemMenu% function to make its own
    copy of the System menu receives the standard System menu.

    The handle returned by the %GetSystemMenu% function can be used with the
    %AppendMenu%, %InsertMenu% or %ModifyMenu% functions to change the System
    menu. The System menu initially contains items identified with various ID
    values such as SC_CLOSE, SC_MOVE, and SC_SIZE. Menu items on the System menu
    send WM_SYSCOMMAND messages. All predefined System-menu items have ID
    numbers greater than 0xF000. If an application adds commands to the System
    menu, it should use ID numbers less than F000.

    Windows automatically grays items on the standard System menu, depending on
    the situation. The application can carry out its own checking or graying by
    responding to the WM_INITMENU message, which is sent before any menu is
    displayed.
--*/

ULONG FASTCALL WU32GetSystemMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSYSTEMMENU16 parg16;

    GETARGPTR(pFrame, sizeof(GETSYSTEMMENU16), parg16);

    ul = GETHMENU16(GetSystemMenu(HWND32(parg16->f1),
                                  BOOL32(parg16->f2)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL HiliteMenuItem(<hwnd>, <hMenu>, <wIDHiliteItem>, <wHilite>)
    HWND <hwnd>;
    HMENU <hMenu>;
    WORD <wIDHiliteItem>;
    WORD <wHilite>;

    The %HiliteMenuItem% function highlights or removes the highlighting from a
    top-level (menu-bar) menu item.

    <hwnd>
        Identifies the window that contains the menu.

    <hMenu>
        Identifies the top-level menu that contains the item to be
        highlighted.

    <wIDHiliteItem>
        Specifies the integer identifier of the menu item or the offset
        of the menu item in the menu, depending on the value of the <wHilite>
        parameter.

    <wHilite>
        Specifies whether the menu item is highlighted or the highlight
        is removed. It can be a combination of MF_HILITE or MF_UNHILITE with
        MF_BYCOMMAND or MF_BYPOSITION. The values can be combined using the
        bitwise OR operator. These values have the following meanings:

    MF_BYCOMMAND
        Interprets wIDHiliteItem as the menu-item ID (the default
        interpretation).

    MF_BYPOSITION
        Interprets <wIDHiliteItem> as an offset.

    MF_HILITE
        Highlights the item. If this value is not given, highlighting is removed
        from the item.

    MF_UNHILITE
        Removes highlighting from the item.

    The return value specifies whether or not the menu item is highlighted the
    outcome of the function. It is TRUE if the item is highlighted was set to
    the specified highlight state. Otherwise, it is FALSE.

    The MF_HILITE and MF_UNHILITE flags can be used only with the
    %HiliteMenuItem% function; they cannot be used with the %ModifyMenu%
    function.
--*/

ULONG FASTCALL WU32HiliteMenuItem(PVDMFRAME pFrame)
{
    ULONG ul;
    register PHILITEMENUITEM16 parg16;

    GETARGPTR(pFrame, sizeof(HILITEMENUITEM16), parg16);

    ul = GETBOOL16(HiliteMenuItem(
    HWND32(parg16->f1),
    HMENU32(parg16->f2),
    WORD32(parg16->f3),
    WORD32(parg16->f4)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL InsertMenu(<hMenu>, <nPosition>, <wFlags>, <wIDNewItem>, <lpNewItem>)
    HMENU <hMenu>;
    WORD <nPosition>;
    WORD <wFlags>;
    WORD <wIDNewItem>;
    LPSTR <lpNewItem>;

    The %InsertMenu% function inserts a new menu item at the position specified
    by the <nPosition> parameter, moving other items down the menu. The
    application can specify the state of the menu item by setting values in the
    <wFlags> parameter.

    <hMenu>
        Identifies the menu to be changed.

    <nPosition>
        Specifies the menu item before which the new menu item is to be
        inserted. The interpretation of the <nPosition> parameter depends upon
        the setting of the <wFlags> parameter.

    MF_BYPOSITION
        Specifies the position of the existing menu item. The first item in the
        menu is at position zero.

        If nPosition is -1, the new menu item is appended to the end of the
        menu.

    MF_BYCOMMAND
        Specifies the command ID of the existing menu item.

    <wFlags>
        Specifies how the <nPosition> parameter is interpreted and
        information about the state of the new menu item when it is added to the
        menu. It consists of one or more values listed in the following Comments
        section.

    <wIDNewItem>
        Specifies either the command ID of the new menu item or, if
        <wFlags> is set to MF_POPUP, the menu handle of the pop-up menu.

    <lpNewItem>
        Specifies the content of the new menu item. If <wFlags> is set
        to MF_STRING (the default), then <lpNewItem> is a long pointer to a
        null-terminated string. If <wFlags> is set to MF_BITMAP instead, then
        <lpNewItem> contains a bitmap handle (%HBITMAP%) in its low-order word.
        If <wFlags> is set to MF_OWNERDRAW, <lpNewItem> specifies an
        application-supplied 32-bit value which the application can use to
        maintain additional data associated with the menu item. This 32-bit
        value is available to the application in the %itemData% member of the
        structure pointed to by the <lParam> parameter of the following
        messages:

         WM_MEASUREITEM
         WM_DRAWITEM

        These messages are sent when the menu item is initially displayed, or is
        changed.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%.

    Each of the following groups lists flags that should not be used together:

    o   MF_BYCOMMAND and MF_BYPOSITION

    o   MF_DISABLED, MF_ENABLED, and MF_GRAYED

    o   MF_BITMAP, MF_STRING, MF_OWNERDRAW, and MF_SEPARATOR

    o   MF_MENUBARBREAK and MF_MENUBREAK

    o   MF_CHECKED and MF_UNCHECKED

    The following list describes the flags which may be set in the <wFlags>
    parameter:

    MF_BITMAP
        Uses a bitmap as the item. The low-order word of the lpNewItem parameter
        contains the handle of the bitmap. MF_BYCOMMAND Specifies that the
        <nPosition> parameter gives the menu-item control ID number (default).

    MF_BYPOSITION
        Specifies that the <nPosition> parameter gives the position of the menu
        item to be changed rather than an ID number.

    MF_CHECKED
        Places a checkmark next to the menu item. If the application has
        supplied checkmark bitmaps (see the %SetMenuItemBitmaps% function),
        setting this flag displays the checkmark on bitmap next to the menu
        item.

    MF_DISABLED
        Disables the menu item so that it cannot be selected, but does not gray
        it.

    MF_ENABLED
        Enables the menu item so that it can be selected and restores it from
        its grayed state.

    MF_GRAYED
        Disables the menu item so that it cannot be selected and grays it.

    MF_MENUBARBREAK
        Same as MF_MENUBREAK except that for pop-up menus, separates the new
        column from the old column with a vertical line.

    MF_MENUBREAK
        Places the menu item on a new line for static menu-bar items. For pop-up
        menus, places the menu item in a new column, with no dividing line
        between the columns.

    MF_OWNERDRAW
        Specifies that the item is an owner-draw item. The window that owns the
        menu receives a WM_MEASUREITEM message when the menu is displayed for
        the first time to retrieve the height and width of the menu item. The
        WM_DRAWITEM message is then sent to the owner whenever the owner must
        update the visual appearance of the menu item. This option is not valid
        for a top-level menu item.

    MF_POPUP
        Specifies that the menu item has a pop-up menu associated with it. The
        <wIDNewItem> parameter specifies a handle to a pop-up menu to be
        associated with the item. Use the MF_OWNERDRAW flag to add either a
        top-level pop-up menu or a hierarchical pop-up menu to a pop-up menu
        item.

    MF_SEPARATOR
        Draws a horizontal dividing line. You can use this flag in a pop-up
        menu. This line cannot be grayed, disabled, or highlighted. Windows
        ignores the <lpNewItem> and <wIDNewItem> parameters.

    MF_STRING
        Specifies that the menu item is a character string; the <lpNewItem>
        parameter points to the string for the item.

    MF_UNCHECKED
        Does not place a checkmark next to the item (default). If the
        application has supplied checkmark bitmaps (see %SetMenuItemBitmaps%),
        setting this flag displays the "checkmark off" bitmap next to the menu
        item.
--*/

ULONG FASTCALL WU32InsertMenu(PVDMFRAME pFrame)
{
    BOOL fNeedToFreePsz5 = FALSE;
    ULONG ul;
    PSZ psz5;
    UINT w4;
    register PINSERTMENU16 parg16;

    GETARGPTR(pFrame, sizeof(INSERTMENU16), parg16);

    if (parg16->f3 & MF_BITMAP) {
        if (LOW(parg16->f5) >= 4)
            psz5 = (PSZ)HBITMAP32(LOW(parg16->f5));
        else
            psz5 = (PSZ)WORD32(parg16->f5);
    }
    else if (parg16->f3 & MF_OWNERDRAW) {
        psz5 = (PSZ)DWORD32(parg16->f5);
    }
    else if (parg16->f3 & MF_SEPARATOR) {
        // lpszNewItem is ignored when inserting a separator bar.
        psz5 = NULL;
    }
    else {
        GETPSZPTR(parg16->f5, psz5);
        fNeedToFreePsz5 = TRUE;
    }

    w4 = (parg16->f3 & MF_POPUP) ? (UINT)HMENU32(parg16->f4) : WORD32(parg16->f4);

    ul = GETBOOL16(InsertMenu(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3),
    w4,
    psz5
    ));

    if (fNeedToFreePsz5) {
        FREEPSZPTR(psz5);
    }
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HMENU LoadMenu(<hInstance>, <lpMenuName>)
    HANDLE <hInstance>;
    LPSTR <lpMenuName>;

    This function loads the menu resource named by the <lpMenuName> parameter
    from the executable file associated with the module specified by the
    <hInstance> parameter.

    <hInstance>
        Identifies an instance of the module whose executable file contains the
        menu.

    <lpMenuName>
        Points to a character string that names the menu resource. The
        string must be a null-terminated string.

    The return value identifies a menu resource if the function is successful.
    Otherwise, it is NULL.

    The <lpMenuName> parameter can contain a value created by the
    %MAKEINTRESOURCE% macro. If it does, the ID must reside in the low-order
    word of <lpMenuName>, and the high-order word must be set to zero.
--*/

ULONG FASTCALL WU32LoadMenu(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ psz2;
    register PLOADMENU16 parg16;
    DWORD cb;
    LPBYTE lpResData;

    LPWSTR lpUniName_Menu;

    GETARGPTR(pFrame, sizeof(LOADMENU16), parg16);
    psz2 = (PSZ)DWORD32(parg16->f2);
    GETPSZIDPTR(parg16->f2, psz2);

    if (HIWORD(psz2) != (WORD) NULL) {
        if (!(MBToWCS(psz2, -1, &lpUniName_Menu, -1, TRUE))) {
            FREEPSZIDPTR(psz2);
            FREEARGPTR(parg16);
            RETURN(ul);
        }
    }
    else {
        lpUniName_Menu = (LPWSTR) psz2;
    }

    cb = parg16->f4 * sizeof(WCHAR);    // see SizeofResource16
    if (cb && (lpResData = malloc_w(cb))) {
        ConvertMenu16(parg16->f5, lpResData, parg16->f3, cb, parg16->f4);

    ul = GETHMENU16((pfnOut.pfnServerLoadCreateMenu)(HMODINST32(parg16->f1),
                                              (LPTSTR) lpUniName_Menu,
                                              lpResData,
                                              cb,
                                              FALSE));



        free_w (lpResData);
    }

    if (HIWORD(psz2) != (WORD) NULL) {
        LocalFree (lpUniName_Menu);
    }

    FREEPSZIDPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HMENU LoadMenuIndirect(<lpMenuTemplate>)
    LPSTR <lpMenuTemplate>;

    The %LoadMenuIndirect% function loads into memory the menu named by the
    <lpMenuTemplate> parameter. The template specified by <lpMenuTemplate> is a
    header followed by a collection of one or more %MENUITEMTEMPLATE%
    structures, each of which may contain one or more menu items and pop-up
    menus.

    <lpMenuTemplate>
        Points to a menu template (which is a collection of one or more
        %MENUITEMTEMPLATE% structures).

    The return value identifies the menu if the function is successful.
    Otherwise, it is NULL.
--*/

ULONG FASTCALL WU32LoadMenuIndirect(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    DWORD cb = 0;
    PVOID pMenu32;
    PLOADMENUINDIRECT16 parg16;

    GETARGPTR(pFrame, sizeof(LOADMENUINDIRECT16), parg16);
    /*
     * we need to convert this randomly created 16-bit resource into a
     * 32-bit resource so that NT user will be able to use it.
     */
    if ((cb = (DWORD)ConvertMenu16((WORD)0x300, NULL, (VPBYTE)parg16->f1, cb, 0)) != 0) {
        pMenu32 = malloc_w(cb);
        if (pMenu32 != NULL) {
            ConvertMenu16((WORD)0x300, pMenu32, (VPBYTE)parg16->f1, cb, 0);
            ul = GETHMENU16(LoadMenuIndirect(pMenu32));
            free_w(pMenu32);
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL ModifyMenu(<hMenu>, <nPosition>, <wFlags>, <wIDNewItem>, <lpNewItem>)
    HMENU <hMenu>;
    WORD <nPosition>;
    WORD <wFlags>;
    WORD <wIDNewItem>;
    LPSTR <lpNewItem>;

    The %ModifyMenu% function changes an existing menu item at the position
    specified by the <nPosition> parameter. The application specifies the new
    state of the menu item by setting values in the <wFlags> parameter. If this
    function replaces a pop-up menu associated with the menu item, it destroys
    the old pop-up menu and frees the memory used by the pop-up menu.

    <hMenu>
        Identifies the menu to be changed.

    <nPosition>
        Specifies the menu item to be changed. The interpretation of the
        <nPosition> parameter depends upon the setting of the <wFlags>
        parameter.

    MF_BYPOSITION
        Specifies the position of the existing menu item. The first item in the
        menu is at position zero.

    MF_BYCOMMAND
        Specifies the command ID of the existing menu item.

    <wFlags>
        Specifies how the <nPosition> parameter is interpreted and
        information about the changes to be made to the menu item. It consists
        of one or more values listed in the following Comments section.

    <wIDNewItem>
        Specifies either the command ID of the modified menu item or, if
        <wFlags> is set to MF_POPUP, the menu handle of the pop-up menu.

    <lpNewItem>
        Specifies the content of the changed menu item. If <wFlags> is
        set to MF_STRING (the default), then <lpNewItem> is a long pointer to a
        null-terminated string. If <wFlags> is set to MF_BITMAP instead, then
        <lpNewItem> contains a bitmap handle (%HBITMAP%) in its low-order word.
        If <wFlags> is set to MF_OWNERDRAW, <lpNewItem> specifies an
        application-supplied 32-bit value which the application can use to
        maintain additional data associated with the menu item. This 32-bit
        value is available to the application in the %itemData% field of the
        structure, pointed to by the <lParam> parameter of the following
        messages:

         WM_MEASUREITEM
         WM_DRAWITEM

        These messages are sent when the menu item is initially displayed, or is
        changed.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%. In order to change
    the attributes of existing menu items, it is much faster to use the
    %CheckMenuItem% and %EnableMenuItem% functions.

    Each of the following groups lists flags that should not be used together:

    o   MF_BYCOMMAND and MF_BYPOSITION

    o   MF_DISABLED, MF_ENABLED, and MF_GRAYED

    o   MF_BITMAP, MF_STRING, MF_OWNERDRAW, and MF_SEPARATOR

    o   MF_MENUBARBREAK and MF_MENUBREAK

    o   MF_CHECKED and MF_UNCHECKED

    The following list describes the flags which may be set in the <wFlags>
    parameter:

    MF_BITMAP
        Uses a bitmap as the menu item. The low-order word of the lpNewItem
        parameter contains the handle of the bitmap.

    MF_BYCOMMAND
        Specifies that the <nPosition> parameter gives the menu item control ID
        number. This is the default if neither MF_BYCOMMAND nor MF_POSITION is
        set.

    MF_BYPOSITION
        Specifies that the <nPosition> parameter gives the position of the menu
        item to be changed rather than an ID number.

    MF_CHECKED
        Places a checkmark next to the menu item. If the application has
        supplied checkmark bitmaps (see %SetMenuItemBitmaps%), setting this flag
        displays the checkmark on bitmap next to the menu item.

    MF_DISABLED
        Disables the menu item so that it cannot be selected, but does not gray
        it.

    MF_ENABLED
        Enables the menu item so that it can be selected and restores it from
        its grayed state.

    MF_GRAYED
        Disables the menu item so that it cannot be selected and grays it.

    MF_MENUBARBREAK
        Same as MF_MENUBREAK except that for pop-up menus, separates the new
        column from the old column with a vertical line.

    MF_MENUBREAK
        Places the menu item on a new line for static menu-bar items. For pop-up
        menus, this flag places the item in a new column, with no dividing line
        between the columns.

    MF_OWNERDRAW
        Specifies that the menu item is an owner-draw item. The window that owns
        the menu receives a WM_MEASUREITEM message when the menu is displayed
        for the first time to retrieve the height and width of the menu item.
        The WM_DRAWITEM message is then sent whenever the owner must update the
        visual appearance of the menu item. This option is not valid for a
        top-level menu item.

    MF_POPUP
        Specifies that the item has a pop-up menu associated with it. The
        <wIDNewItem> parameter specifies a handle to a pop-up menu to be
        associated with the menu item. Use this flag for adding either a
        top-level pop-up menu or adding a hierarchical pop-up menu to a pop-up
        menu item.

    MF_SEPARATOR
        Draws a horizontal dividing line. You can only use this flag in a pop-up
        menu. This line cannot be grayed, disabled, or highlighted. The
        <lpNewItem> and <wIDNewItem> parameters are ignored.

    MF_STRING
        Specifies that the menu item is a character string; the <lpNewItem>
        parameter points to the string for the menu item.

    MF_UNCHECKED
        Does not place a checkmark next to the menu item. No checkmark is the
        default if neither MF_CHECKED nor MF_UNCHECKED is set. If the
        application has supplied checkmark bitmaps (see %SetMenuItemBitmaps%),
        setting this flag displays the checkmark off bitmap next to the menu
        item.
--*/

ULONG FASTCALL WU32ModifyMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz5;
    register PMODIFYMENU16 parg16;
    UINT wIDNewItem;



    GETARGPTR(pFrame, sizeof(MODIFYMENU16), parg16);

    if (parg16->f3 & MF_BITMAP) {
        if (LOW16(parg16->f5) >= 4)
            psz5 = (PSZ)HBITMAP32(LOW(parg16->f5));
        else
            psz5 = (PSZ)WORD32(parg16->f5);
    }
    else if (parg16->f3 & MF_OWNERDRAW)
        psz5 = (PSZ)DWORD32(parg16->f5);
    else
        GETPSZPTR(parg16->f5, psz5);

    wIDNewItem = (UINT) WORD32(parg16->f4);

    if (parg16->f3 & MF_POPUP)
        wIDNewItem = (UINT) HMENU32(parg16->f4);



    ul = GETBOOL16(ModifyMenu(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3),
    wIDNewItem,
    psz5
    ));

    if ( ul == FALSE && (parg16->f3 & MF_POPUP) ) {
        //
        // PowerPoint v4.0c passes an wIDNewItem which is not a menu handle
        // when they do pass MF_POPUP.  This hack allows it to avoid the
        // error path in WINSRV.  On Win 3.1, they never validated it so it
        // got through.  Luckily they quickly modify the menu to not have
        // a popup menu soon after that.
        //
        if ( !IsMenu((HMENU) wIDNewItem) ) {
            //
            // Try again with a better sub-menu handle.
            //
            wIDNewItem = (UINT)GetSubMenu( HMENU32(parg16->f1),
                                           WORD32(parg16->f2) );

            ul = GETBOOL16(ModifyMenu( HMENU32(parg16->f1),
                                       WORD32(parg16->f2),
                                       WORD32(parg16->f3),
                                       wIDNewItem,
                                       psz5 ));
        }
    }

    FREEPSZPTR(psz5);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL RemoveMenu(<hMenu>, <nPosition>, <wFlags>)
    HMENU <hMenu>;
    WORD <nPosition>;
    WORD <wFlags>;

    The %RemoveMenu% function deletes an menu item with an associated pop-up
    menu from the menu identified by the <hMenu> parameter but does not destroy
    the handle for the pop-up menu, allowing the menu to be reused. Before
    calling this function, the application should call %GetSubMenu% to retrieve
    the pop-up menu handle.

    <hMenu>
        Identifies the menu to be changed.

    <nPosition>
        Specifies the menu item to be removed. The interpretation of the
        <nPosition> parameter depends upon the setting of the <wFlags>
        parameter.

    MF_BYCOMMAND
        Specifies the command ID of the existing menu item.

    MF_BYPOSITION
        Specifies the position of the menu item. The first item in the menu is
        at position zero.

    <wFlags>
        Specifies how the <nPosition> parameter is interpreted. It must
        be either MF_BYCOMMAND or MF_BYPOSITION.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    Whenever a menu changes (whether or not the menu resides in a window that is
    displayed), the application should call %DrawMenuBar%.
--*/

ULONG FASTCALL WU32RemoveMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PREMOVEMENU16 parg16;

    GETARGPTR(pFrame, sizeof(REMOVEMENU16), parg16);

    ul = GETBOOL16(RemoveMenu(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL SetMenu(<hwnd>, <hMenu>)
    HWND <hwnd>;
    HMENU <hMenu>;

    The %SetMenu% function sets the given window's menu to the menu specified by
    the <hMenu> parameter. If <hMenu> is NULL, the window's current menu is
    removed. The %SetMenu% function causes the window to be redrawn to reflect
    the menu change.

    <hwnd>
        Identifies the window whose menu is to be changed.

    <hMenu>
        Identifies the new menu.

    The return value specifies whether the menu is changed. It is TRUE if the
    menu is changed. Otherwise, it is FALSE.

    %SetMenu% will not destroy a previous menu. An application should call the
    %DestroyMenu% function to accomplish this task.
--*/

ULONG FASTCALL WU32SetMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETMENU16 parg16;

    GETARGPTR(pFrame, sizeof(SETMENU16), parg16);

    ul = GETBOOL16(SetMenu(
    HWND32(parg16->f1),
    HMENU32(parg16->f2)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL SetMenuItemBitmaps(<hMenu>, <nPosition>, <wFlags>,
        <hBitmapUnchecked>, <hBitmapChecked>)
    HMENU <hMenu>;
    WORD <nPosition>;
    WORD <wFlags>;
    HBITMAP <hBitmapUnchecked>;
    HBITMAP <hBitmapChecked>;

    The %SetMenuItemBitmaps% function associates the specified bitmaps with a
    menu item. Whether the menu item is checked or unchecked, Windows displays
    the appropriate bitmap next to the menu item.

    <hMenu>
        Identifies the menu to be changed.

    <nPosition>
        Specifies the menu item to be changed. If <wFlags> is set to
        MF_BYPOSITION, <nPosition> specifies the position of the menu item; the
        first item in the menu is at position 0. If <wFlags> is set to
        MF_BYCOMMAND, then <nPosition> specifies the command ID of the menu
        item.

    <wFlags>
        Specifies how the <nPosition> parameter is interpreted. It may be
        set to MF_BYCOMMAND (the default) or MF_BYPOSITION.

    <hBitmapUnchecked>
        Identifies the bitmap to be displayed when the menu item is
        not checked.

    <hBitmapChecked>
        Identifies the bitmap to be displayed when the menu item is
        checked.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.

    If either the <hBitmapUnchecked> or the <hBitmapChecked> parameters is NULL,
    then Windows displays nothing next to the menu item for the corresponding
    attribute. If both parameters are NULL, Windows uses the default checkmark
    when the item is checked and removes the checkmark when the item is
    unchecked.

    When the menu is destroyed, these bitmaps are not destroyed; it is the
    responsibility of the application to destroy them.

    The %GetMenuCheckMarkDimensions% function retrieves the dimensions of the
    default checkmark used for menu items. The application should use these
    values to determine the appropriate size for the bitmaps supplied with this
    function.
--*/

ULONG FASTCALL WU32SetMenuItemBitmaps(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETMENUITEMBITMAPS16 parg16;

    GETARGPTR(pFrame, sizeof(SETMENUITEMBITMAPS16), parg16);

    ul = GETBOOL16(SetMenuItemBitmaps(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    WORD32(parg16->f3),
    HBITMAP32(parg16->f4),
    HBITMAP32(parg16->f5)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL TrackPopupMenu(<hMenu>, <wFlags>, <x>, <y>, <nReserved>, <hwnd>,
        <lpReserved>)

    The %TrackPopupMenu% function displays a floating pop-up menu at the
    specified location and tracks the selection of items on the pop-up menu. A
    floating pop-up menu can appear anywhere on the screen. The <hMenu>
    parameter specifies the handle of the menu to be displayed; the application
    obtains this handle by calling %CreatePopupMenu% to create a new pop-up menu
    or by calling %GetSubMenu% to retrieve the handle of a pop-up menu
    associated with an existing menu item.

    Windows sends messages generated by the menu to the window identified by the
    <hwnd> parameter.

    <hMenu>
        Identifies the pop-up menu to be displayed.

    <wFlags>
        Not used. This parameter must be set to zero.

    <x>
        Specifies the horizontal position in screen coordinates of the
        left side of the menu on the screen.

    <y>
        Specifies the vertical position in screen coordinates of the top
        of the menu on the screen.

    <nReserved>
        Is reserved and must be set to zero.

    <hwnd>
        Identifies the window which owns the pop-up menu. This window
        receives all WM_COMMAND messages from the menu.

    <lpReserved>
        Is reserved and must be set to NULL.

    The return value specifies the outcome of the function. It is TRUE if the
    function is successful. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32TrackPopupMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t7, *p7;
    register PTRACKPOPUPMENU16 parg16;

    GETARGPTR(pFrame, sizeof(TRACKPOPUPMENU16), parg16);
    p7 = GETRECT16(parg16->f7, &t7);

    ul = GETBOOL16(TrackPopupMenu(
    HMENU32(parg16->f1),
    WORD32(parg16->f2),
    INT32(parg16->f3),
    INT32(parg16->f4),
    INT32(parg16->f5),
    HWND32(parg16->f6),
    p7
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}
