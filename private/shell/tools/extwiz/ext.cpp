// Extensions.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>
#include "Ext.h"
#include "Extaw.h"

#ifdef _PSEUDO_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE ExtensionsDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("EXTENSIONS.AWX Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(ExtensionsDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(ExtensionsDLL);

		// Register this custom AppWizard with MFCAPWZ.DLL
		SetCustomAppWizClass(&Extensionsaw);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("EXTENSIONS.AWX Terminating!\n");

		// Terminate the library before destructors are called
		AfxTermExtensionModule(ExtensionsDLL);
	}
	return 1;   // ok
}
