#include "proj.h"
#include <idhidden.h>
#include <regitemp.h>
#include <shstr.h>
#include <shlobjp.h>

// Alpha platform doesn't need unicode thunks, seems like this
// should happen automatically in the headerfiles...
//
#if defined(_X86_) || defined(UNIX)
#else
#define NO_W95WRAPS_UNITHUNK
#endif

#include "wininet.h"
#include "w95wraps.h"

//------------------------------------------------------------------------
// Random helpful functions
//------------------------------------------------------------------------
//


// pbIsNamed is true if the i-th item in hm is a named separator
STDAPI_(BOOL) _SHIsMenuSeparator2(HMENU hm, int i, BOOL *pbIsNamed)
{
    MENUITEMINFO mii;
    BOOL         bLocal;

    if (!pbIsNamed)
        pbIsNamed = &bLocal;
        
    *pbIsNamed = FALSE;

    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.cch = 0;    // WARNING: We MUST initialize it to 0!!!
    if (!GetMenuItemInfo(hm, i, TRUE, &mii))
    {
        return(FALSE);
    }

    if (mii.fType & MFT_SEPARATOR)
    {
        // NOTE that there is a bug in either 95 or NT user!!!
        // 95 returns 16 bit ID's and NT 32 bit therefore there is a
        // the following may fail, on win9x, to evaluate to false
        // without casting
        *pbIsNamed = ((WORD)mii.wID != (WORD)-1);
        return(TRUE);
    }

    return(FALSE);
}

STDAPI_(BOOL) _SHIsMenuSeparator(HMENU hm, int i)
{
    return _SHIsMenuSeparator2(hm, i, NULL);
}

//
// _SHPrettyMenu -- make this menu look darn purty
//
// Prune the separators in this hmenu to ensure there isn't one in the first or last
// position and there aren't any runs of >1 separator.
//
// Named separators take precedence over regular separators.
//
STDAPI_(void) _SHPrettyMenu(HMENU hm)
{
    BOOL bSeparated = TRUE;
    BOOL bIsNamed;
    BOOL bWasNamed = TRUE;
    int i;

    for (i = GetMenuItemCount(hm) - 1; i > 0; --i)
    {
        if (_SHIsMenuSeparator2(hm, i, &bIsNamed))
        {
            if (bSeparated)
            {
                // if we have two separators in a row, only one of which is named
                // remove the non named one!
                if (bIsNamed && !bWasNamed)
                {
                    DeleteMenu(hm, i+1, MF_BYPOSITION);
                    bWasNamed = bIsNamed;
                }
                else
                {
                    DeleteMenu(hm, i, MF_BYPOSITION);
                }
            }
            else
            {
                bWasNamed = bIsNamed;
                bSeparated = TRUE;
            }
        }
        else
        {
            bSeparated = FALSE;
        }
    }

    // The above loop does not handle the case of many separators at
    // the beginning of the menu
    while (_SHIsMenuSeparator2(hm, 0, NULL))
    {
        DeleteMenu(hm, 0, MF_BYPOSITION);
    }
}

STDAPI_(DWORD) SHIsButtonObscured(HWND hwnd, PRECT prc, INT_PTR i)
{
    ASSERT(IsWindow(hwnd));
    ASSERT(i < SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0));

    DWORD dwEdge = 0;

    RECT rc, rcInt;
    SendMessage(hwnd, TB_GETITEMRECT, i, (LPARAM)&rc);

    if (!IntersectRect(&rcInt, prc, &rc))
    {
        dwEdge = EDGE_LEFT | EDGE_RIGHT | EDGE_TOP | EDGE_BOTTOM;
    }
    else
    {
        if (rc.top != rcInt.top)
            dwEdge |= EDGE_TOP;

        if (rc.bottom != rcInt.bottom)
            dwEdge |= EDGE_BOTTOM;

        if (rc.left != rcInt.left)
            dwEdge |= EDGE_LEFT;

        if (rc.right != rcInt.right)
            dwEdge |= EDGE_RIGHT;
    }

    return dwEdge;
}

STDAPI_(BYTE) SHBtnStateFromRestriction(DWORD dwRest, BYTE fsState)
{
    if (dwRest == RESTOPT_BTN_STATE_VISIBLE)
        return (fsState & ~TBSTATE_HIDDEN);
    else if (dwRest == RESTOPT_BTN_STATE_HIDDEN)
        return (fsState | TBSTATE_HIDDEN);
    else {
#ifdef DEBUG
        if (dwRest != RESTOPT_BTN_STATE_DEFAULT)
            TraceMsg(TF_ERROR, "bad toolbar button state policy %x", dwRest);
#endif
        return fsState;
    }
}

//
// SHIsDisplayable
//
// Figure out if this unicode string can be displayed by the system
// (i.e., won't be turned into a string of question marks).
//
STDAPI_(BOOL) SHIsDisplayable(LPCWSTR pwszName, BOOL fRunOnFE, BOOL fRunOnNT5)
{
    BOOL fNotDisplayable = FALSE;

    if (pwszName)
    {
        if (!fRunOnNT5)
        {
            // if WCtoMB has to use default characters in mapping pwszName to multibyte,
            // it sets fNotDisplayable == TRUE, in which case we have to use something
            // else for the title string.
            WideCharToMultiByte(CP_ACP, 0, pwszName, -1, NULL, 0, NULL, &fNotDisplayable);
            if (fNotDisplayable)
            {
                if (fRunOnFE)
                {
                    WCHAR wzName[INTERNET_MAX_URL_LENGTH];

                    BOOL fReplaceNbsp = FALSE;

                    StrCpyNW(wzName, pwszName, ARRAYSIZE(wzName));
                    for (int i = 0; i < ARRAYSIZE(wzName); i++)
                    {
                        if (0x00A0 == wzName[i])    // if &nbsp
                        {
                            wzName[i] = 0x0020;     // replace to space
                            fReplaceNbsp = TRUE;
                        }
                        else if (0 == wzName[i])
                            break;
                    }
                    if (fReplaceNbsp)
                    {
                        pwszName = wzName;
                        WideCharToMultiByte(CP_ACP, 0, pwszName, -1, NULL, 0, NULL, &fNotDisplayable);
                    }
                }
            }
        }
    }

    return !fNotDisplayable;
}

// Trident will take URLs that don't indicate their source of
// origin (about:, javascript:, & vbscript:) and will append
// an URL turd and then the source URL.  The turd will indicate
// where the source URL begins and that source URL is needed
// when the action needs to be Zone Checked.
//
// This function will remove that URL turd and everything behind
// it so the URL is presentable for the user.

#define URL_TURD        ((TCHAR)0x01)

STDAPI_(void) SHRemoveURLTurd(LPTSTR pszUrl)
{
    if (!pszUrl)
        return;

    while (TEXT('\0') != pszUrl[0])
    {
        if (URL_TURD == pszUrl[0])
        {
            pszUrl[0] = TEXT('\0');
            break;
        }

        pszUrl = CharNext(pszUrl);
    }
}

STDAPI_(BOOL) SetWindowZorder(HWND hwnd, HWND hwndInsertAfter)
{
    return SetWindowPos(hwnd, hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

BOOL CALLBACK _FixZorderEnumProc(HWND hwnd, LPARAM lParam)
{
    HWND hwndTest = (HWND)lParam;
    HWND hwndOwner = hwnd;

    while (hwndOwner = GetWindow(hwndOwner, GW_OWNER))
    {
        if (hwndOwner == hwndTest)
        {
            TraceMsg(TF_WARNING, "_FixZorderEnumProc: Found topmost window %x owned by non-topmost window %x, fixing...", hwnd, hwndTest);
            SetWindowZorder(hwnd, HWND_NOTOPMOST);
#ifdef DEBUG
            if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
                TraceMsg(TF_ERROR, "_FixZorderEnumProc: window %x is still topmost", hwnd);
#endif
            break;
        }
    }

    return TRUE;
}

STDAPI_(BOOL) SHForceWindowZorder(HWND hwnd, HWND hwndInsertAfter)
{
    BOOL fRet = SetWindowZorder(hwnd, hwndInsertAfter);

    if (fRet && hwndInsertAfter == HWND_TOPMOST)
    {
        if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
        {
            //
            // Dammit, user didn't actually move the hwnd to topmost
            //
            // According to GerardoB, this can happen if the window has
            // an owned window that somehow has become topmost while the 
            // owner remains non-topmost, i.e., the two have become
            // separated in the z-order.  In this state, when the owner
            // window tries to make itself topmost, the call will
            // silently fail.
            //
            // TERRIBLE HORRIBLE NO GOOD VERY BAD HACK
            //
            // Hacky fix is to enumerate the toplevel windows, check to see
            // if any are topmost and owned by hwnd, and if so, make them
            // non-topmost.  Then, retry the SetWindowPos call.
            //

            TraceMsg(TF_WARNING, "SHForceWindowZorder: SetWindowPos(%x, HWND_TOPMOST) failed", hwnd);

            // Fix up the z-order
            EnumWindows(_FixZorderEnumProc, (LPARAM)hwnd);

            // Retry the set.  (This should make all owned windows topmost as well.)
            SetWindowZorder(hwnd, HWND_TOPMOST);

#ifdef DEBUG
            if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
                TraceMsg(TF_ERROR, "SHForceWindowZorder: window %x is still not topmost", hwnd);
#endif
        }
    }

    return fRet;
}

STDAPI_(LPITEMIDLIST) ILCloneParent(LPCITEMIDLIST pidl)
{   
    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent)
        ILRemoveLastID(pidlParent);

    return pidlParent;
}

// in:
//      psf     OPTIONAL, if NULL assume psfDesktop
//      pidl    to bind to from psfParent
//

STDAPI SHBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut)
{
    HRESULT hr;
    IShellFolder *psfRelease;

    if (!psf)
    {
        SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }
    else
    {
        psfRelease = NULL;
    }

    if (!pidl || ILIsEmpty(pidl))
    {
        hr = psf->QueryInterface(riid, ppvOut);
    }
    else
    {
        hr = psf->BindToObject(pidl, NULL, riid, ppvOut);
    }

    if (psfRelease)
    {
        psfRelease->Release();
    }

    if (SUCCEEDED(hr) && (*ppvOut == NULL))
    {
        // Some lame shell extensions (eg WS_FTP) will return success and a null out pointer
        TraceMsg(TF_WARNING, "SHBindToObject: BindToObject succeeded but returned null ppvOut!!");
        hr = E_FAIL;
    }

    return hr;
}

// psfRoot is the base of the bind.  If NULL, then we use the shell desktop.
// If you want to bind relative to the explorer root (e.g., CabView, MSN),
// then use SHBindToIDListParent.
STDAPI SHBindToFolderIDListParent(IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hres;

    ASSERT(!ILIsRooted(pidl));
    
    // Old shell32 code in some cases simply whacked the pidl,
    // but this is unsafe.  Do what shdocvw does and clone/remove:
    //
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent) 
    {
        hres = SHBindToObject(psfRoot, riid, pidlParent, ppv);
        ILFree(pidlParent);
    }
    else
        hres = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hres;
}

//
//  Warning!  brutil.cpp overrides this function
//
STDAPI SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    return SHBindToFolderIDListParent(NULL, pidl, riid, ppv, ppidlLast);
}



STDAPI SHGetIDListFromUnk(IUnknown *punk, LPITEMIDLIST *ppidl)
{
    HRESULT hres = E_NOINTERFACE;

    *ppidl = NULL;

    IPersistFolder2 *ppf;
    if (punk && SUCCEEDED(punk->QueryInterface(IID_IPersistFolder2, (void **)&ppf)))
    {
        hres = ppf->GetCurFolder(ppidl);
        ppf->Release();
    }
    return hres;
}

//
//  _ILResize isnt exported from shell32 so we make a cheap one that
//  do any preallocating
//
STDAPI_(LPITEMIDLIST) _MyILResize(LPITEMIDLIST pidl, UINT cbRequired)
{
    RIP(pidl);
    LPITEMIDLIST pidlNew = (LPITEMIDLIST)SHAlloc(cbRequired);
    if (pidlNew)
        MoveMemory(pidlNew, pidl, ILGetSize(pidl));

    SHFree(pidl);
    return pidlNew;
}

//  the last word of the pidl is where we store the hidden offset
#define _ILHiddenOffset(pidl)   (*((WORD UNALIGNED *)(((BYTE *)_ILNext(pidl)) - SIZEOF(WORD))))
#define _ILSetHiddenOffset(pidl, cb)   ((*((WORD UNALIGNED *)(((BYTE *)_ILNext(pidl)) - SIZEOF(WORD)))) = (WORD)cb)
#define _ILIsHidden(pidhid)     (HIWORD(pidhid->id) == HIWORD(IDLHID_EMPTY))

STDAPI_(PCIDHIDDEN) _ILNextHidden(PCIDHIDDEN pidhid, LPCITEMIDLIST pidlLimit)
{
    PCIDHIDDEN pidhidNext = (PCIDHIDDEN) _ILNext((LPCITEMIDLIST)pidhid);

    if ((BYTE *)pidhidNext < (BYTE *)pidlLimit && _ILIsHidden(pidhidNext))
    {
        return pidhidNext;
    }

    //  if we ever go past the limit,
    //  then this is not really a hidden id
    //  or we have screwed up on some calculation.
    ASSERT((BYTE *)pidhidNext == (BYTE *)pidlLimit);
    return NULL;
}

STDAPI_(PCIDHIDDEN) _ILFirstHidden(LPCITEMIDLIST pidl)
{
    WORD cbHidden = _ILHiddenOffset(pidl);

    if (cbHidden && cbHidden + SIZEOF(HIDDENITEMID) < pidl->mkid.cb)
    {
        //  this means it points to someplace inside the pidl
        //  maybe this has hidden ids
        PCIDHIDDEN pidhid = (PCIDHIDDEN) (((BYTE *)pidl) + cbHidden);

        if (_ILIsHidden(pidhid)
        && (pidhid->cb + cbHidden <= pidl->mkid.cb))
        {
            //  this is more than likely a hidden id
            //  we could walk the chain and verify
            //  that it adds up right...
            return pidhid;
        }
    }

    return NULL;
}
    
//
//  HIDDEN ids are sneakily hidden in the last ID in a pidl.
//  we append our data without changing the existing pidl,
//  (except it is now bigger)  this works because the pidls
//  that we will apply this to are flexible in handling different
//  sized pidls.  specifically this is used in FS pidls.
// 
//  WARNING - it is the callers responsibility to use hidden IDs
//  only on pidls that can handle it.  most shell pidls, and 
//  specifically FS pidls have no problem with this.  however
//  some shell extensions might have fixed length ids, 
//  which makes these unadvisable to append to everything.
//  possibly add an SFGAO_ bit to allow hidden, otherwise key
//  off FILESYSTEM bit.
//

STDAPI_(LPITEMIDLIST) ILAppendHiddenID(LPITEMIDLIST pidl, PCIDHIDDEN pidhid)
{
    //
    // BUGBUG - we dont handle collisions of multiple hidden ids
    //          maybe remove IDs of the same IDLHID?
    //
    // BUGBUG - we dont collapse IDLHID_EMPTY
    //          we should probably see about reusing IDLHID_EMPTY ids
    //
    
    //  we require a pidl to attach the hidden id to
    RIP(pidl);
    if (!ILIsEmpty(pidl))
    {
        UINT cbUsed = ILGetSize(pidl);
        UINT cbRequired = cbUsed + pidhid->cb + SIZEOF(pidhid->cb);

        pidl = _MyILResize(pidl, cbRequired);
        
        if (pidl)
        {
            LPITEMIDLIST pidlLast = ILFindLastID(pidl);
            WORD cbHidden = _ILFirstHidden(pidlLast) ? _ILHiddenOffset(pidlLast) : pidlLast->mkid.cb;
            PIDHIDDEN pidhidCopy = (PIDHIDDEN)_ILSkip(pidl, cbUsed - SIZEOF(pidl->mkid.cb));

            // Append it, overwriting the terminator
            MoveMemory(pidhidCopy, pidhid, pidhid->cb);

            //  grow the copy to allow the hidden offset.
            pidhidCopy->cb += SIZEOF(pidhid->cb);

            //  now we need to readjust pidlLast to encompass 
            //  the hidden bits and the hidden offset.
            pidlLast->mkid.cb += pidhidCopy->cb;

            //  set the hidden offset so that we can find our hidden IDs later
            _ILSetHiddenOffset((LPITEMIDLIST)pidhidCopy, cbHidden);

            // We must put zero-terminator because of LMEM_ZEROINIT.
            _ILSkip(pidl, cbRequired-SIZEOF(pidl->mkid.cb))->mkid.cb = 0;
            ASSERT(ILGetSize(pidl) == cbRequired);
        }
    }
    return pidl;
}

STDAPI_(PCIDHIDDEN) ILFindHiddenID(LPCITEMIDLIST pidl, IDLHID id)
{
    RIP(pidl);
    if (!ILIsEmpty(pidl))
    {
        pidl = ILFindLastID(pidl);
        
        PCIDHIDDEN pidhid = _ILFirstHidden(pidl);

        //  reuse pidl to become the limit.
        //  so that we cant ever walk out of 
        //  the pidl.
        pidl = _ILNext(pidl);

        while (pidhid)
        {
            if (pidhid->id == id)
                break;

            pidhid = _ILNextHidden(pidhid, pidl);
        }
        return pidhid;
    }

    return NULL;
}

STDAPI_(BOOL) ILRemoveHiddenID(LPITEMIDLIST pidl, IDLHID id)
{
    PIDHIDDEN pidhid = (PIDHIDDEN)ILFindHiddenID(pidl, id);

    if (pidhid)
    {
        pidhid->id = IDLHID_EMPTY;
        return TRUE;
    }
    return FALSE;
}

//
//  generically useful to hide.
//
typedef struct _HIDDENCLSID
{
    WORD    cb;
    IDLHID  id;
    CLSID   clsid;
} HIDDENCLSID;

typedef UNALIGNED HIDDENCLSID *PHIDDENCLSID;
typedef const UNALIGNED HIDDENCLSID *PCHIDDENCLSID;

STDAPI_(LPITEMIDLIST) ILAppendHiddenClsid(LPITEMIDLIST pidl, IDLHID id, CLSID *pclsid)
{
    HIDDENCLSID hc = {SIZEOF(hc), id};
    hc.clsid = *pclsid;

    return ILAppendHiddenID(pidl, (PIDHIDDEN)&hc);
}

STDAPI_(BOOL) ILGetHiddenClsid(LPCITEMIDLIST pidl, IDLHID id, CLSID *pclsid)
{
    PCHIDDENCLSID phc = (PCHIDDENCLSID) ILFindHiddenID(pidl, id);

    if (phc)
    {
        *pclsid = phc->clsid;
        return TRUE;
    }
    return FALSE;
}


typedef struct _HIDDENSTRINGA
{
    WORD    cb;
    IDLHID  id;
    WORD    type;
    CHAR    sz[1];   //  variable length string
} HIDDENSTRINGA;

typedef UNALIGNED HIDDENSTRINGA *PHIDDENSTRINGA;
typedef const UNALIGNED HIDDENSTRINGA *PCHIDDENSTRINGA;

typedef struct _HIDDENSTRINGW
{
    WORD    cb;
    IDLHID  id;
    WORD    type;
    WCHAR   sz[1];   //  canonical name to be passed to ISTRING
} HIDDENSTRINGW;

typedef UNALIGNED HIDDENSTRINGW *PHIDDENSTRINGW;
typedef const UNALIGNED HIDDENSTRINGW *PCHIDDENSTRINGW;

#define HIDSTRTYPE_ANSI        0x0001
#define HIDSTRTYPE_WIDE        0x0002

STDAPI_(LPITEMIDLIST) ILAppendHiddenStringW(LPITEMIDLIST pidl, IDLHID id, LPCWSTR psz)
{
    //  terminator is included in the ID definition
    USHORT cb = (USHORT)SIZEOF(HIDDENSTRINGW) + CbFromCchW(lstrlenW(psz));
    
    PHIDDENSTRINGW phs = (PHIDDENSTRINGW) LocalAlloc(LPTR, cb);

    if (phs)
    {
        phs->cb = cb;
        phs->id = id;
        phs->type = HIDSTRTYPE_WIDE;
        StrCpyW(phs->sz, psz);

        pidl = ILAppendHiddenID(pidl, (PIDHIDDEN)phs);
        LocalFree(phs);
        return pidl;
    }
    return NULL;
}
    
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringA(LPITEMIDLIST pidl, IDLHID id, LPCSTR psz)
{
    //  terminator is included in the ID definition
    USHORT cb = (USHORT)SIZEOF(HIDDENSTRINGA) + CbFromCchA(lstrlenA(psz));
    
    PHIDDENSTRINGA phs = (PHIDDENSTRINGA) LocalAlloc(LPTR, cb);

    if (phs)
    {
        phs->cb = cb;
        phs->id = id;
        phs->type = HIDSTRTYPE_ANSI;
        StrCpyA(phs->sz, psz);

        pidl = ILAppendHiddenID(pidl, (PIDHIDDEN)phs);
        LocalFree(phs);
        return pidl;
    }
    return NULL;
}

STDAPI_(void *) _MemDupe(const UNALIGNED void *pv, DWORD cb)
{
    void *pvRet = LocalAlloc(LPTR, cb);
    if (pvRet)
    {
        CopyMemory(pvRet, pv, cb);
    }

    return pvRet;
}

STDAPI_(BOOL) ILGetHiddenStringW(LPCITEMIDLIST pidl, IDLHID id, LPWSTR psz, DWORD cch)
{
    PCHIDDENSTRINGW phs = (PCHIDDENSTRINGW) ILFindHiddenID(pidl, id);

    RIP(psz);
    if (phs)
    {
        if (phs->type == HIDSTRTYPE_WIDE)
        {
            ualstrcpynW(psz, phs->sz, cch);
            return TRUE;
        }
        else 
        {
            ASSERT(phs->type == HIDSTRTYPE_ANSI);
            SHAnsiToUnicode((LPSTR)phs->sz, psz, cch);
            return TRUE;
        }
    }
    return FALSE;
}
        
STDAPI_(BOOL) ILGetHiddenStringA(LPCITEMIDLIST pidl, IDLHID id, LPSTR psz, DWORD cch)
{
    PCHIDDENSTRINGW phs = (PCHIDDENSTRINGW) ILFindHiddenID(pidl, id);

    RIP(psz);
    if (phs)
    {
        if (phs->type == HIDSTRTYPE_ANSI)
        {
            ualstrcpynA(psz, (LPSTR)phs->sz, cch);
            return TRUE;
        }
        else 
        {
            ASSERT(phs->type == HIDSTRTYPE_WIDE);
            //  we need to handle the unalignment here...
            LPWSTR pszT = (LPWSTR) _MemDupe(phs->sz, CbFromCch(lstrlenW(phs->sz) +1));

            if (pszT)
            {
                SHUnicodeToAnsi(pszT, psz, cch);
                LocalFree(pszT);
                return TRUE;
            }
        }
    }
    return FALSE;
}

STDAPI_(int) ILCompareHiddenString(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, IDLHID id)
{

    // if there are fragments in here, then they might
    // differentiate the two pidls
    PCHIDDENSTRINGW ps1 = (PCHIDDENSTRINGW)ILFindHiddenID(pidl1, id);
    PCHIDDENSTRINGW ps2 = (PCHIDDENSTRINGW)ILFindHiddenID(pidl2, id);

    if (ps1 && ps2)
    {
        if (ps1->type == ps2->type)
        {
            if (ps1->type == HIDSTRTYPE_WIDE)
                return ualstrcmpW(ps1->sz, ps2->sz);

            ASSERT(ps1->type == HIDSTRTYPE_ANSI);

            return lstrcmpA((LPCSTR)ps1->sz, (LPCSTR)ps2->sz);
        }
        else
        {
            SHSTRW str;

            if (ps1->type == HIDSTRTYPE_ANSI)
            {
                str.SetStr((LPCSTR)ps1->sz);
                return ualstrcmpW(str, ps2->sz);
            }
            else
            {
                ASSERT(ps2->type == HIDSTRTYPE_ANSI);
                str.SetStr((LPCSTR)ps2->sz);
                return ualstrcmpW(ps1->sz, str);
            }
        }
    }

    if (ps1)
        return 1;
    if (ps2)
        return -1;
    return 0;
}

// {9EAC43C0-53EC-11CE-8230-CA8A32CF5494}
//static const GUID GUID_WINAMP = 
//{ 0x9eac43c0, 0x53ec, 0x11ce, { 0x82, 0x30, 0xca, 0x8a, 0x32, 0xcf, 0x54, 0x94} };

// {E9779583-939D-11CE-8A77-444553540000}
static const GUID GUID_AECOZIPARCHIVE = 
{ 0xE9779583, 0x939D, 0x11ce, { 0x8a, 0x77, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
// {49707377-6974-6368-2E4A-756E6F644A01}
static const GUID CLSID_WS_FTP_PRO_EXPLORER =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x01} };
// {49707377-6974-6368-2E4A-756E6F644A0A}
static const GUID CLSID_WS_FTP_PRO =
{ 0x49707377, 0x6974, 0x6368, {0x2E, 0x4A,0x75, 0x6E, 0x6F, 0x64, 0x4A, 0x0A} };
// {2bbbb600-3f0a-11d1-8aeb-00c04fd28d85}
static const GUID CLSID_KODAK_DC260_ZOOM_CAMERA =
{ 0x2bbbb600, 0x3f0a, 0x11d1, {0x8a, 0xeb, 0x00, 0xc0, 0x4f, 0xd2, 0x8d, 0x85} };
// {00F43EE0-EB46-11D1-8443-444553540000}
static const GUID GUID_MACINDOS =
{ 0x00F43EE0, 0xEB46, 0x11D1, { 0x84, 0x43, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
static const GUID CLSID_EasyZIP = 
{ 0xD1069700, 0x932E, 0x11cf, { 0xAB, 0x59, 0x00, 0x60, 0x8C, 0xBF, 0x2C, 0xE0} };

static const GUID CLSID_PAGISPRO_FOLDER =
{ 0x7877C8E0, 0x8B13, 0x11D0, { 0x92, 0xC2, 0x00, 0xAA, 0x00, 0x4B, 0x25, 0x6F} };
// {61E285C0-DCF4-11cf-9FF4-444553540000}
static const GUID CLSID_FILENET_IDMDS_NEIGHBORHOOD =
{ 0x61e285c0, 0xdcf4, 0x11cf, { 0x9f, 0xf4, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_PGP50_CONTEXTMENU =  //{969223C0-26AA-11D0-90EE-444553540000}
{ 0x969223C0, 0x26AA, 0x11D0, { 0x90, 0xEE, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

static const GUID CLSID_QUICKFINDER_CONTEXTMENU = //  {CD949A20-BDC8-11CE-8919-00608C39D066}
{ 0xCD949A20, 0xBDC8, 0x11CE, { 0x89, 0x19, 0x00, 0x60, 0x8C, 0x39, 0xD0, 0x66} };

static const GUID CLSID_HERCULES_HCTNT_V1001 = // {921BD320-8CB5-11CF-84CF-885835D9DC01}
{ 0x921BD320, 0x8CB5, 0x11CF, { 0x84, 0xCF, 0x88, 0x58, 0x35, 0xD9, 0xDC, 0x01} };

//
// NOTICE - DONT ADD ANYMORE HARDCODED CLSIDS
// add them to the ShellCompatibility key.  register in the client DLL
//

typedef struct {
    DWORD flag;
    LPCTSTR psz;
} FLAGMAP;

DWORD _GetMappedFlags(HKEY hk, const FLAGMAP *pmaps, DWORD cmaps)
{
    DWORD dwRet = 0;
    for (DWORD i = 0; i < cmaps; i++)
    {
        if (NOERROR == SHGetValue(hk, NULL, pmaps[i].psz, NULL, NULL, NULL))
            dwRet |= pmaps[i].flag;
    }

    return dwRet;
}

#define OCFMAPPING(ocf)     {OBJCOMPATF_##ocf, TEXT(#ocf)}

DWORD _GetRegistryCompatFlags(REFGUID clsid)
{
    DWORD dwRet = 0;
    TCHAR szGuid[GUIDSTR_MAX];
    TCHAR sz[MAX_PATH];
    HKEY hk;

    SHStringFromGUID(clsid, szGuid, ARRAYSIZE(szGuid));
    wnsprintf(sz, ARRAYSIZE(sz), TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Objects\\%s"), szGuid);
    
    if (NOERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hk))
    {   
        static const FLAGMAP rgOcfMaps[] = {
            OCFMAPPING(OTNEEDSSFCACHE),
            OCFMAPPING(NO_WEBVIEW),
            OCFMAPPING(UNBINDABLE),
            OCFMAPPING(PINDLL),
            OCFMAPPING(NEEDSFILESYSANCESTOR),
            OCFMAPPING(NOTAFILESYSTEM),
            OCFMAPPING(CTXMENU_NOVERBS),
            OCFMAPPING(CTXMENU_LIMITEDQI),
            OCFMAPPING(COCREATESHELLFOLDERONLY),
            };

        dwRet = _GetMappedFlags(hk, rgOcfMaps, ARRAYSIZE(rgOcfMaps));
        RegCloseKey(hk);
    }

    return dwRet;
}

typedef struct _CLSIDCOMPAT
{
    const GUID *pclsid;
    //  LPCTSTR pszVersion;
    OBJCOMPATFLAGS flags;
}CLSIDCOMPAT, *PCLSIDCOMPAT;

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlags(IUnknown *punk, const CLSID *pclsid)
{
    HRESULT hr = E_INVALIDARG;
    OBJCOMPATFLAGS ocf = 0;
    CLSID clsid;
    if (punk)
        hr = IUnknown_GetClassID(punk, &clsid);
    else if (pclsid)
    {
        clsid = *pclsid;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        static const CLSIDCOMPAT s_rgCompat[] =
        {
            {&CLSID_WS_FTP_PRO_EXPLORER,
                OBJCOMPATF_OTNEEDSSFCACHE},
            {&CLSID_WS_FTP_PRO,
                OBJCOMPATF_UNBINDABLE},
            {&GUID_AECOZIPARCHIVE,
                OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_KODAK_DC260_ZOOM_CAMERA,
                OBJCOMPATF_OTNEEDSSFCACHE | OBJCOMPATF_PINDLL},
            {&GUID_MACINDOS,
                OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_EasyZIP,            
                OBJCOMPATF_NO_WEBVIEW},
            {&CLSID_PAGISPRO_FOLDER,
                OBJCOMPATF_NEEDSFILESYSANCESTOR},
            {&CLSID_FILENET_IDMDS_NEIGHBORHOOD,
                OBJCOMPATF_NOTAFILESYSTEM},
            {&CLSID_PGP50_CONTEXTMENU,
                OBJCOMPATF_CTXMENU_LIMITEDQI},
            {&CLSID_QUICKFINDER_CONTEXTMENU,
                OBJCOMPATF_CTXMENU_NOVERBS},
            {&CLSID_HERCULES_HCTNT_V1001,
                OBJCOMPATF_PINDLL},
            {NULL, 0}
        };

        for (int i = 0; s_rgCompat[i].pclsid; i++)
        {
            if (IsEqualGUID(clsid, *(s_rgCompat[i].pclsid)))
            {
                //  we could check version based
                //  on what is in under HKCR\CLSID\{clsid}
                ocf = s_rgCompat[i].flags;
                break;
            }
        }

        ocf |= _GetRegistryCompatFlags(clsid);

    }

    return ocf;
}

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlagsFromIDList(LPCITEMIDLIST pidl)
{
    OBJCOMPATFLAGS ocf = 0;
    CLSID clsid;

    //  APPCOMPAT: FileNet IDMDS (Panagon)'s shell folder extension returns
    //  E_NOTIMPL for IPersistFolder::GetClassID, so to detect the application,
    //  we have to crack the pidl.  (B#359464: tracysh)

    if (!ILIsEmpty(pidl)
    && pidl->mkid.cb >= SIZEOF(IDREGITEM)
    && pidl->mkid.abID[0] == SHID_ROOT_REGITEM)
    {
        clsid = ((LPCIDLREGITEM)pidl)->idri.clsid;
        ocf = SHGetObjectCompatFlags(NULL, &clsid);
    }

    return ocf;
}

LPCIDREGITEM _IsRooted(LPCITEMIDLIST pidl)
{
    LPCIDREGITEM pidlr = (LPCIDREGITEM)pidl;
    if (!ILIsEmpty(pidl)
    && pidlr->cb > SIZEOF(IDREGITEM)
    && pidlr->bFlags == SHID_ROOTEDREGITEM)
        return pidlr;

    return NULL;
}

STDAPI_(BOOL) ILIsRooted(LPCITEMIDLIST pidl)
{
    return (NULL != _IsRooted(pidl));
}

#define _ROOTEDPIDL(pidlr)      (LPITEMIDLIST)(((LPBYTE)pidlr)+SIZEOF(IDREGITEM))

STDAPI_(LPCITEMIDLIST) ILRootedFindIDList(LPCITEMIDLIST pidl)
{
    LPCIDREGITEM pidlr = _IsRooted(pidl);

    if (pidlr && pidlr->cb > SIZEOF(IDREGITEM))
    {
        //  then we have a rooted IDList in there
        return _ROOTEDPIDL(pidlr);
    }

    return NULL;
}

STDAPI_(BOOL) ILRootedGetClsid(LPCITEMIDLIST pidl, CLSID *pclsid)
{
    LPCIDREGITEM pidlr = _IsRooted(pidl);

    if (pidlr)
    {
        *pclsid = pidlr->clsid;
        return TRUE;
    }

    return FALSE;
}

STDAPI_(LPITEMIDLIST) ILRootedCreateIDList(CLSID *pclsid, LPCITEMIDLIST pidl)
{
    UINT cbPidl = ILGetSize(pidl);
    UINT cbTotal = SIZEOF(IDREGITEM) + cbPidl;

    LPIDREGITEM pidlr = (LPIDREGITEM) SHAlloc(cbTotal + SIZEOF(WORD));

    if (pidlr)
    {
        pidlr->cb = (WORD)cbTotal;

        pidlr->bFlags = SHID_ROOTEDREGITEM;
        pidlr->bOrder = 0;              // Nobody uses this (yet)

        if (pclsid)
            pidlr->clsid = *pclsid;
        else
            pidlr->clsid = CLSID_ShellDesktop;

        MoveMemory(_ROOTEDPIDL(pidlr), pidl, cbPidl);

        //  terminate
        _ILNext((LPITEMIDLIST)pidlr)->mkid.cb = 0;
    }

    return (LPITEMIDLIST) pidlr;
}

int CompareGUID(REFGUID guid1, REFGUID guid2)
{
    TCHAR sz1[GUIDSTR_MAX];
    TCHAR sz2[GUIDSTR_MAX];

    SHStringFromGUIDW(guid1, sz1, SIZECHARS(sz1));
    SHStringFromGUIDW(guid2, sz2, SIZECHARS(sz2));

    return lstrcmp(sz1, sz2);
}

STDAPI_(int) ILRootedCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet;
    LPCIDREGITEM pidlr1 = _IsRooted(pidl1);
    LPCIDREGITEM pidlr2 = _IsRooted(pidl2);

    if (pidlr1 && pidlr2)
    {
        CLSID clsid1 = pidlr1->clsid;
        CLSID clsid2 = pidlr2->clsid;

        iRet = CompareGUID(clsid1, clsid2);
        if (0 == iRet)
        {
            if (!ILIsEqual(_ROOTEDPIDL(pidl1), _ROOTEDPIDL(pidl2)))
            {
                IShellFolder *psfDesktop;
                if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
                {
                    HRESULT hr = psfDesktop->CompareIDs(0, pidl1, pidl2);
                    psfDesktop->Release();
                    iRet = ShortFromResult(hr);
                }
            }
        }
    }
    else if (pidlr1)
    {
        iRet = -1;
    }
    else if (pidlr2)
    {
        iRet = 1;
    }
    else
    {
        //  if neither are rootes, then they share the desktop
        //  as the same root...
        iRet = 0;
    }

    return iRet;
}

LPITEMIDLIST ILRootedTranslate(LPCITEMIDLIST pidlRooted, LPCITEMIDLIST pidlTrans)
{
    LPCITEMIDLIST pidlChild = ILFindChild(ILRootedFindIDList(pidlRooted), pidlTrans);

    if (pidlChild)
    {
        LPITEMIDLIST pidlRoot = ILCloneFirst(pidlRooted);

        if (pidlRoot)
        {
            LPITEMIDLIST pidlRet = ILCombine(pidlRoot, pidlChild);
            ILFree(pidlRoot);
            return pidlRet;
        }
    }
    return NULL;
}

#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])
#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])

STDAPI_(LPITEMIDLIST) IDA_ILClone(LPIDA pida, UINT i)
{
    if (i < pida->cidl)
        return ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, i));
    return NULL;
}

STDAPI_(void) EnableOKButtonFromString(HWND hDlg, LPTSTR pszText)
{
    BOOL bNonEmpty;
    
    PathRemoveBlanks(pszText);   // REVIEW, should we not remove from the end of
    bNonEmpty = lstrlen(pszText); // Not a BOOL, but okay

    EnableWindow(GetDlgItem(hDlg, IDOK), bNonEmpty);
    if (bNonEmpty)
    {
        SendMessage(hDlg, DM_SETDEFID, IDOK, 0L);
    }
}

STDAPI_(void) EnableOKButtonFromID(HWND hDlg, int id)
{
    TCHAR szText[MAX_PATH];

    if (!GetDlgItemText(hDlg, id, szText, ARRAYSIZE(szText)))
    {
        szText[0] = 0;
    }

    EnableOKButtonFromString(hDlg, szText);
}

//
//  C-callable versions of the ATL string conversion functions.
//

STDAPI_(LPWSTR) SHA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    ASSERT(lpa != NULL);
    ASSERT(lpw != NULL);
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

STDAPI_(LPSTR) SHW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    ASSERT(lpw != NULL);
    ASSERT(lpa != NULL);
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

//
//  Helper functions for SHChangeMenuAsIDList
//
//  See comment in declaration of SHChangeMenuAsIDList for caveats about
//  the pSender member.
//
//  This is tricky because IE 5.0 shipped with a Win64-unfriendly version
//  of this notification, so we have to sniff the structure and see if
//  this is an IE 5.0 style notification or a new Win64 style notification.
//  If an IE 5.0 style notification, then it was not sent by us because
//  we send the new Win64-style notification.
//
STDAPI_(BOOL) SHChangeMenuWasSentByMe(LPVOID self, LPCITEMIDLIST pidlNotify)
{
    SHChangeMenuAsIDList UNALIGNED * pcmidl = (SHChangeMenuAsIDList UNALIGNED *)pidlNotify;
    return pcmidl->cb >= FIELD_OFFSET(SHChangeMenuAsIDList, cbZero) &&
           pcmidl->pSender == (INT64)self &&
           pcmidl->dwProcessID == GetCurrentProcessId();
}

//
//
//  Send out an extended event changenotify, using a SHChangeMenuAsIDList
//  as the pidl1 so recipients can identify whether they were the
//  sender or not.
//
//  It's okay to pass self==NULL here.  It means you don't care about
//  detecting whether it was sent by you or not.
//

STDAPI_(void) SHSendChangeMenuNotify(LPVOID self, DWORD shcnee, DWORD shcnf, LPCITEMIDLIST pidl2)
{
    SHChangeMenuAsIDList cmidl;

    cmidl.cb          = FIELD_OFFSET(SHChangeMenuAsIDList, cbZero);
    cmidl.dwItem1     = shcnee;
    cmidl.pSender     = (INT64)self;
    cmidl.dwProcessID = self ? GetCurrentProcessId() : 0;
    cmidl.cbZero      = 0;

    // Nobody had better have specified a type; the type must be
    // SHCNF_IDLIST.
    ASSERT((shcnf & SHCNF_TYPE) == 0);
    SHChangeNotify(SHCNE_EXTENDED_EVENT, shcnf | SHCNF_IDLIST, (LPCITEMIDLIST)&cmidl, pidl2);
}


//Return FALSE if out of memory
STDAPI_(BOOL) Pidl_Set(LPITEMIDLIST* ppidl, LPCITEMIDLIST pidl)
{
    ASSERT(IS_VALID_WRITE_PTR(ppidl, LPITEMIDLIST));
    ASSERT(NULL == *ppidl || IS_VALID_PIDL(*ppidl));
    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));

    if (*ppidl) {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    if (pidl)
        *ppidl = ILClone(pidl);

    return (NULL == pidl || NULL != *ppidl);
}

//  this needs to be the last thing in the file that uses ILClone, because everywhere
//  else, ILClone becomes SafeILClone
#undef ILClone

STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl)
{
    //  the shell32 implementation of ILClone is different for win95 an ie4.
    //  it doesnt check for NULL in the old version, but it does in the new...
    //  so we need to always check
   return pidl ? ILClone(pidl) : NULL;
}

//
// retrieves the UIObject interface for the specified full pidl.
//
STDAPI SHGetUIObjectFromFullPIDL(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    LPCITEMIDLIST pidlChild;
    IShellFolder* psf;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
        psf->Release();
    }

    return hr;
}
