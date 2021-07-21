/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private Header File for SMP
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

VOID
HalpInitializeAPStub(PVOID APStubLocation);

VOID
HalpInitalizeAPPageTable(PVOID APStubLocation);
