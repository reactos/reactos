
/*
 *	Shell Folder stuff
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *
 *	IShellFolder2 and related interfaces
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "pidl.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***************************************************************************
 * debughelper: print out the return address
 *  helps especially to track down unbalanced AddRef/Release
 */
#define MEM_DEBUG 0

#if MEM_DEBUG
#define _CALL_TRACE TRACE("called from: 0x%08x\n", *( ((UINT*)&iface)-1 ));
#else
#define _CALL_TRACE
#endif

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * gets a part of a string till the first backslash
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer whitch receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut)
{
    LPCWSTR pszTail = pszNext;
    DWORD dwCopy;

    TRACE ("(%s %p 0x%08lx)\n", debugstr_w (pszNext), pszOut, dwOut);

    *pszOut = 0x0000;

    if (!pszNext || !*pszNext)
	return NULL;

    while (*pszTail && (*pszTail != (WCHAR) '\\'))
	pszTail++;

    dwCopy = (WCHAR *) pszTail - (WCHAR *) pszNext + 1;
    lstrcpynW (pszOut, pszNext, (dwOut < dwCopy) ? dwOut : dwCopy);

    if (*pszTail)
	pszTail++;
    else
	pszTail = NULL;

    TRACE ("--(%s %s 0x%08lx %p)\n", debugstr_w (pszNext), debugstr_w (pszOut), dwOut, pszTail);
    return pszTail;
}

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc,
				  LPITEMIDLIST * pidlInOut, LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    LPITEMIDLIST pidlOut = NULL,
      pidlTemp = NULL;
    IShellFolder *psfChild;

    TRACE ("(%p, %p, %p, %s)\n", psf, pbc, pidlInOut ? *pidlInOut : NULL, debugstr_w (szNext));

    /* get the shellfolder for the child pidl and let it analyse further */
    hr = IShellFolder_BindToObject (psf, *pidlInOut, pbc, &IID_IShellFolder, (LPVOID *) & psfChild);

    if (SUCCEEDED(hr)) {
	hr = IShellFolder_ParseDisplayName (psfChild, hwndOwner, pbc, szNext, pEaten, &pidlOut, pdwAttributes);
	IShellFolder_Release (psfChild);

	if (SUCCEEDED(hr)) {
	    pidlTemp = ILCombine (*pidlInOut, pidlOut);

	    if (!pidlTemp)
		hr = E_OUTOFMEMORY;
	}

	if (pidlOut)
	    ILFree (pidlOut);
    }

    ILFree (*pidlInOut);
    *pidlInOut = pidlTemp;

    TRACE ("-- pidl=%p ret=0x%08lx\n", pidlInOut ? *pidlInOut : NULL, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_CoCreateInitSF
 *
 * Creates a shell folder and initializes it with a pidl via IPersistFolder.
 * This function is meant for virtual folders not backed by a file system
 * folder.
 */
HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot,
				LPCITEMIDLIST pidlChild, REFCLSID clsid, REFIID iid, LPVOID * ppvOut)
{
    HRESULT hr;

    TRACE ("%p %p\n", pidlRoot, pidlChild);

    if (SUCCEEDED ((hr = SHCoCreateInstance (NULL, clsid, NULL, iid, ppvOut)))) {
	IPersistFolder *pPF;

	if (SUCCEEDED ((hr = IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder, (LPVOID *) & pPF)))) {
	    LPITEMIDLIST pidlAbsolute;

	    pidlAbsolute = ILCombine (pidlRoot, pidlChild);
	    IPersistFolder_Initialize (pPF, pidlAbsolute);
	    IPersistFolder_Release (pPF);
	    SHFree (pidlAbsolute);

	    if (!pidlAbsolute)
		hr = E_OUTOFMEMORY;
	}
    }

    TRACE ("-- (%p) ret=0x%08lx\n", *ppvOut, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_CoCreateInitSFEx
 *
 * Creates a shell folder and initializes it with a pidl and a root folder
 * via IPersistFolder3.
 * This function is meant for virtual folders backed by a file system
 * folder.
 *
 * NOTES
 *   pathRoot can be NULL for Folders beeing a drive.
 *   In this case the absolute path is build from pidlChild (eg. C:)
 */
HRESULT SHELL32_CoCreateInitSFEx (LPCITEMIDLIST pidlRoot,
				  LPCSTR pathRoot, LPCITEMIDLIST pidlChild, REFCLSID clsid, REFIID riid, LPVOID * ppvOut)
{
    HRESULT hr;
    IPersistFolder3 *ppf;

    TRACE ("%p %s %p\n", pidlRoot, pathRoot, pidlChild);

    if (SUCCEEDED ((hr = SHCoCreateInstance (NULL, &CLSID_ShellFSFolder, NULL, riid, ppvOut)))) {
	if (SUCCEEDED (IUnknown_QueryInterface ((IUnknown *) * ppvOut, &IID_IPersistFolder3, (LPVOID *) & ppf))) {
	    PERSIST_FOLDER_TARGET_INFO ppfti;
	    LPITEMIDLIST pidlAbsolute;
	    char szDestPath[MAX_PATH];

	    ZeroMemory (&ppfti, sizeof (ppfti));

	    /* combine pidls */
	    pidlAbsolute = ILCombine (pidlRoot, pidlChild);

	    /* build path */
	    if (pathRoot) {
		lstrcpyA (szDestPath, pathRoot);
		PathAddBackslashA(szDestPath);          /* FIXME: why have drives a backslash here ? */
	    } else {
		szDestPath[0] = '\0';
	    }

	    if (pidlChild) {
		LPSTR pszChild = _ILGetTextPointer(pidlChild);

		if (pszChild)
		    lstrcatA (szDestPath, pszChild);
		else
		    hr = E_INVALIDARG;
	    }

	    /* fill the PERSIST_FOLDER_TARGET_INFO */
	    ppfti.dwAttributes = -1;
	    ppfti.csidl = -1;
	    MultiByteToWideChar (CP_ACP, 0, szDestPath, -1, ppfti.szTargetParsingName, MAX_PATH);

	    IPersistFolder3_InitializeEx (ppf, NULL, pidlAbsolute, &ppfti);
	    IPersistFolder3_Release (ppf);
	    ILFree (pidlAbsolute);
	}
    }
    TRACE ("-- (%p) ret=0x%08lx\n", *ppvOut, hr);
    return hr;
}

/***********************************************************************
 *	SHELL32_BindToChild
 *
 * Common code for IShellFolder_BindToObject.
 * Creates a shell folder by binding to a root pidl.
 */
HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot,
			     LPCSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut)
{
    GUID const *clsid;
    IShellFolder *pSF;
    HRESULT hr;
    LPITEMIDLIST pidlChild;

    if (!pidlRoot || !ppvOut)
	return E_INVALIDARG;

    *ppvOut = NULL;

    pidlChild = ILCloneFirst (pidlComplete);

    if ((clsid = _ILGetGUIDPointer (pidlChild))) {
	/* virtual folder */
	hr = SHELL32_CoCreateInitSF (pidlRoot, pidlChild, clsid, &IID_IShellFolder, (LPVOID *) & pSF);
    } else {
	/* file system folder */
	hr = SHELL32_CoCreateInitSFEx (pidlRoot, pathRoot, pidlChild, &CLSID_ShellFSFolder, &IID_IShellFolder,
				       (LPVOID *) & pSF);
    }
    ILFree (pidlChild);

    if (SUCCEEDED (hr)) {
	if (_ILIsPidlSimple (pidlComplete)) {
	    /* no sub folders */
	    hr = IShellFolder_QueryInterface (pSF, riid, ppvOut);
	} else {
	    /* go deeper */
	    hr = IShellFolder_BindToObject (pSF, ILGetNext (pidlComplete), NULL, riid, ppvOut);
	}
	IShellFolder_Release (pSF);
    }

    TRACE ("-- returning (%p) %08lx\n", *ppvOut, hr);

    return hr;
}

/***********************************************************************
 *	SHELL32_GetDisplayNameOfChild
 *
 * Retrives the display name of a child object of a shellfolder.
 *
 * For a pidl eg. [subpidl1][subpidl2][subpidl3]:
 * - it binds to the child shellfolder [subpidl1]
 * - asks it for the displayname of [subpidl2][subpidl3]
 *
 * Is possible the pidl is a simple pidl. In this case it asks the
 * subfolder for the displayname of a empty pidl. The subfolder
 * returns the own displayname eg. "::{guid}". This is used for
 * virtual folders with the registry key WantsFORPARSING set.
 */
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf,
				       LPCITEMIDLIST pidl, DWORD dwFlags, LPSTR szOut, DWORD dwOutLen)
{
    LPITEMIDLIST pidlFirst;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(pidl=%p 0x%08lx %p 0x%08lx)\n", psf, pidl, dwFlags, szOut, dwOutLen);
    pdump (pidl);

    pidlFirst = ILCloneFirst (pidl);
    if (pidlFirst) {
	IShellFolder2 *psfChild;

	hr = IShellFolder_BindToObject (psf, pidlFirst, NULL, &IID_IShellFolder, (LPVOID *) & psfChild);
	if (SUCCEEDED (hr)) {
	    STRRET strTemp;
	    LPITEMIDLIST pidlNext = ILGetNext (pidl);

	    hr = IShellFolder_GetDisplayNameOf (psfChild, pidlNext, dwFlags, &strTemp);
	    if (SUCCEEDED (hr)) {
		hr = StrRetToStrNA (szOut, dwOutLen, &strTemp, pidlNext);
	    }
	    IShellFolder_Release (psfChild);
	}
	ILFree (pidlFirst);
    } else
	hr = E_OUTOFMEMORY;

    TRACE ("-- ret=0x%08lx %s\n", hr, szOut);

    return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * observerd values:
 *  folder:	0xE0000177	FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:	0x40000177	FILESYSTEM
 *  drive:	0xf0000144	FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:	0xb0000154	HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * win2k:
 *  folder:    0xF0400177      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER
 *  file:      0x40400177      FILESYSTEM | CANMONIKER
 *  drive      0xF0400154      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER | CANRENAME (LABEL)
 *
 * This function does not set flags!! It only resets flags when necessary.
 */
HRESULT SHELL32_GetItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    GUID const *clsid;
    DWORD dwAttributes;
    DWORD dwSupportedAttr=SFGAO_CANCOPY |           /*0x00000001 */
                          SFGAO_CANMOVE |           /*0x00000002 */
                          SFGAO_CANLINK |           /*0x00000004 */
                          SFGAO_CANRENAME |         /*0x00000010 */
                          SFGAO_CANDELETE |         /*0x00000020 */
                          SFGAO_HASPROPSHEET |      /*0x00000040 */
                          SFGAO_DROPTARGET |        /*0x00000100 */
                          SFGAO_LINK |              /*0x00010000 */
                          SFGAO_READONLY |          /*0x00040000 */
                          SFGAO_HIDDEN |            /*0x00080000 */
                          SFGAO_FILESYSANCESTOR |   /*0x10000000 */
                          SFGAO_FOLDER |            /*0x20000000 */
                          SFGAO_FILESYSTEM |        /*0x40000000 */
                          SFGAO_HASSUBFOLDER;       /*0x80000000 */
    
    TRACE ("0x%08lx\n", *pdwAttributes);

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08lx not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    if (_ILIsDrive (pidl)) {
        *pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    } else if ((clsid = _ILGetGUIDPointer (pidl))) {
	if (HCR_GetFolderAttributes (clsid, &dwAttributes)) {
	    *pdwAttributes &= dwAttributes;
	} else {
	    *pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
	}
    } else if (_ILGetDataPointer (pidl)) {
	dwAttributes = _ILGetFileAttributes (pidl, NULL, 0);
	*pdwAttributes &= ~SFGAO_FILESYSANCESTOR;

	if ((SFGAO_FOLDER & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
	    *pdwAttributes &= ~(SFGAO_FOLDER | SFGAO_HASSUBFOLDER);

	if ((SFGAO_HIDDEN & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_HIDDEN))
	    *pdwAttributes &= ~SFGAO_HIDDEN;

	if ((SFGAO_READONLY & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_READONLY))
	    *pdwAttributes &= ~SFGAO_READONLY;

	if (SFGAO_LINK & *pdwAttributes) {
	    char ext[MAX_PATH];

	    if (!_ILGetExtension(pidl, ext, MAX_PATH) || strcasecmp(ext, "lnk"))
		*pdwAttributes &= ~SFGAO_LINK;
	}
    } else {
	*pdwAttributes &= SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
    }
    TRACE ("-- 0x%08lx\n", *pdwAttributes);
    return S_OK;
}

/***********************************************************************
 *  SHELL32_CompareIDs
 */
HRESULT SHELL32_CompareIDs (IShellFolder * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int type1,
      type2;
    char szTemp1[MAX_PATH];
    char szTemp2[MAX_PATH];
    int nReturn = 0;
    LPITEMIDLIST firstpidl,
      nextpidl1,
      nextpidl2;
    IShellFolder *psf;

    /* test for empty pidls */
    BOOL isEmpty1 = _ILIsDesktop (pidl1);
    BOOL isEmpty2 = _ILIsDesktop (pidl2);

    if (isEmpty1 && isEmpty2)
	return 0;
    if (isEmpty1)
	return -1;
    if (isEmpty2)
	return 1;

    /* test for different types. Sort order is the PT_* constant */
    type1 = _ILGetDataPointer (pidl1)->type;
    type2 = _ILGetDataPointer (pidl2)->type;
    if (type1 != type2)
	return (type1 - type2);

    /* test for name of pidl */
    _ILSimpleGetText (pidl1, szTemp1, MAX_PATH);
    _ILSimpleGetText (pidl2, szTemp2, MAX_PATH);
    nReturn = strcasecmp (szTemp1, szTemp2);
    if (nReturn != 0)
	return nReturn;

    /* test of complex pidls */
    firstpidl = ILCloneFirst (pidl1);
    nextpidl1 = ILGetNext (pidl1);
    nextpidl2 = ILGetNext (pidl2);

    /* optimizing: test special cases and bind not deeper */
    /* the deeper shellfolder would do the same */
    isEmpty1 = _ILIsDesktop (nextpidl1);
    isEmpty2 = _ILIsDesktop (nextpidl2);

    if (isEmpty1 && isEmpty2) {
	nReturn = 0;
    } else if (isEmpty1) {
	nReturn = -1;
    } else if (isEmpty2) {
    	nReturn = 1;
    /* optimizing end */
    } else if (SUCCEEDED (IShellFolder_BindToObject (iface, firstpidl, NULL, &IID_IShellFolder, (LPVOID *) & psf))) {
	nReturn = IShellFolder_CompareIDs (psf, lParam, nextpidl1, nextpidl2);
	IShellFolder_Release (psf);
    }
    ILFree (firstpidl);
    return nReturn;
}

/***********************************************************************
 *  SHCreateLinks
 *
 *   Undocumented.
 */
HRESULT WINAPI SHCreateLinks( HWND hWnd, LPCSTR lpszDir, LPDATAOBJECT lpDataObject,
                              UINT uFlags, LPITEMIDLIST *lppidlLinks)
{
    FIXME("%p %s %p %08x %p\n",hWnd,lpszDir,lpDataObject,uFlags,lppidlLinks);
    return E_NOTIMPL;
}
