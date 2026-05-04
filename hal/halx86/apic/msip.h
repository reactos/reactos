/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header file for MSI (Message Signalled Interrupts) support
 * COPYRIGHT:   Copyright 2026 ReactOS Contributors
 */

#pragma once

/* MSI Vector Allocation Flags */
#define MSI_ALLOC_SUCCESS           0x01
#define MSI_ALLOC_NO_VECTORS        0x02
#define MSI_ALLOC_INVALID_IRQL      0x03

/* MSI Vector Range Definitions */
#define MSI_VECTOR_MIN              0x30    /* Start of allocatable MSI vectors */
#define MSI_VECTOR_MAX              0xCF    /* End of allocatable MSI vectors */
#define MSI_VECTOR_COUNT            (MSI_VECTOR_MAX - MSI_VECTOR_MIN + 1)

/* MSI Vector Context Structure */
typedef struct _MSI_VECTOR_CONTEXT {
    UCHAR Vector;                           /* Allocated system vector */
    KIRQL Irql;                             /* IRQL for this vector */
    KAFFINITY Affinity;                     /* CPU affinity */
    BOOLEAN Is64Bit;                        /* 64-bit MSI address support */
    UCHAR Reserved[3];
} MSI_VECTOR_CONTEXT, *PMSI_VECTOR_CONTEXT;

/* Function Prototypes */

/*
 * HalpAllocateMsiVector
 *
 * Allocates a system vector for MSI use.
 *
 * Parameters:
 *   DesiredIrql     - Desired IRQL for the interrupt
 *   OutVector       - Pointer to receive allocated vector
 *   OutIrql         - Pointer to receive actual IRQL
 *   OutAffinity     - Pointer to receive CPU affinity
 *
 * Returns:
 *   TRUE if vector successfully allocated
 *   FALSE if no vectors available
 */
BOOLEAN
NTAPI
HalpAllocateMsiVector(
    _In_ KIRQL DesiredIrql,
    _Out_ PUCHAR OutVector,
    _Out_ PKIRQL OutIrql,
    _Out_ PKAFFINITY OutAffinity
);

/*
 * HalpFreeMsiVector
 *
 * Releases a previously allocated MSI vector.
 *
 * Parameters:
 *   Vector          - Vector to release
 *
 * Returns:
 *   TRUE if vector successfully freed
 *   FALSE if vector was not allocated or invalid
 */
BOOLEAN
NTAPI
HalpFreeMsiVector(
    _In_ UCHAR Vector
);

/*
 * HalpMsiVectorToIrql
 *
 * Converts a vector to its associated IRQL.
 *
 * Parameters:
 *   Vector          - Vector number
 *
 * Returns:
 *   Associated IRQL level
 */
KIRQL
FASTCALL
HalpMsiVectorToIrql(
    _In_ UCHAR Vector
);

/*
 * HalpInitializeMsiVectors
 *
 * Initializes the MSI vector allocation infrastructure during HAL startup.
 *
 * Returns:
 *   TRUE if initialization successful
 *   FALSE if initialization failed
 */
BOOLEAN
NTAPI
HalpInitializeMsiVectors(VOID);
