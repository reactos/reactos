/* ---------------- commands.h ----------------- */

/*
 * Command values sent as the first parameter
 * in the COMMAND message
 *
 * Add application-specific commands to this enum
 */

#ifndef COMMANDS_H
#define COMMANDS_H

enum DfCommands {
    /* --------------- File menu ---------------- */
    DF_ID_OPEN,
    DF_ID_NEW,
    DF_ID_SAVE,
    DF_ID_SAVEAS,
    DF_ID_DELETEFILE,
    DF_ID_PRINT,
    DF_ID_PRINTSETUP,
    DF_ID_DOS,
    DF_ID_EXIT,
    /* --------------- Edit menu ---------------- */
    DF_ID_UNDO,
    DF_ID_CUT,
    DF_ID_COPY,
    DF_ID_PASTE,
    DF_ID_PARAGRAPH,
    DF_ID_CLEAR,
    DF_ID_DELETETEXT,
    /* --------------- Search Menu -------------- */
    DF_ID_SEARCH,
    DF_ID_REPLACE,
    DF_ID_SEARCHNEXT,
	/* --------------- Utilities Menu ------------- */
	DF_ID_CALENDAR,
	DF_ID_BARCHART,
    /* -------------- Options menu -------------- */
    DF_ID_INSERT,
    DF_ID_WRAP,
    DF_ID_LOG,
    DF_ID_TABS,
    DF_ID_DISPLAY,
    DF_ID_SAVEOPTIONS,
    /* --------------- Window menu -------------- */
    DF_ID_CLOSEALL,
    DF_ID_WINDOW,
	DF_ID_MOREWINDOWS,
    /* --------------- Help menu ---------------- */
    DF_ID_HELPHELP,
    DF_ID_EXTHELP,
    DF_ID_KEYSHELP,
    DF_ID_HELPINDEX,
    DF_ID_ABOUT,
    DF_ID_LOADHELP,
    /* --------------- System menu -------------- */
#ifdef INCLUDE_RESTORE
    DF_ID_SYSRESTORE,
#endif
    DF_ID_SYSMOVE,
    DF_ID_SYSSIZE,
#ifdef INCLUDE_MINIMIZE
    DF_ID_SYSMINIMIZE,
#endif
#ifdef INCLUDE_MAXIMIZE
    DF_ID_SYSMAXIMIZE,
#endif
    DF_ID_SYSCLOSE,
    /* ---- FileOpen and SaveAs dialog boxes ---- */
    DF_ID_FILENAME,
    DF_ID_FILES,
    DF_ID_DRIVE,
    DF_ID_PATH,
    /* ----- Search and Replace dialog boxes ---- */
    DF_ID_SEARCHFOR,
    DF_ID_REPLACEWITH,
    DF_ID_MATCHCASE,
    DF_ID_REPLACEALL,
    /* ----------- Windows dialog box ----------- */
    DF_ID_WINDOWLIST,
    /* --------- generic command buttons -------- */
    DF_ID_OK,
    DF_ID_CANCEL,
    DF_ID_HELP,
    /* -------------- TabStops menu ------------- */
    DF_ID_TAB2,
    DF_ID_TAB4,
    DF_ID_TAB6,
    DF_ID_TAB8,
    /* ------------ Display dialog box ---------- */
    DF_ID_BORDER,
    DF_ID_TITLE,
    DF_ID_STATUSBAR,
    DF_ID_TEXTURE,
	DF_ID_SNOWY,
    DF_ID_COLOR,
    DF_ID_MONO,
    DF_ID_REVERSE,
    DF_ID_25LINES,
    DF_ID_43LINES,
    DF_ID_50LINES,
    /* ------------- Log dialog box ------------- */
    DF_ID_LOGLIST,
    DF_ID_LOGGING,
    /* ------------ HelpBox dialog box ---------- */
    DF_ID_HELPTEXT,
    DF_ID_BACK,
    DF_ID_PREV,
    DF_ID_NEXT,
	/* ---------- Print Select dialog box --------- */
	DF_ID_PRINTERPORT,
	DF_ID_LEFTMARGIN,
	DF_ID_RIGHTMARGIN,
	DF_ID_TOPMARGIN,
	DF_ID_BOTTOMMARGIN,
	/* ----------- DfInputBox dialog box ------------ */
	DF_ID_INPUTTEXT
};

#endif
