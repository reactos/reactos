%include 'internal/hal/segment.inc'


%define PREFIX(a) _(a)

BITS 32
extern PREFIX(page_fault_handler)
extern PREFIX exception_handler
segment .text

DECLARE_GLOBAL_SYMBOL exception_handler14
        cli
	push    gs
	push    fs
	push    es
	push    ds
        push    dword 14
        pushad
        mov     ax,KERNEL_DS
        mov     ds,ax
	mov     es,ax
	mov     fs,ax
	mov     gs,ax
        call    _page_fault_handler
        cmp     eax,0
        jne     _ret_from_exp
        call    _exception_handler                
_ret_from_exp:
        popad
        add     esp,4
	pop     ds
	pop     es
	pop     fs
	pop     gs
	add     esp,4
        iretd

