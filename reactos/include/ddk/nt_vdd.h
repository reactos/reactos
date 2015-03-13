/*
 * nt_vdd.h
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#define _NT_VDD

#ifdef __cplusplus
extern "C" {
#endif

/*
 * VDM Control
 */

VOID
WINAPI
VDDSimulate16(VOID);

VOID
WINAPI
VDDTerminateVDM(VOID);


/*
 * IRQ services
 */

WORD
WINAPI
VDDReserveIrqLine
(
    IN HANDLE hVdd,
    IN WORD   IrqLine
);

BOOL
WINAPI
VDDReleaseIrqLine
(
    IN HANDLE hVdd,
    IN WORD   IrqLine
);


/*
 * I/O Port services
 */

typedef VOID (WINAPI *PFNVDD_INB)   (WORD iport, PBYTE data);
typedef VOID (WINAPI *PFNVDD_INW)   (WORD iport, PWORD data);
typedef VOID (WINAPI *PFNVDD_INSB)  (WORD iport, PBYTE data, WORD count);
typedef VOID (WINAPI *PFNVDD_INSW)  (WORD iport, PWORD data, WORD count);
typedef VOID (WINAPI *PFNVDD_OUTB)  (WORD iport, BYTE  data);
typedef VOID (WINAPI *PFNVDD_OUTW)  (WORD iport, WORD  data);
typedef VOID (WINAPI *PFNVDD_OUTSB) (WORD iport, PBYTE data, WORD count);
typedef VOID (WINAPI *PFNVDD_OUTSW) (WORD iport, PWORD data, WORD count);

typedef struct _VDD_IO_HANDLERS
{
    PFNVDD_INB   inb_handler;
    PFNVDD_INW   inw_handler;
    PFNVDD_INSB  insb_handler;
    PFNVDD_INSW  insw_handler;
    PFNVDD_OUTB  outb_handler;
    PFNVDD_OUTW  outw_handler;
    PFNVDD_OUTSB outsb_handler;
    PFNVDD_OUTSW outsw_handler;
} VDD_IO_HANDLERS, *PVDD_IO_HANDLERS;

typedef struct _VDD_IO_PORTRANGE
{
    WORD First;
    WORD Last;
} VDD_IO_PORTRANGE, *PVDD_IO_PORTRANGE;

BOOL
WINAPI
VDDInstallIOHook
(
    IN HANDLE            hVdd,
    IN WORD              cPortRange,
    IN PVDD_IO_PORTRANGE pPortRange,
    IN PVDD_IO_HANDLERS  IOhandler
);

VOID
WINAPI
VDDDeInstallIOHook
(
    IN HANDLE            hVdd,
    IN WORD              cPortRange,
    IN PVDD_IO_PORTRANGE pPortRange
);


/*
 * DMA services
 */

typedef struct _VDD_DMA_INFO
{
    WORD    addr;
    WORD    count;
    WORD    page;
    BYTE    status;
    BYTE    mode;
    BYTE    mask;
} VDD_DMA_INFO, *PVDD_DMA_INFO;

#define VDD_DMA_ADDR    0x01
#define VDD_DMA_COUNT   0x02
#define VDD_DMA_PAGE    0x04
#define VDD_DMA_STATUS  0x08
#define VDD_DMA_ALL     (VDD_DMA_ADDR | VDD_DMA_COUNT | VDD_DMA_PAGE | VDD_DMA_STATUS)

DWORD
WINAPI
VDDRequestDMA
(
    IN HANDLE    hVdd,
    IN WORD      iChannel,
    IN OUT PVOID Buffer,
    IN DWORD     length
);

BOOL
WINAPI
VDDQueryDMA
(
    IN HANDLE        hVdd,
    IN WORD          iChannel,
    IN PVDD_DMA_INFO pDmaInfo
);

BOOL
WINAPI
VDDSetDMA
(
    IN HANDLE        hVdd,
    IN WORD          iChannel,
    IN WORD          fDMA,
    IN PVDD_DMA_INFO pDmaInfo
);


/*
 * Memory services
 */

typedef enum
{
    VDM_V86,
    VDM_PM
} VDM_MODE;

#ifndef MSW_PE
#define MSW_PE  0x0001
#endif

#define getMODE() ((getMSW() & MSW_PE) ? VDM_PM : VDM_V86)

typedef VOID
(WINAPI *PVDD_MEMORY_HANDLER)
(
    PVOID FaultAddress,
    ULONG RWMode
);

PBYTE
WINAPI
Sim32pGetVDMPointer
(
    IN ULONG   Address,
    IN BOOLEAN ProtectedMode
);

PBYTE
WINAPI
MGetVdmPointer
(
    IN ULONG   Address,
    IN ULONG   Size,
    IN BOOLEAN ProtectedMode
);

PVOID
WINAPI
VdmMapFlat
(
    IN USHORT   Segment,
    IN ULONG    Offset,
    IN VDM_MODE Mode
);

BOOL
WINAPI
VdmFlushCache
(
    IN USHORT   Segment,
    IN ULONG    Offset,
    IN ULONG    Size,
    IN VDM_MODE Mode
);

BOOL
WINAPI
VdmUnmapFlat
(
    IN USHORT   Segment,
    IN ULONG    Offset,
    IN PVOID    Buffer,
    IN VDM_MODE Mode
);

BOOL
WINAPI
VDDInstallMemoryHook
(
    IN HANDLE hVdd,
    IN PVOID  pStart,
    IN DWORD  dwCount,
    IN PVDD_MEMORY_HANDLER MemoryHandler
);

BOOL
WINAPI
VDDDeInstallMemoryHook
(
    IN HANDLE hVdd,
    IN PVOID  pStart,
    IN DWORD  dwCount
);

#ifdef __cplusplus
}
#endif

/* EOF */
