; ***************************************************************************
;
;   Microsoft Confidential
;   Copyright (C) Microsoft Corporation 1993
;   All Rights Reserved.
;
; ***************************************************************************
;
; MODIFICATION HISTORY
;
; Created:	8/30/93	NSM	
; 
; 11/15/93	NSM	Modified and moved so that this can be used both
;			in BIOS and DwConfigmg VxD
;
; ***************************************************************************



INCLUDE	DEVINFO.INC

OLD_CONFIGMGR_VXDID	equ	34h	; should match with vmm.h

CM_VERSION_NO	equ	0101h		; CONFIGMGR Version no
 

;
;
; CONFIG API function codes
;
CM_GetVersion	EQU	0
CM_GetConfig	EQU	1
CM_LockConfig	EQU	2
CM_UnlockConfig	EQU	3

;
; Various error codes returned by CONFIGMGR API
;
CM_DEVICE_NOT_FOUND		equ	1

CM_CONFIG_ERROR			equ	1	
CM_IO_PORT_UNAVAILABLE		equ	2  
CM_IRQ_UNAVAILABLE		equ	4  
CM_DMA_CH_UNAVAILABLE		equ	8	
CM_MEM_WINDOW_UNAVAILABLE	equ	10h

