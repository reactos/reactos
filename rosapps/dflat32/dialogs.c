/* ----------- dialogs.c --------------- */

#include "dflat.h"

/* -------------- the File Open dialog box --------------- */
DF_DIALOGBOX( FileOpen )
    DF_DB_TITLE(        "Open File",    -1,-1,19,48)
    DF_CONTROL(DF_TEXT,    "~Filename",     2, 1, 1, 8, DF_ID_FILENAME)
    DF_CONTROL(DF_EDITBOX, NULL,           13, 1, 1,29, DF_ID_FILENAME)
    DF_CONTROL(DF_TEXT,    "Directory:",    2, 3, 1,10, 0)
    DF_CONTROL(DF_TEXT,    NULL,           13, 3, 1,28, DF_ID_PATH)
    DF_CONTROL(DF_TEXT,    "F~iles",        2, 5, 1, 5, DF_ID_FILES)
    DF_CONTROL(DF_LISTBOX, NULL,            2, 6,11,16, DF_ID_FILES)
    DF_CONTROL(DF_TEXT,    "~Directories", 19, 5, 1,11, DF_ID_DRIVE)
    DF_CONTROL(DF_LISTBOX, NULL,           19, 6,11,16, DF_ID_DRIVE)
    DF_CONTROL(DF_BUTTON,  "   ~OK   ",    36, 7, 1, 8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON,  " ~Cancel ",    36,10, 1, 8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON,  "  ~Help  ",    36,13, 1, 8, DF_ID_HELP)
DF_ENDDB

/* -------------- the Save As dialog box --------------- */
DF_DIALOGBOX( SaveAs )
    DF_DB_TITLE(        "Save As",    -1,-1,19,48)
    DF_CONTROL(DF_TEXT,    "~Filename",   2, 1, 1, 8, DF_ID_FILENAME)
    DF_CONTROL(DF_EDITBOX, NULL,         13, 1, 1,29, DF_ID_FILENAME)
    DF_CONTROL(DF_TEXT,    "Directory:",  2, 3, 1,10, 0)
    DF_CONTROL(DF_TEXT,    NULL,         13, 3, 1,28, DF_ID_PATH)
    DF_CONTROL(DF_TEXT,    "~Directories",2, 5, 1,11, DF_ID_DRIVE)
    DF_CONTROL(DF_LISTBOX, NULL,          2, 6,11,16, DF_ID_DRIVE)
    DF_CONTROL(DF_BUTTON,  "   ~OK   ",  36, 7, 1, 8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON,  " ~Cancel ",  36,10, 1, 8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON,  "  ~Help  ",  36,13, 1, 8, DF_ID_HELP)
DF_ENDDB

/* -------------- The Printer Setup dialog box ------------------ */
DF_DIALOGBOX( PrintSetup )
	DF_DB_TITLE( "Printer Setup",   -1, -1, 17, 32)
	DF_CONTROL(DF_BOX,      "Margins",  2,  3,  9, 26, 0 )
	DF_CONTROL(DF_TEXT,     "~Port:",   4,  1,  1,  5, DF_ID_PRINTERPORT)
	DF_CONTROL(DF_COMBOBOX, NULL,      12,  1,  8,  9, DF_ID_PRINTERPORT)
	DF_CONTROL(DF_TEXT,     "~Left:",   6,  4,  1,  5, DF_ID_LEFTMARGIN)
	DF_CONTROL(DF_SPINBUTTON, NULL,    17,  4,  1,  6, DF_ID_LEFTMARGIN)
	DF_CONTROL(DF_TEXT,     "~Right:",  6,  6,  1,  6, DF_ID_RIGHTMARGIN)
	DF_CONTROL(DF_SPINBUTTON, NULL,    17,  6,  1,  6, DF_ID_RIGHTMARGIN)
	DF_CONTROL(DF_TEXT,     "~Top:",    6,  8,  1,  4, DF_ID_TOPMARGIN)
	DF_CONTROL(DF_SPINBUTTON, NULL,    17,  8,  1,  6, DF_ID_TOPMARGIN)
	DF_CONTROL(DF_TEXT,     "~Bottom:", 6, 10,  1,  7, DF_ID_BOTTOMMARGIN)
	DF_CONTROL(DF_SPINBUTTON, NULL,    17, 10,  1,  6, DF_ID_BOTTOMMARGIN)
    DF_CONTROL(DF_BUTTON, "   ~OK   ",  1, 13,  1,  8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON, " ~Cancel ", 11, 13,  1,  8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON, "  ~Help  ", 21, 13,  1,  8, DF_ID_HELP)
DF_ENDDB

/* -------------- the Search Text dialog box --------------- */
DF_DIALOGBOX( SearchTextDB )
    DF_DB_TITLE(        "Search Text",    -1,-1,9,48)
    DF_CONTROL(DF_TEXT,    "~Search for:",          2, 1, 1, 11, DF_ID_SEARCHFOR)
    DF_CONTROL(DF_EDITBOX, NULL,                   14, 1, 1, 29, DF_ID_SEARCHFOR)
    DF_CONTROL(DF_TEXT, "~Match upper/lower case:", 2, 3, 1, 23, DF_ID_MATCHCASE)
	DF_CONTROL(DF_CHECKBOX,  NULL,                 26, 3, 1,  3, DF_ID_MATCHCASE)
    DF_CONTROL(DF_BUTTON, "   ~OK   ",              7, 5, 1,  8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON, " ~Cancel ",             19, 5, 1,  8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON, "  ~Help  ",             31, 5, 1,  8, DF_ID_HELP)
DF_ENDDB

/* -------------- the Replace Text dialog box --------------- */
DF_DIALOGBOX( ReplaceTextDB )
    DF_DB_TITLE(        "Replace Text",    -1,-1,12,50)
    DF_CONTROL(DF_TEXT,    "~Search for:",          2, 1, 1, 11, DF_ID_SEARCHFOR)
    DF_CONTROL(DF_EDITBOX, NULL,                   16, 1, 1, 29, DF_ID_SEARCHFOR)
    DF_CONTROL(DF_TEXT,    "~Replace with:",        2, 3, 1, 13, DF_ID_REPLACEWITH)
    DF_CONTROL(DF_EDITBOX, NULL,                   16, 3, 1, 29, DF_ID_REPLACEWITH)
    DF_CONTROL(DF_TEXT, "~Match upper/lower case:", 2, 5, 1, 23, DF_ID_MATCHCASE)
	DF_CONTROL(DF_CHECKBOX,  NULL,                 26, 5, 1,  3, DF_ID_MATCHCASE)
    DF_CONTROL(DF_TEXT, "Replace ~Every Match:",    2, 6, 1, 23, DF_ID_REPLACEALL)
	DF_CONTROL(DF_CHECKBOX,  NULL,                 26, 6, 1,  3, DF_ID_REPLACEALL)
    DF_CONTROL(DF_BUTTON, "   ~OK   ",              7, 8, 1,  8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON, " ~Cancel ",             20, 8, 1,  8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON, "  ~Help  ",             33, 8, 1,  8, DF_ID_HELP)
DF_ENDDB

/* -------------- generic message dialog box --------------- */
DF_DIALOGBOX( MsgBox )
    DF_DB_TITLE(       NULL,  -1,-1, 0, 0)
    DF_CONTROL(DF_TEXT,   NULL,   1, 1, 0, 0, 0)
    DF_CONTROL(DF_BUTTON, NULL,   0, 0, 1, 8, DF_ID_OK)
    DF_CONTROL(0,      NULL,   0, 0, 1, 8, DF_ID_CANCEL)
DF_ENDDB

/* ----------- DfInputBox Dialog Box ------------ */
DF_DIALOGBOX( InputBoxDB )
    DF_DB_TITLE(        NULL,      -1,-1, 9, 0)
    DF_CONTROL(DF_TEXT,    NULL,       1, 1, 1, 0, 0)
	DF_CONTROL(DF_EDITBOX, NULL,       1, 3, 1, 0, DF_ID_INPUTTEXT)
    DF_CONTROL(DF_BUTTON, "   ~OK   ", 0, 5, 1, 8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON, " ~Cancel ", 0, 5, 1, 8, DF_ID_CANCEL)
DF_ENDDB

/* ----------- DfSliderBox Dialog Box ------------- */
DF_DIALOGBOX( SliderBoxDB )
    DF_DB_TITLE(       NULL,      -1,-1, 9, 0)
    DF_CONTROL(DF_TEXT,   NULL,       0, 1, 1, 0, 0)
    DF_CONTROL(DF_TEXT,   NULL,       0, 3, 1, 0, 0)
    DF_CONTROL(DF_BUTTON, " Cancel ", 0, 5, 1, 8, DF_ID_CANCEL)
DF_ENDDB


/* ------------ Display dialog box -------------- */
DF_DIALOGBOX( Display )
    DF_DB_TITLE(     "Display", -1, -1, 12, 35)

	DF_CONTROL(DF_BOX,      "Window",    7, 1, 6,20, 0)
    DF_CONTROL(DF_CHECKBOX,    NULL,     9, 2, 1, 3, DF_ID_TITLE)
    DF_CONTROL(DF_TEXT,     "~Title",   15, 2, 1, 5, DF_ID_TITLE)
    DF_CONTROL(DF_CHECKBOX,    NULL,     9, 3, 1, 3, DF_ID_BORDER)
    DF_CONTROL(DF_TEXT,     "~Border",  15, 3, 1, 6, DF_ID_BORDER)
    DF_CONTROL(DF_CHECKBOX,    NULL,     9, 4, 1, 3, DF_ID_STATUSBAR)
    DF_CONTROL(DF_TEXT,   "~Status bar",15, 4, 1,10, DF_ID_STATUSBAR)
    DF_CONTROL(DF_CHECKBOX,    NULL,     9, 5, 1, 3, DF_ID_TEXTURE)
    DF_CONTROL(DF_TEXT,     "Te~xture", 15, 5, 1, 7, DF_ID_TEXTURE)

    DF_CONTROL(DF_BUTTON, "   ~OK   ",   2, 8,1,8,DF_ID_OK)
    DF_CONTROL(DF_BUTTON, " ~Cancel ",  12, 8,1,8,DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON, "  ~Help  ",  22, 8,1,8,DF_ID_HELP)
DF_ENDDB

/* ------------ Windows dialog box -------------- */
DF_DIALOGBOX( Windows )
    DF_DB_TITLE(     "Windows", -1, -1, 19, 24)
    DF_CONTROL(DF_LISTBOX, NULL,         1,  1,11,20, DF_ID_WINDOWLIST)
    DF_CONTROL(DF_BUTTON,  "   ~OK   ",  2, 13, 1, 8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON,  " ~Cancel ", 12, 13, 1, 8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON,  "  ~Help  ",  7, 15, 1, 8, DF_ID_HELP)
DF_ENDDB

#ifdef INCLUDE_LOGGING
/* ------------ Message Log dialog box -------------- */
DF_DIALOGBOX( Log )
    DF_DB_TITLE(    "D-Flat Message Log", -1, -1, 18, 41)
    DF_CONTROL(DF_TEXT,  "~Messages",   10,   1,  1,  8, DF_ID_LOGLIST)
    DF_CONTROL(DF_LISTBOX,    NULL,     1,    2, 14, 26, DF_ID_LOGLIST)
    DF_CONTROL(DF_TEXT,    "~Logging:", 29,   4,  1, 10, DF_ID_LOGGING)
    DF_CONTROL(DF_CHECKBOX,    NULL,    31,   5,  1,  3, DF_ID_LOGGING)
    DF_CONTROL(DF_BUTTON,  "   ~OK   ", 29,   7,  1,  8, DF_ID_OK)
    DF_CONTROL(DF_BUTTON,  " ~Cancel ", 29,  10,  1,  8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON,  "  ~Help  ", 29,  13, 1,   8, DF_ID_HELP)
DF_ENDDB
#endif

/* ------------ the Help window dialog box -------------- */
DF_DIALOGBOX( HelpBox )
    DF_DB_TITLE(         NULL,       -1, -1, 0, 45)
    DF_CONTROL(DF_TEXTBOX, NULL,         1,  1, 0, 40, DF_ID_HELPTEXT)
    DF_CONTROL(DF_BUTTON,  "  ~Close ",  0,  0, 1,  8, DF_ID_CANCEL)
    DF_CONTROL(DF_BUTTON,  "  ~Back  ", 10,  0, 1,  8, DF_ID_BACK)
    DF_CONTROL(DF_BUTTON,  "<< ~Prev ", 20,  0, 1,  8, DF_ID_PREV)
    DF_CONTROL(DF_BUTTON,  " ~Next >>", 30,  0, 1,  8, DF_ID_NEXT)
DF_ENDDB

/* EOF */
