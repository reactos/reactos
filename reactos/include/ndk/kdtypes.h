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
