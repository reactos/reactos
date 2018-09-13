/*	File: D:\wacker\ext\fspage.c (Created: 01-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#define _INC_OLE		// WIN32, get ole2 from windows.h
#define CONST_VTABLE

#include <windows.h>
#pragma hdrstop

#include <windowsx.h>
//#include <shell2.h>
#include <shlobj.h>

#include <tdll\stdtyp.h>
#include <tdll\globals.h>
#include <tdll\tdll.h>
#include <term\res.h>
#include <tdll\assert.h>
#include <tdll\mc.h>

#include "pageext.hh"

#include <tdll\session.h>
#include <tdll\sf.h>
#include <tdll\property.h>
#include <cncttapi\cncttapi.h>

//
// Function prototype
//
UINT CALLBACK FSPage_ReleasePage(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE psp);

//---------------------------------------------------------------------------
//
// FSPage_AddPages
//
//  This function is called from CSamplePageExt::AddPages(). It add a page
// if the data object contains file system objects.
//
//---------------------------------------------------------------------------
void FSPage_AddPages(LPDATAOBJECT pdtobj,
		     LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
	{
    //
    // Call IDataObject::GetData asking for a CF_HDROP (i.e., HDROP).
    //
    FORMATETC fmte = {
        	CF_HDROP,
        	(DVTARGETDEVICE FAR *)NULL,
			//DVASPECT_SHORTNAME,
			DVASPECT_CONTENT,
        	-1,
			TYMED_HGLOBAL};
    STGMEDIUM medium;
    HRESULT hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium);

    if (SUCCEEDED(hres))
		{
		//
		//	We need to make a copy of hdrop, because we can't hang on
		// to this medium.
		//
		UINT cbDrop = (UINT)GlobalSize(medium.hGlobal);
		HDROP hdrop = GlobalAlloc(GPTR, cbDrop);
		HSESSION hSession;

		hSession = CreateSessionHandle(NULL);

		if ((hdrop != NULL) && (hSession != NULL))
			{
			PROPSHEETPAGE psp;
			HPROPSHEETPAGE hpage;
			char szFile[MAX_PATH];

   			if (cbDrop)
                MemCopy((LPSTR)hdrop, GlobalLock(medium.hGlobal), cbDrop);

            GlobalUnlock(medium.hGlobal);

			/*
			 * We need to get a session handle that we can pass along to
			 * the property sheet dialogs.  This may take a little bit of
			 * work because the routines to create a session handle usually
			 * expect that there will be a session window.  Not in this case.
			 */

			DragQueryFile(hdrop, 0, szFile, sizeof(szFile));
			InitializeSessionHandle(hSession, NULL, NULL);

			sfOpenSessionFile(sessQuerySysFileHdl(hSession), szFile);
			sessLoadSessionStuff(hSession);

			//
			//	Create a property sheet page object from a dialog box.
			//
			//	We store the hdrop (a copy of medium.hGlobal) in lParam,
			// because it is the only instance data we need.
			//
			//	If the page needs more instance data, you can append
			// arbitrary size of data at the end of this structure,
			// and pass it to the CreatePropSheetPage. In such a case,
			// the size of entire data structure (including page specific
			// data) must be stored in the dwSize field.
			//
			psp.dwSize		= sizeof(psp);	// no extra data.
			psp.dwFlags 	= PSP_USEREFPARENT | PSP_USECALLBACK;
			psp.hInstance	= glblQueryDllHinst();
			psp.pszTemplate = MAKEINTRESOURCE(IDD_TAB_PHONENUMBER);
			psp.pfnDlgProc	= NewPhoneDlg;
			psp.pcRefParent = &g_cRefThisDll;
			psp.pfnCallback = FSPage_ReleasePage;
			psp.lParam		= (LPARAM)hSession;

			hpage = CreatePropertySheetPage(&psp);

			if (hpage)
				{
				if (!lpfnAddPage(hpage, lParam))
					DestroyPropertySheetPage(hpage);
				}

			// Do the terminal page now

			psp.dwSize		= sizeof(psp);	// no extra data.
			psp.dwFlags 	= PSP_USEREFPARENT;
			psp.hInstance	= glblQueryDllHinst();
			psp.pszTemplate = MAKEINTRESOURCE(IDD_TAB_TERMINAL);
			psp.pfnDlgProc	= TerminalTabDlg;
			psp.pcRefParent = &g_cRefThisDll;
			psp.pfnCallback = 0;
			psp.lParam		= (LPARAM)hSession;

			hpage = CreatePropertySheetPage(&psp);

			if (hpage)
				{
				if (!lpfnAddPage(hpage, lParam))
					DestroyPropertySheetPage(hpage);
				}

            /* Make sure we free this up here */
			GlobalFree(hdrop);
			}


		//
		// HACK: We are supposed to call ReleaseStgMedium. This is a temporary
		//	hack until OLE 2.01 for Chicago is released.
		//
		if (medium.pUnkForRelease)
			{
			medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
			}
		else
			{
			GlobalFree(medium.hGlobal);
			}
		}
	}

//
//
//
UINT CALLBACK FSPage_ReleasePage(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE psp)
	{
	HSESSION hSession;
	SF_HANDLE  hsf;

	switch (uMsg)
		{
		case PSPCB_RELEASE:
			hSession = (HSESSION)psp->lParam;
			hsf = sessQuerySysFileHdl(hSession);

			sessSaveSessionStuff(hSession);
			sfFlushSessionFile(hsf);
			DestroySessionHandle(hSession);
			hSession = NULL;

			break;
		}

	return TRUE;
	}
