//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1996
//
// File:      recdocs.cpp
//
// History -  created from recent.c in explorer  - ZekeL - 5-MAR-98
//              combining functionality in to one place
//              now that the desktop lives here.
//---------------------------------------------------------------------------

#include "shellprv.h"
#include "shobjprv.h"
#include "recdocs.h"
#include "fstreex.h"
#include "shcombox.h"
#include "ids.h"
#include <urlhist.h>
#include <runtask.h>

#define DM_RECENTDOCS 0x80000000

#define GETRECNAME(p) ((LPCTSTR)(p))
#define GETRECPIDL(p) ((LPCITEMIDLIST) (((LPBYTE) (p)) + CbFromCch(lstrlen(GETRECNAME(p)) +1)))

#define REGSTR_KEY_RECENTDOCS TEXT("RecentDocs")

#define MAX_RECMRU_BUF      (CbFromCch(3 * MAX_PATH))   // Max MRUBuf size


STDAPI RecentDocs_Install(BOOL bInstall)
{
    TCHAR szPath[MAX_PATH];

    // get the path to the favorites folder. Create it if it is missing and we
    // are installing, otherwise don't bother.

    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_RECENT, bInstall))
    {
        if (bInstall)
        {
            SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_INFOTIP | FCSM_ICONFILE, 0};
            TCHAR szInfoTip[128];
            TCHAR szIconFile[MAX_PATH];
    
            //Get the infotip for the recent files Folder
            LoadString(HINST_THISDLL, IDS_RECENT, szInfoTip, ARRAYSIZE(szInfoTip) );
            fcs.pszInfoTip = szInfoTip;

            // Get the IconFile and IconIndex for the Recent Files Folder
            GetSystemDirectory(szIconFile, ARRAYSIZE(szIconFile));
            PathAppend(szIconFile, TEXT("shdocvw.dll"));
            fcs.pszIconFile = szIconFile;
            fcs.iIconIndex = -20785;    // IDI_HISTFOLDER

            SHGetSetFolderCustomSettings(&fcs, szPath, FCS_WRITE);
        }
    }
    return NOERROR; 
}

class CTaskAddDoc : public CRunnableTask
{
public:
    CTaskAddDoc();
    HRESULT Init(HANDLE hMem, DWORD dwProcId);

    // *** pure virtuals ***
    virtual STDMETHODIMP RunInitRT(void);

private:
    virtual ~CTaskAddDoc();

    void _AddToRecentDocs(LPCITEMIDLIST pidlItem, LPCTSTR pszPath);
    void _TryDeleteMRUItem(HANDLE hmru, DWORD cMax, LPCTSTR pszFileName, LPCITEMIDLIST pidlItem, HANDLE hmruOther, BOOL fOverwrite);
    LPBYTE _CreateMRUItem(LPCITEMIDLIST pidlItem, LPCTSTR pszItem, DWORD *pcbItem, UINT uFlags);
    BOOL _AddDocToRecentAndExtRecent(LPCITEMIDLIST pidlItem, LPCTSTR pszFileName, LPCTSTR pszExt);
    void _TryUpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszFolder);
    void _UpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszShare);

    //  private members
    HANDLE _hMem;
    DWORD  _dwProcId;
    HANDLE _hmruRecent;
    DWORD _cMaxRecent;
    LPITEMIDLIST _pidlTarget;
};



BOOL ShouldAddToRecentDocs(LPCITEMIDLIST pidl)
{
    HKEY hk;
    BOOL fRet = TRUE;  //  default to true

    SHGetClassKey(pidl, &hk, NULL);

    if (hk)
    {
        fRet = !(GetFileTypeAttributes(hk) & FTA_NoRecentDocs);
        SHCloseClassKey(hk);
    }
    return fRet;
}

int cdecl RecentDocsCompareName(const void * p1, const void *p2, size_t cb)
{
    return lstrcmpi(GETRECNAME(p2), (LPCTSTR)p1);
}

int cdecl RecentDocsComparePidl(const void * p1, const void *p2, size_t cb)
{
    //  p2 is the one that is in the MRU and p1 is the one that we are passing in...
    return !ILIsEqual(GETRECPIDL(p2), (LPCITEMIDLIST)p1);
}

CTaskAddDoc::~CTaskAddDoc(void)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc destroyed", this);
}

CTaskAddDoc::CTaskAddDoc(void) : CRunnableTask(RTF_DEFAULT)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc created", this);
}


HRESULT CTaskAddDoc::Init( HANDLE hMem, DWORD dwProcId)
{
    if (hMem)
    {
        _hMem = hMem;
        _dwProcId = dwProcId;
        return S_OK;
    }
    return E_FAIL;
}

typedef struct _ARD {
    DWORD   dwOffsetPath;
    DWORD   dwOffsetPidl;
} XMITARD, *PXMITARD;


HRESULT CTaskAddDoc::RunInitRT(void)
{
    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::RunInitRT() running", this);

    PXMITARD px = (PXMITARD)SHLockShared(_hMem, _dwProcId);
    if (px)
    {
        LPITEMIDLIST pidl = px->dwOffsetPidl ? (LPITEMIDLIST)((LPBYTE)px+px->dwOffsetPidl) : NULL;
        LPTSTR pszPath = px->dwOffsetPath ? (LPTSTR)((LPBYTE)px+px->dwOffsetPath) : NULL;

        ASSERT(pszPath);
        
        _AddToRecentDocs(pidl, pszPath);

        SHUnlockShared(px);
        SHFreeShared(_hMem, _dwProcId);
    }
    
    return S_OK;
}


BOOL GetExtensionClassDescription(LPCTSTR lpszFile)
{
    LPTSTR lpszExt = PathFindExtension(lpszFile);
    if (*lpszExt) 
    {
        TCHAR szClass[128];
        TCHAR szDescription[MAX_PATH];
        long cchClass = SIZEOF(szClass);
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, lpszExt, szClass, &cchClass) != ERROR_SUCCESS) 
        {
            // if this fails, use the extension cause it might be a pseudoclass
            lstrcpyn(szClass, lpszExt, ARRAYSIZE(szClass));
        }

        return GetClassDescription(HKEY_CLASSES_ROOT, szClass, szDescription, ARRAYSIZE(szDescription),
                                   GCD_MUSTHAVEOPENCMD | GCD_ALLOWPSUDEOCLASSES);
    }
    return FALSE;
}

STDAPI_(void) FlushRunDlgMRU(void);

//
//  _CleanRecentDocs()
//  cleans out the recent docs folder and the associate registry keys.
//
void _CleanRecentDocs(void)
{
    LPITEMIDLIST pidlTargetLocal = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    if (pidlTargetLocal)
    {
        HKEY hkey;
        TCHAR szDir[MAX_PATH];

        // first, delete all the files
        SHFILEOPSTRUCT sFileOp =
        {
            NULL,
            FO_DELETE,
            szDir,
            NULL,
            FOF_NOCONFIRMATION | FOF_SILENT,
        };
        
        SHGetPathFromIDList(pidlTargetLocal, szDir);
        PathAppend(szDir, c_szStarDotStar);
        szDir[lstrlen(szDir) +1] = 0;     // double null terminate
        SHFileOperation(&sFileOp);


        ILFree(pidlTargetLocal);

        pidlTargetLocal = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, TRUE);

        if (pidlTargetLocal)
        {
            //  now we take care of cleaning out the nethood
            //  we have to more careful, cuz we let other people
            //  add their own stuff in here.
            
            HANDLE hmru = CreateSharedRecentMRUList(TEXT("NetHood"), NULL, SRMLF_COMPPIDL);

            if (hmru)
            {
                IShellFolder* psf;

                if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidlTargetLocal, (void **)&psf)))
                {
                    BOOL fUpdate = FALSE;
                    int iItem = 0;
                    LPITEMIDLIST pidlItem;

                    ASSERT(psf);

                    while (-1 != EnumSharedRecentMRUList(hmru, iItem++, NULL, &pidlItem))
                    {
                        ASSERT(pidlItem);
                        STRRET str;
                        if (SUCCEEDED(psf->GetDisplayNameOf(pidlItem, SHGDN_FORPARSING, &str))
                        && SUCCEEDED(StrRetToBuf(&str, pidlItem, szDir, ARRAYSIZE(szDir))))
                        {
                            szDir[lstrlen(szDir) +1] = 0;     // double null terminate
                            SHFileOperation(&sFileOp);
                        }

                        ILFree(pidlItem);
                    }

                    if (fUpdate)
                        SHChangeNotify(SHCNE_UPDATEDIR, 0, (void *)pidlTargetLocal, NULL);

                    psf->Release();
                }

                FreeMRUList(hmru);
            }

            ILFree(pidlTargetLocal);
        }

        // now delete the registry stuff
        hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey)
        {
            SHDeleteKey(hkey, REGSTR_KEY_RECENTDOCS);
            RegCloseKey(hkey);
        }

        //  reinit the the folder.
        RecentDocs_Install(TRUE);

        SHChangeNotifyHandleEvents();
    }
    
    FlushRunDlgMRU();

    return;
}

//
//  WARNING - _TryDeleteMRUItem() returns an allocated string that must be freed
//
void CTaskAddDoc::_TryDeleteMRUItem(HANDLE hmru, DWORD cMax, LPCTSTR pszFileName, LPCITEMIDLIST pidlItem, HANDLE hmruOther, BOOL fOverwrite)
{
    BYTE buf[MAX_RECMRU_BUF] = {0};

    DWORD cbItem = CbFromCch(lstrlen(pszFileName) + 1);
    int iItem = fOverwrite ? FindMRUData(hmru, pszFileName, cbItem, NULL) : -1;


    //
    //  if iItem is not -1 then it is already existing item that we will replace.
    //  if it is -1 then we need to point iItem to the last in the list.
    if (iItem == -1)
    {
        //  torch the last one if we have the max number of items in the list.
        //  default to success, cuz if we dont find it we dont need to delete it
        iItem = cMax - 1;
    }

    //  if we cannot get it in order to delete it, 
    //  then we will not overwrite the item.
    if (EnumMRUList(hmru, iItem, buf, SIZEOF(buf)) != -1)
    {
        //  convert the buf into the last segment of the pidl
        LPITEMIDLIST pidlFullLink = ILCombine(_pidlTarget, GETRECPIDL(buf));
        if (pidlFullLink)
        {
            // This is semi-gross, but some link types like calling cards are the
            // actual data.  If we delete and recreate they lose their info for the
            // run.  We will detect this by knowing that their pidl will be the
            // same as the one we are deleting...
            if (!ILIsEqual(pidlFullLink, pidlItem))
            {
                TCHAR sz[MAX_PATH];

                // now remove out link to it
                SHGetPathFromIDList(pidlFullLink, sz);

                Win32DeleteFile(sz);
                TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::_TryDeleteMRUItem() deleting '%s'", this, sz);   

                if (hmruOther) 
                {
                    //  deleted a shortcut, 
                    //  need to try and remove it from the hmruOther...
                    iItem = FindMRUData(hmruOther, GETRECNAME(buf), CbFromCch(lstrlen(GETRECNAME(buf)) +1), NULL);
                    if (iItem != -1)
                        DelMRUString(hmruOther, iItem);
                }
            }
            ILFree(pidlFullLink);
        }
    }
}

// in:
// pidlItem - full IDList for the item being added
// pszItem  - name (file spec) of the item (used in the display to the user)
// uFlags   - SHCL_ flags

LPBYTE CTaskAddDoc::_CreateMRUItem(LPCITEMIDLIST pidlItem, LPCTSTR pszItem, 
                                   DWORD *pcbOut, UINT uFlags)
{
    TCHAR sz[MAX_PATH];
    LPBYTE pitem = NULL;

    // create the new one
    if (SHGetPathFromIDList(_pidlTarget, sz)) 
    {
        LPITEMIDLIST pidlFullLink;

        if (SUCCEEDED(CreateLinkToPidl(pidlItem, sz, &pidlFullLink, uFlags)) && 
            pidlFullLink)
        {
            LPCITEMIDLIST pidlLinkLast = ILFindLastID(pidlFullLink);
            int cbLinkLast = ILGetSize(pidlLinkLast);
            DWORD cbItem = CbFromCch(lstrlen(pszItem) + 1);

            pitem = (LPBYTE) LocalAlloc(LPTR, cbItem + cbLinkLast);
            if (pitem)
            {
                memcpy( pitem, pszItem, cbItem );
                memcpy( pitem + cbItem, pidlLinkLast, cbLinkLast);
                *pcbOut = cbItem + cbLinkLast;
            }
            ILFree(pidlFullLink);
        }
    }
    
    return pitem;
}

int EnumSharedRecentMRUList(HANDLE hmru, int iItem, LPTSTR *ppszName, LPITEMIDLIST *ppidl)
{
    BYTE buf[MAX_RECMRU_BUF] = {0};
    int iRet;
    //  if both out params are NULL or iItem < 0 then this is a query for the size
    //  of the MRU list...
    if (ppszName)
        *ppszName = NULL;
    if (ppidl)
        *ppidl = NULL;
        
    if (iItem < 0 || (!ppszName && !ppidl))
        iRet = EnumMRUList(hmru, -1, NULL, 0);
    else if (-1 != (iRet = EnumMRUList(hmru, iItem, buf, SIZEOF(buf))))
    {
        if (ppszName)
        {
            *ppszName = StrDup(GETRECNAME(buf));
            if (!*ppszName)
                iRet = -1;
        }
        else if (ppidl)
        {
            *ppidl = ILClone(GETRECPIDL(buf));
            if (!*ppidl)
                iRet = -1;
        }
    }

    return iRet;
}

#define MAXRECENT_DEFAULTDOC      10
#define MAXRECENT_MAJORDOC        20

HANDLE CreateSharedRecentMRUList(LPCTSTR pszClass, DWORD *pcMax, DWORD dwFlags)
{
    if (SHRestricted(REST_NORECENTDOCSHISTORY))
        return NULL;

    DWORD cMax;
    MRUCMPDATAPROC procCompare = RecentDocsCompareName;
    TCHAR szKey[MAX_PATH];
    LPCTSTR pszKey = REGSTR_PATH_EXPLORER TEXT("\\") REGSTR_KEY_RECENTDOCS;

    if (pszClass)
    {
        //  want to use the pszExt as a sub key
        lstrcpy(szKey, pszKey);
        StrCatBuff(szKey, TEXT("\\"), SIZECHARS(szKey));
        StrCatBuff(szKey, pszClass, SIZECHARS(szKey));
        pszKey = szKey;

        //  we need to find out how many
        DWORD dwType = REG_DWORD, dwMajor = 0, cbSize = SIZEOF(cbSize);
        SHGetValue(HKEY_CLASSES_ROOT, pszClass, TEXT("MajorDoc"), &dwType, (LPVOID)&dwMajor, &cbSize);
        cMax = dwMajor ? MAXRECENT_MAJORDOC : MAXRECENT_DEFAULTDOC;
    }
    else
    {
        //  this the root MRU
        cMax = SHRestricted(REST_MaxRecentDocs);

        //  default max docs...
        if (cMax < 1)
            cMax = MAXRECENTDOCS * MAXRECENT_DEFAULTDOC;
    }

    if (dwFlags & SRMLF_COMPPIDL)
        procCompare = RecentDocsComparePidl;
        
    MRUDATAINFO mi =  {
        SIZEOF(MRUDATAINFO),
        cMax,
        MRU_BINARY | MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        pszKey,
        procCompare
        };

    if (pcMax)
        *pcMax = cMax;
        
    return CreateMRUList((MRUINFO *)&mi);

}

BOOL CTaskAddDoc::_AddDocToRecentAndExtRecent(LPCITEMIDLIST pidlItem, LPCTSTR pszFileName, 
                                              LPCTSTR pszExt)
{
    DWORD cbItem = CbFromCch(lstrlen(pszFileName) + 1);
    DWORD cMax;
    HANDLE hmru = CreateSharedRecentMRUList(pszExt, &cMax, SRMLF_COMPNAME);

    _TryDeleteMRUItem(_hmruRecent, _cMaxRecent, pszFileName, pidlItem, hmru, TRUE);

    LPBYTE pitem = _CreateMRUItem(pidlItem, pszFileName, &cbItem, 0);
    if (pitem)
    {
        AddMRUData(_hmruRecent, pitem, cbItem);

        if (hmru)
        {
            //  we dont want to delete the file if it already existed, because
            //  the TryDelete on the RecentMRU would have already done that
            //  we only want to delete if we have some overflow from the ExtMRU
            _TryDeleteMRUItem(hmru, cMax, pszFileName, pidlItem, _hmruRecent, FALSE);

            //  can reuse the already created item to this mru
            AddMRUData(hmru, pitem, cbItem);

            FreeMRUList(hmru);
        }
                
        LocalFree(pitem);
    }

    //  its been freed but not nulled out...
    return (pitem != NULL);
}


// 
//  WARNING:  UpdateNetHood() changes _pidlTarget to the NetHood then frees it!
//
void CTaskAddDoc::_UpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszShare)
{
    if (SHRestricted(REST_NORECENTDOCSNETHOOD))
        return;

    //  need to add this boy to the Network Places
    LPITEMIDLIST pidl = ILCreateFromPath(pszShare);
    if (pidl)
    {
        //
        //  BUGBUG - must verify parentage here - ZekeL - 27-MAY-99
        //  http servers exist in both the webfolders namespace 
        //  and the Internet namespace.  thus we must make sure
        //  that what ever parent the folder had, the share has
        //  the same one.
        //
        if (ILIsParent(pidl, pidlFolder, FALSE))
        {
            ASSERT(_pidlTarget);
            ILFree(_pidlTarget);
            
            _pidlTarget = SHCloneSpecialIDList(NULL, CSIDL_NETHOOD, TRUE);
            if (_pidlTarget)
            {
                DWORD cMax;
                HANDLE hmru = CreateSharedRecentMRUList(TEXT("NetHood"), &cMax, SRMLF_COMPNAME);
                if (hmru)
                {
                    _TryDeleteMRUItem(hmru, cMax, pszShare, pidl, NULL, TRUE);
                    DWORD cbItem = CbFromCch(lstrlen(pszShare) + 1);
                    LPBYTE pitem = _CreateMRUItem(pidl, pszShare, &cbItem, SHCL_MAKEFOLDERSHORTCUT);
                    if (pitem)
                    {
                        AddMRUData(hmru, pitem, cbItem);
                        LocalFree(pitem);
                    }

                    FreeMRUList(hmru);
                }

                ILFree(_pidlTarget);
                _pidlTarget = NULL;
            }
        }
        
        ILFree(pidl);
    }
}

BOOL PathIsOneOf(const UINT rgFolders[], LPCTSTR pszFolder);
            
BOOL _IsPlacesFolder(LPCTSTR pszFolder)
{
    static const UINT places[] = {
        CSIDL_PERSONAL,
        CSIDL_DESKTOPDIRECTORY,
        CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_NETHOOD,
        CSIDL_FAVORITES,
        (UINT)-1 // terminator
    };

    return PathIsOneOf(places, pszFolder);
}

void _AddToUrlHistory(LPCTSTR pszPath)
{
    ASSERT(pszPath);
    WCHAR szUrl[MAX_URL_STRING];
    DWORD cchUrl = SIZECHARS(szUrl);

    SHTCharToUnicode(pszPath, szUrl, cchUrl);

    //  the URL parsing APIs tolerate same in/out buffer
    if (SUCCEEDED(UrlCreateFromPathW(szUrl, szUrl, &cchUrl, 0)))
    {
        IUrlHistoryStg *puhs;
        if (SUCCEEDED(CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, 
                IID_IUrlHistoryStg, (void **)&puhs)))
        {
            ASSERT(puhs);
            puhs->AddUrl(szUrl, NULL, 0);
            puhs->Release();
        }
    }
}

void CTaskAddDoc::_TryUpdateNetHood(LPCITEMIDLIST pidlFolder, LPCTSTR pszFolder)
{
    TCHAR sz[MAX_URL_STRING];
    DWORD cch = SIZECHARS(sz);
    BOOL fUpdate = FALSE;
    // changing szFolder, and changing _pidlTarget here...
    //  if this is an URL or a UNC share add it to the nethood

    if (UrlIs(pszFolder, URLIS_URL) 
    && !UrlIs(pszFolder, URLIS_OPAQUE)
    && SUCCEEDED(UrlCombine(pszFolder, TEXT("/"), sz, &cch, 0)))
        fUpdate = TRUE;
    else if (PathIsUNC(pszFolder) 
    && StrCpyN(sz, pszFolder, cch)
    && PathStripToRoot(sz))
        fUpdate = TRUE;

    if (fUpdate)
        _UpdateNetHood(pidlFolder, sz);
}

//-----------------------------------------------------------------
//
// Add the named file to the Recently opened MRU list, that is used
// by the shell to display the recent menu of the tray.

// this registry will hold two pidls:  the target pointing to followed by
// the pidl of the link created pointing it.  In both cases,
// only the last item id is stored. (we may want to change this... but
// then again, we may not)

void CTaskAddDoc::_AddToRecentDocs(LPCITEMIDLIST pidlItem, LPCTSTR pszItem)
{
    TCHAR szUnescaped[MAX_PATH];
    LPTSTR pszFileName;

    //  if these are NULL the caller meant to call _CleanRecentDocs()
    ASSERT(pszItem && *pszItem);

    TraceMsg(DM_RECENTDOCS, "[%X] CTaskAddDoc::_AddToRecentDocs() called for '%s'", this, pszItem);   
    // allow only classes with default commands
    //
    //  dont add if:
    //     it is RESTRICTED
    //     it is in the temporary directory
    //     it actually has a file name
    //     it can be shell exec'd with "open" verb
    //
    if ( (SHRestricted(REST_NORECENTDOCSHISTORY))     ||
         (PathIsTemporary(pszItem))                   ||
         (!(pszFileName = PathFindFileName(pszItem))) ||
         (!*pszFileName)                              ||
         (!GetExtensionClassDescription(pszFileName))   
       )  
        return;

    //  pretty up the URL file names.
    if (UrlIs(pszItem, URLIS_URL))
    {
        StrCpyN(szUnescaped, pszFileName, SIZECHARS(szUnescaped));
        UrlUnescapeInPlace(szUnescaped, 0);
        pszFileName = szUnescaped;
    }
    
    //  otherwise we try our best.
    ASSERT(!_pidlTarget);
    _pidlTarget = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    if (_pidlTarget) 
    {
        _hmruRecent = CreateSharedRecentMRUList(NULL, &_cMaxRecent, SRMLF_COMPNAME);
        if (_hmruRecent)
        {
            if (_AddDocToRecentAndExtRecent(pidlItem, pszFileName, PathFindExtension(pszFileName)))
            {
                _AddToUrlHistory(pszItem);
                //  get the folder and do it to the folder
                LPITEMIDLIST pidlFolder = ILClone(pidlItem);
                
                if (pidlFolder)
                {
                    ILRemoveLastID(pidlFolder);
                    //  if it is a folder we already have quick
                    //  access to from the shell, dont put it in here

                    TCHAR szFolder[MAX_URL_STRING];
                    if (SUCCEEDED(SHGetNameAndFlags(pidlFolder, SHGDN_FORPARSING, szFolder, SIZECHARS(szFolder), NULL))
                    && !_IsPlacesFolder(szFolder))
                    {
                        //  get the friendly name for the folder
                        TCHAR szTitle[MAX_PATH];
                        if (FAILED(SHGetNameAndFlags(pidlFolder, SHGDN_NORMAL, szTitle, SIZECHARS(szTitle), NULL)))
                            StrCpyN(szTitle, PathFindFileName(szFolder), ARRAYSIZE(szTitle));
                            
                        _AddDocToRecentAndExtRecent(pidlFolder, szTitle, TEXT("Folder"));

                        _TryUpdateNetHood(pidlFolder, szFolder);
                    }
                    
                    ILFree(pidlFolder);
                }
                SHChangeNotifyHandleEvents();
            }
            
            FreeMRUList(_hmruRecent);
            _hmruRecent = NULL;
        }

        //cleanup
        if (_pidlTarget)
        {
            ILFree(_pidlTarget);
            _pidlTarget = NULL;
        }
    }
}

STDAPI_(void) OpenWithListSoftRegisterProcess(DWORD dwFlags, LPCTSTR pszExt);

void AddToRecentDocs(LPCITEMIDLIST pidl, LPCTSTR pszItem)
{
    HWND hwnd;
    DWORD cbSizePidl, cbSizePath;

    ASSERT(pidl && pszItem);

    if (!ShouldAddToRecentDocs(pidl))
        return;

    OpenWithListSoftRegisterProcess(0, PathFindExtension(pszItem));   
    
    cbSizePidl = ILGetSize(pidl);
    cbSizePath = CbFromCch(lstrlen(pszItem) + 1);

    hwnd = GetShellWindow();
    if (hwnd)
    {
        PXMITARD px;
        DWORD dwProcId, dwOffset;
        HANDLE hARD;

        GetWindowThreadProcessId(hwnd, &dwProcId);

        hARD = SHAllocShared(NULL, SIZEOF(XMITARD)+cbSizePath+cbSizePidl, dwProcId);
        if (!hARD)
            return;         // Well, we are going to miss one, sorry.

        px = (PXMITARD)SHLockShared(hARD,dwProcId);
        if (!px)
        {
            SHFreeShared(hARD,dwProcId);
            return;         // Well, we are going to miss one, sorry.
        }

        px->dwOffsetPidl = 0;
        px->dwOffsetPath = 0;

        dwOffset = SIZEOF(XMITARD);
        if (pszItem)
        {
            px->dwOffsetPath = dwOffset;
            memcpy((LPBYTE)px + dwOffset, pszItem, cbSizePath);
            dwOffset += cbSizePath;
        }
        if (pidl)
        {
            px->dwOffsetPidl = dwOffset;
            memcpy((LPBYTE)px + dwOffset, pidl, cbSizePidl);
        }

        SHUnlockShared(px);

        PostMessage(hwnd, CWM_ADDTORECENT, (WPARAM)hARD, (LPARAM)dwProcId);
    }
}


//
// put things in the shells recent docs list for the start menu
//
// in:
//      uFlags  SHARD_ (shell add recent docs) flags
//      pv      LPCSTR or LPCITEMIDLIST (path or pidl indicated by uFlags)
//              may be NULL, meaning clear the recent list
//
STDAPI_(void) SHAddToRecentDocs(UINT uFlags, LPCVOID pv)
{
    TCHAR szTemp[MAX_URL_STRING]; // for double null

    TraceMsg(DM_RECENTDOCS, "SHAddToRecentDocs() called with %d, [%X]", uFlags, pv);
    
    if (pv == NULL)     // we should nuke all recent docs.
    {
        //  we do this synchronously
        _CleanRecentDocs();
        return;
    }

    if (SHRestricted(REST_NORECENTDOCSHISTORY))
        // Don't bother tracking recent documents if restriction is set
        // for privacy.
        return;
    
    if (uFlags == SHARD_PIDL)
    {
        // pv is a LPCITEMIDLIST (pidl)
        if (SUCCEEDED(SHGetNameAndFlags((LPCITEMIDLIST)pv, SHGDN_FORPARSING, szTemp, SIZECHARS(szTemp), NULL)))
        {
            AddToRecentDocs((LPCITEMIDLIST)pv, szTemp);
        }
    }
    else if (uFlags == SHARD_PATH)
    {
        // pv is a LPTCSTR (path)
        LPITEMIDLIST pidl = ILCreateFromPath((LPCTSTR)pv);
        if (!pidl)
            pidl = SHSimpleIDListFromPath((LPCTSTR)pv);
        if (pidl)
        {
            AddToRecentDocs(pidl, (LPCTSTR)pv);
            ILFree(pidl);
        }
    }
#ifdef UNICODE
    else if (uFlags == SHARD_PATHA)
    {
        SHAnsiToUnicode((LPCSTR)pv, szTemp, ARRAYSIZE(szTemp));
        SHAddToRecentDocs(SHARD_PATH, szTemp);
    }
#else
    else if (uFlags == SHARD_PATHW)
    {
        SHUnicodeToAnsi((LPCWSTR)pv, szTemp, ARRAYSIZE(szTemp));
        SHAddToRecentDocs(SHARD_PATH, szTemp);
    }
#endif
}

WINSHELLAPI void ReceiveAddToRecentDocs(HANDLE hARD, DWORD dwProcId)
{
    //  NOTIMPL
    ASSERT(FALSE);
}

STDAPI CTaskAddDoc_Create(HANDLE hMem, DWORD dwProcId, IRunnableTask **pptask)
{
    HRESULT hres;
    CTaskAddDoc *ptad = new CTaskAddDoc();
    if (ptad)
    {
        hres = ptad->Init(hMem, dwProcId);
        if (SUCCEEDED(hres))
            hres = ptad->QueryInterface(IID_IRunnableTask, (void **)pptask);
        ptad->Release();
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}

STDAPI RecentDocs_GetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName, DWORD cchName)
{
    HRESULT hr = E_FAIL;
    HANDLE hmru = CreateSharedRecentMRUList(NULL, NULL, SRMLF_COMPPIDL);

    if (hmru)
    {
        int iItem = FindMRUData(hmru, pidl, ILGetSize(pidl), NULL);

        if (-1 != iItem)
        {
            BYTE buf[MAX_RECMRU_BUF];

            if (-1 != EnumMRUList(hmru, iItem, buf, SIZEOF(buf)))
            {
                StrCpyN(pszName, GETRECNAME(buf), cchName);
                hr = S_OK;
            }
        }

        FreeMRUList(hmru);
     }

     return hr;
 }
