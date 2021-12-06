
/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Public Header File for SMP
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* APIC specific functions inside apic/apicsmp.c */

VOID
ApicStartApplicationProcessor(ULONG NTProcessorNumber, PHYSICAL_ADDRESS StartupLoc);

VOID
NTAPI
HalpRequestIpi(KAFFINITY TargetProcessors);
