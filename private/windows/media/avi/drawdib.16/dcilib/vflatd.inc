;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1993-1994
;
;   Title:	Virtual Flat Device Exported Services
;
;   Version:	2.00
;
;   Date:	8-Nov-1993
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   08-Nov-1993 RAP Original
;
;==============================================================================

IFDEF VMM_TRUE
Begin_Service_Table VFLATD

VFLATD_Service VFLATD_Get_Version, LOCAL
VFLATD_Service VFLATD_Unmap_Flat_Selector, LOCAL

End_Service_Table VFLATD
ENDIF

VflatD_Chicago_ID   equ     011Fh           ; OEM #8 Device #31
VflatD_Windows_ID   equ     (0440h + 29d)   ; MMSYS OEM# device 30

ifdef WIN31
VflatD_Device_ID    equ     VflatD_Windows_ID
VflatD_Version      equ     0160h           ; 1.60
else
VflatD_Device_ID    equ     VflatD_Chicago_ID
VflatD_Version      equ     0200h           ; 2.00
endif

;
;   PM API defines
;
VflatD_Query                        equ 0
VflatD_Get_Version                  equ 0
VflatD_Get_Sel                      equ 1
VflatD_Reset                        equ 2
VflatD_Create_Virtual_Frame_Buffer  equ 3
VflatD_Create_Physical_Frame_Buffer equ 4

;******************************************************************************
;
;   VFLATD_PM_API_Query
;
;   DESCRIPTION:
;       Query VFlatD info
;
;   ENTRY:
;       Client_DX = 0
;
;   EXIT:
;       Client_EAX = VFlatD version
;       Client_EBX = selector to the frame buffer
;       Client_ECX = size of frame buffer
;       Client_EDX = Bank size + flags?
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Get_Video_Base
;
;   DESCRIPTION:
;       Return a GDT selector to the flat video buffer
;
;   ENTRY:
;       Client_DX = 1
;       Client_AX = # of pages of video memory
;       Client_CX = size of bank switch code
;       Client_ES:DI -> bank switch code
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Create_Physical_Frame_Buffer
;
;   DESCRIPTION:
;       Return a GDT selector to a flat video buffer
;
;   ENTRY:
;       Client_DL    = 4
;       Client_DH    = flags
;       Client_EAX   = physical base of frame buffer
;       Client_ECX   = size of frame buffer (in bytes)
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Create_Virtual_Frame_Buffer
;
;   DESCRIPTION:
;       Return a GDT selector to the flat video buffer
;
;   ENTRY:
;       Client_DL    = 3
;       Client_DH    = flags
;       Client_EAX   = size of frame buffer (in bytes)
;       Client_EBX   = size of a bank (in bytes)
;       Client_ESI   = location of bank
;       Client_CX    = size of bank switch code
;       Client_ES:DI = ptr to bank switch code
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================
