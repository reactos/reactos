/* 
 * The stubs here are totaly wrong so please help a brother out 
 * and fix this shit. sedwards 9-24-02
 *
 * Added more stubs for bochs 1.3 once again still mostly wrong
 * but bochs gets further now. 12-14-02
 *
 * [8-18-03] AG: I've added PlaySound/A/W and implemented sndPlaySoundA/W to
 * call these. I've also tried to match the parameter names and types with the
 * correct ones.
 *
 */

#include <windows.h>
typedef UINT *LPUINT;
#include <mmsystem.h>
#define NDEBUG
#include <debug.h>

#define NEAR
#define FAR

/* general constants */
#define MAXPNAMELEN      32     /* max product name length (including NULL) */
#define MAXERRORLENGTH   256    /* max error text length (including NULL) */
#define MAX_JOYSTICKOEMVXDNAME 260 /* max oem vxd name length (including NULL) */


// mmsystem.h ends here

MMRESULT
WINAPI 
waveOutReset(HWAVEOUT hwo)
{
    // Possible return values:
    // MMSYSERR_INVALHANDLE, MMSYSERR_NODRIVER, MMSYSERR_NOMEM, MMSYSERR_NOTSUPPORTED

	DbgPrint("waveOutReset stub\n");
    UNIMPLEMENTED;
	return 1;
}


MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh,
			 UINT cbwh)
{
    // Posible return values:
    // MMSYSERR_INVALHANDLE, MMSYSERR_NODRIVER, MMSYSERR_NOMEM, WAVERR_UNPREPARED

	DbgPrint("waveOutWrite stub\n");
    UNIMPLEMENTED;
	return 1;
}

// PlaySound() needs exporting
#undef PlaySound


BOOL WINAPI
PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    DbgPrint("PlaySoundA stub\n");
    UNIMPLEMENTED;
    return TRUE;
}


BOOL WINAPI
PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    DbgPrint("PlaySoundW stub\n");
    UNIMPLEMENTED;
    return TRUE;
}


BOOL WINAPI
PlaySound(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    // ANSI?
    return PlaySoundA(pszSound, hmod, fdwSound);
}


BOOL
WINAPI
sndPlaySoundA(LPCSTR lpszSound, UINT fuSound)
{
    fuSound &= SND_ASYNC | SND_LOOP | SND_MEMORY | SND_NODEFAULT | SND_NOSTOP | SND_SYNC;
    return PlaySoundA(lpszSound, NULL, fuSound);
}

BOOL
WINAPI
sndPlaySoundW(LPCWSTR lpszSound, UINT fuSound)
{
    fuSound &= SND_ASYNC | SND_LOOP | SND_MEMORY | SND_NODEFAULT | SND_NOSTOP | SND_SYNC;
    return PlaySoundW(lpszSound, NULL, fuSound);
}

MMRESULT 
WINAPI
midiOutReset(HMIDIOUT hmo)
{
	DbgPrint("midiOutReset stub\n");
    UNIMPLEMENTED;
	return 1;
}

MMRESULT 
WINAPI
waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh,
			 UINT cbwh)
{
	DbgPrint("waveOutPrepareHeader stub\n");
    UNIMPLEMENTED;
    pwh->dwFlags |= WHDR_PREPARED;
	return 1;
}

MMRESULT
WINAPI
waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText,
			 UINT cchText)
{
	DbgPrint("waveOutGetErrorTextA stub\n");
    UNIMPLEMENTED;
	return 1;
}

MMRESULT
WINAPI
waveOutOpen(LPHWAVEOUT pwho, UINT uDeviceID,
			LPCWAVEFORMATEX pwfx, DWORD dwCallback,
			DWORD dwCallbackInstance, DWORD fdwOpen)
{
	DbgPrint("waveOutOpen stub\n");
    UNIMPLEMENTED;
	return 1;
}

MMRESULT
WINAPI 
waveOutClose(HWAVEOUT hwo)
{
	DbgPrint("waveOutClose stub\n");
	return 1;
}

MMRESULT
WINAPI
midiOutClose(HMIDIOUT hmo)
{
	DbgPrint("midiOutClose stub\n");
	return 1;
}

MMRESULT
WINAPI
midiOutUnprepareHeader(HWAVEOUT hwo, LPMIDIHDR pwh,
			 UINT cbwh)
{
	DbgPrint("midiOutUnprepareHeader stub\n");
	return 1;
}

MMRESULT 
WINAPI
waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh,
			 UINT cbwh)
{
	DbgPrint("waveOutUnprepareHeader stub\n");
    pwh->dwFlags &= ! WHDR_PREPARED;
	return 1;
}


MMRESULT
WINAPI
midiOutPrepareHeader(HMIDIOUT hmo, LPMIDIHDR lpMidiOutHdr,
			 UINT cbMidiOutHdr)
{
	DbgPrint("midiOutPrepareHeader stub\n");
	return 1;
}

MMRESULT 
WINAPI
midiOutLongMsg(HMIDIOUT hmo, LPMIDIHDR lpMidiOutHdr,
			 UINT cbMidiOutHdr)
{
	DbgPrint("midiOutLongMsg stub\n");
	return 1;
}

DWORD
WINAPI
timeGetTime(VOID)
{
	DbgPrint("timeGetTime stub\n");
	return 0;
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
