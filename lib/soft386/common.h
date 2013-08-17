/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.h
 * PURPOSE:         Common functions used internally by Soft386 (header file).
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/* DEFINES ********************************************************************/

#define GET_SEGMENT_RPL(s) ((s) & 3)
#define GET_SEGMENT_INDEX(s) ((s) & 0xFFF8)

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

inline
BOOLEAN
Soft386LoadSegment
(
    PSOFT386_STATE State,
    INT Segment,
    WORD Selector
);

#endif // _COMMON_H_

/* EOF */
