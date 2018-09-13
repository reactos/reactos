#include "stdafx.h"
#pragma hdrstop
//#include "resource.h"
//#include "..\ids.h"
//#include "deskstat.h"
//#include "dutil.h"
//#include "deskmovr.h"
//#include "dsubscri.h"
//#include "..\util.h"
//#include <webcheck.h>
//#include "..\security.h"
//#include <mmhelper.h>
//#include "intshcut.h"
//#include <multimon.h>

#include <mluisupp.h>

#ifdef POSTSPLIT

const TCHAR c_szControlIni[] = TEXT("control.ini");
const TCHAR c_szPatterns[] = TEXT("patterns");
const TCHAR c_szBackgroundPreview2[] = TEXT("BackgroundPreview2");
const TCHAR c_szComponentPreview[] = TEXT("ComponentPreview");
const TCHAR c_szRegDeskHtmlProp[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display\\shellex\\PropertySheetHandlers\\DeskHtmlExt");
const TCHAR c_szZero[] = TEXT("0");
const TCHAR c_szCurVer[] = REGSTR_PATH_SETUP;
const TCHAR c_szWallPaperDir[] = TEXT("WallPaperDir");

void GetNextCompPos(COMPPOS *pcp, int iScreenWidth, int iScreenHeight, int iBorder)
{
    DWORD cComp = 0;
    DWORD dw;
    TCHAR lpszDeskcomp[MAX_PATH];
    // We should not increment the value of REG_VAL_GENERAL_CCOMPPOS everytime this function
    // is called, but only when this function changes these values.
    int     iLeft = pcp->iLeft, iTop = pcp->iTop;
    DWORD   dwWidth = pcp->dwWidth, dwHeight = pcp->dwHeight;

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_GENERAL, NULL);
    //
    // Get component position count.
    //
    HKEY hkey = NULL;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, NULL, 0,
                       KEY_READ | KEY_WRITE, NULL, &hkey, &dw) == ERROR_SUCCESS)
    {
        DWORD cbSize = SIZEOF(cComp);
        RegQueryValueEx(hkey, REG_VAL_GENERAL_CCOMPPOS, NULL, &dw,
                        (LPBYTE)&cComp, &cbSize);
    }

    //
    // Compute the layer we live on (see below).
    //
    DWORD dwLayer = cComp / (COMPONENT_PER_ROW * COMPONENT_PER_COL);
    if (((dwLayer * iBorder) > (DWORD)(iScreenWidth / (COMPONENT_PER_ROW + 1))) ||
        ((dwLayer * iBorder) > (DWORD)(iScreenHeight / (COMPONENT_PER_COL + 1))))
    {
        cComp = 0;
        dwLayer = 0;
    }

    //
    // Compute the position.  Assuming 3 components per row,
    // and 2 per column, we position components thusly:
    //
    //        +-------+
    //        |x 4 2 0|
    //        |x 5 3 1| <-- screen, divided into 4x3 block coordinates
    //        |x x x x|
    //        +-------+
    //
    // The 6th component sits in a new layer, offset down
    // and to the left of component 0 by the amount iBorder.
    //
    // The first calculation for iLeft and iTop determines the
    // block coordinate value (for instance, component 0 would
    // be at block coordinate value [3,0] and component 1 at [3,1]).
    //
    // The second calculation turns the block coordinate into
    // a screen coordinate.
    //
    // The third calculation adjusts for the border (always down and
    // to the right) and the layers (always down and to the left).
    //
    pcp->iLeft = COMPONENT_PER_ROW - ((cComp / COMPONENT_PER_COL) % COMPONENT_PER_ROW); // 3 3 2 2 1 1 3 3 2 2 1 1 ...
    pcp->iLeft *= (iScreenWidth / (COMPONENT_PER_ROW + 1));
    pcp->iLeft += iBorder - (dwLayer * iBorder);

    pcp->iTop = cComp % COMPONENT_PER_COL;  // 0 1 0 1 0 1 ...
    pcp->iTop *= (iScreenHeight / (COMPONENT_PER_COL + 1));
    pcp->iTop += iBorder + (dwLayer * iBorder);

    pcp->dwWidth = (iScreenWidth / (COMPONENT_PER_ROW + 1)) - 2 * iBorder;
    pcp->dwHeight = (iScreenHeight / (COMPONENT_PER_COL + 1)) - 2 * iBorder;

    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
       pcp->iLeft = iScreenWidth - (pcp->iLeft + pcp->dwWidth);
    }

    //
    // Save the new component position count, if the position or dimension has changed.
    //
    if (hkey && (iLeft != pcp->iLeft || iTop != pcp->iTop
            || dwWidth != pcp->dwWidth || dwHeight != pcp->dwHeight))
    {
        dw = cComp + 1;
        RegSetValueEx(hkey, REG_VAL_GENERAL_CCOMPPOS, 0, REG_DWORD, (LPBYTE)&dw,
                      SIZEOF(dw));
        RegCloseKey(hkey);
    }
}

void MoveOnScreen(COMPPOS *pcp, int iXBorders, int iYBorders, int iXLeft, int iYTop, EnumMonitorsArea* ema)
{
    POINT ptVirtualMonitor;
    ptVirtualMonitor.x = ema->rcVirtualMonitor.left;
    ptVirtualMonitor.y = ema->rcVirtualMonitor.top;
    int iIndex = GetWorkAreaIndex(pcp, ema->rcWorkArea, ema->iMonitors, &ptVirtualMonitor);
    if (iIndex < 0 || iIndex >= ema->iMonitors)
    {
        //
        // It doesn't fall in any of the work-areas.
        // Force it to Primary work area.
        //
        iIndex = 0;
    }

    LPRECT lprcScreen;
    RECT rcViewAreas[LV_MAX_WORKAREAS];  // WorkArea minus toolbar/tray areas
    int nViewAreas = ARRAYSIZE(rcViewAreas);
    // Get the ViewAreas
    if (GetViewAreas(rcViewAreas, &nViewAreas))
    {
        lprcScreen = &rcViewAreas[iIndex];
    }
    else
    {
        lprcScreen = &ema->rcWorkArea[iIndex];
    }

    LPRECT lprcVirtualScreen = &ema->rcWorkArea[iIndex];
    if(ema->iMonitors == 1)
    {
        lprcVirtualScreen = lprcScreen;
    }
    else
    {
        lprcVirtualScreen = &ema->rcVirtualMonitor;
    }

    //
    // Make sure that the left edge is NOT too far to the right.
    //
    if (pcp->iLeft >= (lprcScreen->right - lprcVirtualScreen->left))
    {
        pcp->iLeft = (lprcScreen->right - lprcVirtualScreen->left) - (pcp->dwWidth + (iXBorders - iXLeft));
    }

    //Make sure that the right edge is NOT too far to the left.
    if (pcp->iLeft + ((int)pcp->dwWidth + (iXBorders - iXLeft)) < (lprcScreen->left - lprcVirtualScreen->left))
    {
        pcp->iLeft = (lprcScreen->left - lprcVirtualScreen->left) + iXLeft;
    }

    // Make sure that the top edge is NOT too far down.
    if (pcp->iTop >= (lprcScreen->bottom - lprcVirtualScreen->top) - iYTop)
    {
        pcp->iTop = (lprcScreen->bottom - lprcVirtualScreen->top) - (pcp->dwHeight + (iYBorders - iYTop));
    }

    //
    // Make sure that the top edge is NOT too far up.
    //
    if (pcp->iTop < (lprcScreen->top - lprcVirtualScreen->top) + iYTop)
    {
        pcp->iTop = (lprcScreen->top - lprcVirtualScreen->top) + iYTop;
    }
}

//
// PositionComponent will assign a screen position and
// make sure it fits on the screen.
//
void PositionComponent(COMPPOS *pcp, int iCompType)
{
    COMPPOS cpDefault = *pcp;

    int iBorder = GetSystemMetrics(SM_CYSMCAPTION);
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE);
    int iScreenWidth = rect.right - rect.left;
    int iScreenHeight = rect.bottom - rect.top;

    EnumMonitorsArea ema;
    GetMonitorSettings(&ema);

    if (pcp->iLeft == COMPONENT_DEFAULT_LEFT || pcp->iTop == COMPONENT_DEFAULT_TOP)
    {
        // Call this only if the above condition is satisfied, because, GetNextCompPos
        // increments the count of components (used to compute the next default position)
        // in the registry.
        GetNextCompPos(&cpDefault, iScreenWidth, iScreenHeight, iBorder);
        //
        // Assign an X and Y if necessary.
        //
        if (pcp->iLeft == COMPONENT_DEFAULT_LEFT)
        {
            // We have to position this in the primary monitor
            pcp->iLeft = (ema.rcMonitor[0].left - ema.rcVirtualMonitor.left)
                    + cpDefault.iLeft;
        }
        if (pcp->iTop == COMPONENT_DEFAULT_TOP)
        {
            // We have to position this in the primary monitor
            pcp->iTop = (ema.rcMonitor[0].top - ema.rcVirtualMonitor.top)
                    + cpDefault.iTop;
        }
    }

    //
    // Assign width and height if necessary.
    //
    if (((int)(pcp->dwWidth) < 10) && (iCompType != COMP_TYPE_PICTURE))
    {
        pcp->dwWidth = cpDefault.dwWidth;
    }
    if (((int)(pcp->dwHeight) < 10) && (iCompType != COMP_TYPE_PICTURE))
    {
        pcp->dwHeight = cpDefault.dwHeight;
    }

    //
    // Make sure that the component is inside one of the monitors.
    //
    int iXBorder = GET_CXSIZE;
    int iCaptionSize = GET_CYCAPTION;
    int iYBorders = GET_CYSIZE + iCaptionSize;
    // Make sure that the new position does not fall off-screen
    MoveOnScreen(pcp, 2 * iXBorder, iYBorders, iXBorder, iCaptionSize, &ema);
}

typedef struct _tagFILETYPEENTRY {
    DWORD dwFlag;
    int iFilterId;
} FILETYPEENTRY;

FILETYPEENTRY afte[] = {
    { GFN_URL, IDS_URL_FILTER, },
    { GFN_CDF, IDS_CDF_FILTER, },
    { GFN_LOCALHTM, IDS_HTMLDOC_FILTER, },
    { GFN_PICTURE,  IDS_IMAGES_FILTER, },
};

//
// Opens either an HTML page or a picture.
//
BOOL GetFileName(HWND hdlg, LPTSTR pszFileName, int iSize, int iTypeId, DWORD dwFlags)
{
    BOOL fRet = FALSE;

    if (dwFlags)
    {
        int i;
        TCHAR szFilter[MAX_PATH];

        //
        // Set the friendly name.
        //
        LPTSTR pchFilter = szFilter;
        int cchFilter = ARRAYSIZE(szFilter) - 2;    // room for term chars
        int cchRead = MLLoadString(iTypeId, pchFilter, cchFilter);
        pchFilter += cchRead + 1;
        cchFilter -= cchRead + 1;

        //
        // Append the file filters.
        //
        BOOL fAddedToString = FALSE;
        for (i=0; (cchFilter>0) && (i<ARRAYSIZE(afte)); i++)
        {
            if (dwFlags & afte[i].dwFlag)
            {
                if (fAddedToString)
                {
                    *pchFilter++ = TEXT(';');
                    cchFilter--;
                }
                cchRead = MLLoadString(afte[i].iFilterId,
                                     pchFilter, cchFilter);
                pchFilter += cchRead;
                cchFilter -= cchRead;
                fAddedToString = TRUE;
            }
        }
        *pchFilter++ = TEXT('\0');

        //
        // Double-NULL terminate the string.
        //
        *pchFilter = TEXT('\0');

        TCHAR szBrowserDir[MAX_PATH];
        lstrcpy(szBrowserDir, pszFileName);
        PathRemoveArgs(szBrowserDir);
        PathRemoveFileSpec(szBrowserDir);

        TCHAR szBuf[MAX_PATH];
        MLLoadString(IDS_BROWSE, szBuf, ARRAYSIZE(szBuf));

        *pszFileName = TEXT('\0');

        OPENFILENAME ofn;
        ofn.lStructSize       = SIZEOF(ofn);
        ofn.hwndOwner         = hdlg;
        ofn.hInstance         = NULL;
        ofn.lpstrFilter       = szFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nFilterIndex      = 1;
        ofn.nMaxCustFilter    = 0;
        ofn.lpstrFile         = pszFileName;
        ofn.nMaxFile          = iSize;
        ofn.lpstrInitialDir   = szBrowserDir;
        ofn.lpstrTitle        = szBuf;
        ofn.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
        ofn.lpfnHook          = NULL;
        ofn.lpstrDefExt       = NULL;
        ofn.lpstrFileTitle    = NULL;

        fRet = GetOpenFileName(&ofn);
    }

    return fRet;
}

//
// Convert a pattern string to a bottom-up array of DWORDs,
// useful for BMP format files.
//
void PatternToDwords(LPTSTR psz, DWORD *pdwBits)
{
    DWORD i, dwVal;

    //
    // Get eight groups of numbers separated by non-numeric characters.
    //
    for (i=0; i<8; i++)
    {
        dwVal = 0;

        if (*psz != TEXT('\0'))
        {
            //
            // Skip over any non-numeric characters.
            //
            while (*psz && (!(*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                psz++;
            }

            //
            // Get the next series of digits.
            //
            while (*psz && (*psz >= TEXT('0') && *psz <= TEXT('9')))
            {
                dwVal = dwVal*10 + *psz++ - TEXT('0');
            }
        }

        pdwBits[7-i] = dwVal;
    }
}

//
// Convert a pattern string to a top-down array of WORDs,
// useful for CreateBitmap().
//
void PatternToWords(LPTSTR psz, WORD *pwBits)
{
    WORD i, wVal;

    //
    // Get eight groups of numbers separated by non-numeric characters.
    //
    for (i=0; i<8; i++)
    {
        wVal = 0;

        if (*psz != TEXT('\0'))
        {
            //
            // Skip over any non-numeric characters.
            //
            while (*psz && (!(*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                psz++;
            }

            //
            // Get the next series of digits.
            //
            while (*psz && ((*psz >= TEXT('0') && *psz <= TEXT('9'))))
            {
                wVal = wVal*10 + *psz++ - TEXT('0');
            }
        }

        pwBits[i] = wVal;
    }
}

BOOL IsValidPattern(LPCTSTR pszPat)
{
    BOOL fSawANumber = FALSE;

    //
    // We're mainly trying to filter multilingual upgrade cases
    // where the text for "(None)" is unpredictable.
    // Yes, I want to strangle the bastard who decided to
    // write out the none entry.
    //
    while (*pszPat)
    {
        if ((*pszPat < TEXT('0')) || (*pszPat > TEXT('9')))
        {
            //
            // It's not a number, it better be a space.
            //
            if (*pszPat != TEXT(' '))
            {
                return FALSE;
            }
        }
        else
        {
            fSawANumber = TRUE;
        }

        //
        // We avoid the need for AnsiNext by only advancing on US TCHARs.
        //
        pszPat++;
    }

    //
    // TRUE if we saw at least one digit and there were only digits and spaces.
    //
    return fSawANumber;
}

//
// Determines if the wallpaper can be supported in non-active desktop mode.
//
BOOL IsNormalWallpaper(LPCTSTR pszFileName)
{
    BOOL fRet = TRUE;

    if (pszFileName[0] == TEXT('\0'))
    {
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszFileName);

        //Check for specific files that can be shown only in ActiveDesktop mode!
        if((lstrcmpi(pszExt, TEXT(".GIF")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPE")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".JPEG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".PNG")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
           (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
           return FALSE;

        //Everything else (including *.BMP files) are "normal" wallpapers
    }
    return fRet;
}

//
// Determines if the wallpaper is a picture (vs. HTML).
//
BOOL IsWallpaperPicture(LPCTSTR pszWallpaper)
{
    BOOL fRet = TRUE;

    if (pszWallpaper[0] == TEXT('\0'))
    {
        //
        // Empty wallpapers count as empty pictures.
        //
        fRet = TRUE;
    }
    else
    {
        LPTSTR pszExt = PathFindExtension(pszWallpaper);

        if ((lstrcmpi(pszExt, TEXT(".HTM")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTML")) == 0) ||
            (lstrcmpi(pszExt, TEXT(".HTT")) == 0))
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

void OnDesktopSysColorChange(void)
{
    static COLORREF clrBackground = 0xffffffff;
    static COLORREF clrWindowText = 0xffffffff;

    //Get the new colors!
    COLORREF    clrNewBackground = GetSysColor(COLOR_BACKGROUND);
    COLORREF    clrNewWindowText = GetSysColor(COLOR_WINDOWTEXT);

    //Have we initialized these before?
    if(clrBackground != 0xffffffff)  //Have we initialized the statics yet?
    {
        // Our HTML file depends only on these two system colors.
        // Check if either of them has changed!
        // If not, no need to regenerate HTML file. 
        // This avoids infinite loop. And this is a nice optimization.
        if((clrBackground == clrNewBackground) &&
           (clrWindowText == clrNewWindowText))
            return; //No need to do anything. Just return.
    }

    // Remember the new colors in the statics.
    clrBackground = clrNewBackground;
    clrWindowText = clrNewWindowText;

    //
    // The desktop got a WM_SYSCOLORCHANGE.  We need to
    // regenerate the HTML if there are any system colors
    // showing on the desktop.  Patterns and the desktop
    // color are both based on system colors.
    //
    IActiveDesktop *pad;
    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop)))
    {
        BOOL fRegenerateHtml = FALSE;
        WCHAR szWallpaperW[INTERNET_MAX_URL_LENGTH];

        if (SUCCEEDED(pad->GetWallpaper(szWallpaperW, ARRAYSIZE(szWallpaperW), 0)))
        {
            if (!*szWallpaperW)
            {
                //
                // No wallpaper means the desktop color
                // or a pattern is showing - we need to
                // regenerate the desktop HTML.
                //
                fRegenerateHtml = TRUE;
            }
            else
            {
                TCHAR *pszWallpaper;
#ifdef UNICODE
                pszWallpaper = szWallpaperW;
#else
                CHAR szWallpaperA[INTERNET_MAX_URL_LENGTH];
                SHUnicodeToAnsi(szWallpaperW, szWallpaperA, ARRAYSIZE(szWallpaperA));
                pszWallpaper = szWallpaperA;
#endif
                if (IsWallpaperPicture(pszWallpaper))
                {
                    WALLPAPEROPT wpo = { SIZEOF(wpo) };
                    if (SUCCEEDED(pad->GetWallpaperOptions(&wpo, 0)))
                    {
                        if (wpo.dwStyle == WPSTYLE_CENTER)
                        {
                            //
                            // We have a centered picture,
                            // the pattern or desktop color
                            // could be leaking around the edges.
                            // We need to regenerate the desktop
                            // HTML.
                            //
                            fRegenerateHtml = TRUE;
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "SYSCLRCHG: Could not get wallpaper options!");
                    }
                }
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "SYSCLRCHG: Could not get selected wallpaper!");
        }

        if (fRegenerateHtml)
        {
            pad->ApplyChanges(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH);
        }

        pad->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "SYSCLRCHG: Could not create CActiveDesktop!");
    }
}

//
// Convert a .URL file into its target.
//
void CheckAndResolveLocalUrlFile(LPTSTR pszFileName, int cchFileName)
{
    //
    // This function only works on *.URL files.
    //
    if (!PathIsURL(pszFileName))
    {
        LPTSTR pszExt;

        //
        // Check if the extension of this file is *.URL
        //
        pszExt = PathFindExtension(pszFileName);
        if (pszExt && *pszExt)
        {
            TCHAR  szUrl[15];
        
            MLLoadString(IDS_URL_EXTENSION, szUrl, ARRAYSIZE(szUrl));

            if (lstrcmpi(pszExt, szUrl) == 0)
            {
                HRESULT  hr;

                IUniformResourceLocator *purl;

                hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator,
                              (LPVOID *)&purl);

                if (EVAL(SUCCEEDED(hr)))  // This works for both Ansi and Unicode
                {
                    ASSERT(purl);

                    IPersistFile  *ppf;

                    hr = purl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
                    if (SUCCEEDED(hr))
                    {
                        WCHAR szFileW[MAX_PATH];
                        LPTSTR pszTemp;

                        SHTCharToUnicode(pszFileName, szFileW, ARRAYSIZE(szFileW));
                        ppf->Load(szFileW, STGM_READ);
                        hr = purl->GetURL(&pszTemp);    // Wow, an ANSI/UNICODE COM interface!
                        if (EVAL(SUCCEEDED(hr)))
                        {
                            StrCpyN(pszFileName, pszTemp, cchFileName);
                            CoTaskMemFree(pszTemp);
                        }
                        ppf->Release();
                    }
                    purl->Release();
                }
            }
        }
    }
}

void GetCBarStartPos(int *piLeft, int *piTop, DWORD *pdwWidth, DWORD *pdwHeight)
{
#define INVALID_POS 0x80000000
    HKEY hkey;

    //
    // Assume nothing.
    //
    *piLeft = INVALID_POS;
    *piTop = INVALID_POS;
    *pdwWidth = INVALID_POS;
    *pdwHeight = INVALID_POS;

    //
    // Read from registry first.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType, cbData;

        cbData = SIZEOF(*piLeft);
        RegQueryValueEx(hkey, TEXT("CBar_Left"), NULL, &dwType, (LPBYTE)piLeft, &cbData);

        cbData = SIZEOF(*piTop);
        RegQueryValueEx(hkey, TEXT("CBar_Top"), NULL, &dwType, (LPBYTE)piTop, &cbData);

        cbData = SIZEOF(*pdwWidth);
        RegQueryValueEx(hkey, TEXT("CBar_Width"), NULL, &dwType, (LPBYTE)pdwWidth, &cbData);

        cbData = SIZEOF(*pdwHeight);
        RegQueryValueEx(hkey, TEXT("CBar_Height"), NULL, &dwType, (LPBYTE)pdwHeight, &cbData);

        RegCloseKey(hkey);
    }

    //
    // Fill in defaults when registry provides no info.
    //
    if (*piLeft == INVALID_POS)
    {
        *piLeft = -CBAR_WIDTH;
    }
    if (*piTop == INVALID_POS)
    {
        *piTop = CBAR_TOP;
    }
    if (*pdwWidth == INVALID_POS)
    {
        *pdwWidth = CBAR_WIDTH;
    }
    if (*pdwHeight == INVALID_POS)
    {
        //
        // Compute number of buttons.
        //
        DWORD cButtons;
        DWORD dwDataLength = SIZEOF(cButtons);
        DWORD cButtonsDefault = 12;

        SHRegGetUSValue(REG_DESKCOMP, REG_VAL_MISC_CHANNELSIZE, NULL,
            &cButtons, &dwDataLength, FALSE, &cButtonsDefault, SIZEOF(cButtonsDefault));

        //
        // During a new user log-on, the above function returns
        // a very huge value because this gets called while the
        // REG_DESKCOMP itself is being created.  We correct it here.
        //
        if (cButtons > 100)
        {
            cButtons = cButtonsDefault;
        }

        //
        // Convert button count into height.
        //
        *pdwHeight = cButtons * CBAR_BUTTON_HEIGHT + 6;
    }

    //
    // Convert negative values into positive ones.
    //
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE);
    if (*piLeft < 0)
    {
        *piLeft += (rect.right - rect.left) - *pdwWidth;
    }
    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
       *piLeft = (rect.right - rect.left) - (*piLeft + *pdwWidth);
    }
    
    if (*piTop < 0)
    {
        *piTop += (rect.bottom - rect.top) - *pdwHeight;
    }

    // Find the virtual dimensions.
    EnumMonitorsArea ema;
    GetMonitorSettings(&ema);
    // Position it in the primary monitor.
    *piLeft -= ema.rcVirtualMonitor.left;
    *piTop -= ema.rcVirtualMonitor.top;
#undef INVALID_POS
}

//
// Silently adds a specified component to the desktop and use the given
//  apply flags using which you can avoid nested unnecessary HTML generation, 
//  or refreshing which may lead to racing conditions.
// 
//
BOOL AddDesktopComponentNoUI(DWORD dwApplyFlags, LPCTSTR pszUrl, LPCTSTR pszFriendlyName, int iCompType, int iLeft, int iTop, int iWidth, int iHeight, COMPONENTA *pcomp, BOOL fChecked)
{
    COMPONENTA Comp;
    BOOL    fRet = FALSE;
    HRESULT hres;

    Comp.dwSize = sizeof(COMPONENTA);

    if(pcomp == NULL)
        pcomp = &Comp;

    //
    // Build the pcomp structure.
    //
    pcomp->dwID = -1;
    pcomp->iComponentType = iCompType;
    pcomp->fChecked = fChecked;
    pcomp->fDirty = FALSE;
    pcomp->fNoScroll = FALSE;
    pcomp->dwSize = SIZEOF(*pcomp);
    pcomp->cpPos.dwSize = SIZEOF(COMPPOS);
    pcomp->cpPos.iLeft = iLeft;
    pcomp->cpPos.iTop = iTop;
    pcomp->cpPos.dwWidth = iWidth;
    pcomp->cpPos.dwHeight = iHeight;
    pcomp->cpPos.izIndex = COMPONENT_TOP;
    pcomp->cpPos.fCanResize = TRUE;
    pcomp->cpPos.fCanResizeX = TRUE;
    pcomp->cpPos.fCanResizeY = TRUE;
    pcomp->cpPos.iPreferredLeftPercent = 0;
    pcomp->cpPos.iPreferredTopPercent = 0;
    lstrcpyn(pcomp->szSource, pszUrl, ARRAYSIZE(pcomp->szSource));
    lstrcpyn(pcomp->szSubscribedURL, pszUrl, ARRAYSIZE(pcomp->szSource));
    if (pszFriendlyName)
    {
        lstrcpyn(pcomp->szFriendlyName, pszFriendlyName, ARRAYSIZE(pcomp->szFriendlyName));
    }
    else
    {
        pcomp->szFriendlyName[0] = TEXT('\0');
    }

    PositionComponent(&pcomp->cpPos, pcomp->iComponentType);

    IActiveDesktop *pActiveDesk;

    //
    // Add it to the system.
    //
    hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pActiveDesk, IID_IActiveDesktop);
    if (SUCCEEDED(hres))
    {
        COMPONENT  CompW;

        CompW.dwSize = sizeof(CompW);  //Required for the MultiCompToWideComp to work properly.

        MultiCompToWideComp(pcomp, &CompW);

        pActiveDesk->AddDesktopItem(&CompW, 0);
        pActiveDesk->ApplyChanges(dwApplyFlags);
        pActiveDesk->Release();
        fRet = TRUE;
    }

    return fRet;
}



// Little helper function used to change the safemode state
void SetSafeMode(DWORD dwFlags)
{
    IActiveDesktopP * piadp;

    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&piadp, IID_IActiveDesktopP)))
    {
        piadp->SetSafeMode(dwFlags);
        piadp->Release();
    }
}

/****************************************************************************
 *
 *  RefreshWebViewDesktop - regenerates desktop HTML from registry and updates
 *                          the screen
 *
 *  ENTRY:
 *      none
 *
 *  RETURNS:
 *      TRUE on success
 *      
 ****************************************************************************/
BOOL PokeWebViewDesktop(DWORD dwFlags)
{
    IActiveDesktop *pad;
    HRESULT     hres;
    BOOL        fRet = FALSE;

    hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop);

    if (SUCCEEDED(hres))
    {
        pad->ApplyChanges(dwFlags);
        pad->Release();

        fRet = TRUE;
    }

    return (fRet);
}

void SetDefaultWallpaper()
{
    IActiveDesktop *pad;
    if (SUCCEEDED(CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pad, IID_IActiveDesktop)))
    {
        DWORD dwWallpaperStyle;
        WALLPAPEROPT wpo;

        // If we have a wallpaper at the old location in the registry, we use it as the default
        TCHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        // Read the Wallpaper from the Old location.
        if(!GetStringFromReg(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, c_szWallpaper, c_szNULL, szWallpaper, ARRAYSIZE(szWallpaper)))
            szWallpaper[0] = TEXT('\0');
        TrimWhiteSpace(szWallpaper);
        
        // Read the wallpaper style
        ReadWallpaperStyleFromReg(REGSTR_PATH_DESKTOP, &dwWallpaperStyle, FALSE);

        wpo.dwSize = SIZEOF(WALLPAPEROPT);
        pad->GetWallpaperOptions(&wpo, 0);
        wpo.dwStyle = dwWallpaperStyle;		//Set the wallpaper style!
        pad->SetWallpaperOptions(&wpo, 0);
        
        TCHAR szDefaultWallpaper[INTERNET_MAX_URL_LENGTH];
        GetWallpaperDirName(szDefaultWallpaper, ARRAYSIZE(szDefaultWallpaper));
        lstrcat(szDefaultWallpaper, TEXT("\\"));
        TCHAR szWP[INTERNET_MAX_URL_LENGTH];
        GetDefaultWallpaper(szWP);
        lstrcat(szDefaultWallpaper, szWP);
        if(szWallpaper[0] == TEXT('\0') || lstrcmpi(szWallpaper, g_szNone) == 0)
        {
            lstrcpy(szWallpaper, szDefaultWallpaper);
        }
        
        WCHAR *pwszWallpaper;
#ifndef UNICODE
        WCHAR   wszWallpaper[INTERNET_MAX_URL_LENGTH];
        SHAnsiToUnicode(szWallpaper, wszWallpaper, ARRAYSIZE(wszWallpaper));
        pwszWallpaper = wszWallpaper;
#else
        pwszWallpaper = szWallpaper;
#endif
        pad->SetWallpaper(pwszWallpaper, 0);
        pad->ApplyChanges(AD_APPLY_SAVE);
        pad->Release();
        // This is a kinda hack, but the best possible solution right now. The scenario is as follows.
        // The Memphis setup guys replace what the user specifies as the wallpaper in the old location
        // and restore it after setup is complete. But, SetDefaultWallpaper() gets called bet. these
        // two times and we are supposed to take a decision on whether to set the default htm wallpaper or not,
        // depending on what the user had set before the installation. The solution is to delay making
        // this decision until after the setup guys have restored the user's wallpaper. We do this in
        // CActiveDesktop::_ReadWallpaper(). We specify that SetDefaultWallpaper() was called by setting
        // the backup wallpaper in the new location to the default wallpaper.
        TCHAR szDeskcomp[MAX_PATH];

        wsprintf(szDeskcomp, REG_DESKCOMP_GENERAL, TEXT("\\"));
        SHSetValue(HKEY_CURRENT_USER, szDeskcomp,
            REG_VAL_GENERAL_BACKUPWALLPAPER, REG_SZ, (LPBYTE)szDefaultWallpaper,
            SIZEOF(TCHAR)*(lstrlen(szDefaultWallpaper)+1));
    }
}

STDAPI_(void) RefreshWebViewDesktop(void)
{
#ifdef FULL_DEBUG
    //We want to inform those who are currently using this to stop using this.
    ASSERT(FALSE);
#endif
    TraceMsg(TF_WARNING, "**** WARNING *****: SHDOCVW: Stop calling RefreshWebViewDesktop; \r\nUse IActiveDesktop->ApplyChanges(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH) instead\n\r");

    PokeWebViewDesktop(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH);
}


#define CCH_NONE 20 //big enough for "(None)" in german
TCHAR g_szNone[CCH_NONE] = {0};

void InitDeskHtmlGlobals(void)
{
    static fGlobalsInited = FALSE;

    if (fGlobalsInited == FALSE)
    {
        MLLoadString(IDS_WPNONE, g_szNone, ARRAYSIZE(g_szNone));

        fGlobalsInited = TRUE;
    }
}

//
// Loads the preview bitmap for property sheet pages.
//
HBITMAP LoadMonitorBitmap(void)
{
    HBITMAP hbm,hbmT;
    BITMAP bm;
    HBRUSH hbrT;
    HDC hdc;
    COLORREF c3df = GetSysColor(COLOR_3DFACE);

    hbm = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_MONITOR));
    if (hbm == NULL)
    {
        return NULL;
    }

    //
    // Convert the "base" of the monitor to the right color.
    //
    // The lower left of the bitmap has a transparent color
    // we fixup using FloodFill
    //
    hdc = CreateCompatibleDC(NULL);
    hbmT = (HBITMAP)SelectObject(hdc, hbm);
    hbrT = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_3DFACE));

    GetObject(hbm, sizeof(bm), &bm);

    ExtFloodFill(hdc, 0, bm.bmHeight-1, GetPixel(hdc, 0, bm.bmHeight-1), FLOODFILLSURFACE);

    //
    // Round off the corners.
    // The bottom two were done by the floodfill above.
    // The top left is important since SS_CENTERIMAGE uses it to fill gaps.
    // The top right should be rounded because the other three are.
    //
    SetPixel( hdc, 0, 0, c3df );
    SetPixel( hdc, bm.bmWidth-1, 0, c3df );

    //
    // Fill in the desktop here.
    //
    HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_DESKTOP));
    PatBlt(hdc, MON_X, MON_Y, MON_DX, MON_DY, PATCOPY);
    SelectObject(hdc, hbrOld);

    //
    // Clean up after ourselves.
    //
    SelectObject(hdc, hbrT);
    SelectObject(hdc, hbmT);
    DeleteDC(hdc);

    return hbm;
}

DWORD GetDesktopFlags(void)
{
    DWORD dwFlags = 0;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, NULL);

    HKEY  hkey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cbSize = SIZEOF(dwFlags);

        RegQueryValueEx(hkey, REG_VAL_COMP_GENFLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &cbSize);
        RegCloseKey(hkey);
    }

    return dwFlags;
}

STDAPI_(BOOL) SetDesktopFlags(DWORD dwMask, DWORD dwNewFlags)
{
    BOOL  fRet = FALSE;
    HKEY  hkey;
    DWORD dwDisposition;
    TCHAR lpszDeskcomp[MAX_PATH];

    GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, NULL);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 
                       0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hkey,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwFlags;
        DWORD cbSize = SIZEOF(dwFlags);
        DWORD dwType;

        if (RegQueryValueEx(hkey, REG_VAL_COMP_GENFLAGS, NULL, &dwType,
                            (LPBYTE)&dwFlags, &cbSize) != ERROR_SUCCESS)
        {
            dwFlags = 0;
        }

        dwFlags = (dwFlags & ~dwMask) | (dwNewFlags & dwMask);

        if (RegSetValueEx(hkey, REG_VAL_COMP_GENFLAGS, 0, REG_DWORD,
                          (LPBYTE)&dwFlags, sizeof(dwFlags)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    
        RegCloseKey(hkey);
    }

    return fRet;
}

BOOL UpdateComponentFlags(LPCTSTR pszCompId, DWORD dwMask, DWORD dwNewFlags)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;
    DWORD dwDisposition;

    GetRegLocation(szRegPath, REG_DESKCOMP_COMPONENTS, NULL);

    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szRegPath, 0, NULL, 0,
                       KEY_READ | KEY_WRITE, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwType, dwFlags, dwDataLength;

        dwDataLength = sizeof(DWORD);
        if(RegQueryValueEx(hkey, REG_VAL_COMP_FLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &dwDataLength) != ERROR_SUCCESS)
        {
            dwFlags = 0;
        }        

        dwNewFlags = (dwFlags & ~dwMask) | (dwNewFlags & dwMask);

        if (RegSetValueEx(hkey, REG_VAL_COMP_FLAGS, 0, REG_DWORD, (LPBYTE)&dwNewFlags,
                          SIZEOF(DWORD)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

        RegCloseKey(hkey);
    }

    return fRet;
}

DWORD GetCurrentState(LPTSTR pszCompId)
{
    TCHAR szRegPath[MAX_PATH];
    DWORD cbSize, dwType, dwCurState;

    GetRegLocation(szRegPath, REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    cbSize = sizeof(dwCurState);

    if (SHGetValue(HKEY_CURRENT_USER, szRegPath, REG_VAL_COMP_CURSTATE, &dwType, &dwCurState, &cbSize) != ERROR_SUCCESS)
        dwCurState = IS_NORMAL;

    return dwCurState;
}

BOOL GetSavedStateInfo(LPTSTR pszCompId, LPCOMPSTATEINFO    pCompState, BOOL  fRestoredState)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;
    DWORD dwDisposition;
    LPTSTR  lpValName = (fRestoredState ? REG_VAL_COMP_RESTOREDSTATEINFO : REG_VAL_COMP_ORIGINALSTATEINFO);

    GetRegLocation(szRegPath, REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szRegPath, 0, NULL, 0,
                       KEY_READ, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD   cbSize, dwType;
        
        cbSize = SIZEOF(*pCompState);
        dwType = REG_BINARY;
        
        if (RegQueryValueEx(hkey, lpValName, NULL, &dwType, (LPBYTE)pCompState, &cbSize) != ERROR_SUCCESS)
        {
            //If the item state is missing, read the item current position and
            // and return that as the saved state.
            COMPPOS cpPos;

            cbSize = SIZEOF(cpPos);
            dwType = REG_BINARY;
            if (RegQueryValueEx(hkey, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&cpPos, &cbSize) != ERROR_SUCCESS)
            {
                ZeroMemory(&cpPos, SIZEOF(cpPos));
            }            
            SetStateInfo(pCompState, &cpPos, IS_NORMAL);
        }

        RegCloseKey(hkey);
    }

    return fRet;
 }


BOOL UpdateDesktopPosition(LPTSTR pszCompId, int iLeft, int iTop, DWORD dwWidth, DWORD dwHeight, int izIndex,
                            BOOL    fSaveRestorePos, DWORD dwCurState)
{
    BOOL fRet = FALSE;
    TCHAR szRegPath[MAX_PATH];
    HKEY hkey;
    DWORD dwDisposition;

    GetRegLocation(szRegPath, REG_DESKCOMP_COMPONENTS, NULL);
    lstrcat(szRegPath, TEXT("\\"));
    lstrcat(szRegPath, pszCompId);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szRegPath, 0, NULL, 0,
                       KEY_READ | KEY_WRITE, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS)
    {
        COMPPOS cp;
        DWORD   dwType;
        DWORD   dwDataLength;

        dwType = REG_BINARY;
        dwDataLength = sizeof(COMPPOS);

        if(RegQueryValueEx(hkey, REG_VAL_COMP_POSITION, NULL, &dwType, (LPBYTE)&cp, &dwDataLength) != ERROR_SUCCESS)
        {
            cp.fCanResize = cp.fCanResizeX = cp.fCanResizeY = TRUE;
            cp.iPreferredLeftPercent = cp.iPreferredTopPercent = 0;
        }

        if(fSaveRestorePos)
        {
            //We have just read the current position; Let's save it as the restore position.
            COMPSTATEINFO   csiRestore;

            // Fillup the restore state structure.
            csiRestore.dwSize   = sizeof(csiRestore);
            csiRestore.iLeft    = cp.iLeft;
            csiRestore.iTop     = cp.iTop;
            csiRestore.dwWidth  = cp.dwWidth;
            csiRestore.dwHeight = cp.dwHeight;
            
            //Read the current State
            dwType = REG_DWORD;
            dwDataLength = SIZEOF(csiRestore.dwItemState);
            if (RegQueryValueEx(hkey, REG_VAL_COMP_CURSTATE, NULL, &dwType, (LPBYTE)&csiRestore.dwItemState, &dwDataLength) != ERROR_SUCCESS)
            {
                csiRestore.dwItemState = IS_NORMAL;
            }

            //Now that we know the complete current state, save it as the restore state!
            RegSetValueEx(hkey, REG_VAL_COMP_RESTOREDSTATEINFO, 0, REG_BINARY, (LPBYTE)&csiRestore, SIZEOF(csiRestore));
        }

        //Save the current state too!
        if(dwCurState)
            RegSetValueEx(hkey, REG_VAL_COMP_CURSTATE, 0, REG_DWORD, (LPBYTE)&dwCurState, SIZEOF(dwCurState));
            
        cp.dwSize = sizeof(COMPPOS);
        cp.iLeft = iLeft;
        cp.iTop = iTop;
        cp.dwWidth = dwWidth;
        cp.dwHeight = dwHeight;
        cp.izIndex = izIndex;

        if (RegSetValueEx(hkey, REG_VAL_COMP_POSITION, 0, REG_BINARY, (LPBYTE)&cp,
                          SIZEOF(cp)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

        RegCloseKey(hkey);
    }

    return fRet;
}

void GetPerUserFileName(LPTSTR pszOutputFileName, DWORD dwSize, LPTSTR pszPartialFileName)
{
    LPITEMIDLIST    pidlAppData;

    *pszOutputFileName = TEXT('\0');

    if(dwSize < MAX_PATH)
    {
        ASSERT(FALSE);
        return;
    }

    if(EVAL(SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidlAppData))))
    {
        SHGetPathFromIDList(pidlAppData, pszOutputFileName);
        PathAppend(pszOutputFileName, pszPartialFileName);
        ILFree(pidlAppData);
    }
}

void GetRegLocation(LPTSTR lpszResult, LPCTSTR lpszKey, LPCTSTR lpszScheme)
{
    TCHAR szSubkey[MAX_PATH];
    DWORD dwDataLength = sizeof(szSubkey) - 2 * sizeof(TCHAR);
    DWORD dwType;

    lstrcpy(szSubkey, TEXT("\\"));
    if (lpszScheme)
        lstrcat(szSubkey, lpszScheme);
    else
        SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, REG_VAL_SCHEME_DISPLAY, &dwType,
            (LPBYTE)(szSubkey) + sizeof(TCHAR), &dwDataLength);
    if (szSubkey[1])
        lstrcat(szSubkey, TEXT("\\"));

    wsprintf(lpszResult, lpszKey, szSubkey);
}

BOOL ValidateFileName(HWND hwnd, LPCTSTR pszFilename, int iTypeString)
{
    BOOL fRet = TRUE;

    DWORD dwAttributes = GetFileAttributes(pszFilename);
    if ((dwAttributes != 0xFFFFFFFF) &&
        (dwAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)))
    {
        TCHAR szType1[64];
        TCHAR szType2[64];

        MLLoadString(iTypeString, szType1, ARRAYSIZE(szType1));
        MLLoadString(iTypeString+1, szType2, ARRAYSIZE(szType2));
        if (ShellMessageBox(MLGetHinst(), hwnd,
                            MAKEINTRESOURCE(IDS_VALIDFN_FMT),
                            MAKEINTRESOURCE(IDS_VALIDFN_TITLE),
                            MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2,
                            szType1, szType2) == IDNO)
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

void GetWallpaperDirName(LPTSTR lpszWallPaperDir, int iBuffSize)
{
    //Compute the default wallpaper name.
    GetWindowsDirectory(lpszWallPaperDir, iBuffSize);
    lstrcat(lpszWallPaperDir, DESKTOPHTML_WEB_DIR);

    //Read it from the registry key, if it is set!
    GetStringFromReg(HKEY_LOCAL_MACHINE, c_szCurVer, c_szWallPaperDir, NULL, lpszWallPaperDir, iBuffSize);
}

BOOL CALLBACK MultiMonEnumAreaCallBack(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData)
{
    EnumMonitorsArea* pEMA = (EnumMonitorsArea*)lData;
    
    if (pEMA->iMonitors > LV_MAX_WORKAREAS - 1)
    {
        //ignore the other monitors because we can only handle up to LV_MAX_WORKAREAS
        //BUGBUG: should we dynamically allocate this?
        return FALSE;
    }
    GetMonitorRect(hMonitor, &pEMA->rcMonitor[pEMA->iMonitors]);
    GetMonitorWorkArea(hMonitor, &pEMA->rcWorkArea[pEMA->iMonitors]);
    if(pEMA->iMonitors == 0)
    {
        pEMA->rcVirtualMonitor.left = pEMA->rcMonitor[0].left;
        pEMA->rcVirtualMonitor.top = pEMA->rcMonitor[0].top;
        pEMA->rcVirtualMonitor.right = pEMA->rcMonitor[0].right;
        pEMA->rcVirtualMonitor.bottom = pEMA->rcMonitor[0].bottom;

        pEMA->rcVirtualWorkArea.left = pEMA->rcWorkArea[0].left;
        pEMA->rcVirtualWorkArea.top = pEMA->rcWorkArea[0].top;
        pEMA->rcVirtualWorkArea.right = pEMA->rcWorkArea[0].right;
        pEMA->rcVirtualWorkArea.bottom = pEMA->rcWorkArea[0].bottom;
    }
    else
    {
        if(pEMA->rcMonitor[pEMA->iMonitors].left < pEMA->rcVirtualMonitor.left)
        {
            pEMA->rcVirtualMonitor.left = pEMA->rcMonitor[pEMA->iMonitors].left;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].top < pEMA->rcVirtualMonitor.top)
        {
            pEMA->rcVirtualMonitor.top = pEMA->rcMonitor[pEMA->iMonitors].top;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].right > pEMA->rcVirtualMonitor.right)
        {
            pEMA->rcVirtualMonitor.right = pEMA->rcMonitor[pEMA->iMonitors].right;
        }
        if(pEMA->rcMonitor[pEMA->iMonitors].bottom > pEMA->rcVirtualMonitor.bottom)
        {
            pEMA->rcVirtualMonitor.bottom = pEMA->rcMonitor[pEMA->iMonitors].bottom;
        }

        if(pEMA->rcWorkArea[pEMA->iMonitors].left < pEMA->rcVirtualWorkArea.left)
        {
            pEMA->rcVirtualWorkArea.left = pEMA->rcWorkArea[pEMA->iMonitors].left;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].top < pEMA->rcVirtualWorkArea.top)
        {
            pEMA->rcVirtualWorkArea.top = pEMA->rcWorkArea[pEMA->iMonitors].top;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].right > pEMA->rcVirtualWorkArea.right)
        {
            pEMA->rcVirtualWorkArea.right = pEMA->rcWorkArea[pEMA->iMonitors].right;
        }
        if(pEMA->rcWorkArea[pEMA->iMonitors].bottom > pEMA->rcVirtualWorkArea.bottom)
        {
            pEMA->rcVirtualWorkArea.bottom = pEMA->rcWorkArea[pEMA->iMonitors].bottom;
        }
    }
    pEMA->iMonitors++;
    return TRUE;
}

void GetMonitorSettings(EnumMonitorsArea* ema)
{
    ema->iMonitors = 0;

    ema->rcVirtualMonitor.left = 0;
    ema->rcVirtualMonitor.top = 0;
    ema->rcVirtualMonitor.right = 0;
    ema->rcVirtualMonitor.bottom = 0;

    ema->rcVirtualWorkArea.left = 0;
    ema->rcVirtualWorkArea.top = 0;
    ema->rcVirtualWorkArea.right = 0;
    ema->rcVirtualWorkArea.bottom = 0;

    EnumDisplayMonitors(NULL, NULL, MultiMonEnumAreaCallBack, (LPARAM)ema);
}

int GetWorkAreaIndex(COMPPOS *pcp, LPCRECT prect, int crect, LPPOINT lpptVirtualTopLeft)
{
    int iIndex;
    POINT ptComp;
    ptComp.x = pcp->iLeft + lpptVirtualTopLeft->x;
    ptComp.y = pcp->iTop + lpptVirtualTopLeft->y;

    for (iIndex = 0; iIndex < crect; iIndex++)
    {
        if (PtInRect(&prect[iIndex], ptComp))
        {
            return iIndex;
        }
    }

    return -1;
}

// Prepends the Web wallpaper directory or the system directory to szWallpaper, if necessary
// (i.e., if the path is not specified). The return value is in szWallpaperWithPath, which is iBufSize
// bytes long
void GetWallpaperWithPath(LPCTSTR szWallpaper, LPTSTR szWallpaperWithPath, int iBufSize)
{
    if(szWallpaper[0] && lstrcmpi(szWallpaper, g_szNone) != 0 && !StrChr(szWallpaper, TEXT('\\'))
            && !StrChr(szWallpaper, TEXT(':'))) // The file could be d:foo.bmp
    {
        // If the file is a normal wallpaper, we prepend the windows directory to the filename
        if(IsNormalWallpaper(szWallpaper))
        {
            GetWindowsDirectory(szWallpaperWithPath, iBufSize);
        }
        // else we prepend the wallpaper directory to the filename
        else
        {
            GetWallpaperDirName(szWallpaperWithPath, iBufSize);
        }
        lstrcat(szWallpaperWithPath, TEXT("\\"));
        lstrcat(szWallpaperWithPath, szWallpaper);
    }
    else
    {
        lstrcpyn(szWallpaperWithPath, szWallpaper, iBufSize);
    }
}

// The default wallpaper's name might be different depending on the platform. Hence this function.
void GetDefaultWallpaper(LPTSTR lpszDefaultWallpaper)
{
    if (g_bRunOnMemphis)
    {
        lstrcpy(lpszDefaultWallpaper, DESKTOPHTML_DEFAULT_MEMPHIS_WALLPAPER);
    }
    else
    {
        lstrcpy(lpszDefaultWallpaper, DESKTOPHTML_DEFAULT_WALLPAPER);
    }
}

BOOL GetViewAreas(LPRECT lprcViewAreas, int* pnViewAreas)
{
    BOOL bRet = FALSE;
    HWND hwndDesktop = GetShellWindow();    // This is the "normal" desktop
    
    if (hwndDesktop && IsWindow(hwndDesktop))
    {
        DWORD dwProcID, dwCurrentProcID;
        
        GetWindowThreadProcessId(hwndDesktop, &dwProcID);
        dwCurrentProcID = GetCurrentProcessId();
        if (dwCurrentProcID == dwProcID) {
            SendMessage(hwndDesktop, DTM_GETVIEWAREAS, (WPARAM)pnViewAreas, (LPARAM)lprcViewAreas);
            if (*pnViewAreas <= 0)
            {
                bRet = FALSE;
            }
            else
            {
                bRet = TRUE;
            }
        }
        else
        {
            bRet = FALSE;
        }
    }
    return bRet;
}

#endif
