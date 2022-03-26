/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header File for SMP support
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* This table is filled for each physical processor on system */
typedef struct _PROCESSOR_IDENTITY
{
    UCHAR ProcessorId;
    UCHAR LapicId;
    BOOLEAN ProcessorStarted;
    BOOLEAN BSPCheck;
    PKPRCB ProcessorPrcb;

} PROCESSOR_IDENTITY, *PPROCESSOR_IDENTITY;

VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
HalpSetupProcessorsTable(
    _In_ UINT32 NTProcessorNumber);

VOID
HalpPrintApicTables(VOID);
