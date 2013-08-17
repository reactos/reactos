/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.h
 * PURPOSE:         Common functions used internally by Soft386 (header file).
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/* INCLUDES *******************************************************************/

#include <soft386.h>

/* DEFINES ********************************************************************/

#define GET_SEGMENT_DPL(s) ((s) & 3)

/* FUNCTIONS ******************************************************************/

inline
BOOLEAN
Soft386ReadMemory
(
    PSOFT386_STATE State,
    INT SegmentReg,
    ULONG Offset,
    BOOLEAN InstFetch,
    PVOID Buffer,
    ULONG Size
);

inline
BOOLEAN
Soft386WriteMemory
(
    PSOFT386_STATE State,
    INT SegmentReg,
    ULONG Offset,
    PVOID Buffer,
    ULONG Size
);

inline
BOOLEAN
Soft386StackPush
(
    PSOFT386_STATE State,
    ULONG Value
);

inline
BOOLEAN
Soft386StackPop
(
    PSOFT386_STATE State,
    PULONG Value
);

#endif // _COMMON_H_

/* EOF */
