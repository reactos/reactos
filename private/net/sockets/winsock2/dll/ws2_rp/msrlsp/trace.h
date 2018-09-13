/*++

     Copyright c 1996 Intel Corporation
     All Rights Reserved
     
     Permission is granted to use, copy and distribute this software and 
     its documentation for any purpose and without fee, provided, that 
     the above copyright notice and this statement appear in all copies. 
     Intel makes no representations about the suitability of this 
     software for any purpose.  This software is provided "AS IS."  
     
     Intel specifically disclaims all warranties, express or implied, 
     and all liability, including consequential and other indirect 
     damages, for the use of this software, including liability for 
     infringement of any proprietary rights, and including the 
     warranties of merchantability and fitness for a particular purpose. 
     Intel does not assume any responsibility for any errors which may 
     appear in this software nor any responsibility to update it.

Module Name:

    SPI.CPP : 

Abstract:

    This file defines a macros for tracing and the function prototypes for the
    actual output functions. If the symbol TRACING is not defined  all the
    macros expands to ((void)0). 

    There are three global variables that control the behavior of the tracing
    macros/functions.  debugLevel is a 32 bit bitmask that determine controls
    what level of debug messages is output. iTraceDestination controls whether
    the debug output goes to a file or to the aux device. if iTraceDestination
    == TRACE_TO_FILE szTraceFile must contain the filename. 

Author:

    bugs@brandy.jf.intel.com
    
Revision History:

   
--*/

#ifndef __TRACE_H__
#define __TRACE_H__

extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
VOID PrintDebugString(char *format, ...);

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
            PrintDebugString("-| TRACE   :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
         break;                                                               \
         case DBG_WARN:                                                       \
            PrintDebugString("-| WARNING :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
            break;                                                            \
         case DBG_ERR:                                                        \
            PrintDebugString("-| ERROR   :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args;                                        \
            break;                                                            \
        case DBG_MEMORY:                                                      \
            PrintDebugString("-| MEMORY  :: ");                               \
            PrintDebugString(" %s : %d |-\n", __FILE__, __LINE__ );           \
            PrintDebugString var_args ;                                       \
            break;                                                            \
        case DBG_LIST:                                                        \
            PrintDebugString("-| LIST    :: ");                               \
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


#else // TRACING
     // make sure that these are defined if tracing is turned off
#define DEBUGF(sev, va)                     ((void)0)
#define LIST_ADD_LOG(list, element)         ((void)0)
#define LIST_DEL_LOG(list, element)         ((void)0)
#define ENTER_FUNCTION(name)                ((void)0)
#define EXIT_FUNCTION(name)                 ((void)0)
#define ALLOC_LOG( pointer, size)                                             \
    DEBUGF( DBG_MEMORY ,("",                                                  \
                         (pointer),(size)))                                   \

#define DEALLOC_LOG(pointer, size)\
    DEBUGF( DBG_MEMORY ,("",\
                         (pointer),(size)))  \

#endif // TRACING

#endif // __TRACE_H__


