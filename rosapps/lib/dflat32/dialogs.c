/* ----------- dialogs.c --------------- */

#include "dflat32/dflat.h"

/* -------------- the File Open dialog box --------------- */
DIALOGBOX( FileOpen )
    DB_TITLE(        "Open File",    -1,-1,19,48)
    CONTROL(TEXT,    "~Filename",     2, 1, 1, 8, ID_FILENAME)
    CONTROL(EDITBOX, NULL,           13, 1, 1,29, ID_FILENAME)
    CONTROL(TEXT,    "Directory:",    2, 3, 1,10, 0)
    CONTROL(TEXT,    NULL,           13, 3, 1,28, ID_PATH)
    CONTROL(TEXT,    "F~iles",        2, 5, 1, 5, ID_FILES)
    CONTROL(LISTBOX, NULL,            2, 6,11,16, ID_FILES)
    CONTROL(TEXT,    "~Directories", 19, 5, 1,11, ID_DRIVE)
    CONTROL(LISTBOX, NULL,           19, 6,11,16, ID_DRIVE)
    CONTROL(BUTTON,  "   ~OK   ",    36, 7, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",    36,10, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",    36,13, 1, 8, ID_HELP)
ENDDB

/* -------------- the Save As dialog box --------------- */
DIALOGBOX( SaveAs )
    DB_TITLE(        "Save As",    -1,-1,19,48)
    CONTROL(TEXT,    "~Filename",   2, 1, 1, 8, ID_FILENAME)
    CONTROL(EDITBOX, NULL,         13, 1, 1,29, ID_FILENAME)
    CONTROL(TEXT,    "Directory:",  2, 3, 1,10, 0)
    CONTROL(TEXT,    NULL,         13, 3, 1,28, ID_PATH)
    CONTROL(TEXT,    "~Directories",2, 5, 1,11, ID_DRIVE)
    CONTROL(LISTBOX, NULL,          2, 6,11,16, ID_DRIVE)
    CONTROL(BUTTON,  "   ~OK   ",  36, 7, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",  36,10, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",  36,13, 1, 8, ID_HELP)
ENDDB

/* -------------- The Printer Setup dialog box ------------------ */
DIALOGBOX( PrintSetup )
	DB_TITLE( "Printer Setup",   -1, -1, 17, 32)
	CONTROL(BOX,      "Margins",  2,  3,  9, 26, 0 )
	CONTROL(TEXT,     "~Port:",   4,  1,  1,  5, ID_PRINTERPORT)
	CONTROL(COMBOBOX, NULL,      12,  1,  8,  9, ID_PRINTERPORT)
	CONTROL(TEXT,     "~Left:",   6,  4,  1,  5, ID_LEFTMARGIN)
	CONTROL(SPINBUTTON, NULL,    17,  4,  1,  6, ID_LEFTMARGIN)
	CONTROL(TEXT,     "~Right:",  6,  6,  1,  6, ID_RIGHTMARGIN)
	CONTROL(SPINBUTTON, NULL,    17,  6,  1,  6, ID_RIGHTMARGIN)
	CONTROL(TEXT,     "~Top:",    6,  8,  1,  4, ID_TOPMARGIN)
	CONTROL(SPINBUTTON, NULL,    17,  8,  1,  6, ID_TOPMARGIN)
	CONTROL(TEXT,     "~Bottom:", 6, 10,  1,  7, ID_BOTTOMMARGIN)
	CONTROL(SPINBUTTON, NULL,    17, 10,  1,  6, ID_BOTTOMMARGIN)
    CONTROL(BUTTON, "   ~OK   ",  1, 13,  1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 11, 13,  1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ", 21, 13,  1,  8, ID_HELP)
ENDDB

/* -------------- the Search Text dialog box --------------- */
DIALOGBOX( SearchTextDB )
    DB_TITLE(        "Search Text",    -1,-1,9,48)
    CONTROL(TEXT,    "~Search for:",          2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   14, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT, "~Match upper/lower case:", 2, 3, 1, 23, ID_MATCHCASE)
	CONTROL(CHECKBOX,  NULL,                 26, 3, 1,  3, ID_MATCHCASE)
    CONTROL(BUTTON, "   ~OK   ",              7, 5, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             19, 5, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             31, 5, 1,  8, ID_HELP)
ENDDB

/* -------------- the Replace Text dialog box --------------- */
DIALOGBOX( ReplaceTextDB )
    DB_TITLE(        "Replace Text",    -1,-1,12,50)
    CONTROL(TEXT,    "~Search for:",          2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   16, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT,    "~Replace with:",        2, 3, 1, 13, ID_REPLACEWITH)
    CONTROL(EDITBOX, NULL,                   16, 3, 1, 29, ID_REPLACEWITH)
    CONTROL(TEXT, "~Match upper/lower case:", 2, 5, 1, 23, ID_MATCHCASE)
	CONTROL(CHECKBOX,  NULL,                 26, 5, 1,  3, ID_MATCHCASE)
    CONTROL(TEXT, "Replace ~Every Match:",    2, 6, 1, 23, ID_REPLACEALL)
	CONTROL(CHECKBOX,  NULL,                 26, 6, 1,  3, ID_REPLACEALL)
    CONTROL(BUTTON, "   ~OK   ",              7, 8, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             20, 8, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             33, 8, 1,  8, ID_HELP)
ENDDB

/* -------------- generic message dialog box --------------- */
DIALOGBOX( MsgBox )
    DB_TITLE(       NULL,  -1,-1, 0, 0)
    CONTROL(TEXT,   NULL,   1, 1, 0, 0, 0)
    CONTROL(BUTTON, NULL,   0, 0, 1, 8, ID_OK)
    CONTROL(0,      NULL,   0, 0, 1, 8, ID_CANCEL)
ENDDB

/* ----------- InputBox Dialog Box ------------ */
DIALOGBOX( InputBoxDB )
    DB_TITLE(        NULL,      -1,-1, 9, 0)
    CONTROL(TEXT,    NULL,       1, 1, 1, 0, 0)
	CONTROL(EDITBOX, NULL,       1, 3, 1, 0, ID_INPUTTEXT)
    CONTROL(BUTTON, "   ~OK   ", 0, 5, 1, 8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB

/* ----------- SliderBox Dialog Box ------------- */
DIALOGBOX( SliderBoxDB )
    DB_TITLE(       NULL,      -1,-1, 9, 0)
    CONTROL(TEXT,   NULL,       0, 1, 1, 0, 0)
    CONTROL(TEXT,   NULL,       0, 3, 1, 0, 0)
    CONTROL(BUTTON, " Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB


/* ------------ Display dialog box -------------- */
DIALOGBOX( Display )
    DB_TITLE(     "Display", -1, -1, 12, 35)

	CONTROL(BOX,      "Window",    7, 1, 6,20, 0)
    CONTROL(CHECKBOX,    NULL,     9, 2, 1, 3, ID_TITLE)
    CONTROL(TEXT,     "~Title",   15, 2, 1, 5, ID_TITLE)
    CONTROL(CHECKBOX,    NULL,     9, 3, 1, 3, ID_BORDER)
    CONTROL(TEXT,     "~Border",  15, 3, 1, 6, ID_BORDER)
    CONTROL(CHECKBOX,    NULL,     9, 4, 1, 3, ID_STATUSBAR)
    CONTROL(TEXT,   "~Status bar",15, 4, 1,10, ID_STATUSBAR)
    CONTROL(CHECKBOX,    NULL,     9, 5, 1, 3, ID_TEXTURE)
    CONTROL(TEXT,     "Te~xture", 15, 5, 1, 7, ID_TEXTURE)

    CONTROL(BUTTON, "   ~OK   ",   2, 8,1,8,ID_OK)
    CONTROL(BUTTON, " ~Cancel ",  12, 8,1,8,ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",  22, 8,1,8,ID_HELP)
ENDDB

/* ------------ Windows dialog box -------------- */
DIALOGBOX( Windows )
    DB_TITLE(     "Windows", -1, -1, 19, 24)
    CONTROL(LISTBOX, NULL,         1,  1,11,20, ID_WINDOWLIST)
    CONTROL(BUTTON,  "   ~OK   ",  2, 13, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ", 12, 13, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",  7, 15, 1, 8, ID_HELP)
ENDDB

#ifdef INCLUDE_LOGGING
/* ------------ Message Log dialog box -------------- */
DIALOGBOX( Log )
    DB_TITLE(    "D-Flat Message Log", -1, -1, 18, 41)
    CONTROL(TEXT,  "~Messages",   10,   1,  1,  8, ID_LOGLIST)
    CONTROL(LISTBOX,    NULL,     1,    2, 14, 26, ID_LOGLIST)
    CONTROL(TEXT,    "~Logging:", 29,   4,  1, 10, ID_LOGGING)
    CONTROL(CHECKBOX,    NULL,    31,   5,  1,  3, ID_LOGGING)
    CONTROL(BUTTON,  "   ~OK   ", 29,   7,  1,  8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ", 29,  10,  1,  8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ", 29,  13, 1,   8, ID_HELP)
ENDDB
#endif

/* ------------ the Help window dialog box -------------- */
DIALOGBOX( HelpBox )
    DB_TITLE(         NULL,       -1, -1, 0, 45)
    CONTROL(TEXTBOX, NULL,         1,  1, 0, 40, ID_HELPTEXT)
    CONTROL(BUTTON,  "  ~Close ",  0,  0, 1,  8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Back  ", 10,  0, 1,  8, ID_BACK)
    CONTROL(BUTTON,  "<< ~Prev ", 20,  0, 1,  8, ID_PREV)
    CONTROL(BUTTON,  " ~Next >>", 30,  0, 1,  8, ID_NEXT)
ENDDB

/* EOF */