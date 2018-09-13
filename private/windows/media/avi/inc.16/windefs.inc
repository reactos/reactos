;***************************************************************************
;                                                                          *
;   Copyright (C) 1983-1994 by Microsoft Inc.				    *
;                                                                          *
;***************************************************************************

;       Macros for disabling and restoring hardware interrupt enable flag
;
;       The LeaveCrit macro has been updated for the mask problem on
;       the 80286 processor.


EnterCrit   MACRO
        pushf
        cli
ENDM

LeaveCrit   macro   reg 		;;this macro will restore the state of
ifnb	<reg>				;;the interrupt flag to what is was
	pop	reg&x			;;before EnterCrit.  All other flags
	test	reg&h, 2		;;are discarded.
	jz	@f
	sti
@@:
else
	push	bp
	mov	bp, sp
	test	byte ptr [bp+3], 2
	jz	@f
	sti
@@:
	pop	bp
	popf
endif
	endm


if 0
POPFF	equ	<LeaveCrit>

LeaveCrit   MACRO
        POPFF
ENDM
endif

POPFF	MACRO				;;this macro will restore ALL flags,
	local	a			;;EXCEPT the interrupt flag, to
	jmp	$+3			;;their previous state
a       label   near
        iret
        push    cs
        call    a
ENDM




;***************************************************************************
;                                                                          *
;   Inquire data structures for Timer, Keyboard, Mouse and Cursor modules  *
;                                                                          *
;***************************************************************************

TIMERINFO       STRUC
tiResolution    DD      0       ; #microseconds each timer tick
TIMERINFO       ENDS

KBINFO          STRUC
kbRanges        DB      4 dup (0)  ; Far East ranges for KANJI
kbStateSize     DW      0       ; #bytes of state info maintained by TOASCII
KBINFO          ENDS


MOUSEINFO       STRUC
msExists        DB      0       ; true => mouse exists
msRelative      DB      0       ; true => relative coordinate
msNumButtons    DW      0       ; number of buttons on the mouse
msRate          DW      0       ; maximum rate of mouse input events
msXThresh       DW      0       ; threshold before acceleration
msYThresh       DW      0       ;
msXRes          DW      0       ; x resolution
msYRes          DW      0       ; y resolution
MOUSEINFO       ENDS


CURSORINFO      STRUC
dpXRate         DW      0       ; horizontal mickey/pixel ratio
dpYRate         DW      0       ; vertical mickey/pixel ratio
CURSORINFO      ENDS


;***************************************************************************
;                                                                          *
; Cursor data structure passed to OEM routines.  Defines a graphics display*
; cursor in terms of a hotspot, an AND mask and an XOR mask.  The hot      *
; spot defines the pixel within the cursor that is the cursor is "pointing"*
; to.  So when displaying a cursor at location X,Y the pixel that          *
; is the hot spot should be painted at that X,Y coordinate.  The "shape"   *
; of the cursor is defined by two pixel masks.  The first mask is ANDed    *
; with the bits in the display bitmap and the second mask is XORed with    *
; the result to determine the bits that will be placed in the display      *
; bitmap.  The bits for the masks are in the byte array that begins        *
; at the csBits field, with the AND mask bits first, followed by the       *
; XOR mask bits.  The csWidthBytes field is the width of ONE mask, in      *
; bytes.  Currently, MS-WIN will only generate cursors whose width and     *
; height are both 16.                                                      *
;                                                                          *
;***************************************************************************

cursorShape     STRUC
csHotX          DW      0
csHotY          DW      0
csWidth         DW      0
csHeight        DW      0
csWidthBytes    DW      0
csColor         DW      0
    ; Beginning of an array of bytes that contain the bits for the AND and
    ; XOR masks.  The first csHeight * csWidthBytes bytes contain the bits
    ; for the AND mask and the next csHeight * csWidthBytes bytes contain
    ; the bits for the XOR mask.
;csBits          DB  2*2*16 DUP (?)
cursorShape     ENDS
