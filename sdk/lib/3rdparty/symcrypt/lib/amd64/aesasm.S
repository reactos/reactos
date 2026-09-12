//
//  aesasm.symcryptasm   Assembler code for fast AES on the amd64
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This code is derived from the AesFast implementation that
// Niels Ferguson wrote from scratch for BitLocker during Vista.
// That code is still in RSA32.
//

// This file has only been partially translated into symcryptasm, external function calls use the
// generic symcryptasm registers to convert different calling conventions into using the fixed register
// layout used in aesasm. It seems likely that changing which registers AES state will be kept in in
// the macros could impact on performance.
// In general we don't want to touch this code going forward; the vast majority of amd64 CPUs have aesni
// and use the Xmm Aes codepaths.

#include "symcryptasm_shared.cppasm"

#define USE_BLOCK_FUNCTION 1    // Set to 1 to use block function, 0 to use block macro

#if defined(SYMCRYPT_MASM)
extern  SymCryptAesSboxMatrixMult:DWORD
extern  SymCryptAesInvSboxMatrixMult:DWORD
extern  SymCryptAesInvSbox:BYTE
extern  SymCryptFatal:NEAR

#elif defined(SYMCRYPT_GAS)

#else
#error Unknown target assembly
#endif

#if SYMCRYPT_DEBUG
SET(SYMCRYPT_CODE_VERSION, ((SYMCRYPT_CODE_VERSION_API * 65536) + SYMCRYPT_CODE_VERSION_MINOR))
SET(SYMCRYPT_MAGIC_CONSTANT, HEX(53316D76) + SYMCRYPT_CODE_VERSION) // 0x53316D76 == 'S1mv'

.macro SYMCRYPT_CHECK_MAGIC  check_magic_label, ptr, struct_magic_offset, arg_1
        mov     rax, [\ptr + \struct_magic_offset]
        sub     rax, \ptr
        cmp     rax, SYMCRYPT_MAGIC_CONSTANT
        jz      \check_magic_label
        mov     \arg_1, HEX(6D616763) // 0x6D616763 == 'magc'
        call    SymCryptFatal
\check_magic_label:
.endm
#else
.macro SYMCRYPT_CHECK_MAGIC  check_magic_label, ptr, struct_magic_offset, arg_1
.endm
#endif

//
// Structure definition that mirrors the SYMCRYPT_AES_EXPANDED_KEY structure.
//

// SYMCRYPT_AES_EXPANDED_KEY struct
//         RoundKey        dq      2*N_ROUND_KEYS_IN_AESKEY dup (?)        //
//         lastEncRoundKey dq      ?                                       // pointer to last enc round key
//         lastDecRoundKey dq      ?                                       // pointer to last dec round key
//         SYMCRYPT_MAGIC_FIELD
// SYMCRYPT_AES_EXPANDED_KEY ends

SET(N_ROUND_KEYS_IN_AESKEY, 29)
SET(lastEncRoundKeyOffset, (29*16))
SET(lastDecRoundKeyOffset, (29*16 + 8))
SET(magicFieldOffset, (29*16 + 8 + 8))

//
// Shorthand for the 4 tables we will use
// We always use r11 to point to the (inv) SboxMatrixMult tables
//
#define SMM0  (r11 +    0)
#define SMM1  (r11 + 1024)
#define SMM2  (r11 + 2048)
#define SMM3  (r11 + 3072)

#define ISMM0 (r11 +    0)
#define ISMM1 (r11 + 1024)
#define ISMM2 (r11 + 2048)
#define ISMM3 (r11 + 3072)

.macro ENC_MIX  keyptr
        //
        // Perform the unkeyed mixing function for encryption
        // plus a key addition from the key pointer
        //
        // input:block is in     eax, ebx, ecx, edx -  r11 points to AesSboxMatrixMult
        // New state ends up in  eax, ebx, ecx, edx
        // Used registers:       esi, edi, ebp, r8

        //
        // We can use the e<xx> registers for the movzx as the
        // upper 32 bits are automatically set to 0. This saves
        // prefix bytes
        //
        // We use 32-bit registers to store the state.
        // We tried using 64-bit registers, but the extra shifts
        // cost too much.
        // Using 32-bit throughout makes the key xor more expensive
        // but we avoid having to combine the 32-bit halves into
        // 64 bit.
        //

        movzx   esi,al
        mov     esi,[SMM0 + 4 * rsi]
        movzx   edi,ah
        shr     eax,16
        mov     r8d,[SMM1 + 4 * rdi]
        movzx   ebp,al
        mov     ebp,[SMM2 + 4 * rbp]
        movzx   edi,ah
        mov     edi,[SMM3 + 4 * rdi]

        movzx   eax,bl
        xor     edi,[SMM0 + 4 * rax]
        movzx   eax,bh
        shr     ebx,16
        xor     esi,[SMM1 + 4 * rax]
        movzx   eax,bl
        xor     r8d,[SMM2 + 4 * rax]
        movzx   eax,bh
        xor     ebp,[SMM3 + 4 * rax]

        movzx   eax,cl
        xor     ebp,[SMM0 + 4 * rax]
        movzx   ebx,ch
        shr     ecx,16
        xor     edi,[SMM1 + 4 * rbx]
        movzx   eax,cl
        xor     esi,[SMM2 + 4 * rax]
        movzx   ebx,ch
        xor     r8d,[SMM3 + 4 * rbx]

        movzx   eax,dl
        xor     r8d,[SMM0 + 4 * rax]
        movzx   ebx,dh
        shr     edx,16
        xor     ebp,[SMM1 + 4 * rbx]
        movzx   eax,dl
        xor     edi,[SMM2 + 4 * rax]
        movzx   ebx,dh
        xor     esi,[SMM3 + 4 * rbx]

        mov     eax, [\keyptr]
        mov     ebx, [\keyptr + 4]
        xor     eax, esi
        mov     ecx, [\keyptr + 8]
        xor     ebx, edi
        mov     edx, [\keyptr + 12]
        xor     ecx, ebp
        xor     edx, r8d
.endm


.macro DEC_MIX  keyptr
        //
        // Perform the unkeyed mixing function for decryption
        //
        // input:block is in      eax, ebx, ecx, edx
        //       r11 points to AesInvSboxMatrixMult
        // New state ends up in   esi, edi, ebp, r8d

        movzx   esi,al
        mov     esi,[ISMM0 + 4 * rsi]
        movzx   edi,ah
        shr     eax,16
        mov     edi,[ISMM1 + 4 * rdi]
        movzx   ebp,al
        mov     ebp,[ISMM2 + 4 * rbp]
        movzx   eax,ah
        mov     r8d,[ISMM3 + 4 * rax]

        movzx   eax,bl
        xor     edi,[ISMM0 + 4 * rax]
        movzx   eax,bh
        shr     ebx,16
        xor     ebp,[ISMM1 + 4 * rax]
        movzx   eax,bl
        xor     r8d,[ISMM2 + 4 * rax]
        movzx   eax,bh
        xor     esi,[ISMM3 + 4 * rax]

        movzx   eax,cl
        xor     ebp,[ISMM0 + 4 * rax]
        movzx   ebx,ch
        shr     ecx,16
        xor     r8d,[ISMM1 + 4 * rbx]
        movzx   eax,cl
        xor     esi,[ISMM2 + 4 * rax]
        movzx   ebx,ch
        xor     edi,[ISMM3 + 4 * rbx]

        movzx   eax,dl
        xor     r8d,[ISMM0 + 4 * rax]
        movzx   ebx,dh
        shr     edx,16
        xor     esi,[ISMM1 + 4 * rbx]
        movzx   eax,dl
        xor     edi,[ISMM2 + 4 * rax]
        movzx   ebx,dh
        xor     ebp,[ISMM3 + 4 * rbx]

        mov     eax, [\keyptr]
        mov     ebx, [\keyptr + 4]
        xor     eax, esi
        mov     ecx, [\keyptr + 8]
        xor     ebx, edi
        mov     edx, [\keyptr + 12]
        xor     ecx, ebp
        xor     edx, r8d
.endm

.macro AES_ENCRYPT_MACRO  AesEncryptMacroLoopLabel
        //
        // Plaintext in eax, ebx, ecx, edx
        // r9 points to first round key to use (modified)
        // r10 is last key to use (unchanged)
        // r11 points to SboxMatrixMult (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d
        //
        // This macro is free to unroll the cipher completely, or to use a loop
        // over r9
        //

        //
        // xor in first round key
        //
        xor     eax,[r9]
        xor     ebx,[r9+4]
        xor     ecx,[r9+8]
        xor     edx,[r9+12]

        add     r9,32

        // Do not unroll the loop at all because very few CPUs use this codepath so it's worth
        // minimizing the binary size

\AesEncryptMacroLoopLabel:
        // Block is eax, ebx, ecx, edx
        // r9-16 points to next round key

        ENC_MIX r9-16

        cmp     r9,r10
        lea     r9,[r9+16]
        jc      \AesEncryptMacroLoopLabel

        //
        // Now for the final round
        // We use the fact that SboxMatrixMult[0] table is also
        // an Sbox table if you use the second element of each entry.
        //
        // Result is in esi, edi, ebp, r8d
        //

        movzx   esi,al
        movzx   esi,byte ptr[r11 + 1 + 4*rsi]
        movzx   edi,ah
        shr     eax,16
        movzx   r8d,byte ptr[r11 + 1 + 4*rdi]
        movzx   ebp,al
        shl     r8d,8
        movzx   ebp,byte ptr[r11 + 1 + 4*rbp]
        shl     ebp,16
        movzx   edi,ah
        movzx   edi,byte ptr[r11 + 1 + 4*rdi]
        shl     edi,24

        movzx   eax,bl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        or      edi,eax
        movzx   eax,bh
        shr     ebx,16
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shl     eax,8
        or      esi,eax
        movzx   eax,bl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,bh
        shl     eax,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        or      r8d,eax
        shl     ebx,24
        or      ebp,ebx

        movzx   eax,cl
        movzx   ebx,ch
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shr     ecx,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     ebx,8
        or      ebp,eax
        or      edi,ebx
        movzx   eax,cl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,ch
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     eax,16
        shl     ebx,24
        or      esi,eax
        or      r8d,ebx

        movzx   eax,dl
        movzx   ebx,dh
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shr     edx,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     ebx,8
        or      r8d,eax
        or      ebp,ebx
        movzx   eax,dl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,dh
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     eax,16
        shl     ebx,24
        or      edi,eax
        or      esi,ebx

        //
        // xor in final round key
        //

        xor     r8d,[r10+12]
        xor     esi,[r10]
        xor     edi,[r10+4]
        xor     ebp,[r10+8]
.endm

.macro AES_DECRYPT_MACRO  AesDecryptMacroLoopLabel
        //
        // Ciphertext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to InvSboxMatrixMult (unchanged)
        // r12 points to InvSbox (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d
        //

        //
        // xor in first round key
        //
        xor     eax,[r9]
        xor     ebx,[r9+4]
        xor     ecx,[r9+8]
        xor     edx,[r9+12]

        add     r9,32

        // Do not unroll the loop at all because very few CPUs use this codepath so it's worth
        // minimizing the binary size
\AesDecryptMacroLoopLabel:
        // Block is eax, ebx, ecx, edx
        // r9-16 points to next round key

        DEC_MIX r9-16

        cmp     r9,r10
        lea     r9,[r9+16]
        jc      \AesDecryptMacroLoopLabel

        //
        // Now for the final round
        // Result is in esi, edi, ebp, r8d
        //

        movzx   esi,al
        movzx   esi,byte ptr[r12 + rsi]
        movzx   edi,ah
        shr     eax,16
        movzx   edi,byte ptr[r12 + rdi]
        movzx   ebp,al
        shl     edi,8
        movzx   ebp,byte ptr[r12 + rbp]
        shl     ebp,16
        movzx   eax,ah
        movzx   r8d,byte ptr[r12 + rax]
        shl     r8d,24

        movzx   eax,bl
        movzx   eax,byte ptr[r12 + rax]
        or      edi,eax
        movzx   eax,bh
        shr     ebx,16
        movzx   eax,byte ptr[r12 + rax]
        shl     eax,8
        or      ebp,eax
        movzx   eax,bl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,bh
        shl     eax,16
        movzx   ebx,byte ptr[r12 + rbx]
        or      r8d,eax
        shl     ebx,24
        or      esi,ebx

        movzx   eax,cl
        movzx   ebx,ch
        movzx   eax,byte ptr[r12 + rax]
        shr     ecx,16
        movzx   ebx,byte ptr[r12 + rbx]
        shl     ebx,8
        or      ebp,eax
        or      r8d,ebx
        movzx   eax,cl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,ch
        movzx   ebx,byte ptr[r12 + rbx]
        shl     eax,16
        shl     ebx,24
        or      esi,eax
        or      edi,ebx

        movzx   eax,dl
        movzx   ebx,dh
        movzx   eax,byte ptr[r12 + rax]
        shr     edx,16
        movzx   ebx,byte ptr[r12 + rbx]
        shl     ebx,8
        or      r8d,eax
        or      esi,ebx
        movzx   eax,dl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,dh
        movzx   ebx,byte ptr[r12 + rbx]
        shl     eax,16
        shl     ebx,24
        or      edi,eax
        or      ebp,ebx

        //
        // xor in final round key
        //

        xor     esi,[r10]
        xor     edi,[r10+4]
        xor     ebp,[r10+8]
        xor     r8d,[r10+12]
.endm

#if USE_BLOCK_FUNCTION

        //
        // We use a block function, the AES_ENCRYPT macro merely calls the function
        //

.macro AES_ENCRYPT  loopLabel
        call    SymCryptAesEncryptAsmInternal
.endm

.macro AES_DECRYPT  loopLabel
        call    SymCryptAesDecryptAsmInternal
.endm

//========================================
//       SymCryptAesEncryptAsmInternal
//
//       Internal AES encryption routine with modified calling convention.
//       This function has the exact same calling convention as the AES_ENCRYPT_MACRO

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsmInternal):
.globl PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsmInternal)
.def PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsmInternal); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsmInternal)


.seh_endprologue


        AES_ENCRYPT_MACRO SymCryptAesEncryptAsmInternalLoop


ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0

//========================================
//       SymCryptAesDecryptAsmInternal
//
//       Internal AES encryption routine with modified calling convention.
//       This function has the exact same calling convention as the AES_DECRYPT_MACRO
//

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsmInternal):
.globl PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsmInternal)
.def PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsmInternal); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsmInternal)


.seh_endprologue


        AES_DECRYPT_MACRO SymCryptAesDecryptAsmInternalLoop


ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0

#else

        //
        // No block function, use the macro directly
        //

.macro AES_ENCRYPT  loopLabel
        AES_ENCRYPT_MACRO \loopLabel
.endm

.macro AES_DECRYPT  loopLabel
        AES_DECRYPT_MACRO \loopLabel
.endm

#endif

//
//VOID
//SYMCRYPT_CALL
//SymCryptAesEncrypt( _In_                                   PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//                    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_LEN )  PCBYTE                      pbPlaintext,
//                    _Out_writes_bytes_( SYMCRYPT_AES_BLOCK_LEN ) PBYTE                       pbCiphertext )
//

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsm)
.def PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesEncryptAsm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 40
.seh_stackalloc 40

.seh_endprologue


        SYMCRYPT_CHECK_MAGIC SymCryptAesEncryptAsmCheckMagic, Q1, magicFieldOffset, Q1

        // Here we convert from whatever calling convention we are called from externally to our
        // AES internal calling convention.
        // We need to be careful that we don't overwrite an argument register before we copy it to
        // the place it is needed internally in the AES functions.
        // There is no automatic method for checking we do this correctly - modify with care!
        // In SystemV and MSFT x64 ABIs, the possible 3 argument registers are:
        //      rcx, rdx, r8, rdi, rsi

        mov     r10, [Q1 + lastEncRoundKeyOffset]
        mov     r9, Q1

        mov     [rsp + 112 /*MEMSLOT0*/], Q3

        //
        // Load the plaintext
        //
        mov     eax,[Q2     ]
        mov     ebx,[Q2 +  4]
        mov     ecx,[Q2 +  8]
        mov     edx,[Q2 + 12]

        lea     r11,[GET_SYMBOL_ADDRESS(SymCryptAesSboxMatrixMult)]

        AES_ENCRYPT SymCryptAesEncryptAsmLoop
        // Plaintext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to SboxMatrixMult (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d

        // retrieve pbCiphertext using Q0 because it is always rax regardless of calling convention
        mov     Q0,[rsp + 112 /*MEMSLOT0*/]
        mov     [Q0     ], esi
        mov     [Q0 +  4], edi
        mov     [Q0 +  8], ebp
        mov     [Q0 + 12], r8d


add rsp, 40
pop Q14
pop Q13
pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14


//
//VOID
//SYMCRYPT_CALL
//SymCryptAesDecrypt( _In_                                   PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//                    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_LEN )  PCBYTE                      pbCiphertext,
//                    _Out_writes_bytes_( SYMCRYPT_AES_BLOCK_LEN ) PBYTE                       pbPlaintext )

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsm)
.def PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesDecryptAsm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 40
.seh_stackalloc 40

.seh_endprologue


        SYMCRYPT_CHECK_MAGIC SymCryptAesDecryptAsmCheckMagic, Q1, magicFieldOffset, Q1

        // Here we convert from whatever calling convention we are called from externally to our
        // AES internal calling convention.
        // We need to be careful that we don't overwrite an argument register before we copy or use
        // the value appropriately for use in the AES functions.
        // There is no automatic method for checking we do this correctly - modify with care!
        // In SystemV and MSFT x64 ABIs, the possible 3 argument registers are:
        //      rcx, rdx, r8, rdi, rsi

        mov     r9,[Q1 + lastEncRoundKeyOffset]
        mov     r10,[Q1 + lastDecRoundKeyOffset]

        mov     [rsp + 112 /*MEMSLOT0*/], Q3

        mov     eax,[Q2     ]
        mov     ebx,[Q2 +  4]
        mov     ecx,[Q2 +  8]
        mov     edx,[Q2 + 12]

        lea     r11,[GET_SYMBOL_ADDRESS(SymCryptAesInvSboxMatrixMult)]
        lea     r12,[GET_SYMBOL_ADDRESS(SymCryptAesInvSbox)]

        AES_DECRYPT SymCryptAesDecryptAsmLoop
        // Ciphertext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to InvSboxMatrixMult (unchanged)
        // r12 points to InvSbox (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d

        // retrieve pbPlaintext using Q0 because it is always rax regardless of calling convention
        mov     Q0,[rsp + 112 /*MEMSLOT0*/]
        mov     [Q0     ], esi
        mov     [Q0 +  4], edi
        mov     [Q0 +  8], ebp
        mov     [Q0 + 12], r8d


add rsp, 40
pop Q14
pop Q13
pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14

//VOID
//SYMCRYPT_CALL
//SymCryptAesCbcEncrypt(
//    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//    _In_reads_bytes_( cbData )                   PCBYTE                      pbSrc,
//    _Out_writes_bytes_( cbData )                  PBYTE                       pbDst,
//                                            SIZE_T                      cbData )

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptAesCbcEncryptAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptAesCbcEncryptAsm)
.def PLATFORM_NAME_MANGLE(SymCryptAesCbcEncryptAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesCbcEncryptAsm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 40
.seh_stackalloc 40

.seh_endprologue

mov Q5, [rsp + 144]

        // Here we convert from whatever calling convention we are called from externally to our
        // AES internal calling convention.
        // We need to be careful that we don't overwrite an argument register before we copy or use
        // the value appropriately for use in the AES functions.
        // There is no automatic method for checking we do this correctly - modify with care!
        // In SystemV and MSFT x64 ABIs, the possible 5 argument registers are:
        //      rcx, rdx, r8, r9, r10, rdi, rsi

        SYMCRYPT_CHECK_MAGIC SymCryptAesCbcEncryptAsmCheckMagic, Q1, magicFieldOffset, Q1

        and     Q5, NOT 15      // only deal with whole # blocks
        jz      SymCryptAesCbcEncryptNoData

        mov     [rsp + 112 /*MEMSLOT0*/], Q2    // save pbChainingValue
        mov     rax, Q2                 // rax = pbChainingValue
        mov     r13, Q3                 // r13 = pbSrc

        mov     r15, Q5                 // r15 = cbData
        mov     r14, Q4                 // r14 = pbDst

        add     r15, Q3                 // r15 = pbSrcEnd

        mov     r10,[Q1 + lastEncRoundKeyOffset]    // r10 = last enc round key
        mov     r12,Q1                              // r12 = first round key to use

        //
        // Load the chaining state from pbChainingValue
        //
        mov     esi,[rax     ]
        mov     edi,[rax +  4]
        mov     ebp,[rax +  8]
        mov     r8d,[rax + 12]

        lea     r11,[GET_SYMBOL_ADDRESS(SymCryptAesSboxMatrixMult)]

ALIGN(16)
SymCryptAesCbcEncryptAsmLoop:
        // Loop register setup
        // r10 = last round key to use
        // r12 = first round key to use
        // r13 = pbSrc
        // r14 = pbDst
        // r15 = pbSrcEnd

        // chaining state in (esi,edi,ebp,r8d)

        mov     eax, [r13]
        mov     r9, r12
        mov     ebx, [r13+4]
        xor     eax, esi
        mov     ecx, [r13+8]
        xor     ebx, edi
        xor     ecx, ebp
        mov     edx, [r13+12]
        xor     edx, r8d

        add     r13, 16


        AES_ENCRYPT SymCryptAesCbcEncryptAsmInnerLoop
        //
        // Plaintext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to SboxMatrixMult (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d
        //

        mov     [r14], esi
        mov     [r14+4], edi
        mov     [r14+8], ebp
        mov     [r14+12], r8d

        add     r14, 16

        cmp     r13, r15

        jb      SymCryptAesCbcEncryptAsmLoop


        //
        // Update the chaining value
        //
        mov     Q0,[rsp + 112 /*MEMSLOT0*/]
        mov     [Q0], esi
        mov     [Q0+4], edi
        mov     [Q0+8], ebp
        mov     [Q0+12], r8d

SymCryptAesCbcEncryptNoData:


add rsp, 40
pop Q14
pop Q13
pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14


//VOID
//SYMCRYPT_CALL
//SymCryptAesCbcDecrypt(
//    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//    _In_reads_bytes_( cbData )                   PCBYTE                      pbSrc,
//    _Out_writes_bytes_( cbData )                  PBYTE                       pbDst,
//                                            SIZE_T                      cbData )

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptAesCbcDecryptAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptAesCbcDecryptAsm)
.def PLATFORM_NAME_MANGLE(SymCryptAesCbcDecryptAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesCbcDecryptAsm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 40
.seh_stackalloc 40

.seh_endprologue

mov Q5, [rsp + 144]

        // Here we convert from whatever calling convention we are called from externally to our
        // AES internal calling convention.
        // We need to be careful that we don't overwrite an argument register before we copy or use
        // the value appropriately for use in the AES functions.
        // There is no automatic method for checking we do this correctly - modify with care!
        // In SystemV and MSFT x64 ABIs, the possible 5 argument registers are:
        //      rcx, rdx, r8, r9, r10, rdi, rsi

        SYMCRYPT_CHECK_MAGIC SymCryptAesCbcDecryptAsmCheckMagic, Q1, magicFieldOffset, Q1

        and     Q5, NOT 15
        jz      SymCryptAesCbcDecryptNoData

        mov     [rsp + 112 /*MEMSLOT0*/], Q2   // save pbChainingValue
        mov     [rsp + 120 /*MEMSLOT1*/], Q3   // save pbSrc

        lea     r14, [Q5 - 16]
        lea     r15, [Q4 + r14]         // r15 = pbDst pointed to last block
        add     r14, Q3                 // r14 = pbSrc pointed to last block

        mov     r13,[Q1 + lastEncRoundKeyOffset]
        mov     r10,[Q1 + lastDecRoundKeyOffset]

        lea     r11,[GET_SYMBOL_ADDRESS(SymCryptAesInvSboxMatrixMult)]
        lea     r12,[GET_SYMBOL_ADDRESS(SymCryptAesInvSbox)]

        //
        // Load last ciphertext block & save on stack (we need to put it in the pbChaining buffer later)
        //
        mov     eax,[r14]
        mov     ebx,[r14+4]
        mov     ecx,[r14+8]
        mov     edx,[r14+12]

        mov     [rsp + 128 /*MEMSLOT2*/  ], eax
        mov     [rsp + 128 /*MEMSLOT2*/+4], ebx
        mov     [rsp + 136 /*MEMSLOT3*/  ], ecx
        mov     [rsp + 136 /*MEMSLOT3*/+4], edx

        jmp     SymCryptAesCbcDecryptAsmLoopEntry

ALIGN(16)

SymCryptAesCbcDecryptAsmLoop:
        // Loop register setup
        // r13 = first round key to use
        // r14 = pbSrc
        // r15 = pbDst
        // [slot1] = pbSrcStart

        // current ciphertext block (esi,edi,ebp,r8d)

        mov     eax,[r14-16]
        mov     ebx,[r14-12]
        xor     esi,eax
        mov     ecx,[r14-8]
        xor     edi,ebx
        mov     [r15],esi
        mov     edx,[r14-4]
        xor     ebp,ecx
        mov     [r15+4],edi
        xor     r8d,edx
        mov     [r15+8],ebp
        mov     [r15+12],r8d

        sub     r14,16
        sub     r15,16

SymCryptAesCbcDecryptAsmLoopEntry:

        mov     r9, r13

        AES_DECRYPT SymCryptAesCbcDecryptAsmInnerLoop
        //
        // Ciphertext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to InvSboxMatrixMult (unchanged)
        // r12 points to InvSbox (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d
        //

        cmp     r14, [rsp + 120 /*MEMSLOT1*/]  // pbSrc
        ja      SymCryptAesCbcDecryptAsmLoop

        mov     rbx,[rsp + 112 /*MEMSLOT0*/]   // pbChainingValue
        xor     esi,[rbx]
        xor     edi,[rbx+4]
        xor     ebp,[rbx+8]
        xor     r8d,[rbx+12]

        mov     [r15], esi
        mov     [r15+4], edi
        mov     [r15+8], ebp
        mov     [r15+12], r8d

        //
        // Update the chaining value to the last ciphertext block
        //
        mov     rax,[rsp + 128 /*MEMSLOT2*/]
        mov     rcx,[rsp + 136 /*MEMSLOT3*/]
        mov     [rbx], rax
        mov     [rbx+8], rcx

SymCryptAesCbcDecryptNoData:


add rsp, 40
pop Q14
pop Q13
pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14

//VOID
//SYMCRYPT_CALL
//SymCryptAesCtrMsb64(
//    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//    _In_reads_bytes_( cbData )                   PCBYTE                      pbSrc,
//    _Out_writes_bytes_( cbData )                  PBYTE                       pbDst,
//                                            SIZE_T                      cbData )

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptAesCtrMsb64Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptAesCtrMsb64Asm)
.def PLATFORM_NAME_MANGLE(SymCryptAesCtrMsb64Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptAesCtrMsb64Asm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 40
.seh_stackalloc 40

.seh_endprologue

mov Q5, [rsp + 144]

        // Here we convert from whatever calling convention we are called from externally to our
        // AES internal calling convention.
        // We need to be careful that we don't overwrite an argument register before we copy or use
        // the value appropriately for use in the AES functions.
        // There is no automatic method for checking we do this correctly - modify with care!
        // In SystemV and MSFT x64 ABIs, the possible 5 argument registers are:
        //      rcx, rdx, r8, r9, r10, rdi, rsi

        SYMCRYPT_CHECK_MAGIC SymCryptAesCtrMsb64AsmCheckMagic, Q1, magicFieldOffset, Q1

        and     Q5, NOT 15      // only deal with whole # blocks
        jz      SymCryptAesCtrMsb64NoData

        mov     [rsp + 112 /*MEMSLOT0*/], Q2   // save pbChainingState
        mov     rax, Q2         // rax = pbChainingValue
        mov     r13, Q3         // r13 = pbSrc
        mov     r14, Q5         // r14 = cbData
        mov     r15, Q4         // r15 = pbDst
        add     r14, Q3         // r14 = cbData + pbSrc = pbSrcEnd

        mov     r10,[Q1 + lastEncRoundKeyOffset]        // r10 = last enc round key
        mov     r12,Q1                                  // r12 = first round key to use


        lea     r11,[GET_SYMBOL_ADDRESS(SymCryptAesSboxMatrixMult)]

        //
        // Load the chaining state
        //
        mov     rcx, [rax + 8]
        mov     rax, [rax    ]

        //
        // Store it in our local copy (we have no register free to keep pbChainingState in)
        //
        mov     [rsp + 120 /*MEMSLOT1*/], rax
        mov     [rsp + 128 /*MEMSLOT2*/], rcx

        //
        // Move to the right registers
        //
        mov     rbx, rax
        mov     rdx, rcx
        shr     rbx, 32
        shr     rdx, 32

ALIGN(16)
SymCryptAesCtrMsb64AsmLoop:
        // Loop invariant
        // Current chaining state is in (eax, ebx, ecx, edx)
        // r10 = last round key to use
        // r11 = SboxMatrixMult
        // r12 = first round key to use
        // r13 = pbSrc
        // r14 = pbSrcEnd
        // r15 = pbDst
        // [slot1..slot2] = 16 bytes chaining state block

        mov     r9, r12

        AES_ENCRYPT SymCryptAesCtrMsb64AsmInnerLoop
        //
        // Plaintext in eax, ebx, ecx, edx
        // r9 points to first round key to use
        // r10 is last key to use (unchanged)
        // r11 points to SboxMatrixMult (unchanged)
        // Ciphertext ends up in esi, edi, ebp, r8d
        //

        // To improve latency, we FIRST
        // load the chaining state, increment the counter, and write it back.
        // leave the state in the (eax, ebx, ecx, edx) registers

        mov     eax,dword ptr [rsp + 120 /*MEMSLOT1*/ + 0]
        mov     ebx,dword ptr [rsp + 120 /*MEMSLOT1*/ + 4]
        mov     rcx,[rsp +  128 /*MEMSLOT2*/ ]
        bswap   rcx
        add     rcx, 1
        bswap   rcx
        mov     [rsp + 128 /*MEMSLOT2*/ ], rcx
        mov     rdx, rcx
        shr     rdx, 32

        // THEN we process the XOR of the key stream with the data
        // This order is faster as we need to have the chaining state done
        // before we can proceed, but there are no dependencies on the data result
        // So we can loop back to the beginning while the data stream read/writes are
        // still in flight.
        //
        // xor with the source stream

        xor     esi,[r13 + 0 ]
        xor     edi,[r13 + 4 ]
        xor     ebp,[r13 + 8 ]
        xor     r8d,[r13 + 12]

        // store at the destination

        mov     [r15 + 0], esi
        mov     [r15 + 4], edi
        mov     [r15 + 8], ebp
        mov     [r15 + 12], r8d

        add     r13, 16     // pbSrc += 16
        add     r15, 16     // pbDst += 16

        cmp     r13, r14

        jb      SymCryptAesCtrMsb64AsmLoop

        //
        // Copy back the chaining value - we only modified the last 8 bytes, so that is all we copy
        //
        mov     rsi,[rsp + 112 /*MEMSLOT0*/]   // pbChainingState
        mov     [rsi + 8], ecx
        mov     [rsi + 12], edx

        //
        // Wipe the chaining value on stack
        //
        xor     rax, rax
        mov     [rsp + 120 /*MEMSLOT1*/], rax
        mov     [rsp + 128 /*MEMSLOT2*/], rax

SymCryptAesCtrMsb64NoData:


add rsp, 40
pop Q14
pop Q13
pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14

FILE_END()
