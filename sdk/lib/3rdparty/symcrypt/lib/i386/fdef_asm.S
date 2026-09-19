//
//  fdef_asm.cppasm     Assembler code for fast arithmetic
//  Requires C preprocessor to correctly include C_asm_shared.inc and symcrypt_magic.in
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Manually converted to GNU as format for Wine
//

#include "symcryptasm_shared.cppasm"

        .globl @SymCryptFdefRawAddAsm@16
        .def @SymCryptFdefRawAddAsm@16
        .scl 2
        .type 32
        .endef
@SymCryptFdefRawAddAsm@16:

//UINT32
//SYMCRYPT_CALL
//SymCryptFdefRawAdd(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                            UINT32      nDigits )

        // stack frame
        .set SymCryptFdefRawAddAsmFrame_SaveEdi,       0
        .set SymCryptFdefRawAddAsmFrame_SaveEbx,       4
        .set SymCryptFdefRawAddAsmFrame_ReturnAddress, 8
        .set SymCryptFdefRawAddAsmFrame_pDst,          12
        .set SymCryptFdefRawAddAsmFrame_nDigits,       16

        // ecx = pSrc1
        // edx = pSrc2

        mov     edi,edi
        push    ebx
        push    edi

        mov     edi, [esp + SymCryptFdefRawAddAsmFrame_pDst]
        mov     ebx, [esp + SymCryptFdefRawAddAsmFrame_nDigits]

        xor     eax,eax                     // Cy = 0

.LSymCryptFdefRawAddAsmLoop:
        mov     eax, [ecx]
        adc     eax, [edx]
        mov     [edi], eax

        mov     eax, [ecx + 4]
        adc     eax, [edx + 4]
        mov     [edi + 4], eax

        mov     eax, [ecx + 8]
        adc     eax, [edx + 8]
        mov     [edi + 8], eax

        mov     eax, [ecx + 12]
        adc     eax, [edx + 12]
        mov     [edi + 12], eax

        lea     ecx, [ecx + 16]
        lea     edx, [edx + 16]
        lea     edi, [edi + 16]

        dec     ebx
        jnz     .LSymCryptFdefRawAddAsmLoop

        mov     eax, 0
        adc     eax, eax

        pop     edi
        pop     ebx
        ret     8


        .globl @SymCryptFdefRawSubAsm@16
        .def @SymCryptFdefRawSubAsm@16
        .scl 2
        .type 32
        .endef
@SymCryptFdefRawSubAsm@16:
//UINT32
//SYMCRYPT_CALL
//SymCryptFdefRawSub(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                            UINT32      nDigits )

        // stack frame
        .set SymCryptFdefRawSubAsmFrame_SaveEdi,       0
        .set SymCryptFdefRawSubAsmFrame_SaveEbx,       4
        .set SymCryptFdefRawSubAsmFrame_ReturnAddress, 8
        .set SymCryptFdefRawSubAsmFrame_pDst,          12
        .set SymCryptFdefRawSubAsmFrame_nDigits,       16

        // ecx = pSrc1
        // edx = pSrc2

        mov     edi,edi
        push    ebx
        push    edi

        mov     edi, [esp + SymCryptFdefRawSubAsmFrame_pDst]
        mov     ebx, [esp + SymCryptFdefRawSubAsmFrame_nDigits]

        xor     eax,eax                     // Cy = 0

.LSymCryptFdefRawSubAsmLoop:
        mov     eax, [ecx]
        sbb     eax, [edx]
        mov     [edi], eax

        mov     eax, [ecx + 4]
        sbb     eax, [edx + 4]
        mov     [edi + 4], eax

        mov     eax, [ecx + 8]
        sbb     eax, [edx + 8]
        mov     [edi + 8], eax

        mov     eax, [ecx + 12]
        sbb     eax, [edx + 12]
        mov     [edi + 12], eax

        lea     ecx, [ecx + 16]
        lea     edx, [edx + 16]
        lea     edi, [edi + 16]

        dec     ebx
        jnz     .LSymCryptFdefRawSubAsmLoop

        mov     eax, 0
        adc     eax, eax

        pop     edi
        pop     ebx
        ret     8

        .globl @SymCryptFdefMaskedCopyAsm@16
        .def @SymCryptFdefMaskedCopyAsm@16
        .scl 2
        .type 32
        .endef
@SymCryptFdefMaskedCopyAsm@16:
//VOID
//SYMCRYPT_CALL
//SymCryptFdefMaskedCopy(
//    _In_reads_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )      PCBYTE      pbSrc,
//    _InOut_writes_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )  PBYTE       pbDst,
//                                                                UINT32      nDigits,
//                                                                UINT32      mask )

        // stack frame
        .set SymCryptFdefMaskedCopyAsmFrame_SaveEdi,       0
        .set SymCryptFdefMaskedCopyAsmFrame_SaveEsi,       4
        .set SymCryptFdefMaskedCopyAsmFrame_SaveEbx,       8
        .set SymCryptFdefMaskedCopyAsmFrame_Returnaddress, 12
        .set SymCryptFdefMaskedCopyAsmFrame_nDigits,       16
        .set SymCryptFdefMaskedCopyAsmFrame_vMask,         20

        mov     edi,edi
        push    ebx
        push    esi
        push    edi

        mov     esi, [esp + SymCryptFdefMaskedCopyAsmFrame_vMask]
        mov     edi, [esp + SymCryptFdefMaskedCopyAsmFrame_nDigits]

.LSymCryptFdefMaskedCopyAsmLoop:
        mov     eax, [ecx]
        mov     ebx, [edx]
        xor     eax, ebx
        and     eax, esi
        xor     eax, ebx
        mov     [edx], eax

        mov     eax, [ecx + 4]
        mov     ebx, [edx + 4]
        xor     eax, ebx
        and     eax, esi
        xor     eax, ebx
        mov     [edx + 4], eax

        mov     eax, [ecx + 8]
        mov     ebx, [edx + 8]
        xor     eax, ebx
        and     eax, esi
        xor     eax, ebx
        mov     [edx + 8], eax

        mov     eax, [ecx + 12]
        mov     ebx, [edx + 12]
        xor     eax, ebx
        and     eax, esi
        xor     eax, ebx
        mov     [edx + 12], eax

        add     ecx, 16
        add     edx, 16
        sub     edi, 1
        jnz     .LSymCryptFdefMaskedCopyAsmLoop

        // No return value
        pop     edi
        pop     esi
        pop     ebx
        ret     8


        .globl @SymCryptFdefRawMulAsm@20
        .def @SymCryptFdefRawMulAsm@20
        .scl 2
        .type 32
        .endef
@SymCryptFdefRawMulAsm@20:

//VOID
//SYMCRYPT_CALL
//SymCryptFdefRawMul(
//    _In_reads_(nWords1)             PCUINT32    pSrc1,
//                                    UINT32      nDigits1,
//    _In_reads_(nWords2)             PCUINT32    pSrc2,
//                                    UINT32      nDigits2,
//    _Out_writes_(nWords1 + nWords2) PUINT32     pDst )

        // stack frame
        .set SymCryptFdefRawMulAsmFrame_nWords1,       0
        .set SymCryptFdefRawMulAsmFrame_pSrc1,         4
        .set SymCryptFdefRawMulAsmFrame_SaveEbp,       8
        .set SymCryptFdefRawMulAsmFrame_SaveEsi,       12
        .set SymCryptFdefRawMulAsmFrame_SaveEdi,       16
        .set SymCryptFdefRawMulAsmFrame_SaveEbx,       20
        .set SymCryptFdefRawMulAsmFrame_ReturnAddress, 24
        .set SymCryptFdefRawMulAsmFrame_pSrc2,         28
        .set SymCryptFdefRawMulAsmFrame_nDigits2,      32
        .set SymCryptFdefRawMulAsmFrame_pDst,          36

        // Basic code structure:
        //   for each word in Src1:
        //       Dst += Src2 * word
        // Register assignments
        //
        //   eax = tmp/lower half of mult
        //   ebx = multiplicant
        //   ecx = loop counter, initialized to nDigits2
        //   edx = upper half of mult
        //   esi = running pointer into Src2
        //   edi = dst
        //   ebp = carry
        //
        //   esp + pSrc1     running pointer into Src1
        //   esp +


        mov     edi,edi
        push    ebx
        push    esi
        push    edi
        push    ebp
        sub     esp, SymCryptFdefRawMulAsmFrame_SaveEbp

        // ecx = pSrc1
        // edx = nDigits1

        mov     esi,[esp + SymCryptFdefRawMulAsmFrame_pSrc2]

        mov     ebx, [ecx]                                          // ebx = pSrc1[0]
        add     ecx, 4
        mov     [esp + SymCryptFdefRawMulAsmFrame_pSrc1], ecx

        xor     ebp, ebp                                            // carry = 0

        mov     edi,[esp + SymCryptFdefRawMulAsmFrame_pDst]

        shl     edx, 2                                              // convert nDigits1 to nWords1
        sub     edx, 1                                              // the first word is outside the outer loop, decremented here
        mov     [esp + SymCryptFdefRawMulAsmFrame_nWords1], edx

        mov     ecx,[esp + SymCryptFdefRawMulAsmFrame_nDigits2]

.LSymCryptFdefRawMulAsmLoop1:
        mov     eax, [esi]
        mul     ebx
        add     eax, ebp
        adc     edx, 0
        mov     [edi], eax
        mov     ebp, edx

        mov     eax, [esi + 4]
        mul     ebx
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 4], eax
        mov     ebp, edx

        mov     eax, [esi + 8]
        mul     ebx
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 8], eax
        mov     ebp, edx

        mov     eax, [esi + 12]
        mul     ebx
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 12], eax
        mov     ebp, edx

        add     esi, 16
        add     edi, 16
        sub     ecx,1
        jnz     .LSymCryptFdefRawMulAsmLoop1

        mov     [edi], edx                  // write last word; no overflow because Dst is at least 2 digits long


.LSymCryptFdefRawMulAsmLoopOuter:

        mov     esi, [esp + SymCryptFdefRawMulAsmFrame_pSrc2]

        // Fetch next Src1 word & increment pointer
        mov     eax, [esp + SymCryptFdefRawMulAsmFrame_pSrc1]
        mov     ebx, [eax]
        add     eax, 4
        mov     [esp + SymCryptFdefRawMulAsmFrame_pSrc1], eax

        xor     ebp, ebp

        // increment pDst & set in edi
        mov     edi, [esp + SymCryptFdefRawMulAsmFrame_pDst]
        add     edi, 4
        mov     [esp + SymCryptFdefRawMulAsmFrame_pDst], edi

        mov     ecx, [esp + SymCryptFdefRawMulAsmFrame_nDigits2]

.LSymCryptFdefRawMulAsmLoop2:
        mov     eax, [esi]
        mul     ebx
        add     eax, [edi]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi], eax
        mov     ebp, edx

        mov     eax, [esi + 4]
        mul     ebx
        add     eax, [edi + 4]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 4], eax
        mov     ebp, edx

        mov     eax, [esi + 8]
        mul     ebx
        add     eax, [edi + 8]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 8], eax
        mov     ebp, edx

        mov     eax, [esi + 12]
        mul     ebx
        add     eax, [edi + 12]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 12], eax
        mov     ebp, edx

        add     esi, 16
        add     edi, 16
        sub     ecx,1
        jnz     .LSymCryptFdefRawMulAsmLoop2

        mov     [edi], edx

        mov     eax,[esp + SymCryptFdefRawMulAsmFrame_nWords1]
        sub     eax, 1
        mov     [esp + SymCryptFdefRawMulAsmFrame_nWords1], eax

        jnz     .LSymCryptFdefRawMulAsmLoopOuter


        add     esp, SymCryptFdefRawMulAsmFrame_SaveEbp
        pop     ebp
        pop     edi
        pop     esi
        pop     ebx
        ret     12


        .globl @SymCryptFdefMontgomeryReduceAsm@12
        .def @SymCryptFdefMontgomeryReduceAsm@12
        .scl 2
        .type 32
        .endef
@SymCryptFdefMontgomeryReduceAsm@12:
//VOID
//SymCryptFdefMontgomeryReduceAsm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )

        // stack frame
        .set SymCryptFdefMontgomeryReduceAsmFrame_HighCarry,     0
        .set SymCryptFdefMontgomeryReduceAsmFrame_pSrc,          4
        .set SymCryptFdefMontgomeryReduceAsmFrame_pModValue,     8
        .set SymCryptFdefMontgomeryReduceAsmFrame_nWords,        12
        .set SymCryptFdefMontgomeryReduceAsmFrame_SaveEbp,       16
        .set SymCryptFdefMontgomeryReduceAsmFrame_SaveEsi,       20
        .set SymCryptFdefMontgomeryReduceAsmFrame_SaveEdi,       24
        .set SymCryptFdefMontgomeryReduceAsmFrame_SaveEbx,       28
        .set SymCryptFdefMontgomeryReduceAsmFrame_ReturnAddress, 32
        .set SymCryptFdefMontgomeryReduceAsmFrame_pDst,          36

        mov     edi,edi
        push    ebx
        push    esi
        push    edi
        push    ebp

        sub     esp, SymCryptFdefMontgomeryReduceAsmFrame_SaveEbp

        // ecx = pmMod
        // edx = pSrc

        mov     eax, [ecx + SymCryptModulusNdigitsOffsetX86]
        shl     eax, 2
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_nWords], eax

        lea     esi, [ecx + SymCryptModulusValueOffsetX86]
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_pModValue], esi

        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_pSrc], edx
        mov     edi, edx

        xor     eax,eax
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_HighCarry], eax     // set high-carry to zero

.LSymCryptFdefMontgomeryReduceOuterLoop:
        // eax = <undef>
        // ebx = <undef>
        // ecx = <undef>
        // edx = <undef>
        // esi = start of mod value
        // edi = pSrc + 4 * loop iteration count
        // ebp = <undef>

        // compute multiplier for this outer loop iteration.
        mov     ebx, [esi - SymCryptModulusValueOffsetX86 + SymCryptModulusInv64OffsetX86 ]
        imul    ebx, [edi]              // word we want to zero out, ebx = multiplier for this inner loop

        mov     ecx, [esi - SymCryptModulusValueOffsetX86 + SymCryptModulusNdigitsOffsetX86]        // ecx = nDigits
        xor     ebp, ebp        // set carry = 0

.LSymCryptFdefMontgomeryReduceInnerLoop:
        // eax = mul scratch
        // ebx = multiplier
        // ecx = digit counter
        // edx = mul scratch
        // esi = running pointer to mod value
        // edi = running pointer to input/scratch
        // ebp = carry (32 bits)

        mov     eax, [esi]
        mul     ebx
        add     eax, [edi]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi], eax
        mov     ebp, edx

        mov     eax, [esi + 4]
        mul     ebx
        add     eax, [edi + 4]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 4], eax
        mov     ebp, edx

        mov     eax, [esi + 8]
        mul     ebx
        add     eax, [edi + 8]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 8], eax
        mov     ebp, edx

        mov     eax, [esi + 12]
        mul     ebx
        add     eax, [edi + 12]
        adc     edx, 0
        add     eax, ebp
        adc     edx, 0
        mov     [edi + 12], eax
        mov     ebp, edx

        add     esi, 16
        add     edi, 16
        sub     ecx,1
        jnz     .LSymCryptFdefMontgomeryReduceInnerLoop

        // Process the last high-word result with the high carry
        xor     eax, eax
        add     ebp,[esp + SymCryptFdefMontgomeryReduceAsmFrame_HighCarry]
        adc     eax, eax            // eax was 0, so this is equal to adc eax, 0
        add     ebp,[edi]
        adc     eax, 0
        mov     [edi], ebp
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_HighCarry], eax

        mov     esi, [esp + SymCryptFdefMontgomeryReduceAsmFrame_pModValue]

        mov     edi, [esp + SymCryptFdefMontgomeryReduceAsmFrame_pSrc]
        add     edi, 4
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_pSrc], edi

        mov     eax, [esp + SymCryptFdefMontgomeryReduceAsmFrame_nWords]
        sub     eax, 1
        mov     [esp + SymCryptFdefMontgomeryReduceAsmFrame_nWords], eax

        jnz     .LSymCryptFdefMontgomeryReduceOuterLoop

        // At this point:
        // esi = pModValue
        // edi = pSrc
        // Additional 1-bit carry word for the pSrc value is in the HighCarry stack variable.
        // Cy = 0


        mov     ecx, [esi - SymCryptModulusValueOffsetX86 + SymCryptModulusNdigitsOffsetX86]    // loop counter
        mov     edx, [esp + SymCryptFdefMontgomeryReduceAsmFrame_pDst]

        // ecx = nDigits

        // Save some values for the copy loop
        mov     ebx, ecx            // nDigits
        mov     ebp, edx            // pDst

        // Carry = 0 from previous sub eax,1 of the outer loop counter
.LSymCryptFdefMontgomeryReduceAsmSubLoop:
        // edi = running pointer into pSrc result
        // esi = running ptr into modulus value
        // edx = running ptr into pDst

        mov     eax,[edi]
        sbb     eax,[esi]
        mov     [edx], eax

        mov     eax,[edi + 4]
        sbb     eax,[esi + 4]
        mov     [edx + 4], eax

        mov     eax,[edi + 8]
        sbb     eax,[esi + 8]
        mov     [edx + 8], eax

        mov     eax,[edi + 12]
        sbb     eax,[esi + 12]
        mov     [edx + 12], eax

        lea     edi, [edi + 16]
        lea     esi, [esi + 16]
        lea     edx, [edx + 16]
        dec     ecx
        jnz     .LSymCryptFdefMontgomeryReduceAsmSubLoop

        mov     ecx, [esp + SymCryptFdefMontgomeryReduceAsmFrame_HighCarry]
        sbb     ecx, 0
        // eax = mask for masked copy


        mov     edi, [esp + SymCryptFdefMontgomeryReduceAsmFrame_pSrc]

        // ecx = mask
.LSymCryptFdefMontgomeryReduceAsmMaskedCopyLoop:
        // ebx = loop counter
        // ebp = pDst
        // edi = pSrc
        // ecx = mask

        mov     eax, [edi]
        mov     edx, [ebp]
        xor     eax, edx
        and     eax, ecx
        xor     eax, edx
        mov     [ebp], eax

        mov     eax, [edi + 4]
        mov     edx, [ebp + 4]
        xor     eax, edx
        and     eax, ecx
        xor     eax, edx
        mov     [ebp + 4], eax

        mov     eax, [edi + 8]
        mov     edx, [ebp + 8]
        xor     eax, edx
        and     eax, ecx
        xor     eax, edx
        mov     [ebp + 8], eax

        mov     eax, [edi + 12]
        mov     edx, [ebp + 12]
        xor     eax, edx
        and     eax, ecx
        xor     eax, edx
        mov     [ebp + 12], eax

        add     edi, 16
        add     ebp, 16
        sub     ebx, 1
        jnz     .LSymCryptFdefMontgomeryReduceAsmMaskedCopyLoop

        // Done, no return value

        add     esp, SymCryptFdefMontgomeryReduceAsmFrame_SaveEbp
        pop     ebp
        pop     edi
        pop     esi
        pop     ebx

        ret     4
