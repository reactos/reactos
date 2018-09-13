//
//  APITHK.C
//
//  This file has API thunks that allow comctl32 to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//

#include "ctlspriv.h"       // Don't use precompiled header here


typedef BOOL (* PFNANIMATEWINDOW)(HWND hwnd, DWORD dwTime, DWORD dwFlags);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's AnimateWindow.

Returns: 
Cond:    --
*/
BOOL
NT5_AnimateWindow(
    IN HWND hwnd,
    IN DWORD dwTime,
    IN DWORD dwFlags)
{
    BOOL bRet = FALSE;
    static PFNANIMATEWINDOW pfn = NULL;

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNANIMATEWINDOW)GetProcAddress(hmod, "AnimateWindow");
    }

    if (pfn)
        bRet = pfn(hwnd, dwTime, dwFlags);

    return bRet;    
}

/*----------------------------------------------------------
Purpose: Shows the tooltip.  On NT4/Win95, this is a standard
         show window.  On NT5/Memphis, this slides the tooltip
         bubble from an invisible point.

Returns: --
Cond:    --
*/

#define CMS_TOOLTIP 135

void SlideAnimate(HWND hwnd, LPCRECT prc)
{
    DWORD dwPos, dwFlags;

    dwPos = GetMessagePos();
    if (GET_Y_LPARAM(dwPos) > prc->top + (prc->bottom - prc->top) / 2)
    {
        dwFlags = AW_VER_NEGATIVE;
    } 
    else
    {
        dwFlags = AW_VER_POSITIVE;
    }

    AnimateWindow(hwnd, CMS_TOOLTIP, dwFlags | AW_SLIDE);
}

STDAPI_(void) CoolTooltipBubble(IN HWND hwnd, IN LPCRECT prc, BOOL fAllowFade, BOOL fAllowAnimate)
{
    ASSERT(prc);

    if (g_bRunOnNT5 || g_bRunOnMemphis)
    {
#ifdef WINNT
        BOOL fAnimate = TRUE;
        SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &fAnimate, 0);
#else
        // Memphis doesn't support the tooltip SPI's, so we piggyback
        // off of SPI_GETSCREENREADER instead.  Note that we want to
        // animate if SPI_GETSCREENREADER is >off<, so we need to do some
        // flippery.  Fortunately, the compiler will optimize all this out.
        BOOL fScreenRead = FALSE;
        BOOL fAnimate;
        SystemParametersInfo(SPI_GETSCREENREADER, 0, &fScreenRead, 0);
        fAnimate = !fScreenRead;
#endif
        if (fAnimate)
        {
            fAnimate = FALSE;
#ifdef WINNT
            SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &fAnimate, 0);
#endif // WINNT
            if (fAnimate && fAllowFade)
            {
                AnimateWindow(hwnd, CMS_TOOLTIP, AW_BLEND);
            }
            else if (fAllowAnimate)
            {
                SlideAnimate(hwnd, prc);
            }
            else
                goto UseSetWindowPos;
        }
        else
            goto UseSetWindowPos;
    }
    else
    {
UseSetWindowPos:
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, 
                     SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
    }
}



/*----------------------------------------------------------
Purpose: Get the COLOR_HOTLIGHT system color index from NT5 or Memphis.
         Get COLOR_HIGHLIGHT from NT4 or Win95, where COLOR_HOTLIGHT is not defined.

Returns: --
Cond:    --
*/
int GetCOLOR_HOTLIGHT()
{
    return (g_bRunOnNT5 || g_bRunOnMemphis) ? COLOR_HOTLIGHT : COLOR_HIGHLIGHT;
}


STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes)
{
    if (g_bRunOnNT5 || g_bRunOnMemphis)
    {
        HCURSOR hcur = LoadCursor(NULL, IDC_HAND);  // from USER, system supplied
        if (hcur)
            return hcur;
    }

    return LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_HAND_INTERNAL));
}

typedef BOOL (*PFNQUEUEUSERWORKITEM)(LPTHREAD_START_ROUTINE Function,
    PVOID Context, BOOL PreferIo);

STDAPI_(BOOL) NT5_QueueUserWorkItem(LPTHREAD_START_ROUTINE Function,
    PVOID Context, BOOL PreferIo)
{
    BOOL bRet = FALSE;
    static PFNQUEUEUSERWORKITEM pfn = (PFNQUEUEUSERWORKITEM)-1;

    if ((PFNQUEUEUSERWORKITEM)-1 == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));
        
        if (hmod)
            pfn = (PFNQUEUEUSERWORKITEM)GetProcAddress(hmod, "QueueUserWorkItem");
        else
            pfn = NULL;
    }

    if (pfn)
        bRet = pfn( Function, Context, PreferIo);

    return bRet;    
}

//
//  Here's how CAL_ITWODIGITYEARMAX works.
//
//  If a two-digit year is input from the user, we put it into the range
//  (N-99) ... N.  for example, if the maximum value is 2029, then all
//  two-digit numbers will be coerced into the range 1930 through 2029.
//
//  Win95 and NT4 don't have GetCalendarInfo, but they do have
//  EnumCalendarInfo, so you'd think we could avoid the GetProcAddress
//  by enumerating the one calendar we care about.
//
//  Unfortunately, Win98 has a bug where EnumCalendarInfo can't enumerate
//  the maximum two-digit year value!  What a lamer!
//
//  So we're stuck with GetProcAddress.
//
//  But wait, Win98 exports GetCalendarInfoW but doesn't implement it!
//  Double lame!
//
//  So we have to use the Ansi version exclusively.  Fortunately, we
//  are only interested in numbers (so far) so there is no loss of amenity.
//
//  First, here's the dummy function that emulates GetCalendarInfoA
//  on Win95 and NT4.
//

STDAPI_(int)
Emulate_GetCalendarInfoA(LCID lcid, CALID calid, CALTYPE cal,
                         LPSTR pszBuf, int cchBuf, LPDWORD pdwOut)
{
    //
    //  In the absence of the API, we go straight for the information
    //  in the registry.
    //
    BOOL fSuccess = FALSE;
    HKEY hkey;

    ASSERT(cal == CAL_RETURN_NUMBER + CAL_ITWODIGITYEARMAX);
    ASSERT(pszBuf == NULL);
    ASSERT(cchBuf == 0);

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Control Panel\\International\\Calendars\\TwoDigitYearMax",
                      0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        char szKey[16];
        char szBuf[64];
        DWORD dwSize;

        wsprintfA(szKey, "%d", calid);

        dwSize = sizeof(szBuf);
        if (RegQueryValueExA(hkey, szKey, 0, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            *pdwOut = StrToIntA(szBuf);
            fSuccess = TRUE;
        }

        RegCloseKey(hkey);
    }
    return fSuccess;

}

typedef int (CALLBACK *GETCALENDARINFOA)(LCID, CALID, CALTYPE, LPSTR, int, LPDWORD);

GETCALENDARINFOA _GetCalendarInfoA;

STDAPI_(int)
NT5_GetCalendarInfoA(LCID lcid, CALID calid, CALTYPE cal,
                     LPSTR pszBuf, int cchBuf, LPDWORD pdwOut)
{
    // This is the only function our emulator supports
    ASSERT(cal == CAL_RETURN_NUMBER + CAL_ITWODIGITYEARMAX);
    ASSERT(pszBuf == NULL);
    ASSERT(cchBuf == 0);

    if (_GetCalendarInfoA == NULL)
    {
        HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

        //
        //  Must keep in a local to avoid thread races.
        //
        GETCALENDARINFOA pfn = NULL;

        if (hmod)
            pfn = (GETCALENDARINFOA)
                    GetProcAddress(hmod, "GetCalendarInfoA");

        //
        //  If function is not available, then use our fallback
        //
        if (pfn == NULL)
            pfn = Emulate_GetCalendarInfoA;

        ASSERT(pfn != NULL);
        _GetCalendarInfoA = pfn;
    }

    return _GetCalendarInfoA(lcid, calid, cal, pszBuf, cchBuf, pdwOut);
}
