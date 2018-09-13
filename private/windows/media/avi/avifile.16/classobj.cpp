
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#include <win32.h>
#include <shellapi.h>

#include <compobj.h>

#define INITGUID        // make DECLARE_GUILD()
#include <initguid.h>

#ifdef WIN32
//
// !!!for some reason we can't link to GUID's in Win32!!!
//
#include <coguid.h>
#endif

#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "avicmprs.h"
#include "avifps.h"
#include "editstrm.h"

#include "debug.h"

extern "C"	HINSTANCE	ghMod;
		HINSTANCE	ghMod;

/*      -       -       -       -       -       -       -       -       */

EXTERN_C int CALLBACK LibMain(
        HINSTANCE       hInstance,
        UINT            uDataSeg,
        UINT            cbHeapSize,
        LPCSTR          pszCmdLine)
{
	// save our module handle
	ghMod = hInstance;
	return TRUE;
}

EXTERN_C int CALLBACK WEP(BOOL fSystemExit)
{
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI DllGetClassObject(
	const CLSID FAR&	rclsid,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	HRESULT	hresult;

	DPF("DllGetClassObject\n");

	if (rclsid == CLSID_AVIFile ||
#if 0
	    rclsid == CLSID_AVISimpleUnMarshal ||
#endif
	    rclsid == CLSID_AVICmprsStream) {
	    hresult = CAVIFileCF::Create(rclsid, riid, ppv);
	    return hresult;
	} else if (rclsid == CLSID_AVIStreamPS) {
	    return (*ppv = (LPVOID)new CPSFactory()) != NULL
		? NOERROR : ResultFromScode(E_OUTOFMEMORY);
	} else {
	    return ResultFromScode(E_UNEXPECTED);
	}
}

/*      -       -       -       -       -       -       -       -       */

#ifdef WIN32

EXTERN_C BOOL WINAPI DLLEntryPoint(HINSTANCE hModule, ULONG Reason, LPVOID pv)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            LibMain(hModule, 0, 0, NULL);
            break;

        case DLL_PROCESS_DETACH:
            WEP(FALSE);
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;
    }

    return TRUE;
}

#endif
