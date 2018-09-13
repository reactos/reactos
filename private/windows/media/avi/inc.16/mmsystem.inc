;==========================================================================;
;  mmsystem.h -- Include file for Multimedia API's
;  Version 4.00
;  Copyright (C) 1992-1994 Microsoft Corporation.  All Rights Reserved.
;--------------------------------------------------------------------------;
;  Define:         Prevent inclusion of:
;  --------------  --------------------------------------------------------
;  MMNODRV         Installable driver support
;  MMNOSOUND       Sound support
;  MMNOWAVE        Waveform support
;  MMNOMIDI        MIDI support
;  MMNOAUX         Auxiliary audio support
;  MMNOMIXER       Mixer support
;  MMNOTIMER       Timer support
;  MMNOJOY         Joystick support
;  MMNOMCI         MCI support
;  MMNOMMIO        Multimedia file I/O support
;  MMNOMMSYSTEM    General MMSYSTEM functions
;==========================================================================;
; general constants
MAXPNAMELEN	EQU	32     ; max product name length (including NULL)
MAXERRORLENGTH	EQU	256    ; max error text length (including NULL)

MMTIME	STRUC
mmt_wType	DW	?
mmt_TimeUnion	DD	?
MMTIME	ENDS

SMPTE	STRUC
smpte_hour	DB	?
smpte_min	DB	?
smpte_sec	DB	?
smpte_frame	DB	?
smpte_fps	DB	?
smpte_reserved	DB	?
SMPTE	ENDS
; types for wType field in MMTIME struct
TIME_MS	EQU	0001H  ; time in milliseconds
TIME_SAMPLES	EQU	0002H  ; number of wave samples
TIME_BYTES	EQU	0004H  ; current byte offset
TIME_SMPTE	EQU	0008H  ; SMPTE time
TIME_MIDI	EQU	0010H  ; MIDI time
TIME_TICKS	EQU	0020H  ; Ticks within MIDI stream
MM_JOY1MOVE	EQU	3A0H           ; joystick
MM_JOY2MOVE	EQU	3A1H
MM_JOY1ZMOVE	EQU	3A2H
MM_JOY2ZMOVE	EQU	3A3H
MM_JOY1BUTTONDOWN	EQU	3B5H
MM_JOY2BUTTONDOWN	EQU	3B6H
MM_JOY1BUTTONUP	EQU	3B7H
MM_JOY2BUTTONUP	EQU	3B8H
MM_MCINOTIFY	EQU	3B9H           ; MCI
MM_WOM_OPEN	EQU	3BBH           ; waveform output
MM_WOM_CLOSE	EQU	3BCH
MM_WOM_DONE	EQU	3BDH
MM_WIM_OPEN	EQU	3BEH           ; waveform input
MM_WIM_CLOSE	EQU	3BFH
MM_WIM_DATA	EQU	3C0H
MM_MIM_OPEN	EQU	3C1H           ; MIDI input
MM_MIM_CLOSE	EQU	3C2H
MM_MIM_DATA	EQU	3C3H
MM_MIM_LONGDATA	EQU	3C4H
MM_MIM_ERROR	EQU	3C5H
MM_MIM_LONGERROR	EQU	3C6H
MM_MOM_OPEN	EQU	3C7H           ; MIDI output
MM_MOM_CLOSE	EQU	3C8H
MM_MOM_DONE	EQU	3C9H
MMSYSERR_BASE	EQU	0
WAVERR_BASE	EQU	32
MIDIERR_BASE	EQU	64
TIMERR_BASE	EQU	96
JOYERR_BASE	EQU	160
MCIERR_BASE	EQU	256
MIXERR_BASE	EQU	1024
MCI_STRING_OFFSET	EQU	512
MCI_VD_OFFSET	EQU	1024
MCI_CD_OFFSET	EQU	1088
MCI_WAVE_OFFSET	EQU	1152
MCI_SEQ_OFFSET	EQU	1216
;***************************************************************************
;	General error return values
;*************************************************************************
; general error return values
MMSYSERR_NOERROR	EQU	0                    ; no error
MMSYSERR_ERROR	EQU	(MMSYSERR_BASE+1)  ; unspecified error
MMSYSERR_BADDEVICEID	EQU	(MMSYSERR_BASE+2)  ; device ID out of range
MMSYSERR_NOTENABLED	EQU	(MMSYSERR_BASE+3)  ; driver failed enable
MMSYSERR_ALLOCATED	EQU	(MMSYSERR_BASE+4)  ; device already allocated
MMSYSERR_INVALHANDLE	EQU	(MMSYSERR_BASE+5)  ; device handle is invalid
MMSYSERR_NODRIVER	EQU	(MMSYSERR_BASE+6)  ; no device driver present
MMSYSERR_NOMEM	EQU	(MMSYSERR_BASE+7)  ; memory allocation error
MMSYSERR_NOTSUPPORTED	EQU	(MMSYSERR_BASE+8)  ; function isn't supported
MMSYSERR_BADERRNUM	EQU	(MMSYSERR_BASE+9)  ; error value out of range
MMSYSERR_INVALFLAG	EQU	(MMSYSERR_BASE+10) ; invalid flag passed
MMSYSERR_INVALPARAM	EQU	(MMSYSERR_BASE+11) ; invalid parameter passed
MMSYSERR_HANDLEBUSY	EQU	(MMSYSERR_BASE+12) ; handle being used
						   ; simultaneously on another
						   ; thread (eg callback)
MMSYSERR_INVALIDALIAS	EQU	(MMSYSERR_BASE+13) ; specified alias not found
MMSYSERR_BADDB	EQU	(MMSYSERR_BASE+14) ; bad registry database
MMSYSERR_KEYNOTFOUND	EQU	(MMSYSERR_BASE+15) ; registry key not found
MMSYSERR_READERROR	EQU	(MMSYSERR_BASE+16) ; registry read error
MMSYSERR_WRITEERROR	EQU	(MMSYSERR_BASE+17) ; registry write error
MMSYSERR_DELETEERROR	EQU	(MMSYSERR_BASE+18) ; registry delete error
MMSYSERR_VALNOTFOUND	EQU	(MMSYSERR_BASE+19) ; registry value not found
MMSYSERR_LASTERROR	EQU	(MMSYSERR_BASE+19) ; last error in range
ifndef MMNODRV
ifndef DRV_RESERVED
DRV_RESERVED	EQU	0800H
DRV_USER	EQU	4000H
endif
DRV_MCI_FIRST	EQU	DRV_RESERVED
DRV_MCI_LAST	EQU	(DRV_RESERVED+0FFFH)
endif  ; ifndef MMNODRV
;***************************************************************************
;	  Driver callback support
;*************************************************************************
; flags used with waveOutOpen(), waveInOpen(), midiInOpen(), and
; midiOutOpen() to specify the type of the dwCallback parameter.
CALLBACK_TYPEMASK	EQU	00070000H    ; callback type mask
CALLBACK_NULL	EQU	00000000H    ; no callback
CALLBACK_WINDOW	EQU	00010000H    ; dwCallback is a HWND
CALLBACK_TASK	EQU	00020000H    ; dwCallback is a HTASK
CALLBACK_FUNCTION	EQU	00030000H    ; dwCallback is a FARPROC
ifdef WIN32
CALLBACK_THREAD	EQU	(CALLBACK_TASK) ; thread ID replaces 16 bit task
CALLBACK_EVENT	EQU	00050000H    ; dwCallback is an EVENT
endif
SND_SYNC	EQU	0000H  ; play synchronously (default)
SND_ASYNC	EQU	0001H  ; play asynchronously
SND_NODEFAULT	EQU	0002H  ; silence (!default) if sound not found
SND_MEMORY	EQU	0004H  ; pszSound points to a memory file
SND_LOOP	EQU	0008H  ; loop the sound until next sndPlaySound
SND_NOSTOP	EQU	0010H  ; don't stop any currently playing sound
ifndef MMNOWAVE
;***************************************************************************
;	Waveform audio support
;*************************************************************************
; waveform audio error return values
WAVERR_BADFORMAT	EQU	(WAVERR_BASE+0)    ; unsupported wave format
WAVERR_STILLPLAYING	EQU	(WAVERR_BASE+1)    ; still something playing
WAVERR_UNPREPARED	EQU	(WAVERR_BASE+2)    ; header not prepared
WAVERR_SYNC	EQU	(WAVERR_BASE+3)    ; device is synchronous
WAVERR_LASTERROR	EQU	(WAVERR_BASE+3)    ; last error in range
; wave callback messages
WOM_OPEN	EQU	MM_WOM_OPEN
WOM_CLOSE	EQU	MM_WOM_CLOSE
WOM_DONE	EQU	MM_WOM_DONE
WIM_OPEN	EQU	MM_WIM_OPEN
WIM_CLOSE	EQU	MM_WIM_CLOSE
WIM_DATA	EQU	MM_WIM_DATA
; device ID for wave device mapper
WAVE_MAPPER	EQU	(-1)
; flags for dwFlags parameter in waveOutOpen() and waveInOpen()
WAVE_FORMAT_QUERY	EQU	0001H
WAVE_ALLOWSYNC	EQU	0002H

WAVEHDR	STRUC
lpWaveData	DD	?
dwWaveBufferLength	DD	?
dwWaveBytesRecorded	DD	?
dwWaveUser	DD	?
dwWaveFlags	DD	?
dwWaveLoops	DD	?
lpWaveNext	DD	?
Wavereserved	DD	?
WAVEHDR	ENDS
; flags for dwFlags field of WAVEHDR
WHDR_DONE	EQU	00000001H  ; done bit
WHDR_PREPARED	EQU	00000002H  ; set if this header has been prepared
WHDR_BEGINLOOP	EQU	00000004H  ; loop start block
WHDR_ENDLOOP	EQU	00000008H  ; loop end block
WHDR_INQUEUE	EQU	00000010H  ; reserved for driver

WAVEOUTCAPS	STRUC
woc_wMid	DW	?
woc_wPid	DW	?
woc_vDriverVersion	DW	?
woc_szPname	DB	MAXPNAMELEN DUP (?)
woc_dwFormats	DD	?
woc_wChannels	DW	?
woc_dwSupport	DD	?
WAVEOUTCAPS	ENDS
; flags for dwSupport field of WAVEOUTCAPS
WAVECAPS_PITCH	EQU	0001H   ; supports pitch control
WAVECAPS_PLAYBACKRATE	EQU	0002H   ; supports playback rate control
WAVECAPS_VOLUME	EQU	0004H   ; supports volume control
WAVECAPS_LRVOLUME	EQU	0008H   ; separate left-right volume control
WAVECAPS_SYNC	EQU	0010H

WAVEINCAPS	STRUC
wic_wMid	DW	?
wic_wPid	DW	?
wic_vDriverVersion	DW	?
wic_szPname	DB	MAXPNAMELEN DUP (?)
wic_dwFormats	DD	?
wic_wChannels	DW	?
WAVEINCAPS	ENDS
; defines for dwFormat field of WAVEINCAPS and WAVEOUTCAPS
WAVE_INVALIDFORMAT	EQU	00000000H       ; invalid format
WAVE_FORMAT_1M08	EQU	00000001H       ; 11.025 kHz, Mono,   8-bit
WAVE_FORMAT_1S08	EQU	00000002H       ; 11.025 kHz, Stereo, 8-bit
WAVE_FORMAT_1M16	EQU	00000004H       ; 11.025 kHz, Mono,   16-bit
WAVE_FORMAT_1S16	EQU	00000008H       ; 11.025 kHz, Stereo, 16-bit
WAVE_FORMAT_2M08	EQU	00000010H       ; 22.05  kHz, Mono,   8-bit
WAVE_FORMAT_2S08	EQU	00000020H       ; 22.05  kHz, Stereo, 8-bit
WAVE_FORMAT_2M16	EQU	00000040H       ; 22.05  kHz, Mono,   16-bit
WAVE_FORMAT_2S16	EQU	00000080H       ; 22.05  kHz, Stereo, 16-bit
WAVE_FORMAT_4M08	EQU	00000100H       ; 44.1   kHz, Mono,   8-bit
WAVE_FORMAT_4S08	EQU	00000200H       ; 44.1   kHz, Stereo, 8-bit
WAVE_FORMAT_4M16	EQU	00000400H       ; 44.1   kHz, Mono,   16-bit
WAVE_FORMAT_4S16	EQU	00000800H       ; 44.1   kHz, Stereo, 16-bit

WAVEFORMAT	STRUC
wfmt_wFormatTag	DW	?
wfmt_nChannels	DW	?
wfmt_nSamplesPerSec	DD	?
wfmt_nAvgBytesPerSec	DD	?
wfmt_nBlockAlign	DW	?
WAVEFORMAT	ENDS
; flags for wFormatTag field of WAVEFORMAT
WAVE_FORMAT_PCM	EQU	1

PCMWAVEFORMAT	STRUC
pcm_wf	DB	SIZE WAVEFORMAT DUP (?)
pcm_wBitsPerSample	DW	?
PCMWAVEFORMAT	ENDS
endif  ; ifndef MMNOWAVE
ifndef MMNOMIDI
;***************************************************************************
;	    MIDI audio support
;*************************************************************************
; MIDI error return values
MIDIERR_UNPREPARED	EQU	(MIDIERR_BASE+0)   ; header not prepared
MIDIERR_STILLPLAYING	EQU	(MIDIERR_BASE+1)   ; still something playing
MIDIERR_NOMAP	EQU	(MIDIERR_BASE+2)   ; no configured instruments
MIDIERR_NOTREADY	EQU	(MIDIERR_BASE+3)   ; hardware is still busy
MIDIERR_NODEVICE	EQU	(MIDIERR_BASE+4)   ; port no longer connected
MIDIERR_INVALIDSETUP	EQU	(MIDIERR_BASE+5)   ; invalid MIF
MIDIERR_BADOPENMODE	EQU	(MIDIERR_BASE+6)   ; operation unsupported w/ open mode
MIDIERR_LASTERROR	EQU	(MIDIERR_BASE+6)   ; last error in range
MIDIPATCHSIZE	EQU	128
; MIDI callback messages
MIM_OPEN	EQU	MM_MIM_OPEN
MIM_CLOSE	EQU	MM_MIM_CLOSE
MIM_DATA	EQU	MM_MIM_DATA
MIM_LONGDATA	EQU	MM_MIM_LONGDATA
MIM_ERROR	EQU	MM_MIM_ERROR
MIM_LONGERROR	EQU	MM_MIM_LONGERROR
MOM_OPEN	EQU	MM_MOM_OPEN
MOM_CLOSE	EQU	MM_MOM_CLOSE
MOM_DONE	EQU	MM_MOM_DONE
; device ID for MIDI mapper
MIDIMAPPER	EQU	(-1)
MIDI_MAPPER	EQU	(-1)
MIDI_CACHE_ALL	EQU	1
MIDI_CACHE_BESTFIT	EQU	2
MIDI_CACHE_QUERY	EQU	3
MIDI_UNCACHE	EQU	4

MIDIOUTCAPS	STRUC
moc_wMid	DW	?
moc_wPid	DW	?
moc_vDriverVersion	DW	?
moc_szPname	DB	MAXPNAMELEN DUP (?)
moc_wTechnology	DW	?
moc_wVoices	DW	?
moc_wNotes	DW	?
moc_wChannelMask	DW	?
moc_dwSupport	DD	?
MIDIOUTCAPS	ENDS
; flags for wTechnology field of MIDIOUTCAPS structure
MOD_MIDIPORT	EQU	1  ; output port
MOD_SYNTH	EQU	2  ; generic internal synth
MOD_SQSYNTH	EQU	3  ; square wave internal synth
MOD_FMSYNTH	EQU	4  ; FM internal synth
MOD_MAPPER	EQU	5  ; MIDI mapper
; flags for dwSupport field of MIDIOUTCAPS structure
MIDICAPS_VOLUME	EQU	0001H  ; supports volume control
MIDICAPS_LRVOLUME	EQU	0002H  ; separate left-right volume control
MIDICAPS_CACHE	EQU	0004H

MIDIINCAPS	STRUC
mic_wMid	DW	?
mic_wPid	DW	?
mic_vDriverVersion	DW	?
mic_szPname	DB	MAXPNAMELEN DUP (?)
MIDIINCAPS	ENDS

MIDIHDR	STRUC
lpMidiData	DD	?
dwMidiBufferLength	DD	?
dwMidiBytesRecorded	DD	?
dwMidiUser	DD	?
dwMidiFlags	DD	?
lpMidiNext	DD	?
Midireserved	DD	?
MIDIHDR	ENDS
; flags for dwFlags field of MIDIHDR structure
MHDR_DONE	EQU	00000001H       ; done bit
MHDR_PREPARED	EQU	00000002H       ; set if header prepared
MHDR_INQUEUE	EQU	00000004H       ; reserved for driver
MHDR_ISPOLY	EQU	00000008H       ; Buffer is polymsg buffer
MHDR_CHANIGNORE	EQU	0FFFF0000H       ; Ignore channels 1-16
endif  ; ifndef MMNOMIDI
ifndef MMNOAUX
;***************************************************************************
;	Auxiliary audio support
;*************************************************************************
; device ID for aux device mapper
AUX_MAPPER	EQU	(-1)

AUXCAPS	STRUC
acaps_wMid	DW	?
acaps_wPid	DW	?
acaps_vDriverVersion	DW	?
acaps_szPname	DB	MAXPNAMELEN DUP (?)
acaps_wTechnology	DW	?
acaps_dwSupport	DD	?
AUXCAPS	ENDS
; flags for wTechnology field in AUXCAPS structure
AUXCAPS_CDAUDIO	EQU	1       ; audio from internal CD-ROM drive
AUXCAPS_AUXIN	EQU	2       ; audio from auxiliary input jacks
; flags for dwSupport field in AUXCAPS structure
AUXCAPS_VOLUME	EQU	0001H  ; supports volume control
AUXCAPS_LRVOLUME	EQU	0002H  ; separate left-right volume control
endif  ; ifndef MMNOAUX
ifndef MMNOTIMER
;***************************************************************************
;	    Timer support
;*************************************************************************
; timer error return values
TIMERR_NOERROR	EQU	(0)                  ; no error
TIMERR_NOCANDO	EQU	(TIMERR_BASE+1)      ; request not completed
TIMERR_STRUCT	EQU	(TIMERR_BASE+33)     ; time struct size
; flags for wFlags parameter of timeSetEvent() function
TIME_ONESHOT	EQU	00H   ; program timer for single event
TIME_PERIODIC	EQU	01H   ; program for continuous periodic event

TIMECAPS	STRUC
tc_wPeriodMin	DW	?
tc_wPeriodMax	DW	?
TIMECAPS	ENDS
endif  ; ifndef MMNOTIMER
ifndef MMNOJOY
;***************************************************************************
;	    Joystick support
;*************************************************************************
; joystick error return values
JOYERR_NOERROR	EQU	(0)                  ; no error
JOYERR_PARMS	EQU	(JOYERR_BASE+5)      ; bad parameters
JOYERR_NOCANDO	EQU	(JOYERR_BASE+6)      ; request not completed
JOYERR_UNPLUGGED	EQU	(JOYERR_BASE+7)      ; joystick is unplugged
; constants used with JOYINFO structure and MM_JOY* messages
JOY_BUTTON1	EQU	0001H
JOY_BUTTON2	EQU	0002H
JOY_BUTTON3	EQU	0004H
JOY_BUTTON4	EQU	0008H
JOY_BUTTON1CHG	EQU	0100H
JOY_BUTTON2CHG	EQU	0200H
JOY_BUTTON3CHG	EQU	0400H
JOY_BUTTON4CHG	EQU	0800H
; joystick ID constants
JOYSTICKID1	EQU	0
JOYSTICKID2	EQU	1

JOYCAPS	STRUC
jcaps_wMid	DW	?
jcaps_wPid	DW	?
jcaps_szPname	DB	MAXPNAMELEN DUP (?)
jcaps_wXmin	DW	?
jcaps_wXmax	DW	?
jcaps_wYmin	DW	?
jcaps_wYmax	DW	?
jcaps_wZmin	DW	?
jcaps_wZmax	DW	?
jcaps_wNumButtons	DW	?
jcaps_wPeriodMin	DW	?
jcaps_wPeriodMax	DW	?
JOYCAPS	ENDS

JOYINFO	STRUC
jinfo_wXpos	DW	?
jinfo_wYpos	DW	?
jinfo_wZpos	DW	?
jinfo_wButtons	DW	?
JOYINFO	ENDS
endif  ; ifndef MMNOJOY
ifndef MMNOMMIO
;***************************************************************************
;	Multimedia File I/O support
;*************************************************************************
; MMIO error return values
MMIOERR_BASE	EQU	256
MMIOERR_FILENOTFOUND	EQU	(MMIOERR_BASE+1)  ; file not found
MMIOERR_OUTOFMEMORY	EQU	(MMIOERR_BASE+2)  ; out of memory
MMIOERR_CANNOTOPEN	EQU	(MMIOERR_BASE+3)  ; cannot open
MMIOERR_CANNOTCLOSE	EQU	(MMIOERR_BASE+4)  ; cannot close
MMIOERR_CANNOTREAD	EQU	(MMIOERR_BASE+5)  ; cannot read
MMIOERR_CANNOTWRITE	EQU	(MMIOERR_BASE+6)  ; cannot write
MMIOERR_CANNOTSEEK	EQU	(MMIOERR_BASE+7)  ; cannot seek
MMIOERR_CANNOTEXPAND	EQU	(MMIOERR_BASE+8)  ; cannot expand file
MMIOERR_CHUNKNOTFOUND	EQU	(MMIOERR_BASE+9)  ; chunk not found
MMIOERR_UNBUFFERED	EQU	(MMIOERR_BASE+10) ;
MMIOERR_PATHNOTFOUND	EQU	(MMIOERR_BASE+11) ; path incorrect
MMIOERR_ACCESSDENIED	EQU	(MMIOERR_BASE+12) ; file was protected
MMIOERR_SHARINGVIOLATION	EQU	(MMIOERR_BASE+13) ; file in use
MMIOERR_NETWORKERROR	EQU	(MMIOERR_BASE+14) ; network not responding
MMIOERR_TOOMANYOPENFILES	EQU	(MMIOERR_BASE+15) ; no more file handles
MMIOERR_INVALIDFILE	EQU	(MMIOERR_BASE+16) ; default error file error
; MMIO constants
CFSEPCHAR	EQU	<'+'>             ; compound file name separator char.

MMIOINFO	STRUC
mmio_dwFlags	DD	?
mmio_fccIOProc	DD	?
mmio_pIOProc	DD	?
mmio_wErrorRet	DW	?
mmio_htask	DW	?
mmio_cchBuffer	DD	?
mmio_pchBuffer	DD	?
mmio_pchNext	DD	?
mmio_pchEndRead	DD	?
mmio_pchEndWrite	DD	?
mmio_lBufOffset	DD	?
mmio_lDiskOffset	DD	?
mmio_adwInfo	DD	3 DUP (?)
mmio_dwReserved1	DD	?
mmio_dwReserved2	DD	?
mmio_hmmio	DW	?
MMIOINFO	ENDS
; bit field masks
MMIO_RWMODE	EQU	00000003H      ; open file for reading/writing/both
MMIO_SHAREMODE	EQU	00000070H      ; file sharing mode number
; constants for dwFlags field of MMIOINFO
MMIO_CREATE	EQU	00001000H      ; create new file (or truncate file)
MMIO_PARSE	EQU	00000100H      ; parse new file returning path
MMIO_DELETE	EQU	00000200H      ; create new file (or truncate file)
MMIO_EXIST	EQU	00004000H      ; checks for existence of file
MMIO_ALLOCBUF	EQU	00010000H      ; mmioOpen() should allocate a buffer
MMIO_GETTEMP	EQU	00020000H      ; mmioOpen() should retrieve temp name
MMIO_DIRTY	EQU	10000000H      ; I/O buffer is dirty
; read/write mode numbers (bit field MMIO_RWMODE)
MMIO_READ	EQU	00000000H      ; open file for reading only
MMIO_WRITE	EQU	00000001H      ; open file for writing only
MMIO_READWRITE	EQU	00000002H      ; open file for reading and writing
; share mode numbers (bit field MMIO_SHAREMODE)
MMIO_COMPAT	EQU	00000000H      ; compatibility mode
MMIO_EXCLUSIVE	EQU	00000010H      ; exclusive-access mode
MMIO_DENYWRITE	EQU	00000020H      ; deny writing to other processes
MMIO_DENYREAD	EQU	00000030H      ; deny reading to other processes
MMIO_DENYNONE	EQU	00000040H      ; deny nothing to other processes
; various MMIO flags
MMIO_FHOPEN	EQU	0010H  ; mmioClose: keep file handle open
MMIO_EMPTYBUF	EQU	0010H  ; mmioFlush: empty the I/O buffer
MMIO_TOUPPER	EQU	0010H  ; mmioStringToFOURCC: to u-case
MMIO_INSTALLPROC	EQU	00010000H  ; mmioInstallIOProc: install MMIOProc
MMIO_GLOBALPROC	EQU	10000000H  ; mmioInstallIOProc: install globally
MMIO_REMOVEPROC	EQU	00020000H  ; mmioInstallIOProc: remove MMIOProc
MMIO_FINDPROC	EQU	00040000H  ; mmioInstallIOProc: find an MMIOProc
MMIO_FINDCHUNK	EQU	0010H  ; mmioDescend: find a chunk by ID
MMIO_FINDRIFF	EQU	0020H  ; mmioDescend: find a LIST chunk
MMIO_FINDLIST	EQU	0040H  ; mmioDescend: find a RIFF chunk
MMIO_CREATERIFF	EQU	0020H  ; mmioCreateChunk: make a LIST chunk
MMIO_CREATELIST	EQU	0040H  ; mmioCreateChunk: make a RIFF chunk
; message numbers for MMIOPROC I/O procedure functions
MMIOM_READ	EQU	MMIO_READ       ; read
MMIOM_WRITE	EQU	MMIO_WRITE       ; write
MMIOM_SEEK	EQU	2       ; seek to a new position in file
MMIOM_OPEN	EQU	3       ; open file
MMIOM_CLOSE	EQU	4       ; close file
MMIOM_WRITEFLUSH	EQU	5       ; write and flush
MMIOM_RENAME	EQU	6       ; rename specified file
MMIOM_USER	EQU	8000H       ; beginning of user-defined messages
; flags for mmioSeek()
ifndef SEEK_SET
SEEK_SET	EQU	0               ; seek to an absolute position
SEEK_CUR	EQU	1               ; seek relative to current position
SEEK_END	EQU	2               ; seek relative to end of file
endif  ; ifndef SEEK_SET
; other constants
MMIO_DEFAULTBUFFER	EQU	8192    ; default buffer size
endif  ; ifndef MMNOMMIO
ifndef MMNOMCI
; MCI error return values
MCIERR_INVALID_DEVICE_ID	EQU	(MCIERR_BASE+1)
MCIERR_UNRECOGNIZED_KEYWORD	EQU	(MCIERR_BASE+3)
MCIERR_UNRECOGNIZED_COMMAND	EQU	(MCIERR_BASE+5)
MCIERR_HARDWARE	EQU	(MCIERR_BASE+6)
MCIERR_INVALID_DEVICE_NAME	EQU	(MCIERR_BASE+7)
MCIERR_OUT_OF_MEMORY	EQU	(MCIERR_BASE+8)
MCIERR_DEVICE_OPEN	EQU	(MCIERR_BASE+9)
MCIERR_CANNOT_LOAD_DRIVER	EQU	(MCIERR_BASE+10)
MCIERR_MISSING_COMMAND_STRING	EQU	(MCIERR_BASE+11)
MCIERR_PARAM_OVERFLOW	EQU	(MCIERR_BASE+12)
MCIERR_MISSING_STRING_ARGUMENT	EQU	(MCIERR_BASE+13)
MCIERR_BAD_INTEGER	EQU	(MCIERR_BASE+14)
MCIERR_PARSER_INTERNAL	EQU	(MCIERR_BASE+15)
MCIERR_DRIVER_INTERNAL	EQU	(MCIERR_BASE+16)
MCIERR_MISSING_PARAMETER	EQU	(MCIERR_BASE+17)
MCIERR_UNSUPPORTED_FUNCTION	EQU	(MCIERR_BASE+18)
MCIERR_FILE_NOT_FOUND	EQU	(MCIERR_BASE+19)
MCIERR_DEVICE_NOT_READY	EQU	(MCIERR_BASE+20)
MCIERR_INTERNAL	EQU	(MCIERR_BASE+21)
MCIERR_DRIVER	EQU	(MCIERR_BASE+22)
MCIERR_CANNOT_USE_ALL	EQU	(MCIERR_BASE+23)
MCIERR_MULTIPLE	EQU	(MCIERR_BASE+24)
MCIERR_EXTENSION_NOT_FOUND	EQU	(MCIERR_BASE+25)
MCIERR_OUTOFRANGE	EQU	(MCIERR_BASE+26)
MCIERR_FLAGS_NOT_COMPATIBLE	EQU	(MCIERR_BASE+28)
MCIERR_FILE_NOT_SAVED	EQU	(MCIERR_BASE+30)
MCIERR_DEVICE_TYPE_REQUIRED	EQU	(MCIERR_BASE+31)
MCIERR_DEVICE_LOCKED	EQU	(MCIERR_BASE+32)
MCIERR_DUPLICATE_ALIAS	EQU	(MCIERR_BASE+33)
MCIERR_BAD_CONSTANT	EQU	(MCIERR_BASE+34)
MCIERR_MUST_USE_SHAREABLE	EQU	(MCIERR_BASE+35)
MCIERR_MISSING_DEVICE_NAME	EQU	(MCIERR_BASE+36)
MCIERR_BAD_TIME_FORMAT	EQU	(MCIERR_BASE+37)
MCIERR_NO_CLOSING_QUOTE	EQU	(MCIERR_BASE+38)
MCIERR_DUPLICATE_FLAGS	EQU	(MCIERR_BASE+39)
MCIERR_INVALID_FILE	EQU	(MCIERR_BASE+40)
MCIERR_NULL_PARAMETER_BLOCK	EQU	(MCIERR_BASE+41)
MCIERR_UNNAMED_RESOURCE	EQU	(MCIERR_BASE+42)
MCIERR_NEW_REQUIRES_ALIAS	EQU	(MCIERR_BASE+43)
MCIERR_NOTIFY_ON_AUTO_OPEN	EQU	(MCIERR_BASE+44)
MCIERR_NO_ELEMENT_ALLOWED	EQU	(MCIERR_BASE+45)
MCIERR_NONAPPLICABLE_FUNCTION	EQU	(MCIERR_BASE+46)
MCIERR_ILLEGAL_FOR_AUTO_OPEN	EQU	(MCIERR_BASE+47)
MCIERR_FILENAME_REQUIRED	EQU	(MCIERR_BASE+48)
MCIERR_EXTRA_CHARACTERS	EQU	(MCIERR_BASE+49)
MCIERR_DEVICE_NOT_INSTALLED	EQU	(MCIERR_BASE+50)
MCIERR_GET_CD	EQU	(MCIERR_BASE+51)
MCIERR_SET_CD	EQU	(MCIERR_BASE+52)
MCIERR_SET_DRIVE	EQU	(MCIERR_BASE+53)
MCIERR_DEVICE_LENGTH	EQU	(MCIERR_BASE+54)
MCIERR_DEVICE_ORD_LENGTH	EQU	(MCIERR_BASE+55)
MCIERR_NO_INTEGER	EQU	(MCIERR_BASE+56)
MCIERR_WAVE_OUTPUTSINUSE	EQU	(MCIERR_BASE+64)
MCIERR_WAVE_SETOUTPUTINUSE	EQU	(MCIERR_BASE+65)
MCIERR_WAVE_INPUTSINUSE	EQU	(MCIERR_BASE+66)
MCIERR_WAVE_SETINPUTINUSE	EQU	(MCIERR_BASE+67)
MCIERR_WAVE_OUTPUTUNSPECIFIED	EQU	(MCIERR_BASE+68)
MCIERR_WAVE_INPUTUNSPECIFIED	EQU	(MCIERR_BASE+69)
MCIERR_WAVE_OUTPUTSUNSUITABLE	EQU	(MCIERR_BASE+70)
MCIERR_WAVE_SETOUTPUTUNSUITABLE	EQU	(MCIERR_BASE+71)
MCIERR_WAVE_INPUTSUNSUITABLE	EQU	(MCIERR_BASE+72)
MCIERR_WAVE_SETINPUTUNSUITABLE	EQU	(MCIERR_BASE+73)
MCIERR_SEQ_DIV_INCOMPATIBLE	EQU	(MCIERR_BASE+80)
MCIERR_SEQ_PORT_INUSE	EQU	(MCIERR_BASE+81)
MCIERR_SEQ_PORT_NONEXISTENT	EQU	(MCIERR_BASE+82)
MCIERR_SEQ_PORT_MAPNODEVICE	EQU	(MCIERR_BASE+83)
MCIERR_SEQ_PORT_MISCERROR	EQU	(MCIERR_BASE+84)
MCIERR_SEQ_TIMER	EQU	(MCIERR_BASE+85)
MCIERR_SEQ_PORTUNSPECIFIED	EQU	(MCIERR_BASE+86)
MCIERR_SEQ_NOMIDIPRESENT	EQU	(MCIERR_BASE+87)
MCIERR_NO_WINDOW	EQU	(MCIERR_BASE+90)
MCIERR_CREATEWINDOW	EQU	(MCIERR_BASE+91)
MCIERR_FILE_READ	EQU	(MCIERR_BASE+92)
MCIERR_FILE_WRITE	EQU	(MCIERR_BASE+93)
MCIERR_NO_IDENTITY	EQU	(MCIERR_BASE+94)
; all custom device driver errors must be >= than this value
MCIERR_CUSTOM_DRIVER_BASE	EQU	(MCIERR_BASE+256)
; MCI command message identifiers
MCI_OPEN	EQU	0803H
MCI_CLOSE	EQU	0804H
MCI_ESCAPE	EQU	0805H
MCI_PLAY	EQU	0806H
MCI_SEEK	EQU	0807H
MCI_STOP	EQU	0808H
MCI_PAUSE	EQU	0809H
MCI_INFO	EQU	080AH
MCI_GETDEVCAPS	EQU	080BH
MCI_SPIN	EQU	080CH
MCI_SET	EQU	080DH
MCI_STEP	EQU	080EH
MCI_RECORD	EQU	080FH
MCI_SYSINFO	EQU	0810H
MCI_BREAK	EQU	0811H
;reserved                              0x0812
MCI_SAVE	EQU	0813H
MCI_STATUS	EQU	0814H
MCI_CUE	EQU	0830H
MCI_REALIZE	EQU	0840H
MCI_WINDOW	EQU	0841H
MCI_PUT	EQU	0842H
MCI_WHERE	EQU	0843H
MCI_FREEZE	EQU	0844H
MCI_UNFREEZE	EQU	0845H
MCI_LOAD	EQU	0850H
MCI_CUT	EQU	0851H
MCI_COPY	EQU	0852H
MCI_PASTE	EQU	0853H
MCI_UPDATE	EQU	0854H
MCI_RESUME	EQU	0855H
MCI_DELETE	EQU	0856H
; all custom MCI command messages must be >= than this value
MCI_USER_MESSAGES	EQU	(DRV_MCI_FIRST+400H)
MCI_ALL_DEVICE_ID	EQU	-1
; constants for predefined MCI device types
MCI_DEVTYPE_VCR	EQU	(MCI_STRING_OFFSET+1)
MCI_DEVTYPE_VIDEODISC	EQU	(MCI_STRING_OFFSET+2)
MCI_DEVTYPE_OVERLAY	EQU	(MCI_STRING_OFFSET+3)
MCI_DEVTYPE_CD_AUDIO	EQU	(MCI_STRING_OFFSET+4)
MCI_DEVTYPE_DAT	EQU	(MCI_STRING_OFFSET+5)
MCI_DEVTYPE_SCANNER	EQU	(MCI_STRING_OFFSET+6)
MCI_DEVTYPE_ANIMATION	EQU	(MCI_STRING_OFFSET+7)
MCI_DEVTYPE_DIGITAL_VIDEO	EQU	(MCI_STRING_OFFSET+8)
MCI_DEVTYPE_OTHER	EQU	(MCI_STRING_OFFSET+9)
MCI_DEVTYPE_WAVEFORM_AUDIO	EQU	(MCI_STRING_OFFSET+10)
MCI_DEVTYPE_SEQUENCER	EQU	(MCI_STRING_OFFSET+11)
MCI_DEVTYPE_FIRST	EQU	MCI_DEVTYPE_VCR
MCI_DEVTYPE_LAST	EQU	MCI_DEVTYPE_SEQUENCER
; return values for 'status mode' command
MCI_MODE_NOT_READY	EQU	(MCI_STRING_OFFSET+12)
MCI_MODE_STOP	EQU	(MCI_STRING_OFFSET+13)
MCI_MODE_PLAY	EQU	(MCI_STRING_OFFSET+14)
MCI_MODE_RECORD	EQU	(MCI_STRING_OFFSET+15)
MCI_MODE_SEEK	EQU	(MCI_STRING_OFFSET+16)
MCI_MODE_PAUSE	EQU	(MCI_STRING_OFFSET+17)
MCI_MODE_OPEN	EQU	(MCI_STRING_OFFSET+18)
; constants used in 'set time format' and 'status time format' commands
MCI_FORMAT_MILLISECONDS	EQU	0
MCI_FORMAT_HMS	EQU	1
MCI_FORMAT_MSF	EQU	2
MCI_FORMAT_FRAMES	EQU	3
MCI_FORMAT_SMPTE_24	EQU	4
MCI_FORMAT_SMPTE_25	EQU	5
MCI_FORMAT_SMPTE_30	EQU	6
MCI_FORMAT_SMPTE_30DROP	EQU	7
MCI_FORMAT_BYTES	EQU	8
MCI_FORMAT_SAMPLES	EQU	9
MCI_FORMAT_TMSF	EQU	10
; flags for wParam of MM_MCINOTIFY message
MCI_NOTIFY_SUCCESSFUL	EQU	0001H
MCI_NOTIFY_SUPERSEDED	EQU	0002H
MCI_NOTIFY_ABORTED	EQU	0004H
MCI_NOTIFY_FAILURE	EQU	0008H
; common flags for dwFlags parameter of MCI command messages
MCI_NOTIFY	EQU	00000001H
MCI_WAIT	EQU	00000002H
MCI_FROM	EQU	00000004H
MCI_TO	EQU	00000008H
MCI_TRACK	EQU	00000010H
; flags for dwFlags parameter of MCI_OPEN command message
MCI_OPEN_SHAREABLE	EQU	00000100H
MCI_OPEN_ELEMENT	EQU	00000200H
MCI_OPEN_ALIAS	EQU	00000400H
MCI_OPEN_ELEMENT_ID	EQU	00000800H
MCI_OPEN_TYPE_ID	EQU	00001000H
MCI_OPEN_TYPE	EQU	00002000H
; flags for dwFlags parameter of MCI_SEEK command message
MCI_SEEK_TO_START	EQU	00000100H
MCI_SEEK_TO_END	EQU	00000200H
; flags for dwFlags parameter of MCI_STATUS command message
MCI_STATUS_ITEM	EQU	00000100H
MCI_STATUS_START	EQU	00000200H
; flags for dwItem field of the MCI_STATUS_PARMS parameter block
MCI_STATUS_LENGTH	EQU	00000001H
MCI_STATUS_POSITION	EQU	00000002H
MCI_STATUS_NUMBER_OF_TRACKS	EQU	00000003H
MCI_STATUS_MODE	EQU	00000004H
MCI_STATUS_MEDIA_PRESENT	EQU	00000005H
MCI_STATUS_TIME_FORMAT	EQU	00000006H
MCI_STATUS_READY	EQU	00000007H
MCI_STATUS_CURRENT_TRACK	EQU	00000008H
; flags for dwFlags parameter of MCI_INFO command message
MCI_INFO_PRODUCT	EQU	00000100H
MCI_INFO_FILE	EQU	00000200H
MCI_INFO_MEDIA_UPC	EQU	00000400H
MCI_INFO_MEDIA_IDENTITY	EQU	00000800H
; flags for dwFlags parameter of MCI_GETDEVCAPS command message
MCI_GETDEVCAPS_ITEM	EQU	00000100H
; flags for dwItem field of the MCI_GETDEVCAPS_PARMS parameter block
MCI_GETDEVCAPS_CAN_RECORD	EQU	00000001H
MCI_GETDEVCAPS_HAS_AUDIO	EQU	00000002H
MCI_GETDEVCAPS_HAS_VIDEO	EQU	00000003H
MCI_GETDEVCAPS_DEVICE_TYPE	EQU	00000004H
MCI_GETDEVCAPS_USES_FILES	EQU	00000005H
MCI_GETDEVCAPS_COMPOUND_DEVICE	EQU	00000006H
MCI_GETDEVCAPS_CAN_EJECT	EQU	00000007H
MCI_GETDEVCAPS_CAN_PLAY	EQU	00000008H
MCI_GETDEVCAPS_CAN_SAVE	EQU	00000009H
; flags for dwFlags parameter of MCI_SYSINFO command message
MCI_SYSINFO_QUANTITY	EQU	00000100H
MCI_SYSINFO_OPEN	EQU	00000200H
MCI_SYSINFO_NAME	EQU	00000400H
MCI_SYSINFO_INSTALLNAME	EQU	00000800H
; flags for dwFlags parameter of MCI_SET command message
MCI_SET_DOOR_OPEN	EQU	00000100H
MCI_SET_DOOR_CLOSED	EQU	00000200H
MCI_SET_TIME_FORMAT	EQU	00000400H
MCI_SET_AUDIO	EQU	00000800H
MCI_SET_VIDEO	EQU	00001000H
MCI_SET_ON	EQU	00002000H
MCI_SET_OFF	EQU	00004000H
; flags for dwAudio field of MCI_SET_PARMS or MCI_SEQ_SET_PARMS
ifdef WIN32
MCI_SET_AUDIO_ALL	EQU	00004001H
MCI_SET_AUDIO_LEFT	EQU	00004002H
MCI_SET_AUDIO_RIGHT	EQU	00004003H
else
MCI_SET_AUDIO_ALL	EQU	00000000H
MCI_SET_AUDIO_LEFT	EQU	00000001H
MCI_SET_AUDIO_RIGHT	EQU	00000002H
endif
; flags for dwFlags parameter of MCI_BREAK command message
MCI_BREAK_KEY	EQU	00000100H
MCI_BREAK_HWND	EQU	00000200H
MCI_BREAK_OFF	EQU	00000400H
; flags for dwFlags parameter of MCI_RECORD command message
MCI_RECORD_INSERT	EQU	00000100H
MCI_RECORD_OVERWRITE	EQU	00000200H
; flags for dwFlags parameter of MCI_SAVE command message
MCI_SAVE_FILE	EQU	00000100H
; flags for dwFlags parameter of MCI_LOAD command message
MCI_LOAD_FILE	EQU	00000100H

MCI_GENERIC_PARMS	STRUC
mcigen_dwCallback	DD	?
MCI_GENERIC_PARMS	ENDS

MCI_OPEN_PARMS	STRUC
mciopen_dwCallback	DD	?
mciopen_wDeviceID	DW	?
mciopen_wReserved0	DW	?
mciopen_lpstrDeviceType	DD	?
mciopen_lpstrElementName	DD	?
mciopen_lpstrAlias	DD	?
MCI_OPEN_PARMS	ENDS

MCI_PLAY_PARMS	STRUC
mciplay_dwCallback	DD	?
mciplay_dwFrom	DD	?
mciplay_dwTo	DD	?
MCI_PLAY_PARMS	ENDS

MCI_SEEK_PARMS	STRUC
mciseek_dwCallback	DD	?
mciseek_dwTo	DD	?
MCI_SEEK_PARMS	ENDS

MCI_STATUS_PARMS	STRUC
mcistat_dwCallback	DD	?
mcistat_dwReturn	DD	?
mcistat_dwItem	DD	?
mcistat_dwTrack	DD	?
MCI_STATUS_PARMS	ENDS

MCI_INFO_PARMS	STRUC
mciinfo_dwCallback	DD	?
mciinfo_lpstrReturn	DD	?
mciinfo_dwRetSize	DD	?
MCI_INFO_PARMS	ENDS

MCI_GETDEVCAPS_PARMS	STRUC
mcigdc_dwCallback	DD	?
mcigdc_dwReturn	DD	?
mcigdc_dwItem	DD	?
MCI_GETDEVCAPS_PARMS	ENDS

MCI_SYSINFO_PARMS	STRUC
mcisi_dwCallback	DD	?
mcisi_lpstrReturn	DD	?
mcisi_dwRetSize	DD	?
mcisi_dwNumber	DD	?
mcisi_wDeviceType	DW	?
mcisi_wReserved0	DW	?
MCI_SYSINFO_PARMS	ENDS

MCI_SET_PARMS	STRUC
mciset_dwCallback	DD	?
mciset_dwTimeFormat	DD	?
mciset_dwAudio	DD	?
MCI_SET_PARMS	ENDS

MCI_BREAK_PARMS	STRUC
mcibreak_dwCallback	DD	?
mcibreak_nVirtKey	DW	?
mcibreak_wReserved0	DW	?
mcibreak_hwndBreak	DW	?
mcibreak_wReserved1	DW	?
MCI_BREAK_PARMS	ENDS

MCI_SAVE_PARMS	STRUC
mcisave_dwCallback	DD	?
mcisave_lpfilename	DD	?
MCI_SAVE_PARMS	ENDS

MCI_LOAD_PARMS	STRUC
mciload_dwCallback	DD	?
mciload_lpfilename	DD	?
MCI_LOAD_PARMS	ENDS

MCI_RECORD_PARMS	STRUC
mcirec_dwCallback	DD	?
mcirec_dwFrom	DD	?
mcirec_dwTo	DD	?
MCI_RECORD_PARMS	ENDS
; MCI extensions for videodisc devices
; flag for dwReturn field of MCI_STATUS_PARMS
; MCI_STATUS command, (dwItem == MCI_STATUS_MODE)
MCI_VD_MODE_PARK	EQU	(MCI_VD_OFFSET+1)
; flag for dwReturn field of MCI_STATUS_PARMS
; MCI_STATUS command, (dwItem == MCI_VD_STATUS_MEDIA_TYPE)
MCI_VD_MEDIA_CLV	EQU	(MCI_VD_OFFSET+2)
MCI_VD_MEDIA_CAV	EQU	(MCI_VD_OFFSET+3)
MCI_VD_MEDIA_OTHER	EQU	(MCI_VD_OFFSET+4)
MCI_VD_FORMAT_TRACK	EQU	4001H
; flags for dwFlags parameter of MCI_PLAY command message
MCI_VD_PLAY_REVERSE	EQU	00010000H
MCI_VD_PLAY_FAST	EQU	00020000H
MCI_VD_PLAY_SPEED	EQU	00040000H
MCI_VD_PLAY_SCAN	EQU	00080000H
MCI_VD_PLAY_SLOW	EQU	00100000H
; flag for dwFlags parameter of MCI_SEEK command message
MCI_VD_SEEK_REVERSE	EQU	00010000H
; flags for dwItem field of MCI_STATUS_PARMS parameter block
MCI_VD_STATUS_SPEED	EQU	00004002H
MCI_VD_STATUS_FORWARD	EQU	00004003H
MCI_VD_STATUS_MEDIA_TYPE	EQU	00004004H
MCI_VD_STATUS_SIDE	EQU	00004005H
MCI_VD_STATUS_DISC_SIZE	EQU	00004006H
; flags for dwFlags parameter of MCI_GETDEVCAPS command message
MCI_VD_GETDEVCAPS_CLV	EQU	00010000H
MCI_VD_GETDEVCAPS_CAV	EQU	00020000H
MCI_VD_SPIN_UP	EQU	00010000H
MCI_VD_SPIN_DOWN	EQU	00020000H
; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_VD_GETDEVCAPS_CAN_REVERSE	EQU	00004002H
MCI_VD_GETDEVCAPS_FAST_RATE	EQU	00004003H
MCI_VD_GETDEVCAPS_SLOW_RATE	EQU	00004004H
MCI_VD_GETDEVCAPS_NORMAL_RATE	EQU	00004005H
; flags for the dwFlags parameter of MCI_STEP command message
MCI_VD_STEP_FRAMES	EQU	00010000H
MCI_VD_STEP_REVERSE	EQU	00020000H
; flag for the MCI_ESCAPE command message
MCI_VD_ESCAPE_STRING	EQU	00000100H

MCI_VD_PLAY_PARMS	STRUC
mcivdplay_dwCallback	DD	?
mcivdplay_dwFrom	DD	?
mcivdplay_dwTo	DD	?
mcivdplay_dwSpeed	DD	?
MCI_VD_PLAY_PARMS	ENDS

MCI_VD_STEP_PARMS	STRUC
mcivdstep_dwCallback	DD	?
mcivdstep_dwFrames	DD	?
MCI_VD_STEP_PARMS	ENDS

MCI_VD_ESCAPE_PARMS	STRUC
mcivcesc_dwCallback	DD	?
mcivcesc_lpstrCommand	DD	?
MCI_VD_ESCAPE_PARMS	ENDS
; MCI extensions for CD audio devices
; flags for the dwItem field of the MCI_STATUS_PARMS parameter block
MCI_CDA_STATUS_TYPE_TRACK	EQU	00004001H
; flags for the dwReturn field of MCI_STATUS_PARMS parameter block
; MCI_STATUS command, (dwItem == MCI_CDA_STATUS_TYPE_TRACK)
MCI_CDA_TRACK_AUDIO	EQU	(MCI_CD_OFFSET+0)
MCI_CDA_TRACK_OTHER	EQU	(MCI_CD_OFFSET+1)
; MCI extensions for waveform audio devices
; flags for the dwFlags parameter of MCI_OPEN command message
MCI_WAVE_OPEN_BUFFER	EQU	00010000H
; flags for the dwFlags parameter of MCI_SET command message
MCI_WAVE_SET_FORMATTAG	EQU	00010000H
MCI_WAVE_SET_CHANNELS	EQU	00020000H
MCI_WAVE_SET_SAMPLESPERSEC	EQU	00040000H
MCI_WAVE_SET_AVGBYTESPERSEC	EQU	00080000H
MCI_WAVE_SET_BLOCKALIGN	EQU	00100000H
MCI_WAVE_SET_BITSPERSAMPLE	EQU	00200000H
; flags for the dwFlags parameter of MCI_STATUS, MCI_SET command messages
MCI_WAVE_INPUT	EQU	00400000H
MCI_WAVE_OUTPUT	EQU	00800000H
; flags for the dwItem field of MCI_STATUS_PARMS parameter block
MCI_WAVE_STATUS_FORMATTAG	EQU	00004001H
MCI_WAVE_STATUS_CHANNELS	EQU	00004002H
MCI_WAVE_STATUS_SAMPLESPERSEC	EQU	00004003H
MCI_WAVE_STATUS_AVGBYTESPERSEC	EQU	00004004H
MCI_WAVE_STATUS_BLOCKALIGN	EQU	00004005H
MCI_WAVE_STATUS_BITSPERSAMPLE	EQU	00004006H
MCI_WAVE_STATUS_LEVEL	EQU	00004007H
; flags for the dwFlags parameter of MCI_SET command message
MCI_WAVE_SET_ANYINPUT	EQU	04000000H
MCI_WAVE_SET_ANYOUTPUT	EQU	08000000H
; flags for the dwFlags parameter of MCI_GETDEVCAPS command message
MCI_WAVE_GETDEVCAPS_INPUTS	EQU	00004001H
MCI_WAVE_GETDEVCAPS_OUTPUTS	EQU	00004002H

MCI_WAVE_OPEN_PARMS	STRUC
mciwopen_dwCallback	DD	?
mciwopen_wDeviceID	DW	?
mciwopen_wReserved0	DW	?
mciwopen_lpstrDeviceType	DD	?
mciwopen_lpstrElementName	DD	?
mciwopen_lpstrAlias	DD	?
mciwopen_dwBufferSeconds	DD	?
MCI_WAVE_OPEN_PARMS	ENDS

MCI_WAVE_DELETE_PARMS	STRUC
mciwdel_dwCallback	DD	?
mciwdel_dwFrom	DD	?
mciwdel_dwTo	DD	?
MCI_WAVE_DELETE_PARMS	ENDS

MCI_WAVE_SET_PARMS	STRUC
mciwset_dwCallback	DD	?
mciwset_dwTimeFormat	DD	?
mciwset_dwAudio	DD	?
mciwset_wInput	DW	?
mciwset_wReserved0	DW	?
mciwset_wOutput	DW	?
mciwset_wReserved1	DW	?
mciwset_wFormatTag	DW	?
mciwset_wReserved2	DW	?
mciwset_nChannels	DW	?
mciwset_wReserved3	DW	?
mciwset_nSamplesPerSec	DW	?
mciwset_nAvgBytesPerSec	DW	?
mciwset_nBlockAlign	DW	?
mciwset_wReserved4	DW	?
mciwset_wBitsPerSample	DW	?
mciwset_wReserved5	DW	?
MCI_WAVE_SET_PARMS	ENDS
; MCI extensions for MIDI sequencer devices
; flags for the dwReturn field of MCI_STATUS_PARMS parameter block
; MCI_STATUS command, (dwItem == MCI_SEQ_STATUS_DIVTYPE)
MCI_SEQ_DIV_PPQN	EQU	(0+MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_24	EQU	(1+MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_25	EQU	(2+MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_30DROP	EQU	(3+MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_30	EQU	(4+MCI_SEQ_OFFSET)
; flags for the dwMaster field of MCI_SEQ_SET_PARMS parameter block
; MCI_SET command, (dwFlags == MCI_SEQ_SET_MASTER)
MCI_SEQ_FORMAT_SONGPTR	EQU	4001H
MCI_SEQ_FILE	EQU	4002H
MCI_SEQ_MIDI	EQU	4003H
MCI_SEQ_SMPTE	EQU	4004H
MCI_SEQ_NONE	EQU	65533
; flags for the dwItem field of MCI_STATUS_PARMS parameter block
MCI_SEQ_STATUS_TEMPO	EQU	00004002H
MCI_SEQ_STATUS_PORT	EQU	00004003H
MCI_SEQ_STATUS_SLAVE	EQU	00004007H
MCI_SEQ_STATUS_MASTER	EQU	00004008H
MCI_SEQ_STATUS_OFFSET	EQU	00004009H
MCI_SEQ_STATUS_DIVTYPE	EQU	0000400AH
; flags for the dwFlags parameter of MCI_SET command message
MCI_SEQ_SET_TEMPO	EQU	00010000H
MCI_SEQ_SET_PORT	EQU	00020000H
MCI_SEQ_SET_SLAVE	EQU	00040000H
MCI_SEQ_SET_MASTER	EQU	00080000H
MCI_SEQ_SET_OFFSET	EQU	01000000H

MCI_SEQ_SET_PARMS	STRUC
mcisset_dwCallback	DD	?
mcisset_dwTimeFormat	DD	?
mcisset_dwAudio	DD	?
mcisset_dwTempo	DD	?
mcisset_dwPort	DD	?
mcisset_dwSlave	DD	?
mcisset_dwMaster	DD	?
mcisset_dwOffset	DD	?
MCI_SEQ_SET_PARMS	ENDS
; MCI extensions for animation devices
; flags for dwFlags parameter of MCI_OPEN command message
MCI_ANIM_OPEN_WS	EQU	00010000H
MCI_ANIM_OPEN_PARENT	EQU	00020000H
MCI_ANIM_OPEN_NOSTATIC	EQU	00040000H
; flags for dwFlags parameter of MCI_PLAY command message
MCI_ANIM_PLAY_SPEED	EQU	00010000H
MCI_ANIM_PLAY_REVERSE	EQU	00020000H
MCI_ANIM_PLAY_FAST	EQU	00040000H
MCI_ANIM_PLAY_SLOW	EQU	00080000H
MCI_ANIM_PLAY_SCAN	EQU	00100000H
; flags for dwFlags parameter of MCI_STEP command message
MCI_ANIM_STEP_REVERSE	EQU	00010000H
MCI_ANIM_STEP_FRAMES	EQU	00020000H
; flags for dwItem field of MCI_STATUS_PARMS parameter block
MCI_ANIM_STATUS_SPEED	EQU	00004001H
MCI_ANIM_STATUS_FORWARD	EQU	00004002H
MCI_ANIM_STATUS_HWND	EQU	00004003H
MCI_ANIM_STATUS_HPAL	EQU	00004004H
MCI_ANIM_STATUS_STRETCH	EQU	00004005H
; flags for the dwFlags parameter of MCI_INFO command message
MCI_ANIM_INFO_TEXT	EQU	00010000H
; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_ANIM_GETDEVCAPS_CAN_REVERSE	EQU	00004001H
MCI_ANIM_GETDEVCAPS_FAST_RATE	EQU	00004002H
MCI_ANIM_GETDEVCAPS_SLOW_RATE	EQU	00004003H
MCI_ANIM_GETDEVCAPS_NORMAL_RATE	EQU	00004004H
MCI_ANIM_GETDEVCAPS_PALETTES	EQU	00004006H
MCI_ANIM_GETDEVCAPS_CAN_STRETCH	EQU	00004007H
MCI_ANIM_GETDEVCAPS_MAX_WINDOWS	EQU	00004008H
; flags for the MCI_REALIZE command message
MCI_ANIM_REALIZE_NORM	EQU	00010000H
MCI_ANIM_REALIZE_BKGD	EQU	00020000H
; flags for dwFlags parameter of MCI_WINDOW command message
MCI_ANIM_WINDOW_HWND	EQU	00010000H
MCI_ANIM_WINDOW_STATE	EQU	00040000H
MCI_ANIM_WINDOW_TEXT	EQU	00080000H
MCI_ANIM_WINDOW_ENABLE_STRETCH	EQU	00100000H
MCI_ANIM_WINDOW_DISABLE_STRETCH	EQU	00200000H
; flags for hWnd field of MCI_ANIM_WINDOW_PARMS parameter block
; MCI_WINDOW command message, (dwFlags == MCI_ANIM_WINDOW_HWND)
MCI_ANIM_WINDOW_DEFAULT	EQU	00000000H
; flags for dwFlags parameter of MCI_PUT command message
MCI_ANIM_RECT	EQU	00010000H
MCI_ANIM_PUT_SOURCE	EQU	00020000H
MCI_ANIM_PUT_DESTINATION	EQU	00040000H
; flags for dwFlags parameter of MCI_WHERE command message
MCI_ANIM_WHERE_SOURCE	EQU	00020000H
MCI_ANIM_WHERE_DESTINATION	EQU	00040000H
; flags for dwFlags parameter of MCI_UPDATE command message
MCI_ANIM_UPDATE_HDC	EQU	00020000H

MCI_ANIM_OPEN_PARMS	STRUC
mciaopen_dwCallback	DD	?
mciaopen_wDeviceID	DW	?
mciaopen_wReserved0	DW	?
mciaopen_lpstrDeviceType	DD	?
mciaopen_lpstrElementName	DD	?
mciaopen_lpstrAlias	DD	?
mciaopen_dwStyle	DD	?
mciaopen_hWndParent	DW	?
mciaopen_wReserved1	DW	?
MCI_ANIM_OPEN_PARMS	ENDS

MCI_ANIM_PLAY_PARMS	STRUC
mciaplay_dwCallback	DD	?
mciaplay_dwFrom	DD	?
mciaplay_dwTo	DD	?
mciaplay_dwSpeed	DD	?
MCI_ANIM_PLAY_PARMS	ENDS

MCI_ANIM_STEP_PARMS	STRUC
mciastep_dwCallback	DD	?
mciastep_dwFrames	DD	?
MCI_ANIM_STEP_PARMS	ENDS

MCI_ANIM_WINDOW_PARMS	STRUC
mciawin_dwCallback	DD	?
mciawin_hWnd	DW	?
mciawin_wReserved1	DW	?
mciawin_nCmdShow	DW	?
mciawin_wReserved2	DW	?
mciawin_lpstrText	DD	?
MCI_ANIM_WINDOW_PARMS	ENDS

MCI_ANIM_RECT_PARMS	STRUC
mciarect_dwCallback	DD	?
ifdef MCI_USE_OFFEXT
mciarect_ptOffset	DB	SIZE POINT DUP (?)
mciarect_ptExtent	DB	SIZE POINT DUP (?)
else	;ifdef MCI_USE_OFFEXT
mciarect_rc	DB	SIZE RECT DUP (?)
endif  ;ifdef MCI_USE_OFFEXT
MCI_ANIM_RECT_PARMS	ENDS

MCI_ANIM_UPDATE_PARMS	STRUC
mciaupd_dwCallback	DD	?
mciaupd_rc	DB	SIZE RECT DUP (?)
mciaupd_hDC	DW	?
MCI_ANIM_UPDATE_PARMS	ENDS
; MCI extensions for video overlay devices
; flags for dwFlags parameter of MCI_OPEN command message
MCI_OVLY_OPEN_WS	EQU	00010000H
MCI_OVLY_OPEN_PARENT	EQU	00020000H
; flags for dwFlags parameter of MCI_STATUS command message
MCI_OVLY_STATUS_HWND	EQU	00004001H
MCI_OVLY_STATUS_STRETCH	EQU	00004002H
; flags for dwFlags parameter of MCI_INFO command message
MCI_OVLY_INFO_TEXT	EQU	00010000H
; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_OVLY_GETDEVCAPS_CAN_STRETCH	EQU	00004001H
MCI_OVLY_GETDEVCAPS_CAN_FREEZE	EQU	00004002H
MCI_OVLY_GETDEVCAPS_MAX_WINDOWS	EQU	00004003H
; flags for dwFlags parameter of MCI_WINDOW command message
MCI_OVLY_WINDOW_HWND	EQU	00010000H
MCI_OVLY_WINDOW_STATE	EQU	00040000H
MCI_OVLY_WINDOW_TEXT	EQU	00080000H
MCI_OVLY_WINDOW_ENABLE_STRETCH	EQU	00100000H
MCI_OVLY_WINDOW_DISABLE_STRETCH	EQU	00200000H
; flags for hWnd parameter of MCI_OVLY_WINDOW_PARMS parameter block
MCI_OVLY_WINDOW_DEFAULT	EQU	00000000H
; flags for dwFlags parameter of MCI_PUT command message
MCI_OVLY_RECT	EQU	00010000H
MCI_OVLY_PUT_SOURCE	EQU	00020000H
MCI_OVLY_PUT_DESTINATION	EQU	00040000H
MCI_OVLY_PUT_FRAME	EQU	00080000H
MCI_OVLY_PUT_VIDEO	EQU	00100000H
; flags for dwFlags parameter of MCI_WHERE command message
MCI_OVLY_WHERE_SOURCE	EQU	00020000H
MCI_OVLY_WHERE_DESTINATION	EQU	00040000H
MCI_OVLY_WHERE_FRAME	EQU	00080000H
MCI_OVLY_WHERE_VIDEO	EQU	00100000H

MCI_OVLY_OPEN_PARMS	STRUC
mcioopen_dwCallback	DD	?
mcioopen_wDeviceID	DW	?
mcioopen_wReserved0	DW	?
mcioopen_lpstrDeviceType	DD	?
mcioopen_lpstrElementName	DD	?
mcioopen_lpstrAlias	DD	?
mcioopen_dwStyle	DD	?
mcioopen_hWndParent	DW	?
mcioopen_wReserved1	DW	?
MCI_OVLY_OPEN_PARMS	ENDS

MCI_OVLY_WINDOW_PARMS	STRUC
mciowin_dwCallback	DD	?
mciowin_hWnd	DW	?
mciowin_wReserved1	DW	?
mciowin_nCmdShow	DW	?
mciowin_wReserved2	DW	?
mciowin_lpstrText	DD	?
MCI_OVLY_WINDOW_PARMS	ENDS

MCI_OVLY_RECT_PARMS	STRUC
mciorect_dwCallback	DD	?
ifdef MCI_USE_OFFEXT
mciorect_ptOffset	DB	SIZE POINT DUP (?)
mciorect_ptExtent	DB	SIZE POINT DUP (?)
else	;ifdef MCI_USE_OFFEXT
mciorect_rc	DB	SIZE RECT DUP (?)
endif  ;ifdef MCI_USE_OFFEXT
MCI_OVLY_RECT_PARMS	ENDS

MCI_OVLY_SAVE_PARMS	STRUC
mciosave_dwCallback	DD	?
mciosave_lpfilename	DD	?
mciosave_rc	DB	SIZE RECT DUP (?)
MCI_OVLY_SAVE_PARMS	ENDS

MCI_OVLY_LOAD_PARMS	STRUC
mcioload_dwCallback	DD	?
mcioload_lpfilename	DD	?
mcioload_rc	DB	SIZE RECT DUP (?)
MCI_OVLY_LOAD_PARMS	ENDS
endif  ; ifndef MMNOMCI
;***************************************************************************
;	DISPLAY Driver extensions
;*************************************************************************
ifndef C1_TRANSPARENT
CAPS1	EQU	94          ; other caps
C1_TRANSPARENT	EQU	0001H      ; new raster cap
NEWTRANSPARENT	EQU	3           ; use with SetBkMode()
QUERYROPSUPPORT	EQU	40          ; use to determine ROP support
endif  ; ifndef C1_TRANSPARENT
;***************************************************************************
;	DIB Driver extensions
;*************************************************************************
SELECTDIB	EQU	41                      ; DIB.DRV select dib escape
ifndef SC_SCREENSAVE
SC_SCREENSAVE	EQU	0F140H
endif  ; ifndef SC_SCREENSAVE
