/*
 *  OLE32 Initialization
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "ole32_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE OLE32_hInstance = 0;

/***********************************************************************
 *		OleMetafilePictFromIconAndLabel (OLE32.@)
 */
HGLOBAL WINAPI OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel,
                                               LPOLESTR lpszSourceFile, UINT iIconIndex)
{
	METAFILEPICT mfp;
	HDC hdc;
	UINT dy;
	HGLOBAL hmem = NULL;
	LPVOID mfdata;
	static const char szIconOnly[] = "IconOnly";

	TRACE("%p %p %s %d\n", hIcon, lpszLabel, debugstr_w(lpszSourceFile), iIconIndex);

	if( !hIcon )
		return NULL;

	hdc = CreateMetaFileW(NULL);
	if( !hdc )
		return NULL;

	ExtEscape(hdc, MFCOMMENT, sizeof(szIconOnly), szIconOnly, 0, NULL);

	/* FIXME: things are drawn in the wrong place */
	DrawIcon(hdc, 0, 0, hIcon);
	dy = GetSystemMetrics(SM_CXICON);
	if(lpszLabel)
		TextOutW(hdc, 0, dy, lpszLabel, lstrlenW(lpszLabel));

	if (lpszSourceFile)
	{
		char szIconIndex[10];
		int path_length = WideCharToMultiByte(CP_ACP,0,lpszSourceFile,-1,NULL,0,NULL,NULL);
		if (path_length > 1)
		{
			char * szPath = CoTaskMemAlloc(path_length * sizeof(CHAR));
			if (szPath)
			{
				WideCharToMultiByte(CP_ACP,0,lpszSourceFile,-1,szPath,path_length,NULL,NULL);
				ExtEscape(hdc, MFCOMMENT, path_length, szPath, 0, NULL);
				CoTaskMemFree(szPath);
			}
		}
		snprintf(szIconIndex, 10, "%u", iIconIndex);
		ExtEscape(hdc, MFCOMMENT, strlen(szIconIndex)+1, szIconIndex, 0, NULL);
	}

	mfp.mm = MM_ISOTROPIC;
	mfp.xExt = mfp.yExt = 0; /* FIXME ? */
	mfp.hMF = CloseMetaFile(hdc);
	if( !mfp.hMF )
		return NULL;

	hmem = GlobalAlloc( GMEM_MOVEABLE, sizeof(mfp) );
	if( !hmem )
	{
		DeleteMetaFile(mfp.hMF);
		return NULL;
	}

	mfdata = GlobalLock( hmem );
	if( !mfdata )
	{
		GlobalFree( hmem );
		DeleteMetaFile(mfp.hMF);
		return NULL;
	}

	memcpy(mfdata,&mfp,sizeof(mfp));
	GlobalUnlock( hmem );

	TRACE("returning %p\n",hmem);

	return hmem;
}


/***********************************************************************
 *		DllMain (OLE32.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        OLE32_hInstance = hinstDLL;
        COMPOBJ_InitProcess();
	if (TRACE_ON(ole)) CoRegisterMallocSpy((LPVOID)-1);
	break;

    case DLL_PROCESS_DETACH:
        if (TRACE_ON(ole)) CoRevokeMallocSpy();
        COMPOBJ_UninitProcess();
        OLE32_hInstance = 0;
	break;
    }
    return TRUE;
}

/* NOTE: DllRegisterServer and DllUnregisterServer are in regsvr.c */
