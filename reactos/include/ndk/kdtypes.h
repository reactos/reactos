/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/kdtypes.h
 * PURPOSE:         Definitions for Kernel Debugger Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KDTYPES_H
#define _KDTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define DPFLTR_ERROR_LEVEL                  0
#define DPFLTR_WARNING_LEVEL                1
#define DPFLTR_TRACE_LEVEL                  2
#define DPFLTR_INFO_LEVEL                   3
#define DPFLTR_MASK                         0x80000000

#define DBG_STATUS_CONTROL_C                1
#define DBG_STATUS_SYSRQ                    2
#define DBG_STATUS_BUGCHECK_FIRST           3
#define DBG_STATUS_BUGCHECK_SECOND          4
#define DBG_STATUS_FATAL                    5
#define DBG_STATUS_DEBUG_CONTROL            6
#define DBG_STATUS_WORKER                   7

#define BREAKPOINT_PRINT                    1
#define BREAKPOINT_PROMPT                   2
/* ENUMERATIONS **************************************************************/

typedef enum _DEBUG_CONTROL_CODE
{
    DebugGetTraceInformation = 1,
    DebugSetInternalBreakpoint,
    DebugSetSpecialCall,
    DebugClearSpecialCalls,
    DebugQuerySpecialCalls,
    DebugDbgBreakPoint,
    DebugDbgLoadSymbols
} DEBUG_CONTROL_CODE;

/* TYPES *********************************************************************/

typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;

#endif
