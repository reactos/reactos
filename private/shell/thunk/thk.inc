f16ptr  typedef ptr far16
f32ptr  typedef ptr far32


;==============================================================================
; BODY_PARAM_16
; Macro to generate equate for parameter on stack
;
;==============================================================================

BODY_PARAM_16 macro name, offset
name    equ <[bp+offsetThkParams+offset]>
        endm

;==============================================================================
; CB_THRU_COMMON
; Push address to be called and jump to common entry to 32bit code
;
;==============================================================================

CB_THRU_COMMON macro addr
        local   ret_addr
        local   log_msg_before
        local   log_msg_after

        externDef W32S_BackTo32:near32

ifdef DEBUG
LogCBThkSL  proto  near stdcall, psz:DWORD

        push    offset log_msg_before
        call    LogCBThkSL
endif

        push    ret_addr
        push    addr

; BUGBUG [KevinR] 12-Oct-1993
; This hardcoded offset is a hack for M5.  We should use a proper kernel
; dispatcher.
        assume  fs:nothing
Win16LockCount  equ  <word ptr fs:[30]> ; BUGBUG  from k32share.inc
        mov     di,-1
        xchg    di,Win16LockCount

        ; PERFORMANCE! is is possible to jump directly thru import thunk
        ; W32S_BackTo32 just does a retn
        jmp     W32S_BackTo32

ifdef DEBUG
log_msg_before  db  "SL before",0
log_msg_after   db  "SL after",0
endif

ret_addr:
        mov     Win16LockCount,di
        assume  fs:error

ifdef DEBUG
        push    offset log_msg_after
        call    LogCBThkSL
endif

        endm



;==============================================================================
; local macro, maps 32-bit pointer to 16-bit pointer,
;
; Guarantees:
;     If &SaveAddr& referenes di, the value of di entering MAP_POINTER
;     is used.
;==============================================================================
MAP_POINTER  macro   SaveAddr
    lodsd   ds:[esi]

    push    eax
    call    MapLS

ifnb <SaveAddr>
	mov	&SaveAddr&,eax
endif
    stosd   es:[di]              ; dst in 16:16 stack
endm

;==============================================================================
; local macro, clean up after MAP_POINTER.
;
; Guarantees:
;     If &SaveAddr& references si, the value of si entering UMAP_POINTER
;     is used.
;==============================================================================
UMAP_POINTER    macro   SaveAddr
ifnb <SaveAddr>
	push	es
	pushd	&SaveAddr&
	call	UnmapLS
	pop	    es
endif

    add     si,4
    add     edi,4
endm


;==============================================================================
; local macro, gets current process hInstance if null
; Win32/NT allows NULL hInstance where win3.1 doesn't
;
;==============================================================================
MAP_NULL_HINST  macro   hInst
        local   not_null
;        externDef GetNullhInst:far16
        externDef MaphInstLS:far16
        ifnb    <hInst>
        mov     eax, hInst
        endif
;        call    GetNullhInst
	call	MaphInstLS
not_null:
endm

;==============================================================================
; local macro, gets current process hInstance if null
; Win32/NT allows NULL hInstance where win3.1 doesn't
;
;==============================================================================
UMAP_NULL_HINST  macro   hInst
        local   not_null
        externDef MaphInstSL:far16
        ifnb    <hInst>
        mov     ax, hInst
        endif
	call	MaphInstSL
not_null:
endm



;==============================================================================
; Convert either an instance handle, a global Win16 handle or junk.
; Used by commdlg.
;==============================================================================
MAP_CD_NULL_HINST  macro   hInst,isInst
        local   not_inst,exit
	cmp	isInst,0
	jz	not_inst
	MAP_NULL_HINST	hinst
	jmp	exit
	
not_inst:  ;"hInst" is really 32-bit junk, or a zero-extended global handle.
	;; No need to do anything: "ax" already contains lo-word.
exit:
endm


;==============================================================================
; Convert either an instance handle, a global Win16 handle or junk.
; Used by commdlg.
;==============================================================================
UMAP_CD_NULL_HINST  macro   hInst,isInst
        local   not_inst,exit
	cmp	isInst,0
	jz	not_inst
	UMAP_NULL_HINST	hinst
	jmp	exit
	
not_inst:  ;"hInst" is really junk, or a global handle.
	ror	eax,16
	xor	ax,ax
	ror	eax,16
exit:
endm



;==============================================================================
; local macro, maps 32-bit call-back function pointer to 16-bit
; call back function pointer,
; check if it is a NULL pointer before calling AllocCallBack.
;==============================================================================
MAP_CALLBACK  macro   CallBackType:req
    lods    dword ptr ds:[esi]              ;wndproc must have a 16-bit
    push    bx                              ;save BX
    push    es                              ;save ES

    push    eax                             ;push address as parameter
    push    dword ptr CallBackType          ;push cb type as parameter
    call    AllocCallback                   ;DX:AX = 16-bit cb address
    pop     es                              ;restore ES
    pop     bx                              ;restore BX

    stos    dword ptr es:[di]               ;pass on NULL pointer
endm

;==============================================================================
; local macro, free resources allocated for a call-back function
; check if it is a NULL pointer before calling FreeCallBack.
; eax contains the 32-bit address of the mapped call-back function
;==============================================================================
FREE_CALLBACK  macro    iCallbackType:req
endm

;==============================================================================
; local macro, maps byte size field to byte size field
;==============================================================================
MAP_BYTETOBYTE  macro
    lodsb   ds:[esi]
    stosb   es:[di]
endm

;==============================================================================
; local macro, maps byte size field to byte size field
;==============================================================================
UMAP_BYTETOBYTE  macro
    lodsb   ds:[si]
    stosb   es:[edi]
endm

;==============================================================================
; local macro, maps word size field to word size field
;==============================================================================
MAP_WORDTOWORD  macro
    lodsw   ds:[esi]
    stosw   es:[di]
endm

;==============================================================================
; local macro, maps word size field to word size field
;==============================================================================
UMAP_WORDTOWORD  macro
    lodsw   ds:[si]
    stosw   es:[edi]
endm

;==============================================================================
; local macro, maps dword size field to word size field
;==============================================================================
MAP_DWORDTOWORD  macro
    lodsd   ds:[esi]
    stosw   es:[di]
endm

;==============================================================================
; local macro, maps dword size field to word size field
;==============================================================================
UMAP_WORDTODWORD  macro
    lodsw   ds:[si]
    movzx   eax,ax
    stosd   es:[edi]
endm

;==============================================================================
; local macro, maps long size field to int size field
;==============================================================================
MAP_LONGTOINT  macro
    lodsd   ds:[esi]
    stosw   es:[di]
endm

;==============================================================================
; local macro, maps int size field to long size field
;==============================================================================
UMAP_INTTOLONG  macro
    lodsw   ds:[si]
    cwde
    stosd   es:[edi]
endm

;==============================================================================
; local macro, maps dword size field to dword size field
;==============================================================================
MAP_DWORDTODWORD  macro
    movsd   es:[di], ds:[esi]
endm

;==============================================================================
; local macro, maps dword size field to dword size field
;==============================================================================
UMAP_DWORDTODWORD  macro
    movsd   es:[edi], ds:[si]
endm

;==============================================================================
; The following are variants of the above macros that are better suited for
; repacking structures in-place.
;==============================================================================

;==============================================================================
; local macro, maps 32-bit call-back function pointer to 16-bit
; call back function pointer,
; check if it is a NULL pointer before calling AllocCallBack.
;==============================================================================
MAP_CALLBACK_32_16_IP  macro   reg:=<ds>,CallBackType:req
    lods    dword ptr reg:[si]              ;wndproc must have a 16-bit

    push    bx
    push    es
    push    eax
    push    dword ptr CallBackType
    call    AllocCallback
    pop     es
    pop     bx

    stosd
endm

;==============================================================================
; local macro, maps 16-bit call-back function pointer to 32-bit
; call back function pointer,
; check if it is a NULL pointer before calling AllocCallback.
;
; ASSUMES DIRECTION FLAG DOWN, i.e. STD.
;==============================================================================
MAP_CALLBACK_16_32_IP  macro   reg:=<ds>,CallBackType:req
    lods    dword ptr reg:[si]              ;wndproc must have a 16-bit

    push    bx
    push    es
    push    eax
    push    dword ptr CallBackType
    call    AllocCallback32
    pop     es
    pop     bx

    stosd
endm

;==============================================================================
; local macro, maps 32-bit pointer to 16-bit pointer,
;==============================================================================
MAP_POINTER_32_16_IP  macro   reg:=<ds>
    lodsd   reg:[si]
    push    eax
    call    MapLS
    stosd   es:[di]
endm


;==============================================================================
; MAP_HINST_32_16_IP
; Map an instance handle from 32->16 in-place.  If the original value is 0,
; replace it with the instance handle from win32s16.dll.
;
; Assumes ds = _DATA.
;==============================================================================
MAP_HINST_32_16_IP macro reg:=<ds>
        local   have_hinst

        lods    dword ptr reg:[si]              ; convert hinst to word
        or      ax,ax                           ; translate to hComboInst if
        jnz     have_hinst                      ;     null
        mov     ax,hComboInst
have_hinst:
        stosw
endm

;==============================================================================
; MAP_HINST_16_32_IP
; Map an instance handle from 16->32 in-place.  If the original value is
; hComboInst, replace it with 0.
;
; Assumes ds = _DATA.
;==============================================================================
MAP_HINST_16_32_IP macro reg:=<ds>
        local   have_hinst

        lods    word ptr reg:[si]               ; convert hinst to word
        cmp     ax,hComboInst                   ; translate to null if
        jnz     have_hinst                      ;     hComboInst
        sub     ax,ax
have_hinst:
        movzx   eax,ax
        stosd
endm

;==============================================================================
; local macro, maps word size field to word size field
;==============================================================================
MAP_WORDTOWORD_IP  macro        reg:=<ds>
    movsw   es:[di], reg:[si]
endm

;==============================================================================
; local macro, maps dword size field to word size field
;==============================================================================
MAP_DWORDTOWORD_IP  macro reg:=<ds>
    lodsd   reg:[si]
    stosw   es:[di]
endm

;==============================================================================
; local macro, maps dword size field to word size field
;==============================================================================
MAP_LONGTOINT_IP  macro reg:=<ds>
    lodsd   reg:[si]
    stosw   es:[di]
endm

;==============================================================================
; local macro, maps dword size field to dword size field
;==============================================================================
MAP_DWORDTODWORD_IP  macro      reg:=<ds>
    movsd   es:[di], reg:[si]
endm

;==============================================================================
; local macro, maps word size field to dword size field by zero-extension
;==============================================================================
MAP_WORDTODWORD_IP  macro reg:=<ds>
    lodsw   reg:[si]
    movzx   eax,ax
    stosd   es:[di]
endm

;==============================================================================
; local macro, maps word size field to dword size field by sign-extension
;==============================================================================
MAP_INTTOLONG_IP  macro reg:=<ds>
    lodsw   reg:[si]
    cwde
    stosd   es:[di]
endm

;==============================================================================
; save all 16-bit registers, except dx:ax
;
;==============================================================================
SAVEALL     macro
    push    cx      ; save all 16-bit registers, except dx:ax
    push    bx
    push    bp
    push    si
    push    di
    push    ds
    push    es
endm


;==============================================================================
; restore all 16-bit registers, except dx:ax
;
;==============================================================================
RESTOREALL  macro  StackType:=<Stack16>
    POPW    es      ; restore all 16-bit registers, except dx:ax

    CHECKW  ds, StackType
    POPW    ds
    CHECKW  di, StackType
    pop     di

    CHECKW  si, StackType
    pop     si
    CHECKW  bp, StackType
    pop     bp

    pop     bx
    pop     cx
endm


;==============================================================================
; save all 32-bit registers, except eax
;
;==============================================================================
SAVEALL32     macro
    push    edx         ;save all 32-bit registers except eax
    push    ecx
    push    ebx
    push    ebp
    push    esi
    push    edi
    push    ds
    push    es
endm


;==============================================================================
; restore all 32-bit registers, except eax
;
;==============================================================================
RESTOREALL32  macro  StackType:=<Stack16>
    POPD    es      ; restore all 32-bit registers, except eax

    CHECKD  ds, StackType
    POPD    ds
    CHECKD  edi, StackType
    pop     edi
    CHECKD  esi, StackType
    pop     esi
    CHECKD  ebp, StackType
    pop     ebp
    CHECKD  ebx, StackType
    pop     ebx

    pop     ecx
    pop     edx
endm


;==============================================================================
; test two text macros for equality
;
;
;==============================================================================
TextEqual? macro   Text_1, Text_2
    ifidni <Text_1>, <Text_2>
        exitm <not 0>
    endif
    exitm <0>
endm


;==============================================================================
; test two text macros for difference
;
;
;==============================================================================
TextDiff? macro   Text_1, Text_2
    ifidni <Text_1>, <Text_2>
        exitm <0>
    endif
    exitm <not 0>
endm


;==============================================================================
; check a word on top of the stack
; if not equal, break
;
;==============================================================================
CHECKW    macro   CurrentReg, StackType:=<Stack16>
    local   skip_int3
    local   skip_another_int3

    if (@WordSize eq 4) and TextDiff? (&StackType&,Stack16)
    push    eax
    mov     ax,&CurrentReg&
    cmp     ax,word ptr [esp+4]
    pop     eax
    je      skip_int3
    int     3
skip_int3:
    else


    push    bp
    push    ax
    mov     ax,&CurrentReg&
    mov     bp,sp
    and     ebp,0ffffh
    cmp     ax,word ptr [ebp+4]
    pop     ax
    pop     bp
    je      skip_another_int3
    int     3
skip_another_int3:
    endif

endm


;==============================================================================
; check a dword on top of the stack
; if not equal, break
;
;==============================================================================
CHECKD    macro   CurrentReg, StackType:=<Stack32>
    local   skip_int3
    local   skip_another_int3

if 0
;!!! fix this
    if (@WordSize eq 4) and TextDiff? (&StackType&,Stack32)
    push    eax
    mov     ax,&CurrentReg&
    cmp     ax,word ptr [esp+4]
    pop     eax
    je      skip_int3
    int     3
skip_int3:
    else


    push    bp
    push    ax
    mov     ax,&CurrentReg&
    mov     bp,sp
    and     ebp,0ffffh
    cmp     ax,word ptr [ebp+4]
    pop     ax
    pop     bp
    je      skip_another_int3
    int     3
skip_another_int3:
    endif
endif
endm


;==============================================================================
; pop word
;
;==============================================================================
POPW    macro   SegReg
    if @WordSize eq 4
        db      66h
    endif
    pop     SegReg
endm


;==============================================================================
; pop dword
;
;==============================================================================
POPD    macro   SegReg
    if @WordSize eq 2
        db      66h
    endif
    pop     SegReg
endm


;==============================================================================
; operand-size override
;
;==============================================================================
OTHER_OPERAND_SIZE  macro   arg
    db  66h
    arg
endm


;==============================================================================
; address-size override
;
;==============================================================================
OTHER_ADDRESS_SIZE  macro   arg
    db  67h
    arg
endm


;==============================================================================
; pop 16
;
;==============================================================================
POP16    macro   Reg
    if @WordSize eq 4
        irp curreg,<cs,ds,ss,es,fs,gs>
           ifidni <curreg>,<Reg>
                db      66h
           endif
        endm
    endif
    pop     Reg
endm


;==============================================================================
; allocate and public a byte flag
;
;==============================================================================
PubByte  macro   name, value
    public  name
    name    db      value
endm





;==============================================================================
; log a 16=>32 api call
;
;==============================================================================
APILOGSL  macro   argName
        local   exit,szLogMsg

ifdef DEBUG

LogApiThkSL	proto	near stdcall, psz:dword

        push    offset szLogMsg
        call    LogApiThkSL
	jmp	exit

szLogMsg  db	'&argName&',13,10,0

exit:
endif
endm



;==============================================================================
; log a 32=>16 api call, non-flat
;
;==============================================================================
APILOGLS  macro   argName
        local   exit,szLogMsg

ifdef DEBUG
externDef LogApiThkLS:far16

        push    ax

	push	cs
	push	offset szLogMsg
        call    LogApiThkLS

        pop     ax
	jmp	exit

szLogMsg  db	'&argName&',13,10,0

exit:
endif
endm



;==============================================================================
; log an api call, 16-bit
;
; BUGBUG [KevinR] 26-Aug-1993
; rip out this macro when we get rid of fNewDispatcherLS
;
;==============================================================================
APILOG16  macro   argName, argUnused, argComment
        local   exit,szApiName,szComment

ifdef DEBUG
externDef Log16BitThunkCall:far16

dsOffsetInCS CATSTR @code, <CodeData>

        push    ds
        push    ax
        mov     ds,cs:dsOffsetInCS

        push    cs
        push    offset szApiName
	push	cs
	push	offset szComment
        call    Log16BitThunkCall
        pop     ax
        pop     ds
	jmp	exit

szApiName db	'&argName& ',0
szComment db	'&argComment& ',0

exit:
endif
endm



;==============================================================================
; log an api call, 32-bit
;
;==============================================================================
APILOG  macro   argName, argFlag
        local   do_it,done,szApiName
ifdef DEBUG
;externDef _DbgPrint:near32

        ;;If argFlag is nonzero, print out the message.
        cmp     argFlag&,0
        jnz     do_it
        jmp     short done
;;Define the name here so we can pass it to _DbgPrint.
szApiName db '&argName&',0
do_it:
        push    offset FLAT:szApiName
        push    offset FLAT:szApiFmt
        ;call    _DbgPrint
        add     esp,2*4
done:
endif
endm


;==============================================================================
; log 16-bit api return, in 32-bit code
;
;==============================================================================
RETLOG  macro   argFlag
endm



;==============================================================================
; conditionally break
;
;==============================================================================
SWITCHABLE_INT3 macro argLabel, argFlag
        local   skip_int3

externDef   argLabel :far16

        push    ds
        push    ax
        mov     ax,seg &argFlag
        mov     ds,ax
        cmp     &argFlag,0
        je      skip_int3
argLabel& label far16
        int     3
skip_int3:
        pop     ax
        pop     ds
endm


;==============================================================================
;
;
;==============================================================================
STUB0   macro   module, argLabel, nBytes, argComment:=<stub0>
externDef   argLabel&16 :far16
argLabel&16 label far16

ifdef FSAVEALL
        SAVEALL
endif
        APILOG16 argLabel&16, f&module&ApiLog, argComment
ifdef INT3
        SWITCHABLE_INT3 argLabel&_stub, f&module&Int3
endif
        xor     ax,ax
        cwd
ifdef FSAVEALL
        RESTOREALL
endif
        retf    &nBytes&
endm


;==============================================================================
;
;
;==============================================================================
STUB    macro   module, argLabel, nBytes, nRetAX, argComment:=<stub>
externDef   argLabel&16 :far16
externDef   PCodeDebug16 :far16
argLabel&16 label far16

ifdef FSAVEALL
        SAVEALL
endif
        APILOG16 argLabel&16, f&module&ApiLog, argComment nRetAX
ifdef INT3
        SWITCHABLE_INT3 argLabel&_stub, f&module&Int3
endif
        mov     ax,&nRetAX
ifdef FSAVEALL
        RESTOREALL
endif
        retf    nBytes
endm

;==============================================================================
; entry code for flat common callback
;
;==============================================================================
CALLBACK_PROLOGUE   macro

        pop     eax                     ; 16:16 callback
        pop     edx                     ; eip, API32

        push    cs                      ; flat cs
        push    edx                     ; eip, API32
        push    eax                     ; 16:16 callback

        push    ebp
        mov     ebp,esp

        push    ds                      ; save registers
        push    es
        push    ebx
        push    edi
        push    esi
endm


;==============================================================================
; exit code for flat common callback
;
;==============================================================================
CALLBACK_EPILOGUE   macro   size

        LOCAL   bad_esp

;--------------------------------------------------
; switch stacks and jump to 16:16 callback

; when the 16:16 callback does a retf, we will hit our cleanup routine

        push    dword ptr ADDR_THK_CLEANUP_&size

; prepare to transfer to the 16-bit callback function

        push    pCallback16

; get the ss16 we had when we entered the callback API16
; make the 16-bit ss:sp point to the same linear address as the flat ss:esp

        call    UsrQuerySS16
        mov     esi,eax                 ; save ss16
        push    eax
        call    GetSelectorBase32       ; LATER: LDT lookup
        xchg    eax,esp
        sub     eax,esp
        jb      bad_esp
        cmp     eax,65535
        ja      bad_esp

        mov     ss,si
        mov     sp,ax

; effectively, jmp to 16:16 callback
        retw

bad_esp:
        int     3

endm




;==============================================================================
; save flat stack and thunkID
;
;==============================================================================
SAVE_STACK_AND_THUNKID  macro

        lea     eax,[addr_registers]    ; save flat stack
        push    ss
        push    eax

        call    GetThunkID32
        push    eax                     ; save 16:16 thunkID
endm

;==============================================================================
; 32-bit callback cleanup code
;
;==============================================================================
CALLBACK_CLEANUP32    macro   size
externDef CALLBACK_CLEANUP_&size&:near32
CALLBACK_CLEANUP_&size&:
;;;-----------------------------------------------------------------------
;;; DO NOT REMOVE THE OVERRIDE PREFIX. IT IS NEEDED TO GET PAST A BUG
;;; IN A CERTAIN BRAND OF CLONE CHIPS.
;;;-----------------------------------------------------------------------
        lss     sp,ss:[ebx]
        POP16   ds
        POP16   di
        POP16   si
        POP16   bp

        add     sp, 8                   ; pop dispatcher ptr+index


        OTHER_OPERAND_SIZE
        retf    size                    ; return to the 16-bit API
endm


;==============================================================================
; SetLastError mechanism
;
;==============================================================================
SETERROR     macro   Value:req, Error:req
        local   done

externDef SetLastError16:far16
        cmp     ax,Value
        jne     short done

        push    dword ptr Error
        call    SetLastError16
done:
endm

;-----------------------------------------------------------------------;
; MoveBytes -- generate code to move n consecutive bytes
;
; Entry:
;       DS:ESI --> source
;       ES:EDI --> destination
;-----------------------------------------------------------------------;
MoveBytes       macro   n
    local q, r

    q = n / 4
    r = n and 3

    if q
        if q lt 256
            mov     ecx,byte ptr q
        else
            mov     ecx,q
        endif
        rep     movs dword ptr es:[edi], dword ptr ds:[esi]
    endif

    if r
        mov     ecx,byte ptr r
        rep     movs byte ptr es:[edi], byte ptr ds:[esi]
    endif
endm

;-----------------------------------------------------------------------;
; ZeroBytes -- generate optimized code to set n consecutive bytes to 0
;
; Entry:
;       ES:EDI --> address at which to start
;-----------------------------------------------------------------------;
ZeroBytes       macro   n
    local q, r

    q = n / 4
    r = n and 3

        sub     eax,eax
    if q
        if q lt 256
            mov     ecx,byte ptr q
        else
            mov     ecx,q
        endif
        rep     stos dword ptr es:[edi]
    endif

    if r
        mov     ecx,byte ptr r
        rep     stos byte ptr es:[edi]
    endif
endm

;-----------------------------------------------------------------------;
; GMH2Sel
;
; This macro encapsulates the assumption that a global memory handle
; in win3.1 is either a selector (if fixed) or a selector with the
; low bit cleared (if moveable).  Therefore, it can be turned into a
; selector by always setting the low bit.
;-----------------------------------------------------------------------;
GMH2Sel         macro   reg:req, regMask1, regMask2
    ifb <regMask1>
        or      reg,1
    else
        .errb <regMask2>

        ;The following sequence uses two mask registers to
        ;convert only non-zero values of "reg" to selectors.
        mov     regMask1,1              ;mov cx,1
        cmp     reg,regMask1            ;cmp ax,cx      C=1 if AX=0
        cmc                             ;cmc            C=0 if AX=0
        sbb     regMask2,regMask2       ;sbb dx,dx      DX=0 if AX=0
        and     regMask1,regMask2       ;and cx,dx      CX=0 if AX=0
        or      reg,regMask1            ;or  ax,cx      AX=0 if AX=0
    endif
endm



;-----------------------------------------------------------------------;
; PACK_CALLBACK 
;-----------------------------------------------------------------------;

PACK_CALLBACK	macro	iOffset,iTempOffset,dwCallbackType

	local	exit
	local	not_null

	mov	esi,[bp+&iOffset&]
	or	esi,esi
	jnz	not_null

	sub	sp,18		;Space the thunklet would have taken
	jmp	exit

not_null:
; Push the following code on the stack:
;
;  66 68 xx xx xx xx    push  imm32 <flat lpCallback>
;  66 68 xx xx xx xx	push  imm32 <dwCallbackType>
;  90			nop		;for convenient
;  EA xx xx xx xx	jmp   CALLBACK_BODY_16

	mov	ax,seg CALLBACK_BODY_16
	push	ax
	mov	ax,offset CALLBACK_BODY_16
	push	ax
	push	0ea90h

	push	dword ptr &dwCallbackType&
	push	6866h

	push	esi
	push	6866h

	push	ss
	call	GetCSAlias
	mov	[bp-&iTempOffset&],sp
	mov	[bp-(&iTempOffset&)+2],ax

exit:

	
endm; PACK_CALLBACK


;-----------------------------------------------------------------------;
; UNPACK_CALLBACK 
;-----------------------------------------------------------------------;

UNPACK_CALLBACK	macro	iOffset,iTempOffset,dwCallbackType


	push	word ptr [bp-&iTempOffset&+2]
	call	FreeCSAlias


endm; UNPACK_CALLBACK




;-----------------------------------------------------------------------;
; TILE_BUFFER
;
;   Allocates overlapping tiling selectors for a huge memory block.
;
;   Inputs:
;       eax:        32-bit linear address (NULL is allowed)
;       ecx:        32-bit buffer size (if 0, eax is treated as NULL)
;
;   Outputs:
;	eax:	    upper 16 bits = first selector
;		    lower 16 bits = 0 (thus, eax is the 16:16 pointer)
;	ecx:	    upper 16 bits = first selector (same as upper eax)
;		    lower 16 bits = # of selectors allocated
;	CF:	    0 = success, 1 = failure.
;
;   Registers preserved:
;       bp, sp, all segment registers.
;
;   Special cases:
;       if buffer address is NULL or the size is 0, macro returns 
;          eax == ecx == 0, CF = 0.
;       if macro fails, it returns eax == 0, CF = 1.
;
;   Use UNTILE_BUFFER to deallocate tiling selectors.
;-----------------------------------------------------------------------;

TILE_BUFFER	macro
	extern	TileBuffer:far16	;Dynalink to win32c.dll
	call	TileBuffer
endm	;TILEBUFFER



;-----------------------------------------------------------------------;
; UNTILE_BUFFER
;
;   Releases tiling selectors allocated by TILE_BUFFER.
;
;   Inputs:
;	ecx == sel:count (as returned by TILE_BUFFER. 00:00 is legal).
;
;   Outputs:
;       None.
;
;   Registers preserved:
;       bp, sp, all segment registers
;
;   Special case:
;       UNTILE_BUFFER does nothing if sel:count == 00:00.
;-----------------------------------------------------------------------;
UNTILE_BUFFER	macro
	extern	UntileBuffer:far16	;Dynalink to win32c.dll
	call	UntileBuffer
endm	;UNTILE_BUFFER
