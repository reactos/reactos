/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mtrr.h

Abstract:

    This module contains the i386 specific mtrr register 
    hardware definitions.

Author:

    Ken Reneris (kenr)  11-Oct-95

Environment:

    Kernel mode only.

Revision History:

--*/

//
// MTRR MSR architecture definitions
//

#define MTRR_MSR_CAPABILITIES       0x0fe
#define MTRR_MSR_DEFAULT            0x2ff
#define MTRR_MSR_VARIABLE_BASE      0x200
#define MTRR_MSR_VARIABLE_MASK     (MTRR_MSR_VARIABLE_BASE+1)

#define MTRR_PAGE_SIZE              4096
#define MTRR_PAGE_MASK              (~(MTRR_PAGE_SIZE-1))

//
// Memory range types
//

#define MTRR_TYPE_UC            0
#define MTRR_TYPE_USWC          1
#define MTRR_TYPE_WT            4
#define MTRR_TYPE_WP            5
#define MTRR_TYPE_WB            6
#define MTRR_TYPE_MAX           7

//
// MTRR specific registers - capability register, default
// register, and variable mask and base register
//

#include "pshpack1.h"

typedef struct _MTRR_CAPABILITIES {
    union {
        struct {
            ULONG   VarCnt:8;
            ULONG   FixSupported:1;
            ULONG   Reserved_0:1;
            ULONG   UswcSupported:1;
            ULONG   Reserved_1:21;
            ULONG   Reserved_2;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_CAPABILITIES, *PMTRR_CAPABILITIES;

typedef struct _MTRR_DEFAULT {
    union {
        struct {
            ULONG   Type:8;
            ULONG   Reserved_0:2;
            ULONG   FixedEnabled:1;
            ULONG   MtrrEnabled:1;
            ULONG   Reserved_1:20;
            ULONG   Reserved_2;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_DEFAULT, *PMTRR_DEFAULT;

typedef struct _MTRR_VARIABLE_BASE {
    union {
        struct {
            ULONG       Type:8;
            ULONG       Reserved_0:4;
            ULONG       PhysBase_1:20;
            ULONG       PhysBase_2:4;
            ULONG       Reserved_1:28;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_VARIABLE_BASE, *PMTRR_VARIABLE_BASE;

#define MTRR_MASK_BASE  0x0000000ffffff000

typedef struct _MTRR_VARIABLE_MASK {
    union {
        struct {
            ULONG      Reserved_0:11;
            ULONG      Valid:1;
            ULONG      PhysMask_1:20;
            ULONG      PhysMask_2:4;
            ULONG      Reserved_1:28;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} MTRR_VARIABLE_MASK, *PMTRR_VARIABLE_MASK;

#define MTRR_MASK_MASK  0x0000000ffffff000

//
// Masks/constants to check for non-contiguous masks,
// mask out reserved bits of variable MTRR's, 
// and construct MTRR variable register masks 
//

#define MASK_OVERFLOW_MASK  (~0x1000000000)
#define MTRR_RESVBIT_MASK   0xfffffffff
#define MTRR_MAX_RANGE_SHIFT    36

#include "poppack.h"
