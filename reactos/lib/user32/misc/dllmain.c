#include <windows.h>

extern HWINSTA WinStation;
extern HDESK Desktop;

WINBOOL STDCALL DllMain (HANDLE hInst, 
			 ULONG ul_reason_for_call,
			 LPVOID lpReserved);



BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
   return(DllMain(hDll,dwReason,lpReserved));
}

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
			WINPOS_CreateInternalPosAtom();
			SYSCOLOR_Init();
        		WIDGETS_Init();
			ICONTITLE_Init();
			DIALOG_Init();
			COMBO_Init();
			MENU_Init();
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

