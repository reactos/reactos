/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            ntoskrnl/io/pnpmgr/arbs.c
 * PURPOSE:         Root arbiters of the PnP manager
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootBusNumberArbiter;
extern ARBITER_INSTANCE IopRootIrqArbiter;
extern ARBITER_INSTANCE IopRootDmaArbiter;
extern ARBITER_INSTANCE IopRootMemArbiter;
extern ARBITER_INSTANCE IopRootPortArbiter;

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

/* BusNumber arbiter */

NTSTATUS
NTAPI
IopBusNumberInitialize(VOID)
{
    NTSTATUS Status;

    DPRINT("IopRootBusNumberArbiter %p\n", &IopRootBusNumberArbiter);

    Status = ArbInitializeArbiterInstance(&IopRootBusNumberArbiter,
                                          NULL,
                                          CmResourceTypeBusNumber,
                                          L"RootBusNumber",
                                          L"Root",
                                          NULL);
    return Status;
}

/* Irq arbiter */

NTSTATUS
NTAPI
IopIrqInitialize(VOID)
{
    NTSTATUS Status;

    DPRINT("IopRootIrqArbiter %p\n", &IopRootIrqArbiter);

    Status = ArbInitializeArbiterInstance(&IopRootIrqArbiter,
                                          NULL,
                                          CmResourceTypeInterrupt,
                                          L"RootIRQ",
                                          L"Root",
                                          NULL);
    return Status;
}

/* Dma arbiter */

NTSTATUS
NTAPI
IopDmaInitialize(VOID)
{
    NTSTATUS Status;

    DPRINT("IopRootDmaArbiter %p\n", &IopRootDmaArbiter);

    Status = ArbInitializeArbiterInstance(&IopRootDmaArbiter,
                                          NULL,
                                          CmResourceTypeDma,
                                          L"RootDMA",
                                          L"Root",
                                          NULL);
    return Status;
}

/* Memory arbiter */

NTSTATUS
NTAPI
IopMemInitialize(VOID)
{
    NTSTATUS Status;

    DPRINT("IopRootMemArbiter %p\n", &IopRootMemArbiter);

    Status = ArbInitializeArbiterInstance(&IopRootMemArbiter,
                                          NULL,
                                          CmResourceTypeMemory,
                                          L"RootMemory",
                                          L"Root",
                                          NULL);
    return Status;
}

/* Port arbiter */

NTSTATUS
NTAPI
IopPortInitialize(VOID)
{
    NTSTATUS Status;

    DPRINT("IopRootPortArbiter %p\n", &IopRootPortArbiter);

    Status = ArbInitializeArbiterInstance(&IopRootPortArbiter,
                                          NULL,
                                          CmResourceTypePort,
                                          L"RootPort",
                                          L"Root",
                                          NULL);
    return Status;
}

/* EOF */
