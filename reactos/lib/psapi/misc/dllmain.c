/* $Id: dllmain.c,v 1.2 2002/06/18 22:15:58 hyperion Exp $
 * 
 * ReactOS PSAPI.DLL
 */
#include <windows.h>

BOOLEAN STDCALL DllMain
(
 PVOID hinstDll,
 ULONG dwReason,
 PVOID reserved
)
{
 return (TRUE);
}
/* EOF */
