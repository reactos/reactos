#include <windows.h>
#include <msvcrt/stdlib.h>

#undef _cpumode
unsigned char _cpumode = 0;
unsigned char *_cpumode_dll = &_cpumode;

WINBOOL STDCALL Beep(DWORD dwFreq, DWORD dwDuration);
VOID    STDCALL Sleep(DWORD dwMilliseconds);
UINT    STDCALL SetErrorMode(UINT uMode);

/*
 * @implemented
 */
void _seterrormode(int nMode)
{
	SetErrorMode(nMode);
	return;
}

/*
 * @implemented
 */
void _beep(unsigned nFreq, unsigned nDur)
{
	Beep(nFreq,nDur);
	return;
}

/*
 * @implemented
 */
void _sleep(unsigned long ulTime)
{
	Sleep(ulTime);
	return;
}
