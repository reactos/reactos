/*
 * pidl - PIDLs and diddles
 *
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  pidlFromPath
 *
 *	Create a pidl from an psf and a relative path.
 *
 *****************************************************************************/

PIDL PASCAL
pidlFromPath(LPSHELLFOLDER psf, LPCTSTR lqn)
{
    PIDL pidl;
    UnicodeFromPtsz(wsz, lqn);
    if (SUCCEEDED(psf->lpVtbl->ParseDisplayName(psf, 0, 0, wsz, 0, &pidl, 0))) {
	return pidl;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  SetNameOfPidl
 *
 *	Change a pidl's name.
 *
 *****************************************************************************/

HRESULT PASCAL
SetNameOfPidl(PSF psf, PIDL pidl, LPCTSTR ptszName)
{
    UnicodeFromPtsz(wsz, ptszName);
    return psf->lpVtbl->SetNameOf(psf, 0, pidl, wsz, 0, 0);
}

/*****************************************************************************
 *
 *  ComparePidls
 *
 *	Compare two pidls.
 *
 *****************************************************************************/

HRESULT PASCAL
ComparePidls(PIDL pidl1, PIDL pidl2)
{
    return psfDesktop->lpVtbl->CompareIDs(psfDesktop, 0, pidl1, pidl2);
}

/*****************************************************************************
 *
 *  GetSystemImageList
 *
 *	Get the large or small image list handle.
 *
 *	The dword argument is 0 for the large image list, or
 *	SHGFI_SMALLICON for the small image list.
 *
 *****************************************************************************/

HIML PASCAL
GetSystemImageList(DWORD dw)
{
    SHFILEINFO sfi;
    return (HIML)SHGetFileInfo(g_tszPathShell32, FILE_ATTRIBUTE_DIRECTORY,
			       &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES |
			       SHGFI_SYSICONINDEX | dw);
}

