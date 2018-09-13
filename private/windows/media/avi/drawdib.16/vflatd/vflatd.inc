;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1988
;
;   Title:      VflatD.INC -
;
;   Version:	1.00
;
;   Date:	10-Aug-1988
;
;   Author:     LDS
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   10-Aug-1988 LDS Original
;
;==============================================================================

WIN31 equ 1                                 ; support Windows 3.1

VflatD_Chicago_ID   equ     011Fh           ; OEM #8 Device #31
VflatD_Windows_ID   equ     (0440h + 29d)   ; MMSYS OEM# device 30

if WIN31
VflatD_Device_ID    equ     VflatD_Windows_ID
else
VflatD_Device_ID    equ     VflatD_Chicago_ID
endif

VflatD_Version      equ (0100h + 50)        ;    1.50


VflatD_Get_Version  equ 0
VflatD_Get_Sel      equ 1
VflatD_Reset        equ 2
