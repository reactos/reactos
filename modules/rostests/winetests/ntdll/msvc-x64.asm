

.code

; USHORT __readsegds();
PUBLIC __readsegds
__readsegds PROC
    mov ax, ds
    ret
__readsegds ENDP

; USHORT __readseges();
PUBLIC __readseges
__readseges PROC
    mov ax, es
    ret
__readseges ENDP

; USHORT __readsegfs();
PUBLIC __readsegfs
__readsegfs PROC
    mov ax, fs
    ret
__readsegfs ENDP

; USHORT __readseggs();
PUBLIC __readseggs
__readseggs PROC
    mov ax, gs
    ret
__readseggs ENDP

; USHORT __readsegss();
PUBLIC __readsegss
__readsegss PROC
    mov ax, ss
    ret
__readsegss ENDP

; void __cld(void);
PUBLIC __cld
__cld PROC
    cld
    ret
__cld ENDP

; void Call_NtRaiseException(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context, BOOLEAN FirstChance, PVOID pNtRaiseException);
PUBLIC Call_NtRaiseException
Call_NtRaiseException PROC
    mov r12, 0DEADCAFEh
    call r9
    ret
Call_NtRaiseException ENDP

END
