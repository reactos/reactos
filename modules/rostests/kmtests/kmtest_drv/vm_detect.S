/*
 * PROJECT:     ReactOS Kernel-Mode Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ASM helper functions for VM detection
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#include <asm.inc>

#define VMWARE_SIGNATURE    HEX(564D5868) // 'VMXh'
#define VMWARE_PORT         HEX(5658) // 'VX'

.code

/*
 * BOOLEAN
 * VmIsVMware(VOID);
 */
#ifdef _M_IX86
PUBLIC _VmIsVMware
_VmIsVMware:
    push ebx
    push ecx
    push edx
#else
PUBLIC VmIsVMware
VmIsVMware:
    push rbx
    push rcx
    push rdx
#endif

    mov eax, VMWARE_SIGNATURE
    mov ebx, 0
    mov ecx, 10
    mov edx, VMWARE_PORT
    in eax, dx
    cmp ebx, VMWARE_SIGNATURE
    sete al

#ifdef _M_IX86
    pop edx
    pop ecx
    pop ebx
#else
    pop rdx
    pop rcx
    pop rbx
#endif

    ret

END
