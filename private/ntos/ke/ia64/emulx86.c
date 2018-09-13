/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    emulx86.c

Abstract:

    This module implements all of the x86 opcode handling regardles of which
    mode the trap occured in (V86, protected, etc.). It is basically
    a combination of instemul.asm and emv86.asm from the .../ke/i386
    directory

Author:

    Charles Spirakis (intel) 1 Feb 1996

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"
#include "ia32def.h"
#include "vdmntos.h"

#if DEVL
ULONG ExVdmOpcodeDispatchCounts[MAX_VDM_INDEX];
ULONG ExVdmSegmentNotPresent;
#endif

extern ULONG KeIA32EFlagsAndMaskV86;
extern ULONG KeIA32EFlagsOrMaskV86;
extern BOOLEAN KeIA32VdmIoplAllowed;
extern ULONG KeIA32VirtualIntExtensions;

#if defined(WX86) && defined(TRY_NTVDM)
//
// These are external functions....
//

BOOLEAN
KiIA32VdmDispatchIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN UCHAR InstructionSize,
    IN PKIA32_FRAME TrapFrame
    );

BOOLEAN
KiIA32VdmDispatchStringIo(
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Rep,
    IN BOOLEAN Read,
    IN ULONG Count,
    IN ULONG Address,
    IN UCHAR InstructionSize,
    IN PKIA32_FRAME TrapFrame
    );


//
// Declare non-ke and non-ki function prototypes
//
BOOLEAN
PushInt(
    IN ULONG InterruptNumber,
    IN OUT PREGINFO Regs
    );

BOOLEAN
PushException(
    IN ULONG ExceptionNumber,
    IN OUT PREGINFO Regs
    );

BOOLEAN
CsToLinear(
    IN OUT PREGINFO Regs,
    IN BOOLEAN IsV86
    );

BOOLEAN
SsToLinear(
    IN OUT PREGINFO Regs,
    IN BOOLEAN IsV86
    );

BOOLEAN
CheckEip(
    IN PREGINFO Regs
    );

BOOLEAN
CheckEsp(
    IN PREGINFO Regs,
    IN ULONG StackNeeded
    );

VOID
GetVirtualBits(
    IN OUT PULONG PFlags,
    IN PKIA32_FRAME Frame
    );

VOID
SetVirtualBits(
    IN ULONG Flags,
    IN PREGINFO PRegs
    );

VOID
VdmDispatchIntAck(
    IN PKIA32_FRAME Frame
    );

VOID
CheckVdmFlags(
    IN OUT PREGINFO Regs
    );

VOID
KeIA32AndOrVdmLock(
    IN ULONG AndMask,
    IN ULONG OrMask
    );

BOOLEAN
KeIA32UnscrambleLdtEntry(
    IN ULONG Selector,
    OUT PKXDESCRIPTOR XDescriptor
    );

ULONGLONG
KeIA32Unscramble(
    IN PLDT_ENTRY Descriptor
    );

NTSTATUS
PspQueryDescriptorThread (
    PETHREAD Thread,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
    );




BOOLEAN
KiIA32DispatchOpcode(
    IN PKIA32_FRAME Frame
    )

/*++

Routine Description:

    This routine dispatches to the opcode of the specific emulation routine.
    based on the first byte of the opcode. It is a combination of the
    original x86 emulation routines written in assembly (i386/instemul.asm
    and i386/emv86.asm).

Arguments:

    Frame - pointer to the ia32 trap frame.

Return Value:

    Returns true if the opcode was handled, otherwise false

--*/

{
    REGINFO Regs;
    ULONG IOPort;
    ULONG IOCount;
    ULONG IOAddress;
    ULONG flags;
    KXDESCRIPTOR NewXDescriptor;

    //
    // Temporary variables that are needed to access memory in
    // different sizes...
    //
    PULONG longPtr;
    PUSHORT shortPtr;
    PUCHAR charPtr;

    //
    // Set up the valules that are always needed
    //
    Regs.RiInstLength = 1;
    Regs.RiPrefixFlags = 0;
    Regs.RiTrapFrame = Frame;
    Regs.RiSegCs = Frame->SegCs;
    Regs.RiEip = Frame->Eip;

    //
    // Convert and verify the linear address is legal
    //
    if (! CsToLinear(&Regs, VM86USER(Frame))) {
        return FALSE;
    }

    //
    // The ASM files used an arbitrary maximum length of 128 prefixes...
    // This constant comes from MI.INC (but is not 128)
    //
    while (Regs.RiInstLength <= MAX_INSTRUCTION_LENGTH) {

        // Should there be a Probe or a try/except block?
        Regs.RiOpcode = *Regs.RiLinearAddr;

        switch(Regs.RiOpcode) {
            case 0x0f:
                // Handle the 0x0f series of opcodes
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_0F]++;
#endif
#if VDMDBG
                DbgPrintf("Saw 0x0f opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                return(VdmOpcode0F(&Regs));

            case 0x26:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_ESPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw ES Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_ES;
                break;

            case 0x2e:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_CSPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw CS Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_CS;
                break;

            case 0x36:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_SSPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw SS Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_SS;
                break;

            case 0x3e:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_DSPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw DS Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_DS;
                break;

            case 0x64:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_FSPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw FS Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_FS;
                break;

            case 0x65:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_GSPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw GS Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_GS;
                break;

            case 0x66:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OPER32Prefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw Operand 32 Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_OPER32;
                break;

            case 0x67:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_ADDR32Prefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw Address 32 Prefix opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_ADDR32;
                break;

            //
            // The next four instructions (INSB, INSW, OUTSB, and OUTSW)
            // could be optimized to pass fewer parameters to 
            // KiIA32VdmDispatchStringIo(). But, in an attempt to change as
            // little as possible, this will be left alone for the moment.
            // Really only need to pass &regs structure, size info (byte/word)
            // and read/write info. All else is contained in &Regs...
            //
            case 0x6c:
                // The INSB instruction
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INSB]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INSB opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOAddress = ( Frame->SegEs << 16 ) + Frame->Edi;

                if (Regs.RiPrefixFlags & PREFIX_REP) {
                    IOCount = Frame->Ecx;
                }
                else {
                    IOCount = 1;
                }
                IOPort = Frame->Edx;

                return(KiIA32VdmDispatchStringIo(IOPort, 1, Regs.RiPrefixFlags & PREFIX_REP, TRUE, IOCount, IOAddress, Regs.RiInstLength, Frame));

            case 0x6d:
                // The INSW instruction
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INSW]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INSW opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOAddress = ( Frame->SegEs << 16 ) + Frame->Edi;

                if (Regs.RiPrefixFlags & PREFIX_REP) {
                    IOCount = Frame->Ecx;
                }
                else {
                    IOCount = 1;
                }
                IOPort = Frame->Edx;

                return(KiIA32VdmDispatchStringIo(IOPort, 2, Regs.RiPrefixFlags & PREFIX_REP, TRUE, IOCount, IOAddress, Regs.RiInstLength, Frame));

            case 0x6e:
                // The OUTSB instruction
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSB]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTSB opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOAddress = ( Frame->SegEs << 16 ) + Frame->Edi;

                if (Regs.RiPrefixFlags & PREFIX_REP) {
                    IOCount = Frame->Ecx;
                }
                else {
                    IOCount = 1;
                }
                IOPort = Frame->Edx;

                return(KiIA32VdmDispatchStringIo(IOPort, 1, Regs.RiPrefixFlags & PREFIX_REP, FALSE, IOCount, IOAddress, Regs.RiInstLength, Frame));

            case 0x6f:
                // The OUTSW instruction
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTSW]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTSW opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOAddress = ( Frame->SegEs << 16 ) + Frame->Edi;

                if (Regs.RiPrefixFlags & PREFIX_REP) {
                    IOCount = Frame->Ecx;
                }
                else {
                    IOCount = 1;
                }
                IOPort = Frame->Edx;

                return(KiIA32VdmDispatchStringIo(IOPort, 2, Regs.RiPrefixFlags & PREFIX_REP, FALSE, IOCount, IOAddress, Regs.RiInstLength, Frame));

            case 0x9b:
            case 0x0d8:
            case 0x0d9:
            case 0x0da:
            case 0x0db:
            case 0x0dc:
            case 0x0dd:
            case 0x0de:
            case 0x0df:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_NPX]++;
#endif
#if VDMDBG
                DbgPrintf("Saw NPX opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                break;

            case 0x09c:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_PUSHF]++;
#endif
#if VDMDBG
                DbgPrintf("Saw PUSHF opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // This one should only happen in V86 mode
                if (! VM86USER(Frame)) {
                    return FALSE;
                }

                flags = Frame->EFlags & (~EFLAGS_INTERRUPT_MASK);
                flags |= EFLAGS_IOPL_MASK;
                flags |= (*VdmFixedStateLinear & (VDM_VIRTUAL_INTERRUPTS | EFLAGS_ALIGN_CHECK | EFLAGS_NT_MASK));

                if (Regs.RiPrefixFlags & PREFIX_OPER32) {
                    Frame->HardwareEsp = (Frame->HardwareEsp - 4) & 0xffff;
                    longPtr = (PULONG) (Frame->SegSs << 4) + Frame->HardwareEsp;
                    *longPtr = flags;
                }
                else {
                    Frame->HardwareEsp = (Frame->HardwareEsp - 2) & 0xffff;
                    shortPtr = (PUSHORT) (Frame->SegSs << 4) + Frame->HardwareEsp;
                    *shortPtr = flags & 0xffff;
                }

                Frame->Eip += Regs.RiInstLength;

                return TRUE;

            case 0x9d:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_POPF]++;
#endif
#if VDMDBG
                DbgPrintf("Saw POPF opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // This one should only happen in V86 mode
                if (! VM86USER(Frame)) {
                    return FALSE;
                }

                if (Regs.RiPrefixFlags & PREFIX_OPER32) {
                    longPtr = (PULONG) ((Frame->SegSs << 4) + Frame->HardwareEsp);
                    flags = *longPtr;
                    Frame->HardwareEsp = (Frame->HardwareEsp + 4) & 0xffff;
                }
                else {
                    shortPtr = (PUSHORT) ((Frame->SegSs << 4) + Frame->HardwareEsp);
                    flags = *shortPtr;
                    Frame->HardwareEsp = (Frame->HardwareEsp + 2) & 0xffff;
                }

                flags &= ~EFLAGS_IOPL_MASK;

                KeIA32AndOrVdmLock(~(EFLAGS_INTERRUPT_MASK | EFLAGS_ALIGN_CHECK | EFLAGS_NT_MASK), (flags & (EFLAGS_INTERRUPT_MASK | EFLAGS_ALIGN_CHECK | EFLAGS_NT_MASK)));

                flags &= ~EFLAGS_NT_MASK;

                //
                // Original x86 code checked for virtual extensions.. 
                // Still being decided if these are worth the trouble...
                //
                if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                    flags |= EFLAGS_VIF;
                    if (! (flags & EFLAGS_INTERRUPT_MASK)) {
                        flags &= ~EFLAGS_VIF;
                    }
                }
                flags |= (EFLAGS_INTERRUPT_MASK | EFLAGS_V86_MASK);
                Frame->EFlags = flags;

                Frame->Eip += Regs.RiInstLength;

                return TRUE;

            case 0x0cd:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INTnn]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INTnn opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                if (VM86USER(Frame)) {
                    //
                    // V86 mode int XX
                    //

                    //
                    // Two EFLAGS to handle - one put in the trap frame
                    // and one that is pushed on the stack... First handle
                    // the one pushed on the stack
                    //

                    Frame->HardwareEsp = (Frame->HardwareEsp - 2) & 0xffff;
                    shortPtr = (PUSHORT) ((Frame->SegSs << 4) + Frame->HardwareEsp);
                    flags = Frame->EFlags;

                    if (! KeIA32VdmIoplAllowed) {
                        flags = (flags & ~EFLAGS_INTERRUPT_MASK) | (*VdmFixedStateLinear & (VDM_INTERRUPT_PENDING | EFLAGS_ALIGN_CHECK));
                    }

                    flags |= EFLAGS_IOPL_MASK;
                    *shortPtr = flags;

                    //
                    // Now handle the one on the trap frame...
                    //
                    flags = Frame->EFlags;

                    if (KeIA32VdmIoplAllowed) {
                        flags &= ~EFLAGS_INTERRUPT_MASK;
                    }
                    else {
                        KeIA32AndOrVdmLock(~VDM_VIRTUAL_INTERRUPTS, 0);
                        flags |= EFLAGS_INTERRUPT_MASK;
                    }

                    flags &= ~(EFLAGS_NT_MASK | EFLAGS_TF_MASK);
                    Frame->EFlags = flags;

                    // Now push CS on the stack
                    Frame->HardwareEsp = (Frame->HardwareEsp - 2) & 0xffff;
                    shortPtr = (PUSHORT) ((Frame->SegSs << 4) + Frame->HardwareEsp);
                    *shortPtr = Frame->SegCs;

                    // And push IP on the stack
                    Frame->HardwareEsp = (Frame->HardwareEsp - 2) & 0xffff;
                    shortPtr = (PUSHORT) ((Frame->SegSs << 4) + Frame->HardwareEsp);
                    *shortPtr = Frame->Eip;

                    //
                    // get the interrupt number and the address in memeory
                    // that it points to...
                    //
                    Regs.RiLinearAddr++;
                    longPtr = (PULONG) (*(Regs.RiLinearAddr) * 4);

                    //
                    // And set up the proper CS and IP based on the int number
                    // from above...
                    //
                    Frame->Eip = *longPtr & 0xffff;
                    Frame->SegCs = *longPtr >> 16;
                }
                else {
                    //
                    // protected mode intXX
                    //

                    //
                    // Get the eflags
                    //
                    flags = Frame->EFlags;
                    GetVirtualBits(&flags, Frame);
                    Regs.RiEFlags = flags;

                    //
                    // And get the stack
                    //
                    Regs.RiSegSs = Frame->SegSs;
                    Regs.RiEsp = Frame->HardwareEsp;
                    if (! SsToLinear(&Regs, VM86USER(Frame))) { 
                        return FALSE;
                    }

                    //
                    // Make sure that the nn part of INTnn
                    // can be accessed legitimately
                    //
                    Regs.RiEip++;
                    Regs.RiLinearAddr++;
                    if (! CheckEip(&Regs)) {
                        return FALSE;
                    }

                    //
                    // Point to the next instruction after the INTnn
                    //
                    Regs.RiEip++;
                    Regs.RiLinearAddr++;


                    //
                    // And put the right things on the stack...
                    //
                    if (! PushInt(*(Regs.RiLinearAddr - 1), &Regs)) {
                        return FALSE;
                    }

                    //
                    // If all went well, update the trap frame
                    //
// QUESTION
                    if (! KeIA32UnscrambleLdtEntry(Regs.RiSegCs, &NewXDescriptor)) {
                        return FALSE;
                    }

                    Frame->HardwareEsp = Regs.RiEsp;
                    Frame->EFlags = Regs.RiEFlags;
                    Frame->SegCs = Regs.RiSegCs;
                    Frame->Eip = Regs.RiEip;
                }

                return TRUE;

            case 0x0ce:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INTO]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INTO opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif

                //
                // Both v86 and protected mode don't want to 
                // deal with into...
                //
                return FALSE;
                
                break;

            case 0x0cf:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_IRET]++;
#endif
#if VDMDBG
                DbgPrintf("Saw IRET opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                //
                // This one should only happen in V86 mode
                //

                if (! VM86USER(Frame)) {
                    return FALSE;
                }


                //
                // update the Trap frame with the iret values (CS, IP, flags)
                // based on size of iret (16 vs. 32 bits)
                //
                if (Regs.RiPrefixFlags & PREFIX_OPER32) {
                    longPtr = (PULONG) ((Frame->SegSs << 4) + Frame->HardwareEsp);

                    Frame->Eip = *longPtr++;
                    Frame->SegCs = *longPtr++;
                    flags = *longPtr;

                    Frame->HardwareEsp = (Frame->HardwareEsp + 12) & 0xffff;
                }
                else {
                    shortPtr = (PUSHORT) ((Frame->SegSs << 4) + Frame->HardwareEsp);

                    Frame->Eip  = *shortPtr++ & 0xffff;
                    Frame->SegCs = *shortPtr++ & 0xffff;
                    flags = *shortPtr & 0xffff;

                    Frame->HardwareEsp = (Frame->HardwareEsp + 6) & 0xffff;
                }

                flags &= ~(EFLAGS_IOPL_MASK | EFLAGS_NT_MASK);

                KeIA32AndOrVdmLock(~VDM_VIRTUAL_INTERRUPTS, flags & EFLAGS_INTERRUPT_MASK );

                //
                // Original x86 code checked for virtual extensions.. 
                // We haven't decided if they are worth the effort yet...
                //
                if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                    if (flags & EFLAGS_INTERRUPT_MASK) {
                        flags |= EFLAGS_VIF;
                    }
                    else {
                        flags &= ~EFLAGS_VIF;
                    }
                }

                flags |= (EFLAGS_INTERRUPT_MASK | EFLAGS_V86_MASK);
                Frame->EFlags = flags;

                // Before we return, check for a BOP or a virtual interrupt
                shortPtr = (Frame->SegCs << 4) + Frame->Eip;

                if ((*shortPtr & 0xffff) == BOP_OPCODE) {
                    VdmDispatchBop(Frame);
                }
                else if ((*VdmFixedStateLinear & VDM_INTERRUPT_PENDING) &&
                                (*VdmFixedStateLinear & VDM_VIRTUAL_INTERRUPTS)) {
                    VdmDispatchIntAck(Frame);
                }

                return TRUE;

            case 0x0e4:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INBimm]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INB immediate opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // The immediate byte means the instruction is one byte longer
                Regs.RiInstLength++;

                // Check that the immediate byte address is legit
                charPtr = Regs.RiLinearAddr + 1;
                if ((charPtr - Regs.RiCsBase) > Regs.RiCsLimit) {
                    return FALSE;
                }

                IOPort = *charPtr;

                return(KiIA32VdmDispatchIo(IOPort, 1, TRUE, Regs.RiInstLength, Frame));

            case 0x0e5:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INWimm]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INW immediate opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // The immediate byte means the instruction is one byte longer
                Regs.RiInstLength++;

                // Check that the immediate byte address is legit
                charPtr = Regs.RiLinearAddr + 1;
                if ((charPtr - Regs.RiCsBase) > Regs.RiCsLimit) {
                    return FALSE;
                }

                IOPort = *charPtr;

                return(KiIA32VdmDispatchIo(IOPort, 2, TRUE, Regs.RiInstLength, Frame));
            case 0x0e6:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTBimm]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTB immediate opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // The immediate byte means the instruction is one byte longer
                Regs.RiInstLength++;

                // Check that the immediate byte address is legit
                charPtr = Regs.RiLinearAddr + 1;
                if ((charPtr - Regs.RiCsBase) > Regs.RiCsLimit) {
                    return FALSE;
                }

                IOPort = *charPtr;

                return(KiIA32VdmDispatchIo(IOPort, 1, FALSE, Regs.RiInstLength, Frame));
            case 0x0e7:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTWimm]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTW immediate opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // The immediate byte means the instruction is one byte longer
                Regs.RiInstLength++;

                // Check that the immediate byte address is legit
                charPtr = Regs.RiLinearAddr + 1;
                if ((charPtr - Regs.RiCsBase) > Regs.RiCsLimit) {
                    return FALSE;
                }

                IOPort = *charPtr;

                return(KiIA32VdmDispatchIo(IOPort, 2, FALSE, Regs.RiInstLength, Frame));

            case 0x0ec:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INB]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INB opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOPort = Frame->Edx;

                //
                // In V86 mode, handle Japan support for non PC/AT compatible machines
                //
#ifdef NON_PCAT
                if (VM86USER(Frame)) {
                    if (KeIA32MachineType & MACHINE_TYPE_MASK) {
                        return(KiIA32VdmDispatchIo(IOPort, 1, TRUE, Regs.RiInstLength, Frame));
                    }
                }
#endif
                // Check for printer status request...
                switch (IOPort) {
                    case 0x03bd:
                    case 0x0379:
                    case 0x0279:
                    if (VdmPrinterStatus(IOPort, Regs.RiInstLength, Frame)) {
                        return TRUE;
                    }
                    // Fall through
                    default:
                        return(KiIA32VdmDispatchIo(IOPort, 1, TRUE, Regs.RiInstLength, Frame));
                }
                // Should never get here...
                return FALSE;

            case 0x0ed:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_INW]++;
#endif
#if VDMDBG
                DbgPrintf("Saw INW opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOPort = Frame->Edx;
                return(KiIA32VdmDispatchIo(IOPort, 2, TRUE, Regs.RiInstLength, Frame));

            case 0x0ee:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTB]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTB opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOPort = Frame->Edx;

                // Check for printer status request...
                switch (IOPort) {
                    case 0x03bc:
                    case 0x0378:
                    case 0x0278:
                        if (VdmPrinterWriteData(IOPort, Regs.RiInstLength, Frame)) {
                            return TRUE;
                        }
                        //
                        // Fall through to dispatch if VdmPrinterWriteData() fails
                        //
                    default:
                        return(KiIA32VdmDispatchIo(IOPort, 1, FALSE, Regs.RiInstLength, Frame));
                }
                // Should never get here...
                return FALSE;

            case 0x0ef:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_OUTW]++;
#endif
#if VDMDBG
                DbgPrintf("Saw OUTW opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                IOPort = Frame->Edx;
                return(KiIA32VdmDispatchIo(IOPort, 2, FALSE, Regs.RiInstLength, Frame));

            case 0x0f0:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_LOCKPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw LOCK Prefix (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_LOCK;
                break;

            case 0x0f2:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_REPNEPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw REPNE Prefix (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_REPNE;
                break;

            case 0x0f3:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_REPPrefix]++;
#endif
#if VDMDBG
                DbgPrintf("Saw REP Prefix (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                Regs.RiPrefixFlags |= PREFIX_REP;
                break;

            case 0x0f4:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_HLT]++;
#endif
#if VDMDBG
                DbgPrintf("Saw HLT opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                // This one should only happen in V86 mode

                if (! VM86USER(Frame)) {
                    return FALSE;
                }

                // We are in V86 mode...

                //
                // Skip over the instruction
                //

                Frame->Eip = (Frame->Eip + Regs.RiInstLength) & 0xffff;

                return TRUE;

                break;

            case 0x0fa:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_CLI]++;
#endif
#if VDMDBG
                DbgPrintf("Saw CLI opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                if (! VM86USER(Frame)) {
                    flags = Frame->EFlags & ~EFLAGS_INTERRUPT_MASK;
                    SetVirtualBits(flags, &Regs);
                }
                else {
                    KeIA32AndOrVdmLock(~VDM_VIRTUAL_INTERRUPTS, 0);
                }
                
                // Skip over the instruction
                Frame->Eip += Regs.RiInstLength;

                return TRUE;

            case 0x0fb:
#if DEVL
                ExVdmOpcodeDispatchCounts[VDM_INDEX_STI]++;
#endif
#if VDMDBG
                DbgPrintf("Saw STI opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                if (! VM86USER(Frame)) {
                    flags = Frame->EFlags | EFLAGS_INTERRUPT_MASK;
                    SetVirtualBits(flags, &Regs);
                }
                else {
                    // Original x86 code checked for virtual extensions.. 
                    // We haven't decided if they're worth it or not...
                    if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                        Frame->EFlags |= EFLAGS_VIF;
                    }
                        
                    KeIA32AndOrVdmLock(0xffffffff, EFLAGS_INTERRUPT_MASK);
                }

                // Skip over the instruction
                Frame->Eip += Regs.RiInstLength;

                // Interrupts are now "enabled", so handle any
                // that are pending
                if (*VdmFixedStateLinear & VDM_INTERRUPT_PENDING) {
                    VdmDispatchIntAck(Frame);
                }
                return TRUE;

            default:
                // Opcode Invalid
#if VDMDBG
                DbgPrintf("Saw unexpected opcode (0x%02x) at linear address 0x%08lx\n", (int) Regs.RiOpcode, (long) Regs.RiLinearAddr);
#endif
                return FALSE;

        }

        // If we get here, it was a prefix instruction

        Regs.RiLinearAddr++;
        Regs.RiInstLength++;
    }

    // Prefix was too long, so allow a GP fault

    return (FALSE);
}


VOID
VdmDispatchIntAck(
    IN PKIA32_FRAME Frame
    )

/*++

Routine Description: 

    Pushes stack arguments for VdmDispatchInterrupts
    and invokes VdmDispatchInterrupts

    Expects VDM_INTERRUPT_PENDING, and VDM_VIRTUAL_INTERRUPTS

Arguments:

   Frame - pointer to the ia32 trap frame

Returns:

  nothing

--*/
{
    PVDM_TIB VdmTib;

    VdmTib = (PsGetCurrentThread()->Tcb.Teb)->Vdm;
    
    if (*VdmFixedStateLinear & VDM_INT_HARDWARE) {
        // Dispatch interrupt directly from kernel

        VdmDispatchInterrupts(Frame, VdmTib);
    }
    else {
        // Dispatch interrupt via monitor

        VdmTib->EventInfo.Event = VdmIntAck;
        VdmTib->EventInfo.Size = 0;
        VdmTib->EventInfo.IntAckInfo = 0;
        VdmEndExecution(Frame, VdmTib);
    }
}

//
// These are only used by the GetVirtualBits() routine
//
#define _VDM_EXTOFF_FUNC(x) (x = (x & ~EFLAGS_INTERRUPT_MASK) | (*VdmFixedStateLinear & (VDM_VIRTUAL_INTERRUPTS | EFLAGS_ALIGN_CHECK)) | EFLAGS_IOPL_MASK)
#define _VDM_EXTON_FUNC(x)  (x = (x & ~EFLAGS_INTERRUPT_MASK) | (*VdmFixedStateLinear & (VDM_VIRTUAL_INTERRUPTS | EFLAGS_ALIGN_CHECK)) | EFLAGS_IOPL_MASK)
#define _VDM_IOPL_FUNC(x)   (x = (x & ~EFLAGS_ALIGN_CHECK) | (*VdmFixedStateLinear & EFLAGS_ALIGN_CHECK) | EFLAGS_IOPL_MASK)


VOID
GetVirtualBits(
    IN OUT PULONG PFlags,
    IN PKIA32_FRAME Frame
    )

/*++

Routine Description: 

    This routine correctly gets the VDMs virtual interrupt flag
    and puts it into an EFlags image

Arguments:

   PFlags - pointer to the Eflags
   Frame - pointer to the ia32 trap frame

Returns:

  nothing

--*/
{
    // We need to decide if we want this or not...
    if (KeIA32VdmIoplAllowed) {
        if (*PFlags & EFLAGS_V86_MASK) {
            _VDM_IOPL_FUNC(*PFlags);
        }
        else {
            _VDM_EXTOFF_FUNC(*PFlags);
        }
    }
    else {

        //
        // A check was made for virtual extensions. We don't know if
        // we want to implement those extensions or not, so...
        //

        if (KeIA32VirtualIntExtensions & (V86_VIRTUAL_INT_EXTENSIONS | PM_VIRTUAL_INT_EXTENSIONS)) {
            if (*PFlags & EFLAGS_V86_MASK) {
                if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                    _VDM_EXTON_FUNC(*PFlags);
                }
                else {
                    _VDM_EXTOFF_FUNC(*PFlags);
                }
            }
            else {
                if (KeIA32VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
                    _VDM_EXTON_FUNC(*PFlags);
                }
                else {
                    _VDM_EXTOFF_FUNC(*PFlags);
                }
            }
        }
        else {
            _VDM_EXTOFF_FUNC(*PFlags);
        }
    }
}

VOID
SetVirtualBits(
    IN ULONG Flags,
    IN PREGINFO PRegs
    )

/*++

Routine Description: 

    This routine correctly sets the VDMs virtual interrupt flag

Arguments:

   Flags - the EFlags value 
   PRegs - pointer to instruction information (prefix, inst length, etc.)

Returns:

  nothing

--*/
{
    KeIA32AndOrVdmLock(~VDM_VIRTUAL_INTERRUPTS, Flags & EFLAGS_INTERRUPT_MASK);

    //
    // There was a lot of code here to check for Virtual Extensions...
    // Depending on if they existed or not, a value in eax was changed
    // (in ../i386/instemul.asm - at label svb50: ) but it is never used
    // (not even as a return value...)
    // So, rather than repeat all that silly code that doesn't do anything,
    // I only show the code that is actually executed...  Enjoy...
    //
  
#if 0
    if (KeIA32VirtualIntExtensions & (V86_VIRTUAL_INT_EXTENSIONS | PM_VIRTUAL_INT_EXTENSIONS)) {
        if (Flags & EFLAGS_V86_MASK) {
            if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
            }
            else {
            }
        }
        else {
            if (KeIA32VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
            }
            else {
            }
        }
    }
    else {
    }
#endif

    if (PRegs->RiPrefixFlags & PREFIX_OPER32) {
        KeIA32AndOrVdmLock(~EFLAGS_ALIGN_CHECK, Flags & EFLAGS_ALIGN_CHECK);
    }
}


BOOLEAN
KiIA32VdmReflectException(
    IN OUT PKIA32_FRAME Frame,
    IN ULONG Code
    )
/*++

Routine Description:

    This routine reflects an exception to a VDM.  It uses the information
    in the trap frame to determine what exception to reflect, and updates
    the trap frame with the new CS, EIP, SS, and SP values

Arguments:

   Frame - Pointer to the IA32 trap frame
   Code - The trap number that brought us here

Returns

    Nothing

Notes:
    Interrupts  are enable upon entry, Irql is at APC level
    This routine may not preserve all of the non-volatile registers if
    a fault occurs.
--*/
{
    REGINFO Regs;
    KXDESCRIPTOR NewXCsDescriptor, NewXSsDescriptor;

    //
    // First see if it is a GP fault and if we are set up to catch them
    //
    if ((Code == EXCEPTION_GP_FAULT) && (*VdmFixedStateLinear & VDM_BREAK_EXCEPTIONS)) {
        VdmDispatchException(Frame, STATUS_ACCESS_VIOLATION, Frame->Eip, 2, 0, -1,  0);
        return TRUE;
    }

    //
    // Then see if this is a debug exception
    //
    if (*VdmFixedStateLinear & VDM_BREAK_DEBUGGER) {
        if (Code == EXCEPTION_DEBUG) {
            //
            // BUGBUG: iVE doesn't respond to TF, so need to reset
            // it elsewhere as well
            // 
            Frame->EFlags &= ~EFLAGS_TF_MASK;
            VdmDispatchException(Frame, STATUS_SINGLE_STEP, Frame->Eip, 0, 0, 0,  0);
            return TRUE;
        }
        if (Code == EXCEPTION_INT3) {
            VdmDispatchException(Frame, STATUS_BREAKPOINT, Frame->Eip - 1, 3, USER_BREAKPOINT, 0,  0);

            return TRUE;
        }
    }

    //
    // Don't really know why this is here... But it is...
    //
    if (FLATUSER(Frame)) {
        return TRUE;
    }


#if DEVL
    if (Code == EXCEPTION_SEGMENT_NOT_PRESENT) {
        ExVdmSegmentNotPresent++;
    }
#endif

    //
    // Allow for fiddling of the stack frame later
    //

    Regs.RiTrapFrame = Frame;
    Regs.RiSegSs = Frame->SegSs;
    Regs.RiEsp = Frame->HardwareEsp;
    Regs.RiEFlags = Frame->EFlags;
    Regs.RiEip = Frame->Eip;
    Regs.RiSegCs = Frame->SegCs;

    if (! CsToLinear(&Regs, VM86USER(Frame))) {
        return FALSE;
    }

    if (! SsToLinear(&Regs, VM86USER(Frame))) {
        return FALSE;
    }

    if (! PushException(Code, &Regs)) {
        return FALSE;
    }

    //
    // For EM, when we change any segment register, we
    // need to change the descriptor as well.. PushException()
    // may have changed CS and SS, so need to update their descriptors
    //
    if (Regs.RiEFlags & EFLAGS_V86_MASK) {
// QUESTION
        NewXCsDescriptor.DescriptorWords = (ULONGLONG) (Regs.RiSegCs << 4);
        NewXSsDescriptor.DescriptorWords = (ULONGLONG) (Regs.RiSegSs << 4);
    }
    else {
        if (! KeIA32UnscrambleLdtEntry(Regs.RiSegCs, &NewXCsDescriptor)) {
            return FALSE;
        }

        if (! KeIA32UnscrambleLdtEntry(Regs.RiSegSs, &NewXSsDescriptor)) {
            return FALSE;
        }
    }

    Frame->HardwareEsp = Regs.RiEsp;

    //
    // Ss touched in PushException()...
    //
    Frame->SegSs = Regs.RiSegSs;
    Frame->EFlags = Regs.RiEFlags;

    //
    // Cs touched in PushException()...
    //
    Frame->SegCs = Regs.RiSegCs;
    Frame->Eip = Regs.RiEip;

    if (Code == EXCEPTION_DEBUG) {
        //
        // BUGBUG: iVE doesn't respond to TF, so need to reset
        // it elsewhere as well
        // 
        Frame->EFlags &= ~EFLAGS_TF_MASK;
    }

    return TRUE;
}


BOOLEAN
KiIA32VdmSegmentNotPresent(
    IN OUT PKIA32_FRAME Frame
    )
/*++

Routine Description:

    This routine reflects a TRAP 0x0B to a VDM. It uses information
    in the trap frame to determine what exception to reflect
    and updates the trap frame with the new CS, EIP, SS, and ESP values

Arguments:

    Frame - Pointer to the IA32 trap frame

Returns

    True if the reflection was successful, false otherwise

Notes:
    none
--*/
{
    PVDM_TIB VdmTib;
    PVDM_FAULTHANDLER NoSegFault;
    ULONG Flags;
    KXDESCRIPTOR NewXCsDescriptor;
    KXDESCRIPTOR NewXSsDescriptor;
    ULONG Offset;

    //
    // Get the Segment Not Present handler information
    //
    VdmTib = (PVDM_TIB) (PsGetCurrentThread()->Tcb.Teb)->Vdm;

    //
    // If not switching stacks, let regular reflection code handle this...
    //
    if (VdmTib->PmStackInfo.LockCount != 0) {
        return(KiIA32VdmReflectException(Frame, EXCEPTION_SEGMENT_NOT_PRESENT));
    }

    NoSegFault = &(VdmTib->VdmFaultHandlers[EXCEPTION_SEGMENT_NOT_PRESENT]);

#if DEVL
    ExVdmSegmentNotPresent++;
#endif

    //
    // Just like SwitchToStack() if there had been one...
    //
    VdmTib->PmStackInfo.LockCount++;
    VdmTib->PmStackInfo.SaveEip = Frame->Eip;
    VdmTib->PmStackInfo.SaveEsp = Frame->HardwareEsp;
    VdmTib->PmStackInfo.SaveSsSelector = Frame->SegSs;

    //
    // We'll need to update the Frame with the proper descriptors,
    // so get them and unscramble them now...
    //
    if (! KeIA32UnscrambleLdtEntry(NoSegFault->CsSelector, &NewXCsDescriptor)) {
        return FALSE;
    }

    if (! KeIA32UnscrambleLdtEntry(VdmTib->PmStackInfo.SsSelector, &NewXSsDescriptor)) {
        return FALSE;
    }

    //
    // Regardless of 16/32 bit handler, still need the flags
    //
    Flags = Frame->EFlags;
    GetVirtualBits(&Flags, Frame);

    //
    // See if this is a 32-bit handler
    //
    if (VdmTib->PmStackInfo.Flags & 1) {
        PULONG ExcptStack32;

        //
        // Now make ExcptStack points to the actual stack space...
        //
        ExcptStack32 = (PULONG) (NewXSsDescriptor.Words.Bits.Base + DPMISTACK_OFFSET);

        //
        // And put normal fault arguments on the stack...
        //
// QUESTION
        *--ExcptStack32 = Frame->SegSs;
        *--ExcptStack32 = Frame->HardwareEsp;
        *--ExcptStack32 = Flags;

        *--ExcptStack32 = Frame->SegCs;
        *--ExcptStack32 = Frame->Eip;
        *--ExcptStack32 = Frame->ISRCode;

        //
        // And push dosx iret segment and offset
        //
        *--ExcptStack32 = VdmTib->PmStackInfo.DosxFaultIretD >> 16;
        *--ExcptStack32 = VdmTib->PmStackInfo.DosxFaultIretD &  0x0ffff;

        Offset = ((PUCHAR) ExcptStack32) - NewXSsDescriptor.Words.Bits.Base;
    }
    else {
        PUSHORT ExcptStack16;

        //
        // Now make ExcptStack points to the actual stack space...
        //
        ExcptStack16 = NewXSsDescriptor.Words.Bits.Base + DPMISTACK_OFFSET;

        //
        // And put normal fault arguments on the stack...
        //
        *--ExcptStack16 = Frame->SegSs;
        *--ExcptStack16 = Frame->HardwareEsp;
        *--ExcptStack16 = Flags;

        *--ExcptStack16 = Frame->SegCs;
        *--ExcptStack16 = Frame->Eip;
        *--ExcptStack16 = Frame->ISRCode;

        //
        // And push dosx iret segment and offset
        //
        *--ExcptStack16 = VdmTib->PmStackInfo.DosxFaultIretD >> 16;
        *--ExcptStack16 = VdmTib->PmStackInfo.DosxFaultIretD &  0x0ffff;
        Offset = ((PUCHAR) ExcptStack16) - NewXSsDescriptor.Words.Bits.Base;
    }

    if (NoSegFault->Flags & VDM_INT_INT_GATE) {
        KeIA32AndOrVdmLock(~VDM_VIRTUAL_INTERRUPTS, 0);
        Frame->EFlags &= ~EFLAGS_VIF;
    }

    Frame->SegCs = NoSegFault->CsSelector;
    Frame->Eip = NoSegFault->Eip;
    Frame->SegSs = VdmTib->PmStackInfo.SsSelector;
    Frame->HardwareEsp = Offset;

    return TRUE;
}


VOID
VdmDispatchException(
    IN PKIA32_FRAME Frame,
    IN NTSTATUS ExcepCode,
    IN PVOID ExcepAddr,
    IN ULONG NumParms,
    IN ULONG Parm1,
    IN ULONG Parm2,
    IN ULONG Parm3
    )

/*++

Routine Description: 

    Dispatches an exception for the VDM to the kernel by invoking the
    KiIA32DispatchException() routine.

Arguments:

    See KiIA32DispatchException()

Returns:

    nothing

--*/
{
    //
    // Lower the IRQL to 0 and give any APC's and Debuggers a chance
    //
// QUESTION
    KeLowerIrql(0);

    //
        // Might as well make sure things are legal...
    //
    ASSERT(NumParms <= 3);

    //
    // This was a call to CommonDispatchException() but that routine
    // doesn't exist in EM, so trying to call the nearest available...
    //
    KiIA32ExceptionDispatch(Frame, ExcepCode, ExcepAddr, NumParms, Parm1, Parm2, Parm3);

        //
    // Should Never get here... KiIA32ExceptionDispatch() never returns...
        //
}


BOOLEAN
PushInt(IN ULONG InterruptNumber,
        IN OUT PREGINFO Regs
        )
/*++

Routine Description:

    This routine pushes an interrupt frame on the user stack

Arguments:

   InterruptNumber - self explanitory...
   Regs - Pointer to a REGINFO structure

Returns:

    True if stack successfully modified and Regs updated, otherwise false

Notes:

    None
--*/
{
    PVDM_TIB VdmTib;
    PVDM_INTERRUPTHANDLER IntHandler;
    PULONG ExcptStack32;
    PUSHORT ExcptStack16;
    ULONG Flags;

    ASSERT(InterruptNumber < 256);

    VdmTib = (PVDM_TIB) (PsGetCurrentThread()->Tcb.Teb)->Vdm;

    IntHandler = &(VdmTib->VdmInterruptHandlers[InterruptNumber]);

        //
        // If we are a small segment, make sure the ESP is 16 bits.
        //
        if (! Regs->RiSsDescriptor.Words.Bits.Default_Big) {
                Regs->RiEsp &= 0xffff;
        }

        //
        // Make sure there is space on the stack
        //
        if (IntHandler->Flags & VDM_INT_32) {
                if (Regs->RiEsp <= (3 * sizeof(PVOID))) {
                        return FALSE;
                }

                Regs->RiEsp -= 3 * sizeof(PVOID);
        }
        else {
                if (Regs->RiEsp <= (3 * sizeof(SHORT))) {
                        return FALSE;
                }

                Regs->RiEsp -= 3 * sizeof(SHORT);
        }

        //
        // Check that esp is still OK
        //
        if (Regs->RiSsDescriptor.Words.Bits.Type & DESCRIPTOR_EXPAND_DOWN) {
                if (Regs->RiEsp <= Regs->RiSsLimit) {
                        return FALSE;
                }
        }
        else {
                if (Regs->RiEsp >= Regs->RiSsLimit) {
                        return FALSE;
                }
        }

        //
        // Push the iret frame
        //

        //
        // Get the modified EFlags
        //
        Flags = Regs->RiEFlags;
        GetVirtualBits(&Flags, Regs->RiTrapFrame);

        if (IntHandler->Flags & VDM_INT_32) {
                ExcptStack32 = Regs->RiSsBase + Regs->RiEsp;

                //
                // And put arguments on the stack...
                //
                *ExcptStack32++ = Regs->RiEip;
                *ExcptStack32++ = Regs->RiSegCs;
                *ExcptStack32 = Flags;
        }
        else {
                ExcptStack16 = Regs->RiSsBase + Regs->RiEsp;

                //
                // And put arguments on the stack...
                //
                *ExcptStack16++ = Regs->RiEip;
                *ExcptStack16++ = Regs->RiSegCs;
                *ExcptStack16 = Flags;
        }

        //
        // Make sure the address pointed to by CS exists
        //
        Regs.RiSegCs = IntHandler->CsSelector;
        Regs.RiEip = IntHandler->Eip;

        if (CsToLinear(&Regs, ! (IntHandler->Flags & VDM_INT_32))) {
                if (! Regs->RiCsDescriptor.Words.Bits.Pres) {
                        return FALSE;
                }
        }
        else {
                //
                // This is a little strange... CsToLinear fails if either the
                // type of the descriptor is wrong, or if the limit is bad...
                // Only reject the INT handler if the limit is bad...
                //

                if (Regs.RiEip >= Regs.RiCsLimit) {
                        return FALSE;
                }
        }

        Regs.RiEFlags &= ~EFLAGS_TF_MASK;

        if (IntHandler->Flags & VDM_INT_INT_GATE) {
                if (KeIA32VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
                        Regs->RiEFlags &= ~EFLAGS_VIF;
                }
                KeIA32AndOrVdmLock(~EFLAGS_INTERRUPT_MASK, 0);
        }

        Regs.RiEFlags &= ~(EFLAGS_IOPL_MASK | EFLAGS_NT_MASK | EFLAGS_V86_MASK);
        Regs.RiEFlags |= EFLAGS_INTERRUPT_MASK;

        return TRUE;
}


BOOLEAN
PushException(
        IN ULONG ExceptionNumber,
        IN OUT PREGINFO Regs
        )
/*++

Routine Description:

    This routine pushes an exception onto the user stack

Arguments:

   ExceptionNumber - self explanitory...
   Regs - Pointer to a REGINFO structure

Returns:

    True if stack successfully modified and Regs updated, otherwise false

Notes:

    None
--*/
{
    PVDM_TIB VdmTib;
    PVDM_FAULTHANDLER ExceptHandler;
    ULONG Flags;

    ASSERT(ExceptionNumber < 32);

    VdmTib = (PVDM_TIB) (PsGetCurrentThread()->Tcb.Teb)->Vdm;
    ExceptHandler = &(VdmTib->VdmFaultHandlers[ExceptionNumber]);

    if (Regs->RiEFlags & EFLAGS_V86_MASK) {
        PUSHORT ExcptStack16;

        // We were in V86 mode when exception happened...

        //
        // device not available fault... Per win3.1, no exceptions
        // above 7 for v86 mode
        //
        if (ExceptionNumber > 7) {
            return FALSE;
        }

        //
        // And push things on the stack for V86 mode
        // making sure to handle wrap-around each time...
        //
        Regs->RiEsp = ((Regs->RiEsp - 2) & 0xffff);
        ExcptStack16 = Regs->RiSsBase + Regs->RiEsp;
        Flags = Regs->RiEFlags;
        GetVirtualBits(&Flags, Regs->RiTrapFrame);
        *ExcptStack16 = Flags;

        Regs->RiEsp = ((Regs->RiEsp - 2) & 0xffff);
        ExcptStack16 = Regs->RiSsBase + Regs->RiEsp;
        *ExcptStack16 = Regs->RiSegCs;

        Regs->RiEsp = ((Regs->RiEsp - 2) & 0xffff);
        ExcptStack16 = Regs->RiSsBase + Regs->RiEsp;
        *ExcptStack16 = Regs->RiEip;
    }
    else {
        // We were in protected mode when the exception happened...

        //
        // This was originally part of a seperate routine
        // called SwitchToHandlerStack(), but it was never called
        // anywhere else, so now it's inline...
        //

        //
        // Have we already switched stacks?
        //
        if (VdmTib->PmStackInfo.LockCount == 0) {
            VdmTib->PmStackInfo.SaveEip = Regs.RiEip;
            VdmTib->PmStackInfo.SaveEsp = Regs.RiEsp;
            VdmTib->PmStackInfo.SaveSsSelector = Regs.RiSegSs;

            Regs->RiSegSs = VdmTib->PmStackInfo.SsSelector;
            Regs->RiEsp = DPMISTACK_EXCEPTION_OFFSET;

            //
            // And update and verify the Regs structure
            //
            if (! SsToLinear(Regs, FALSE)) {
                return FALSE;
            }

            VdmTib->PmStackInfo.LockCount++;
        }

        //
        // Windows 3.1 undocumented feature
        //
        Regs->RiEsp -= 0x20;

        if (! Regs->RiSsDescriptor.Words.Bits.Default_Big) {
            //
            // If we are not "big" then the ESP should stay 16 bit
            //
            Regs->RiEsp &= 0xffff;
        }

        Flags = Regs->RiEFlags;
        GetVirtualBits(&Flags, Regs->RiTrapFrame);

        if (VdmTib->PmStackInfo.Flags) {
            PULONG ExcptStack32;

            //
            // This is a 32-bit handler
            //

            if (! CheckEsp(Regs, 32)) {
                return FALSE;
            }

            ExcptStack32 = Regs.RiSsBase + Regs.RiEsp;
            *--ExcptStack32 = VdmTib->PmStackInfo.SaveSsSelector;
            *--ExcptStack32 = VdmTib->PmStackInfo.SaveEsp;
            *--ExcptStack32 = Flags;
            *--ExcptStack32 = Regs->RiSegCs;
            *--ExcptStack32 = VdmTib->PmStackInfo.SaveEip;
            *--ExcptStack32 = Regs->RiTrapFrame->ISRCode;

            //
            // And push dosx iret segment and offset
            //
            *--ExcptStack32 = VdmTib->PmStackInfo.DosxFaultIretD >> 16;
            *--ExcptStack32 = VdmTib->PmStackInfo.DosxFaultIretD &  0x0ffff;

// QUESTION
            Regs.RiEsp = ExcptStack32 - Regs.RiSsBase;

        }
        else {
            PUSHORT ExcptStack16;

            //
            // Do the 16 bit handler thing...
            //

            if (! CheckEsp(Regs, 16)) {
                return FALSE;
            }

            ExcptStack16 = Regs.RiSsBase + Regs.RiEsp;
            *--ExcptStack16 = VdmTib->PmStackInfo.SaveSsSelector;
            *--ExcptStack16 = VdmTib->PmStackInfo.SaveEsp;
            *--ExcptStack16 = Flags;
            *--ExcptStack16 = Regs->RiSegCs;
            *--ExcptStack16 = VdmTib->PmStackInfo.SaveEip;
            *--ExcptStack16 = Regs->RiTrapFrame->ISRCode;

            //
            // And push dosx iret segment and offset
            //
            *--ExcptStack16 = VdmTib->PmStackInfo.DosxFaultIretD >> 16;
            *--ExcptStack16 = VdmTib->PmStackInfo.DosxFaultIretD &  0x0ffff;

            Regs->RiEsp = ExcptStack16 - Regs.RiSsBase;
        }
    }

    //
    // Make sure the address pointed to by CS exists
    //
    Regs.RiSegCs = ExceptHandler->CsSelector;
    Regs.RiEip = ExceptHandler->Eip;

    if (! CsToLinear(&Regs, Regs->RiEFlags & EFLAGS_V86_MASK)) {
        return FALSE;
    }

    //
    // Strange, they do this test twice  - it is part of CsToLinear()
    //
    // if (Regs.RiEip > Regs.RiCsLimit) {
    //      return FALSE;
    // }

    if (ExceptHandler->Flags & VDM_INT_INT_GATE) {
        SetVirtualBits(Regs->RiEFlags & ~(EFLAGS_INTERRUPT_MASK | EFLAGS_TF_MASK), &Regs);
    }

    CheckVdmFlags(&Regs);

    Regs.RiEFlags &= ~EFLAGS_TF_MASK;

    return TRUE;
}


BOOLEAN
CsToLinear(
        IN OUT PREGINFO Regs,
        IN BOOLEAN IsV86
        )
/*++

Routine Description:

    This routine converts the address shown in CS and IP into a linear
    address based on the mode (V86 vs. protected)

Arguments:

    Regs - Pointer to a REGINFO structure
    IsV86 - True if the CS and IP should be checked against V86 constraints

Returns:

    True if Regs->RiLinearAddr is valid, otherwise false

Notes:

    None
--*/
{
    if (IsV86) {
        //
        // Don't need the Descriptor in V86 mode
        //
        Regs->RiCsBase = Regs->RiSegCs << 4;
// QUESTION
        Regs->RiCsDescriptor.DescriptorWords = (ULONGLONG) Regs->RiCsBase;
        Regs->RiCsLimit = 0xffff;
    }
    else {

        if (! KeIA32UnscrambleLdtEntry(Regs->RiSegCs, &Regs->RiCsDescriptor)) {
            return FALSE;
        }

        //
        // Need to fill in the cs stuff to make translation to linear
        // addresses easier
        //
        Regs->RiCsBase = (ULONG) Regs->RiCsDescriptor.Words.Bits.Base;
        Regs->RiCsLimit = (ULONG) Regs->RiCsDescriptor.Words.Bits.Limit;

        //
        // Check for the big limits
        //
        if (Regs->RiCsDescriptor.Words.Bits.Granularity) {
                Regs->RiCsLimit <<= 12;
                Regs->RiCsLimit |= 0xfff;
        }

        //
        // Make sure the cs segment is executable code...
        // (see Ki386GetSelectorParameters()...)
        //
        // Check is done here because the other stuff should be done even
        // if this check fails
        //
        if ((Regs->RiCsDescriptor.Words.Bits.Type & TYPE_CODE_USER ) != 
             TYPE_CODE_USER) {
             return FALSE;
        }
    }

    //
    // Now make sure the ip is in the correct range
    //
    if (Regs->RiEip > Regs->RiCsLimit) {
        return FALSE;
    }

    //
    // Finally, generate the linear address
    //
    Regs->RiLinearAddr = Regs->RiEip + Regs->RiCsBase;

    return TRUE;
}


BOOLEAN
CheckEip(
        IN PREGINFO Regs
        )
/*++

Routine Description:

    This routine verifies the EIP is legal before putting it back into
    a trap frame.

Arguments:

   Regs - Pointer to a REGINFO structure

Returns:

    True if the EIP is legal, false otherwise

Notes:

--*/
{
    if (Regs->RiEFlags & EFLAGS_V86_MASK) {
        Regs->RiEip &= Regs->RiCsLimit;
    }
    else {
        if (Regs->RiEip > Regs->RiCsLimit) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
SsToLinear(
        IN OUT PREGINFO Regs,
        IN BOOLEAN IsV86
        )
/*++

Routine Description:

    This routine converts the SS selector given in the regs structure
    into a base and limit based on the mode (V86 vs. protected)


Arguments:

    Regs - Pointer to a REGINFO structure
    IsV86 - True if the Base and Limit should be set using V86 constraints

Returns:

    True if Regs->RiSsBase and Regs->RiSsLimit are valid, false otherwise

Notes:

    Unlike FastCsToLinear(), this routine does not verify that the user of
    the segment (in this case, ESP) is actually within the proper limits
--*/
{
    if (IsV86) {
        //
        // Don't need the descriptor in this case
        //
        Regs->RiSsBase = Regs->RiSegSs << 4;
        Regs->RiSsDescriptor.DescriptorWords = (ULONGLONG) Regs->RiSsBase;
        Regs->RiSsLimit = 0xffff;
    }
    else {
        if (! KeIA32UnscrambleLdtEntry(Regs->RiSegSs, &Regs->RiSsDescriptor)) {
            return FALSE;
        }

        //
        // Make sure the ss segment is not executable code but is read/write
        // (see Ki386GetSelectorParameters()...)
        //
        if ((Regs->RiSsDescriptor.Words.Bits.Type & DESCRIPTOR_DATA_READWRITE) !=
            DESCRIPTOR_DATA_READWRITE ) {
            return FALSE;
        }

        //
        // Now fill in the ss stuff...
        //
        Regs->RiSsBase = (ULONG) (Regs->RiSsDescriptor.Words.Bits.Base);
        Regs->RiSsLimit = (ULONG) (Regs->RiSsDescriptor.Words.Bits.Limit);

        //
        // Check for the big limits...
        //
        if (Regs->RiSsDescriptor.Words.Bits.Default_Big) {
                Regs->RiSsLimit <<= 12;
                Regs->RiSsLimit |= 0xfff;
        }
    }

    return TRUE;
}


BOOLEAN
CheckEsp(
        IN PREGINFO Regs,
        IN ULONG StackNeeded
        )
/*++

Routine Description:

    This routine verifies the ESP is legal before putting it back into
    a trap frame.

Arguments:

   Regs - Pointer to a REGINFO structure
   StackNeeded - Need to be this far away from stack limit

Returns:

    True if the ESP is legal (and there is available space on the stack),
    false otherwise

Notes:

--*/
{
    if (Regs->RiEFlags & EFLAGS_V86_MASK) {
        Regs->RiEsp &= Regs->RiSsLimit;
    }
    else {
        if (! Regs->RiSsDescriptor.Words.Bits.Default_Big) {
            //
            // If we are not "big" then the ESP should stay 16 bit
            //
            Regs->RiEsp &= 0xffff;
        }

        //
        // If we are expand up or expand down, this is a problem...
        //
        if (StackNeeded > Regs->RiEsp) {
            return FALSE;
        }

        if ((Regs->RiSsDescriptor.Words.Bits.Type & DESCRIPTOR_EXPAND_DOWN) ==
            DESCRIPTOR_EXPAND_DOWN ) {
            if (Regs->RiEsp - StackNeeded - 1 < Regs->RiSsLimit) {
                return FALSE;
            }
        }
        else { 
            if (Regs->RiEsp >= Regs->RiSsLimit) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOLEAN
KeIA32UnscrambleLdtEntry(
        IN ULONG Selector,
        OUT PKXDESCRIPTOR XDescriptor
        )
/*++

Routine Description:

    This routine gets the requested LDT entry and converts it into
    the unscrambled format needed by the EM descriptor registers

Arguments:

   Selector - selector number to use
   XDescriptor - unscrambled descriptor

Returns:

    True if the the selector was found, false otherwise

Notes:

--*/
{
    PETHREAD Thread;
    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    NTSTATUS Status;

    //
    // We only deal with LDT, so make sure that's what we got...
    //
    if ((Selector & (SELECTOR_TABLE_INDEX | DPL_USER)) != (SELECTOR_TABLE_INDEX | DPL_USER)) {
    
        return FALSE;
    }

    Thread = KeGetCurrentThread();
    DescriptorEntry.Selector = Selector;

    Status = PspQueryDescriptorThread(Thread, &DescriptorEntry, sizeof(DescriptorEntry), NULL);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    XDescriptor->Words.DescriptorWords = KeIA32Unscramble(&DescriptorEntry.Descriptor);

    return TRUE;
}


ULONGLONG
KeIA32Unscramble(
        IN PLDT_ENTRY Descriptor
        )
/*++

Routine Description:

    This routine converts from the scrambled format used by memory
    into the unscrambled format used by the iVE. Assumes that the following
    flags are a constant:
         the DPL is always 3,
         the accessed bit is always 1,
         and the system bit is always 0

Arguments:

   Descriptor - scrambled descriptor

Returns:

    An unscrambled descriptor

Notes:

--*/
{
    KXDESCRIPTOR Result;

    //
    // Fill in the base
    //
    Result.Words.Bits.Base = Descriptor->BaseLow | (Descriptor->HighWord.Bytes.BaseMid << 16) | (Descriptor->HighWord.Bytes.BaseHi << 24);

    //
    // Fill in the limit
    //
    Result.Words.Bits.Limit = (Descriptor->LimitLow | (Descriptor->HighWord.Bits.LimitHi << 16)) << UNSCRAM_LIMIT_OFFSET;

    //
    // Fill in the flags - since the DPL is always 3, the accessed bit
    // is always 1, and the system bit is always 0, use a constant for
    // those values (0x61)...
    //
    Result.Words.Bits.Type = Descriptor->HighWord.Bits.Type;
    Result.Words.Bits.Dpl = Descriptor->HighWord.Bits.Dpl;
    Result.Words.Bits.Pres = Descriptor->HighWord.Bits.Pres;
    Result.Words.Bits.Sys = Descriptor->HighWord.Bits.Sys;
    Result.Words.Bits.Reserved_0 = Descriptor->HighWord.Bits.Reserved_0;
    Result.Words.Bits.Default_Big = Descriptor->HighWord.Bits.Default_Big;
    Result.Words.Bits.Granularity = Descriptor->HighWord.Bits.Granularity;

    return Result.Words.DescriptorWords;
}

//
// These definitions are only used in the CheckVdmFlags()
// routine below.
//
#define _VDM_CHECK_OR_FUNC(x) ((x) |= EFLAGS_INTERRUPT_MASK)
#define _VDM_CHECK_AND_FUNC(x) ((x) &= ~(EFLAGS_IOPL_MASK | EFLAGS_NT_MASK | EFLAGS_VIF | EFLAGS_VIP))


VOID
CheckVdmFlags(
    IN OUT PREGINFO Regs
    )

/*++

Routine Description: 

    This routine checks the flags that are going to be used for the
    dos or windows application

Arguments:

   Regs - Pointer to REGINFO structure...

Returns:

   Nothing. But modifies EFlags field in Regs to an acceptable value

--*/
{
    if (KeIA32VdmIoplAllowed) {
        if (! (Regs->RiEFlags & EFLAGS_V86_MASK)) {
            _VDM_CHECK_OR_FUNC(Regs->RiEFlags);
        }
        _VDM_CHECK_AND_FUNC(Regs->RiEFlags);
        return;
    }

    //
    // There was a lot of code here to check for Virtual Extensions...
    // Depending on if they existed or not, a value in eax was changed
    // (in ../i386/instemul.asm - at label cvf50: ) but it is never used
    // (not even as a return value...)
    // So, rather than repeat all that silly code that doesn't do anything,
    // I only show the code that is actually executed...  Enjoy...
    //

#if 0
    if (KeIA32VirtualIntExtensions & (V86_VIRTUAL_INT_EXTENSIONS | PM_VIRTUAL_INT_EXTENSIONS)) {
        if (Regs->RiEflags & EFLAGS_V86_MASK) {
            if (KeIA32VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
            }
            else {
            }
        }
        else {
            if (KeIA32VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
            }
            else {
            }
        }
    }
#endif

    _VDM_CHECK_OR_FUNC(Regs->RiEFlags);
    _VDM_CHECK_AND_FUNC(Regs->RiEFlags);
}


VOID
KeIA32AndOrVdmLock(
    IN ULONG AndMask,
    IN ULONG OrMask
    )

/*++

Routine Description: 

    This routine modifies the VdmFixedStateLinear location in an MP
    safe way. In the original IA code, they used the LOCK prefix.

Arguments:

   AndMask - The value to use as the AND mask
   OrMask - The value to use as the OR mask

Returns:

  nothing

--*/
{
#ifdef NT_UP
    *VdmFixedStateLinear = (*VdmFixedStateLinear & AndMask) | OrMask;
#else
    VdmGenericAndOrLock(VdmFixedStateLinear, AndMask, OrMask);
#endif
}

#else   // defined(WX86) && defined(TRY_NTVDM)


BOOLEAN
KiIA32VdmReflectException(
    IN OUT PKIA32_FRAME Frame,
    IN ULONG Code
    )
/*++

Routine Description:

    This routine reflects an exception to a VDM.  It uses the information
    in the trap frame to determine what exception to reflect, and updates
    the trap frame with the new CS, EIP, SS, and SP values

Arguments:

   Frame - Pointer to the IA32 trap frame
   Code - The trap number that brought us here

Returns

    Nothing

Notes:
    Interrupts  are enable upon entry, Irql is at APC level
    This routine may not preserve all of the non-volatile registers if
    a fault occurs.
--*/
{
    //
    // Not yet implemented...
    //
    return(FALSE);
}


BOOLEAN
KiIA32DispatchOpcode(
    IN PKIA32_FRAME Frame
    )

/*++

Routine Description:

    This routine dispatches to the opcode of the specific emulation routine.
    based on the first byte of the opcode. It is a combination of the
    original x86 emulation routines written in assembly (i386/instemul.asm
    and i386/emv86.asm).

Arguments:

    Frame - pointer to the ia32 trap frame.

Return Value:

    Returns true if the opcode was handled, otherwise false

--*/

{
    //
    // Not yet implemented
    //

    return(FALSE);
}


BOOLEAN
KiIA32VdmSegmentNotPresent(
    IN OUT PKIA32_FRAME Frame
    )
/*++

Routine Description:

    This routine reflects a TRAP 0x0B to a VDM. It uses information
    in the trap frame to determine what exception to reflect
    and updates the trap frame with the new CS, EIP, SS, and ESP values

Arguments:

    Frame - Pointer to the IA32 trap frame

Returns

    True if the reflection was successful, false otherwise

Notes:
    none
--*/
{
    //
    // Not yet implemented
    //

    return(FALSE);
}

#endif  // defined(WX86) && defined(TRY_NTVDM)
