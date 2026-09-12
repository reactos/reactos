//
//  AesAsm.cppasm   Assembler code for fast AES on the x86
//  Requires C preprocessor to correctly include C_asm_shared.inc and symcrypt_magic.inc
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This code is derived from the AesFast implementation that
// Niels Ferguson wrote from scratch for BitLocker during Vista.
// That code is still in RSA32.
//
// Manually converted to GNU as format for Wine
//

#include "symcryptasm_shared.cppasm"

//
// Structure definition that mirrors the SYMCRYPT_AES_EXPANDED_KEY structure.
//

#define N_ROUND_KEYS_IN_AESKEY 29

        .set SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey, 16*N_ROUND_KEYS_IN_AESKEY
        .set SYMCRYPT_AES_EXPANDED_KEY_lastDecRoundKey, 16*N_ROUND_KEYS_IN_AESKEY + 4


//=====================================================================
//       AesEncrypt & AesDecrypt Macros
//

// Shorthand for the 4 tables we will use


#define SMM0     _SymCryptAesSboxMatrixMult
#define SMM1     _SymCryptAesSboxMatrixMult + 0x0400
#define SMM2     _SymCryptAesSboxMatrixMult + 0x0800
#define SMM3     _SymCryptAesSboxMatrixMult + 0x0c00

#define ISMM0     _SymCryptAesInvSboxMatrixMult
#define ISMM1     _SymCryptAesInvSboxMatrixMult + 0x0400
#define ISMM2     _SymCryptAesInvSboxMatrixMult + 0x0800
#define ISMM3     _SymCryptAesInvSboxMatrixMult + 0x0c00


        .macro ENC_MIX   load_into_esi
        //
        // Perform the unkeyed mixing function for encryption
        // load_into_esi is the value loaded into esi by the end
        //
        // input block is in      eax, ebx, edx, ebp
        // New state ends up in   ecx, edi, eax, ebp

        push    ebp

        movzx   ecx,al
        mov     ecx,[SMM0 + 4 * ecx]
        movzx   esi,ah
        shr     eax,16
        mov     ebp,[SMM1 + 4 * esi]
        movzx   edi,ah
        mov     edi,[SMM3 + 4 * edi]
        movzx   eax,al
        mov     eax,[SMM2 + 4 * eax]

        movzx   esi,bl
        xor     edi,[SMM0 + 4 * esi]
        movzx   esi,bh
        shr     ebx,16
        xor     ecx,[SMM1 + 4 * esi]
        movzx   esi,bl
        xor     ebp,[SMM2 + 4 * esi]
        movzx   ebx,bh
        xor     eax,[SMM3 + 4 * ebx]

        pop     ebx

        movzx   esi,dl
        xor     eax,[SMM0 + 4 * esi]
        movzx   esi,dh
        xor     edi,[SMM1 + 4 * esi]
        shr     edx,16
        movzx   esi,dl
        xor     ecx,[SMM2 + 4 * esi]
        movzx   edx,dh
        xor     ebp,[SMM3 + 4 * edx]

        mov     esi,[\load_into_esi]

        movzx   edx,bl
        xor     ebp,[SMM0 + 4 * edx]
        movzx   edx,bh
        xor     eax,[SMM1 + 4 * edx]
        shr     ebx,16
        movzx   edx,bl
        xor     edi,[SMM2 + 4 * edx]
        movzx   ebx,bh
        xor     ecx,[SMM3 + 4 * ebx]

        .endm


        .macro DEC_MIX   load_into_esi
        //
        // Perform the unkeyed mixing function for decryption
        // load_into_esi is the value loaded into esi by the end
        //
        // input block is in      eax, ebx, edx, ebp
        // New state ends up in   ecx, edi, eax, ebp

        push    ebp

        movzx   ecx,al
        mov     ecx,[ISMM0 + 4 * ecx]
        movzx   esi,ah
        shr     eax,16
        mov     edi,[ISMM1 + 4 * esi]
        movzx   ebp,ah
        mov     ebp,[ISMM3 + 4 * ebp]
        movzx   eax,al
        mov     eax,[ISMM2 + 4 * eax]

        movzx   esi,bl
        xor     edi,[ISMM0 + 4 * esi]
        movzx   esi,bh
        shr     ebx,16
        xor     eax,[ISMM1 + 4 * esi]
        movzx   esi,bl
        xor     ebp,[ISMM2 + 4 * esi]
        movzx   ebx,bh
        xor     ecx,[ISMM3 + 4 * ebx]

        pop     ebx

        movzx   esi,dl
        xor     eax,[ISMM0 + 4 * esi]
        movzx   esi,dh
        xor     ebp,[ISMM1 + 4 * esi]
        shr     edx,16
        movzx   esi,dl
        xor     ecx,[ISMM2 + 4 * esi]
        movzx   edx,dh
        xor     edi,[ISMM3 + 4 * edx]

        mov     esi,[\load_into_esi]

        movzx   edx,bl
        xor     ebp,[ISMM0 + 4 * edx]
        movzx   edx,bh
        xor     ecx,[ISMM1 + 4 * edx]
        shr     ebx,16
        movzx   edx,bl
        xor     edi,[ISMM2 + 4 * edx]
        movzx   ebx,bh
        xor     eax,[ISMM3 + 4 * ebx]

        .endm


        .macro ADD_KEY   keyptr
        //
        // Input is block in     ecx, edi, eax, ebp
        // keyptr points to the round key
        // Output is in          eax, ebx, edx, ebp
        //

        mov     edx, [\keyptr+8]
        xor     edx, eax
        mov     eax, [\keyptr]
        xor     eax, ecx
        xor     ebp, [\keyptr+12]
        mov     ebx, [\keyptr+4]
        xor     ebx, edi

        .endm



        .macro AES_ENC   load_into_esi_offset, key_limit, key_ptr
        //
        // Plaintext in eax,ebx,edx,ebp
        // ecx points to first round key to use
        // [key_limit] is last key to use
        // [key_ptr] is temp storage area on stack
        //
        // Ciphertext ends up in ecx,edi,eax,ebp
        //
        // Loads [esp+load_into_esi_offset] into esi, except of offset = -1
        //

        xor     eax,[ecx]
        xor     ebx,[ecx+4]
        xor     edx,[ecx+8]
        xor     ebp,[ecx+12]

        lea     ecx,[ecx+16]
        mov     [\key_ptr],ecx

        .balign   16
1:
        // Block is in eax,ebx,edx,ebp
        // ecx points to next round key

        ENC_MIX \key_ptr
        ADD_KEY esi

        add     esi,16
        cmp     esi,[\key_limit]

        mov     [\key_ptr],esi

        jc      1b

        push    ebp

        movzx   ecx,al
        movzx   ecx,byte ptr SMM0[1 + 4 * ecx]
        movzx   esi,ah
        shr     eax,16
        movzx   ebp,byte ptr SMM0[1 + 4 * esi]
        movzx   edi,ah
        shl     ebp,8
        movzx   edi,byte ptr SMM0[1 + 4 * edi]
        movzx   eax,al
        shl     edi,24
        movzx   eax,byte ptr SMM0[1 + 4 * eax]
        shl     eax,16

        movzx   esi,bl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        or      edi,esi
        movzx   esi,bh
        shr     ebx,16
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        shl     esi,8
        or      ecx,esi
        movzx   esi,bl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        movzx   ebx,bh
        shl     esi,16
        movzx   ebx,byte ptr SMM0[1 + 4 * ebx]
        or      ebp,esi
        shl     ebx,24
        or      eax,ebx

        movzx   esi,dl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        movzx   ebx,dh
        shr     edx,16
        movzx   ebx,byte ptr SMM0[1 + 4 * ebx]
        or      eax,esi
        shl     ebx,8
        movzx   esi,dl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        or      edi,ebx
        pop     ebx
        movzx   edx,dh
        movzx   edx,byte ptr SMM0[1 + 4 * edx]
        shl     esi,16
        or      ecx,esi
        shl     edx,24
        or      ebp,edx

        movzx   esi,bl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        movzx   edx,bh
        shr     ebx,16
        movzx   edx,byte ptr SMM0[1 + 4 * edx]
        or      ebp,esi
        shl     edx,8
        or      eax,edx
        mov     edx,[\key_ptr]
        movzx   esi,bl
        movzx   esi,byte ptr SMM0[1 + 4 * esi]
        movzx   ebx,bh
        movzx   ebx,byte ptr SMM0[1 + 4 * ebx]
        shl     esi,16
        shl     ebx,24
        or      edi,esi

        .ifgt      \load_into_esi_offset
        mov     esi,[esp+\load_into_esi_offset]
        .endif

        or      ecx,ebx


        xor     ecx,[edx]
        xor     edi,[edx+4]
        xor     eax,[edx+8]
        xor     ebp,[edx+12]

        .endm

        .macro AES_DEC   load_into_esi_offset, key_limit, key_ptr
        //
        //
        // Ciphertext in eax,ebx,edx,ebp
        // ecx points to first round key to use
        // [key_limit] is last key to use
        // [key_ptr] is temp storage area on stack
        //
        // Plaintext ends up in eax,ebc,edx,ebp
        //

        xor     eax,[ecx]
        xor     ebx,[ecx+4]
        xor     edx,[ecx+8]
        xor     ebp,[ecx+12]

        lea     ecx,[ecx+16]
        mov     [\key_ptr],ecx

        .balign   16
1:
        // Block is in eax,ebx,edx,ebp
        // ecx points to next round key

        DEC_MIX \key_ptr
        ADD_KEY esi

        add     esi,16
        cmp     esi,[\key_limit]

        mov     [\key_ptr],esi

        jc      1b


        push    ebp

        movzx   ecx,al
        movzx   ecx,byte ptr _SymCryptAesInvSbox[ecx]
        movzx   edi,ah
        shr     eax,16
        movzx   edi,byte ptr _SymCryptAesInvSbox[edi]
        movzx   esi,ah
        shl     edi,8
        movzx   ebp,byte ptr _SymCryptAesInvSbox[esi]
        movzx   eax,al
        shl     ebp,24
        movzx   eax,byte ptr _SymCryptAesInvSbox[eax]
        shl     eax,16

        movzx   esi,bl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        or      edi,esi
        movzx   esi,bh
        shr     ebx,16
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        shl     esi,8
        or      eax,esi
        movzx   esi,bl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        movzx   ebx,bh
        shl     esi,16
        movzx   ebx,byte ptr _SymCryptAesInvSbox[ebx]
        or      ebp,esi
        shl     ebx,24
        or      ecx,ebx

        movzx   esi,dl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        movzx   ebx,dh
        shr     edx,16
        movzx   ebx,byte ptr _SymCryptAesInvSbox[ebx]
        or      eax,esi
        shl     ebx,8
        movzx   esi,dl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        or      ebp,ebx
        pop     ebx
        movzx   edx,dh
        movzx   edx,byte ptr _SymCryptAesInvSbox[edx]
        shl     esi,16
        or      ecx,esi
        shl     edx,24
        or      edi,edx

        movzx   esi,bl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        movzx   edx,bh
        shr     ebx,16
        movzx   edx,byte ptr _SymCryptAesInvSbox[edx]
        or      ebp,esi
        shl     edx,8
        or      ecx,edx
        mov     edx,[\key_ptr]
        movzx   esi,bl
        movzx   esi,byte ptr _SymCryptAesInvSbox[esi]
        movzx   ebx,bh
        movzx   ebx,byte ptr _SymCryptAesInvSbox[ebx]
        shl     esi,16
        shl     ebx,24
        or      edi,esi

        .ifge      \load_into_esi_offset
        mov     esi,[esp+\load_into_esi_offset]
        .endif

        or      eax,ebx

        xor     ecx,[edx]
        xor     edi,[edx+4]
        xor     eax,[edx+8]
        xor     ebp,[edx+12]

        .endm

        .globl @SymCryptAesEncryptAsm@12
        .def @SymCryptAesEncryptAsm@12
        .scl 2
        .type 32
        .endef
@SymCryptAesEncryptAsm@12:
//VOID
//SYMCRYPT_CALL
//SymCryptAesEncrypt( _In_                                   PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//                    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_LEN )  PCBYTE                      pbPlaintext,
//                    _Out_writes_bytes_( SYMCRYPT_AES_BLOCK_LEN ) PBYTE                       pbCiphertext );

        // stack frame
        .set AesEncryptFrame_key_pointer,   0
        .set AesEncryptFrame_lastRoundKey,  4
        .set AesEncryptFrame_SaveEdi,       8
        .set AesEncryptFrame_SaveEsi,       12
        .set AesEncryptFrame_SaveEbp,       16
        .set AesEncryptFrame_SaveEbx,       20
        .set AesEncryptFrame_ReturnAddress, 24
        .set AesEncryptFrame_Ciphertext,    28

        // ecx = AesKey
        // edx = Plaintext
        // [esp+4] = Ciphertext

        //
        // 2-byte NOP for hot patching
        // This is what our current compiler does for every function, so we will follow
        // that.
        //
        mov     edi,edi

        //
        // Set up our stack frame
        //
        push    ebx
        push    ebp
        push    esi
        push    edi
        sub     esp, 8

        // Load the plaintext block into eax,ebx,edx,ebp

        mov     eax,[edx]
        mov     ebx,[edx+4]
        mov     ebp,[edx+12]
        mov     edx,[edx+8]     // Plaintext ptr no longer needed

        // Get the address of the last round key
        mov     esi,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey]
        mov     [esp+AesEncryptFrame_lastRoundKey],esi

        AES_ENC AesEncryptFrame_Ciphertext, esp + AesEncryptFrame_lastRoundKey, esp + AesEncryptFrame_key_pointer
//       call    @SymCryptAesEncryptAsmInternal@16

        //mov     esi,[esp+AesEncryptFrame_Ciphertext]
        mov     [esi],ecx
        mov     [esi+4],edi
        mov     [esi+8],eax
        mov     [esi+12],ebp

        // wipe the stack location we used for temporary pushes

        mov     [esp-8],esp

        add     esp,8
        pop     edi
        pop     esi
        pop     ebp
        pop     ebx
        ret     4


        .globl @SymCryptAesDecryptAsm@12
        .def @SymCryptAesDecryptAsm@12
        .scl 2
        .type 32
        .endef
@SymCryptAesDecryptAsm@12:
//VOID
//AesDecrypt( _In_                            PCADD_KEY   AesKey,
//            _In_reads_bytes_(AES_BLOCK_SIZE)     PCBYTE      Ciphertext,
//            _Out_writes_bytes_(AES_BLOCK_SIZE)    PBYTE       Plaintext
//            )

        // stack frame
        .set AesDecryptFrame_key_pointer,   0
        .set AesDecryptFrame_lastRoundKey,  4
        .set AesDecryptFrame_SaveEdi,       8
        .set AesDecryptFrame_SaveEsi,       12
        .set AesDecryptFrame_SaveEbp,       16
        .set AesDecryptFrame_SaveEbx,       20
        .set AesDecryptFrame_ReturnAddress, 24
        .set AesDecryptFrame_Plaintext,     28

        // ecx = AesKey
        // edx = Ciphertext
        // [esp+4] = Plaintext

        //
        // 2-byte NOP for hot patching
        // This is what our current compiler does for every function, so we will follow
        // that.
        //
        mov     edi,edi

        //
        // Set up our stack frame
        //
        push    ebx
        push    ebp
        push    esi
        push    edi
        sub     esp, 8

        // Load the ciphertext block into eax,ebx,edx,ebp

        mov     eax,[edx]
        mov     ebx,[edx+4]
        mov     ebp,[edx+12]
        mov     edx,[edx+8]     // Ciphertext ptr no longer needed


        // Get the address of the last round key
        mov     esi,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastDecRoundKey]       // esi = lastDecRoundKey
        mov     ecx,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey]       // ecx = lastEncRoundKey = first Dec round key
        mov     [esp+AesDecryptFrame_lastRoundKey],esi

        AES_DEC AesDecryptFrame_Plaintext, esp + AesDecryptFrame_lastRoundKey, esp + AesDecryptFrame_key_pointer


        mov     [esi],ecx
        mov     [esi+4],edi
        mov     [esi+8],eax
        mov     [esi+12],ebp

        mov     [esp-8],esp

        add     esp,8
        pop     edi
        pop     esi
        pop     ebp
        pop     ebx
        ret     4




        .globl @SymCryptAesCbcEncryptAsm@20
        .def @SymCryptAesCbcEncryptAsm@20
        .scl 2
        .type 32
        .endef
@SymCryptAesCbcEncryptAsm@20:
//VOID
//SYMCRYPT_CALL
//SymCryptAesCbcEncrypt(
//    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//    _In_reads_bytes_( cbData )                   PCBYTE                      pbSrc,
//    _Out_writes_bytes_( cbData )                  PBYTE                       pbDst,
//                                            SIZE_T                      cbData );
//

        // stack frame
        .set AesCbcEncryptFrame_key_pointer,     0
        .set AesCbcEncryptFrame_lastRoundKey,    4
        .set AesCbcEncryptFrame_pbEndDst,        8
        .set AesCbcEncryptFrame_firstRoundKey,   12
        .set AesCbcEncryptFrame_pbChainingValue, 16
        .set AesCbcEncryptFrame_SaveEdi,         20
        .set AesCbcEncryptFrame_SaveEsi,         24
        .set AesCbcEncryptFrame_SaveEbp,         28
        .set AesCbcEncryptFrame_SaveEbx,         32
        .set AesCbcEncryptFrame_ReturnAddress,   36
        .set AesCbcEncryptFrame_pbSrc,           40
        .set AesCbcEncryptFrame_pbDst,           44
        .set AesCbcEncryptFrame_cbData,          48

        // ecx = pExpandedKey
        // edx = pbChainingValue
        // [esp+4...] = pbSrc, pbDst, cbData

        //
        // 2-byte NOP for hot patching
        // This is what our current compiler does for every function, so we will follow
        // that.
        //
        mov     edi,edi

        //
        // Set up our stack frame
        //
        push    ebx
        push    ebp
        push    esi
        push    edi

        push    edx             // pbChainingValue
        push    ecx             // pbExpandedKey points to the first round key
        sub     esp, 12

        mov     eax,[esp + AesCbcEncryptFrame_cbData]
        and     eax, NOT 15
        jz      AesCbcEncryptDoNothing

        // Get & store the address of the last round key
        mov     ebx,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey]
        mov     [esp+AesCbcEncryptFrame_lastRoundKey],ebx


        mov     esi,[esp + AesCbcEncryptFrame_pbDst]
        add     eax,esi
        mov     [esp + AesCbcEncryptFrame_pbEndDst], eax

        //
        // Convert pbSrc to an offset from pbDst to reduce the # updates
        // in a loop
        //
        //mov    ecx,[esp + AesCbcEncryptFrame_pbSrc]
        //sub    ecx, esi
        //mov    [esp + AesCbcEncryptFrame_pbSrc], ecx

        // esi = pbDst

        // Load state from chaining value
        // State is in ecx,edi,eax,ebp
        mov     ecx,[edx]
        mov     edi,[edx + 4]
        mov     eax,[edx + 8]
        mov     ebp,[edx + 12]


AesCbcEncryptLoop:
        // Invariant:
        //  State in (ecx, edi, eax, ebp)
        //  esi = pbDst for next block

        //
        // Change esi to point to current pbSrc pointer. pbSrc is here an offset from pbDst.
        //
        mov     esi,[esp + AesCbcEncryptFrame_pbSrc]

        //
        // Xor next plaintext block & move state to (eax, ebx, edx, ebp)
        // We keep the reads sequentially to help the HW prefetch logic in the CPU
        //
        xor     ecx, [esi]

        mov     ebx, [esi + 4]
        xor     ebx, edi

        mov     edx, [esi + 8]
        xor     edx, eax

        mov     eax, ecx

        xor     ebp, [esi + 12]

        add     esi, 16
        mov     [esp + AesCbcEncryptFrame_pbSrc], esi

        mov     ecx,[esp + AesCbcEncryptFrame_firstRoundKey]


        AES_ENC AesCbcEncryptFrame_pbDst, esp + AesCbcEncryptFrame_lastRoundKey, esp + AesCbcEncryptFrame_key_pointer

        //call   @SymCryptAesEncryptAsmInternal@16
        //mov    esi,[esp + AesCbcEncryptFrame_pbDst]

        mov     [esi], ecx
        mov     [esi + 4], edi
        mov     [esi + 8], eax
        mov     [esi + 12], ebp

        add     esi,16
        cmp     esi,[esp + AesCbcEncryptFrame_pbEndDst]
        mov     [esp + AesCbcEncryptFrame_pbDst], esi
        jb      AesCbcEncryptLoop

        mov     edx,[esp + AesCbcEncryptFrame_pbChainingValue]
        mov     [edx], ecx
        mov     [edx + 4], edi
        mov     [edx + 8], eax
        mov     [edx + 12], ebp

        //
        // Wipe the one stack location that the internal encrypt routine uses
        // for temporary data storage
        //
        mov     [esp - 8], esp

AesCbcEncryptDoNothing:

        add     esp, 20
        pop     edi
        pop     esi
        pop     ebp
        pop     ebx
        ret     12




        .globl @SymCryptAesCbcDecryptAsm@20
        .def @SymCryptAesCbcDecryptAsm@20
        .scl 2
        .type 32
        .endef
@SymCryptAesCbcDecryptAsm@20:
//VOID
//SYMCRYPT_CALL
//SymCryptAesCbcDecrypt(
//    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//    _In_reads_bytes_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//    _In_reads_bytes_( cbData )                   PCBYTE                      pbSrc,
//    _Out_writes_bytes_( cbData )                  PBYTE                       pbDst,
//                                            SIZE_T                      cbData );
//

        // stack frame
        .set AesCbcDecryptFrame_key_pointer,     0
        .set AesCbcDecryptFrame_lastRoundKey,    4
        .set AesCbcDecryptFrame_newChainValue,   8     // New chaining value after the call
        .set AesCbcDecryptFrame_pbCurrentDst,    24
        .set AesCbcDecryptFrame_firstRoundKey,   28
        .set AesCbcDecryptFrame_pbChainingValue, 32
        .set AesCbcDecryptFrame_SaveEdi,         36
        .set AesCbcDecryptFrame_SaveEsi,         40
        .set AesCbcDecryptFrame_SaveEbp,         44
        .set AesCbcDecryptFrame_SaveEbx,         48
        .set AesCbcDecryptFrame_ReturnAddress,   52
        .set AesCbcDecryptFrame_pbSrc,           56
        .set AesCbcDecryptFrame_pbDst,           60
        .set AesCbcDecryptFrame_cbData,          64

        // ecx = pExpandedKey
        // edx = pbChainingValue
        // [esp+4...] = pbSrc, pbDst, cbData

        //
        // 2-byte NOP for hot patching
        // This is what our current compiler does for every function, so we will follow
        // that.
        //
        mov     edi,edi

        //
        // Set up our stack frame
        //
        push    ebx
        push    ebp
        push    esi
        push    edi

        push    edx             // pbChainingValue
        sub     esp, 32

        mov     eax,[esp + AesCbcDecryptFrame_cbData]
        and     eax, NOT 15
        jz      AesCbcDecryptDoNothing

        // Get & store the address of the first & last round key
        mov     ebx,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastDecRoundKey]
        mov     [esp+AesCbcDecryptFrame_lastRoundKey],ebx

        sub     eax, 16

        mov     ecx,[ecx + SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey]           // = first dec round key
        mov     [esp + AesCbcDecryptFrame_firstRoundKey], ecx

        mov     edi,[esp + AesCbcDecryptFrame_pbDst]
        add     edi, eax
        mov     [esp + AesCbcDecryptFrame_pbCurrentDst], edi    // points to last block in buffer

        mov     esi,[esp + AesCbcDecryptFrame_pbSrc]
        add     esi, eax
        mov     [esp + AesCbcDecryptFrame_pbSrc], esi

        //
        // Load last block & store it for the chaining output
        //
        mov     eax,[esi]
        mov     [esp + AesCbcDecryptFrame_newChainValue], eax
        mov     ebx,[esi + 4]
        mov     [esp + AesCbcDecryptFrame_newChainValue + 4], ebx
        mov     edx,[esi + 8]
        mov     [esp + AesCbcDecryptFrame_newChainValue + 8], edx
        mov     ebp,[esi + 12]
        mov     [esp + AesCbcDecryptFrame_newChainValue + 12], ebp

        jmp     AesCbcDecryptLoopEnter

AesCbcDecryptLoop:
        // Invariant:
        //  State in (ecx, edi, eax, ebp)
        //  State is just after decrypt, but before feed-forward xor
        //  esi = pbCurrentDst for just decrypted state
        //  pbSrc = corresponding position in Src buffer

        mov     ebx,[esp + AesCbcDecryptFrame_pbSrc]

        mov     edx,[ebx - 16 + 8]
        xor     eax, edx
        mov     [esi + 16 + 8], eax

        mov     eax,[ebx - 16 + 0]
        xor     ecx, eax
        mov     [esi + 16 + 0], ecx

        lea     ecx, [ebx - 16]         // decrement + move ptr to allow register shuffle

        mov     ebx,[ebx - 16 + 4]
        xor     edi, ebx
        mov     [esi + 16 + 4], edi

        mov     edi,[ecx + 12]
        xor     ebp, edi
        mov     [esi + 16 + 12], ebp
        mov     ebp, edi

        mov     [esp + AesCbcDecryptFrame_pbSrc], ecx

        mov     ecx,[esp + AesCbcDecryptFrame_firstRoundKey]

AesCbcDecryptLoopEnter:

        AES_DEC AesCbcDecryptFrame_pbCurrentDst, esp + AesCbcDecryptFrame_lastRoundKey, esp + AesCbcDecryptFrame_key_pointer
        //call   @SymCryptAesDecryptAsmInternal@16
        //mov    esi,[esp + AesCbcDecryptFrame_pbCurrentDst]

        cmp     esi,[esp + AesCbcDecryptFrame_pbDst]
        lea     esi,[esi-16]
        mov     [esp + AesCbcDecryptFrame_pbCurrentDst], esi

        ja      AesCbcDecryptLoop

        //
        // Xor with chaining value and store last block (first in output buffer)
        //

        mov     ebx,[esp + AesCbcDecryptFrame_pbChainingValue]
        xor     ecx,[ebx]
        mov     [esi + 16], ecx
        xor     edi,[ebx + 4]
        mov     [esi + 16 + 4], edi
        xor     eax,[ebx + 8]
        mov     [esi + 16 + 8], eax
        xor     ebp,[ebx + 12]
        mov     [esi + 16+12], ebp

        //
        // Set the new chaining value
        //

        mov     eax,[esp + AesCbcDecryptFrame_newChainValue + 0]
        mov     [ebx + 0], eax
        mov     ecx,[esp + AesCbcDecryptFrame_newChainValue + 4]
        mov     [ebx + 4], ecx
        mov     eax,[esp + AesCbcDecryptFrame_newChainValue + 8]
        mov     [ebx + 8], eax
        mov     ecx,[esp + AesCbcDecryptFrame_newChainValue + 12]
        mov     [ebx + 12], ecx


        // Wipe the one stack location that the internal encrypt routine uses
        // for temporary data storage
        //
        mov     [esp - 8], esp

AesCbcDecryptDoNothing:

        add     esp, 36
        pop     edi
        pop     esi
        pop     ebp
        pop     ebx
        ret     12



        .globl @SymCryptAesCtrMsb64Asm@20
        .def @SymCryptAesCtrMsb64Asm@20
        .scl 2
        .type 32
        .endef
@SymCryptAesCtrMsb64Asm@20:
//VOID
//SYMCRYPT_CALL
//SymCryptAesCtrMsb64(
//        _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
//        _Inout_updates_bytes_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
//        _In_reads_bytes_( cbData )                       PCBYTE                      pbSrc,
//        _Out_writes_bytes_( cbData )                      PBYTE                       pbDst,
//                                                    SIZE_T                      cbData )

        // stack frame
        .set AesCtrMsb64Frame_key_pointer,        0
        .set AesCtrMsb64Frame_lastRoundKey,       4
        .set AesCtrMsb64Frame_chainValue,         8
        .set AesCtrMsb64Frame_pbEndDst,           24
        .set AesCtrMsb64Frame_firstRoundKey,      28
        .set AesCtrMsb64Frame_pbChainingValue,    32
        .set AesCtrMsb64Frame_SaveEdi,            36
        .set AesCtrMsb64Frame_SaveEsi,            40
        .set AesCtrMsb64Frame_SaveEbp,            44
        .set AesCtrMsb64Frame_SaveEbx,            48
        .set AesCtrMsb64Frame_ReturnAddress,      52
        .set AesCtrMsb64Frame_pbSrc,              56
        .set AesCtrMsb64Frame_pbDst,              60
        .set AesCtrMsb64Frame_cbData,             64

        // ecx = pExpandedKey
        // edx = pbChainingValue
        // [esp+4...] = pbSrc, pbDst, cbData

        //
        // 2-byte NOP for hot patching
        // This is what our current compiler does for every function, so we will follow
        // that.
        //
        mov     edi,edi

        //
        // Set up our stack frame
        //
        push    ebx
        push    ebp
        push    esi
        push    edi

        push    edx             // pbChainingValue
        push    ecx             // pbExpandedKey points to the first round key
        sub     esp, 28

        mov     eax,[esp + AesCtrMsb64Frame_cbData]
        and     eax, NOT 15
        jz      AesCtrMsb64DoNothing

        // Get & store the address of the last round key
        mov     ebx,[ecx+SYMCRYPT_AES_EXPANDED_KEY_lastEncRoundKey]
        mov     [esp+AesCtrMsb64Frame_lastRoundKey],ebx


        mov     esi,[esp + AesCtrMsb64Frame_pbDst]
        add     eax,esi
        mov     [esp + AesCtrMsb64Frame_pbEndDst], eax

        // esi = pbDst

        // Load state from chaining value & copy into local stack
        // State is in ecx,edi,eax,ebp
        mov     eax,[edx]
        mov     [esp + AesCtrMsb64Frame_chainValue], eax
        mov     ebx,[edx + 4]
        mov     [esp + AesCtrMsb64Frame_chainValue + 4], ebx
        mov     ebp,[edx + 12]
        mov     edx,[edx + 8]


AesCtrMsb64Loop:
        // Invariant:
        //  counter State in (eax, ebx, edx, ebp)
        //  first 8 bytes of counter state in [esp+chainValue]

        //
        // Write the updated counter value to the local copy
        //
        mov     [esp + AesCtrMsb64Frame_chainValue + 8], edx
        mov     ecx,[esp + AesCtrMsb64Frame_firstRoundKey]
        mov     [esp + AesCtrMsb64Frame_chainValue + 12], ebp

        AES_ENC AesCtrMsb64Frame_pbSrc, esp + AesCtrMsb64Frame_lastRoundKey, esp + AesCtrMsb64Frame_key_pointer
        //       result is in (ecx, edi, eax, ebp)
        //       esi = pbSrc

        mov     ebx,[esp + AesCtrMsb64Frame_pbDst]

        //
        // Read the Src block, xor it with the state, and write it to Dst
        //
        xor     ecx,[esi]
        mov     [ebx],ecx
        xor     edi,[esi+4]
        mov     [ebx+4],edi
        xor     eax,[esi+8]
        mov     [ebx+8],eax
        xor     ebp,[esi+12]
        mov     [ebx+12],ebp

        add     esi, 16
        mov     [esp + AesCtrMsb64Frame_pbSrc], esi

        lea     ecx,[ebx + 16]                  // increment and move pointer to allow reg shuffle

        //
        // Load the current chain value, and do the increment
        //
        mov     ebp,[esp + AesCtrMsb64Frame_chainValue+12]
        bswap   ebp
        mov     edx,[esp + AesCtrMsb64Frame_chainValue+8]
        bswap   edx
        add     ebp,1
        mov     eax,[esp + AesCtrMsb64Frame_chainValue]
        adc     edx, 0
        mov     ebx,[esp + AesCtrMsb64Frame_chainValue+4]
        bswap   ebp
        bswap   edx


        cmp     ecx,[esp + AesCtrMsb64Frame_pbEndDst]
        mov     [esp + AesCtrMsb64Frame_pbDst], ecx

        jb      AesCtrMsb64Loop

        //
        // Write the updated chaining value; we only need to update 8 bytes
        //
        mov     ebx,[esp + AesCtrMsb64Frame_pbChainingValue]
        mov     [ebx + 8], edx
        mov     [ebx + 12], ebp

        //
        // Wipe our local copy of the chaining value
        //
        xor     eax,eax
        mov     [esp + AesCtrMsb64Frame_chainValue], eax
        mov     [esp + AesCtrMsb64Frame_chainValue + 4], eax
        mov     [esp + AesCtrMsb64Frame_chainValue + 8], eax
        mov     [esp + AesCtrMsb64Frame_chainValue + 12], eax

        //
        // Wipe the one stack location that the internal encrypt routine uses
        // for temporary data storage
        //
        mov     [esp - 8], esp

AesCtrMsb64DoNothing:

        add     esp, 36
        pop     edi
        pop     esi
        pop     ebp
        pop     ebx
        ret     12
