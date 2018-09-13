; /*
;
; Copyright (c) 1997 Microsoft Corporation
;
; Module Name:
;
;    fastsys.inc
;
; Abstract:
;
;    This module implements the header for Fast System Calls for Intel
;    x86 family processors.
;
; Author:
;
; Environment:
;
;    kernel mode.
;
; Revision History:
;
;--

if 0  ; Begin C only code   */

// CPUID Feature bit in EDX indicating that fast system calls are supported
#define KI_FAST_SYSCALL_SUPPORTED  0x0800

#define SYSENTER_CS_MSR         0x00000174
#define SYSENTER_ESP_MSR        0x00000175
#define SYSENTER_EIP_MSR        0x00000176

#define SYSENTER_INSTR          __asm { _emit 0fh  } \
                                __asm { _emit 34h  }
#define SYSEXIT_INSTR           __asm { _emit 0fh  } \
                                __asm { _emit 35h  }

/*
endif
;
;  Begin Assembly definitions
;


SYSENTER_INSTR macro
    db 0fH,34H
endm ;; SYSENTER_INSTR

SYSEXIT_INSTR macro
    db 0fH,35H
endm ;; SYSEXIT_INSTR

; CPUID Feature bit in EDX indicating that fast system calls are supported
KI_FAST_SYSCALL_SUPPORTED            equ         00000800h

SYSENTER_CS_MSR         equ         00000174h
SYSENTER_ESP_MSR        equ         00000175h
SYSENTER_EIP_MSR        equ         00000176h

UPDATE_MSR macro MsrAddress, Value

    push    eax
    push    edx
    push    ecx
    mov     eax, Value
    mov     edx, 0
    mov     ecx, MsrAddress
.586
    wrmsr
    pop     ecx
    pop     edx
    pop     eax

endm    ;; UPDATE_MSR

; */
