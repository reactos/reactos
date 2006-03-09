/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * Purpose:   multimedia declarations (external to WINMM & MMSYSTEM DLLs
 *                                     for other DLLs (MCI, drivers...))
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 */
#ifndef __MMDDK_H
#define __MMDDK_H

#include <mmsystem.h>
#include <winbase.h>

/* Maxium drivers  */
#define MAXWAVEDRIVERS 10

#define MAX_MIDIINDRV 	(16)
/* For now I'm making 16 the maximum number of midi devices one can
 * have. This should be more than enough for everybody. But as a purist,
 * I intend to make it unbounded in the future, as soon as I figure
 * a good way to do so.
 */
#define MAX_MIDIOUTDRV 	(16)

/* ==================================
 *   Multimedia DDK compatible part
 * ================================== */

#include <pshpack1.h>

#define DRVM_INIT		100
#define DRVM_EXIT		101
#define DRVM_DISABLE		102
#define DRVM_ENABLE		103

/* messages that have IOCTL format
 *    dw1 = NULL or handle
 *    dw2 = NULL or ptr to DRVM_IOCTL_DATA
 *    return is MMRESULT
 */
#define DRVM_IOCTL		0x100
#define DRVM_ADD_THRU		(DRVM_IOCTL+1)
#define DRVM_REMOVE_THRU	(DRVM_IOCTL+2)
#define DRVM_IOCTL_LAST		(DRVM_IOCTL+5)
typedef struct {
    DWORD  dwSize; 	/* size of this structure */
    DWORD  dwCmd;  	/* IOCTL command code, 0x80000000 and above reserved for system */
} DRVM_IOCTL_DATA, *LPDRVM_IOCTL_DATA;

/* command code ranges for dwCmd field of DRVM_IOCTL message
 * - codes from 0 to 0x7FFFFFFF are user defined
 * - codes from 0x80000000 to 0xFFFFFFFF are reserved for future definition by microsoft
 */
#define DRVM_IOCTL_CMD_USER   0x00000000L
#define DRVM_IOCTL_CMD_SYSTEM 0x80000000L

#define DRVM_MAPPER			0x2000
#define DRVM_USER			0x4000
#define DRVM_MAPPER_STATUS		(DRVM_MAPPER+0)
#define DRVM_MAPPER_RECONFIGURE 	(DRVM_MAPPER+1)
#define DRVM_MAPPER_PREFERRED_GET	(DRVM_MAPPER+21)
#define DRVM_MAPPER_CONSOLEVOICECOM_GET	(DRVM_MAPPER+23)

#define DRV_QUERYDRVENTRY		(DRV_RESERVED + 1)
#define DRV_QUERYDEVNODE		(DRV_RESERVED + 2)
#define DRV_QUERYNAME			(DRV_RESERVED + 3)
#define DRV_QUERYDRIVERIDS		(DRV_RESERVED + 4)
#define DRV_QUERYMAPPABLE		(DRV_RESERVED + 5)
#define DRV_QUERYMODULE			(DRV_RESERVED + 9)
#define DRV_PNPINSTALL			(DRV_RESERVED + 11)
#define DRV_QUERYDEVICEINTERFACE	(DRV_RESERVED + 12)
#define DRV_QUERYDEVICEINTERFACESIZE	(DRV_RESERVED + 13)
#define DRV_QUERYSTRINGID		(DRV_RESERVED + 14)
#define DRV_QUERYSTRINGIDSIZE		(DRV_RESERVED + 15)
#define DRV_QUERYIDFROMSTRINGID		(DRV_RESERVED + 16)
#ifdef __WINESRC__
#define DRV_QUERYDSOUNDIFACE		(DRV_RESERVED + 20)
#define DRV_QUERYDSOUNDDESC		(DRV_RESERVED + 21)
#define DRV_QUERYDSOUNDGUID		(DRV_RESERVED + 22)
#endif

#define WODM_INIT		DRVM_INIT
#define WODM_GETNUMDEVS		 3
#define WODM_GETDEVCAPS		 4
#define WODM_OPEN		 5
#define WODM_CLOSE		 6
#define WODM_PREPARE		 7
#define WODM_UNPREPARE		 8
#define WODM_WRITE		 9
#define WODM_PAUSE		10
#define WODM_RESTART		11
#define WODM_RESET		12
#define WODM_GETPOS		13
#define WODM_GETPITCH		14
#define WODM_SETPITCH		15
#define WODM_GETVOLUME		16
#define WODM_SETVOLUME		17
#define WODM_GETPLAYBACKRATE	18
#define WODM_SETPLAYBACKRATE	19
#define WODM_BREAKLOOP		20
#define WODM_PREFERRED		21

#define WODM_MAPPER_STATUS      (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#define WAVEOUT_MAPPER_STATUS_FORMAT    2

#define WODM_BUSY		21

#define WIDM_INIT		DRVM_INIT
#define WIDM_GETNUMDEVS		50
#define WIDM_GETDEVCAPS		51
#define WIDM_OPEN		52
#define WIDM_CLOSE		53
#define WIDM_PREPARE		54
#define WIDM_UNPREPARE		55
#define WIDM_ADDBUFFER		56
#define WIDM_START		57
#define WIDM_STOP		58
#define WIDM_RESET		59
#define WIDM_GETPOS		60
#define WIDM_PREFERRED		61
#define WIDM_MAPPER_STATUS      (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#define WAVEIN_MAPPER_STATUS_FORMAT     2

#define MODM_INIT		DRVM_INIT
#define MODM_GETNUMDEVS		1
#define MODM_GETDEVCAPS		2
#define MODM_OPEN		3
#define MODM_CLOSE		4
#define MODM_PREPARE		5
#define MODM_UNPREPARE		6
#define MODM_DATA		7
#define MODM_LONGDATA		8
#define MODM_RESET          	9
#define MODM_GETVOLUME		10
#define MODM_SETVOLUME		11
#define MODM_CACHEPATCHES	12
#define MODM_CACHEDRUMPATCHES	13

#define MIDM_INIT		DRVM_INIT
#define MIDM_GETNUMDEVS  	53
#define MIDM_GETDEVCAPS  	54
#define MIDM_OPEN        	55
#define MIDM_CLOSE       	56
#define MIDM_PREPARE     	57
#define MIDM_UNPREPARE   	58
#define MIDM_ADDBUFFER   	59
#define MIDM_START       	60
#define MIDM_STOP        	61
#define MIDM_RESET       	62


#define AUXM_INIT             	DRVM_INIT
#define AUXDM_GETNUMDEVS    	3
#define AUXDM_GETDEVCAPS    	4
#define AUXDM_GETVOLUME     	5
#define AUXDM_SETVOLUME     	6

#define MXDM_INIT		DRVM_INIT
#define MXDM_USER               DRVM_USER
#define MXDM_MAPPER             DRVM_MAPPER

#define	MXDM_GETNUMDEVS		1
#define	MXDM_GETDEVCAPS		2
#define	MXDM_OPEN		3
#define	MXDM_CLOSE		4
#define	MXDM_GETLINEINFO	5
#define	MXDM_GETLINECONTROLS	6
#define	MXDM_GETCONTROLDETAILS	7
#define	MXDM_SETCONTROLDETAILS	8

/* pre-defined joystick types */
#define JOY_HW_NONE			0
#define JOY_HW_CUSTOM			1
#define JOY_HW_2A_2B_GENERIC		2
#define JOY_HW_2A_4B_GENERIC		3
#define JOY_HW_2B_GAMEPAD		4
#define JOY_HW_2B_FLIGHTYOKE		5
#define JOY_HW_2B_FLIGHTYOKETHROTTLE	6
#define JOY_HW_3A_2B_GENERIC		7
#define JOY_HW_3A_4B_GENERIC		8
#define JOY_HW_4B_GAMEPAD		9
#define JOY_HW_4B_FLIGHTYOKE		10
#define JOY_HW_4B_FLIGHTYOKETHROTTLE	11
#define JOY_HW_LASTENTRY		12

/* calibration flags */
#define	JOY_ISCAL_XY		0x00000001l	/* XY are calibrated */
#define	JOY_ISCAL_Z		0x00000002l	/* Z is calibrated */
#define	JOY_ISCAL_R		0x00000004l	/* R is calibrated */
#define	JOY_ISCAL_U		0x00000008l	/* U is calibrated */
#define	JOY_ISCAL_V		0x00000010l	/* V is calibrated */
#define	JOY_ISCAL_POV		0x00000020l	/* POV is calibrated */

/* point of view constants */
#define JOY_POV_NUMDIRS          4
#define JOY_POVVAL_FORWARD       0
#define JOY_POVVAL_BACKWARD      1
#define JOY_POVVAL_LEFT          2
#define JOY_POVVAL_RIGHT         3

/* Specific settings for joystick hardware */
#define JOY_HWS_HASZ		0x00000001l	/* has Z info? */
#define JOY_HWS_HASPOV		0x00000002l	/* point of view hat present */
#define JOY_HWS_POVISBUTTONCOMBOS 0x00000004l	/* pov done through combo of buttons */
#define JOY_HWS_POVISPOLL	0x00000008l	/* pov done through polling */
#define JOY_HWS_ISYOKE		0x00000010l	/* joystick is a flight yoke */
#define JOY_HWS_ISGAMEPAD	0x00000020l	/* joystick is a game pad */
#define JOY_HWS_ISCARCTRL	0x00000040l	/* joystick is a car controller */
/* X defaults to J1 X axis */
#define JOY_HWS_XISJ1Y		0x00000080l	/* X is on J1 Y axis */
#define JOY_HWS_XISJ2X		0x00000100l	/* X is on J2 X axis */
#define JOY_HWS_XISJ2Y		0x00000200l	/* X is on J2 Y axis */
/* Y defaults to J1 Y axis */
#define JOY_HWS_YISJ1X		0x00000400l	/* Y is on J1 X axis */
#define JOY_HWS_YISJ2X		0x00000800l	/* Y is on J2 X axis */
#define JOY_HWS_YISJ2Y		0x00001000l	/* Y is on J2 Y axis */
/* Z defaults to J2 Y axis */
#define JOY_HWS_ZISJ1X		0x00002000l	/* Z is on J1 X axis */
#define JOY_HWS_ZISJ1Y		0x00004000l	/* Z is on J1 Y axis */
#define JOY_HWS_ZISJ2X		0x00008000l	/* Z is on J2 X axis */
/* POV defaults to J2 Y axis, if it is not button based */
#define JOY_HWS_POVISJ1X	0x00010000l	/* pov done through J1 X axis */
#define JOY_HWS_POVISJ1Y	0x00020000l	/* pov done through J1 Y axis */
#define JOY_HWS_POVISJ2X	0x00040000l	/* pov done through J2 X axis */
/* R defaults to J2 X axis */
#define JOY_HWS_HASR		0x00080000l	/* has R (4th axis) info */
#define JOY_HWS_RISJ1X		0x00100000l	/* R done through J1 X axis */
#define JOY_HWS_RISJ1Y		0x00200000l	/* R done through J1 Y axis */
#define JOY_HWS_RISJ2Y		0x00400000l	/* R done through J2 X axis */
/* U & V for future hardware */
#define JOY_HWS_HASU		0x00800000l	/* has U (5th axis) info */
#define JOY_HWS_HASV		0x01000000l	/* has V (6th axis) info */

/* Usage settings */
#define JOY_US_HASRUDDER	0x00000001l	/* joystick configured with rudder */
#define JOY_US_PRESENT		0x00000002l	/* is joystick actually present? */
#define JOY_US_ISOEM		0x00000004l	/* joystick is an OEM defined type */


/* struct for storing x,y, z, and rudder values */
typedef struct joypos_tag {
    DWORD	dwX;
    DWORD	dwY;
    DWORD	dwZ;
    DWORD	dwR;
    DWORD	dwU;
    DWORD	dwV;
} JOYPOS, *LPJOYPOS;

/* struct for storing ranges */
typedef struct joyrange_tag {
    JOYPOS	jpMin;
    JOYPOS	jpMax;
    JOYPOS	jpCenter;
} JOYRANGE,*LPJOYRANGE;

typedef struct joyreguservalues_tag {
    DWORD	dwTimeOut;	/* value at which to timeout joystick polling */
    JOYRANGE	jrvRanges;	/* range of values app wants returned for axes */
    JOYPOS	jpDeadZone;	/* area around center to be considered
    				   as "dead". specified as a percentage
				   (0-100). Only X & Y handled by system driver */
} JOYREGUSERVALUES, *LPJOYREGUSERVALUES;

typedef struct joyreghwsettings_tag {
    DWORD	dwFlags;
    DWORD	dwNumButtons;		/* number of buttons */
} JOYREGHWSETTINGS, *LPJOYHWSETTINGS;

/* range of values returned by the hardware (filled in by calibration) */
typedef struct joyreghwvalues_tag {
    JOYRANGE	jrvHardware;		/* values returned by hardware */
    DWORD	dwPOVValues[JOY_POV_NUMDIRS];/* POV values returned by hardware */
    DWORD	dwCalFlags;		/* what has been calibrated */
} JOYREGHWVALUES, *LPJOYREGHWVALUES;

/* hardware configuration */
typedef struct joyreghwconfig_tag {
    JOYREGHWSETTINGS	hws;		/* hardware settings */
    DWORD		dwUsageSettings;/* usage settings */
    JOYREGHWVALUES	hwv;		/* values returned by hardware */
    DWORD		dwType;		/* type of joystick */
    DWORD		dwReserved;	/* reserved for OEM drivers */
} JOYREGHWCONFIG, *LPJOYREGHWCONFIG;

/* joystick calibration info structure */
typedef struct joycalibrate_tag {
    UINT    wXbase;
    UINT    wXdelta;
    UINT    wYbase;
    UINT    wYdelta;
    UINT    wZbase;
    UINT    wZdelta;
} JOYCALIBRATE;
typedef JOYCALIBRATE *LPJOYCALIBRATE;

/* prototype for joystick message function */
typedef UINT (CALLBACK * JOYDEVMSGPROC)(DWORD dwID, UINT uMessage, LPARAM lParam1, LPARAM lParam2);
typedef JOYDEVMSGPROC *LPJOYDEVMSGPROC;

/* messages sent to joystick driver's DriverProc() function */
#define JDD_GETNUMDEVS          (DRV_RESERVED + 0x0001)
#define JDD_GETDEVCAPS          (DRV_RESERVED + 0x0002)
#define JDD_GETPOS              (DRV_RESERVED + 0x0101)
#define JDD_SETCALIBRATION      (DRV_RESERVED + 0x0102)
#define JDD_CONFIGCHANGED       (DRV_RESERVED + 0x0103)
#define JDD_GETPOSEX            (DRV_RESERVED + 0x0104)

#define MCI_MAX_DEVICE_TYPE_LENGTH 80

#define MCI_FALSE                       (MCI_STRING_OFFSET + 19)
#define MCI_TRUE                        (MCI_STRING_OFFSET + 20)

#define MCI_FORMAT_RETURN_BASE          MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S       (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S                (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S                (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S             (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S           (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S           (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S           (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S       (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S              (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S            (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S               (MCI_STRING_OFFSET + 31)

#define MCI_VD_FORMAT_TRACK_S           (MCI_VD_OFFSET + 5)

#define WAVE_FORMAT_PCM_S               (MCI_WAVE_OFFSET + 0)
#define WAVE_MAPPER_S                   (MCI_WAVE_OFFSET + 1)

#define MCI_SEQ_MAPPER_S                (MCI_SEQ_OFFSET + 5)
#define MCI_SEQ_FILE_S                  (MCI_SEQ_OFFSET + 6)
#define MCI_SEQ_MIDI_S                  (MCI_SEQ_OFFSET + 7)
#define MCI_SEQ_SMPTE_S                 (MCI_SEQ_OFFSET + 8)
#define MCI_SEQ_FORMAT_SONGPTR_S        (MCI_SEQ_OFFSET + 9)
#define MCI_SEQ_NONE_S                  (MCI_SEQ_OFFSET + 10)
#define MIDIMAPPER_S                    (MCI_SEQ_OFFSET + 11)

#define MCI_RESOURCE_RETURNED       0x00010000  /* resource ID */
#define MCI_COLONIZED3_RETURN       0x00020000  /* colonized ID, 3 bytes data */
#define MCI_COLONIZED4_RETURN       0x00040000  /* colonized ID, 4 bytes data */
#define MCI_INTEGER_RETURNED        0x00080000  /* integer conversion needed */
#define MCI_RESOURCE_DRIVER         0x00100000  /* driver owns returned resource */

#define MCI_NO_COMMAND_TABLE    0xFFFF

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

#define MAKEMCIRESOURCE(wRet, wRes) MAKELRESULT((wRet), (wRes))

typedef struct {
	DWORD   		dwCallback;
	DWORD   		dwInstance;
	HMIDIOUT		hMidi;
	DWORD   		dwFlags;
} PORTALLOC, *LPPORTALLOC;

typedef struct {
	HWAVE			hWave;
	LPWAVEFORMATEX		lpFormat;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT			uMappedDeviceID;
        DWORD			dnDevNode;
} WAVEOPENDESC, *LPWAVEOPENDESC;

typedef struct {
        DWORD  			dwStreamID;
        WORD   			wDeviceID;
} MIDIOPENSTRMID;

typedef struct {
	HMIDI			hMidi;
	DWORD			dwCallback;
	DWORD			dwInstance;
        DWORD          		dnDevNode;
        DWORD          		cIds;
        MIDIOPENSTRMID 		rgIds;
} MIDIOPENDESC, *LPMIDIOPENDESC;

typedef struct tMIXEROPENDESC
{
	HMIXEROBJ		hmx;
        LPVOID			pReserved0;
	DWORD			dwCallback;
	DWORD			dwInstance;
} MIXEROPENDESC, *LPMIXEROPENDESC;

typedef struct {
	UINT			wDeviceID;		/* device ID */
	LPSTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
	UINT			wCustomCommandTable;	/* custom command table (0xFFFF if none) * filled in by the driver */
	UINT			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMSA, *LPMCI_OPEN_DRIVER_PARMSA;

typedef struct {
	UINT			wDeviceID;		/* device ID */
	LPWSTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
	UINT			wCustomCommandTable;	/* custom command table (0xFFFF if none) * filled in by the driver */
	UINT			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMSW, *LPMCI_OPEN_DRIVER_PARMSW;

DWORD 			WINAPI	mciGetDriverData(UINT uDeviceID);
BOOL			WINAPI	mciSetDriverData(UINT uDeviceID, DWORD dwData);
UINT			WINAPI	mciDriverYield(UINT uDeviceID);
BOOL			WINAPI	mciDriverNotify(HWND hwndCallback, UINT uDeviceID,
						UINT uStatus);
UINT			WINAPI	mciLoadCommandResource(HINSTANCE hInstance,
					       LPCWSTR lpResName, UINT uType);
BOOL			WINAPI	mciFreeCommandResource(UINT uTable);

#define DCB_NULL		0x0000
#define DCB_WINDOW		0x0001			/* dwCallback is a HWND */
#define DCB_TASK		0x0002			/* dwCallback is a HTASK */
#define DCB_FUNCTION		0x0003			/* dwCallback is a FARPROC */
#define DCB_EVENT		0x0005			/* dwCallback is an EVENT Handler */
#define DCB_TYPEMASK		0x0007
#define DCB_NOSWITCH		0x0008			/* don't switch stacks for callback */

BOOL		 	WINAPI	DriverCallback(DWORD dwCallBack, UINT uFlags, HDRVR hDev,
					       UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

typedef void (*LPTASKCALLBACK)(DWORD dwInst);

#define TASKERR_NOTASKSUPPORT 1
#define TASKERR_OUTOFMEMORY   2
MMRESULT WINAPI mmTaskCreate(LPTASKCALLBACK, HANDLE*, DWORD);
void     WINAPI mmTaskBlock(HANDLE);
BOOL     WINAPI mmTaskSignal(HANDLE);
void     WINAPI mmTaskYield(void);
HANDLE   WINAPI mmGetCurrentTask(void);


#define  WAVE_DIRECTSOUND               0x0080


#include <poppack.h>

#endif /* __MMDDK_H */
