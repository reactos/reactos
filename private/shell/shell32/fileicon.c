//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1992
//
// File: fileicon.c
//
//---------------------------------------------------------------------------
#include "shellprv.h"
#pragma  hdrstop
#include "ovrlaymn.h"
#include "fstreex.h"

// REVIEW: More clean up should be done.

BOOL _ShellImageListInit(int cxIcon, int cyIcon, int cxSmIcon, int cySmIcon, UINT flags, BOOL fRestore);
BOOL _ShellImageListTerm(void);


// global shell image lists owned by shelldll

#pragma data_seg(DATASEG_SHARED)

HIMAGELIST g_himlIcons = NULL;            // ImageList of large icons
HIMAGELIST g_himlIconsSmall = NULL;       // ImageList of small icons
int        g_ccIcon = 0;                // color depth of ImageLists
int        g_MaxIcons = DEF_MAX_ICONS;  // panic limit for icons in cache
int        g_lrFlags = 0;

int g_cxIcon = 0;
int g_cyIcon = 0;
int g_cxSmIcon = 0;
int g_cySmIcon = 0;


#pragma data_seg()

TCHAR const g_szMaxCachedIcons[]      = TEXT("Max Cached Icons");
TCHAR const g_szShellIconSize[]       = TEXT("Shell Icon Size");
TCHAR const g_szShellSmallIconSize[]  = TEXT("Shell Small Icon Size");
TCHAR const g_szShellIconDepth[]      = TEXT("Shell Icon Bpp");
TCHAR const g_szShellIcons[]          = TEXT("Shell Icons");
TCHAR const g_szD[]                   = TEXT("%d");

//
// System imagelist - Don't change the order of this list.
// If you need to add a new icon, add it to the end of the
// array, and update shellp.h.
//
UINT const c_SystemImageListIndexes[] = { IDI_DOCUMENT,
                                          IDI_DOCASSOC,
                                          IDI_APP,
                                          IDI_FOLDER,
                                          IDI_FOLDEROPEN,
                                          IDI_DRIVE525,
                                          IDI_DRIVE35,
                                          IDI_DRIVEREMOVE,
                                          IDI_DRIVEFIXED,
                                          IDI_DRIVENET,
                                          IDI_DRIVENETDISABLED,
                                          IDI_DRIVECD,
                                          IDI_DRIVERAM,
                                          IDI_WORLD,
                                          IDI_NETWORK,
                                          IDI_SERVER,
                                          IDI_PRINTER,
                                          IDI_MYNETWORK,
                                          IDI_GROUP,

                                          IDI_STPROGS,
                                          IDI_STDOCS,
                                          IDI_STSETNGS,
                                          IDI_STFIND,
                                          IDI_STHELP,
                                          IDI_STRUN,
                                          IDI_STSUSPEND,
                                          IDI_STEJECT,
                                          IDI_STSHUTD,

                                          IDI_SHARE,
                                          IDI_LINK,
                                          IDI_SLOWFILE,
                                          IDI_RECYCLER,
                                          IDI_RECYCLERFULL,
                                          IDI_RNA,
                                          IDI_DESKTOP,

                                          IDI_CPLFLD,
                                          IDI_STSPROGS,
                                          IDI_PRNFLD,
                                          IDI_STFONTS,
                                          IDI_STTASKBR,

                                          IDI_CDAUDIO,
                                          IDI_TREE,
                                          IDI_STCPROGS,
                                          IDI_STFAV,
                                          IDI_STLOGOFF,
                                          IDI_STFLDRPROP,
                                          IDI_WINUPDATE

#ifdef WINNT // hydra specific stuff
                                          ,IDI_MU_SECURITY,
                                          IDI_MU_DISCONN
#endif
                                          };



int GetRegInt(HKEY hk, LPCTSTR szKey, int def)
{
    DWORD cb;
    TCHAR ach[20];

    if (hk == NULL)
        return def;

    ach[0] = 0;
    cb = SIZEOF(ach);
    SHQueryValueEx(hk, szKey, NULL, NULL, (LPBYTE)ach, &cb);

    if (ach[0] >= TEXT('0') && ach[0] <= TEXT('9'))
        return (int)StrToLong(ach);
    else
        return def;
}

//
// get g_MaxIcons from the registry, returning TRUE if it has changed
//
BOOL QueryNewMaxIcons(void)
{
    int MaxIcons, OldMaxIcons;

    MaxIcons = GetRegInt(g_hklmExplorer, g_szMaxCachedIcons, DEF_MAX_ICONS);

    if (MaxIcons < 0)
        MaxIcons = DEF_MAX_ICONS;

    OldMaxIcons = InterlockedExchange(&g_MaxIcons, MaxIcons);

    return (OldMaxIcons != MaxIcons);
}

//
// Initializes shared resources for Shell_GetIconIndex and others
//
BOOL WINAPI FileIconInit( BOOL fRestoreCache )
{
    BOOL fNotify = FALSE;
    BOOL fInit = FALSE;
    HKEY hkey;
    int cxIcon, cyIcon, ccIcon, cxSmIcon, cySmIcon, res;

#ifndef WINNT
    // Try always restoring cache because of:
    //   BUGBUG (kurte): there appears to be a race condition on first boot
    //   to initialize properly.  This started happening after our 3rd NT
    //   merge.
    //
    // BOBDAY: Don't restore the cache always, it makes every NT process
    // have a complete copy of the shell's icon cache (a very bad thing).

    fRestoreCache=TRUE;
#endif

    QueryNewMaxIcons(); // in case the size of the icon cache has changed

    ccIcon   = 0;
    cxIcon   = GetSystemMetrics(SM_CXICON);
    cyIcon   = GetSystemMetrics(SM_CYICON);
////cxSmIcon = GetSystemMetrics(SM_CXSMICON);
////cySmIcon = GetSystemMetrics(SM_CYSMICON);
    cxSmIcon = GetSystemMetrics(SM_CXICON) / 2;
    cySmIcon = GetSystemMetrics(SM_CYICON) / 2;

    //
    //  get the user prefered icon size (and color depth) from the
    //  registry.
    //
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_METRICS, &hkey) == 0)
    {
        cxIcon   = GetRegInt(hkey, g_szShellIconSize, cxIcon);
        cxSmIcon = GetRegInt(hkey, g_szShellSmallIconSize, cxSmIcon);
        ccIcon   = GetRegInt(hkey, g_szShellIconDepth, ccIcon);

        cyIcon   = cxIcon;      // icons are always square
        cySmIcon = cxSmIcon;

        RegCloseKey(hkey);
    }

    res = (int)GetCurColorRes();

    if (ccIcon > res)
        ccIcon = 0;

    if (res <= 8)
        ccIcon = 0; // wouldn't have worked anyway

#if 0
    //
    // use a 8bpp imagelist on a HiColor device iff we will
    // be stretching icons.
    //
    if (res > 8 && ccIcon == 0 && (cxIcon != GetSystemMetrics(SM_CXICON) ||
         cxSmIcon != GetSystemMetrics(SM_CXICON)/2))
    {
        ccIcon = 8;
    }
#endif

    ENTERCRITICAL;

    //
    // if we already have a icon cache make sure it is the right size etc.
    //
    if (g_himlIcons)
    {
        // we now support changing the colour depth...
        if (g_cxIcon   == cxIcon &&
            g_cyIcon   == cyIcon &&
            g_cxSmIcon == cxSmIcon &&
            g_cySmIcon == cySmIcon &&
            g_ccIcon   == ccIcon)
        {
            fInit = TRUE;
            goto Exit;
        }

        FlushIconCache();
        FlushFileClass();

        // make sure every one updates.
        fNotify = TRUE;
    }

    // if we are the desktop process (explorer.exe), then force us to re-init the cache, so we get
    // the basic set of icons in the right order....
    if ( !fRestoreCache && g_himlIcons && GetInProcDesktop() )
    {
        fRestoreCache = TRUE;
    }
    
    g_cxIcon   = cxIcon;
    g_cyIcon   = cyIcon;
    g_ccIcon   = ccIcon;
    g_cxSmIcon = cxSmIcon;
    g_cySmIcon = cySmIcon;

    if (res > 4 && g_ccIcon <= 4)
        g_lrFlags = LR_VGACOLOR;
    else
        g_lrFlags = 0;

    DebugMsg(DM_TRACE, TEXT("IconCache: Size=%dx%d SmSize=%dx%d Bpp=%d"), cxIcon, cyIcon, cxSmIcon, cySmIcon, (ccIcon&ILC_COLOR));

    if (g_iLastSysIcon == 0)        // Keep track of which icons are perm.
    {
        if (fRestoreCache)
            g_iLastSysIcon = II_LASTSYSICON;
        else
            g_iLastSysIcon = (II_OVERLAYLAST - II_OVERLAYFIRST) + 1;
    }

    // try to restore the icon cache (if we have not loaded it yet)
    if (g_himlIcons != NULL || !fRestoreCache || !_IconCacheRestore(g_cxIcon, g_cyIcon, g_cxSmIcon, g_cySmIcon, g_ccIcon))
    {
        if (!_ShellImageListInit(g_cxIcon, g_cyIcon, g_cxSmIcon, g_cySmIcon, g_ccIcon, fRestoreCache))
            goto Exit;
    }

    fInit = TRUE;

Exit:
    LEAVECRITICAL;

    if (fInit && fNotify)
    {
        DebugMsg(DM_TRACE, TEXT("IconCache: icon size has changed sending SHCNE_UPDATEIMAGE(-1)..."));
        SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)-1, NULL);
    }

    return fInit;
}

void FileIconTerm(void)
{
    ENTERCRITICAL;

    _ShellImageListTerm();

    LEAVECRITICAL;
}
/*************************************************************************
 *************************************************************************/

BOOL _ShellImageListInit(int cxIcon, int cyIcon, int cxSmIcon, int cySmIcon, UINT flags, BOOL fRestore)
{
    int  i;
    IShellIconOverlayManager *psiom;
    ASSERTCRITICAL;

    //
    // Check if we need to create a mirrored imagelist. [samera]
    //
    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
        flags |= ILC_MIRROR;
    }
    

    if (g_himlIcons == NULL) 
    {
        g_himlIcons = ImageList_Create(cxIcon, cyIcon, ILC_MASK|ILC_SHARED|flags, 0, 32);
    }
    else
    {
        // set the flags incase the colour depth has changed...
        // ImageList_setFlags already calls ImageList_remove on success
        if (!ImageList_SetFlags( g_himlIcons, ILC_MASK|ILC_SHARED|flags ))
        {
            // Couldn't change flags; tough.  At least remove them all.
            ImageList_Remove(g_himlIcons, -1);
        }
        ImageList_SetIconSize(g_himlIcons, cxIcon, cyIcon);
    }

    if (g_himlIcons == NULL) 
    {
        return FALSE;
    }

    if (g_himlIconsSmall == NULL) 
    {
        g_himlIconsSmall = ImageList_Create(cxSmIcon, cySmIcon, ILC_MASK|ILC_SHARED|flags, 0, 32);
    }
    else 
    {
        // ImageList_setFlags already calls ImageList_remove on success
        if (!ImageList_SetFlags( g_himlIconsSmall, ILC_MASK|ILC_SHARED|flags ))
        {
            // Couldn't change flags; tough.  At least remove them all.
            ImageList_Remove(g_himlIconsSmall, -1);
        }
        ImageList_SetIconSize(g_himlIconsSmall, cxSmIcon, cySmIcon);
    }

    if (g_himlIconsSmall == NULL) 
    {
        ImageList_Destroy(g_himlIcons);
        g_himlIcons = NULL;
        return FALSE;
    }

    // set the bk colors to COLOR_WINDOW since this is what will
    // be used most of the time as the bk for these lists (cabinet, tray)
    // this avoids having to do ROPs when drawing, thus making it fast

    ImageList_SetBkColor(g_himlIcons, GetSysColor(COLOR_WINDOW));
    ImageList_SetBkColor(g_himlIconsSmall, GetSysColor(COLOR_WINDOW));

    // Load all of the icons with fRestore == TRUE
    if (fRestore)
    {
        TCHAR szModule[MAX_PATH];
        HKEY hkeyIcons;

        GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));

        // WARNING: this code assumes that these icons are the first in
        // our RC file and are in this order and these indexes correspond
        // to the II_ constants in shell.h.

        hkeyIcons = SHGetExplorerSubHkey(HKEY_LOCAL_MACHINE, g_szShellIcons, FALSE);


        for (i = 0; i < ARRAYSIZE(c_SystemImageListIndexes); i++) 
        {
            HICON hIcon=NULL;
            HICON hSmallIcon=NULL;
            int iIndex;

            // check to see if icon is overridden in the registry
            if (hkeyIcons)
            {
                TCHAR val[10];
                TCHAR ach[MAX_PATH];
                DWORD cb = SIZEOF(ach);

                wsprintf(val, g_szD, i);

                ach[0] = 0;
                SHQueryValueEx(hkeyIcons, val, NULL, NULL, (LPBYTE)ach, &cb);

                if (ach[0])
                {
                    HICON hIcons[2] = {0, 0};
                    int iIcon = PathParseIconLocation(ach);

                    ExtractIcons(ach, iIcon,
                                 MAKELONG(g_cxIcon,g_cxSmIcon),
                                 MAKELONG(g_cyIcon,g_cySmIcon),
                                 hIcons, NULL, 2, g_lrFlags);

                    hIcon = hIcons[0];
                    hSmallIcon = hIcons[1];

                    if (hIcon)
                    {
                        DebugMsg(DM_TRACE, TEXT("ShellImageListInit: Got default icon #%d from registry: %s,%d"), i, ach, iIcon);
                    }
                }
            }

            if (hIcon == NULL)
            {
                hIcon      = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(c_SystemImageListIndexes[i]), IMAGE_ICON, cxIcon, cyIcon, g_lrFlags);
                hSmallIcon = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(c_SystemImageListIndexes[i]), IMAGE_ICON, cxSmIcon, cySmIcon, g_lrFlags);
            }

            if (hIcon)
            {
                iIndex = SHAddIconsToCache(hIcon, hSmallIcon, szModule, i, 0);
                ASSERT(!fRestore || (iIndex == i));     // assume index
            }
        }

        if (hkeyIcons)
            RegCloseKey(hkeyIcons);
    }

    //
    // Refresh the overlay image so that the overlays are added to the imaglist.
    // GetIconOverlayManager() will initialize the overlay manager if necessary.
    //
    if (SUCCEEDED(GetIconOverlayManager(&psiom)))
    {
        psiom->lpVtbl->RefreshOverlayImages(psiom, SIOM_OVERLAYINDEX | SIOM_ICONINDEX);
        psiom->lpVtbl->Release(psiom);
    }

    return TRUE;
}

BOOL _ShellImageListTerm(void)
{
    ENTERCRITICAL;
    if (g_himlIcons) {
        ImageList_Destroy(g_himlIcons);
        g_himlIcons = NULL;
    }

    if (g_himlIconsSmall) {
        ImageList_Destroy(g_himlIconsSmall);
        g_himlIconsSmall = NULL;
    }
    LEAVECRITICAL;
    return TRUE;
}

// get a hold of the system image lists

BOOL WINAPI Shell_GetImageLists(HIMAGELIST *phiml, HIMAGELIST *phimlSmall)
{
    FileIconInit( FALSE );  // make sure they are created and the right size.

    if (phiml)
        *phiml = g_himlIcons;

    if (phimlSmall)
        *phimlSmall = g_himlIconsSmall;

    return TRUE;
}


void WINAPI Shell_SysColorChange(void)
{
    COLORREF clrWindow;

    ENTERCRITICAL;
    clrWindow = GetSysColor(COLOR_WINDOW);
    ImageList_SetBkColor(g_himlIcons     , clrWindow);
    ImageList_SetBkColor(g_himlIconsSmall, clrWindow);
    LEAVECRITICAL;
}

// simulate the document icon by crunching a copy of an icon and putting it in the
// middle of our default document icon, then add it to the passsed image list
//
// in:
//      hIcon   icon to use as a basis for the simulation
//
// returns:
//      hicon

HICON SimulateDocIcon(HIMAGELIST himl, HICON hIcon, BOOL fSmall)
{
    int cx = fSmall ? g_cxSmIcon : g_cxIcon;
    int cy = fSmall ? g_cxSmIcon : g_cxIcon;

    HDC hdc;
    HDC hdcMem;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    HBITMAP hbmT;
    ICONINFO ii;
    UINT iIndex;

    if (himl == NULL || hIcon == NULL)
        return NULL;

    hdc = GetDC(NULL);
    hdcMem = CreateCompatibleDC(hdc);
    hbmColor = CreateCompatibleBitmap(hdc, cx, cy);
    hbmMask = CreateBitmap(cx, cy, 1, 1, NULL);
    ReleaseDC(NULL, hdc);

    hbmT = SelectObject(hdcMem, hbmMask);
    iIndex = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);
    ImageList_Draw(himl, iIndex, hdcMem, 0, 0, ILD_MASK);

    SelectObject(hdcMem, hbmColor);
    ImageList_DrawEx(himl, iIndex, hdcMem, 0, 0, 0, 0, RGB(0,0,0), CLR_DEFAULT, ILD_NORMAL);

    //BUGBUG this assumes the generic icon is white
    PatBlt(hdcMem, cx/4-1, cy/4-1, cx/2+(fSmall?2:4), cy/2+2, WHITENESS);
    DrawIconEx(hdcMem, cx/4, cy/4, hIcon, cx/2, cy/2, 0, NULL, DI_NORMAL);

    SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);

    ii.fIcon    = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmColor = hbmColor;
    ii.hbmMask  = hbmMask;
    hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbmColor);
    DeleteObject(hbmMask);

    return hIcon;
}

// add icons to the system imagelist (icon cache) and put the location
// in the location cache
//
// in:
//      hIcon, hIconSmall       the icons, hIconSmall can be NULL
//      pszIconPath             locations (for location cache)
//      iIconIndex              index in pszIconPath (for location cache)
//      uIconFlags              GIL_ flags (for location cahce)
// returns:
//      location in system image list
//
int SHAddIconsToCache(HICON hIcon, HICON hIconSmall, LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    int iImage;
    int iImageSmall;
    int iImageFree;

    ASSERT(g_himlIcons);

    if (hIcon == NULL)
    {
        SHDefExtractIcon(pszIconPath, iIconIndex, uIconFlags, &hIcon, &hIconSmall, 0);
    }

    if (hIcon == NULL)
        return -1;

    if (g_himlIcons == NULL)  // If we ran out of memory initing the icon cache
        return -1;          // We'd better not play with the NULL image list.


    if (hIconSmall == NULL)
        hIconSmall = hIcon;  // ImageList_AddIcon will shrink for us

    ENTERCRITICAL;

    iImageFree = GetFreeImageIndex();

    iImage = ImageList_ReplaceIcon(g_himlIcons, iImageFree, hIcon);

    if (iImage >= 0)
    {
        iImageSmall = ImageList_ReplaceIcon(g_himlIconsSmall, iImageFree, hIconSmall);

        if (iImageSmall < 0)
        {
            DebugMsg(DM_TRACE, TEXT("AddIconsToCache() ImageList_AddIcon failed (small)"));
            // only remove it if it was added at the end otherwise all the
            // index's above iImage will change.
            // ImageList_ReplaceIcon should only fail on the end anyway.
            if (iImageFree == -1)
                ImageList_Remove(g_himlIcons, iImage);   // remove big
            iImage = -1;
        }
        else
        {
            ASSERT(iImageSmall == iImage);
        }
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("AddIconsToCache() ImageList_AddIcon failed"));
        iImage = -1;
    }

    if (iImage >= 0)
        AddToIconTable(pszIconPath, iIconIndex, uIconFlags, iImage);

    LEAVECRITICAL;

    if (hIcon)
        DestroyIcon(hIcon);

    if (hIconSmall && hIcon != hIconSmall)
        DestroyIcon(hIconSmall);

    return iImage;
}

//
//  default handler to extract a icon from a file
//
//  supports GIL_SIMULATEDOC
//
//  returns S_OK if success
//  returns S_FALSE if the file has no icons (or not the asked for icon)
//  returns E_FAIL for files on a slow link.
//  returns E_FAIL if cant access the file
//
//  LOWORD(nIconSize) = normal icon size
//  HIWORD(nIconSize) = smal icon size
//
STDAPI
SHDefExtractIcon(LPCTSTR pszIconFile, int iIndex, UINT uFlags,
        HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HICON hIcons[2] = {0, 0};
    UINT u;

#ifdef DEBUG
    TCHAR ach[128];
    GetModuleFileName(HINST_THISDLL, ach, ARRAYSIZE(ach));

    if (lstrcmpi(pszIconFile, ach) == 0 && iIndex >= 0)
    {
        TraceMsg(TF_WARNING, "Re-extracting %d from SHELL32.DLL", iIndex);
    }
#endif

    if (NULL == g_himlIcons)
        FileIconInit( FALSE );  // make sure they are created and the right size.

    //
    //  get the icon from the file
    //
    if (PathIsSlow(pszIconFile, -1))
    {
        DebugMsg(DM_TRACE, TEXT("not extracting icon from '%s' because of slow link"), pszIconFile);
        return E_FAIL;
    }

#ifdef XXDEBUG
    TraceMsg(TF_ALWAYS, "Extracting icon %d from %s.", iIndex, pszIconFile);
    Sleep(500);
#endif

    //
    // nIconSize == 0 means use the default size.
    // Backup is passing nIconSize == 1 need to support them too.
    //
    if (nIconSize <= 2)
        nIconSize = MAKELONG(g_cxIcon, g_cxSmIcon);

    if (uFlags & GIL_SIMULATEDOC)
    {
        HICON hIconSmall;

        u = ExtractIcons(pszIconFile, iIndex, g_cxSmIcon, g_cySmIcon,
            &hIconSmall, NULL, 1, g_lrFlags);

        if (u == -1)
            return E_FAIL;

        hIcons[0] = SimulateDocIcon(g_himlIcons, hIconSmall, FALSE);
        hIcons[1] = SimulateDocIcon(g_himlIconsSmall, hIconSmall, TRUE);

        if (hIconSmall)
            DestroyIcon(hIconSmall);
    }
    else
    {
        u = ExtractIcons(pszIconFile, iIndex, nIconSize, nIconSize,
            hIcons, NULL, 2, g_lrFlags);

        if (-1 == u)
            return E_FAIL;

#ifdef DEBUG
        if (0 == u)
        {
            TraceMsg(TF_WARNING, "Failed to extract icon %d from %s.", iIndex, pszIconFile);    
        }
#endif
    }

    *phiconLarge = hIcons[0];
    *phiconSmall = hIcons[1];

    return u==0 ? S_FALSE : S_OK;
}


#ifdef UNICODE

STDAPI
SHDefExtractIconA(
    LPCSTR pszIconFile,
    int iIndex,
    UINT uFlags,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_STRING_PTRA(pszIconFile, -1))
    {
        WCHAR wsz[MAX_PATH];

        SHAnsiToUnicode(pszIconFile, wsz, ARRAYSIZE(wsz));
        hres = SHDefExtractIcon(wsz, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
    }
    return hres;
}

#else

STDAPI
SHDefExtractIconW(
    LPCWSTR pszIconFile,
    int iIndex,
    UINT uFlags,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_STRING_PTRW(pszIconFile, -1))
    {
        char sz[MAX_PATH];

        SHUnicodeToAnsi(pszIconFile, sz, ARRAYSIZE(sz));
        hres = SHDefExtractIcon(sz, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
    }
    return hres;
}

#endif

//
// in:
//      pszIconPath     file to get icon from (eg. cabinet.exe)
//      iIconIndex      icon index in pszIconPath to get
//      uIconFlags      GIL_ values indicating simulate doc icon, etc.

int WINAPI Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    int iImageIndex;

    // lots of random codepaths from APIs end up here before init
    if (g_himlIcons == NULL)
        FileIconInit( FALSE );

    iImageIndex = LookupIconIndex(PathFindFileName(pszIconPath), iIconIndex, uIconFlags);

    if (iImageIndex == -1)
    {
        iImageIndex = SHAddIconsToCache(NULL, NULL, pszIconPath, iIconIndex, uIconFlags);
    }

    return iImageIndex;
}

STDAPI_(void) FixPlusIcons()
{
    int i;

    // nuke all of the shell internal icons
    {
        HKEY hkeyIcons = SHGetExplorerSubHkey(HKEY_LOCAL_MACHINE, g_szShellIcons, FALSE);
        if (hkeyIcons)
        {
            for (i = 0; i < ARRAYSIZE(c_SystemImageListIndexes); i++) 
            {
                TCHAR szRegPath[10], szBuf[MAX_PATH];
                DWORD cb = SIZEOF(szBuf);

                wsprintf(szRegPath, g_szD, i);

                if (SHQueryValueEx(hkeyIcons, szRegPath, NULL, NULL, (LPBYTE)szBuf, &cb) == ERROR_SUCCESS &&
                    StrStrI(szBuf, TEXT("cool.dll")))
                {
                    RegDeleteValue(hkeyIcons, szRegPath);
                }
            }
            RegCloseKey(hkeyIcons);
        }
    }
    // all of the DefaultIcons under the CLSIDs for shell folders
    {
        static const struct {
            REFCLSID clsid;
            LPCTSTR pszIcon;
        } c_rgCLSID[] = {
            { &CLSID_NetworkPlaces,     TEXT("shell32.dll,17") },
            { &CLSID_ControlPanel,      TEXT("shell32.dll,-137") },
            { &CLSID_Printers,          TEXT("shell32.dll,-138") },
            { &CLSID_MyComputer,        TEXT("explorer.exe,0") },
            { &CLSID_Remote,            TEXT("rnaui.dll,0") },
            { &CLSID_CFonts,            TEXT("fontext.dll,-101") },
            { &CLSID_RecycleBin,        NULL },
            { &CLSID_Briefcase,         NULL },
        };

        for (i = 0; i < ARRAYSIZE(c_rgCLSID); i++)
        {
            TCHAR szCLSID[64], szRegPath[128], szBuf[MAX_PATH];
            DWORD cb = SIZEOF(szBuf);

            SHStringFromGUID(c_rgCLSID[i].clsid, szCLSID, ARRAYSIZE(szCLSID));
            wsprintf(szRegPath, TEXT("CLSID\\%s\\DefaultIcon"), szCLSID);

            if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegPath, szBuf, &cb) == ERROR_SUCCESS &&
                StrStrI(szBuf, TEXT("cool.dll")))
            {
                if (IsEqualGUID(c_rgCLSID[i].clsid, &CLSID_RecycleBin))
                {
                    RegSetValueString(HKEY_CLASSES_ROOT, szRegPath, TEXT("Empty"), TEXT("shell32.dll,31"));
                    RegSetValueString(HKEY_CLASSES_ROOT, szRegPath, TEXT("Full"), TEXT("shell32.dll,32"));
                    if (StrStr(szBuf, TEXT("20")))
                        RegSetString(HKEY_CLASSES_ROOT, szRegPath, TEXT("shell32.dll,31")); // empty
                    else
                        RegSetString(HKEY_CLASSES_ROOT, szRegPath, TEXT("shell32.dll,32")); // full
                }
                else
                {
                    if (c_rgCLSID[i].pszIcon)
                        RegSetString(HKEY_CLASSES_ROOT, szRegPath, c_rgCLSID[i].pszIcon);
                    else
                        RegDeleteValue(HKEY_CLASSES_ROOT, szRegPath);
                }
            }
        }
    }

    // all of the DefaultIcons under the ProgIDs for file types
    {
        static const struct {
            LPCTSTR pszProgID;
            LPCTSTR pszIcon;
        } c_rgProgID[] = {
            { TEXT("Folder"),   TEXT("shell32.dll,3") },
            { TEXT("Directory"),TEXT("shell32.dll,3") },
            { TEXT("Drive"),    TEXT("shell32.dll,8") },
            { TEXT("drvfile"),  TEXT("shell32.dll,-154") },
            { TEXT("vxdfile"),  TEXT("shell32.dll,-154") },
            { TEXT("dllfile"),  TEXT("shell32.dll,-154") },
            { TEXT("sysfile"),  TEXT("shell32.dll,-154") },
            { TEXT("txtfile"),  TEXT("shell32.dll,-152") },
            { TEXT("inifile"),  TEXT("shell32.dll,-151") },
            { TEXT("inffile"),  TEXT("shell32.dll,-151") },
        };

        for (i = 0; i < ARRAYSIZE(c_rgProgID); i++)
        {
            TCHAR szRegPath[128], szBuf[MAX_PATH];
            DWORD cb = SIZEOF(szBuf);

            wsprintf(szRegPath, TEXT("%s\\DefaultIcon"), c_rgProgID[i].pszProgID);

            if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegPath, szBuf, &cb) == ERROR_SUCCESS &&
                StrStrI(szBuf, TEXT("cool.dll")))
            {
                RegSetString(HKEY_CLASSES_ROOT, szRegPath, c_rgProgID[i].pszIcon);
            }
        }
    }

    FlushIconCache();
}
