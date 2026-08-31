//
// Wipe.asm
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Manually converted to GNU as format for Wine
//

#include "symcryptasm_shared.cppasm"

// VOID
// SYMCRYPT_CALL
// SymCryptWipe( _Out_writes_bytes_( cbData )   PVOID  pbData,
//                                        SIZE_T cbData )

        .globl @SymCryptWipeAsm@8
        .def @SymCryptWipeAsm@8
        .scl 2
        .type 32
        .endef
@SymCryptWipeAsm@8:

        // ecx = pbData
        // edx = cbData

        //
        // This function will handle any alignment of pbData and any size, but it is optimized for
        // the case where pbData is 4-aligned, and cbData is a multiple of 4.
        //

        xor     eax,eax
        cmp     edx,8
        jb      .LSymCryptWipeAsmSmall     // if cbData < 8, this is a rare case

        test    ecx,7
        jnz     .LSymCryptWipeAsmUnaligned // if data pointer is unaligned, we jump to the code that aligns the pointer
                                           // For well-optimized code the aligned case is the common one, and that is
                                           // the fall-through.

.LSymCryptWipeAsmAligned:
        //
        // Here ecx is aligned, and edx contains the # bytes left to wipe, and edx >= 8
        //
        // Our loop wipes in 8-byte increments;
        //
        sub     edx,8

        .balign   16
.LSymCryptWipeAsmLoop:
        mov     [ecx],eax
        mov     [ecx+4],eax           // Wipe 8 bytes
        add     ecx,8
        sub     edx,8
        ja      .LSymCryptWipeAsmLoop   // Loop if we have >8 byte to wipe. <= 8 bytes is handled by the code below

        //
        // The end of the buffer to wipe is at ecx + edx + 8 as
        // the number of bytes we wiped is 8 less than the number we subtracted from edx.
        //
        // We know we had 8 or more bytes to wipe, so we just wipe the last 8 bytes.
        // This avoids a lot of conditional jumping etc.
        //
        mov     [ecx+edx+8-8],eax
        mov     [ecx+edx+8-4],eax

.LSymCryptWipeAsmDone:
        ret

.LSymCryptWipeAsmUnaligned:
        //
        // At this point we know that cbData(rdx) >= 8 and pbData(rcx) is unaligned.
        // We wipe 4 bytes and set our pointer & length to the remaining aligned buffer
        //
        mov     [ecx],eax
        mov     [ecx+4],eax
        sub     eax,ecx
        and     eax,7
        add     ecx,eax
        sub     edx,eax
        xor     eax,eax

        //
        // edx = cbData could be less than 4 now. If it is, go directly to the
        // tail wiping.
        //
        cmp     edx,8
        jae     .LSymCryptWipeAsmAligned  // if cbData >= 8, do aligned wipes

.LSymCryptWipeAsmSmall:
        jmp     dword ptr [.LSymCryptWipeAsmSmallTable + 4*edx]

.LSymCryptWipeAsmT4:
        mov     [ecx],eax
.LSymCryptWipeAsmT0:
        ret

.LSymCryptWipeAsmT1:
        mov     [ecx],al
        ret

.LSymCryptWipeAsmT3:
        mov     [ecx+2],al
.LSymCryptWipeAsmT2:
        mov     [ecx],ax
        ret

.LSymCryptWipeAsmT5:
        mov     [ecx],eax
        mov     [ecx+1],eax
        ret

.LSymCryptWipeAsmT6:
        mov     [ecx],eax
        mov     [ecx+2],eax
        ret

.LSymCryptWipeAsmT7:
        mov     [ecx],eax
        mov     [ecx+3],eax
        ret


        .balign   4
.LSymCryptWipeAsmSmallTable:
        .long      .LSymCryptWipeAsmT0
        .long      .LSymCryptWipeAsmT1
        .long      .LSymCryptWipeAsmT2
        .long      .LSymCryptWipeAsmT3
        .long      .LSymCryptWipeAsmT4
        .long      .LSymCryptWipeAsmT5
        .long      .LSymCryptWipeAsmT6
        .long      .LSymCryptWipeAsmT7
