#include <windows.h>
#include <shlobj.h>
#include "debug.h"
#include "idlist.h"

#include "common.h"

#ifdef DEBUG
// Dugging aids for making sure we dont use free pidls
#define VALIDATE_PIDL(pidl) Assert((pidl)->mkid.cb != 0xC5C5)
#else
#define VALIDATE_PIDL(pidl)
#endif


// BUGBUG: should call shell alocator
#define SHAlloc(cb)         (void *)LocalAlloc(LPTR, cb)
#define SHFree(p)           LocalFree((HLOCAL)p)
#define SHGetSize(p)        LocalSize((HLOCAL)p)
#define SHRealloc(p, cb)    (void *)LocalReAlloc((HLOCAL)p, cb, LMEM_MOVEABLE | LMEM_ZEROINIT)

LPITEMIDLIST ILGetNext(LPCITEMIDLIST pidl)
{
    if (pidl && pidl->mkid.cb)
    {
        VALIDATE_PIDL(pidl);
        return _ILNext(pidl);
    }

    return NULL;
}

UINT ILGetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        VALIDATE_PIDL(pidl);
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
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

LPITEMIDLIST _ILCreate(UINT cbSize)
{
    return (LPITEMIDLIST)SHAlloc(cbSize);
}

LPITEMIDLIST ILCreate()
{
    return (LPITEMIDLIST)SHAlloc(CBIDL_MIN);
}

/*
 ** _ILResize
 *
 *  PARAMETERS:
 *      cbExtra is the amount to add to cbRequired if the block needs to grow,
 *      or it is 0 if we want to resize to the exact size
 *
 *  DESCRIPTION:
 *
 *  RETURNS:
 *
 */

LPITEMIDLIST _ILResize(LPITEMIDLIST pidl, UINT cbRequired, UINT cbExtra)
{
    if (pidl==NULL)
    {
        pidl = _ILCreate(cbRequired+cbExtra);
    }
    else if (!cbExtra || SHGetSize(pidl) < cbRequired)
    {
        pidl = (LPITEMIDLIST)SHRealloc(pidl, cbRequired+cbExtra);
    }
    return pidl;
}

LPITEMIDLIST ILFindLastID(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    VALIDATE_PIDL(pidl);
    if (pidl == NULL)
        return NULL;

    // Find the last one
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = _ILNext(pidlLast);
    }

    return (LPITEMIDLIST)pidlLast;
}


BOOL ILRemoveLastID(LPITEMIDLIST pidl)
{
    BOOL fRemoved = FALSE;

    if (pidl == NULL)
        return(FALSE);

    if (pidl->mkid.cb)
    {
        LPITEMIDLIST pidlLast = (LPITEMIDLIST)ILFindLastID(pidl);

        Assert(pidlLast->mkid.cb);
        Assert(_ILNext(pidlLast)->mkid.cb==0);

        // Remove the last one
        pidlLast->mkid.cb = 0; // null-terminator
        fRemoved = TRUE;
    }

    return fRemoved;
}

LPITEMIDLIST ILClone(LPCITEMIDLIST pidl)
{
    UINT cb = ILGetSize(pidl);
    LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);

    if (pidlRet)
    {
        // Notes: no need to zero-init.
        CopyMemory(pidlRet, pidl, cb);
    }
    return pidlRet;
}

LPITEMIDLIST ILCloneFirst(LPCITEMIDLIST pidl)
{
    UINT cb = pidl->mkid.cb+sizeof(pidl->mkid.cb);
    LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);
    if (pidlRet)
    {
        // Notes: no need to zero-init.
        CopyMemory(pidlRet, pidl, pidl->mkid.cb);
        _ILNext(pidlRet)->mkid.cb = 0;
    }

    return pidlRet;
}

BOOL ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IShellFolder *psfDesktop;

    if (FAILED(SHGetDesktopFolder(&psfDesktop)))
        return FALSE;

    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);
    return psfDesktop->lpVtbl->CompareIDs(psfDesktop, 0, pidl1, pidl2) == 0;
}

// test if
//      pidl1 is a parent of pidl2

BOOL ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate)
{
    LPITEMIDLIST pidl2Prefix;
    UINT cb;
    LPCITEMIDLIST pidl1T;
    LPCITEMIDLIST pidl2T;

    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);

    if (!pidl1 || !pidl2)
        return FALSE;

    for (pidl1T = pidl1, pidl2T = pidl2; !ILIsEmpty(pidl1T);
         pidl1T = _ILNext(pidl1T), pidl2T = _ILNext(pidl2T))
    {
        // if pidl2 is shorter than pidl1, pidl1 can't be its parent.
        if (ILIsEmpty(pidl2T))
            return FALSE;
    }

    if (fImmediate)
    {
        // If fImmediate is TRUE, pidl2T should contain exactly one ID.
        if (ILIsEmpty(pidl2T) || !ILIsEmpty(_ILNext(pidl2T)))
            return FALSE;
    }

    //
    // Create a new IDList from a portion of pidl2, which contains the
    // same number of IDs as pidl1.
    //
    cb = (UINT)pidl2T - (UINT)pidl2;
    pidl2Prefix = _ILCreate(cb + sizeof(pidl2->mkid.cb));
    if (pidl2Prefix)
    {
        BOOL fRet;

        CopyMemory(pidl2Prefix, pidl2, cb);

        Assert(ILGetSize(pidl2Prefix) == cb + sizeof(pidl2->mkid.cb));

        fRet = ILIsEqual(pidl1, pidl2Prefix);

        ILFree(pidl2Prefix);

        return fRet;
    }

    return FALSE;
}

// this returns a pointer to the child id ie:
// given pidlParent = \chicago\desktop
//      pidlChild = \chicago\desktop\foo\bar
// the return will point to the ID that represents \foo\bar
// NULL is returned if pidlParent is not a parent of pidlChild
LPITEMIDLIST ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
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

LPITEMIDLIST  ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST pidlNew;
    UINT cb1 = ILGetSize(pidl1) - sizeof(pidl1->mkid.cb);
    UINT cb2 = ILGetSize(pidl2);

    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);
    pidlNew = _ILCreate(cb1 + cb2);
    if (pidlNew)
    {
        CopyMemory(pidlNew, pidl1, cb1);
        CopyMemory(((LPSTR)pidlNew) + cb1, pidl2, cb2);
        Assert(ILGetSize(pidlNew) == cb1+cb2);
    }
    return pidlNew;
}

void ILFree(LPITEMIDLIST pidl)
{
    if (pidl)
    {
#ifdef DEBUG
        UINT cbSize = SHGetSize(pidl);
        VALIDATE_PIDL(pidl);

        // Fill the memory with some bad value...
        _memset(pidl, 0xE5, cbSize);

        // If large enough try to save the call return address of who
        // freed us in the 3-6 byte of the structure.
        if (cbSize >= 6)
            *((UINT*)((LPSTR)pidl + 2)) =  *(((UINT*)&pidl) - 1);

#endif
        SHFree(pidl);
    }
}

LPITEMIDLIST ILCreateFromPath(LPCSTR pszPath)
{
    LPITEMIDLIST pidl = NULL;
    IShellFolder *psfDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        ULONG cchEaten;
        WCHAR wszPath[MAX_PATH];

        StrToOleStrN(wszPath, ARRAYSIZE(wszPath), pszPath, -1);

        psfDesktop->lpVtbl->ParseDisplayName(psfDesktop, NULL, NULL, wszPath, &cchEaten, &pidl, NULL);
    }
    return pidl;
}

//===========================================================================
// IDLIST: Stream access
// BUGBUG: check bytes read on Read calls?
//===========================================================================

HRESULT ILLoadFromStream(LPSTREAM pstm, LPITEMIDLIST * ppidl)
{
    HRESULT hres;
    ULONG cb;

    Assert(ppidl);

    // Delete the old one if any.
    if (*ppidl)
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    // Read the size of the IDLIST
    cb = 0;             // WARNING: We need to fill its HIWORD!
    hres = pstm->lpVtbl->Read(pstm, &cb, sizeof(USHORT), NULL); // Yes, USHORT
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

HRESULT ILSaveToStream(LPSTREAM pstm, LPCITEMIDLIST pidl)
{
    HRESULT hres;
    ULONG cb = ILGetSize(pidl);
    Assert(HIWORD(cb) == 0);
    hres = pstm->lpVtbl->Write(pstm, &cb, sizeof(USHORT), NULL); // Yes, USHORT
    if (SUCCEEDED(hres) && cb)
    {
        if (SUCCEEDED(hres))
        {
            hres = pstm->lpVtbl->Write(pstm, pidl, cb, NULL);
        }
    }

    return hres;
}

#ifdef _HIDA

//===========================================================================
// IDLARRAY stuff
//===========================================================================

#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])


HIDA HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST * apidl)
{
    HIDA hida;
    UINT i;
    UINT offset = sizeof(CIDA) + sizeof(UINT)*cidl;
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
            CopyMemory(((LPBYTE)pida)+offset, pidlNext, cbSize);
            offset += cbSize;

            Assert(ILGetSize(HIDA_GetPIDLItem(pida,i-1)) == cbSize);

            if (i==cidl)
                break;
        }

        Assert(offset == cbTotal);
    }

    return hida;
}

HIDA HIDA_Create2(LPVOID pida, UINT cb)
{
    HIDA hida = GlobalAlloc(GPTR, cb);
    if (hida)
    {
        CopyMemory((LPIDA)hida, pida, cb);      // no need to lock
    }
    return hida;
}

HIDA HIDA_Clone(HIDA hida)
{
    UINT cbTotal = GlobalSize(hida);
    HIDA hidaCopy = GlobalAlloc(GPTR, cbTotal);
    if (hidaCopy)
    {
        LPIDA pida = (LPIDA)GlobalLock(hida);
        CopyMemory((LPIDA)hidaCopy, pida, cbTotal);     // no need to lock
        GlobalUnlock(hida);
    }
    return hidaCopy;
}

UINT HIDA_GetCount(HIDA hida)
{
    UINT count = 0;
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida) {
        count = pida->cidl;
        GlobalUnlock(hida);
    }
    return count;
}

UINT HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
        LPCITEMIDLIST pidlItem   = HIDA_GetPIDLItem(pida, i);
        UINT cbFolder  = ILGetSize(pidlFolder)-sizeof(USHORT);
        UINT cbItem = ILGetSize(pidlItem);
        if (cbMax < cbFolder+cbItem) {
            if (pidlOut) {
                pidlOut->mkid.cb = 0;
            }
        } else {
            CopyMemory(pidlOut, pidlFolder, cbFolder);
            CopyMemory(((LPBYTE)pidlOut)+cbFolder, pidlItem, cbItem);
        }
        GlobalUnlock(hida);

        return (cbFolder+cbItem);
    }
    return 0;
}

//
// This one reallocated pidl if necessary. NULL is valid to pass in as pidl.
//
LPITEMIDLIST HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl)
{
    UINT cbRequired = HIDA_GetIDList(hida, i, NULL, 0);
    pidl = _ILResize(pidl, cbRequired, 32); // extra 32-byte if we realloc
    if (pidl)
    {
        HIDA_GetIDList(hida, i, pidl, cbRequired);
    }

    return pidl;
}

LPCITEMIDLIST IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    if (i == (UINT)-1 || i < pida->cidl)
    {
        return HIDA_GetPIDLItem(pida, i);
    }

    return NULL;
}

LPITEMIDLIST IDA_ILClone(LPIDA pida, UINT i)
{
    if (i < pida->cidl)
        return ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, i));
    return NULL;
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

void HIDA_ReleaseStgMedium(LPIDA pida, STGMEDIUM *pmedium)
{
    if (pmedium->hGlobal && (pmedium->tymed==TYMED_HGLOBAL))
    {
#ifdef DEBUG
        if (pida)
        {
            LPIDA pidaT = (LPIDA)GlobalLock(pmedium->hGlobal);
            Assert(pidaT == pida);
            GlobalUnlock(pmedium->hGlobal);
        }
#endif
        GlobalUnlock(pmedium->hGlobal);
    }
    else
    {
        Assert(0);
    }

    SHReleaseStgMedium(pmedium);
}

#endif // _HIDA

BOOL StrRetToStrN(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl)
{
    switch (pStrRet->uType)
    {
    case STRRET_WSTR:
        OleStrToStrN(szOut, uszOut, pStrRet->pOleStr, -1);
        SHFree(pStrRet->pOleStr);
        break;

    case STRRET_CSTR:
        lstrcpyn(szOut, pStrRet->cStr, uszOut);
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            lstrcpyn(szOut, ((LPCSTR)&pidl->mkid)+pStrRet->uOffset, uszOut);
            break;
        }

        // Fall through
    default:
        if (uszOut)
            *szOut = '\0';
        return FALSE;
    }

    return TRUE;
}


#if 0
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
        if (ILIsEmpty(pidlRel2)) {
            hres = 0;
        } else {
            hres = (HRESULT)-1;
        }
    }
    else
    {
        if (ILIsEmpty(pidlRel2))
        {
            hres = 1;
        }
        else
        {
            //
            // Neither pidlRel1 nor pidlRel2 is empty.
            //  (1) Bind to the next level of the IShellFolder
            //  (2) Call its CompareIDs to let it compare the rest of IDs.
            //
            // Notes: We should create pidlNext not from pidl2 but from pidl1
            //  because fstreex.c may pass simple pidl2.
            //
            LPITEMIDLIST pidlNext = ILClone(pidl1);
            if (pidlNext)
            {
                IShellFolder *psfNext;
                _ILNext(pidlNext)->mkid.cb = 0;
                hres = psfParent->lpVtbl->BindToObject(psfParent, pidlNext, NULL,
                                &IID_IShellFolder, &psfNext);

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

void StrRetFormat(LPSTRRET pStrRet, LPCITEMIDLIST pidlRel, LPCSTR pszTemplate, LPCSTR pszAppend)
{
     LPSTR pszRet;
     char szT[MAX_PATH];

     StrRetToStrN(szT, sizeof(szT), pStrRet, pidlRel);
     pszRet = ShellConstructMessageString(HINST_THISDLL, pszTemplate, pszAppend, szT);
     if (pszRet)
     {
        pStrRet->uType = STRRET_CSTR;
        lstrcpyn(pStrRet->cStr, pszRet, sizeof(pStrRet->cStr));
        Free(pszRet);
     }
}

//
// Notes: This one passes SHGDN_FORMARSING to ISF::GetDisplayNameOf.
//
HRESULT ILGetRelDisplayName(IShellFolder *psf, LPSTRRET pStrRet,
                                   LPCITEMIDLIST pidlRel, LPCSTR pszName,
                                   LPCSTR pszTemplate)
{
    HRESULT hres;
    LPITEMIDLIST pidlLeft = ILCloneFirst(pidlRel);

    if (pidlLeft)
    {
        IShellFolder *psfNext;
        hres = psf->lpVtbl->BindToObject(psf, pidlLeft, NULL,
                    &IID_IShellFolder, &psfNext);
        if (SUCCEEDED(hres))
        {
            LPCITEMIDLIST pidlRight = _ILNext(pidlRel);
            hres = psfNext->lpVtbl->GetDisplayNameOf(psfNext, pidlRight, SHGDN_INFOLDER|SHGDN_FORPARSING, pStrRet);
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

LPITEMIDLIST ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend)
{
    UINT cbUsed, cbRequired;

    // Create the ID list, if it is not given.
    if (!pidl)
    {
        pidl = ILCreate();
        if (!pidl)
            return NULL;        // memory overflow
    }

    cbUsed = ILGetSize(pidl);
    cbRequired = cbUsed + pmkid->cb;

    pidl = _ILResize(pidl, cbRequired, CBIDL_INCL);
    if (!pidl)
        return NULL;    // memory overflow

    if (fAppend)
    {
        // Append it.
        CopyMemory(_ILSkip(pidl, cbUsed-sizeof(pidl->mkid.cb)), pmkid, pmkid->cb);
    }
    else
    {
        // Put it at the top
        MoveMemory(_ILSkip(pidl, pmkid->cb), pidl, cbUsed);
        CopyMemory(pidl, pmkid, pmkid->cb);

        Assert(ILGetSize(_ILNext(pidl))==cbUsed);
    }

    // We must put zero-terminator because of LMEM_ZEROINIT.
    _ILSkip(pidl, cbRequired-sizeof(pidl->mkid.cb))->mkid.cb = 0;
    Assert(ILGetSize(pidl) == cbRequired);

    return pidl;
}


BOOL ILGetDisplayName(LPCITEMIDLIST pidl, LPSTR pszPath)
{
    BOOL fSuccess = FALSE; // assume error
    STRRET srName;
    IShellFolder *psfDesktop;

    SHGetDesktopFolder(&psfDesktop);

    VALIDATE_PIDL(pidl);
    if (SUCCEEDED(psfDesktop->lpVtbl->GetDisplayNameOf(psfDesktop, pidl, SHGDN_FORPARSING, &srName)))
    {
        StrRetToStrN(pszPath, MAX_PATH, &srName, pidl);
        fSuccess = TRUE;
    }

    return fSuccess;
}



#endif // not used



