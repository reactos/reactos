; DWORD CallCPLEntry16(UINT hinst, DWORD lpfnEntry,
;                            HWND hwndCPL, UINT msg,
;                            DWORD lParam1, DWORD lParam2)

BODYQT_CALLCPLENTRY16 macro index

    local CPL_afMapping
    local do_callcplentry16
    local apm_map_explicit
    local before_call
    local call_the_bugger
    local no_unmapping

    hinst       equ   <[ebp_qttop]>
    lpfnEntry   equ   <[ebp_qttop+4]>
    hwndCPL     equ   <[ebp_qttop+8]>
    msg         equ   <[ebp_qttop+12]>
    lParam1     equ   <[ebp_qttop+16]>
    lParam2     equ   <[ebp_qttop+20]>

        ; skip past our table below to the meat of the routine
        jmp     do_callcplentry16

;
; This table indicates whether we need to thunk lParam2 or not.
;
CPL_afMapping	label byte
        db      0   ; CPL_INIT
        db      0   ; CPL_GETCOUNT
        db      1   ; CPL_INQUIRE      int, LPCPINFO
        db      0   ; CPL_SELECT       int, DWORD
        db      0   ; CPL_DBLCLK       int, DWORD
        db      0   ; CPL_STOP         int, DWORD
        db      0   ; CPL_EXIT
        db      1   ; CPL_NEWINQUIRE   int, LPNEWCPLINFO
        db      1   ; CPL_STARTWPARAMS int, LPSTR
CPL_MAX equ     ($-CPL_afMapping)

apm_map_explicit:
        inc     ecx                     ;explicitly set mapping flag
        jmp     before_call

do_callcplentry16:
        xor     ecx, ecx                ;ecx controls pointer mapping, init 0
        mov     edx, dword ptr msg      ;message index so we can decide

	; HACK: OEM APM extensions use entrypoints identical to CPlApplet,
	;	but have msgs needing special care.  Caller sets bit 15
	;	(0x8000) on these msgs.  Both msgs needing special care
	;	map identically to CPL_NEWINQUIRE
	test	edx, 8000h		;special APM msg?
	jnz	apm_map_explicit	;we will need to map

        ; use table above to determine mapping, assumes ecx is still zero
        dec     edx                     ;the table is 0 based
        cmp     edx, CPL_MAX            ;check for index within table
        jnc     before_call             ;let it through with no mapping

        ; move the correct mapping flag from our table into ecx
        mov     cl, byte ptr cs:[edx + CPL_afMapping]

before_call:
        push    ecx                     ;save mapping flag for thunk return
        jcxz    call_the_bugger         ;skip mapping if it's zero

        ; map lParam2 to 16:16
        push    dword ptr lParam2
        call    MapLS                   
        mov     dword ptr lParam2, eax  ;pass mapped result instead

call_the_bugger:
	push	word ptr hinst  	;hinst: dword->word
	push	dword ptr lpfnEntry	;lpfnEntry: dword->dword
	push	word ptr hwndCPL	;hwndCPL: dword->word
	push	word ptr msg    	;msg: dword->word
	push	dword ptr lParam1	;lParam1: dword->dword
	push	dword ptr lParam2	;lParam2: dword->dword
	mov	cl,index	        ;thunk index

	call	QT_Call16_DWordToDWord

        pop     ecx                     ;restore mapping flag from above
        jcxz    no_unmapping            ;skip unmapping if it's zero

        ; free 16:16 mapping for lParam2
        push     eax                    ;save CplApplet return value
	push	 dword ptr lParam2
	call	 UnMapLS
        pop      eax                    ;restore CplApplet return value

no_unmapping:
endm
