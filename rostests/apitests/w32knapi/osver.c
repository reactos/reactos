#include "w32knapi.h"

OSVERSIONINFOW g_OsVer;
UINT g_OsIdx;

ASPI gNOPARAM_ROUTINE_CREATEMENU = {-1,-1,0x00,-1,0x00};
ASPI gNOPARAM_ROUTINE_CREATEMENUPOPUP = {-1,-1,0x01,-1,0x01};
ASPI gNOPARAM_ROUTINE_LOADUSERAPIHOOK = {-1,-1,0x1d,-1,0x0e};
ASPI gONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT = {-1, -1, 0x21, 0x21, 0x25};
ASPI gONEPARAM_ROUTINE_MAPDEKTOPOBJECT = {-1,-1,0x30,-1,0x31};
ASPI gONEPARAM_ROUTINE_SWAPMOUSEBUTTON = {-1,-1,0x42,-1,0x44};

ASPI gHWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW = {-1,-1,0x45,-1,0x46};
ASPI gHWND_ROUTINE_GETWNDCONTEXTHLPID = {-1,-1,0x47,-1,0x48};
ASPI gHWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID = {-1,-1,0x51,-1,0x52};

BOOL InitOsVersion()
{
	g_OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	GetVersionExW((LPOSVERSIONINFOW)&g_OsVer);
	if (g_OsVer.dwMajorVersion == 4)
	{
		g_OsIdx = 0;
		return TRUE;
	}
	else if (g_OsVer.dwMajorVersion == 5)
	{
		if (g_OsVer.dwMinorVersion == 0)
		{
			g_OsIdx = 1;
			return TRUE;
		}
		else if (g_OsVer.dwMinorVersion == 1)
		{
			g_OsIdx = 2;
			return TRUE;
		}
		else if (g_OsVer.dwMinorVersion == 2)
		{
			g_OsIdx = 3;
			return TRUE;
		}
	}
	else if (g_OsVer.dwMajorVersion == 6)
	{
		g_OsIdx = 4;
		return TRUE;
	}
	return FALSE;
}
