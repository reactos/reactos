/* -------------- menus.c ------------- */

#include "dflat.h"

/* --------------------- the main menu --------------------- */
DF_DEFMENU(DfMainMenu)
    /* --------------- the File popdown menu ----------------*/
    DF_POPDOWN( "~File",  DfPrepFileMenu, "Read/write/print files. Go to DOS" )
        DF_SELECTION( "~New",        DF_ID_NEW,          0, 0 )
        DF_SELECTION( "~Open...",    DF_ID_OPEN,         0, 0 )
        DF_SEPARATOR
        DF_SELECTION( "~Save",       DF_ID_SAVE,     DF_ALT_S, DF_INACTIVE)
        DF_SELECTION( "Save ~as...", DF_ID_SAVEAS,       0, DF_INACTIVE)
        DF_SELECTION( "D~elete",     DF_ID_DELETEFILE,   0, DF_INACTIVE)
        DF_SEPARATOR
        DF_SELECTION( "~Print",      DF_ID_PRINT,        0, DF_INACTIVE)
        DF_SELECTION( "P~rinter setup...", DF_ID_PRINTSETUP, 0, 0   )
        DF_SEPARATOR
        DF_SELECTION( "~DOS",        DF_ID_DOS,          0, 0 )
        DF_SELECTION( "E~xit",       DF_ID_EXIT,     DF_ALT_X, 0 )
    DF_ENDPOPDOWN

    /* --------------- the Edit popdown menu ----------------*/
    DF_POPDOWN( "~Edit", DfPrepEditMenu, "DfClipboard, delete text, paragraph" )
        DF_SELECTION( "~Undo",      DF_ID_UNDO,  DF_ALT_BS,    DF_INACTIVE)
        DF_SEPARATOR
        DF_SELECTION( "Cu~t",       DF_ID_CUT,   DF_SHIFT_DEL, DF_INACTIVE)
        DF_SELECTION( "~Copy",      DF_ID_COPY,  DF_CTRL_INS,  DF_INACTIVE)
        DF_SELECTION( "~Paste",     DF_ID_PASTE, DF_SHIFT_INS, DF_INACTIVE)
        DF_SEPARATOR
        DF_SELECTION( "Cl~ear",     DF_ID_CLEAR, 0,         DF_INACTIVE)
        DF_SELECTION( "~Delete",    DF_ID_DELETETEXT, DF_DEL,  DF_INACTIVE)
        DF_SEPARATOR
        DF_SELECTION( "Pa~ragraph", DF_ID_PARAGRAPH,  DF_ALT_P,DF_INACTIVE)
    DF_ENDPOPDOWN

    /* --------------- the Search popdown menu ----------------*/
    DF_POPDOWN( "~Search", DfPrepSearchMenu, "Search and replace" )
        DF_SELECTION( "~Search...", DF_ID_SEARCH,      0,    DF_INACTIVE)
        DF_SELECTION( "~Replace...",DF_ID_REPLACE,     0,    DF_INACTIVE)
        DF_SELECTION( "~Next",      DF_ID_SEARCHNEXT,  DF_F3,   DF_INACTIVE)
    DF_ENDPOPDOWN

	/* ------------ the Utilities popdown menu --------------- */
	DF_POPDOWN( "~Utilities", NULL, "Utility programs" )
		DF_SELECTION( "~Calendar",   DF_ID_CALENDAR,     0,   0)
//                DF_SELECTION( "~Bar chart",  DF_ID_BARCHART,     0,   0)
	DF_ENDPOPDOWN

    /* ------------- the Options popdown menu ---------------*/
    DF_POPDOWN( "~Options", NULL, "Editor and display options" )
        DF_SELECTION( "~Display...",   DF_ID_DISPLAY,     0,      0 )
        DF_SEPARATOR
#ifdef INCLUDE_LOGGING
        DF_SELECTION( "~Log messages", DF_ID_LOG,     DF_ALT_L,      0 )
        DF_SEPARATOR
#endif
        DF_SELECTION( "~Insert",       DF_ID_INSERT,     DF_INS, DF_TOGGLE)
        DF_SELECTION( "~Word wrap",    DF_ID_WRAP,        0,  DF_TOGGLE)
        DF_SELECTION( "~Tabs ( )",     DF_ID_TABS,        0,  DF_CASCADED)
        DF_SEPARATOR
        DF_SELECTION( "~Save options", DF_ID_SAVEOPTIONS, 0,      0 )
    DF_ENDPOPDOWN

    /* --------------- the Window popdown menu --------------*/
    DF_POPDOWN( "~Window", DfPrepWindowMenu, "Select/close document windows" )
        DF_SELECTION(  NULL,  DF_ID_CLOSEALL, 0, 0)
        DF_SEPARATOR
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
        DF_SELECTION(  "~More Windows...", DF_ID_MOREWINDOWS, 0, 0)
        DF_SELECTION(  NULL,  DF_ID_WINDOW, 0, 0 )
    DF_ENDPOPDOWN

    /* --------------- the Help popdown menu ----------------*/
    DF_POPDOWN( "~Help", NULL, "Get help" )
        DF_SELECTION(  "~Help for help...",  DF_ID_HELPHELP,  0, 0 )
        DF_SELECTION(  "~Extended help...",  DF_ID_EXTHELP,   0, 0 )
        DF_SELECTION(  "~Keys help...",      DF_ID_KEYSHELP,  0, 0 )
        DF_SELECTION(  "Help ~index...",     DF_ID_HELPINDEX, 0, 0 )
        DF_SEPARATOR
        DF_SELECTION(  "~About...",          DF_ID_ABOUT,     0, 0 )
#ifdef TESTING_DFLAT
        DF_SEPARATOR
        DF_SELECTION(  "~Reload help database",DF_ID_LOADHELP,0, 0 )
#endif
    DF_ENDPOPDOWN

	/* ----- cascaded pulldown from Tabs... above ----- */
	DF_CASCADED_POPDOWN( DF_ID_TABS, NULL )
		DF_SELECTION( "~2 tab stops", DF_ID_TAB2, 0, 0)
		DF_SELECTION( "~4 tab stops", DF_ID_TAB4, 0, 0)
		DF_SELECTION( "~6 tab stops", DF_ID_TAB6, 0, 0)
		DF_SELECTION( "~8 tab stops", DF_ID_TAB8, 0, 0)
    DF_ENDPOPDOWN

DF_ENDMENU

/* ------------- the System Menu --------------------- */
DF_DEFMENU(DfSystemMenu)
    DF_POPDOWN("System Menu", NULL, NULL)
#ifdef INCLUDE_RESTORE
        DF_SELECTION("~Restore",  DF_ID_SYSRESTORE,  0,         0 )
#endif
        DF_SELECTION("~Move",     DF_ID_SYSMOVE,     0,         0 )
        DF_SELECTION("~Size",     DF_ID_SYSSIZE,     0,         0 )
#ifdef INCLUDE_MINIMIZE
        DF_SELECTION("Mi~nimize", DF_ID_SYSMINIMIZE, 0,         0 )
#endif
#ifdef INCLUDE_MAXIMIZE
        DF_SELECTION("Ma~ximize", DF_ID_SYSMAXIMIZE, 0,         0 )
#endif
        DF_SEPARATOR
        DF_SELECTION("~Close",    DF_ID_SYSCLOSE,    DF_CTRL_F4,   0 )
    DF_ENDPOPDOWN
DF_ENDMENU

