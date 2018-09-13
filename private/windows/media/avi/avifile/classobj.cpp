
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993 - 1995. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#define INITGUID
#include <win32.h>
#include <vfw.h>

#include "avifilei.h"
#include "avicmprs.h"
#include "avifps.h"
#include "editstrm.h"
#include "wavefile.h"

#include "debug.h"

#ifndef _WIN32
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
DEFINE_OLEGUID(IID_IMalloc,             0x00000002L, 0, 0);
DEFINE_OLEGUID(IID_IMarshal,            0x00000003L, 0, 0);
#endif

extern "C"	HINSTANCE	ghMod;
		HINSTANCE	ghMod;

/*      -       -       -       -       -       -       -       -       */

EXTERN_C int CALLBACK LibMain(
        HINSTANCE       hInstance,
        UINT            uDataSeg,
        UINT            cbHeapSize,
        LPCTSTR          pszCmdLine)
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
	    rclsid == CLSID_ACMCmprs ||
#ifdef CHICAGO
	    rclsid == CLSID_AVISimpleUnMarshal ||
#endif
	    rclsid == CLSID_AVIWaveFileReader ||
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

#ifdef _WIN32

EXTERN_C BOOL WINAPI DLLEntryPoint(HINSTANCE hModule, ULONG Reason, LPVOID pv)
{
    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            LibMain(hModule, 0, 0, NULL);
            DisableThreadLibraryCalls(hModule);
            break;

        case DLL_PROCESS_DETACH:
            WEP(FALSE);
            break;

        //case DLL_THREAD_DETACH:
        //    break;

        //case DLL_THREAD_ATTACH:
        //    break;
    }

    return TRUE;
}

#endif
