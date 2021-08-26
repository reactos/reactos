/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private Header File for SMP
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

VOID
HalpInitializeAPStub(PVOID APStubLocation, 
                     KDESCRIPTOR FinalGdt, 
                     KDESCRIPTOR FinalIdt);

VOID
HalpWriteProcessorState(PVOID APStubLocation,
                        PKPROCESSOR_STATE ProcessorState,
                        UINT32 LoaderBlock);

VOID
HalpAdjustTempPageTable(PVOID APStubLocation, 
                        UINT32 PTELocationPhysical, 
                        PVOID PTELocationBase,
                        PKPROCESSOR_STATE ProcessorState,
                        UINT32 PageTableLocationPhysical);
