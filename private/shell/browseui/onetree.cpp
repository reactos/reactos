#define  DONT_USE_ATL
#include "priv.h"

#include <shguidp.h>
#include "vdate.h"
#include "resource.h"

//#define OT_DEBUG

#include "onetree.h"
#include "mluisupp.h"

DWORD g_dwEnumTimeout = (DWORD)0;

#ifdef DEBUG
UINT g_cTotalOTBindToFolder = 0;
UINT g_cHitOTBindToFolder = 0;
#define DEBUG_INCL_TOTAL        g_cTotalOTBindToFolder++;
#define DEBUG_INCL_HIT          g_cHitOTBindToFolder++;
#else
#define DEBUG_INCL_TOTAL
#define DEBUG_INCL_HIT
#endif

#define NOKIDS          ((HDPA)-1)
#define KIDSUNKNOWN     ((HDPA)0)

// We want to use the real ILIsParent in this file
#undef ILIsParent

// these do no checking.
// just convenience macros
//
#define GetNthKid(lpnParent, i)         ((LPOneTreeNode)DPA_GetPtr(lpnParent->hdpaKids, i))
#define GetKidCount(lpnParent)          DPA_GetPtrCount(lpnParent->hdpaKids)
#define NodeHasKids(lpNode)             ((lpNode)->hdpaKids != KIDSUNKNOWN && (lpNode)->hdpaKids != NOKIDS)

extern const ITEMIDLIST s_idlNULL;

#define ADDCHILD_FAILED 0   // error, didn't add it
#define ADDCHILD_ADDED   1  // didn't find it, so we added it.
#define ADDCHILD_EXISTED 2  // didn't add, it already existed.

UINT AddChild(IShellFolder *psfParent, LPOneTreeNode lpnParent,
                          LPCITEMIDLIST pidl, BOOL bAllowDup,
                          LPOneTreeNode *lplpnKid);

#ifdef DEBUG
void AssertValidNode(LPOneTreeNode lpNode);
#else
#define AssertValidNode(lpn)
#endif

LPOneTreeNode   s_lpnRoot = NULL;
IShellFolder *  s_pshfRoot = NULL;
ULONG           s_uFSRegisterID = 0;
HWND            s_hwndOT = NULL;

LONG            g_OTRegRefCount = 0;
ATOM            g_OTWndClassAtom = 0;

BOOL            g_fShowCompColor = FALSE;       // Display compressed items in a different color
COLORREF        g_crAltColor = RGB(0,0,255);    // blue

const TCHAR     c_szOTClass[] = TEXT("OTClass");
const TCHAR     c_szImeCls[]  = TEXT("IME");
const TCHAR     c_szProgMan[] = TEXT("Program Manager");

void _OTGetImageIndex(IShellFolder *psfParent, LPOneTreeNode lpNode);
void HandleFileSysChange(LONG lNotification, LPITEMIDLIST*lplpidl);
BOOL SearchForKids(HWND hwndOwner, LPOneTreeNode lpnParent, LPEnumInfo pei, BOOL fInteractive);
LRESULT CALLBACK OneTreeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPOneTreeNode _GetNodeFromIDList(LPCITEMIDLIST pidlFull, UINT uFlags, HRESULT *phresOut);
void InvalidateImageIndices();
LPITEMIDLIST OTGetRealFolderIDL(LPOneTreeNode lpnParent, LPCITEMIDLIST pidlSimple);
void _OTUnregister(HWND hwnd);
void _OTRegister(HWND hwnd);

#define WM_OT_FSNOTIFY  (WM_USER + 11)
//#define WM_OT_REGISTER  (WM_USER + 12)
#define WM_OT_UNREGISTER (WM_USER + 13)
#define WM_OT_DESTROY    (WM_USER + 14)


#ifdef DEBUG
void AssertValidNode(LPOneTreeNode lpNode)
{
    if (lpNode) {
        ASSERT(lpNode->dwDebugSig == OTDEBUG_SIG);
        ASSERT(lpNode->cRef >= 0);
    }
}
#endif

//
// The enum window proc that finds IME window on the thread and kills it.
//
BOOL CALLBACK KillIMEEnumProc(HWND hwnd, LPARAM lParam)
{
    UINT tidOT = (UINT)lParam;
    UINT tidhwnd = GetWindowThreadProcessId(hwnd, NULL);
    TCHAR szCls[16];
    
    if (tidOT == tidhwnd)
    {
         GetClassName(hwnd, szCls, ARRAYSIZE(szCls));
         if (StrCmpNI(szCls, c_szImeCls, ARRAYSIZE(szCls)) == 0)
         {
             DestroyWindow(hwnd);
             return FALSE;
         }
    }
    return(TRUE);
}

// kill orphan ime window
void KillIMEWindow()
{
    UINT tidOT = GetWindowThreadProcessId(s_hwndOT, NULL);
    
    EnumWindows(KillIMEEnumProc, (LPARAM)tidOT);
}


INT CALLBACK OTCompareIDs(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPOTCompareInfo lpinfo )
{
    int iRet;
    
    if (lpinfo->fRooted && (ILIsRooted(pidl1) || ILIsRooted(pidl2)))
    {
        iRet = ILRootedCompare(pidl1, pidl2);
    }
    else
    {
        HRESULT hres = lpinfo->psf->CompareIDs(0, pidl1, pidl2);

        iRet = (int)ShortFromResult(hres);
    }
    
    lpinfo->bFound |= (iRet == 0);

    return iRet;
}

//
// OneTree DPA_Sort comparison function for LPOneTreeNodes
//

INT CALLBACK OTCompareNodes( LPOneTreeNode lpn1, LPOneTreeNode lpn2, LPOTCompareInfo lpinfo )
{
    return OTCompareIDs(lpn1->pidl, lpn2->pidl, lpinfo);
}

//
// OneTree DPA_Search comparison function for LPOneTreeNodes
//
// Compare the two LPOneTreeNodes

INT CALLBACK OTCompareID2Node(LPCITEMIDLIST pidl, LPOneTreeNode lpn, LPOTCompareInfo lpinfo )
{
    return OTCompareIDs(pidl, lpn->pidl, lpinfo);
}


BOOL OTIsMainThread()
{
    // returns true if this is the main onetree thread (creator of s_hwndOT)
    return s_hwndOT && (GetCurrentThreadId() == GetWindowThreadProcessId(s_hwndOT, NULL));
}

STDAPI_(void) OTAssumeThread()
{
    if (OTIsMainThread()) 
    {
        MSG msg;

        // BUGBUG:: Jazz/Zip drives have a hook proc on our threads which will
        // fault at shutdown time.  They don't appear to programatically remove
        // the hook and just let it get cleaned up at thread termination.  The old
        // onetree code in explorer did not have this hooked in.  To get around this
        // try to release the hook if we become the OneTree Thread...
        // This ain't pretty!
        HMODULE hmod = GetModuleHandleA("imghook.dll");
        if (hmod)
        {
            // Below is the address of the hook that will killing us...
            UnhookWindowsHook(WH_CALLWNDPROCRET, (HOOKPROC)((LPBYTE)hmod + 0x1644L));
        }
        
        // For FarEast platform, the IME window remains as an orphan after the IEFrame
        // window has been killed. sounds like a bug of the system, but we want to work it
        // around by explicitly killing it.
        if (GetSystemMetrics(SM_DBCSENABLED))
            KillIMEWindow();

#ifdef UNIX
        // I don't know who the hell is supposed to send it to me when 
        // I'm in file view->explorer mode, but nobody does.
        // BTW, I get the same on WinNT - hang on exit.
        if (s_hwndOT != NULL)
            PostMessage(s_hwndOT, WM_DESTROY, 0, 0);
#endif

        while (s_hwndOT && GetMessage(&msg, NULL, 0, 0)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

LRESULT CALLBACK OneTreeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INSTRUMENT_WNDPROC(SHCNFI_ONETREE_WNDPROC, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_OT_UNREGISTER:
        _OTUnregister((HWND)lParam);
        break;
        
    case WM_OT_FSNOTIFY:
        {
            LPSHChangeNotificationLock pshcnl;
            LPITEMIDLIST *ppidl;
            LONG lEvent;

            if (g_fNewNotify && (wParam || lParam))
            {
                // New style of notifications need to lock and unlock in order to free the memory...
                pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
                if (!pshcnl)
                    break;
            } else {
                lEvent = (LONG)lParam;
                ppidl = (LPITEMIDLIST*)wParam;
                pshcnl = NULL;
            }

            HandleFileSysChange(lEvent, ppidl);
    
            if (pshcnl)
            {
                SHChangeNotification_Unlock(pshcnl);
            }
        }
        break;
        
    case WM_OT_DESTROY:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        // null only in the process of shutting down everything
        if (s_hwndOT) {
            s_hwndOT = NULL;
        }
        SHChangeNotifyDeregister(s_uFSRegisterID);

        if (s_lpnRoot) {
            LPOneTreeNode lpn = s_lpnRoot;
            s_lpnRoot = NULL;
            OTRelease(lpn);
        }

        if (s_pshfRoot)
        {
            s_pshfRoot->Release();
            s_pshfRoot = NULL;
        }
        break;

    case WM_SETTINGCHANGE:
        if (SHIsExplorerIniChange(wParam, lParam) & EICH_KWINEXPLORER)
        {
            OneTree_GetAltColor();
        }
        break;

    default:
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
    return 0L;
}

void OneTree_Terminate()
{
    if (IsWindow(s_hwndOT))
        PostMessage(s_hwndOT, WM_OT_DESTROY, 0, 0);
}

//----------------------------------------------------------------------------
STDAPI_(int) MapCLSIDToSystemImageListIndex(const CLSID *pclsid)
{
    TCHAR sz[MAX_PATH];
    TCHAR szDefIcon[MAX_PATH];
    DWORD cbDefIcon = SIZEOF(szDefIcon);
    int i = 0;

    StrCpy(sz, TEXT("CLSID\\"));
    SHStringFromGUID(*pclsid, sz + 6, ARRAYSIZE(sz) - 6);
    StrCatBuff(sz, TEXT("\\DefaultIcon"), ARRAYSIZE(sz));

    if (SHGetValueGoodBoot(HKEY_CLASSES_ROOT, sz, NULL, NULL, (BYTE *)szDefIcon, &cbDefIcon) == ERROR_SUCCESS)
    {
        i = Shell_GetCachedImageIndex(szDefIcon, PathParseIconLocation(szDefIcon), 0);
    }
    return i;
}

UINT AddChildEx(IShellFolder *psfParent, LPOneTreeNode lpnParent,
                          LPCITEMIDLIST pidl, BOOL bAllowDup,
                          LPOneTreeNode *lplpnKid, LPCTSTR pszText);

void OneTree_GetAltColor(void)
{
    if (g_fRunningOnNT)
    {
        // Fetch the NT specific settings.
        SHELLSTATE ss = {0};

        SHGetSetSettings(&ss, SSF_SHOWCOMPCOLOR, FALSE);
        g_fShowCompColor = ss.fShowCompColor;

        DWORD cbData = sizeof(g_crAltColor);
        SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("AltColor"), NULL, &g_crAltColor, &cbData);
    }
}

BOOL OneTree_Initialize(void)
{
    WNDCLASS wndclass = {0};

    ASSERTCRITICAL;
    
    OneTree_GetAltColor();

    // dummy window for the FSNotify
    wndclass.lpfnWndProc   = OneTreeWndProc ;
    wndclass.hInstance     = HINST_THISDLL;
    wndclass.lpszClassName = c_szOTClass;

    if (!g_OTWndClassAtom) {
        if (!(g_OTWndClassAtom = RegisterClass(&wndclass))) {
            return FALSE;
        }
    }

    if (AddChild(NULL, NULL, &s_idlNULL, FALSE, &s_lpnRoot) != ADDCHILD_ADDED)
    {
        OneTree_Terminate();
        return(FALSE);
    }

    SHGetDesktopFolder(&s_pshfRoot);
    
    s_lpnRoot->iImage = SHMapPIDLToSystemImageListIndex(s_pshfRoot,
            &s_idlNULL, &s_lpnRoot->iSelectedImage);
    
    s_lpnRoot->dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_SHARE | SFGAO_REMOVABLE |SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM | SFGAO_CAPABILITYMASK | SFGAO_NEWCONTENT;
    s_pshfRoot->GetAttributesOf(0, NULL, &s_lpnRoot->dwAttribs);
    s_lpnRoot->dwAttribs &= ~(SFGAO_CANRENAME);

    s_lpnRoot->cChildren = 1;

    SearchForKids(NULL, s_lpnRoot, NULL, FALSE);

    return TRUE;
}

LPITEMIDLIST OTCloneFolderID(LPOneTreeNode lpn)
{
    LPITEMIDLIST pidl = NULL;

    ENTERCRITICAL;
    if (lpn) {
        pidl = ILClone(lpn->pidl);
    }
    LEAVECRITICAL;

    return pidl;
}

LPTSTR OTGetNodeName(LPOneTreeNode lpn, LPTSTR pszText, int cch)
{
    pszText[0] = 0;

    ENTERCRITICAL;
    if (lpn->lpText && lpn->lpText != LPSTR_TEXTCALLBACK) {
        ASSERT(pszText);
        lstrcpyn(pszText, lpn->lpText, cch);
    }
    LEAVECRITICAL;

    return pszText;
}

void KillKids(LPOneTreeNode lpNode)
{
    //
    // free this kid  and all its decendants
    //
    if (NodeHasKids(lpNode))
    {
        int i;
        HDPA hdpaKids = lpNode->hdpaKids;

        // null out the hdpaKids var first because debug version
        // make sure that the parent doesn't have a pointe to us (above)
        lpNode->hdpaKids = KIDSUNKNOWN;
        for( i = DPA_GetPtrCount(hdpaKids) - 1; i >= 0 ; i--) {
            OTRelease((LPOneTreeNode)DPA_GetPtr(hdpaKids, i));
        }
        DPA_Destroy(hdpaKids);
    }
}

void OTSetNodeName(LPOneTreeNode lpNode, LPTSTR pszName)
{
    CoTaskMemFree(lpNode->lpText);
    lpNode->lpText = pszName;
}

void OTFreeNodeData(LPOneTreeNode lpNode)
{
    ENTERCRITICAL;
    OTSetNodeName(lpNode, NULL);

    if (lpNode->pidl) {
        ILFree(lpNode->pidl);
        lpNode->pidl = NULL;
    }
    LEAVECRITICAL;
}

void KillNode(LPOneTreeNode lpNode)
{
    AssertValidNode(lpNode);

//
//  I'm disabling this debug code since we'll hit GPF when the parent
// node is already freed.
//
#if 0
#ifdef DEBUG
    // make sure our parent doesn't have a pointer to us.  This will help
    // (but not ensure) that we're not doing extra OTReleases.
    {
        LPOneTreeNode lpnParent = lpNode->lpnParent;
        int i;
        if (lpnParent && NodeHasKids(lpnParent)) {
            i = GetKidCount(lpnParent);
            while( i--) {
                // this is REALLY REALLY bad if this assert fails.
                // let chee know.
                ASSERT(GetNthKid(lpnParent, i) != lpNode);
            }
        }
    }

#endif
#endif

    KillKids(lpNode);

    OTFreeNodeData(lpNode);
    LocalFree(lpNode);
}


void OTRelease(LPOneTreeNode lpNode)
{
    AssertValidNode(lpNode);

    if (lpNode) 
    {
        ASSERT(lpNode->cRef > 0);
        ASSERT((lpNode->cRef > 1) || (lpNode != s_lpnRoot));

        // this should NEVER happen.. but just in case,
        // don't free out the s_lpnRoot while it's still
        // in our global
        if ((lpNode->cRef == 1) && (lpNode == s_lpnRoot))
            return;

        lpNode->cRef--;
        if (lpNode->cRef == 0)
            KillNode(lpNode);
    }
}

LPOneTreeNode OTGetParent(LPOneTreeNode lpNode)
{
    AssertValidNode(lpNode);

    // sanity check
    if (!lpNode)
        return NULL;

    if (lpNode->lpnParent)
        OTAddRef(lpNode->lpnParent);
    return lpNode->lpnParent;
}

void DoInvalidateAll(LPOneTreeNode lpNode, int iImage)
{
    int i;
    LPOneTreeNode lpnKid ;

    if (lpNode == NULL)
        return;

    AssertValidNode(lpNode); // this validate just validates  that it's a node...

    ENTERCRITICAL;
    if (iImage == -1 ||
        iImage == lpNode->iImage ||
        iImage == lpNode->iSelectedImage) {
        lpNode->fInvalid = TRUE;
    }

    if (NodeHasKids(lpNode)) {
        for (i = GetKidCount(lpNode) - 1; i >= 0; i--) {
            lpnKid = GetNthKid(lpNode, i);
            if (lpnKid) {
                DoInvalidateAll(lpnKid, iImage);
            }
        }
    }
    LEAVECRITICAL;
}

LPOneTreeNode FindKid(IShellFolder *psfParent, LPOneTreeNode lpnParent, LPCITEMIDLIST pidlKid, BOOL bValidate, INT * pPos)
{
    ASSERT(psfParent);

    // there can be only one...
    ASSERT(ILIsEmpty(_ILNext(pidlKid)));
    ASSERTCRITICAL;

    *pPos = 0;
    if (lpnParent->hdpaKids == KIDSUNKNOWN || lpnParent->fInvalid) {
        if (bValidate)
            SearchForKids(NULL, lpnParent, NULL, FALSE);
    }

    //
    // since we might not have validated, we might still have kidsunknow.
    //
    if ((lpnParent->hdpaKids != NOKIDS) && (lpnParent->hdpaKids != KIDSUNKNOWN)) 
    {

        OTCompareInfo info = {0};

        info.psf = psfParent;
        info.fRooted = (lpnParent == s_lpnRoot);
        
        *pPos = DPA_Search( lpnParent->hdpaKids,
                        (LPVOID)pidlKid,
                        0,
                        (PFNDPACOMPARE)OTCompareID2Node,
                        (LPARAM)&info,
                        DPAS_SORTED | DPAS_INSERTAFTER
                       );

        if (info.bFound)
        {
            return (LPOneTreeNode)DPA_FastGetPtr( lpnParent->hdpaKids, *pPos );
        }

    }

    return NULL;
}

#define IsEqualItemID(pmkid1, pmkid2)   (0 == memcmp(pmkid1, pmkid2, (pmkid1)->cb))

// lplpnKidAdded -- this is in/out.  it holds the kid to be added coming in..
// it holds the kid added (or found) coming out
void AdoptKid(IShellFolder *psfParent, LPOneTreeNode* lplpnKidAdded, int iPos, DWORD dwLastChecked)
{
    LPOneTreeNode lpnFind;
    LPOneTreeNode lpnNewKid = *lplpnKidAdded;
    LPOneTreeNode lpnParent = lpnNewKid->lpnParent;

    ENTERCRITICAL;

    if (lpnParent->hdpaKids == KIDSUNKNOWN) 
    {
        lpnParent->fInvalid = TRUE;
        lpnParent->hdpaKids = NOKIDS;
    }

    if (lpnParent->hdpaKids == NOKIDS)
    {
        lpnParent->hdpaKids = DPA_Create(0);
        iPos = 0;
    }

    //
    // has anyone added anything to the DPA since
    // we did the initial find?
    if ((iPos != -1 && dwLastChecked == lpnParent->dwLastChanged) ||
        !(lpnFind = FindKid(psfParent, lpnParent, lpnNewKid->pidl, FALSE, &iPos))) 
    {
        OTAddRef(lpnNewKid);
        lpnParent->dwLastChanged = GetTickCount();
        lpnParent->cChildren = 1; // there are now kids - we just added one
        DPA_InsertPtr( lpnParent->hdpaKids, iPos, lpnNewKid );
    }
    else
    {
        *lplpnKidAdded = lpnFind;
    }
    LEAVECRITICAL;
}

#ifdef FOR_GEORGEST
void DebugDumpNode(LPOneTreeNode lpn, LPTSTR lpsz)
{
    ASSERT(lpn->lpText);
    TraceMsg(DM_TRACE, "ONETREE: %s (%d) %s", lpsz, lpn, lpn->lpText);
}
#endif

LPOneTreeNode OTNCreate(LPCITEMIDLIST pidl, LPCTSTR pszName, DWORD dwAttribs, LPOneTreeNode lpnParent)
{
    LPOneTreeNode lpnKid = (LPOneTreeNode)LocalAlloc(LPTR, sizeof(OneTreeNode));
    if (lpnKid)
    {
        // Note that we copy only a mkid. Because we allocate
        // two extra bytes (with 0-init), we'll get a IDL.
        ASSERT(ILIsEmpty(_ILNext(pidl)));
        lpnKid->pidl = ILClone(pidl);
        SHStrDup(pszName, &lpnKid->lpText);
#ifdef DEBUG
        lpnKid->dwDebugSig = OTDEBUG_SIG;
#endif
        lpnKid->lpnParent = lpnParent;
        lpnKid->hdpaKids = KIDSUNKNOWN;
        lpnKid->cChildren = (BYTE)I_CHILDRENCALLBACK;
        lpnKid->iImage = (USHORT)I_IMAGECALLBACK;
        lpnKid->iSelectedImage = (USHORT)I_IMAGECALLBACK;
        lpnKid->cRef = 1;
        lpnKid->dwAttribs = dwAttribs;
        lpnKid->dwLastChanged = 0;

        DebugDumpNode(lpnKid, TEXT("AdoptingKid"));

        lpnKid->fInvalid = TRUE;
        ASSERT(!lpnKid->fHasAttributes);
        
    }

    if (lpnKid->pidl && lpnKid->lpText)
        return lpnKid;
    else
    {
        OTFreeNodeData(lpnKid);
        return NULL;
    }
}

HRESULT AddRoot(LPCITEMIDLIST pidlRoot, LPOneTreeNode *lplpnRoot)
{
    WCHAR szName[MAX_PATH];
    DWORD dwAttribs = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
    HRESULT hr = IEGetNameAndFlags(pidlRoot, SHGDN_INFOLDER | SHGDN_NORMAL, szName, SIZECHARS(szName), &dwAttribs);

    if (SUCCEEDED(hr))
    {
        LPOneTreeNode lpn = OTNCreate(pidlRoot, szName, dwAttribs, s_lpnRoot);

        if (lpn)
        {
            LPOneTreeNode lpnRet = lpn;
            lpn->fRoot = TRUE;

            AdoptKid(s_pshfRoot, &lpnRet, 0, s_lpnRoot->dwLastChanged);
            lpnRet->fMark = 0;

            if (lplpnRoot)
            {
                *lplpnRoot = lpnRet;
                OTAddRef(lpnRet);
            }

            // release now..  if adopt kid took it, it did an addref to lpn
            OTRelease(lpn);
            hr = S_OK;
        } 
    }

    return hr;
}
        
    
UINT AddChildEx(IShellFolder *psfParent, LPOneTreeNode lpnParent,
                          LPCITEMIDLIST pidl, BOOL bAllowDup,
                          LPOneTreeNode *lplpnKid, LPCTSTR pszText)
{
    DWORD dwAttribs = 0;
    LPOneTreeNode lpnKid = NULL;
    INT iPos = 0;
    UINT uReturn;
    DWORD dwLastChecked = 0;

    AssertValidNode(lpnParent);

    // Is it already in the tree?
    if (lpnParent)
    {
        dwLastChecked = lpnParent->dwLastChanged;
        ENTERCRITICAL;
        lpnKid = FindKid(psfParent, lpnParent, pidl, FALSE, &iPos);
        LEAVECRITICAL;
    }

    if (!lpnKid || bAllowDup)
    {
        HRESULT hres = S_OK;    // assume success
        TCHAR szName[MAX_PATH];
        uReturn = ADDCHILD_FAILED; // assume error
        szName[0] = 0;

        // First, ask for the display name of this subfolder.
        if (pszText && *pszText)
        {
            lstrcpyn(szName, pszText, ARRAYSIZE(szName));
        }
        else
        {
            if (psfParent)
            {
                STRRET strret;
                hres = IShellFolder_GetDisplayNameOf(psfParent, pidl,
                    SHGDN_INFOLDER | SHGDN_NORMAL, &strret, 0);
                if (SUCCEEDED(hres))
                    hres = StrRetToBuf(&strret, pidl, szName, ARRAYSIZE(szName));

                // ask about minimal stuff because this could be expensive
                dwAttribs = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
                psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttribs);
            }
            else
            {
                MLLoadString(IDS_DESKTOP, szName, ARRAYSIZE(szName));
            }
        }

        if (SUCCEEDED(hres))
        {
            LPOneTreeNode lpnKid = OTNCreate(pidl, szName, dwAttribs, lpnParent);
            if (lpnKid)
            {
                *lplpnKid = lpnKid;

                if (lpnParent)
                {
                    AdoptKid(psfParent, lplpnKid, iPos, dwLastChecked);
                    (*lplpnKid)->fMark = 0;
                    // release now..  if adopt kid took it, it did an addref to lpnKid
                    OTRelease( lpnKid );

                    if (lpnKid == *lplpnKid)
                        uReturn = ADDCHILD_ADDED;
                    else
                        uReturn = ADDCHILD_EXISTED;
                } 
                else 
                {
                    // this needs to return success if there's no parent.
                    // only onetreeinitialize does this, and it needs the return
                    uReturn = ADDCHILD_ADDED;
                }
            }
        } 
    }
    else
    {
        // this kid still exists, set fMark to 0 so
        // that we don't kill the kid below
        lpnKid->fMark = 0;

        // didn't add
        *lplpnKid = lpnKid;
        uReturn = ADDCHILD_EXISTED;
    }

#ifdef OT_DEBUG
    TraceMsg(DM_TRACE, "OneTree: Addchild retuns %d", uReturn);
#endif
    return uReturn;
}

UINT AddChild(IShellFolder *psfParent, LPOneTreeNode lpnParent,
                          LPCITEMIDLIST pidl, BOOL bAllowDup,
                          LPOneTreeNode *lplpnKid)
{
    return AddChildEx(psfParent, lpnParent, pidl, bAllowDup, lplpnKid, NULL);
}



//
// Notes:   We can't combine this function with _BindFromJunctoinPoint
//          to simplify the code. We need to avoid allocating szPath[]
//          each time we call OTBindToFolder recursively.
//
HRESULT OTBindToFolderEx(LPOneTreeNode lpNode, IShellFolder **ppshf)
{
    HRESULT hres;

    *ppshf = NULL;      // assume error

    AssertValidNode(lpNode);

    if (!IsWindow(s_hwndOT)) 
        s_hwndOT = NULL;

    if (lpNode)
    {
        if (lpNode == s_lpnRoot)
        {
            // Special case for the desktop node
            hres = s_pshfRoot->QueryInterface(IID_IShellFolder, (void **)ppshf);
        }
        else
            hres = OTRealBindToFolder(lpNode, ppshf);
    }
    else
    {
        hres = E_FAIL;
    }
    return hres;
}

IShellFolder *OTBindToFolder(LPOneTreeNode lpnd)
{
    IShellFolder *pshf;
    OTBindToFolderEx(lpnd, &pshf);
    return pshf;
}

HRESULT OTBindToParentFolder(LPOneTreeNode lpNode, IShellFolder **ppsf, LPITEMIDLIST *ppidlChild)
{
    HRESULT hr;
    LPCITEMIDLIST pidlChild;

    *ppsf = NULL;
    *ppidlChild = NULL;

    if (lpNode->fRoot)
    {
        hr = IEBindToParentFolder(lpNode->pidl, ppsf, &pidlChild);
    }
    else
    {
        LPITEMIDLIST pidl = OTCreateIDListFromNode(lpNode->lpnParent);

        if (pidl)
        {
            hr = IEBindToObject(pidl, ppsf);
            ILFree(pidl);
            pidlChild = lpNode->pidl;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pidlChild);
        
        *ppidlChild = ILClone(pidlChild);
        if (!*ppidlChild)
        {
            hr = E_OUTOFMEMORY;
            (*ppsf)->Release();
            *ppsf = NULL;
        }
    }

    return hr;
}
        
HRESULT OTRealBindToFolder(LPOneTreeNode lpNode, IShellFolder **ppshf)
{
    HRESULT hres;
    LPITEMIDLIST pidl;

    *ppshf = NULL;      // assume error

    pidl = OTCreateIDListFromNode(lpNode);
    if (pidl)
    {
        hres = IEBindToObject(pidl, ppshf);
        ASSERT((FAILED(hres) && *ppshf==NULL) || (SUCCEEDED(hres) && *ppshf));
        ILFree(pidl);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

BOOL OTUpdateNodeName(IShellFolder *psf, LPCITEMIDLIST pidlChild, LPOneTreeNode lpNode, BOOL fResort)
{
    STRRET sr;

    ASSERT(lpNode->pidl);
    ASSERT(pidlChild);

    if (SUCCEEDED(IShellFolder_GetDisplayNameOf(psf, pidlChild, SHGDN_INFOLDER | SHGDN_NORMAL, &sr, 0)))
    {
        LPTSTR pszNewName;

        if (SUCCEEDED(StrRetToStr(&sr, pidlChild, &pszNewName)))
        {
            ENTERCRITICAL;
            if (!lpNode->lpText || lstrcmp(pszNewName, lpNode->lpText)) 
            {
                OTSetNodeName(lpNode, pszNewName);

                if (fResort && lpNode->lpnParent && NodeHasKids(lpNode->lpnParent)) 
                {
                    OTCompareInfo ci = {0};
                    // resort in our parent hdpa
                    ci.psf = psf;
                    ci.fRooted = (s_lpnRoot == lpNode->lpnParent);
                    
                    DPA_Sort( lpNode->lpnParent->hdpaKids, (PFNDPACOMPARE)OTCompareNodes, (LPARAM)&ci);
                    lpNode->lpnParent->dwLastChanged = GetTickCount();
                }
            }
            else
                CoTaskMemFree(pszNewName);
            LEAVECRITICAL;
        }
        return TRUE;
    }
    return FALSE;
}

void CheckDestroyHDPAKids(LPOneTreeNode lpnParent)
{
    AssertValidNode(lpnParent);

    //
    // do we have any kids left?
    //
    lpnParent->cChildren = GetKidCount(lpnParent) ? 1 : 0;
    if (!lpnParent->cChildren) {

        lpnParent->cChildren = 0;
        DPA_Destroy(lpnParent->hdpaKids);
        lpnParent->hdpaKids = NOKIDS;

    }
}

//
// nuke all the kids with the ref still set
//
void KillAbandonedKids(LPOneTreeNode lpnParent)
{
    AssertValidNode(lpnParent);

    for (int i = GetKidCount(lpnParent) - 1; i >= 0; i--) 
    {
        ENTERCRITICAL;
        LPOneTreeNode lpnKid = GetNthKid(lpnParent, i);
        if (lpnKid && lpnKid->fMark) 
        {
            if (lpnKid->fRoot)
            {
                lpnKid->fMark = FALSE;
                continue;
            }

            // kill de wabbit
            DPA_DeletePtr(lpnParent->hdpaKids, i);
            lpnParent->dwLastChanged = GetTickCount();
            OTRelease(lpnKid);
        }
        LEAVECRITICAL;
    }

    CheckDestroyHDPAKids(lpnParent);
}

//
//  Sets fHasAttributes = FALSE for this node and all its children.
//
void OTInvalidateAttributeRecursive(LPOneTreeNode lpn)
{
    if (lpn)
    {
        lpn->fHasAttributes = FALSE;

        // If the kids is KIDSUNKNOWN, that's okay.  No point invalidating
        // stuff that didn't even get lazy-evaluated into existence yet.
        if (NodeHasKids(lpn))
        {
            for (int i = GetKidCount(lpn) - 1; i >= 0; i--)
            {
                LPOneTreeNode lpnKid = GetNthKid(lpn, i);
                lpnKid->fValidatePidl = TRUE;
                OTInvalidateAttributeRecursive(lpnKid);
            }
        }
    }
}

void _UpdateNode(IShellFolder *psfParent, LPCITEMIDLIST pidlChild, LPOneTreeNode lpNode)
{
    ASSERTCRITICAL;
    
    if (OTUpdateNodeName(psfParent, pidlChild, lpNode, TRUE)) 
    {
        // SFGAO_HASSUBFOLDER is slow, avoid doing making this call
        if (!lpNode->fHasAttributes || lpNode->fValidatePidl)
        {
            lpNode->dwAttribs = 0;
            DWORD dwAttribs = SFGAO_VALIDATE;

            //
            //  WARNING - SFGAO_VALIDATE requires its own call. - ZekeL 15-DEC-98
            //  the SF implementations will return S_OK if the pidl 
            //  is valid without returning any other attribs.
            //  
            if (!lpNode->fValidatePidl
                || SUCCEEDED(psfParent->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlChild, &dwAttribs)))
            {
                dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_SHARE | 
                            SFGAO_REMOVABLE    | SFGAO_FILESYSANCESTOR | 
                            SFGAO_FILESYSTEM   | SFGAO_CAPABILITYMASK | 
                            SFGAO_COMPRESSED   | SFGAO_NEWCONTENT;
        
                if (SUCCEEDED(psfParent->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlChild, &dwAttribs)))
                    lpNode->dwAttribs = dwAttribs;

                lpNode->fValidatePidl = FALSE;
            }

            if (!(lpNode->dwAttribs & SFGAO_HASSUBFOLDER))
            {
                //  if this bit isnt set then we need to 
                //  make sure that we clean out the node before
                //  we zero out the kid count.  otherwise
                //  those kids are leaked.
                KillKids(lpNode);

                lpNode->cChildren = 0;
            }
            else
                lpNode->cChildren = 1;
        
            lpNode->fHasAttributes = TRUE;
        }
    }

    lpNode->iImage = SHMapPIDLToSystemImageListIndex(psfParent,
        pidlChild, &lpNode->iSelectedImage);
}

void ForceNode(LPOneTreeNode lpNode)
{
    AssertValidNode(lpNode);

    IShellFolder *psfParent;
    LPITEMIDLIST pidlFree;

    //
    //  WARNING must use OTBindToParentFolder here - ZekeL 13-JUL-99
    //  to support ROOTed explorers in the onetree.
    //
    if (SUCCEEDED(OTBindToParentFolder(lpNode, &psfParent, &pidlFree)))
    {
        LPCITEMIDLIST pidlChild = pidlFree;
        ENTERCRITICAL;
        if (lpNode->fValidatePidl && !lpNode->fRoot)
        {
            //  we may have gotten an UPDATEITEM
            //  so the pidl may need to be replaced.
            LPITEMIDLIST pidl;

            ASSERT(pidlChild->mkid.cb == lpNode->pidl->mkid.cb && 0 == memcmp(pidlChild, lpNode->pidl, pidlChild->mkid.cb));
            
            if (SUCCEEDED(SHGetRealIDL(psfParent, lpNode->pidl, &pidl)))
            {
                ILFree(lpNode->pidl);
                lpNode->pidl = pidl;
                pidlChild = pidl;
            }
        }

        if (pidlChild)
            _UpdateNode(psfParent, pidlChild, lpNode);

        LEAVECRITICAL;

        psfParent->Release();
        ILFree(pidlFree);
    }
}


BOOL SearchForKids(HWND hwndOwner, LPOneTreeNode lpnParent, LPEnumInfo pei, BOOL fInteractive)
{
    IShellFolder *psfParent;
    IEnumIDList *penum;
    LPITEMIDLIST pidl;
    LPOneTreeNode lpnKid;
    BOOL fSuccess = TRUE;
    ULONG celt;
    int iDestroyCount = 0;
    SHELLSTATE ss = {0};
    BOOL fAllowTimeout = (pei && pei->fAllowTimeout);
    DWORD dwStartTime;
    
    AssertValidNode(lpnParent);
    
    // Have a seat, this could take a while
    psfParent = OTBindToFolder(lpnParent);
    if (!psfParent)
    {
        ASSERT(FALSE);
        fSuccess = FALSE;
        goto Error1;
    }
    
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    
    // decide if we should supress the UI
    
    if (pei && !fInteractive)
        hwndOwner = NULL;
    
    if (FAILED(psfParent->EnumObjects(hwndOwner,
        ss.fShowAllObjects ? SHCONTF_FOLDERS|SHCONTF_INCLUDEHIDDEN : SHCONTF_FOLDERS,
        &penum)))
    {
        // Notes: It means the enumeration is either canceled by the
        //  user, or something went wrong.
        //
        // Return the state of lpnParent->hdpaKids to KIDSUNKNOWN.
        //
        KillKids(lpnParent);
        lpnParent->hdpaKids = KIDSUNKNOWN;
        lpnParent->fInvalid = TRUE;
        
        TraceMsg(DM_TRACE, "ca TR - SearchForKids ISF::EnumObjects failed");
        
        fSuccess = FALSE;
        goto Error2;
    }
    
    // after this point, we're gonna succeed enough that we can turn off the invalid flag.
    lpnParent->fInvalid = FALSE;
    
    if (NodeHasKids(lpnParent)) 
    {
        for (int i = GetKidCount(lpnParent) - 1; i >= 0; i--) 
        {
            lpnKid = GetNthKid(lpnParent, i);
            if (lpnKid) 
            {
                lpnKid->fMark = 1;
            }
        }
    }
    
    // Build list of all the items
    
    // Enumerate over all the sub folders within this folder.
    // BUGBUG: Call GetMaxIDSize!
    //
    if (lpnParent->hdpaKids == KIDSUNKNOWN)
        lpnParent->hdpaKids = NOKIDS;
    
    if (fAllowTimeout) 
    {
        dwStartTime = GetTickCount();
        if (!g_dwEnumTimeout) 
        {
            DWORD dwSize = SIZEOF(g_dwEnumTimeout);
            // not set yet, read it from the registry.
            if (SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("EnumTimeout"), NULL, &g_dwEnumTimeout, &dwSize) != ERROR_SUCCESS || 
                g_dwEnumTimeout == 0) 
            {
                g_dwEnumTimeout = (3 * 1000);    // 3 seconds
            }
        }
    }
    
    while (penum->Next(1, &pidl, &celt) == S_OK && celt == 1)
    {
        AddChild(psfParent, lpnParent, pidl, FALSE, &lpnKid);
        SHFree(pidl);
        
        if (fAllowTimeout &&
            (GetTickCount() - dwStartTime > g_dwEnumTimeout)) {
            TraceMsg(DM_TRACE, "Enumeration timed out!");
            pei->fTimedOut = TRUE;
            // mark that the parent isn't completely valid yet.
            lpnParent->fInvalid = TRUE;
            break;
        }
    }
    
    
    {
        ENTERCRITICAL;
        if (NodeHasKids(lpnParent))
            KillAbandonedKids(lpnParent);
        else
            lpnParent->cChildren = 0;
        LEAVECRITICAL;
    }
    
    // Tidyup.
    penum->Release();
    
Error2:
    psfParent->Release();
    
Error1:
    return fSuccess;
}

LPOneTreeNode FindNearestNodeFromIDList(LPCITEMIDLIST pidl, UINT uFlags)
{
    LPOneTreeNode lpnd = NULL;
    LPITEMIDLIST pidlCopy = ILClone(pidl);
    if (pidlCopy)
    {
        while (ILRemoveLastID(pidlCopy))
        {
            uFlags &= ~OTGNF_NEARESTMATCH; // so we won't recurse forever
            lpnd = OTGetNodeFromIDList(pidlCopy, uFlags);
            if (lpnd) {
                OTRelease(lpnd);
                break;
            }
        }
        ILFree(pidlCopy);
    }
    return lpnd;
}

void OTInvalidateRoot()
{
    s_lpnRoot->fInvalid = TRUE;
}

void OTAbandonKid(LPOneTreeNode lpnParent, LPOneTreeNode lpnKid)
{
    if (lpnParent && lpnKid && NodeHasKids(lpnParent)) {
        LPOneTreeNode lpn;
        IShellFolder *psfParent;
        int i;

        // invalidate the new node's parent to get any children flags right
        lpnParent->fInvalid = TRUE;

        // remove the node from it's parent's list
        psfParent = OTBindToFolder(lpnParent);
        if (psfParent) {

            // the crit section must be done up here
            // because we need to make sure that the index i is correct
            // when we go to delete it
            ENTERCRITICAL;
            lpn = FindKid(psfParent, lpnParent, lpnKid->pidl, FALSE, &i);


            if (lpn) {
#ifdef OT_DEBUG
                TraceMsg(DM_TRACE, "OneTree: QuickRename found old node to nuke");
#endif
                DPA_DeletePtr(lpnParent->hdpaKids, i);
                lpnParent->dwLastChanged = GetTickCount();
                OTRelease(lpn);

                CheckDestroyHDPAKids(lpnParent);
            }

            LEAVECRITICAL;
            psfParent->Release();
        }
    }
}

// if the RENAMEFOLDER event was a rename within the same
// this whole things should be in a critical section from the call of
// DoHandleFileSysChange
BOOL TryQuickRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    LPOneTreeNode lpnSrc;
    BOOL fRet = FALSE;

    // This can happen when a folder is moved from a "rooted" Explorer outside
    // of the root
    if (!pidl || !pidlExtra)
    {
        return(FALSE);
    }

    // this one was deleted
    lpnSrc = _GetNodeFromIDList(pidl, 0, NULL);
    if (!lpnSrc) 
    {
        return FALSE;
    } 
    else if (lpnSrc == s_lpnRoot) 
    {
        OTInvalidateRoot();
        return TRUE;
    } 
    else 
    {
        // this one was created
        LPITEMIDLIST pidlParent = ILCloneParent(pidlExtra);
        if (pidlParent) 
        {
            // if the parent isn't created yet, let's not bother
            LPOneTreeNode lpnDestParent = _GetNodeFromIDList(pidlParent, 0, NULL);
            ILFree(pidlParent);

            if (lpnDestParent) 
            {
                LPITEMIDLIST pidlLast = OTGetRealFolderIDL(lpnDestParent, ILFindLastID(pidlExtra));
                if (pidlLast) 
                {
                    IShellFolder *psf = OTBindToFolder(lpnDestParent);
                    if (psf) 
                    {
                        LPOneTreeNode pnKid;

                        OTAddRef(lpnSrc); // addref because AdoptKid doesn't and OTAbandonKid releases

                        // remove lpnSrc from its parent's list.
                        OTAbandonKid(lpnSrc->lpnParent, lpnSrc);

                        // invalidate the new node's parent to get any children flags right
                        lpnDestParent->fInvalid = TRUE;

                        // Force the attributes of the entire tree
                        // rooted at the new node to be recompute,
                        // since moving a directory can change stuff like
                        // SFGAO_SHARE and SFGAO_COMPRESSED.
                        OTInvalidateAttributeRecursive(lpnSrc);

                        OTFreeNodeData(lpnSrc);
                        lpnSrc->pidl = pidlLast;
                        lpnSrc->lpnParent = lpnDestParent;
                        OTUpdateNodeName(psf, pidlLast, lpnSrc, FALSE);
                        pnKid = lpnSrc;
                        AdoptKid(psf, &pnKid, -1, 0);
                        OTRelease(lpnSrc);
                        fRet = TRUE;

                        psf->Release();

                    } 
                    else 
                    {
                        ILFree(pidlLast);
                    }
                }
            }
        }
    }
    return fRet;
}


// avoids eating stack
BOOL DoHandleFileSysChange(LONG lNotification, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    LPOneTreeNode lpNode = NULL;
    LPITEMIDLIST pidlClone;
    BOOL fRet = FALSE;

    switch(lNotification)
    {
    case SHCNE_RENAMEFOLDER:
        // first try to just swap the nodes if it's  true rename (not a move)
        fRet = TryQuickRename(pidl, pidlExtra);
        if (fRet)
            break;

        // Rename is special.  We need to invalidate both
        // the pidl and the pidlExtra. so we call ourselves
        fRet = DoHandleFileSysChange(0, pidlExtra, NULL);

        // Handle the rooted case where this pidl may be NULL!
        if (!pidl)
            break;
        // fall through

    case SHCNE_RMDIR:
        if (ILIsEmpty(pidl)) {
            // we've deleted the desktop dir.
            lpNode = s_lpnRoot;
            OTInvalidateRoot();
            break;
        }

        if(lpNode = _GetNodeFromIDList(pidl, 0, NULL))
        {
            lpNode = OTGetParent(lpNode);
            break;
        }
        // fall through

    case 0:
    case SHCNE_MKDIR:
    case SHCNE_DRIVEADD:
    case SHCNE_DRIVEREMOVED:
        if (!pidl)
            break;

        pidlClone = ILClone(pidl);
        if (pidlClone)
        {
            ILRemoveLastID(pidlClone);
            lpNode = _GetNodeFromIDList(pidlClone, 0, NULL);
            ILFree(pidlClone);
        }
        break;

    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
        fRet = TRUE;
        lpNode = _GetNodeFromIDList(pidl, 0, NULL);
        if (lpNode)
            lpNode = lpNode->lpnParent;
        break;

    case SHCNE_DRIVEADDGUI:
        fRet = TRUE;
        // fall through
    case SHCNE_UPDATEITEM:
    case SHCNE_NETSHARE:
    case SHCNE_NETUNSHARE:
    case SHCNE_ATTRIBUTES:
    case SHCNE_UPDATEDIR:
        lpNode = _GetNodeFromIDList(pidl, 0, NULL);
        if (lpNode)
        {
            lpNode->fValidatePidl = TRUE;
            lpNode->fHasAttributes = FALSE;

            // SHCNE_UPDATEDIR is the generic "Something happened
            // somewhere in this tree but I collapsed them all"
            // notification, so if we get one, we have to invalidate
            // the entire tree.
            if (lNotification == SHCNE_UPDATEDIR)
                OTInvalidateAttributeRecursive(lpNode);
        }
        break;

    case SHCNE_SERVERDISCONNECT:
        // nuke all our kids and mark ourselves invalid
        lpNode = _GetNodeFromIDList(pidl, 0, NULL);
        if (lpNode && NodeHasKids(lpNode))
        {
            int i;

            for (i = GetKidCount(lpNode) -1; i >= 0; i--) {
                OTRelease(GetNthKid(lpNode, i));
            }
            DPA_Destroy(lpNode->hdpaKids);
            lpNode->hdpaKids = KIDSUNKNOWN;
            lpNode->fInvalid = TRUE;
            lpNode->fHasAttributes = FALSE;
        } 
        else 
        {
            lpNode = NULL;
        }
        fRet = TRUE;
        break;

    case SHCNE_ASSOCCHANGED:
        fRet = TRUE;
        break;

    case SHCNE_UPDATEIMAGE:
        if (pidl)
        {
            int iImage;
            if ( pidlExtra )
            {
                iImage = SHHandleUpdateImage( pidlExtra );
                if ( iImage == -1 )
                {
                    break;
                }
            }
            else
            {
                iImage = *(int UNALIGNED *)((BYTE*)pidl + 2);
            }
            
            DoInvalidateAll(s_lpnRoot, iImage);
            TraceMsg(DM_TRACE, "SHCNE_UPDATEIMAGE :  %d ", iImage);
            fRet = TRUE;
        }
        break;
    }

    if (lpNode)
    {
        lpNode->fInvalid = TRUE;
        lpNode->fHasAttributes = FALSE;
        fRet = TRUE;
    }

    return fRet;
}


void HandleFileSysChange(LONG lNotification, LPITEMIDLIST*lplpidl)
{
    LPITEMIDLIST pidl1 = lplpidl[0];
    LPITEMIDLIST pidl2 = lplpidl[1];

    ENTERCRITICAL;
    DoHandleFileSysChange(lNotification, pidl1, pidl2);
    LEAVECRITICAL;

}

BOOL OTSubNodeCount(HWND hwndOwner, LPOneTreeNode lpNode, LPEnumInfo pei, UINT* pcnd, BOOL fInteractive)
{
    BOOL fSuccess = TRUE;
    AssertValidNode(lpNode);
    *pcnd = 0;

    if (lpNode)
    {
        if (lpNode->hdpaKids == KIDSUNKNOWN || lpNode->fInvalid)
        {
            fSuccess = SearchForKids(hwndOwner, lpNode, pei, fInteractive);
        }

        if (NodeHasKids(lpNode))
        {
            *pcnd = GetKidCount(lpNode);
        }
    }
    else
    {
        fSuccess = FALSE;
    }

    return fSuccess;
}

LPOneTreeNode OTGetNthSubNode(HWND hwndOwner, LPOneTreeNode lpnd, UINT i)
{
    UINT cnd;
    AssertValidNode(lpnd);

    LPOneTreeNode lpNode = NULL;

    // OTSubNodeCount will validate
    OTSubNodeCount(hwndOwner, lpnd, NULL, &cnd, FALSE);

    ENTERCRITICAL;

    // Re-validate inside critical section (without UI) in
    // case another thread jumped in and killed the kids
    OTSubNodeCount(NULL, lpnd, NULL, &cnd, FALSE);

    if (i < cnd)
    {
        lpNode = GetNthKid(lpnd, i);
        if (lpNode && !lpNode->fRoot) 
            OTAddRef(lpNode);
        else
            lpNode = NULL;
    }

    LEAVECRITICAL;

    return lpNode;
}

BOOL OTIsBold(LPOneTreeNode lpNode)
{
    if (!lpNode->fHasAttributes)
        ForceNode(lpNode);

    return lpNode->dwAttribs & SFGAO_NEWCONTENT;
}

BOOL OTIsCompressed(LPOneTreeNode lpNode)
{
    // Only NT supports per-file compression
    if (g_fRunningOnNT && !lpNode->fHasAttributes)
        ForceNode(lpNode);

    return lpNode->dwAttribs & SFGAO_COMPRESSED;
}

void OTNodeFillTV_ITEM(LPOneTreeNode lpNode, LPTV_ITEM lpItem)
{
    ASSERT(lpNode);
    AssertValidNode(lpNode);

    if (lpNode->fInvalid ||
        lpNode->iImage == (USHORT)I_IMAGECALLBACK ||
        lpNode->iSelectedImage == (USHORT)I_IMAGECALLBACK)
        ForceNode(lpNode);


    if (lpItem->mask & TVIF_TEXT) {
        OTGetNodeName(lpNode, lpItem->pszText, lpItem->cchTextMax);
    }

    if (lpItem->mask & TVIF_CHILDREN) {
        if (lpNode->fInvalid || lpNode->hdpaKids == KIDSUNKNOWN)
            lpItem->cChildren = lpNode->cChildren;
        else if (lpNode->hdpaKids == NOKIDS)
            lpItem->cChildren = 0;
        else
            lpItem->cChildren = GetKidCount(lpNode) ? 1 : 0;
    }

    if (lpItem->mask & TVIF_IMAGE) {
        lpItem->iImage = lpNode->iImage;
    }

    if (lpItem->mask & TVIF_SELECTEDIMAGE) {
        lpItem->iSelectedImage = lpNode->iSelectedImage;
    }
}

BOOL OTHasSubFolders(LPOneTreeNode lpNode)
{
    ASSERT(lpNode);
    AssertValidNode(lpNode);

    if (lpNode->fInvalid) {
        ForceNode(lpNode);
    }
    return lpNode->cChildren;
}

void OTUnregister(HWND hwnd)
{
    PostMessage(s_hwndOT, WM_OT_UNREGISTER, 0, (LPARAM)hwnd);
}

void OTHwndInitialize()
{
    s_hwndOT = CreateWindow(c_szOTClass, NULL, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, HINST_THISDLL, NULL);
    if (s_hwndOT) 
    {
        SHChangeNotifyEntry fsne = { &s_idlNULL, TRUE };     // Global & recursive.
        s_uFSRegisterID = RegisterNotify(s_hwndOT, WM_OT_FSNOTIFY, (LPITEMIDLIST)&s_idlNULL, 
                                         SHCNE_ALLEVENTS & ~(SHCNE_FREESPACE|SHCNE_CREATE|SHCNE_DELETE|SHCNE_RENAMEITEM),
                                         SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_CreateSuspended, TRUE);
    }
}

void OTRegister(HWND hwnd)
{
    ENTERCRITICAL;
    // If the refcount is zero, we need to initialize ourselves.
    if (!g_OTRegRefCount) {
        OneTree_Initialize();
    }
    InterlockedIncrement(&g_OTRegRefCount);

    if (!s_hwndOT) 
    {
        OTHwndInitialize();
    }
    LEAVECRITICAL;
    
}

void _OTUnregister(HWND hwnd)
{
    ENTERCRITICAL;
    if (0 == InterlockedDecrement(&g_OTRegRefCount)) 
    {       
        //  we no longer cache the onetree
        //  in between explorer sessions.
        //  if we want to enable this, we should
        //  try to move the window proc to the explorer
        OneTree_Terminate();
    }
    LEAVECRITICAL;

}

// this validates that the subpidl is a folder pidl and returns the full (non-simple)
// pidl for it
// get the pidlReal because we might have a simple pidl

LPITEMIDLIST OTGetRealFolderIDL(LPOneTreeNode lpnParent, LPCITEMIDLIST pidlSimple)
{
    IShellFolder *psfParent;
    HRESULT hres;
    LPITEMIDLIST pidlReal = NULL;

    ASSERT(_ILNext(pidlSimple)->mkid.cb==0);
    ASSERT(lpnParent);
    AssertValidNode(lpnParent);

    if (lpnParent == s_lpnRoot && ILIsRooted(pidlSimple))
    {
        pidlReal = ILClone(pidlSimple);
    }
    else
    {
        hres = OTBindToFolderEx(lpnParent, &psfParent);
        if (psfParent)
        {
            hres = SHGetRealIDL(psfParent, pidlSimple, &pidlReal);
            if (SUCCEEDED(hres) && pidlReal)
            {
                DWORD dwAttribs = SFGAO_FOLDER | SFGAO_BROWSABLE;

                hres = psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlReal, &dwAttribs);
                if (FAILED(hres) || !(dwAttribs & (SFGAO_FOLDER | SFGAO_BROWSABLE)))
                {
                    ILFree(pidlReal);
                    pidlReal = NULL;
                }
            }

            psfParent->Release();
        } 
        else 
        {
            TraceMsg(DM_ERROR, "Unable to bind to folder %s in OTAddSubFolder", TEXT("?"));
        }
    }
    return pidlReal;
}

HRESULT OTAddSubFolder(LPOneTreeNode lpNode, LPCITEMIDLIST pidl,
        DWORD dwFlags, LPOneTreeNode *ppndOut)
{
    IShellFolder *psfParent;
    LPOneTreeNode lpNewNode = NULL;
    HRESULT hres = OTBindToFolderEx(lpNode, &psfParent);
    
    ASSERT(lpNode != s_lpnRoot || !ILIsRooted(pidl));
    
    if (SUCCEEDED(hres)) 
    {
        LPITEMIDLIST pidlFree = NULL;

        if (dwFlags & OTASF_MAYBESIMPLE)
        {
             pidl = pidlFree = OTGetRealFolderIDL(lpNode, pidl);
        }

        hres = E_FAIL;
        if (pidl) 
        {
            if (AddChild(psfParent, lpNode, pidl, dwFlags & OTASF_ALLOWDUP, &lpNewNode) != ADDCHILD_FAILED) 
                hres = NOERROR;

            ILFree(pidlFree);
        }

        psfParent->Release();
    }

    if (ppndOut) 
    {
        *ppndOut = lpNewNode;
        if (lpNewNode)
            OTAddRef(lpNewNode);
    }

    return hres;
}


void OTGetImageIndex(LPOneTreeNode lpNode, int *lpiImage, int *lpiSelectedImage)
{
    ASSERT(lpNode);
    AssertValidNode(lpNode);

    if (lpNode->fInvalid ||
        lpNode->iImage == (USHORT)I_IMAGECALLBACK ||
        lpNode->iSelectedImage == (USHORT)I_IMAGECALLBACK)
    {
        
        IShellFolder *psfParent;
        LPITEMIDLIST pidlChild;

        if (SUCCEEDED(OTBindToParentFolder(lpNode, &psfParent, &pidlChild)))
        {
            lpNode->iImage = SHMapPIDLToSystemImageListIndex(psfParent,
                pidlChild, &lpNode->iSelectedImage);

            psfParent->Release();
            ILFree(pidlChild);
        }
        // this gets the sharing info aswell
        ForceNode(lpNode);
    }

    if (lpiImage)
        *lpiImage = lpNode->iImage;

    if (lpiSelectedImage)
        *lpiSelectedImage = lpNode->iSelectedImage;
}

LPOneTreeNode OTGetNodeFromIDListEx(LPCITEMIDLIST pidl, UINT uFlags, HRESULT* phresOut)
{
    ENTERCRITICAL;

    LPOneTreeNode lpNode = _GetNodeFromIDList(pidl, uFlags, phresOut);
    if (!lpNode && (uFlags & OTGNF_NEARESTMATCH))
    {
        lpNode = FindNearestNodeFromIDList(pidl, uFlags);
    }

    if (lpNode)
        OTAddRef(lpNode);

    LEAVECRITICAL;

    return lpNode;
}

//
// Replacement of OTGetPathFromNode
//
LPITEMIDLIST OTCreateIDListFromNode(LPOneTreeNode lpn)
{
    LPITEMIDLIST pidl = NULL;

    // Walk backwards to construct the path.
    for (; lpn; lpn = lpn->lpnParent)
    {
        LPCITEMIDLIST pidlNext;

        ENTERCRITICAL;
        if (lpn == s_lpnRoot)
            pidlNext = &s_idlNULL;
        else
        {
            if (lpn->fValidatePidl)
            {
                ForceNode(lpn);
            }
                
            pidlNext = lpn->pidl;
        }

        if (pidlNext) 
        {
            pidl = ILAppendID(pidl, &pidlNext->mkid, FALSE);
        }

        LEAVECRITICAL;
        if (!pidl || ! pidlNext || lpn == s_lpnRoot)
            break;
    }

    return pidl;
}


//
// Replacement of GetNodeFromPath
//
LPOneTreeNode _GetNodeFromIDList(LPCITEMIDLIST pidlFull, UINT uFlags, HRESULT *phresOut)
{
    LPCITEMIDLIST pidl;
    LPOneTreeNode lpNode = s_lpnRoot;
    HRESULT hres = S_OK;
    BOOL fAddRoot = ILIsRooted(pidlFull);

    ASSERTCRITICAL;
    
    if (!pidlFull)
    {
        return(NULL);
    }

    //
    // REVIEW: We can eliminate ILCloneFirst by copying each mkid to pidlT.
    //
    for (pidl = pidlFull; !ILIsEmpty(pidl) && lpNode; pidl = _ILNext(pidl))
    {
        LPOneTreeNode lpnParent = lpNode;
        LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);

        AssertValidNode(lpnParent);
        lpNode = NULL;  // assume error
        if (pidlFirst)
        {
            IShellFolder *psfParent = OTBindToFolder(lpnParent);
            if (psfParent)
            {
                INT i;


                lpNode = FindKid(psfParent, lpnParent, pidlFirst, uFlags & OTGNF_VALIDATE, &i);

                if (!lpNode && (uFlags & OTGNF_TRYADD))
                {
                    if (fAddRoot)
                    {
                        hres = AddRoot(pidlFirst, &lpNode);
                    }
                    else
                    {
                        TraceMsg(DM_TRACE, "Onetree: Unable to Find %s", (LPTSTR)pidl->mkid.abID);
                        // doing no notify for right now because
                        // several notify handlers call this.
                        // just an efficiency thing
                        hres = OTAddSubFolder(lpnParent, pidlFirst, OTASF_MAYBESIMPLE, &lpNode);
                    }
                    OTRelease(lpNode);
                }
                psfParent->Release();
            }
            ILFree(pidlFirst);
        }
        fAddRoot = FALSE;
    }

    if (phresOut) {
        TraceMsg(DM_TRACE, "ca TR - _GetNodeFromIDList returning %x as *phresOut", hres);
        *phresOut = hres;
    }

    return lpNode;
}

int CALLBACK OTTreeViewCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPOneTreeNode lpn1 = (LPOneTreeNode)lParam1;
    LPOneTreeNode lpn2 = (LPOneTreeNode)lParam2;
    IShellFolder *psfParent = (IShellFolder *)lParamSort;
    HRESULT hres;

    AssertValidNode(lpn1);
    AssertValidNode(lpn2);

    //  roots are never added to the 
    //  treeview in explband, so we dont need
    //  to worry about sorting them there.
    ASSERT(!lpn1->fRoot && !lpn2->fRoot);
    
    hres = psfParent->CompareIDs(0, lpn1->pidl, lpn2->pidl);

    if (FAILED(hres))
        return(0);

    return (short)HRESULT_CODE(hres);
}
