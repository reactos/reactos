/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    npxnp.c

Abstract:

    This module contains support for non-Flat mode NPX faults when
    the application has it's CR0_EM bit clear.

Author:

    Ken Reneris (kenr) 8-Dec-1994

Environment:

    User Mode only

Revision History:

--*/


#include "csrdll.h"

static UCHAR MOD16[] = { 0, 1, 2, 0 };
static UCHAR MOD32[] = { 0, 1, 4, 0 };

UCHAR
NpxNpReadCSEip (
    IN PCONTEXT Context
    )
#pragma warning(disable:4035)
{
    _asm {
        push    es
        mov     ecx, Context
        mov     eax, [ecx] CONTEXT.SegCs
        mov     es, ax
        mov     eax, [ecx] CONTEXT.Eip
        inc     dword ptr [ecx] CONTEXT.Eip     ; Advance EIP
        mov     al, es:[eax]
        pop     es
    }
}
#pragma warning(default:4035)


VOID
NpxNpSkipInstruction (
    IN PCONTEXT Context
    )
/*++

Routine Description:

    This functions gains control when the system has no installed
    NPX support, but the thread has cleared it's EM bit in CR0.

    The purpose of this function is to move the instruction
    pointer forward over the current NPX instruction.

Enviroment:

    16:16 mode

Arguments:

Return Value:

--*/
{
    BOOLEAN     fPrefix;
    UCHAR       ibyte, Mod, rm;
    UCHAR       Address32Bits;
    ULONG       CallerCs;

    Address32Bits = 0;                          // assume called from 16:16

    //
    // Lookup and determine callers default mode
    //

    CallerCs = Context->SegCs;
    _asm {
        mov     eax, CallerCs
        lar     eax, eax
        test    eax, 400000h
        jz      short IsDefault16Bit

        mov     Address32Bits, 1

IsDefault16Bit:
    }

    //
    // No sense in using a try-except since we are not on the
    // correct stack.  A fault here could occur if the start
    // of an NPX instruction is near the end of a selector, and the
    // end of the instruction is past the selectors end.  This
    // would kill the app anyway.
    //

    //
    // Read any instruction prefixes
    //

    fPrefix = TRUE;
    while (fPrefix) {
        ibyte = NpxNpReadCSEip(Context);

        switch (ibyte) {
            case 0x2e:  // cs override, skip it
            case 0x36:  // ss override, skip it
            case 0x3e:  // ds override, skip it
            case 0x26:  // es override, skip it
            case 0x64:  // fs override, skip it
            case 0x65:  // gs override, skip it
            case 0x66:  // operand size override, skip it
                break;

            case 0x67:
                // address size override
                Address32Bits ^= 1;
                break;

            default:
                fPrefix = FALSE;
                break;
        }
    }

    //
    // Handle first byte of NPX instruction
    //

    if (ibyte == 0x9b) {

        //
        // FWait instruction - single byte opcode - all done
        //

        return;
    }

    if (ibyte < 0xD8 || ibyte > 0xDF) {

        //
        // Not an ESC instruction
        //

#if DBG
        DbgPrint ("P5_FPU_PATCH: 16: Not NPX ESC instruction\n");
#endif
        return;
    }

    //
    // Get ModR/M byte for NPX opcode
    //

    ibyte = NpxNpReadCSEip(Context);

    if (ibyte > 0xbf) {
        //
        // Outside of ModR/M range for addressing, all done
        //

        return;
    }

    Mod = ibyte >> 6;
    rm  = ibyte & 0x7;
    if (Address32Bits) {
        Context->Eip += MOD32 [Mod];
        if (Mod == 0  &&  rm == 5) {
            // disp 32
            Context->Eip += 4;
        }

        //
        // If SIB byte, read it
        //

        if (rm == 4) {
            ibyte = NpxNpReadCSEip(Context);

            if (Mod == 0  &&  (ibyte & 7) == 5) {
                // disp 32
                Context->Eip += 4;
            }
        }

    } else {
        Context->Eip += MOD16 [Mod];
        if (Mod == 0  &&  rm == 6) {
            // disp 16
            Context->Eip += 2;
        }
    }
}
