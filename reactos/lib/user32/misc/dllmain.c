#include <windows.h>

INT
STDCALL
DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			//WinStation = CreateWindowStationA(NULL,0,GENERIC_ALL,NULL);
			//Desktop = CreateDesktopA(NULL,NULL,NULL,0,0,NULL);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(1);
}

