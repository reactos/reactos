/*
 * with - With-procedures
 *
 *  A "With-procedure" creates an object, calls the callback (passing
 *  the object), then frees the object.  The return value
 *  of the with-procedure is the value returned by the callback.
 *
 *  This encapsulates the concept of "Get something, do something with
 *  it, then free it."  Forgetting to free objects on the error path
 *  is a common mistake; by doing it this way, the act of freeing the
 *  object is done automatically.
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DEFINE_SHLGUID(name, l, w1, w2) \
	 DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

DEFINE_SHLGUID(IID_IShellFolder,    	0x000214E6L, 0, 0);

const CHAR CODESEG c_szStarDotStar[] = "*.*";

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  WithPidl
 *
 *	Create a pidl from an psf and a relative path, call the callback,
 *	then free the pidl.
 *
 *	Returns 0 on error, else propagates the callback's return value
 *	through.
 *
 *****************************************************************************/

BOOL PASCAL
WithPidl(PSF psf, LPCSTR lqn, BOOL (*pfn)(PIDL, LPVOID), LPVOID pv)
{
    PIDL pidl = pidlFromPath(psf, lqn);
    if (pidl) {
	BOOL fRc = pfn(pidl, pv);
	Ole_Free(pidl);
	return fRc;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  WithPsf
 *
 *	Bind to an IShellFolder, call the callback, then release the
 *	IShellFolder.
 *
 *	Returns 0 on error, else propagates the callback's return value
 *	through.
 *
 *****************************************************************************/

BOOL PASCAL
WithPsf(PSF psf, PIDL pidl, BOOL (*pfn)(PSF, LPVOID), LPVOID pv)
{
    PSF psfNew;
    if (SUCCEEDED(psf->lpVtbl->BindToObject(psf, pidl, 0, 
		  &IID_IShellFolder, &psfNew))) {
	BOOL fRc;
	fRc = pfn(psfNew, pv);
	Ole_Release(psfNew);
	return fRc;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  EmptyDirectory
 *
 *	Delete all the files in the indicated directory, first calling a
 *	function in that directory.
 *
 *****************************************************************************/

BOOL PASCAL
EmptyDirectory(LPCSTR pszDir, BOOL (*pfn)(LPCSTR, LPVOID), LPVOID pv)
{
    BOOL fRc;
    char szPrevDir[MAX_PATH];

    GetCurrentDirectory(cA(szPrevDir), szPrevDir); /* For restore */
    if (SetCurrentDirectory(pszDir)) {
	WIN32_FIND_DATA wfd;
	HANDLE h;
	if (pfn) {
	    fRc = pfn(pszDir, pv);
	}
	h = FindFirstFile(c_szStarDotStar, &wfd);
	if (h != INVALID_HANDLE_VALUE) {
	    do {
		DeleteFile(wfd.cFileName);
	    } while (FindNextFile(h, &wfd));
            FindClose(h);
	}
	SetCurrentDirectory(szPrevDir);
    }
    return fRc;
}

/*****************************************************************************
 *
 *  WithTempDirectory
 *
 *	Ask KERNEL for a unique temp name and create a temp directory there.
 *	Change to the new directory, call the callback, and then change
 *	back to the previous directory.  Remove all the files in the temp
 *	directory, then remove the temp directory.  Note that we don't
 *	implement full recursive cleanup, so if you create any subdirectories
 *	in the temp directory, you have to remove them yourself.
 *
 *	Returns 0 on error, else propagates the callback's return value
 *	through.
 *
 *****************************************************************************/

BOOL PASCAL
WithTempDirectory(BOOL (*pfn)(LPCSTR, LPVOID), LPVOID pv)
{
    BOOL fRc;
    char szTmpDir[MAX_PATH + 1 + 8 + 1 + 3 + 1];
    if (GetTempPath(MAX_PATH, szTmpDir) &&
	GetTempFileName(szTmpDir, "", 0, szTmpDir)) { /* Got a unique file */
	DeleteFile(szTmpDir);		/* Nuke the file; we want a dir */
	if (CreateDirectory(szTmpDir, 0)) {
	    fRc = EmptyDirectory(szTmpDir, pfn, pv);
	    RemoveDirectory(szTmpDir);
	} else {
	    fRc = 0;		/* Couldn't create the directory */
	}
    } else {
	fRc = 0;		/* Couldn't generate unique name */
    }
    return fRc;
}
