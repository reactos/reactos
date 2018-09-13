;	mdstring.inc -- MagicDrv strings definitions
;/*
; *			 Microsoft Confidential
; *			 Copyright (C) Microsoft Corporation 1992
; *			 All Rights Reserved.
; */

;	History:
;
;	    20-Jul-1992 chuckst 	initial version
;	    10-Aug-1992 bens		Changed strings
;	    03-Sep-1992 chuckst 	changed to DBLSPACE
;           26-Jul-1993 bens            Added DoubleGuard


IFDEF  OLD_NAMES
    dev_name_7	    equ     "DUBLDSK"	    ; last 7 chars of device name field
ELSE ; !OLD_NAMES
if 0
    ;;;WHY IS THIS CODE REMOVED? Because although it works fine for
    ;;;control panel, it also works fine for windows setup, which won't
    ;;;write the autoexec.bat and config.sys out on squish drives.
    ;;;Oh well.

    ;;;BUG BUGBUG warning we use the byte sequence S,Q,U,I,S,H,+ to
    ;;;make windows 31 control panel consider us ineligible for
    ;;;a permanent swap file
    dev_name_7	    equ     "SQUISH+"	    ; last 7 chars of device name field
endif
    dev_name_7	    equ     "DBLSPAC"
ENDIF ; OLD_NAMES

magic_ini_file_name equ '\dblspace.ini'

IFDEF  OLD_NAMES
    CVF_NAME_BASE   equ     '\dubldisk'
ELSE ; !OLD_NAMES
    CVF_NAME_BASE   equ     '\DBLSPACE'
ENDIF ; OLD_NAMES

KWD_ACT 	equ	'ACTIVATEDRIVE'
KWD_FIRST	equ	'FIRSTDRIVE'
KWD_LAST	equ	'LASTDRIVE'
KWD_MAXREM	equ	'MAXREMOVABLEDRIVES'
KWD_MAXFILE	equ	'MAXFILEFRAGMENTS'
KWD_ENABLE	equ	'ENABLE386'
KWD_ROMSERVER	equ	'ROMSERVER'
KWD_CHECKSUM    equ     'CHECKSUM'      ; NOTE: Undocumented!
KWD_DBLGUARD    equ     'DOUBLEGUARD'
KWD_SWITCHES	equ	'SWITCHES'
KWD_AUTOMOUNT	equ	'AUTOMOUNT'

NSWITCH_CHAR	=	'N'
FSWITCH_CHAR	=	'F'

OEM_LOWORD	equ	'2L'
OEM_MIDWORD	equ	'6.'
OEM_HIBYTE	equ	' '

IFDEF  OLD_NAMES
    MD_SIG_LOWORD   equ     'BD'
    MD_SIG_HIBYTE   equ     'L'

    MD_STAMP	    equ     "DD"
    OK_STAMP	    equ     'OK'
ELSE ; !OLD_NAMES
    MD_SIG_LOWORD   equ     044F8h  ; Low CVF signature is F8,'D','R'
    MD_SIG_HIBYTE   equ     'R'

    MD_STAMP	    equ     "SD"
    OK_STAMP	    equ     'OK'
ENDIF ; OLD_NAMES

; MD_EXT_STAMP is used in IOCTL & Int 2Fh calls to DblSpace driver -- it
; should not change even if MD_STAMP does.  MD_EXT_STAMP is the value
; external software expects to see and use.

MD_EXT_STAMP	equ	"DM"

; MD_MOUNT_CMD identifies activate records for use with the activate
; Int 2Fh API.	MS-DOS 6 DBLSPACE.BIN used a MD_MOUNT_CMD value of 'M'.
; The MS-DOS 6.2 activate record is not compatible with MS-DOS 6 so
; the comand value has been changed to cleanly fail mount/activate
; attempts with the MS-DOS 6 structure.

MD_MOUNT_CMD	equ	'A'
