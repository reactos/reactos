
#include <asm.inc>
#include <ks386.inc>

EXTERN _KdbEnterDebuggerException:PROC

.code

PUBLIC _KdbEnter
_KdbEnter:
    /*
     * Set up a trap frame
     */
    pushf                      /* Eflags */
    push cs                 /* Cs */
    push 0                  /* ErrorCode */
    push ebp                /* Ebp */
    push ebx                /* Ebx */
    mov ebp, [esp + 20]      /* Eip */
    mov ebx, [esp + 16]      /* Eflags */
    mov [esp + 20], ebx
    mov ebx, [esp + 12]      /* Cs */
    mov [esp + 16], ebx
    mov [esp + 12], ebp
    push esi                /* Esi */
    push edi                /* Edi */
    push fs                 /* Fs */
    push 0                  /* ExceptionList */
    push 0                  /* PreviousMode */
    push eax                /* Eax */
    push ecx                /* Ecx */
    push edx                /* Edx */
    push ds                 /* Ds */
    push es                 /* Es */
    push gs                 /* Gs */
    mov eax, dr7
    push eax                /* Dr7 */

    /* Clear all breakpoint enables in dr7. */
    and eax, HEX(0FFFF0000)
    mov dr7, eax
    mov eax, dr6
    push eax                /* Dr6 */
    mov eax, dr3
    push eax                /* Dr3 */
    mov eax, dr2
    push eax                /* Dr2 */
    mov eax, dr1
    push eax                /* Dr1 */
    mov eax, dr0
    push eax                /* Dr0 */
    lea eax, [esp + HEX(58)]
    push eax                /* TempEsp */
    push ss                 /* TempSegSs */
    push 0                  /* DebugPointer */
    push 3                  /* DebugArgMark (Exception number) */
    push [esp + HEX(60)]          /* DebugEip */
    push ebp                /* DebugEbp */

    /*
     * Call KDB
     */
    mov eax, esp
    push 1                  /* FirstChance */
    push eax                /* Push a pointer to the trap frame */
    push 0                  /* Context */
    push 0                  /* PreviousMode (KernelMode) */
    push 0                  /* ExceptionRecord */
    call _KdbEnterDebuggerException

    /*
     * Pop the arguments and unused portions of the trap frame:
     *   DebugEbp
     *   DebugEip
     *   DebugArgMark
     *   DebugPointer
     *   TempSegSs
     *   TempEsp
     */
    add esp, 11*4

    /*
     * Restore/update debugging registers.
     */
    pop eax            /* Dr0 */
    mov dr0, eax
    pop eax            /* Dr1 */
    mov dr1, eax
    pop eax            /* Dr2 */
    mov dr2, eax
    pop eax            /* Dr3 */
    mov dr3, eax
    pop eax            /* Dr6 */
    mov dr6, eax
    pop eax            /* Dr7 */
    mov dr7, eax

    /*
     * Restore registers including any that might have been changed
     * inside the debugger.
     */
    pop gs         /* Gs */
    pop es         /* Es */
    pop ds         /* Ds */
    pop edx        /* Edx */
    pop ecx        /* Ecx */
    pop eax        /* Eax */
    add esp, 8    /* PreviousMode, ExceptionList */
    pop fs         /* Fs */
    pop edi        /* Edi */
    pop esi        /* Esi */
    pop ebx        /* Ebx */
    pop ebp        /* Ebp */
    add esp, 4    /* ErrorCode */

    /*
     * Return to the caller.
     */
    iretd


PUBLIC _KdbpStackSwitchAndCall@8
_KdbpStackSwitchAndCall@8:
    push ebp
    mov ebp, esp

    mov eax, [esp + 8]         /* New stack */
    mov ecx, [esp + 12]         /* Function to call */
    mov edx, esp              /* Old stack */

    /* Switch stack */
    mov esp, eax
    push edx

    /* Call function */
    call ecx

    /* Switch back to old stack */
    pop esp

    /* Return */
    pop ebp
    ret 8

END
