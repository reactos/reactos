;==========================================================================;
;
;  mmddk.inc -- Include file for Multimedia Device Development
;
;  Version 4.00
;
;  Copyright (C) 1992-1994 Microsoft Corporation.  All Rights Reserved.
;
;--------------------------------------------------------------------------;
;
;  Define:         Prevent inclusion of:
;  --------------  --------------------------------------------------------
;  MMNOMIDIDEV     MIDI support
;  MMNOWAVEDEV     Waveform support
;  MMNOAUXDEV      Auxiliary output support
;  MMNOTIMERDEV    Timer support
;  MMNOJOYDEV      Joystick support
;  MMNOMCIDEV      MCI support
;  MMNOTASKDEV     Task support
;
;==========================================================================;

OFFSEL  STRUC
Off     dw  ?
Sel     dw  ?
OFFSEL  ENDS

LOHI    STRUC
Lo      dw  ?
Hi      dw  ?
LOHI    ENDS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                      Helper functions for drivers
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DCB_TYPEMASK                equ 0007h   ; callback type mask
DCB_NULL                    equ 0000h   ; unknown callback type

; flags for wFlags parameter of DriverCallback()
DCB_WINDOW                  equ 0001h   ; dwCallback is a HWND
DCB_TASK                    equ 0002h   ; dwCallback is a HTASK
DCB_FUNCTION                equ 0003h   ; dwCallback is a FARPROC
DCB_WINDOW32                equ 0004h   ; dwCallback is a WINDOW ; Internal

;  message sent by mmsystem to wodMessage(), widMessage(), modMessage(),
;  and midMessage() when it initializes the wave and midi drivers 

DRVM_INIT                   equ 100

;  message base for driver specific messages
;
DRVM_USER                   equ 4000h

ifndef  MMNOWAVEDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
;                      Waveform device driver support
; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; maximum number of wave device drivers loaded
MAXWAVEDRIVERS              equ 10


; waveform input and output device open information structure
WAVEOPENDESC struc
    wod_hWave         dw    ?        ; handle
    wod_lpFormat      dd    ?        ; format of wave data
    wod_dwCallback    dd    ?        ; callback
    wod_dwInstance    dd    ?        ; app's private instance information
WAVEOPENDESC ends

WODM_INIT                   equ DRVM_INIT
WIDM_INIT                   equ DRVM_INIT
WODM_USER                   equ DRVM_USER
WIDM_USER                   equ DRVM_USER

; messages sent to wodMessage() entry-point function
WODM_GETNUMDEVS             equ 3
WODM_GETDEVCAPS             equ 4
WODM_OPEN                   equ 5
WODM_CLOSE                  equ 6
WODM_PREPARE                equ 7
WODM_UNPREPARE              equ 8
WODM_WRITE                  equ 9
WODM_PAUSE                  equ 10
WODM_RESTART                equ 11
WODM_RESET                  equ 12 
WODM_GETPOS                 equ 13
WODM_GETPITCH               equ 14
WODM_SETPITCH               equ 15
WODM_GETVOLUME              equ 16
WODM_SETVOLUME              equ 17
WODM_GETPLAYBACKRATE        equ 18
WODM_SETPLAYBACKRATE        equ 19
WODM_BREAKLOOP              equ 20

; messages sent to widMessage() entry-point function
WIDM_GETNUMDEVS             equ 50
WIDM_GETDEVCAPS             equ 51
WIDM_OPEN                   equ 52
WIDM_CLOSE                  equ 53
WIDM_PREPARE                equ 54
WIDM_UNPREPARE              equ 55
WIDM_ADDBUFFER              equ 56
WIDM_START                  equ 57
WIDM_STOP                   equ 58
WIDM_RESET                  equ 59
WIDM_GETPOS                 equ 60

endif   ;ifndef MMNOWAVEDEV


ifndef  MMNOMIDIDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                         MIDI device driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; maximum number of MIDI device drivers loaded
MAXMIDIDRIVERS equ    10

MODM_INIT                   equ DRVM_INIT
MIDM_INIT                   equ DRVM_INIT
MODM_USER                   equ DRVM_USER  
MIDM_USER                   equ DRVM_USER  

; MIDI input and output device open information structure
MIDIOPENDESC struc
    mod_hMidi         dw    ?        ; handle
    mod_dwCallback    dd    ?        ; callback
    mod_dwInstance    dd    ?        ; app's private instance information
MIDIOPENDESC ends

; messages sent to modMessage() entry-point function
MODM_GETNUMDEVS             equ 1
MODM_GETDEVCAPS             equ 2
MODM_OPEN                   equ 3
MODM_CLOSE                  equ 4
MODM_PREPARE                equ 5
MODM_UNPREPARE              equ 6
MODM_DATA                   equ 7
MODM_LONGDATA               equ 8
MODM_RESET                  equ 9
MODM_GETVOLUME              equ 10
MODM_SETVOLUME              equ 11
MODM_CACHEPATCHES           equ 12      
MODM_CACHEDRUMPATCHES       equ 13


; messages sent to midMessage() entry-point function
MIDM_GETNUMDEVS             equ 53
MIDM_GETDEVCAPS             equ 54
MIDM_OPEN                   equ 55
MIDM_CLOSE                  equ 56
MIDM_PREPARE                equ 57
MIDM_UNPREPARE              equ 58
MIDM_ADDBUFFER              equ 59
MIDM_START                  equ 60
MIDM_STOP                   equ 61
MIDM_RESET                  equ 62

endif   ;ifndef MMNOMIDIDEV


ifndef  MMNOAUXDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                   Auxiliary audio device driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; maximum number of auxiliary device drivers loaded
MAXAUXDRIVERS               equ 10

AUXM_INIT                   equ DRVM_INIT
AUXM_USER                   equ DRVM_USER

; messages sent to auxMessage() entry-point function
AUXDM_GETNUMDEVS            equ 3
AUXDM_GETDEVCAPS            equ 4
AUXDM_GETVOLUME             equ 5
AUXDM_SETVOLUME             equ 6

endif   ;ifndef MMNOAUXDEV


ifndef  MMNOTIMERDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       Timer device driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TIMEREVENT struc
    te_wDelay         dw    ?     ; delay required
    te_wResolution    dw    ?     ; resolution required
    te_lpFunction     dd    ?     ; ptr to callback function
    te_dwUser         dd    ?     ; user DWORD
    te_wFlags         dw    ?     ; defines how to program event
TIMEREVENT ends

; messages sent to tddMessage() function
TDD_KILLTIMEREVENT          equ (DRV_RESERVED + 0)  ; indices into a table of
TDD_SETTIMEREVENT           equ (DRV_RESERVED + 4)  ; functions; thus offset by
TDD_GETSYSTEMTIME           equ (DRV_RESERVED + 8)  ; four each time...
TDD_GETDEVCAPS              equ (DRV_RESERVED + 12) ; room for future expansion
TDD_BEGINMINPERIOD          equ (DRV_RESERVED + 16) ; room for future expansion
TDD_ENDMINPERIOD            equ (DRV_RESERVED + 20) ; room for future expansion

endif   ;ifndef MMNOTIMERDEV


ifndef  MMNOJOYDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                      Joystick device driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; joystick calibration info structure
JOYCALIBRATE struc
    jcal_wXbase     dw    ?
    jcal_wXdelta    dw    ?
    jcal_wYbase     dw    ?
    jcal_wYdelta    dw    ?
    jcal_wZbase     dw    ?
    jcal_wZdelta    dw    ?
JOYCALIBRATE ends

; messages sent to joystick driver's DriverProc() function
JDD_GETNUMDEVS              equ (DRV_RESERVED + 0001h)
JDD_GETDEVCAPS              equ (DRV_RESERVED + 0002h)
JDD_GETPOS                  equ (DRV_RESERVED + 0101h)
JDD_SETCALIBRATION          equ (DRV_RESERVED + 0102h)

endif   ;ifndef MMNOJOYDEV


ifndef  MMNOMCIDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       MCI device driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; internal MCI messages
MCI_OPEN_DRIVER             equ 0801h
MCI_CLOSE_DRIVER            equ 0802h

MAKEMCIRESOURCE MACRO a,b
        mov     ax,a
        mov     dx,b
        ENDM

; string return values only used with MAKEMCIRESOURCE
MCI_FALSE                   equ (MCI_STRING_OFFSET + 19)
MCI_TRUE                    equ (MCI_STRING_OFFSET + 20)

; resource string return values
MCI_FORMAT_RETURN_BASE      equ MCI_FORMAT_MILLISECONDS_S
MCI_FORMAT_MILLISECONDS_S   equ (MCI_STRING_OFFSET + 21)
MCI_FORMAT_HMS_S            equ (MCI_STRING_OFFSET + 22)
MCI_FORMAT_MSF_S            equ (MCI_STRING_OFFSET + 23)
MCI_FORMAT_FRAMES_S         equ (MCI_STRING_OFFSET + 24)
MCI_FORMAT_SMPTE_24_S       equ (MCI_STRING_OFFSET + 25)
MCI_FORMAT_SMPTE_25_S       equ (MCI_STRING_OFFSET + 26)
MCI_FORMAT_SMPTE_30_S       equ (MCI_STRING_OFFSET + 27)
MCI_FORMAT_SMPTE_30DROP_S   equ (MCI_STRING_OFFSET + 28)
MCI_FORMAT_BYTES_S          equ (MCI_STRING_OFFSET + 29)
MCI_FORMAT_SAMPLES_S        equ (MCI_STRING_OFFSET + 30)
MCI_FORMAT_TMSF_S           equ (MCI_STRING_OFFSET + 31)

MCI_VD_FORMAT_TRACK_S       equ (MCI_VD_OFFSET + 5)

WAVE_FORMAT_PCM_S           equ (MCI_WAVE_OFFSET + 0)
WAVE_MAPPER_S               equ (MCI_WAVE_OFFSET + 1)

MCI_SEQ_MAPPER_S            equ (MCI_SEQ_OFFSET + 5)
MCI_SEQ_FILE_S              equ (MCI_SEQ_OFFSET + 6)
MCI_SEQ_MIDI_S              equ (MCI_SEQ_OFFSET + 7)
MCI_SEQ_SMPTE_S             equ (MCI_SEQ_OFFSET + 8)
MCI_SEQ_FORMAT_SONGPTR_S    equ (MCI_SEQ_OFFSET + 9)
MCI_SEQ_NONE_S              equ (MCI_SEQ_OFFSET + 10)
MIDIMAPPER_S                equ (MCI_SEQ_OFFSET + 11)

; parameters for internal version of MCI_OPEN message sent from
; mciOpenDevice() to the driver
MCI_OPEN_DRIVER_PARMS struc
    mciodrv_wDeviceID           dw    ? ; device ID
    mciodrv_lpstrParams         dw    ? ; parameter string for entry in SYSTEM.INI
    mciodrv_wCustomCommandTable dw    ? ; custom command table (0FFFFh if none)
                                        ; filled in by the driver
    mciodrv_wType               dw    ? ; driver type filled in by the driver
MCI_OPEN_DRIVER_PARMS ends

; maximum length of an MCI device type
MCI_MAX_DEVICE_TYPE_LENGTH  equ 80

; flags for mciSendCommandInternal() which direct mciSendString() how to
; interpret the return value
MCI_RESOURCE_RETURNED       equ 00010000h  ; resource ID
MCI_COLONIZED3_RETURN       equ 00020000h  ; colonized ID, 3 bytes data
MCI_COLONIZED4_RETURN       equ 00040000h  ; colonized ID, 4 bytes data
MCI_INTEGER_RETURNED        equ 00080000h  ; integer conversion needed
MCI_RESOURCE_DRIVER         equ 00100000h  ; driver owns returned resource

; invalid command table ID
MCI_NO_COMMAND_TABLE        equ 0FFFFh

; command table information type tags
MCI_COMMAND_HEAD            equ 0
MCI_STRING                  equ 1
MCI_INTEGER                 equ 2
MCI_END_COMMAND             equ 3
MCI_RETURN                  equ 4
MCI_FLAG                    equ 5
MCI_END_COMMAND_LIST        equ 6
MCI_RECT                    equ 7
MCI_CONSTANT                equ 8
MCI_END_CONSTANT            equ 9

endif   ;ifndef MMNOMCIDEV


ifndef  MMNOTASKDEV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                              Task support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; error return values
TASKERR_NOTASKSUPPORT       equ 1
TASKERR_OUTOFMEMORY         equ 2

endif   ;ifndef MMNOTASKDEV
