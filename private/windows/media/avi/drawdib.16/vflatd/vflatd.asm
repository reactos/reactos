PAGE 58,132
;******************************************************************************
TITLE VFLATD.ASM - Virtual Flat Device
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1987, 1988, 1989
;
;   Title:      VFLATD.ASM - Virtual flat video buffer Device
;
;           This virtual device simulates a flat video display buffer on
;           hardware that is designed to map video memory 64k at a time
;           at segment A000.  The video card is assumed to have 512K VRAM
;
;           It does this by alocating 512k of linear address space and
;           handling the pages faults in this memory
;
;           Our PageFault handler is last in the chain, it will only be called
;           by Win386 on pages it does not reconize (ie PT_RESERVED2 pages)
;           this keeps the PageFault overhead to a minimum.
;
;           Only 16 pages are marked present at a time these corespond to
;           the current 64k hunk of memory mapped by the video card at A0000.
;
;           When a PageFault occurs the 16 curently present pages are marked
;           not present and a new set of 16 are marked as present.  The
;           Video hardware is instructed to map in the 64k page that caused
;           the page fault.
;
;   PM API Services:
;
;       Get_Ver (DX = 0000)
;
;           returns
;               AX      version of VFLATD  (1.0)
;
;       Get_Video_Base (DX = 0001)
;
;           returns
;               AX      selector to the video buffer
;               EDX     Linear Address of video buffer
;
;       Reset (DX = 0002)
;           Assume the hardware 64k bank has been modifed and readjust
;           the PageTable mapping
;
;           returns
;               nothing
;
;   Version:    1.00
;
;   Date:       15-Sep-1989
;
;   Author:     LDS
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Sep-1989 LDS Original
;
;==============================================================================
;
;   DESCRIPTION:
;
;
;******************************************************************************

	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

IFDEF WIN31
%OUT SUPPORT FOR Windows 3.1
WIN31COMPAT=1
ENDIF
        .XLIST
	INCLUDE VMM.Inc
;;      INCLUDE Debug.Inc
        INCLUDE VFLATD.Inc
        .LIST

;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device VflatD,   \
        <VflatD_Version shr 8>,  \
        <VflatD_Version and 255>,\
        VFLATD_Control,          \
        VFLATD_Device_ID,        \
        UNDEFINED_INIT_ORDER,    \
        0,                       \
        VFLATD_PM_API_Handler

;******************************************************************************
;			    L O C A L	D A T A
;******************************************************************************

VxD_LOCKED_DATA_SEG

VFLATD_Video_Sel	    dw 0	; GDT Selector to virtual frame buffer

VFLATD_SlidingWindowBase    dd ?        ;base of 64k sliding window
VFLATD_GDTBase              df ?        ;base of GDT
VFLATD_VideoSelGDT	    dd ?	;ptr to video sel's GDT entry

VxD_LOCKED_DATA_ENDS

IFDEF WIN31
 VIDEO_BASE                 equ 0A0000h     ; Physical start of VRAM
 VIDEO_LIMIT                equ 0100000h-1  ; 1M VRAM
 VIDEO_NPAGES               equ ((VIDEO_LIMIT + 4095) / 4096)
ENDIF

MAXBANKSWITCHSIZE equ 100               ;maximum size of bank switch code

;******************************************************************************
;	       D E V I C E   C O N T R O L   P R O C E D U R E
;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VFLATD_Control
;
;   DESCRIPTION:
;       This is the VFLATD's control procedure.
;
;   ENTRY:
;	EAX = Control call ID
;
;   EXIT:
;	If carry clear then
;	    Successful
;	else
;	    Control call failed
;
;   USES:
;	EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;==============================================================================

BeginProc VFLATD_Control

        Control_Dispatch Device_Init, VFLATD_Device_Init

IFDEF DEBUG
        Control_Dispatch Debug_Query, VFLATD_Debug_Query
ENDIF
	clc					; Ignore other control calls
	ret

EndProc VFLATD_Control

;******************************************************************************
;  A P I   H A N D L E R S   F O R   P M    V M ' S
;******************************************************************************

VxD_DATA_SEG

VFLATD_API_Call_Table LABEL DWORD
        dd      OFFSET32 VFLATD_PM_API_Get_Ver
        dd      OFFSET32 VFLATD_PM_API_Get_Video_Base
        dd      OFFSET32 VFLATD_PM_API_Reset

VFLATD_Max_API_Function = ($ - VFLATD_API_Call_Table) / 4 - 1

VxD_DATA_ENDS

;******************************************************************************
;
;   VFLATD_Device_Init
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;       C for error
;
;   USES:
;	All registers and flags
;
;==============================================================================

BeginProc VFLATD_Device_Init

        clc
	ret

EndProc VFLATD_Device_Init

;******************************************************************************
;
;   VFLATD_PM_API_Handler
;
;   DESCRIPTION:
;
;   ENTRY:
;	EBX = Handle of VM that called API
;	EBP -> Client register structure
;
;   EXIT:
;	Client registers and flags may be altered
;
;   USES:
;	All registers and flags
;
;==============================================================================

BeginProc VFLATD_PM_API_Handler

	movzx	eax, [ebp.Client_DX]
        cmp     eax, VFLATD_Max_API_Function       ; Q: Is this a valid function?
        ja      SHORT VFLATD_PM_API_Failed
        CallRet VFLATD_API_Call_Table[eax*4]

VFLATD_PM_API_Failed:
	or	[ebp.Client_EFlags], CF_Mask
	ret

VFLATD_PM_API_Success:
	and	[ebp.Client_EFlags], NOT CF_Mask
	ret

EndProc VFLATD_PM_API_Handler

;******************************************************************************
;
;   VFLATD_PM_API_Get_Ver
;
;   DESCRIPTION:
;       Return the VFLATD version
;
;   ENTRY:
;	Client_DX = 0
;
;   EXIT:
;	Client_AX = 200h
;	Client carry flag clear
;
;   USES:
;	EAX, Flags, Client_AX, Client_Flags
;
;==============================================================================

BeginProc VFLATD_PM_API_Get_Ver

        mov     [ebp.Client_AX], VflatD_Version
        jmp     VFLATD_PM_API_Success

EndProc VFLATD_PM_API_Get_Ver

;******************************************************************************
;
;   VFLATD_PM_API_Get_Video_Base
;
;   DESCRIPTION:
;       Return a GDT selector to the flat video buffer
;
;   ENTRY:
;	Client_DX = 1
;	Client_AX = # of pages of video memory
;	Client_CX = size of bank switch code
;	Client_ES:DI -> bank switch code
;~~~~~ document guidlines for bank switch code
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer (not usable)
;	Client carry flag clear
;
;   USES:
;	EAX, Flags, Client_AX, Client_Flags
;
;==============================================================================

BeginProc VFLATD_PM_API_Get_Video_Base

        cmp     [ebp.Client_CX],MAXBANKSWITCHSIZE
        ja      VFLATD_PM_API_Failed             ;bank switch code too big

	cmp	[VFLATD_Video_sel], 0
	jne	already_initialized

IFNDEF WIN31
	movzx	eax, [ebp.Client_AX]
	add	eax, eax
	add	eax, 64/4
	push	eax
	VMMCall _MMReserve, <PR_SYSTEM, eax, PR_FIXED>
	pop	ecx
	inc	eax
	jz	VFLATD_PM_API_Failed
	add	eax, (64*1024-2)
ELSE

        ; allocate a hunk of linear address space to use as a virtual
        ; frame buffer  eax = handle, edx = linear addr
        ;

        VMMCall _MapPhysToLinear, <3*1024*1024*1024 - (2*1024*1024+64*1024),2*1024*1024+64*1024,0>
        or      eax, eax
        jz      VFLATD_PM_API_Failed
        add     eax,(64*1024-1)
ENDIF


; round up to 64Kb boundary

	and	eax, 0FFFF0000h
	mov	edx, eax

        ;   Save linear addr of video buffer
        ;
	mov	esi, edx

IFNDEF WIN31
	mov	[VFLATD_Video_Base], edx
	sub	ecx, 64/4
	shl	ecx, 12 		; # of bytes in allocated region
	dec	ecx
	lea	ecx, [edx+ecx]
	mov	[VFLATD_Video_Top], ecx

	shr	esi, 12 		; virt page # of start of region
	movzx	ecx, [ebp.Client_AX]
	add	esi, ecx
	VMMCall _MMCommitPhys, <esi, 16, 0A0h, PC_INCR+PC_USER+PC_WRITEABLE>
	or	eax, eax
	jz	VFLATD_PM_API_Failed

ELSE
        mov     [VFLATD_Video_Base],edx
        mov     eax,edx
        add     eax,2*VIDEO_LIMIT+1
        mov     [VFLATD_Video_Top],eax

        ; NOTE I dont handle the case where the frame buffer
        ; crosses multiple page tables, if it does, FAIL
        ;
        shr     edx,12
        and     edx,3FFh                ; edx = page number
        cmp     edx,0400h - VIDEO_NPAGES
        jg      VFLATD_PM_API_Failed

        ;   find the PTE that describes the base of video memory
        ;
        mov     eax,cr3                 ; eax --> Physical base of page dir
        VMMCall _MapPhysToLinear, <eax,4*1024,0>
        mov     edi,eax                 ; edi --> Linear base of page dir

        mov     eax,esi                 ; eax = linear video buffer base
        shr     eax,22                  ; eax = page dir number
        mov     edi,[edi + 4*eax]       ; edi = page dir entry (PDE)
        and     edi,not 0FFFh           ; edi = physical addr of PT

        mov     eax,esi                 ; eax = linear video buffer base
        shr     eax,12
        and     eax,3FFh                ; eax = page number
        lea     edi,[edi + 4*eax]       ; edi = physical addr of PTE

        VMMCall _MapPhysToLinear, <edi,VIDEO_NPAGES*4,0>

        ; now eax contains the Linear addr of our page table
        ;
        ; mark all pages as not present and set the page type to PT_RESERVED2
        ;
        mov     edi,eax
        mov     eax,PG_RESERVED2 OR P_WRITE OR P_USER
        mov     ecx,VIDEO_NPAGES 
        rep     stosd

; mark the next 64K present and map it to the 64k of video memory.

        m = 0
        n = 0

REPT 16
        mov     dword ptr [edi+m],((VIDEO_BASE+n) OR PG_RESERVED2 OR P_AVAIL)
        n = n + 1000h
        m = m + 4
ENDM
        add     edi,m

; mark the next (VIDEO_NPAGES - 16) pages as not present.

        mov     eax,PG_RESERVED2 OR P_WRITE OR P_USER
        mov     ecx,VIDEO_NPAGES - 16
        rep     stosd

ENDIF

        ; allocate a GDT selector to give to PM programs
        ;
	mov	eax, [VFLATD_Video_Base]
	movzx	ecx, [ebp.Client_AX]
	dec	ecx
	VMMCall _BuildDescriptorDWORDs, <eax,ecx,RW_Data_Type,D_GRAN_PAGE,0>
	VMMCall _Allocate_GDT_Selector, <edx,eax,0>
	mov	[VFLATD_Video_sel],ax
	or	eax, eax
	jz	VFLATD_PM_API_Failed

	mov	edx, [VFLATD_Video_Base]
	shr	edx, 16
	mov	[VFLATD_SlidingWindowBase], edx
	add	edx, (1024*1024 SHR 16)
	mov	[VFLATD_PresentWindowBase], edx

        sgdt    [VFLATD_GDTBase]
	mov	edx, dword ptr [VFLATD_GDTBase+2]
	and	eax, 0fff8h
	add	edx, eax		;-> base of sel's GDT entry
	add	edx, 4
	mov	[VFLATD_VideoSelGDT_B2], edx
	add	edx, 3
	mov	[VFLATD_VideoSelGDT_B3], edx

	pushfd
	cli
        sidt    [VFLATD_GDTBase]
        mov     eax, dword ptr [VFLATD_GDTBase+2]
        add     eax, 14*8
        mov     esi, OFFSET32 VFLATD_Direct_PageFault_Handler
        xchg    word ptr [eax], si      ; LSW
        ror     esi, 16
        xchg    word ptr [eax+6], si    ; MSW
	ror	esi, 16 		; esi = old handler offset
	sub	esi, OFFSET32 next_handler + 4
	mov	[next_handler], esi
	popfd

; make space for copying in the device specific bank switch code in the page
; fault handler and copy the code in.

        mov     esi, OFFSET32 VFLATD_EndOfDPH - 1
        movzx   edi, [ebp.Client_CX]
        add     edi, esi
        std
        mov     ecx, VFLATD_EndOfDPH - VFLATD_DPH_BankSwitchCode
        rep     movsb
        cld

	Client_Ptr_Flat esi, ES, DI
        mov     edi, OFFSET32 VFLATD_DPH_BankSwitchCode
        movzx   ecx, [ebp.Client_CX]
        rep     movsb

already_initialized:
        mov     ax, [VFLATD_Video_Sel]
        mov     [ebp.Client_AX], ax

        mov     eax, [VFLATD_Video_Base]
	mov	[ebp.Client_EDX], eax

        jmp     VFLATD_PM_API_Success

EndProc VFLATD_PM_API_Get_Video_Base

;******************************************************************************
;
;   VFLATD_PM_API_Reset
;
;   DESCRIPTION:
;       Called when Video card 64k page mapping has changed by someone other
;       than  VFLATD,  We mark not present any pages we think are currently
;       mapped to the hardware.
;
;   ENTRY:
;       Client_DX = 2
;
;   EXIT:
;	Client carry flag clear
;
;   USES:
;	EAX, Flags, Client_AX, Client_Flags
;
;==============================================================================

BeginProc VFLATD_PM_API_Reset

        ;   pretend we got a page fault on page zero so the bank gets
        ;   setup right.
        ;
        ;   we need to fake up a fault frame, and jump into the
        ;   fault handler.

;       int 3

        pushfd                                      ; iretd frame flags
        push    cs
        push    OFFSET32 VFLATD_PM_API_Success      ; iretd frame return
        push    0                                   ; fault type
        push    0                                   ; saved eax (cr2)
        mov     eax,[VFLATD_Video_Base]             ; faulting address (fake cr2)
        jmp     short VFLATD_Handle_Fault

EndProc VFLATD_PM_API_Reset

;******************************************************************************
;	  H A R D W A R E   I N T E R R U P T	P R O C E D U R E S
;******************************************************************************

;******************************************************************************
;
;   BeginProc VFLATD_Direct_PageFault_Handler
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:
;
;   USES:
;
;==============================================================================

BeginProc VFLATD_Direct_PageFault_Handler, NO_PROLOG, HIGH_FREQ

        push    eax
        mov     eax,cr2

        ;   Is the fault for us? or in other words does it fall in our
        ;   virtual video buffer.
        ;
        cmp     eax,12345678h
VFLATD_Video_Base equ dword ptr $-4

        jb      short VFLATD_Chain_Fault

        cmp     eax,12345678h
VFLATD_Video_Top equ dword ptr $-4

        jb      short VFLATD_Handle_Fault

VFLATD_Chain_Fault:     ;Chain the PageFault to the next guy in the chain
	pop	eax
;;	jmp	next_handler
        db      0E9h
next_handler	dd  ?

        ;   NOTE we jump here from VFLATD_PM_API_Reset

VFLATD_Handle_Fault:
        push    ds
        push    es
        push    edx

        ;   Calculate what 64k bank the fault occured in.
        ;
	shr	eax, 16
	sub	eax, ss:[VFLATD_SlidingWindowBase]  ; eax = bank #

;==============================================================================
; >>>>>>>>>>>>> BANK SWITCH CODE GETS INSERTED HERE <<<<<<<<<<<<<<<<<<
;==============================================================================
VFLATD_DPH_BankSwitchCode label byte

; we shift the code from here upto the label "VFLATD_EndOfDPH" down to
; accomodate device specific bank switch code.

;==============================================================================

	mov	edx, 12345678h
VFLATD_PresentWindowBase equ dword ptr $-4

	sub	edx, eax
	mov	ss:[VFLATD_SlidingWindowBase], edx
;
; modify the selector base
;
        mov     ss:[12345678h], dl
VFLATD_VideoSelGDT_B2 equ dword ptr $-4

        mov     ss:[12345678h], dh
VFLATD_VideoSelGDT_B3 equ dword ptr $-4

        pop     edx
        pop     es
        pop     ds
        pop     eax
	add	esp, 4
        iretd

EndProc VFLATD_Direct_PageFault_Handler

VFLATD_EndOfDPH label byte
        db      MAXBANKSWITCHSIZE dup (?) ;space for handler to grow when bank
                                          ; switching 

VxD_LOCKED_CODE_ENDS

;******************************************************************************
;		       D E B U G G I N G   C O D E
;******************************************************************************

;******************************************************************************
;
;   VFLATD_Debug_Query
;
;   DESCRIPTION:
;       This procedure is only assembled in the debug version of VFLATD.
;
;	Note that since this procedure can be called at any time by the
;	debugger it is not allowed to call any non-asynchronous services and
;	it must be in a LOCKED code segment.
;
;   ENTRY:
;	EAX = Debug_Query (equate)
;
;   EXIT:
;	None
;
;   USES:
;	EBX, ECX, EDX, ESI, Flags
;
;==============================================================================

IFDEF DEBUG

VxD_LOCKED_CODE_SEG

BeginProc VFLATD_Debug_Query

;       Trace_Out "******* VFLATD Debug_Query *******"

        mov     eax, VFLATD_Video_Base
;       Trace_Out "Video_Base:   #EAX"

        mov     ax, VFLATD_Video_Sel
;       Trace_Out "Video_Sel:    #AX"

VFLATD_DQ_Exit:
	ret

EndProc VFLATD_Debug_Query

VxD_LOCKED_CODE_ENDS

ENDIF

	END
