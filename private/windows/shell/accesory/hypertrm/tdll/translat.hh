/*	File: D:\WACKER\translat.hh (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

struct stTransInternal
	{
	HSESSION hSession;
	HINSTANCE hInstance;			/* Returned from LoadLibrary */
	};

typedef	struct stTransInternal ST_TRANS_INT;
typedef	ST_TRANS_INT *PST_TRANS_INT;

// For now we'll include the .HH file where we need access to the
// structure.  Later, we'll go back and write wrappers for the
// few places this needs to referenced.  Also, the session handle
// should probably not own the translate handle.  It may be more
// appropriate to think of this as function of the cloop - mrw, 3/1/95
//
struct stTranslate
	{
	int   (*pfnIsDeviceLoaded)(VOID *pV);
	int   (*pfnDoDialog)(HWND hWnd, VOID *pV);
	VOID *(*pfnCreate)(HSESSION hSession);	// why is this void *? - mrw
	int   (*pfnInit)(VOID *pV);
	int   (*pfnLoad)(VOID *pV);
	int   (*pfnSave)(VOID *pV);
	int   (*pfnDestroy)(VOID *pV);
	int   (*pfnIn)(VOID *pV, TCHAR cC, int *nReady, int nSize, TCHAR *pszC);
	int   (*pfnOut)(VOID *pV, TCHAR cC, int *nReady, int nSize, TCHAR *pszC);

	VOID *pDllHandle;			/* the handle from the DLL */
	VOID *pInternal;			/* internal handle used internally */
	};

typedef	struct stTranslate	ST_TRANSLATE;
typedef ST_TRANSLATE *HHTRANSLATE;

// These functions as well as the function pointers above take VOID *
// pointers.  I'ld like to see them point to some incomplete type
// at the very least for typechecking purposes. - mrw 3/1/95
//
static int translateInternalFalse(VOID *pV);
static int translateInternalTrue(VOID *pV);
static VOID *translateInternalVoid(HSESSION hSession);
static int translateInternalDoDlg(HWND hWnd, VOID *pV);
static int translateInternalCio(VOID *pV, TCHAR cC, int *nR, int nS, TCHAR *pC);
