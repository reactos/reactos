

#include "../../dll/doscalls/doscalls.h"

void Eingang()
{
	DosBeep(3000,300);
	DosSleep(1000);
	DosBeep(4000,200);
	DosExit(0,0);
}

void WinMainCRTStartup()
{
	int a, b;
	a= b+3;
	b=a+3;
	Eingang();
}