/*==========================================================================;
 *
 *  mmddk.h -- Include file for Multimedia Device Development
 *
 *  Version 4.00
 *
 *  Copyright (C) 1992-1994 Microsoft Corporation.  All rights reserved.
 *
 *--------------------------------------------------------------------------;
 *  Note: You must include the WINDOWS.H and MMSYSTEM.H header files
 *        before including this file.
 *
 *  Define:         Prevent inclusion of:
 *  --------------  --------------------------------------------------------
 *  MMNOMIDIDEV     MIDI support
 *  MMNOWAVEDEV     Waveform support
 *  MMNOAUXDEV      Auxiliary output support
 *  MMNOMIXERDEV    Mixer support
 *  MMNOTIMERDEV    Timer support
 *  MMNOJOYDEV      Joystick support
 *  MMNOMCIDEV      MCI support
 *  MMNOTASKDEV     Task support
 *
 *==========================================================================;
 */

#ifndef _INC_MMDDK
#define _INC_MMDDK   /* #defined if mmddk.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/***************************************************************************

                       Helper functions for drivers

***************************************************************************/

#ifdef WIN32

#ifndef NODRIVERS
#define DRV_LOAD                0x0001
#define DRV_ENABLE              0x0002
#define DRV_OPEN                0x0003
#define DRV_CLOSE               0x0004
#define DRV_DISABLE             0x0005
#define DRV_FREE                0x0006
#define DRV_CONFIGURE           0x0007
#define DRV_QUERYCONFIGURE      0x0008
#define DRV_INSTALL             0x0009
#define DRV_REMOVE              0x000A

#define DRV_RESERVED            0x0800
#define DRV_USER                0x4000

#define DRIVERS_SECTION  TEXT("DRIVERS32")     // Section name for installed drivers
#define MCI_SECTION      TEXT("MCI32")         // Section name for installed MCI drivers

LRESULT   WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
#endif /* !NODRIVERS */

#endif /* ifdef WIN32 */

#if (WINVER < 0x0400)
#define DCB_NOSWITCH   0x0008           /* obsolete switch */
#endif
#define DCB_TYPEMASK   0x0007           /* callback type mask */
#define DCB_NULL       0x0000           /* unknown callback type */

/* flags for wFlags parameter of DriverCallback() */
#define DCB_WINDOW     0x0001           /* dwCallback is a HWND */
#define DCB_TASK       0x0002           /* dwCallback is a HTASK */
#define DCB_FUNCTION   0x0003           /* dwCallback is a FARPROC */
#define DCB_WINDOW32   0x0004           /* dwCallback is a WINDOW */ /* ;Internal */

BOOL WINAPI DriverCallback(DWORD dwCallback, UINT uFlags,
    HANDLE hDevice, UINT uMessage, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

#if (WINVER < 0x0400)
void WINAPI StackEnter(void);
void WINAPI StackLeave(void);
#endif

/* generic prototype for audio device driver entry-point functions */
/* midMessage(), modMessage(), widMessage(), wodMessage(), auxMessage() */
typedef DWORD (CALLBACK SOUNDDEVMSGPROC)(UINT uDeviceID, UINT uMessage,
    DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
typedef SOUNDDEVMSGPROC FAR *LPSOUNDDEVMSGPROC;

/* 
 * Message sent by mmsystem to media specific entry points when it first
 * initializes the drivers, and when they are closed.
 */

#define DRVM_INIT               100
#define DRVM_EXIT		101

// message base for driver specific messages.
// 
#define DRVM_MAPPER             0x2000
#define DRVM_USER               0x4000
#define DRVM_MAPPER_STATUS      (DRVM_MAPPER+0)
#define	DRVM_MAPPER_RECONFIGURE	(DRVM_MAPPER+1)

#if (WINVER >= 0x0400)
#define DRV_QUERYDRVENTRY    (DRV_RESERVED + 1)
#define DRV_QUERYDEVNODE     (DRV_RESERVED + 2)
#define DRV_QUERYNAME        (DRV_RESERVED + 3)

#define	DRV_F_ADD	0x00000000
#define	DRV_F_REMOVE	0x00000001
#define	DRV_F_CHANGE	0x00000002
#define DRV_F_PROP_INSTR 0x00000004
#define DRV_F_PARAM_IS_DEVNODE 0x10000000
#endif

/* PnP version of device caps */
typedef struct {
    DWORD	cbSize;
    LPVOID	pCaps;
} DEVICECAPSEX;

#ifndef MMNOWAVEDEV
/****************************************************************************
 
                       Waveform device driver support
 
****************************************************************************/

/* maximum number of wave device drivers loaded */
#define MAXWAVEDRIVERS          10

/* waveform input and output device open information structure */
typedef struct waveopendesc_tag {
    HWAVE          hWave;             /* handle */
    const WAVEFORMAT FAR* lpFormat;   /* format of wave data */
    DWORD          dwCallback;        /* callback */
    DWORD          dwInstance;        /* app's private instance information */
    UINT           uMappedDeviceID;   /* device to map to if WAVE_MAPPED set */
    DWORD         dnDevNode;          /* if device is PnP */
} WAVEOPENDESC;
typedef WAVEOPENDESC FAR *LPWAVEOPENDESC;

#define WODM_USER               DRVM_USER
#define WIDM_USER               DRVM_USER
#define WODM_MAPPER             DRVM_MAPPER
#define WIDM_MAPPER             DRVM_MAPPER

#define WODM_INIT               DRVM_INIT
#define WIDM_INIT               DRVM_INIT

/* messages sent to wodMessage() entry-point function */
#define WODM_GETNUMDEVS         3
#define WODM_GETDEVCAPS         4
#define WODM_OPEN               5
#define WODM_CLOSE              6
#define WODM_PREPARE            7
#define WODM_UNPREPARE          8
#define WODM_WRITE              9
#define WODM_PAUSE              10
#define WODM_RESTART            11
#define WODM_RESET              12 
#define WODM_GETPOS             13
#define WODM_GETPITCH           14
#define WODM_SETPITCH           15
#define WODM_GETVOLUME          16
#define WODM_SETVOLUME          17
#define WODM_GETPLAYBACKRATE    18
#define WODM_SETPLAYBACKRATE    19
#define WODM_BREAKLOOP          20
#if (WINVER >= 0x0400)
#define WODM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#define WAVEOUT_MAPPER_STATUS_FORMAT    2
#endif

/* messages sent to widMessage() entry-point function */
#define WIDM_GETNUMDEVS         50
#define WIDM_GETDEVCAPS         51
#define WIDM_OPEN               52
#define WIDM_CLOSE              53
#define WIDM_PREPARE            54
#define WIDM_UNPREPARE          55
#define WIDM_ADDBUFFER          56
#define WIDM_START              57
#define WIDM_STOP               58
#define WIDM_RESET              59
#define WIDM_GETPOS             60
#if (WINVER >= 0x0400)
#define WIDM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#define WAVEIN_MAPPER_STATUS_FORMAT     2
#endif
#endif  /*ifndef MMNOWAVEDEV */


#ifndef MMNOMIDIDEV
/****************************************************************************

                          MIDI device driver support

****************************************************************************/

/* maximum number of MIDI device drivers loaded */
#define MAXMIDIDRIVERS 10

#define MODM_USER      DRVM_USER
#define MIDM_USER      DRVM_USER
#define MODM_MAPPER    DRVM_MAPPER
#define MIDM_MAPPER    DRVM_MAPPER

#define MODM_INIT      DRVM_INIT
#define MIDM_INIT      DRVM_INIT

/* MIDI input and output device open information structure */
typedef struct midiopendesc_tag {
    HMIDI          hMidi;             /* handle */
    DWORD          dwCallback;        /* callback */
    DWORD          dwInstance;        /* app's private instance information */
    UINT           uMappedDeviceID;   /* device to map to if WAVE_MAPPED set */
    DWORD          dnDevNode;         /* if device is PnP */
} MIDIOPENDESC;
typedef MIDIOPENDESC FAR *LPMIDIOPENDESC;

#if (WINVER >= 0x0400)
/* structure pointed to by lParam1 for MODM_GET/MODM_SETTIMEPARMS */

typedef struct miditimeparms_tag {
    DWORD	    dwTimeDivision;   /* time division ala MIDI file spec */
    DWORD	    dwTempo;	      /* tempo ala MIDI file spec */
} MIDITIMEPARMS,
  FAR *LPMIDITIMEPARMS;

#endif


/* messages sent to modMessage() entry-point function */
#define MODM_GETNUMDEVS             1
#define MODM_GETDEVCAPS             2
#define MODM_OPEN                   3
#define MODM_CLOSE                  4
#define MODM_PREPARE                5
#define MODM_UNPREPARE              6
#define MODM_DATA                   7
#define MODM_LONGDATA               8
#define MODM_RESET                  9
#define MODM_GETVOLUME              10
#define MODM_SETVOLUME              11
#define MODM_CACHEPATCHES           12      
#define MODM_CACHEDRUMPATCHES	    13

#if (WINVER >= 0x0400)
#define MODM_POLYMSG                14
//#define MODM_SETTIMEPARMS           15
//#define MODM_GETTIMEPARMS           16
#define MODM_GETPOS                 17
#define MODM_PAUSE                  18
#define MODM_RESTART                19
#define MODM_STOP                   20
#define MODM_PROPERTIES             21
#define MODM_RECONFIGURE			(MODM_USER+0x0768)
#endif



/* messages sent to midMessage() entry-point function */
#define MIDM_GETNUMDEVS             53
#define MIDM_GETDEVCAPS             54
#define MIDM_OPEN                   55
#define MIDM_CLOSE                  56
#define MIDM_PREPARE                57
#define MIDM_UNPREPARE              58
#define MIDM_ADDBUFFER              59
#define MIDM_START                  60
#define MIDM_STOP                   61
#define MIDM_RESET                  62
#if (WINVER >= 0x0400)
//#define MIDM_SETTIMEPARMS           63
//#define MIDM_GETTIMEPARMS           64  /* Who will need to call this? */
#define MIDM_GETPOS                 65
#define MIDM_PROPERTIES             66
#endif

#endif  /*ifndef MMNOMIDIDEV */


#ifndef MMNOAUXDEV
/****************************************************************************

                    Auxiliary audio device driver support

****************************************************************************/

/* maximum number of auxiliary device drivers loaded */
#define MAXAUXDRIVERS           10

#define AUXM_INIT               DRVM_INIT
#define AUXM_USER               DRVM_USER
#define AUXDM_MAPPER            DRVM_MAPPER

/* messages sent to auxMessage() entry-point function */
#define AUXDM_GETNUMDEVS        3
#define AUXDM_GETDEVCAPS        4
#define AUXDM_GETVOLUME         5
#define AUXDM_SETVOLUME         6

#endif  /*ifndef MMNOAUXDEV */




#ifndef MMNOMIXERDEV
#if (WINVER >= 0x0400)
/****************************************************************************

                        Mixer Driver Support

****************************************************************************/

//
//  maximum number of mixer drivers that can be loaded by MSMIXMGR.DLL
//
#define MAXMIXERDRIVERS         10

//
//  mixer device open information structure
//
//
typedef struct tMIXEROPENDESC
{
    HMIXER          hmx;            // handle that will be used
    LPVOID          pReserved0;     // reserved--driver should ignore
    DWORD           dwCallback;     // callback
    DWORD           dwInstance;     // app's private instance information
    DWORD           dnDevNode;      // if device is PnP

} MIXEROPENDESC, *PMIXEROPENDESC, FAR *LPMIXEROPENDESC;

//
//
//
//
#define MXDM_INIT                  DRVM_INIT
#define MXDM_USER                  DRVM_USER
#define MXDM_MAPPER                DRVM_MAPPER

#define MXDM_BASE                   (1)
#define MXDM_GETNUMDEVS             (MXDM_BASE + 0)
#define MXDM_GETDEVCAPS             (MXDM_BASE + 1)
#define MXDM_OPEN                   (MXDM_BASE + 2)
#define MXDM_CLOSE                  (MXDM_BASE + 3)
#define MXDM_GETLINEINFO            (MXDM_BASE + 4)
#define MXDM_GETLINECONTROLS        (MXDM_BASE + 5)
#define MXDM_GETCONTROLDETAILS      (MXDM_BASE + 6)
#define MXDM_SETCONTROLDETAILS      (MXDM_BASE + 7)

#endif /* ifdef WINVER >= 0x0400 */
#endif /* ifndef MMNOMIXERDEV */


#ifndef MMNOTIMERDEV
/****************************************************************************

                        Timer device driver support

****************************************************************************/

typedef struct timerevent_tag {
    UINT                wDelay;         /* delay required */
    UINT                wResolution;    /* resolution required */
    LPTIMECALLBACK      lpFunction;     /* ptr to callback function */
    DWORD               dwUser;         /* user DWORD */
    UINT                wFlags;         /* defines how to program event */
} TIMEREVENT;
typedef TIMEREVENT FAR *LPTIMEREVENT;

/* messages sent to tddMessage() function */
#define TDD_KILLTIMEREVENT      (DRV_RESERVED + 0)  /* indices into a table of */
#define TDD_SETTIMEREVENT       (DRV_RESERVED + 4)  /* functions; thus offset by */
#define TDD_GETSYSTEMTIME       (DRV_RESERVED + 8)  /* four each time... */
#define TDD_GETDEVCAPS          (DRV_RESERVED + 12) /* room for future expansion */
#define TDD_BEGINMINPERIOD      (DRV_RESERVED + 16) /* room for future expansion */
#define TDD_ENDMINPERIOD        (DRV_RESERVED + 20) /* room for future expansion */

#endif  /*ifndef MMNOTIMERDEV */


#ifndef MMNOJOYDEV
/****************************************************************************

                       Joystick device driver support

****************************************************************************/

/* joystick calibration info structure */
typedef struct joycalibrate_tag {
    UINT    wXbase;
    UINT    wXdelta;
    UINT    wYbase;
    UINT    wYdelta;
    UINT    wZbase;
    UINT    wZdelta;
} JOYCALIBRATE;
typedef JOYCALIBRATE FAR *LPJOYCALIBRATE;

/* prototype for joystick message function */
typedef UINT (CALLBACK JOYDEVMSGPROC)(DWORD dwID, UINT uMessage, LPARAM lParam1, LPARAM lParam2);
typedef JOYDEVMSGPROC FAR *LPJOYDEVMSGPROC;

/* messages sent to joystick driver's DriverProc() function */
#define JDD_GETNUMDEVS          (DRV_RESERVED + 0x0001)
#define JDD_GETDEVCAPS          (DRV_RESERVED + 0x0002)
#define JDD_GETPOS              (DRV_RESERVED + 0x0101)
#define JDD_SETCALIBRATION      (DRV_RESERVED + 0x0102)

#endif  /*ifndef MMNOJOYDEV */


#ifndef MMNOMCIDEV
/****************************************************************************

                        MCI device driver support

****************************************************************************/

/* internal MCI messages */
#define MCI_OPEN_DRIVER         (DRV_RESERVED + 0x0001)
#define MCI_CLOSE_DRIVER        (DRV_RESERVED + 0x0002)

#define MAKEMCIRESOURCE(wRet, wRes) MAKELRESULT((wRet), (wRes))

/* string return values only used with MAKEMCIRESOURCE */
#define MCI_FALSE                   (MCI_STRING_OFFSET + 19)
#define MCI_TRUE                    (MCI_STRING_OFFSET + 20)

/* resource string return values */
#define MCI_FORMAT_RETURN_BASE      MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S   (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S            (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S            (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S         (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S       (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S       (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S       (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S   (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S          (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S        (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S           (MCI_STRING_OFFSET + 31)

#define MCI_VD_FORMAT_TRACK_S       (MCI_VD_OFFSET + 5)

#define WAVE_FORMAT_PCM_S           (MCI_WAVE_OFFSET + 0)
#define WAVE_MAPPER_S               (MCI_WAVE_OFFSET + 1)

#define MCI_SEQ_MAPPER_S            (MCI_SEQ_OFFSET + 5)
#define MCI_SEQ_FILE_S              (MCI_SEQ_OFFSET + 6)
#define MCI_SEQ_MIDI_S              (MCI_SEQ_OFFSET + 7)
#define MCI_SEQ_SMPTE_S             (MCI_SEQ_OFFSET + 8)
#define MCI_SEQ_FORMAT_SONGPTR_S    (MCI_SEQ_OFFSET + 9)
#define MCI_SEQ_NONE_S              (MCI_SEQ_OFFSET + 10)
#define MIDIMAPPER_S                (MCI_SEQ_OFFSET + 11)

/* parameters for internal version of MCI_OPEN message sent from */
/* mciOpenDevice() to the driver */
typedef struct {
    MCIDEVICEID wDeviceID;         /* device ID */
    LPCSTR  lpstrParams;           /* parameter string for entry in SYSTEM.INI */
    UINT    wCustomCommandTable;   /* custom command table (0xFFFF if none) */
                                   /* filled in by the driver */
    UINT    wType;                 /* driver type */
                                   /* filled in by the driver */
} MCI_OPEN_DRIVER_PARMS,
FAR *LPMCI_OPEN_DRIVER_PARMS;

/* maximum length of an MCI device type */
#define MCI_MAX_DEVICE_TYPE_LENGTH 80

/* flags for mciSendCommandInternal() which direct mciSendString() how to */
/* interpret the return value */
#define MCI_RESOURCE_RETURNED   0x00010000  /* resource ID */
#define MCI_COLONIZED3_RETURN   0x00020000  /* colonized ID, 3 bytes data */
#define MCI_COLONIZED4_RETURN   0x00040000  /* colonized ID, 4 bytes data */
#define MCI_INTEGER_RETURNED    0x00080000  /* integer conversion needed */
#define MCI_RESOURCE_DRIVER     0x00100000  /* driver owns returned resource */

/* invalid command table ID */
#define MCI_NO_COMMAND_TABLE    -1

/* command table information type tags */
#define MCI_COMMAND_HEAD        0
#define MCI_STRING              1
#define MCI_INTEGER             2
#define MCI_END_COMMAND         3
#define MCI_RETURN              4
#define MCI_FLAG                5
#define MCI_END_COMMAND_LIST    6
#define MCI_RECT                7
#define MCI_CONSTANT            8
#define MCI_END_CONSTANT        9

/* function prototypes for MCI driver functions */
DWORD WINAPI mciGetDriverData(UINT uDeviceID);
BOOL  WINAPI mciSetDriverData(UINT uDeviceID, DWORD dwData);
UINT  WINAPI mciDriverYield(UINT uDeviceID);
BOOL  WINAPI mciDriverNotify(HWND hwndCallback, UINT uDeviceID,
    UINT uStatus);
#ifdef WIN32
UINT  WINAPI mciLoadCommandResource(HINSTANCE hInstance, LPCWSTR lpResName, UINT uType);
#else
UINT  WINAPI mciLoadCommandResource(HINSTANCE hInstance, LPCSTR lpResName, UINT uType);
#endif
BOOL  WINAPI mciFreeCommandResource(UINT uTable);

#endif  /*ifndef MMNOMCIDEV */


#ifndef MMNOTASKDEV
/*****************************************************************************

                               Task support

*****************************************************************************/

/* error return values */
#define TASKERR_NOTASKSUPPORT   1
#define TASKERR_OUTOFMEMORY     2

/* task support function prototypes */
#ifdef  BUILDDLL                                        /* ;Internal */
typedef void (FAR PASCAL TASKCALLBACK) (DWORD dwInst);  /* ;Internal */
#else   /*ifdef BUILDDLL*/                              /* ;Internal */
typedef void (CALLBACK TASKCALLBACK) (DWORD dwInst);
#endif  /*ifdef BUILDDLL*/                              /* ;Internal */

typedef TASKCALLBACK FAR *LPTASKCALLBACK;

UINT    WINAPI mmTaskCreate(LPTASKCALLBACK lpfnTaskProc, HTASK FAR * lphTask, DWORD dwInst);
UINT    WINAPI mmTaskBlock(HTASK h);
BOOL    WINAPI mmTaskSignal(HTASK h);
void    WINAPI mmTaskYield(void);
HTASK   WINAPI mmGetCurrentTask(void);


#endif  /*ifndef MMNOTASKDEV */

#define MMDDKINC                /* ;Internal */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif

#endif  /* _INC_MMDDK */
