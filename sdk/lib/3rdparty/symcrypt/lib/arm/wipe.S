//  SymCryptWipe
//
//  Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
//  Secure wipe
//


// VOID
// SYMCRYPT_CALL
// SymCryptWipe( _Out_writes_bytes_( cbData )   PVOID  pbData,
//                                        SIZE_T cbData )

SymCryptWipeAsm:
.globl SymCryptWipeAsm
.def SymCryptWipeAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptWipeAsm
push {lr}
.seh_save_regs {lr}
.seh_endprologue
        // we just jump to memset.
        // this is enough to stop the compiler optimizing the memset away.
        mov     r2, r1
        mov     r1, #0
        bl       memset
pop {pc}
.seh_endproc
