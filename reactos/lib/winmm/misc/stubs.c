/* 
 * The stubs here are totaly wrong so please help a brother out 
 * and fix this shit. sedwards 9-24-02
 *
 * Added more stubs for bochs 1.3 once again still mostly wrong
 * but bochs gets further now. 12-14-02
 *
 */

#include <windows.h>

DECLARE_HANDLE(HWAVEOUT); // mmsystem.h

#define NEAR
#define FAR

typedef UINT 	MMRESULT; // error returncode, 0 means no error
typedef UINT    MCIDEVICEID;    /* MCI device ID type */
typedef DWORD	MCIERROR;

/* general constants */
#define MAXPNAMELEN      32     /* max product name length (including NULL) */
#define MAXERRORLENGTH   256    /* max error text length (including NULL) */
#define MAX_JOYSTICKOEMVXDNAME 260 /* max oem vxd name length (including NULL) */

#define MMSYSERR_BASE	0

#define MMSYSERR_NOERROR	0
#define MMSYSERR_ERROR	(MMSYSERR_BASE + 1)

/* joystick device capabilities data structure */
typedef struct tagJOYCAPSA {
    WORD    wMid;                /* manufacturer ID */
    WORD    wPid;                /* product ID */
    CHAR    szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    UINT    wXmin;               /* minimum x position value */
    UINT    wXmax;               /* maximum x position value */
    UINT    wYmin;               /* minimum y position value */
    UINT    wYmax;               /* maximum y position value */
    UINT    wZmin;               /* minimum z position value */
    UINT    wZmax;               /* maximum z position value */
    UINT    wNumButtons;         /* number of buttons */
    UINT    wPeriodMin;          /* minimum message period when captured */
    UINT    wPeriodMax;          /* maximum message period when captured */
    UINT    wRmin;               /* minimum r position value */
    UINT    wRmax;               /* maximum r position value */
    UINT    wUmin;               /* minimum u (5th axis) position value */
    UINT    wUmax;               /* maximum u (5th axis) position value */
    UINT    wVmin;               /* minimum v (6th axis) position value */
    UINT    wVmax;               /* maximum v (6th axis) position value */
    UINT    wCaps;	 	 /* joystick capabilites */
    UINT    wMaxAxes;	 	 /* maximum number of axes supported */
    UINT    wNumAxes;	 	 /* number of axes in use */
    UINT    wMaxButtons;	 /* maximum number of buttons supported */
    CHAR    szRegKey[MAXPNAMELEN];/* registry key */
    CHAR    szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
} JOYCAPSA, *PJOYCAPSA, *NPJOYCAPSA, *LPJOYCAPSA;

typedef struct joyinfoex_tag {
    DWORD dwSize;		 /* size of structure */
    DWORD dwFlags;		 /* flags to indicate what to return */
    DWORD dwXpos;                /* x position */
    DWORD dwYpos;                /* y position */
    DWORD dwZpos;                /* z position */
    DWORD dwRpos;		 /* rudder/4th axis position */
    DWORD dwUpos;		 /* 5th axis position */
    DWORD dwVpos;		 /* 6th axis position */
    DWORD dwButtons;             /* button states */
    DWORD dwButtonNumber;        /* current button number pressed */
    DWORD dwPOV;                 /* point of view state */
    DWORD dwReserved1;		 /* reserved for communication between winmm & driver */
    DWORD dwReserved2;		 /* reserved for future expansion */
} JOYINFOEX, *PJOYINFOEX, NEAR *NPJOYINFOEX, FAR *LPJOYINFOEX;


// mmsystem.h ends here

UINT 
WINAPI 
waveOutReset(HWAVEOUT hWaveOut)
{
	DbgPrint("waveOutReset stub\n");
	return 1;
}


UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("waveOutWrite stub\n");
	return 1;
}


WINBOOL 
STDCALL
sndPlaySoundA(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("sndPlaySoundA stub\n");
	return 1;
}

WINBOOL 
STDCALL
sndPlaySoundW(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("sndPlaySoundW stub\n");
	return 1;
}

WINBOOL 
STDCALL
midiOutReset(HWAVEOUT hWaveOut)
{
	DbgPrint("midiOutReset stub\n");
	return 1;
}

WINBOOL 
STDCALL
waveOutPrepareHeader(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("waveOutPrepareHeader stub\n");
	return 1;
}

WINBOOL 
STDCALL
waveOutGetErrorTextA(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("waveOutGetErrorTextA stub\n");
	return 1;
}

WINBOOL 
STDCALL
waveOutOpen(HWAVEOUT* lphWaveOut, UINT uDeviceID,
			const lpFormat, DWORD dwCallback,
			DWORD dwInstance, DWORD dwFlags)
{
	DbgPrint("waveOutOpen stub\n");
	return 1;
}

UINT 
WINAPI 
waveOutClose(HWAVEOUT hWaveOut)
{
	DbgPrint("waveOutClose stub\n");
	return 1;
}

WINBOOL 
STDCALL
midiOutClose(HWAVEOUT hWaveOut)
{
	DbgPrint("midiOutClose stub\n");
	return 1;
}

WINBOOL 
STDCALL
midiOutUnprepareHeader(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("midiOutUnprepareHeader stub\n");
	return 1;
}

WINBOOL 
STDCALL
waveOutUnprepareHeader(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("waveOutUnprepareHeader stub\n");
	return 1;
}


WINBOOL 
STDCALL
midiOutPrepareHeader(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("midiOutPrepareHeader stub\n");
	return 1;
}

WINBOOL 
STDCALL
midiOutLongMsg(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("midiOutLongMsg stub\n");
	return 1;
}

UINT/*MMRESULT*/
STDCALL
timeBeginPeriod(UINT uPeriod)
{
	DbgPrint("timeBeginPeriod stub\n");
	return 97/*TIMERR_NOCANDO*/;
}

DWORD
STDCALL
timeGetTime(VOID)
{
	DbgPrint("timeGetTime stub\n");
	return 0;
}

UINT/*MMRESULT*/
STDCALL
timeEndPeriod(UINT uPeriod)
{
	DbgPrint("timeEndPeriod stub\n");
	return 97/*TIMERR_NOCANDO*/;
}

MMRESULT WINAPI joyGetDevCapsA(UINT uJoyID, LPJOYCAPSA pjc, UINT cbjc)
{
	DbgPrint("joyGetDevCapsA stub\n");
	return MMSYSERR_ERROR;
}

UINT WINAPI joyGetNumDevs(void)
{
	DbgPrint("joyGetNumDevs stub\n");
	return 0;
}

MMRESULT WINAPI joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji)
{
	DbgPrint("joyGetPosEx stub\n");
	return MMSYSERR_ERROR;
}

MCIERROR WINAPI mciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
	DbgPrint("mciSendCommandA stub\n");
	return MMSYSERR_ERROR;
}
