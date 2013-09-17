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

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SIGN_FLAG_BYTE 0x80
#define SIGN_FLAG_WORD 0x8000
#define SIGN_FLAG_LONG 0x80000000
#define GET_SEGMENT_RPL(s) ((s) & 3)
#define GET_SEGMENT_INDEX(s) ((s) & 0xFFF8)
#define EXCEPTION_HAS_ERROR_CODE(x) (((x) == 8) || ((x) >= 10 && (x) <= 14))
#define PAGE_ALIGN(x) ((x) & 0xFFFFF000)
#define PAGE_OFFSET(x) ((x) & 0x00000FFF)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

typedef struct _SOFT386_MOD_REG_RM
{
    INT Register;
    BOOLEAN Memory;
    union
    {
        INT SecondRegister;
        ULONG MemoryAddress;
    };
} SOFT386_MOD_REG_RM, *PSOFT386_MOD_REG_RM;

#pragma pack(push, 1)

typedef union _SOFT386_PAGE_DIR
{
    struct
    {
        ULONG Present : 1;
        ULONG Writeable : 1;
        ULONG Usermode : 1;
        ULONG WriteThrough : 1;
        ULONG NoCache : 1;
        ULONG Accessed : 1;
        ULONG AlwaysZero : 1;
        ULONG Size : 1;
        ULONG Unused : 4;
        ULONG TableAddress : 20;
    };
    ULONG Value;
} SOFT386_PAGE_DIR, *PSOFT386_PAGE_DIR;

typedef struct _SOFT386_PAGE_TABLE
{
    union
    {
        ULONG Present : 1;
        ULONG Writeable : 1;
        ULONG Usermode : 1;
        ULONG WriteThrough : 1;
        ULONG NoCache : 1;
        ULONG Accessed : 1;
        ULONG Dirty : 1;
        ULONG AlwaysZero : 1;
        ULONG Global : 1;
        ULONG Unused : 3;
        ULONG Address : 20;
    };
    ULONG Value;
} SOFT386_PAGE_TABLE, *PSOFT386_PAGE_TABLE;

#pragma pack(pop)

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
FASTCALL
Soft386ExceptionWithErrorCode
(
    PSOFT386_STATE State,
    INT ExceptionCode,
    ULONG ErrorCode
);

inline
VOID
Soft386Exception
(
    PSOFT386_STATE State,
    INT ExceptionCode
);

inline
BOOLEAN
Soft386CalculateParity
(
    UCHAR Number
);

inline
BOOLEAN
Soft386ParseModRegRm
(
    PSOFT386_STATE State,
    BOOLEAN AddressSize,
    PSOFT386_MOD_REG_RM ModRegRm
);

inline
BOOLEAN
Soft386ReadModrmByteOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    PUCHAR RegValue,
    PUCHAR RmValue
);

inline
BOOLEAN
Soft386ReadModrmWordOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    PUSHORT RegValue,
    PUSHORT RmValue
);

inline
BOOLEAN
Soft386ReadModrmDwordOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    PULONG RegValue,
    PULONG RmValue
);

inline
BOOLEAN
Soft386WriteModrmByteOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    BOOLEAN WriteRegister,
    UCHAR Value
);

inline
BOOLEAN
Soft386WriteModrmWordOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    BOOLEAN WriteRegister,
    USHORT Value
);

inline
BOOLEAN
Soft386WriteModrmDwordOperands
(
    PSOFT386_STATE State,
    PSOFT386_MOD_REG_RM ModRegRm,
    BOOLEAN WriteRegister,
    ULONG Value
);

#endif // _COMMON_H_

/* EOF */
