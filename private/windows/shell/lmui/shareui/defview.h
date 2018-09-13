// defview.h: Stolen from shelldll and stripped

// This is from shlobjp.h:

#ifndef FCIDM_SHVIEWFIRST

//--------------------------------------------------------------------------
//
// Command/menuitem IDs
//
//  The explorer dispatches WM_COMMAND messages based on the range of
// command/menuitem IDs. All the IDs of menuitems that the view (right
// pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer
// won't dispatch them). The view should not deal with any menuitems
// in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future
// version of the shell).
//
//  FCIDM_SHVIEWFIRST/LAST      for the right pane (IShellView)
//  FCIDM_BROWSERFIRST/LAST     for the explorer frame (IShellBrowser)
//  FCIDM_GLOBAL/LAST           for the explorer's submenu IDs
//
//--------------------------------------------------------------------------

#define FCIDM_SHVIEWFIRST           0x0000
#define FCIDM_SHVIEWLAST            0x7fff
#define FCIDM_BROWSERFIRST          0xa000
#define FCIDM_BROWSERLAST           0xbf00
#define FCIDM_GLOBALFIRST           0x8000
#define FCIDM_GLOBALLAST            0x9fff

//
// Global submenu IDs and separator IDs
//
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)

//--------------------------------------------------------------------------
// control IDs known to the view
//--------------------------------------------------------------------------

#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)
#define FCIDM_DRIVELIST    (FCIDM_BROWSERFIRST + 2) /* NOT_EVEN_IN_B_LIST */
#define FCIDM_TREE         (FCIDM_BROWSERFIRST + 3) /* NOT_EVEN_IN_B_LIST */
#define FCIDM_TABS         (FCIDM_BROWSERFIRST + 4) /* NOT_EVEN_IN_B_LIST */

#endif // FCIDM_SHVIEWFIRST


// This is from shellp.h:

#ifndef SFVIDM_FIRST

// to subtract for it.
#if (FCIDM_SHVIEWLAST != 0x7fff)
#error FCIDM_SHVIEWLAST has changed, so shellp.h needs to also
#endif

#define SFVIDM_FIRST                    (0x7000)
#define SFVIDM_LAST                     (FCIDM_SHVIEWLAST)

// Popup menu ID's used in merging menus
#define SFVIDM_MENU_ARRANGE     (SFVIDM_FIRST + 0x0001)
#define SFVIDM_MENU_VIEW        (SFVIDM_FIRST + 0x0002)
#define SFVIDM_MENU_SELECT      (SFVIDM_FIRST + 0x0003)

#endif // SFVIDM_FIRST

// This is from defview.h:

#define SHARED_FILE_FIRST               0x0010
#define SHARED_FILE_LINK                (SHARED_FILE_FIRST + 0x0000)
#define SHARED_FILE_DELETE              (SHARED_FILE_FIRST + 0x0001)
#define SHARED_FILE_RENAME              (SHARED_FILE_FIRST + 0x0002)
#define SHARED_FILE_PROPERTIES          (SHARED_FILE_FIRST + 0x0003)

#define SHARED_EDIT_FIRST               0x0018
#define SHARED_EDIT_CUT                 (SHARED_EDIT_FIRST + 0x0000)
#define SHARED_EDIT_COPY                (SHARED_EDIT_FIRST + 0x0001)
#define SHARED_EDIT_PASTE               (SHARED_EDIT_FIRST + 0x0002)
#define SHARED_EDIT_UNDO                (SHARED_EDIT_FIRST + 0x0003)
#define SHARED_EDIT_PASTELINK           (SHARED_EDIT_FIRST + 0x0004)
#define SHARED_EDIT_PASTESPECIAL        (SHARED_EDIT_FIRST + 0x0005)

#define SFVIDM_FILE_FIRST               (SFVIDM_FIRST + SHARED_FILE_FIRST)
#define SFVIDM_FILE_LINK                (SFVIDM_FIRST + SHARED_FILE_LINK)
#define SFVIDM_FILE_DELETE              (SFVIDM_FIRST + SHARED_FILE_DELETE)
#define SFVIDM_FILE_RENAME              (SFVIDM_FIRST + SHARED_FILE_RENAME)
#define SFVIDM_FILE_PROPERTIES          (SFVIDM_FIRST + SHARED_FILE_PROPERTIES)

#define SFVIDM_EDIT_FIRST               (SFVIDM_FIRST + SHARED_EDIT_FIRST)
#define SFVIDM_EDIT_CUT                 (SFVIDM_FIRST + SHARED_EDIT_CUT)
#define SFVIDM_EDIT_COPY                (SFVIDM_FIRST + SHARED_EDIT_COPY)
#define SFVIDM_EDIT_PASTE               (SFVIDM_FIRST + SHARED_EDIT_PASTE)
#define SFVIDM_EDIT_UNDO                (SFVIDM_FIRST + SHARED_EDIT_UNDO)
#define SFVIDM_EDIT_PASTELINK           (SFVIDM_FIRST + SHARED_EDIT_PASTELINK)
#define SFVIDM_EDIT_PASTESPECIAL        (SFVIDM_FIRST + SHARED_EDIT_PASTESPECIAL)

#define SFVIDM_SELECT_FIRST             (SFVIDM_FIRST + 0x0020)
#define SFVIDM_SELECT_ALL               (SFVIDM_SELECT_FIRST + 0x0001)
#define SFVIDM_SELECT_INVERT            (SFVIDM_SELECT_FIRST + 0x0002)
#define SFVIDM_DESELECT_ALL             (SFVIDM_SELECT_FIRST + 0x0003)

#define SFVIDM_VIEW_FIRST               (SFVIDM_FIRST + 0x0028)
#define SFVIDM_VIEW_ICON                (SFVIDM_VIEW_FIRST + 0x0001)
#define SFVIDM_VIEW_SMALLICON           (SFVIDM_VIEW_FIRST + 0x0002)
#define SFVIDM_VIEW_LIST                (SFVIDM_VIEW_FIRST + 0x0003)
#define SFVIDM_VIEW_DETAILS             (SFVIDM_VIEW_FIRST + 0x0004)
#define SFVIDM_VIEW_OPTIONS             (SFVIDM_VIEW_FIRST + 0x0005)

#define SFVIDM_ARRANGE_FIRST            (SFVIDM_FIRST + 0x0030)
#define SFVIDM_ARRANGE_AUTO             (SFVIDM_ARRANGE_FIRST + 0x0001)
#define SFVIDM_ARRANGE_GRID             (SFVIDM_ARRANGE_FIRST + 0x0002)

#define SFVIDM_TOOL_FIRST               (SFVIDM_FIRST + 0x0035)
#define SFVIDM_TOOL_CONNECT             (SFVIDM_TOOL_FIRST + 0x0001)
#define SFVIDM_TOOL_DISCONNECT          (SFVIDM_TOOL_FIRST + 0x0002)

#define SFVIDM_HELP_FIRST               (SFVIDM_FIRST + 0x0040)
#define SFVIDM_HELP_TOPIC               (SFVIDM_HELP_FIRST + 0x0001)

#define SFVIDM_MISC_FIRST               (SFVIDM_FIRST + 0x0100)
#define SFVIDM_MISC_MENUTERM1           (SFVIDM_MISC_FIRST + 0x0001)
#define SFVIDM_MISC_MENUTERM2           (SFVIDM_MISC_FIRST + 0x0002)

//
// Reserved for debug only commands for defview
//
#define SFVIDM_DEBUG_FIRST              (SFVIDM_FIRST + 0x0180)
#define SFVIDM_DEBUG_LAST               (SFVIDM_FIRST + 0x01ff)
#define SFVIDM_DEBUG_HASH               (SFVIDM_DEBUG_FIRST + 10)
#define SFVIDM_DEBUG_MEMMON             (SFVIDM_DEBUG_FIRST + 11)
#define SFVIDM_DEBUG_ICON               (SFVIDM_DEBUG_FIRST + 12)
#define SFVIDM_DEBUG_ICON_SAVE          (SFVIDM_DEBUG_FIRST + 13)
#define SFVIDM_DEBUG_ICON_FLUSH         (SFVIDM_DEBUG_FIRST + 14)

// Range for the client's additional menus
#define SFVIDM_CLIENT_FIRST             (SFVIDM_FIRST + 0x0200)
#define SFVIDM_CLIENT_LAST              (SFVIDM_FIRST + 0x02ff)

// Range for context menu id's
#define SFVIDM_CONTEXT_FIRST            (SFVIDM_FIRST + 0x0800)
#define SFVIDM_CONTEXT_LAST             (SFVIDM_FIRST + 0x0900)
