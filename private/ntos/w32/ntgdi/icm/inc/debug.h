/****************************Module*Header******************************\
* Module Name: DEBUG.H
*
* Module Descripton: Debugging macros for ICM project
*
* Warnings:
*
* Issues:
*
* Created:  8 January 1996
* Author:   Srinivasan Chandraekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef DBG

//
// These are used for debugging purposes, and do not generate code in the
// retail version of the driver.
//
// A global variable (again present only in the debug build) maintains the
// current debug level in the low order WORD. The high order WORD is treated
// as a bitfield and is used to give more flexibility to debugging.
//

PSTR StripDirPrefixA(PSTR);
VOID MyDebugPrintW(PWSTR, ...);
VOID MyDebugPrintA(PSTR, ...);

#ifdef UNICODE
#define MyDebugPrint    MyDebugPrintW
#else
#define MyDebugPrint    MyDebugPrintA
#endif

#define DBGPRINT            MyDebugPrint
#define DBGPRINTA           MyDebugPrintA
#define DBGBREAK()          DebugBreak()

//
// List of debug levels for low WORD of gdwDebugControl
//

#define DBG_LEVEL_VERBOSE   0x00000001
#define DBG_LEVEL_TERSE     0x00000002
#define DBG_LEVEL_WARNING   0x00000003
#define DBG_LEVEL_ERROR     0x00000004
#define DBG_LEVEL_FATAL     0x00000005

//
// Bits used in the high WORD of gdwDebugControl
//

#define FLAG_TRACEAPI       0x00010000      // Trace API entries

#define CHECK_DBG_LEVEL(level)  ((level) >= gdwDebugControl)

#define TRACEAPI(funcname)                                              \
    {                                                                   \
        if (gdwDebugControl & FLAG_TRACEAPI)                            \
        {                                                               \
            DBGPRINTA("ICM: Entering function ");                       \
            DBGPRINT funcname;                                          \
        }                                                               \
    }

#define DBGMSG(level, mesg)                                             \
    {                                                                   \
        if (CHECK_DBG_LEVEL(level))                                     \
        {                                                               \
            DBGPRINTA("ICM: %s (%d): ",                                 \
                    StripDirPrefixA(__FILE__), __LINE__);               \
            DBGPRINT mesg;                                              \
        }                                                               \
    }

//
// These are the main macros that you'll be using in your code.
// For giving additional parameters enclose the parameters in
// paranthesis as shown in the example below.
//
// WARNING((__TEXT("Out of memory")));
// ERR((__TEXT("Incorrect return value: %d"), rc));  // Note extra brackets
//

#define VERBOSE(mesg)       DBGMSG(DBG_LEVEL_VERBOSE, mesg)
#define TERSE(mesg)         DBGMSG(DBG_LEVEL_TERSE,   mesg)
#define WARNING(mesg)       DBGMSG(DBG_LEVEL_WARNING, mesg)
#define ERR(mesg)           DBGMSG(DBG_LEVEL_ERROR,   mesg)
#define FATAL(mesg)         DBGMSG(DBG_LEVEL_FATAL,   mesg)

//
// These macros are for Asserting and work independently of the
// debugging variable.
//

#define ASSERT(expr)                                                    \
    {                                                                   \
        if (! (expr)) {                                                 \
            DBGPRINTA("ICM: Assertion failed: %s (%d)\n",               \
                    StripDirPrefixA(__FILE__), __LINE__);               \
            DBGBREAK();                                                 \
        }                                                               \
    }

//
// For giving additional parameters, enclose the message and the other
// parameters in extra paranthesis as shown below.
//
// ASSERTMSG(x>0, "x less than 0");
// ASSERTMSG(x>0, ("x less than 0: x=%d", x));
//

#define ASSERTMSG(expr, mesg)                                           \
    {                                                                   \
        if (! (expr)) {                                                 \
            DBGPRINTA("ICM: Assertion failed: %s (%d)\n",               \
                    StripDirPrefixA(__FILE__), __LINE__);               \
            DBGPRINT mesg;                                              \
            DBGPRINTA("\n");                                            \
            DBGBREAK();                                                 \
        }                                                               \
    }

#define RIP(mesg)                                                       \
    {                                                                   \
        DBGPRINTA("ICM: ");                                             \
        DBGPRINT mesg;                                                  \
        DBGBREAK();                                                     \
    }

#else   // !DBG

#define TRACEAPI(mesg)
#define DBGMSG(level, mesg)
#define VERBOSE(mesg)
#define TERSE(mesg)
#define WARNING(mesg)
#define ERR(mesg)
#define FATAL(mesg)

#define ASSERT(expr)
#define ASSERTMSG(expr, mesg)

#define RIP(mesg)

#endif  // !DBG

#endif  // ifndef _DEBUG_H_

