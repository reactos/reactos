#include <windows.h>
#include <msvcrt/stdlib.h>

#undef _cpumode
unsigned char _cpumode = 0;
unsigned char *_cpumode_dll = &_cpumode;

void _seterrormode(int nMode)
{
	SetErrorMode(nMode);
	return;
}

void _beep(unsigned nFreq, unsigned nDur)
{
	Beep(nFreq,nDur);
	return;
}

void _sleep(unsigned long ulTime)
{
	Sleep(ulTime);
	return;
}
