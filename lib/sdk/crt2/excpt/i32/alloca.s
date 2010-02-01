.intel_syntax noprefix

//_PAGESIZE_      equ     1000h
#define _PAGESIZE_ 0x1000

.globl __alloca_probe_16
.globl __alloca_probe_8

.func __alloca_probe_16
__alloca_probe_16:						// 16 byte aligned alloca
        push    ecx
        lea     ecx, [esp] + 8          // TOS before entering this function
        sub     ecx, eax                // New TOS
        and     ecx, (16 - 1)           // Distance from 16 bit align (align down)
        add     eax, ecx                // Increase allocation size
        sbb     ecx, ecx                // ecx = 0xFFFFFFFF if size wrapped around
        or      eax, ecx                // cap allocation size on wraparound
        pop     ecx                     // Restore ecx
        jmp     __chkstk
.endfunc

.func __alloca_probe_8
__alloca_probe_8:						// 8 byte aligned alloca
        push    ecx
        lea     ecx, [esp] + 8          // TOS before entering this function
        sub     ecx, eax                // New TOS
        and     ecx, (8 - 1)            // Distance from 8 bit align (align down)
        add     eax, ecx                // Increase allocation Size
        sbb     ecx, ecx                // ecx = 0xFFFFFFFF if size wrapped around
        or      eax, ecx                // cap allocation size on wraparound
        pop     ecx                     // Restore ecx
        jmp     __chkstk
.endfunc


