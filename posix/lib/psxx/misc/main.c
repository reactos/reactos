/* $Id: main.c,v 1.1 2001/09/08 22:13:16 ea Exp $
 */
#define NTOS_MODE_USER
#include <ntos.h>

BOOL STDCALL DllMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
/* EOF */
