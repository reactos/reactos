/*
 *  CONFIG.H - Used to configure what will be compiled into the shell.
 *
 *
 *  History:
 *
 *    27 Jul 1998 - John P. Price
 *        started.
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN

//#define NT4_INTERNAL_COMMANDS

/* Define to enable the alias command, and aliases.*/
#define FEATURE_ALIASES

/* Define to enable history */
#define FEATURE_HISTORY

/*Define to enable history wrap (4nt's style)*/
#define WRAP_HISTORY

/* Define one of these to enable filename completion */
//#define FEATURE_UNIX_FILENAME_COMPLETION
#define FEATURE_4NT_FILENAME_COMPLETION

/* Define to enable the directory stack */
#define FEATURE_DIRECTORY_STACK

/* Define to activate redirections and piping */
#define FEATURE_REDIRECTION

/* Define to enable dynamic switching of TRACE output in console */
#define FEATURE_DYNAMIC_TRACE

/* Define one of these to select the used locale. */
/*  (date and time formats etc.) used in DATE, TIME, */
/*  DIR, PROMPT etc. */
/* #define LOCALE_WINDOWS */   /* System locale */
/* #define LOCALE_GERMAN */    /* German locale */
/* #define LOCALE_DEFAULT */   /* United States locale */

#ifdef NT4_INTERNAL_COMMANDS
#define INCLUDE_CMD_ACTIVATE
#endif
#define INCLUDE_CMD_ASSOC
#define INCLUDE_CMD_CHDIR
#define INCLUDE_CMD_CHOICE
#define INCLUDE_CMD_CLS
#define INCLUDE_CMD_COLOR
#define INCLUDE_CMD_COPY
#define INCLUDE_CMD_CTTY
#define INCLUDE_CMD_DATE
#define INCLUDE_CMD_DEL
#define INCLUDE_CMD_DELAY
#define INCLUDE_CMD_DIR
#define INCLUDE_CMD_FREE
#define INCLUDE_CMD_MEMORY
#define INCLUDE_CMD_MKDIR
#define INCLUDE_CMD_MKLINK
#define INCLUDE_CMD_MOVE
#ifdef NT4_INTERNAL_COMMANDS
#define INCLUDE_CMD_MSGBOX
#endif
#define INCLUDE_CMD_PATH
#define INCLUDE_CMD_PROMPT
#define INCLUDE_CMD_RMDIR
#define INCLUDE_CMD_RENAME
#define INCLUDE_CMD_SCREEN
#define INCLUDE_CMD_SET
#define INCLUDE_CMD_START
#define INCLUDE_CMD_TIME
#define INCLUDE_CMD_TIMER
#define INCLUDE_CMD_TITLE
#define INCLUDE_CMD_TYPE
#define INCLUDE_CMD_VER
#define INCLUDE_CMD_REM
#define INCLUDE_CMD_PAUSE
#define INCLUDE_CMD_BEEP
#define INCLUDE_CMD_VERIFY
#define INCLUDE_CMD_VOL
#ifdef NT4_INTERNAL_COMMANDS
#define INCLUDE_CMD_WINDOW
#endif
