/*  Edit's Dialogs

    Part of FreeDOS Edit

*/

#include "dflat.h"

/* --- The File/Open Dialog Box */
DIALOGBOX(FileOpen)
    DB_TITLE("Open File", -1,-1,19,57)
    CONTROL(TEXT,    "File ~Name:",   3, 1, 1,10, ID_FILENAME)
    CONTROL(EDITBOX, NULL,           14, 1, 1,40, ID_FILENAME)
    CONTROL(TEXT,    NULL,            3, 3, 1,50, ID_PATH) 
    CONTROL(TEXT,    "~Files:",       3, 5, 1, 6, ID_FILES)
    CONTROL(LISTBOX, NULL,            3, 6,10,14, ID_FILES)
    CONTROL(TEXT,    "~Directories:",19, 5, 1,12, ID_DIRECTORY)
    CONTROL(LISTBOX, NULL,           19, 6,10,13, ID_DIRECTORY)
    CONTROL(TEXT,    "Dri~ves:",     34, 5, 1, 7, ID_DRIVE)
    CONTROL(LISTBOX, NULL,           34, 6,10,10, ID_DRIVE)
    CONTROL(BUTTON,  "   ~OK   ",    46, 5, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",    46, 8, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",    46,11, 1, 8, ID_HELP)
    CONTROL(TEXT, "~Read",           50,13, 1, 5, ID_READONLY)
    CONTROL(TEXT, "Only",            50,14, 1, 5, ID_READONLY)
    CONTROL(CHECKBOX,  NULL,         46,13, 1, 3, ID_READONLY)
ENDDB

/* --- The Save As Dialog Box --- */
DIALOGBOX(SaveAs)
    DB_TITLE("Save As", -1,-1,19,57)
    CONTROL(TEXT,    "File ~Name:",   3, 1, 1, 9, ID_FILENAME)
    CONTROL(EDITBOX, NULL,           13, 1, 1,40, ID_FILENAME)
    CONTROL(TEXT,    NULL,            3, 3, 1,50, ID_PATH) 
    CONTROL(TEXT,    "~Files:",       3, 5, 1, 6, ID_FILES)
    CONTROL(LISTBOX, NULL,            3, 6,10,14, ID_FILES)
    CONTROL(TEXT,    "~Directories:",19, 5, 1,12, ID_DIRECTORY)
    CONTROL(LISTBOX, NULL,           19, 6,10,13, ID_DIRECTORY)
    CONTROL(TEXT,    "Dri~ves:",     34, 5, 1, 7, ID_DRIVE)
    CONTROL(LISTBOX, NULL,           34, 6,10,10, ID_DRIVE)
    CONTROL(BUTTON,  "   ~OK   ",    46, 7, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",    46,10, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",    46,13, 1, 8, ID_HELP)
ENDDB

/* --- The Printer Setup Dialog Box --- */
DIALOGBOX(PrintSetup)
    DB_TITLE("Print Setup", -1, -1, 17, 32)
    CONTROL(BOX,      "Margins",  2,  3,  9, 26, 0)
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

/* --- The Search Text Dialog Box --- */
DIALOGBOX(SearchTextDB)
    DB_TITLE("Find",-1,-1,9,48)
    CONTROL(TEXT,    "~Find What:",           2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   14, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT, "Match ~Case:"  ,           2, 3, 1, 23, ID_MATCHCASE)
    CONTROL(CHECKBOX,  NULL,                 26, 3, 1,  3, ID_MATCHCASE)
    CONTROL(BUTTON, "   ~OK   ",              7, 5, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             19, 5, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             31, 5, 1,  8, ID_HELP)
ENDDB

/* --- The Replace Text dialog box --- */
DIALOGBOX(ReplaceTextDB)
    DB_TITLE("Replace",-1,-1,12,50)
    CONTROL(TEXT,    "~Search for:",          2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   16, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT,    "~Replace with:",        2, 3, 1, 13, ID_REPLACEWITH)
    CONTROL(EDITBOX, NULL,                   16, 3, 1, 29, ID_REPLACEWITH)
    CONTROL(TEXT, "Match ~Case:",             2, 5, 1, 23, ID_MATCHCASE)
    CONTROL(CHECKBOX,  NULL,                 26, 5, 1,  3, ID_MATCHCASE)
    CONTROL(TEXT, "Replace ~All:",            2, 6, 1, 23, ID_REPLACEALL)
    CONTROL(CHECKBOX,  NULL,                 26, 6, 1,  3, ID_REPLACEALL)
    CONTROL(BUTTON, "   ~OK   ",              7, 8, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             20, 8, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             33, 8, 1,  8, ID_HELP)
ENDDB

/* --- Generic message dialog box --- */
DIALOGBOX(MsgBox)
    DB_TITLE(NULL,-1,-1, 0, 0)
    CONTROL(TEXT,   NULL,   1, 1, 0, 0, 0)
    CONTROL(BUTTON, NULL,   0, 0, 1, 8, ID_OK)
    CONTROL(0,      NULL,   0, 0, 1, 8, ID_CANCEL)
    CONTROL(0,      NULL,   0, 0, 1, 8, ID_THREE)
ENDDB

/* ----------- InputBox Dialog Box ------------ */
DIALOGBOX(InputBoxDB)
    DB_TITLE(NULL,-1,-1, 9, 0)
    CONTROL(TEXT,    NULL,       1, 1, 1, 0, 0)
    CONTROL(EDITBOX, NULL,       1, 3, 1, 0, ID_INPUTTEXT)
    CONTROL(BUTTON, "   ~OK   ", 0, 5, 1, 8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB

/* ----------- SliderBox Dialog Box ------------- */
DIALOGBOX(SliderBoxDB)
    DB_TITLE(NULL,-1,-1, 9, 0)
    CONTROL(TEXT,   NULL,       0, 1, 1, 0, 0)
    CONTROL(TEXT,   NULL,       0, 3, 1, 0, 0)
    CONTROL(BUTTON, " Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB

#ifdef INCLUDE_WINDOWOPTIONS
#define offset 7
#else
#define offset 0
#endif

/* ------------ Options dialog box -------------- */
DIALOGBOX(Display)
    DB_TITLE("Display", -1, -1, 12+offset, 35)
#ifdef INCLUDE_WINDOWOPTIONS
    CONTROL(BOX,      "Window",    7, 1, 6,20, 0)
    CONTROL(CHECKBOX,    NULL,     9, 2, 1, 3, ID_TITLE)
    CONTROL(TEXT,     "~Title",   15, 2, 1, 5, ID_TITLE)
    CONTROL(CHECKBOX,    NULL,     9, 3, 1, 3, ID_BORDER)
    CONTROL(TEXT,     "~Border",  15, 3, 1, 6, ID_BORDER)
    CONTROL(CHECKBOX,    NULL,     9, 4, 1, 3, ID_STATUSBAR)
    CONTROL(TEXT,   "~Status bar",15, 4, 1,10, ID_STATUSBAR)
    CONTROL(CHECKBOX,    NULL,     9, 5, 1, 3, ID_TEXTURE)
    CONTROL(TEXT,     "Te~xture", 15, 5, 1, 7, ID_TEXTURE)
#endif
    CONTROL(BOX,      "Colors",    1, 1+offset,5,15, 0)
    CONTROL(RADIOBUTTON, NULL,     3, 2+offset,1,3,ID_COLOR)
    CONTROL(TEXT,     "Co~lor",    7, 2+offset,1,5,ID_COLOR)
    CONTROL(RADIOBUTTON, NULL,     3, 3+offset,1,3,ID_MONO)
    CONTROL(TEXT,     "~Mono",     7, 3+offset,1,4,ID_MONO)
    CONTROL(RADIOBUTTON, NULL,     3, 4+offset,1,3,ID_REVERSE)
    CONTROL(TEXT,     "~Reverse",  7, 4+offset,1,7,ID_REVERSE)

    CONTROL(BOX,      "Lines",    17, 1+offset,5,15, 0)
    CONTROL(RADIOBUTTON, NULL,    19, 2+offset,1,3,ID_25LINES)
    CONTROL(TEXT,     "~25",      23, 2+offset,1,2,ID_25LINES)
    CONTROL(RADIOBUTTON, NULL,    19, 3+offset,1,3,ID_43LINES)
    CONTROL(TEXT,     "~43",      23, 3+offset,1,2,ID_43LINES)
    CONTROL(RADIOBUTTON, NULL,    19, 4+offset,1,3,ID_50LINES)
    CONTROL(TEXT,     "~50",      23, 4+offset,1,2,ID_50LINES)
/*
    CONTROL(CHECKBOX,    NULL,    11, 6+offset,1,3,ID_SNOWY)
    CONTROL(TEXT,     "S~nowy",   15, 6+offset,1,7,ID_SNOWY)
*/
    CONTROL(CHECKBOX, NULL,                       1, 6+offset, 1, 3,ID_LOADBLANK)
    CONTROL(TEXT,     "Open ~new window on load", 5, 6+offset, 1,23,ID_LOADBLANK)

    CONTROL(BUTTON, "   ~OK   ",   2, 8+offset,1,8,ID_OK)
    CONTROL(BUTTON, " ~Cancel ",  12, 8+offset,1,8,ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",  22, 8+offset,1,8,ID_HELP)
ENDDB

/* ------------ Windows dialog box -------------- */
DIALOGBOX(Windows)
    DB_TITLE("Windows", -1, -1, 19, 24)
    CONTROL(LISTBOX, NULL,         1,  1,11,20, ID_WINDOWLIST)
    CONTROL(BUTTON,  "   ~OK   ",  2, 13, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ", 12, 13, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",  7, 15, 1, 8, ID_HELP)
ENDDB

#ifdef INCLUDE_LOGGING
/* ------------ Message Log dialog box -------------- */
DIALOGBOX(Log)
    DB_TITLE("Edit Message Log", -1, -1, 18, 41)
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
DIALOGBOX(HelpBox)
    DB_TITLE(NULL, -1, -1, 0, 45)
    CONTROL(TEXTBOX, NULL,         1,  1, 0, 40, ID_HELPTEXT)
    CONTROL(BUTTON,  "  ~Close ",  0,  0, 1,  8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Back  ", 10,  0, 1,  8, ID_BACK)
    CONTROL(BUTTON,  "<< ~Prev ", 20,  0, 1,  8, ID_PREV)
    CONTROL(BUTTON,  " ~Next >>", 30,  0, 1,  8, ID_NEXT)
ENDDB

