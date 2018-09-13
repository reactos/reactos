//////////////////////////////////////////////////////////////////////////
//
// INTEL Corporation Proprietary Information
// Copyright (c) Intel Corporation
//
// This listing is supplied under the terms of a license aggreement
// with INTEL Corporation and may not be used, copied nor disclosed
// except in accordance with that agreement.
//
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// $Workfile:   TRACE.H  $
// $Revision:   1.5  $
// $Modtime:   12 Jan 1996 15:09:00  $
//
// DESCRIPTION:
//
// this file defines a macros for tracing and the function prototypes
// for the actual output functions. If the symbol TRACING is not
// defined  all the macros expands to ((void)0).
//
// There are three global variables that control the behavior of the
// tracing macros/functions.  debugLevel is a 32 bit bitmask that
// determine controls what level of debug messages is output.
// iTraceDestination controls whether the debug output goes to a file or
// to the aux device. if iTraceDestination == TRACE_TO_FILE szTraceFile
// must contain the filename
//
/////////////////////////////////////////////////////////////////////

#ifndef __TRACE_H__
#define __TRACE_H__

extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
VOID PrintDebugString(char *format, ...);

extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
LONG
Ws2ExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    );

//
// defines for where the debug output should go
//
#define TRACE_TO_FILE    0
#define TRACE_TO_AUX     1

// the size of the string buffers used as arg to wsprintf
// in trace.c
#define TRACE_OUTPUT_BUFFER_SIZE  1024

// Debug level masks
#define DBG_TRACE       0x00000001
#define DBG_WARN        0x00000002
#define DBG_ERR         0x00000004
#define DBG_MEMORY      0x00000008
#define DBG_LIST        0x00000010
#define DBG_FUNCTION    0x00000020

#if defined(TRACING)

extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
DWORD debugLevel;

//
// This macro creates debug output depending on the debug mask "sev" and
// calls PrintDebugString output function. PrintDebugString makes the
// descision on whether the output goes into a file or to the aux device.
//
#define  DEBUGF(sev, var_args)                                                \
{                                                                             \
   if ((sev) & debugLevel) {                                                  \
      switch (sev) {                                                          \
         case DBG_TRACE:                                                      \
            PrintDebugString("-| WS2_32 TRACE   :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
         break;                                                               \
         case DBG_WARN:                                                       \
            PrintDebugString("-| WS2_32 WARNING :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
            break;                                                            \
         case DBG_ERR:                                                        \
            PrintDebugString("-| WS2_32 ERROR   :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args;                                        \
            break;                                                            \
        case DBG_MEMORY:                                                      \
            PrintDebugString("-| WS2_32 MEMORY  :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
            break;                                                            \
        case DBG_LIST:                                                        \
            PrintDebugString("-| WS2_32 LIST    :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );\
            PrintDebugString var_args ;                                       \
            break;                                                            \
        case DBG_FUNCTION:                                                    \
            PrintDebugString var_args;                                        \
            break;                                                            \
      }                                                                       \
   }                                                                          \
}                                                                             \



#define ALLOC_LOG( pointer, size)                                             \
    DEBUGF( DBG_MEMORY ,("MEMORY %lX size %X Allocated \n",                   \
                         (pointer),(size)))                                   \

#define DEALLOC_LOG(pointer, size)                                            \
DEBUGF( DBG_MEMORY ,("MEMORY %lX size %X Deallocated \n",                     \
                         (pointer),(size)))                                   \


#define LIST_ADD_LOG(list, element)                                           \
    DEBUGF( DBG_LIST ,("LIST %lX element %lX Added \n",                       \
                       (list),(element)))                                     \

#define LIST_DEL_LOG(list, element)                                           \
    DEBUGF( DBG_LIST ,("LIST %lX element %lX Deleted \n",                     \
                       (list),(element)))                                     \

#define ENTER_FUNCTION(name)                                                  \
DEBUGF( DBG_FUNCTION,name)                                \


#define EXIT_FUNCTION(name)                                                   \
DEBUGF( DBG_FUNCTION,name)                                \

#define WS2_EXCEPTION_FILTER()                            \
            Ws2ExceptionFilter(                           \
                GetExceptionInformation(),                \
                (LPSTR)__FILE__,                          \
                (LONG)__LINE__                            \
                )

#else // TRACING
     // make sure that these are defined if tracing is turned off
#define DEBUGF(sev, va)                     ((void)0)
#define LIST_ADD_LOG(list, element)         ((void)0)
#define LIST_DEL_LOG(list, element)         ((void)0)
#define ENTER_FUNCTION(name)                ((void)0)
#define EXIT_FUNCTION(name)                 ((void)0)
#define WS2_EXCEPTION_FILTER()              EXCEPTION_EXECUTE_HANDLER
#define ALLOC_LOG( pointer, size)                                             \
    DEBUGF( DBG_MEMORY ,("",                                                  \
                         (pointer),(size)))                                   \

#define DEALLOC_LOG(pointer, size)\
    DEBUGF( DBG_MEMORY ,("",\
                         (pointer),(size)))  \

#endif // TRACING

#endif // __TRACE_H__


