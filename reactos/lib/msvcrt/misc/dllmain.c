/* $Id: dllmain.c,v 1.2 2000/12/03 17:57:25 ekohl Exp $
 * 
 * ReactOS MSVCRT.DLL Compatibility Library
 */
#include <windows.h>

#include <msvcrt/internal/tls.h>


static int nAttachCount = 0;

unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;

/* FUNCTIONS **************************************************************/

BOOLEAN
__stdcall
DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			/* initialize version info */
			_osver = GetVersion();
			_winmajor = (_osver >> 8) & 0xFF;
			_winminor = _osver & 0xFF;
			_winver = (_winmajor << 8) + _winminor;
			_osver = (_osver >> 16) & 0xFFFF;

			/* create tls stuff */
			if (!CreateThreadData())
				return FALSE;

			/* FIXME: more initializations... */

			nAttachCount++;
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			FreeThreadData(NULL);
			break;

		case DLL_PROCESS_DETACH:
			if (nAttachCount > 0)
			{
				nAttachCount--;

				/* FIXME: more cleanup... */


				/* destroy tls stuff */
				DestroyThreadData();
			}
			break;
	}

	return TRUE;
}

/* EOF */
