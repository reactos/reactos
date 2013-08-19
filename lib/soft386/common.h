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
    USHORT Selector
);

inline
BOOLEAN
Soft386FetchByte
(
    PSOFT386_STATE State,
    PUCHAR Data
);

inline
BOOLEAN
Soft386FetchWord
(
    PSOFT386_STATE State,
    PUSHORT Data
);

inline
BOOLEAN
Soft386FetchDword
(
    PSOFT386_STATE State,
    PULONG Data
);

inline
BOOLEAN
Soft386InterruptInternal
(
    PSOFT386_STATE State,
    USHORT SegmentSelector,
    ULONG Offset,
    BOOLEAN InterruptGate
);

inline
BOOLEAN
Soft386GetIntVector
(
    PSOFT386_STATE State,
    UCHAR Number,
    PSOFT386_IDT_ENTRY IdtEntry
);

VOID
__fastcall
Soft386Exception
(
    PSOFT386_STATE State,
    INT ExceptionCode
);

#endif // _COMMON_H_

/* EOF */
