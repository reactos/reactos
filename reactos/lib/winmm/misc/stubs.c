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
