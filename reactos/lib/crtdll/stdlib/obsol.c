#include <windows.h>
#include <stdlib.h>

void _seterrormode(int nMode)
{
	SetErrorMode(nMode);
	return;
}

void _beep(unsigned nFreq, unsigned dur)
{
	Beep(nFreq,nDur);
	return;
}

void _sleep(unsigned long ulTime)
{
	Sleep(ulTime);
	return;
}