/* 
  The stubs here are totaly wrong so please help a brother out 
  and fix this shit. sedwards 9-24-02

  Added more stubs for bochs 1.3 once again still mostly wrong
  but bochs gets further now. 12-14-02

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


UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, LPCSTR pszSoundA,
			 UINT uSize)
{
	DbgPrint("waveOutWrite unimplemented\n");
	return 1;
}


WINBOOL 
STDCALL
sndPlaySoundA(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("sndPlaySoundA unimplemented\n");
	return 1;
}

WINBOOL 
STDCALL
sndPlaySoundW(LPCSTR pszSoundA, UINT uFlags)
{
	DbgPrint("sndPlaySoundW unimplemented\n");
	return 1;
}

WINBOOL 
STDCALL
midiOutReset(HWAVEOUT hWaveOut)
{
	DbgPrint("midiOutReset unimplemented\n");
	return 1;
}
