/* 
  The stubs here are totaly wrong so please help a brother out 
  and fix this shit. sedwards 9-24-02
*/

#include <windows.h>

DECLARE_HANDLE(HWAVEOUT); // mmsystem.h

UINT 
WINAPI 
waveOutReset(HWAVEOUT hWaveOut)
{
	DbgPrint("waveOutReset unimplemented\n");
	return 1;
}

WINBOOL 
STDCALL
sndPlaySoundA(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("waveOutReset unimplemented\n");
	return 1;
}

WINBOOL 
STDCALL
sndPlaySoundW(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("waveOutReset unimplemented\n");
	return 1;
}
