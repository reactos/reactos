/***	DllMain - Entry point for Win32 Expression Evaluator
 *
 *
 *
 */

#include "debexpr.h"
#include <windows.h>

static HINSTANCE hInstance;
static char      szAutoName [_MAX_PATH ];

BOOL	WINAPI	DllMain (HINSTANCE hDll,
						ULONG ul_reason_for_call,
						LPVOID lpReserved)
{
	Unreferenced (lpReserved);
	switch (ul_reason_for_call) {
    	case DLL_PROCESS_ATTACH: {
        	char    szDrive[_MAX_DRIVE];
        	char    szDir  [_MAX_DIR];
        	char    szFile [_MAX_FNAME];
        	char    szExt  [_MAX_EXT];
            DWORD   cb;

            hInstance = hDll;

            cb = GetModuleFileName ( hInstance, szAutoName, _MAX_PATH );
            _tsplitpath( szAutoName, szDrive, szDir, szFile, szExt );
            _tmakepath ( szAutoName, szDrive, szDir, "autoexp", "dat" );

			DisableThreadLibraryCalls(hInstance);
    		break;
        }

	}

	return TRUE;
}


/*
 *  	EE routines that use the Win32 API
 */

/** 	LoadEEMsg - Load EE message
 *
 *		len = LoadEEMsg (wID, lpBuf, nBufMax);
 *
 *		Entry	wID = integer identifier of message string to be loaded
 *				lpbuf = pointer to buffer to receive string
 *				nBufMax = max number of characters to be copied
 *
 *		Exit	string resource copied into lpBuf
 *
 *		Returns number of characters copied into the buffer,
 *				not including the null-terminating character
 */

int	PASCAL	LoadEEMsg (uint wID, char FAR *lpBuf, int nBufMax)
{
	return LoadString(hInstance, wID, lpBuf, nBufMax);
}


/** 	LoadAERule - Load Auto-Expansion rule
 *
 *		flag = LoadAERule (lszKey, buf, cchbuf)
 *
 *		Entry	lszKey = null terminated string containing class name
 *					to be used as a key for the rule lookup
 *				buf = buffer to hold auto-expansion rule string
 *				cchbuf = maxumum buffer size
 *
 *		Exit	auto-expand rule copied into buf
 *
 *		Returns TRUE if rule was found and loaded
 *				FALSE otherwise
 */

bool_t PASCAL LoadAERule (LSZ lszKey, LSZ buf, uint cchbuf)
{
	bool_t  retval = FALSE;

	if (lszKey) {
		GetPrivateProfileString ("AutoExpand", lszKey, "", buf, cchbuf, szAutoName );
		retval = (*buf != 0);
	}
	return retval;
}
