%include 'internal/hal/segment.inc'

BITS 32
GLOBAL _exception_handler14
EXTERN _exception_handler
EXTERN _page_fault_handler
segment .text

_exception_handler14:
        cli
	push    ds
        push    dword 14
        pushad
        mov     ax,KERNEL_DS
        mov     ds,ax
        call    _page_fault_handler
        cmp     eax,0
        jne     _ret_from_exp
        call    _exception_handler                
_ret_from_exp:
        popad
        add     esp,12
        iretd

