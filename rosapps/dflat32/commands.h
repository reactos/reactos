/* ---------------- commands.h ----------------- */

/*
 * Command values sent as the first parameter
 * in the COMMAND message
 *
 * Add application-specific commands to this enum
 */

#ifndef COMMANDS_H
#define COMMANDS_H

enum commands {
    /* --------------- File menu ---------------- */
    ID_OPEN,
    ID_NEW,
    ID_SAVE,
    ID_SAVEAS,
    ID_DELETEFILE,
    ID_PRINT,
    ID_PRINTSETUP,
    ID_DOS,
    ID_EXIT,
    /* --------------- Edit menu ---------------- */
    ID_UNDO,
    ID_CUT,
    ID_COPY,
    ID_PASTE,
    ID_PARAGRAPH,
    ID_CLEAR,
    ID_DELETETEXT,
    /* --------------- Search Menu -------------- */
    ID_SEARCH,
    ID_REPLACE,
    ID_SEARCHNEXT,
	/* --------------- Utilities Menu ------------- */
	ID_CALENDAR,
	ID_BARCHART,
    /* -------------- Options menu -------------- */
    ID_INSERT,
    ID_WRAP,
    ID_LOG,
    ID_TABS,
    ID_DISPLAY,
    ID_SAVEOPTIONS,
    /* --------------- Window menu -------------- */
    ID_CLOSEALL,
    ID_WINDOW,
	ID_MOREWINDOWS,
    /* --------------- Help menu ---------------- */
    ID_HELPHELP,
    ID_EXTHELP,
    ID_KEYSHELP,
    ID_HELPINDEX,
    ID_ABOUT,
    ID_LOADHELP,
    /* --------------- System menu -------------- */
#ifdef INCLUDE_RESTORE
    ID_SYSRESTORE,
#endif
    ID_SYSMOVE,
    ID_SYSSIZE,
#ifdef INCLUDE_MINIMIZE
    ID_SYSMINIMIZE,
#endif
#ifdef INCLUDE_MAXIMIZE
    ID_SYSMAXIMIZE,
#endif
    ID_SYSCLOSE,
    /* ---- FileOpen and SaveAs dialog boxes ---- */
    ID_FILENAME,
    ID_FILES,
    ID_DRIVE,
    ID_PATH,
    /* ----- Search and Replace dialog boxes ---- */
    ID_SEARCHFOR,
    ID_REPLACEWITH,
    ID_MATCHCASE,
    ID_REPLACEALL,
    /* ----------- Windows dialog box ----------- */
    ID_WINDOWLIST,
    /* --------- generic command buttons -------- */
    ID_OK,
    ID_CANCEL,
    ID_HELP,
    /* -------------- TabStops menu ------------- */
    ID_TAB2,
    ID_TAB4,
    ID_TAB6,
    ID_TAB8,
    /* ------------ Display dialog box ---------- */
    ID_BORDER,
    ID_TITLE,
    ID_STATUSBAR,
    ID_TEXTURE,
	ID_SNOWY,
    ID_COLOR,
    ID_MONO,
    ID_REVERSE,
    ID_25LINES,
    ID_43LINES,
    ID_50LINES,
    /* ------------- Log dialog box ------------- */
    ID_LOGLIST,
    ID_LOGGING,
    /* ------------ HelpBox dialog box ---------- */
    ID_HELPTEXT,
    ID_BACK,
    ID_PREV,
    ID_NEXT,
	/* ---------- Print Select dialog box --------- */
	ID_PRINTERPORT,
	ID_LEFTMARGIN,
	ID_RIGHTMARGIN,
	ID_TOPMARGIN,
	ID_BOTTOMMARGIN,
	/* ----------- InputBox dialog box ------------ */
	ID_INPUTTEXT
};

#endif
