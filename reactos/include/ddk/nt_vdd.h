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

/* VDM Control */

VOID
WINAPI
VDDSimulate16(VOID);

VOID
WINAPI
VDDTerminateVDM(VOID);


/* IRQ services */

WORD
WINAPI
VDDReserveIrqLine(
  _In_ HANDLE hVdd,
  _In_ WORD IrqLine);

BOOL
WINAPI
VDDReleaseIrqLine(
  _In_ HANDLE hVdd,
  _In_ WORD IrqLine);


/* I/O Port services */

typedef VOID
(WINAPI *PFNVDD_INB)(
  WORD iport,
  PBYTE data);

typedef VOID
(WINAPI *PFNVDD_INW)(
  WORD iport,
  PWORD data);

typedef VOID
(WINAPI *PFNVDD_INSB)(
  WORD iport,
  PBYTE data,
  WORD count);

typedef VOID
(WINAPI *PFNVDD_INSW)(
  WORD iport,
  PWORD data,
  WORD count);

typedef VOID
(WINAPI *PFNVDD_OUTB)(
  WORD iport,
  BYTE data);

typedef VOID
(WINAPI *PFNVDD_OUTW)(
  WORD iport,
  WORD data);

typedef VOID
(WINAPI *PFNVDD_OUTSB)(
  WORD iport,
  PBYTE data,
  WORD count);

typedef VOID
(WINAPI *PFNVDD_OUTSW)(
  WORD iport,
  PWORD data,
  WORD count);

typedef struct _VDD_IO_HANDLERS {
  PFNVDD_INB inb_handler;
  PFNVDD_INW inw_handler;
  PFNVDD_INSB insb_handler;
  PFNVDD_INSW insw_handler;
  PFNVDD_OUTB outb_handler;
  PFNVDD_OUTW outw_handler;
  PFNVDD_OUTSB outsb_handler;
  PFNVDD_OUTSW outsw_handler;
} VDD_IO_HANDLERS, *PVDD_IO_HANDLERS;

typedef struct _VDD_IO_PORTRANGE {
  WORD First;
  WORD Last;
} VDD_IO_PORTRANGE, *PVDD_IO_PORTRANGE;

BOOL
WINAPI
VDDInstallIOHook(
  _In_ HANDLE hVdd,
  _In_ WORD cPortRange,
  _In_ PVDD_IO_PORTRANGE pPortRange,
  _In_ PVDD_IO_HANDLERS IOhandler);

VOID
WINAPI
VDDDeInstallIOHook(
  _In_ HANDLE hVdd,
  _In_ WORD cPortRange,
  _In_ PVDD_IO_PORTRANGE pPortRange);


/* DMA services */

typedef struct _VDD_DMA_INFO {
  WORD addr;
  WORD count;
  WORD page;
  BYTE status;
  BYTE mode;
  BYTE mask;
} VDD_DMA_INFO, *PVDD_DMA_INFO;

#define VDD_DMA_ADDR   0x01
#define VDD_DMA_COUNT  0x02
#define VDD_DMA_PAGE   0x04
#define VDD_DMA_STATUS 0x08
#define VDD_DMA_ALL    (VDD_DMA_ADDR | VDD_DMA_COUNT | VDD_DMA_PAGE | VDD_DMA_STATUS)

DWORD
WINAPI
VDDRequestDMA(
  _In_ HANDLE hVdd,
  _In_ WORD iChannel,
  _Inout_ PVOID Buffer,
  _In_ DWORD length);

BOOL
WINAPI
VDDQueryDMA(
  _In_ HANDLE hVdd,
  _In_ WORD iChannel,
  _In_ PVDD_DMA_INFO pDmaInfo);

BOOL
WINAPI
VDDSetDMA(
  _In_ HANDLE hVdd,
  _In_ WORD iChannel,
  _In_ WORD fDMA,
  _In_ PVDD_DMA_INFO pDmaInfo);


/* Memory services */

typedef enum {
  VDM_V86,
  VDM_PM
} VDM_MODE;

#ifndef MSW_PE
#define MSW_PE 0x0001
#endif

#define getMODE() ((getMSW() & MSW_PE) ? VDM_PM : VDM_V86)

typedef VOID
(WINAPI *PVDD_MEMORY_HANDLER)(
  PVOID FaultAddress,
  ULONG RWMode);

PBYTE
WINAPI
Sim32pGetVDMPointer(
  _In_ ULONG Address,
  _In_ BOOLEAN ProtectedMode);

PBYTE
WINAPI
MGetVdmPointer(
  _In_ ULONG Address,
  _In_ ULONG Size,
  _In_ BOOLEAN ProtectedMode);

PVOID
WINAPI
VdmMapFlat(
  _In_ USHORT Segment,
  _In_ ULONG Offset,
  _In_ VDM_MODE Mode);

BOOL
WINAPI
VdmFlushCache(
  _In_ USHORT Segment,
  _In_ ULONG Offset,
  _In_ ULONG Size,
  _In_ VDM_MODE Mode);

BOOL
WINAPI
VdmUnmapFlat(
  _In_ USHORT Segment,
  _In_ ULONG Offset,
  _In_ PVOID Buffer,
  _In_ VDM_MODE Mode);

BOOL
WINAPI
VDDInstallMemoryHook(
  _In_ HANDLE hVdd,
  _In_ PVOID pStart,
  _In_ DWORD dwCount,
  _In_ PVDD_MEMORY_HANDLER MemoryHandler);

BOOL
WINAPI
VDDDeInstallMemoryHook(
  _In_ HANDLE hVdd,
  _In_ PVOID pStart,
  _In_ DWORD dwCount);

BOOL
WINAPI
VDDAllocMem(
  _In_ HANDLE hVdd,
  _In_ PVOID Address,
  _In_ ULONG Size);

BOOL
WINAPI
VDDFreeMem(
  _In_ HANDLE hVdd,
  _In_ PVOID Address,
  _In_ ULONG Size);

BOOL
WINAPI
VDDIncludeMem(
  _In_ HANDLE hVdd,
  _In_ PVOID Address,
  _In_ ULONG Size);

BOOL
WINAPI
VDDExcludeMem(
  _In_ HANDLE hVdd,
  _In_ PVOID Address,
  _In_ ULONG Size);

#ifdef __cplusplus
}
#endif

/* EOF */
