/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Menu.h

Abstract:

    This module contains the function prototypes and identifiers for
    Windbg's menus and menu items.

Author:

    David J. Gilman (davegi) 15-May-1992

Environment:

    Win32, User Mode

--*/


//
// Offset from the bottom of the menu to the popup menu
//
//
//
//      File
//      |----------------|
//      | Open           |
//
//          etc....
//
//      |----------------|
//      | MRU Files     >|       GetMenuItemCount() - 4
//      | MRU Files     >|       GetMenuItemCount() - 3
//      |----------------|       GetMenuItemCount() - 2
//      | Exit           |       GetMenuItemCount() - 1
//      |----------------|
//

#define MRU_FILES_OFFSET_FROM_BOTTOM                ( 4 )
#define MRU_WORKSPACES_OFFSET_FROM_BOTTOM           ( 3 )



//
// Number of menus w/o a maximized child.
//

#define NUMBER_OF_MENUS            ( 6 )
//
// Maximum MRU names for File and Program menu.
//

#define MAX_MRU_FILES_KEPT          ( 16 )

//
// Width of names in File and Program menu.
//

#define FILES_MENU_WIDTH            ( 72 )

// NOTENOTE davegi From here to the next NOTENOTE needs to be removed.

//Keep the 4 most recently used files (editor and project)
extern HANDLE hFileKept[PROJECT_FILE + 1][MAX_MRU_FILES_KEPT];
extern int nbFilesKept[PROJECT_FILE + 1];

//Window submenu
extern HMENU hWindowSubMenu;

//Last menu id & id state
extern IWORD FAR lastMenuId;
extern IWORD FAR lastMenuIdState;

void InsertKeptFileNames(
        WORD wType,
        int nMenuPos,
        LPSTR lpszNewName);

UINT
CommandIdEnabled(
    IN UINT MenuID
    );

void AddWindowMenuItem(
    int doc,
    int view);

void DeleteWindowMenuItem(
    int view);

int FindWindowMenuId(
    WORD type,
    int viewLimit,
    BOOL sendDocMenuId);

// NOTENOTE davegi See above.

//
// Handle to main window menu.
//

extern HMENU hMainMenu;

//
//  INT
//  GetActualMenuCount(
//      IN HMENU hMenu
//      );
//

#define GetActualMenuCount( )                                       \
    ( GetMenuItemCount(( HMENU )( hMainMenu )) == NUMBER_OF_MENUS )        \
    ? NUMBER_OF_MENUS                                                      \
    : NUMBER_OF_MENUS + 1

UINT
GetPopUpMenuID(
    IN HMENU hMenu
    );

VOID
InitializeMenu(
    IN HMENU hmenu
    );


//
// Menu Resource Signature
//

#define MENU_SIGNATURE              0x4000

//
// BOOL
// IsMenuID(
//     IN DWORD ID
//     )
//
// Look for MENU_SIGNATURE while ensuring that we don't find
// SC_* IDs.
//

#if SC_SIZE != 0xF000
#error IsMenuID incompatible.
#endif

#define IsMenuID( ID )              \
    ((( ID ) & MENU_SIGNATURE ) && ( ! (( ID ) & SC_SIZE )))

//
// Accelerator IDs
//

#define IDA_BASE                    ( 10000 )
#define IDA_FINDNEXT                ( IDA_BASE + 1 )

// NOTENOTE davegi Get rid of FIRST/LAST bull.

//
// Base menu ID
//
// Note that I would have liked to define each pop-up menu ID as
//
//  #define IDM_FILE                    (( IDM_BASE * 1 ) | MENU_SIGNATURE )
//
// but RC doesn't support multiplication.
//

#define IDM_BASE                    ( 100 )


//
// File
//

#define IDM_FILE                    ( 100 | MENU_SIGNATURE )
#define IDM_FILE_OPEN               ( IDM_FILE + 1 )
#define IDM_FILE_CLOSE              ( IDM_FILE + 2 )
#define IDM_FILE_OPEN_EXECUTABLE    ( IDM_FILE + 3 )
#define IDM_FILE_OPEN_CRASH_DUMP    ( IDM_FILE + 4 )
#define IDM_FILE_NEW_WORKSPACE      ( IDM_FILE + 5 )
#define IDM_FILE_SAVE_WORKSPACE     ( IDM_FILE + 7 )
#define IDM_FILE_SAVEAS_WORKSPACE   ( IDM_FILE + 8 )

// MRUs must be in sequential order. That way a position can be
// calculated by: IDM_FILE_MRU_FILE5 - IDM_FILE_MRU_FILE1, etc...
#define IDM_FILE_MRU_FILE1          ( IDM_FILE + 10 )
#define IDM_FILE_MRU_FILE2          ( IDM_FILE + 11 )
#define IDM_FILE_MRU_FILE3          ( IDM_FILE + 12 )
#define IDM_FILE_MRU_FILE4          ( IDM_FILE + 13 )
#define IDM_FILE_MRU_FILE5          ( IDM_FILE + 14 )
#define IDM_FILE_MRU_FILE6          ( IDM_FILE + 15 )
#define IDM_FILE_MRU_FILE7          ( IDM_FILE + 16 )
#define IDM_FILE_MRU_FILE8          ( IDM_FILE + 17 )
#define IDM_FILE_MRU_FILE9          ( IDM_FILE + 18 )
#define IDM_FILE_MRU_FILE10         ( IDM_FILE + 19 )
#define IDM_FILE_MRU_FILE11         ( IDM_FILE + 20 )
#define IDM_FILE_MRU_FILE12         ( IDM_FILE + 21 )
#define IDM_FILE_MRU_FILE13         ( IDM_FILE + 22 )
#define IDM_FILE_MRU_FILE14         ( IDM_FILE + 23 )
#define IDM_FILE_MRU_FILE15         ( IDM_FILE + 24 )
#define IDM_FILE_MRU_FILE16         ( IDM_FILE + 25 )

// ditto. same as above
#define IDM_FILE_MRU_WORKSPACE1     ( IDM_FILE + 26 )
#define IDM_FILE_MRU_WORKSPACE2     ( IDM_FILE + 27 )
#define IDM_FILE_MRU_WORKSPACE3     ( IDM_FILE + 28 )
#define IDM_FILE_MRU_WORKSPACE4     ( IDM_FILE + 29 )
#define IDM_FILE_MRU_WORKSPACE5     ( IDM_FILE + 30 )
#define IDM_FILE_MRU_WORKSPACE6     ( IDM_FILE + 31 )
#define IDM_FILE_MRU_WORKSPACE7     ( IDM_FILE + 32 )
#define IDM_FILE_MRU_WORKSPACE8     ( IDM_FILE + 33 )
#define IDM_FILE_MRU_WORKSPACE9     ( IDM_FILE + 34 )
#define IDM_FILE_MRU_WORKSPACE10    ( IDM_FILE + 35 )
#define IDM_FILE_MRU_WORKSPACE11    ( IDM_FILE + 36 )
#define IDM_FILE_MRU_WORKSPACE12    ( IDM_FILE + 37 )
#define IDM_FILE_MRU_WORKSPACE13    ( IDM_FILE + 38 )
#define IDM_FILE_MRU_WORKSPACE14    ( IDM_FILE + 39 )
#define IDM_FILE_MRU_WORKSPACE15    ( IDM_FILE + 40 )
#define IDM_FILE_MRU_WORKSPACE16    ( IDM_FILE + 41 )

// Included temporarily
#define IDM_FILE_MANAGE_WORKSPACE   ( IDM_FILE + 42 )
#define IDM_FILE_SAVE_AS_WINDOW_LAYOUTS  ( IDM_FILE + 43 )
#define IDM_FILE_MANAGE_WINDOW_LAYOUTS   ( IDM_FILE + 44 )

#define IDM_FILE_EXIT               ( IDM_FILE + 45)
#define IDM_FILE_FIRST              IDM_FILE
#define IDM_FILE_LAST               IDM_FILE_EXIT


//
// Edit
//

#define IDM_EDIT                    ( 200 | MENU_SIGNATURE )
#define IDM_EDIT_CUT                ( IDM_EDIT + 1 )
#define IDM_EDIT_COPY               ( IDM_EDIT + 2 )
#define IDM_EDIT_PASTE              ( IDM_EDIT + 3 )
#define IDM_EDIT_DELETE             ( IDM_EDIT + 4 )
#define IDM_EDIT_FIND               ( IDM_EDIT + 5 )
#define IDM_EDIT_REPLACE            ( IDM_EDIT + 6 )
#define IDM_EDIT_GOTO_ADDRESS       ( IDM_EDIT + 7 )
#define IDM_EDIT_GOTO_LINE          ( IDM_EDIT + 8 )
#define IDM_EDIT_BREAKPOINTS        ( IDM_EDIT + 9 )
#define IDM_EDIT_TOGGLEBREAKPOINT   ( IDM_EDIT + 10 )
#define IDM_EDIT_PROPERTIES         ( IDM_EDIT + 11 )
#define IDM_EDIT_FIRST              IDM_EDIT
#define IDM_EDIT_LAST               IDM_EDIT_PROPERTIES


//
// View
//

#define IDM_VIEW                    ( 300 | MENU_SIGNATURE )
#define IDM_VIEW_WATCH              ( IDM_VIEW + 1 )
#define IDM_VIEW_CALLSTACK          ( IDM_VIEW + 2 )
#define IDM_VIEW_MEMORY             ( IDM_VIEW + 3 )
#define IDM_VIEW_LOCALS             ( IDM_VIEW + 4 )
#define IDM_VIEW_REGISTERS          ( IDM_VIEW + 5 )
#define IDM_VIEW_DISASM             ( IDM_VIEW + 6 )
#define IDM_VIEW_COMMAND            ( IDM_VIEW + 7 )
#define IDM_VIEW_FLOAT              ( IDM_VIEW + 8 )
#define IDM_VIEW_TOGGLETAG          ( IDM_VIEW + 9 )
#define IDM_VIEW_NEXTTAG            ( IDM_VIEW + 10 )
#define IDM_VIEW_PREVIOUSTAG        ( IDM_VIEW + 11 )
#define IDM_VIEW_CLEARALLTAGS       ( IDM_VIEW + 12 )
#define IDM_VIEW_TOOLBAR            ( IDM_VIEW + 13 )
#define IDM_VIEW_STATUS             ( IDM_VIEW + 14 )
#define IDM_VIEW_FONT               ( IDM_VIEW + 15 )
#define IDM_VIEW_COLORS             ( IDM_VIEW + 16 )
#define IDM_VIEW_OPTIONS            ( IDM_VIEW + 17 )
#define IDM_VIEW_FIRST              IDM_VIEW
#define IDM_VIEW_LAST               IDM_VIEW_OPTIONS


//
// Debug
//

#define IDM_DEBUG                   ( 400 | MENU_SIGNATURE )
#define IDM_DEBUG_GO                ( IDM_DEBUG + 1 )
#define IDM_DEBUG_RESTART           ( IDM_DEBUG + 2 )
#define IDM_DEBUG_STOPDEBUGGING     ( IDM_DEBUG + 3 )
#define IDM_DEBUG_BREAK             ( IDM_DEBUG + 4 )
#define IDM_DEBUG_STEPINTO          ( IDM_DEBUG + 5 )
#define IDM_DEBUG_STEPOVER          ( IDM_DEBUG + 6 )
#define IDM_DEBUG_RUNTOCURSOR       ( IDM_DEBUG + 7 )
#define IDM_DEBUG_QUICKWATCH        ( IDM_DEBUG + 8 )
#define IDM_DEBUG_SOURCE_MODE       ( IDM_DEBUG + 9 )
#define IDM_DEBUG_ADVANCED          ( IDM_DEBUG + 10 )
#define IDM_DEBUG_EXCEPTIONS        ( IDM_DEBUG + 11 )
#define IDM_DEBUG_SET_THREAD        ( IDM_DEBUG + 12 )
#define IDM_DEBUG_SET_PROCESS       ( IDM_DEBUG + 13 )
#define IDM_DEBUG_ATTACH            ( IDM_DEBUG + 14 )
#define IDM_DEBUG_GO_HANDLED        ( IDM_DEBUG + 15 )
#define IDM_DEBUG_GO_UNHANDLED      ( IDM_DEBUG + 16 )

// These are not used by the menu but by the toolbar
#define IDM_DEBUG_SOURCE_MODE_ON    ( IDM_DEBUG + 17 )
#define IDM_DEBUG_SOURCE_MODE_OFF   ( IDM_DEBUG + 18 )

// Not used by the toolbar or menu, but by the accelerator table
#define IDM_DEBUG_CTRL_C            ( IDM_DEBUG + 19 )

#define IDM_DEBUG_FIRST             IDM_DEBUG
#define IDM_DEBUG_LAST              IDM_DEBUG_GO_UNHANDLED


//
// Window
//

#define IDM_WINDOW                  ( 500 | MENU_SIGNATURE )
#define IDM_WINDOW_NEWWINDOW        ( IDM_WINDOW + 1 )
#define IDM_WINDOW_CASCADE          ( IDM_WINDOW + 2 )
#define IDM_WINDOW_TILE_HORZ        ( IDM_WINDOW + 3 )
#define IDM_WINDOW_TILE_VERT        ( IDM_WINDOW + 4 )
#define IDM_WINDOW_ARRANGE          ( IDM_WINDOW + 5 )
#define IDM_WINDOW_ARRANGE_ICONS    ( IDM_WINDOW + 6 )
#define IDM_WINDOW_SOURCE_OVERLAY   ( IDM_WINDOW + 7 )
#define IDM_WINDOWCHILD             ( IDM_WINDOW + 8 )
#define IDM_WINDOW_FIRST            IDM_WINDOW
#define IDM_WINDOW_LAST             IDM_WINDOWCHILD


// Used for testing new windows
#define IDM_TEST_NEW_WND            (IDM_WINDOW_LAST +1)


//
// Help
//

#define IDM_HELP                    ( 900 | MENU_SIGNATURE )
#define IDM_HELP_CONTENTS           ( IDM_HELP + 1 )
#define IDM_HELP_SEARCH             ( IDM_HELP + 2 )
#define IDM_HELP_ABOUT              ( IDM_HELP + 3 )
#define IDM_HELP_FIRST              IDM_HELP
#define IDM_HELP_LAST               IDM_HELP_ABOUT


