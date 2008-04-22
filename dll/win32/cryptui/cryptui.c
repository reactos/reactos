/* $Id: dllmain.c 21873 2006-05-10 08:41:27Z cwittich $
*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS system libraries
* FILE:            dll/win32/crypui/cryptui.c
* PURPOSE:         Library main function
* PROGRAMMER:      Christoph von Wittich
* UPDATE HISTORY:
*
*/

#include <windows.h>
#include <cryptuiapi.h>

#define NDEBUG
#include <debug.h>

INT STDCALL
DllMain(PVOID hinstDll,
		ULONG dwReason,
		PVOID reserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

BOOL
WINAPI
CryptUIDlgCertMgr(PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr)
{
	UNIMPLEMENTED
	return FALSE;
}


/* EOF */
