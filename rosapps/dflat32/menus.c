/* -------------- menus.c ------------- */

#include "dflat.h"

/* --------------------- the main menu --------------------- */
DEFMENU(MainMenu)
    /* --------------- the File popdown menu ----------------*/
    POPDOWN( "~File",  PrepFileMenu, "Read/write/print files. Go to DOS" )
        SELECTION( "~New",        ID_NEW,          0, 0 )
        SELECTION( "~Open...",    ID_OPEN,         0, 0 )
        SEPARATOR
        SELECTION( "~Save",       ID_SAVE,     ALT_S, INACTIVE)
        SELECTION( "Save ~as...", ID_SAVEAS,       0, INACTIVE)
        SELECTION( "D~elete",     ID_DELETEFILE,   0, INACTIVE)
        SEPARATOR
        SELECTION( "~Print",      ID_PRINT,        0, INACTIVE)
        SELECTION( "P~rinter setup...", ID_PRINTSETUP, 0, 0   )
        SEPARATOR
        SELECTION( "~DOS",        ID_DOS,          0, 0 )
        SELECTION( "E~xit",       ID_EXIT,     ALT_X, 0 )
    ENDPOPDOWN

    /* --------------- the Edit popdown menu ----------------*/
    POPDOWN( "~Edit", PrepEditMenu, "Clipboard, delete text, paragraph" )
        SELECTION( "~Undo",      ID_UNDO,  ALT_BS,    INACTIVE)
        SEPARATOR
        SELECTION( "Cu~t",       ID_CUT,   SHIFT_DEL, INACTIVE)
        SELECTION( "~Copy",      ID_COPY,  CTRL_INS,  INACTIVE)
        SELECTION( "~Paste",     ID_PASTE, SHIFT_INS, INACTIVE)
        SEPARATOR
        SELECTION( "Cl~ear",     ID_CLEAR, 0,         INACTIVE)
        SELECTION( "~Delete",    ID_DELETETEXT, DEL,  INACTIVE)
        SEPARATOR
        SELECTION( "Pa~ragraph", ID_PARAGRAPH,  ALT_P,INACTIVE)
    ENDPOPDOWN

    /* --------------- the Search popdown menu ----------------*/
    POPDOWN( "~Search", PrepSearchMenu, "Search and replace" )
        SELECTION( "~Search...", ID_SEARCH,      0,    INACTIVE)
        SELECTION( "~Replace...",ID_REPLACE,     0,    INACTIVE)
        SELECTION( "~Next",      ID_SEARCHNEXT,  F3,   INACTIVE)
    ENDPOPDOWN

	/* ------------ the Utilities popdown menu --------------- */
	POPDOWN( "~Utilities", NULL, "Utility programs" )
		SELECTION( "~Calendar",   ID_CALENDAR,     0,   0)
//                SELECTION( "~Bar chart",  ID_BARCHART,     0,   0)
	ENDPOPDOWN

    /* ------------- the Options popdown menu ---------------*/
    POPDOWN( "~Options", NULL, "Editor and display options" )
        SELECTION( "~Display...",   ID_DISPLAY,     0,      0 )
        SEPARATOR
#ifdef INCLUDE_LOGGING
        SELECTION( "~Log messages", ID_LOG,     ALT_L,      0 )
        SEPARATOR
#endif
        SELECTION( "~Insert",       ID_INSERT,     INS, TOGGLE)
        SELECTION( "~Word wrap",    ID_WRAP,        0,  TOGGLE)
        SELECTION( "~Tabs ( )",     ID_TABS,        0,  CASCADED)
        SEPARATOR
        SELECTION( "~Save options", ID_SAVEOPTIONS, 0,      0 )
    ENDPOPDOWN

    /* --------------- the Window popdown menu --------------*/
    POPDOWN( "~Window", PrepWindowMenu, "Select/close document windows" )
        SELECTION(  NULL,  ID_CLOSEALL, 0, 0)
        SEPARATOR
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
        SELECTION(  "~More Windows...", ID_MOREWINDOWS, 0, 0)
        SELECTION(  NULL,  ID_WINDOW, 0, 0 )
    ENDPOPDOWN

    /* --------------- the Help popdown menu ----------------*/
    POPDOWN( "~Help", NULL, "Get help" )
        SELECTION(  "~Help for help...",  ID_HELPHELP,  0, 0 )
        SELECTION(  "~Extended help...",  ID_EXTHELP,   0, 0 )
        SELECTION(  "~Keys help...",      ID_KEYSHELP,  0, 0 )
        SELECTION(  "Help ~index...",     ID_HELPINDEX, 0, 0 )
        SEPARATOR
        SELECTION(  "~About...",          ID_ABOUT,     0, 0 )
#ifdef TESTING_DFLAT
        SEPARATOR
        SELECTION(  "~Reload help database",ID_LOADHELP,0, 0 )
#endif
    ENDPOPDOWN

	/* ----- cascaded pulldown from Tabs... above ----- */
	CASCADED_POPDOWN( ID_TABS, NULL )
		SELECTION( "~2 tab stops", ID_TAB2, 0, 0)
		SELECTION( "~4 tab stops", ID_TAB4, 0, 0)
		SELECTION( "~6 tab stops", ID_TAB6, 0, 0)
		SELECTION( "~8 tab stops", ID_TAB8, 0, 0)
    ENDPOPDOWN

ENDMENU

/* ------------- the System Menu --------------------- */
DEFMENU(SystemMenu)
    POPDOWN("System Menu", NULL, NULL)
#ifdef INCLUDE_RESTORE
        SELECTION("~Restore",  ID_SYSRESTORE,  0,         0 )
#endif
        SELECTION("~Move",     ID_SYSMOVE,     0,         0 )
        SELECTION("~Size",     ID_SYSSIZE,     0,         0 )
#ifdef INCLUDE_MINIMIZE
        SELECTION("Mi~nimize", ID_SYSMINIMIZE, 0,         0 )
#endif
#ifdef INCLUDE_MAXIMIZE
        SELECTION("Ma~ximize", ID_SYSMAXIMIZE, 0,         0 )
#endif
        SEPARATOR
        SELECTION("~Close",    ID_SYSCLOSE,    CTRL_F4,   0 )
    ENDPOPDOWN
ENDMENU

