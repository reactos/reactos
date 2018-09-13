;----------------------------------------------------------------------------
;
; DCIDDI.INC
;
; Copyright (c) 1994 Microsoft Corporation
;
; DCI 1.0 Interface Definitions
;
;----------------------------------------------------------------------------

ifndef _INC_DCIDDI
_INC_DCIDDI equ 1

DCICOMMAND      		equ 3075    ; GDI Escape for DCI
DCI_VERSION     		equ 0x0100  ; version number of DCI 1.0

;----------------------------------------------------------------------------
;  DCI command codes.
;----------------------------------------------------------------------------
DCICREATEPRIMARYSURFACE        	equ 1       ; used to create primary surface
DCICREATEOFFSCREENSURFACE      	equ 2       ; used to create offscreen surface
DCICREATEOVERLAYSURFACE		equ 3
DCIENUMSURFACE                 	equ 4       ; used to enumerate
DCIESCAPE			equ 5

;----------------------------------------------------------------------------
;  error codes
;----------------------------------------------------------------------------
DCI_OK                      	equ 0

DCI_FAIL_GENERIC             	equ (-1)
DCI_FAIL_UNSUPPORTEDVERSION  	equ (-2)
DCI_FAIL_INVALIDSURFACE      	equ (-3)
DCI_FAIL_NOTSUPPORTED        	equ (-4)

DCI_ERR_CURRENTLYNOTAVAIL   	equ (-5)
DCI_ERR_INVALIDRECT         	equ (-6)
DCI_ERR_UNSUPPORTEDFORMAT   	equ (-7)
DCI_ERR_UNSUPPORTEDMASK     	equ (-8)
DCI_ERR_TOOBIGHEIGHT        	equ (-9)
DCI_ERR_TOOBIGWIDTH         	equ (-10)
DCI_ERR_TOOBIGSIZE          	equ (-11)
DCI_ERR_OUTOFMEMORY         	equ (-12)
DCI_ERR_INVALIDPOSITION     	equ (-13)
DCI_ERR_INVALIDSTRETCH      	equ (-14)
DCI_ERR_INVALIDCLIPLIST     	equ (-15)
DCI_ERR_SURFACEISOBSCURED   	equ (-16)
DCI_ERR_XALIGN			equ (-18)
DCI_ERR_YALIGN			equ (-19)
DCI_ERR_XYALIGN			equ (-20)
DCI_ERR_WIDTHALIGN		equ (-21)
DCI_ERR_HEIGHTALIGN		equ (-22)
									 
DCI_STATUS_POINTERCHANGED    	equ 1
DCI_STATUS_STRIDECHANGED        equ 2
DCI_STATUS_FORMATCHANGED        equ 4
DCI_STATUS_SURFACEINFOCHANGED   equ 8
DCI_STATUS_CHROMAKEYCHANGED     equ 16
DCI_STATUS_WASSTILLDRAWING     	equ 32

;----------------------------------------------------------------------------
; Definitions for DCISURFACE::dwFlags
;----------------------------------------------------------------------------
DCI_SURFACE_TYPE   equ 0000000Fh  ; 16 types
DCI_PRIMARY        equ 00000000h  ; primary surface
DCI_OFFSCREEN      equ 00000001h  ; offscreen surface
DCI_OVERLAY        equ 00000002h  ; overlay surface

DCI_VISIBLE        equ 00000010h  ; surface is visible
DCI_CHROMAKEY	   equ 00000020h  ; chroma key surface
DCI_1632_ACCESS    equ 00000040h  ; must access using 16:32 pointers
DCI_DWORDSIZE      equ 00000080h  ; support only aligned dest. sizes
DCI_DWORDALIGN     equ 00000100h  ; support only aligned dest. positions
DCI_WRITEONLY      equ 00000200h  ; write access only.
DCI_ASYNC          equ 00000400h  ; supports async draw

DCI_CAN_STRETCHX   equ 00001000h  ; can stretchX
DCI_CAN_STRETCHY   equ 00002000h  ; can stretchY
DCI_CAN_STRETCHXY  equ 00003000h  ; can stretchXY
DCI_CAN_STRETCHXN  equ 00004000h  ; can stretchX by integer
DCI_CAN_STRETCHYN  equ 00008000h  ; can stretchY by integer
DCI_CAN_STRETCHXYN equ 0000C000h  ; can stretchXY by integer

DCI_CAN_OVERLAY    equ 00010000h  ; supports overlay.

;----------------------------------------------------------------------------
; common DCI command structure
;----------------------------------------------------------------------------
DCICMD  struc
	dciCommandCode  dd ?         ; DCI command
	dciDWParam1     dd ?         ; dwParam1
	dciDWParam2     dd ?         ; dwParam2
	dciVersion      dd ?         ; version
			dd ?         ; reserved
DCICMD  ends

;----------------------------------------------------------------------------
; used in DCICREATE... calls
;----------------------------------------------------------------------------
DCICREATEINPUT struc
	dcicCmd			db size DCICMD dup (?)
	dcicDWCompression  	dd ?                ; biCompression
	dcicDWMask         	dd 3 dup(?)         ; BI_BITFIELD masks
	dcicDWWidth        	dd ?                ; width in pixels
	dcicDWHeight       	dd ?                ; height in pixels
	dcicDWDCICaps	   	dd ?
	dcicDWSurface	   	dd ?
DCICREATEINPUT ends

;----------------------------------------------------------------------------
; used with DCIENUMINPUT
;----------------------------------------------------------------------------
DCIENUMINPUT struc
	dcieCmd             	db size DCICMD dup(?)
	dcieSourceRect      	dw 4 dup(?)         ; source
	dcieDestRect        	dw 4 dup(?)         ; dest
	dcieDWCallback         	dd ?                ; callback
	dcieDWCallbackParam    	dd ?                ; callback lParam
DCIENUMINPUT ends

;----------------------------------------------------------------------------
; DCISURFACE structure
;----------------------------------------------------------------------------
DCISURFACEINFO  struc
	dciDWSize                	dd  ?
	dciDWDCICaps                	dd  ?
	dciDWCompression          	dd  ?
	dciDWMasks                	dd  3 dup(?)
	dciDWWidth                	dd  ?
	dciDWHeight               	dd  ?
	dciLStride            		dd  ?
	dciDWBitCount             	dd  ?
	dciDWOffSurface        		dd  ?
	dciWSelSurface           	dw  ?
	dciWReserved             	dw  ?
	dciDWReserved1           	dd  ?
	dciDWReserved2           	dd  ?
	dciDWReserved3           	dd  ?
	dciDWBeginAccess         	dd  ?
	dciDWEndAccess            	dd  ?
	dciDWDestroySurface         	dd  ?
DCISURFACEINFO ends

;----------------------------------------------------------------------------
; used with DCICREATEOFFSCREENSURFACE
;----------------------------------------------------------------------------
DCIOFFSCREEN struc
	dcioInfo		 	db size DCISURFACEINFO dup (?)
	dcioDWDraw                 	dd  ?
	dcioDWSetClipList           	dd  ?
	dcioDWSetDestination       	dd  ?
DCIOFFSCREEN ends

;----------------------------------------------------------------------------
; used with DCICREATEOVERLAYSURFACE
;----------------------------------------------------------------------------
DCIOVERLAY struc
	dcivInfo			db size DCISURFACEINFO dup (?)
	dcivDWChromaKeyValue 		dd (?)
	dcivDWChromaKeyMask		dd (?)
DCIOVERLAY ends

endif	; _INC_DCIDDI


