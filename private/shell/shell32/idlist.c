#include "shellprv.h"
#pragma  hdrstop
#include "fstreex.h"    // FS_MakeCommonItem

#ifdef DEBUG
// Dugging aids for making sure we dont use free pidls
#define VALIDATE_PIDL(pidl) ASSERT(IS_VALID_PIDL(pidl))
#else
#define VALIDATE_PIDL(pidl)
#endif


STDAPI_(BOOL) SHIsValidPidl(LPCITEMIDLIST pidl)
{
    try
    {
        // I use my own while loop instead of ILGetSize to avoid extra debug spew,
        // including asserts, that would result from the VALIDATE_PIDL call.  We are
        // testing for validity so an invalid pidl is an OK condition.
        while (pidl->mkid.cb)
        {
            pidl = _ILNext(pidl);
        }
        return TRUE;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
}


STDAPI_(LPITEMIDLIST) ILGetNext(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlRet = NULL;
    if (pidl && pidl->mkid.cb)
    {
        VALIDATE_PIDL(pidl);
        pidlRet = _ILNext(pidl);
    }

    return pidlRet;
}

STDAPI_(UINT) ILGetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        VALIDATE_PIDL(pidl);
        cbTotal += SIZEOF(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = _ILNext(pidl);
        }
    }

    return cbTotal;
}

#define CBIDL_MIN       256
#define CBIDL_INCL      256

STDAPI_(LPITEMIDLIST) _ILCreate(UINT cbSize)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbSize);
    if (pidl)
        memset(pidl, 0, cbSize);      // zero-init for external task allocator

    return pidl;
}

STDAPI_(LPITEMIDLIST) ILCreate()
{
    return _ILCreate(CBIDL_MIN);
}

// cbExtra is the amount to add to cbRequired if the block needs to grow,
// or it is 0 if we want to resize to the exact size

STDAPI_(LPITEMIDLIST) ILResize(LPITEMIDLIST pidl, UINT cbRequired, UINT cbExtra)
{
    LPITEMIDLIST pidlsave = pidl;
    if (pidl == NULL)
    {
        pidl = _ILCreate(cbRequired + cbExtra);
    }
    else if (!cbExtra || SHGetSize(pidl) < cbRequired)
    {
        pidl = (LPITEMIDLIST)SHRealloc(pidl, cbRequired + cbExtra);
    }
    return pidl;
}

STDAPI_(LPITEMIDLIST) ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend)
{
    UINT cbUsed, cbRequired;
    LPITEMIDLIST pidlSave = pidl;

    // Create the ID list, if it is not given.
    if (!pidl)
    {
        pidl = ILCreate();
        if (!pidl)
            return NULL;        // memory overflow
    }

    cbUsed = ILGetSize(pidl);
    cbRequired = cbUsed + pmkid->cb;

    pidl = ILResize(pidl, cbRequired, CBIDL_INCL);
    if (!pidl)
        return NULL;    // memory overflow

    if (fAppend)
    {
        // Append it.
        MoveMemory(_ILSkip(pidl, cbUsed-SIZEOF(pidl->mkid.cb)), pmkid, pmkid->cb);
    }
    else
    {
        // Put it at the top
        MoveMemory(_ILSkip(pidl, pmkid->cb), pidl, cbUsed);
        MoveMemory(pidl, pmkid, pmkid->cb);

        ASSERT((ILGetSize(_ILNext(pidl))==cbUsed) ||
               (pmkid->cb == 0)); // if we're prepending the empty pidl, nothing changed
    }

    // We must put zero-terminator because of LMEM_ZEROINIT.
    _ILSkip(pidl, cbRequired-SIZEOF(pidl->mkid.cb))->mkid.cb = 0;
    ASSERT(ILGetSize(pidl) == cbRequired);

    return pidl;
}


STDAPI_(LPITEMIDLIST) ILFindLastID(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    if (pidl == NULL)
        return NULL;

    VALIDATE_PIDL(pidl);

    // Find the last one
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = _ILNext(pidlLast);
    }

    return (LPITEMIDLIST)pidlLast;
}


STDAPI_(BOOL) ILRemoveLastID(LPITEMIDLIST pidl)
{
    BOOL fRemoved = FALSE;

    if (pidl == NULL)
        return FALSE;

    if (pidl->mkid.cb)
    {
        LPITEMIDLIST pidlLast = (LPITEMIDLIST)ILFindLastID(pidl);

        ASSERT(pidlLast->mkid.cb);
        ASSERT(_ILNext(pidlLast)->mkid.cb==0);

        // Remove the last one
        pidlLast->mkid.cb = 0; // null-terminator
        fRemoved = TRUE;
    }

    return fRemoved;
}

STDAPI_(LPITEMIDLIST) ILClone(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        UINT cb = ILGetSize(pidl);
        LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);
        if (pidlRet)
            memcpy(pidlRet, pidl, cb);

        return pidlRet;
    }
    return NULL;
}


STDAPI_(LPITEMIDLIST) ILCloneCB(LPCITEMIDLIST pidl, UINT cbPidl)
{
    UINT cb = cbPidl + SIZEOF(pidl->mkid.cb);
    LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);
    if (pidlRet)
    {
        memcpy(pidlRet, pidl, cbPidl);
        // NT5 221998: cbPidl can be odd, must use UNALIGNED
        *((UNALIGNED WORD *)((BYTE *)pidlRet + cbPidl)) = 0;  // NULL terminate
    }
    return pidlRet;
}

STDAPI_(LPITEMIDLIST) ILCloneUpTo(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlUpTo)
{
    return ILCloneCB(pidl, (UINT)((BYTE *)pidlUpTo - (BYTE *)pidl));
}

STDAPI_(LPITEMIDLIST) ILCloneFirst(LPCITEMIDLIST pidl)
{
    return ILCloneCB(pidl, pidl->mkid.cb);
}

STDAPI_(LPITEMIDLIST) ILGlobalClone(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        UINT cb = ILGetSize(pidl);
        LPITEMIDLIST pidlRet = (LPITEMIDLIST)Alloc(cb);
        if (pidlRet)
        {
            memcpy(pidlRet, pidl, cb);
        }
        return pidlRet;
    }
    return NULL;
}

STDAPI_(BOOL) ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);

    if (pidl1 == pidl2)
        return TRUE;
    else
    {
        UINT cb = ILGetSize(pidl1);
        if (cb == ILGetSize(pidl2) && memcmp(pidl1, pidl2, cb) == 0)
            return TRUE;
        else
        {
            BOOL fRet = FALSE;
            IShellFolder *psfDesktop;
            if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
            {
                if (psfDesktop->lpVtbl->CompareIDs(psfDesktop, 0, pidl1, pidl2) == 0)
                    fRet = TRUE;
                psfDesktop->lpVtbl->Release(psfDesktop);
            }
            return fRet;
        }
    }
}

// test if
//      pidlParent is a parent of pidlBelow
//      fImmediate requires that pidlBelow be a direct child of pidlParent.
//      Otherwise, self and grandchildren are okay too.
//
// example:
//      pidlParent: [my comp] [c:\] [windows]
//      pidlBelow:  [my comp] [c:\] [windows] [system32] [vmm.vxd]
//      fImmediate == FALSE result: TRUE
//      fImmediate == TRUE  result: FALSE

STDAPI_(BOOL) ILIsParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlBelow, BOOL fImmediate)
{
    LPITEMIDLIST pidlBelowPrefix;
    UINT cb;
    LPCITEMIDLIST pidlParentT;
    LPCITEMIDLIST pidlBelowT;

    VALIDATE_PIDL(pidlParent);
    VALIDATE_PIDL(pidlBelow);

    if (!pidlParent || !pidlBelow)
        return FALSE;

    /* BUGBUG: This code will not work correctly when comparing simple NET id lists
    /  against, real net ID lists.  Simple ID lists DO NOT contain network provider
    /  information therefore cannot pass the initial check of is pidlBelow longer than pidlParent.
    /  daviddv (2/19/1996) */

    for (pidlParentT = pidlParent, pidlBelowT = pidlBelow; !ILIsEmpty(pidlParentT);
         pidlParentT = _ILNext(pidlParentT), pidlBelowT = _ILNext(pidlBelowT))
    {
        // if pidlBelow is shorter than pidlParent, pidlParent can't be its parent.
        if (ILIsEmpty(pidlBelowT))
            return FALSE;
    }

    if (fImmediate)
    {
        // If fImmediate is TRUE, pidlBelowT should contain exactly one ID.
        if (ILIsEmpty(pidlBelowT) || !ILIsEmpty(_ILNext(pidlBelowT)))
            return FALSE;
    }

    //
    // Create a new IDList from a portion of pidlBelow, which contains the
    // same number of IDs as pidlParent.
    //
    cb = (UINT)((UINT_PTR)pidlBelowT - (UINT_PTR)pidlBelow);
    pidlBelowPrefix = _ILCreate(cb + SIZEOF(pidlBelow->mkid.cb));
    if (pidlBelowPrefix)
    {
        BOOL fRet;
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            MoveMemory(pidlBelowPrefix, pidlBelow, cb);

            ASSERT(ILGetSize(pidlBelowPrefix) == cb + SIZEOF(pidlBelow->mkid.cb));

            fRet = psfDesktop->lpVtbl->CompareIDs(psfDesktop, 0, pidlParent, pidlBelowPrefix) == ResultFromShort(0);
            psfDesktop->lpVtbl->Release(psfDesktop);

            ILFree(pidlBelowPrefix);

            return fRet;
        }
    }

    return FALSE;
}

// this returns a pointer to the child id ie:
// given 
//  pidlParent = [my comp] [c] [windows] [desktop]
//  pidlChild  = [my comp] [c] [windows] [desktop] [dir] [bar.txt]
// return pointer to:
//  [dir] [bar.txt]
// NULL is returned if pidlParent is not a parent of pidlChild
STDAPI_(LPITEMIDLIST) ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    if (ILIsParent(pidlParent, pidlChild, FALSE))
    {
        while (!ILIsEmpty(pidlParent))
        {
            pidlChild = _ILNext(pidlChild);
            pidlParent = _ILNext(pidlParent);
        }
        return (LPITEMIDLIST)pidlChild;
    }
    return NULL;
}

STDAPI_(LPITEMIDLIST) ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // Let me pass in NULL pointers
    if (!pidl1)
    {
        if (!pidl2)
        {
            return NULL;
        }
        return ILClone(pidl2);
    }
    else if (!pidl2)
    {
        return ILClone(pidl1);
    }

    {
        LPITEMIDLIST pidlNew;
        UINT cb1 = ILGetSize(pidl1) - SIZEOF(pidl1->mkid.cb);
        UINT cb2 = ILGetSize(pidl2);

        VALIDATE_PIDL(pidl1);
        VALIDATE_PIDL(pidl2);
        pidlNew = _ILCreate(cb1 + cb2);
        if (pidlNew)
        {
            MoveMemory(pidlNew, pidl1, cb1);
            MoveMemory((LPTSTR)(((LPBYTE)pidlNew) + cb1), pidl2, cb2);
            ASSERT(ILGetSize(pidlNew) == cb1+cb2);
        }

        return pidlNew;
    }
}

STDAPI_(void) ILFree(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        ASSERT(IS_VALID_PIDL(pidl));
        SHFree(pidl);
    }
}

STDAPI_(void) ILGlobalFree(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        ASSERT(IS_VALID_PIDL(pidl));
        Free(pidl);
    }
}

STDAPI ILCreateFromPathEx(LPCTSTR pszPath, IUnknown *punkToSkip, ILCFP_FLAGS dwFlags, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        WCHAR wszPath[MAX_URL_STRING];
        ULONG cchEaten;
        IBindCtx *pbc = NULL;
        if (punkToSkip || (dwFlags & ILCFP_FLAG_SKIPJUNCTIONS))
            SHCreateSkipBindCtx(punkToSkip, &pbc);

        // Must use a private buffer because ParseDisplayName takes a non-const pointer
        SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

        hres = psfDesktop->lpVtbl->ParseDisplayName(psfDesktop, NULL, pbc, wszPath, &cchEaten, ppidl, rgfInOut);
        psfDesktop->lpVtbl->Release(psfDesktop);

        if (pbc)
            pbc->lpVtbl->Release(pbc);
    }
    return hres;
}


STDAPI SHILCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    return ILCreateFromPathEx(pszPath, NULL, ILCFP_FLAG_NORMAL, ppidl, rgfInOut);
}


STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hres = SHILCreateFromPath(pszPath, &pidl, NULL);

    ASSERT(SUCCEEDED(hres) ? pidl != NULL : pidl == NULL);

    return pidl;
}


#ifdef UNICODE

LPITEMIDLIST ILCreateFromPathA(IN LPCSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    SHAnsiToUnicode(pszPath, szPath, SIZECHARS(szPath));

    return ILCreateFromPath(szPath);
}

#else

LPITEMIDLIST ILCreateFromPathW(IN LPCWSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    SHUnicodeToAnsi(pszPath, szPath, SIZECHARS(szPath));

    return ILCreateFromPath(szPath);
}

#endif // UNICODE


STDAPI_(BOOL) ILGetDisplayNameExA(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPSTR pszName, DWORD cchSize, int fType)
{
    TCHAR szPath[MAX_URL_STRING];       // The size is required by ILGetDisplayNameEx (because it doesn't take a size param).

    if (ILGetDisplayNameEx(psfRoot, pidl, szPath, fType))
    {
        //  we assume the incoming buffer is MAX_PATH;
        SHTCharToAnsiCP(CP_ACP, szPath, pszName, cchSize);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) ILGetDisplayNameExW(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPWSTR pszName, DWORD cchSize, int fType)
{
    TCHAR szPath[MAX_URL_STRING];       // The size is required by ILGetDisplayNameEx (because it doesn't take a size param).

    if (ILGetDisplayNameEx(psfRoot, pidl, szPath, fType))
    {
        //  we assume the incoming buffer is MAX_PATH;
        SHTCharToUnicodeCP(CP_ACP, szPath, pszName, cchSize);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) ILGetDisplayNameEx(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPTSTR pszName, int fType)
{
    STRRET srName;
    DWORD dwGDNFlags;
    BOOL fReleaseNeeded = FALSE;
    BOOL fRetVal = FALSE;

    TraceMsg(TF_WARNING, "WARNING: ILGetDisplayNameEx() has been deprecated, should use SHGetNameAndFlags() instead!!!");
    
    if (!pszName)
        return FALSE;

    *pszName = 0;

    if (!pidl)
        return FALSE;

    // no root specified, get the desktop folder as the default
    if (!psfRoot)
    {
        if (FAILED(SHGetDesktopFolder(&psfRoot)))
            return FALSE;

        //
        // We created this psf, so we need to release it.
        // The caller doesn't know anything about it.
        //
        fReleaseNeeded = TRUE;
    }

    //
    // From here on, don't return.
    // Set fRetVal and goto done to ensure that psfRoot is released
    // if necessary.
    //

    switch (fType)
    {
    case ILGDN_FULLNAME:
        dwGDNFlags = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;

SingleLevelPidl:
        if (SUCCEEDED(psfRoot->lpVtbl->GetDisplayNameOf(psfRoot, pidl, dwGDNFlags, &srName)) &&
            SUCCEEDED(StrRetToBuf(&srName, pidl, pszName, MAX_PATH)))
        {
            fRetVal = TRUE;
        }
        break;

    case ILGDN_INFOLDER:
    case ILGDN_ITEMONLY:
        dwGDNFlags = fType == ILGDN_INFOLDER ? SHGDN_INFOLDER : SHGDN_NORMAL;

        if (!ILIsEmpty(pidl))
        {
            IShellFolder *psfParent;
            LPITEMIDLIST pidlLast;
            
            if (SUCCEEDED(SHBindToIDListParent(pidl, &IID_IShellFolder, (void **)&psfParent, &pidlLast)))
            {
                if (SUCCEEDED(psfParent->lpVtbl->GetDisplayNameOf(psfParent, pidlLast, dwGDNFlags, &srName)))
                {
                    StrRetToBuf(&srName, pidlLast, pszName, MAX_PATH);
                    fRetVal = TRUE;
                }
                psfParent->lpVtbl->Release(psfParent);
            }
        }
        else
        {
            goto SingleLevelPidl;
        }
        break;
    }

    if (fReleaseNeeded && psfRoot) {
        psfRoot->lpVtbl->Release(psfRoot);
        psfRoot = NULL;
    } // if

    return fRetVal;
}

STDAPI_(BOOL) ILGetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    return ILGetDisplayNameEx(NULL, pidl, pszPath, ILGDN_FULLNAME);
}

extern int GetSpecialFolderParentIDAndOffset(LPCITEMIDLIST pidl, ULONG *pcbOffset);

//***   ILGetPseudoName% -- encode pidl relative to base
// ENTRY/EXIT
//  pidlSpec    [pidl], csidl, -1 (computed base), or 0 (no base)
//  fType       NYI
// EG
//  "c:/winnt/Profiles/joe/start menu/programs/foo.lnk", CSIDL_PROGRAMS =>
//      %csidl3%/foo.lnk
//  "c:/winnt/Profiles/joe/start menu/programs/foo.lnk", -1 =>
//      %csidl3%/foo.lnk
//  "c:/winnt/Profiles/joe/start menu/programs/foo.lnk", "c:/winnt" =>
//      %xxx%/Profiles/... (NYI)
// NOTES
//  NYI: pidlBase of type pidl (since we'd need to generate and store %xxx%)
//  WARNING: CSIDL_DESKTOP is currently 0, causing some ambiguity
//  BUGBUG todo: might want to make ILGetDisplayNameEx(..., ILGDN_PSEUDONAME)
//  however that could only support the -1 form
//
STDAPI_(BOOL) ILGetPseudoName(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlSpec, TCHAR *pszBuf, int fType)
{
    int i;
    int idFolder;
    TCHAR szSpec[MAX_PATH], szFull[MAX_URL_STRING];

    ASSERT(IS_INTRESOURCE(pidlSpec));
    idFolder = PtrToUlong(pidlSpec);
    ASSERT(idFolder != 0);  // BUGBUG CSIDL_DESKTOP==0

    // warning: 0 means both 'none' and CSIDL_DESKTOP (see def)
    // so for now we don't handle CSIDL_DESKTOP
    // BUGBUG fCreate=FALSE or TRUE?
    EVAL(SHGetSpecialFolderPath(NULL, szSpec, idFolder, FALSE));
    SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szFull, SIZECHARS(szFull), NULL);

    if ((i = PathCommonPrefix(szSpec, szFull, NULL)) && (i == lstrlen(szSpec))) {
        // %csidl#%\x\y\z
        wsprintf(pszBuf, TEXT("%%csidl%d%%%s"), idFolder, szFull + i);
    }
    else {
        // \x\y\z
        lstrcpy(pszBuf, szFull);
    }

    return TRUE;
}

#ifndef UNICODE
STDAPI_(BOOL) ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlSpec, WCHAR *pszBuf, int fType)
{
    TCHAR sz[MAX_URL_STRING];

    BOOL bRet = ILGetPseudoName(pidl, pidlSpec, sz, fType);
    SHAnsiToUnicode(sz, pszBuf, ARRAYSIZE(sz));
    return bRet;
}
#endif




//===========================================================================
// IDLIST: Stream access
// BUGBUG: check bytes read on Read calls?
//===========================================================================

STDAPI ILLoadFromStream(LPSTREAM pstm, LPITEMIDLIST * ppidl)
{
    HRESULT hres;
    ULONG cb;

    ASSERT(ppidl);

    // Delete the old one if any.
    if (*ppidl)
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    // Read the size of the IDLIST
    cb = 0;             // WARNING: We need to fill its HIWORD!
    hres = pstm->lpVtbl->Read(pstm, &cb, SIZEOF(USHORT), NULL); // Yes, USHORT
    if (SUCCEEDED(hres) && cb)
    {
        // Create a IDLIST
        LPITEMIDLIST pidl = _ILCreate(cb);
        if (pidl)
        {
            // Read its contents
            hres = pstm->lpVtbl->Read(pstm, pidl, cb, NULL);
            if (SUCCEEDED(hres))
            {
                *ppidl = pidl;
            }
            else
            {
                ILFree(pidl);
            }
        }
        else
        {
           hres = E_OUTOFMEMORY;
        }
    }

    return hres;
}

// BUGBUG: check bytes written on Write calls?

STDAPI ILSaveToStream(LPSTREAM pstm, LPCITEMIDLIST pidl)
{
    HRESULT hres;
    ULONG cb = ILGetSize(pidl);
    ASSERT(HIWORD(cb) == 0);
    hres = pstm->lpVtbl->Write(pstm, &cb, SIZEOF(USHORT), NULL); // Yes, USHORT
    if (SUCCEEDED(hres) && cb)
    {
        if (SUCCEEDED(hres))
        {
            hres = pstm->lpVtbl->Write(pstm, pidl, cb, NULL);
        }
    }

    return hres;
}
//===========================================================================
// IDLARRAY stuff
//===========================================================================

#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])


STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST * apidl)
{
    HIDA hida;
#if _MSC_VER == 1100
// BUGBUG: Workaround code generate bug in VC5 X86 compiler (12/30 version).
    volatile
#endif
    UINT i;
    UINT offset = SIZEOF(CIDA) + SIZEOF(UINT)*cidl;
    UINT cbTotal = offset + ILGetSize(pidlFolder);
    for (i=0; i<cidl ; i++) {
        cbTotal += ILGetSize(apidl[i]);
    }

    hida = GlobalAlloc(GPTR, cbTotal);  // This MUST be GlobalAlloc!!!
    if (hida)
    {
        LPIDA pida = (LPIDA)hida;       // no need to lock

        LPCITEMIDLIST pidlNext;
        pida->cidl = cidl;

        for (i=0, pidlNext=pidlFolder; ; pidlNext=apidl[i++])
        {
            UINT cbSize = ILGetSize(pidlNext);
            pida->aoffset[i] = offset;
            MoveMemory(((LPBYTE)pida)+offset, pidlNext, cbSize);
            offset += cbSize;

            ASSERT(ILGetSize(HIDA_GetPIDLItem(pida,i-1)) == cbSize);

            if (i==cidl)
                break;
        }

        ASSERT(offset == cbTotal);
    }

    return hida;
}

STDAPI_(void) HIDA_Free(HIDA hida)
{
    GlobalFree(hida);
}

HIDA HIDA_Create2(void *pida, UINT cb)
{
    HIDA hida = GlobalAlloc(GPTR, cb);
    if (hida)
    {
        MoveMemory(hida, pida, cb);
    }
    return hida;
}

STDAPI_(HIDA) HIDA_Clone(HIDA hida)
{
    SIZE_T cbTotal = GlobalSize(hida);
    HIDA hidaCopy = GlobalAlloc(GPTR, cbTotal);
    if (hidaCopy)
    {
        MoveMemory(hidaCopy, GlobalLock(hida), cbTotal);
        GlobalUnlock(hida);
    }
    return hidaCopy;
}

STDAPI_(UINT) HIDA_GetCount(HIDA hida)
{
    UINT count = 0;
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        count = pida->cidl;
        GlobalUnlock(hida);
    }
    return count;
}

STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
        LPCITEMIDLIST pidlItem   = HIDA_GetPIDLItem(pida, i);
        UINT cbFolder  = ILGetSize(pidlFolder)-SIZEOF(USHORT);
        UINT cbItem = ILGetSize(pidlItem);
        if (cbMax < cbFolder+cbItem) {
            if (pidlOut) {
                pidlOut->mkid.cb = 0;
            }
        } else {
            MoveMemory(pidlOut, pidlFolder, cbFolder);
            MoveMemory(((LPBYTE)pidlOut)+cbFolder, pidlItem, cbItem);
        }
        GlobalUnlock(hida);

        return (cbFolder+cbItem);
    }
    return 0;
}

//
// This one reallocated pidl if necessary. NULL is valid to pass in as pidl.
//
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl)
{
    UINT cbRequired = HIDA_GetIDList(hida, i, NULL, 0);
    pidl = ILResize(pidl, cbRequired, 32); // extra 32-byte if we realloc
    if (pidl)
    {
        HIDA_GetIDList(hida, i, pidl, cbRequired);
    }

    return pidl;
}

LPCITEMIDLIST IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    if (NULL == pida)
    {
        return NULL;
    }

    if (i == (UINT)-1 || i < pida->cidl)
    {
        return HIDA_GetPIDLItem(pida, i);
    }

    return NULL;
}

LPCITEMIDLIST IDA_GetRelativeIDListPtr(LPIDA pida, UINT i, BOOL * pfAllocated)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)IDA_GetIDListPtr(pida, i);

    *pfAllocated = FALSE;

    if (pidl && ILIsEmpty(HIDA_GetPIDLFolder(pida)))
    {
        LPITEMIDLIST pidlCommon = SHCloneSpecialIDList(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
        if (pidlCommon)
        {
            BOOL bCommon = ILIsParent(pidlCommon, pidl, TRUE);

            pidl = ILClone(ILFindLastID(pidl));
            if (pidl)
            {
                *pfAllocated = TRUE;
                if (bCommon)
                    FS_MakeCommonItem(pidl);
            }
            ILFree(pidlCommon);
        }
    }

    return pidl;
}

LPITEMIDLIST HIDA_ILClone(HIDA hida, UINT i)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPITEMIDLIST pidl = IDA_ILClone(pida, i);
        GlobalUnlock(hida);
        return pidl;
    }
    return NULL;
}

//
//  This is a helper function to be called from within IShellFolder::CompareIDs.
// When the first IDs of pidl1 and pidl2 are the (logically) same.
//
// Required:
//  psf && pidl1 && pidl2 && !ILEmpty(pidl1) && !ILEmpty(pidl2)
//
HRESULT ILCompareRelIDs(IShellFolder *psfParent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres;
    LPCITEMIDLIST pidlRel1 = _ILNext(pidl1);
    LPCITEMIDLIST pidlRel2 = _ILNext(pidl2);
    if (ILIsEmpty(pidlRel1))
    {
        if (ILIsEmpty(pidlRel2))
            hres = ResultFromShort(0);
        else
            hres = ResultFromShort(-1);
    }
    else
    {
        if (ILIsEmpty(pidlRel2))
        {
            hres = ResultFromShort(1);
        }
        else
        {
            //
            // pidlRel1 and pidlRel2 point to something
            //  (1) Bind to the next level of the IShellFolder
            //  (2) Call its CompareIDs to let it compare the rest of IDs.
            //
            LPITEMIDLIST pidlNext = ILCloneFirst(pidl1);    // pidl2 would work as well
            if (pidlNext)
            {
                IShellFolder *psfNext;
                hres = psfParent->lpVtbl->BindToObject(psfParent, pidlNext, NULL, &IID_IShellFolder, &psfNext);
                if (SUCCEEDED(hres))
                {
                    hres = psfNext->lpVtbl->CompareIDs(psfNext, 0, pidlRel1, pidlRel2);
                    psfNext->lpVtbl->Release(psfNext);
                }
                ILFree(pidlNext);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }
    return hres;
}

// in:
//      pszLeft
//      pidl
//
// in/out:
//      pStrRet

STDAPI StrRetCatLeft(LPCTSTR pszLeft, STRRET *pStrRet, LPCITEMIDLIST pidl)
{
    HRESULT hres;
    TCHAR szRight[MAX_PATH];
    UINT cchRight, cchLeft = ualstrlen(pszLeft);

    switch(pStrRet->uType)
    {
    case STRRET_CSTR:
        cchRight = lstrlenA(pStrRet->cStr);
        break;
    case STRRET_OFFSET:
        cchRight = lstrlenA(STRRET_OFFPTR(pidl, pStrRet));
        break;
    case STRRET_WSTR:
        cchRight = lstrlenW(pStrRet->pOleStr);
        break;
    }

    if (cchLeft + cchRight < MAX_PATH) 
    {
        hres = StrRetToBuf(pStrRet, pidl, szRight, ARRAYSIZE(szRight)); // will free pStrRet for us
        if (SUCCEEDED(hres))
        {
#ifdef UNICODE
            pStrRet->pOleStr = SHAlloc((lstrlen(pszLeft) + 1 + cchRight) * SIZEOF(TCHAR));
            if (pStrRet->pOleStr == NULL)
            {
                hres = E_OUTOFMEMORY;
            }
            else
            {
                pStrRet->uType = STRRET_WSTR;
                lstrcpy(pStrRet->pOleStr, pszLeft);
                lstrcat(pStrRet->pOleStr, szRight);
                hres = S_OK;
            }
#else
            pStrRet->uType = STRRET_CSTR;
            lstrcpy(pStrRet->cStr, pszLeft);
            lstrcat(pStrRet->cStr, szRight);
            hres = S_OK;
#endif
        }
    } 
    else 
    {
        hres = E_NOTIMPL;       // BUGBUG
    }
    return hres;
}

void StrRetFormat(STRRET *pStrRet, LPCITEMIDLIST pidlRel, LPCTSTR pszTemplate, LPCTSTR pszAppend)
{
     LPTSTR pszRet;
     TCHAR szT[MAX_PATH];

     StrRetToBuf(pStrRet, pidlRel, szT, ARRAYSIZE(szT));
     pszRet = ShellConstructMessageString(HINST_THISDLL, pszTemplate, pszAppend, szT);
     if (pszRet)
     {
         StringToStrRet(pszRet, pStrRet);
         LocalFree(pszRet);
     }
}

//
// Notes: This one passes SHGDN_FORPARSING to ISF::GetDisplayNameOf.
//
HRESULT ILGetRelDisplayName(IShellFolder *psf, STRRET *pStrRet,
    LPCITEMIDLIST pidlRel, LPCTSTR pszName, LPCTSTR pszTemplate)
{
    HRESULT hres;
    LPITEMIDLIST pidlLeft = ILCloneFirst(pidlRel);
    if (pidlLeft)
    {
        IShellFolder *psfNext;
        hres = psf->lpVtbl->BindToObject(psf, pidlLeft, NULL, &IID_IShellFolder, &psfNext);
        if (SUCCEEDED(hres))
        {
            LPCITEMIDLIST pidlRight = _ILNext(pidlRel);
            hres = psfNext->lpVtbl->GetDisplayNameOf(psfNext, pidlRight, SHGDN_INFOLDER | SHGDN_FORPARSING, pStrRet);
            if (SUCCEEDED(hres))
            {
                if (pszTemplate)
                {
                    StrRetFormat(pStrRet, pidlRight, pszTemplate, pszName);
                }
                else
                {
                    hres = StrRetCatLeft(pszName, pStrRet, pidlRight);
                }
            }
            psfNext->lpVtbl->Release(psfNext);
        }

        ILFree(pidlLeft);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

//
// ILClone using Task allocator
//
STDAPI SHILClone(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlOut)
{
    *ppidlOut = ILClone(pidl);
    return *ppidlOut ? NOERROR : E_OUTOFMEMORY;
}

//
// ILCombine using Task allocator
//
STDAPI SHILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPITEMIDLIST * ppidlOut)
{
    *ppidlOut = ILCombine(pidl1, pidl2);
    return *ppidlOut ? NOERROR : E_OUTOFMEMORY;
}
