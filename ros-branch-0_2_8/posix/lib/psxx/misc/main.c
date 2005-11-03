/* $Id: main.c,v 1.3 2002/10/29 04:45:50 rex Exp $
 */
#define NTOS_MODE_USER
#include <ntos.h>

BOOL STDCALL DllMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
/* EOF */
