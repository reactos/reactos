#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "dataitem.h"
#include "resource.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#define c_cyBarToTitlePadding           12  // vertical padding from botton of fade bar to top of title text
#define c_cyTitleToBodyPadding          6   // vertical padding from bottom of title text to top of body text
#define c_cxRightPanelPadding           16  // generic horizontal padding used on both edges of the right pane

#define c_cyAnimationControl            70

const TCHAR CURITEMKEY[] = TEXT("_Current Item");
const TCHAR ROTATESPEEDKEY[] = TEXT("_Tip Rotation msec");

CDataItem::CDataItem()
{
    m_pszTitle = m_pszMenuName = m_pszDescription = m_pszCmdLine = m_pszArgs = NULL;
    m_dwFlags = 0;
    m_chAccel = NULL;
    m_hBitmap = 0;
    m_cxBitmap = 0;
    m_cyBitmap = 0;
}

CDataItem::~CDataItem()
{
    if ( m_pszTitle )
        delete [] m_pszTitle;
    if ( m_pszMenuName )
        delete [] m_pszMenuName;
    if ( m_pszDescription )
        delete [] m_pszDescription;
    if ( m_pszCmdLine )
        delete [] m_pszCmdLine;
    if ( m_pszArgs )
        delete [] m_pszArgs;
    if ( m_hBitmap )
        DeleteObject(m_hBitmap);
}

BOOL CDataItem::SetData( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex )
{
    TCHAR * psz;

    // This function should only be called once or else we will leak like a, like a, a thing that leaks a lot.
    ASSERT( NULL==m_pszTitle && NULL==m_pszMenuName && NULL==m_pszDescription && NULL==m_pszCmdLine && NULL==m_pszArgs );

    m_pszTitle = new TCHAR[lstrlen(szTitle)+1];
    lstrcpy( m_pszTitle, szTitle );
    if ( szMenu )
    {
        // menuname is allowed to remain NULL.  This is only used if you want the
        // text on the menu item to be different than the description. This could
        // be useful for localization where a shortened name might be required.
        m_pszMenuName = new TCHAR[lstrlen(szMenu)+1];
        lstrcpy( m_pszMenuName, szMenu );

        psz = StrChr(szMenu, TEXT('&'));
        if ( psz )
            m_chAccel = *(CharNext(psz));
    }

    // The description field is special in that it can contain "line feed" information.
    // The only valid escape sequence is the "\n" sequence and everything else is ignored.
    psz = new TCHAR[lstrlen(szDesc)+1];
    m_pszDescription = psz;
    while ( *szDesc )
    {
        if (TEXT('\\') == *szDesc)
        {
            // skip the '\' character and treat the next character as an escape sequence
            TCHAR * pszNext = CharNext(szDesc);
            if ( *pszNext )
            {
                // only advance szDesc if it won't put us past a NULL character
                szDesc = pszNext;
            }

            // We only care about a limited number of escape sequences:
            switch ( *pszNext )
            {
            case TEXT('n'):
                *psz = TEXT('\n');
                break;

            case TEXT('t'):
                *psz = TEXT('\t');
                break;

            case TEXT('x'):
                {
                    TCHAR * pszMajor;
                    TCHAR * pszMinor;

                    pszMajor = CharNext(pszNext);
                    if ( *pszMajor )
                    {
                        pszMinor = CharNext(pszMajor);
                        if ( *pszMinor )
                        {
                            *psz = (TCHAR)((0xf * ((*pszMajor)-TEXT('0'))) + ((*pszMinor)-TEXT('0')));
                            szDesc = pszMinor;
                        }
                    }
                }
                break;

            // if this isn't an escape we care about, just ingore the '\' and continue
            default:
                *psz = *pszNext;
#ifndef WINNT
                if (IsDBCSLeadByte(*pszNext))
                {
                    *(psz+1) = *(pszNext+1);
                }
#endif
            }
        }
        else
        {
            *psz = *szDesc;
#ifndef WINNT
            if (IsDBCSLeadByte(*szDesc))
            {
                *(psz+1) = *(szDesc+1);
            }
#endif
        }
        psz = CharNext(psz);
        szDesc = CharNext(szDesc);
    }
    *psz = TEXT('\0');

    m_pszCmdLine = new TCHAR[lstrlen(szCmd)+1];
    lstrcpy( m_pszCmdLine, szCmd );
    if ( szArgs )
    {
        // Some commands don't have any args so this can remain NULL.  This is only used
        // if the executable requires arguments.
        m_pszArgs = new TCHAR[lstrlen(szArgs)+1];
        lstrcpy( m_pszArgs, szArgs );
    }
    m_dwFlags = dwFlags;

    m_hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BKGND0+iImgIndex), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
    if ( m_hBitmap )
    {
        BITMAP bm;
        GetObject(m_hBitmap,sizeof(bm),&bm);
        m_cxBitmap = bm.bmWidth;
        m_cyBitmap = bm.bmHeight;
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

int CDataItem::GetMinHeight(HDC hdc, int cx, int cyBottomPad)
{
    RECT rect;

    rect.left = c_cxRightPanelPadding;
    rect.top =  c_cyBarToTitlePadding;
    rect.right = cx-c_cxRightPanelPadding;
    SelectObject(hdc,g_pdi->hfontTitle);
    DrawTextEx(hdc,m_pszTitle,-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);

    rect.left = c_cxRightPanelPadding;
    rect.top = rect.bottom + c_cyTitleToBodyPadding;
    rect.right = cx-c_cxRightPanelPadding;
    SelectObject(hdc,g_pdi->hfontBody);
    DrawTextEx(hdc,m_pszDescription,-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);

    return rect.bottom + cyBottomPad;
}

bool CDataItem::DrawPane(HDC hdc, LPRECT prc)
{
    RECT rect = *prc;

    FillRect(hdc, prc, g_pdi->hbrRightPanel);

    // draw background image
    if ( !g_pdi->bHighContrast )
    {
        HDC hdcBkgnd = CreateCompatibleDC(hdc);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBkgnd, m_hBitmap);
        BitBlt( hdc,
                rect.right - m_cxBitmap,
                rect.bottom - m_cyBitmap,
                m_cxBitmap,
                m_cyBitmap,
                hdcBkgnd,0,0, SRCCOPY );
        SelectObject(hdcBkgnd,hbmOld);
        DeleteDC(hdcBkgnd);
    }

    // draw title text
    rect.top += c_cyBarToTitlePadding;
    rect.left += c_cxRightPanelPadding;
    rect.right -= c_cxRightPanelPadding;
    HFONT hfontOld = (HFONT)SelectObject(hdc,g_pdi->hfontTitle);
    SetTextColor(hdc,g_pdi->crTitleText);
    rect.top += c_cyTitleToBodyPadding +
        DrawTextEx(hdc,m_pszTitle,-1,&rect,DT_NOCLIP|DT_WORDBREAK,NULL);

    // draw body text
    SelectObject(hdc,g_pdi->hfontBody);
    SetTextColor(hdc,g_pdi->crNormalText);
    DrawTextEx(hdc,m_pszDescription,-1,&rect,DT_NOCLIP|DT_WORDBREAK,NULL);

    return TRUE;
}

BOOL CDataItem::Invoke(HWND hwnd)
{
    BOOL fResult;
    SHELLEXECUTEINFO ei;
    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.fMask           = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
    ei.hwnd            = NULL;
    ei.lpVerb          = TEXT("open");
    ei.lpFile          = m_pszCmdLine;
    ei.lpParameters    = m_pszArgs;
    ei.lpDirectory     = NULL;
    ei.nShow           = SW_SHOWNORMAL;

    if (fResult = ShellExecuteEx(&ei))
    {
        if (NULL != ei.hProcess)
        {
            DWORD dwObject;

            while (1)
            {
                dwObject = MsgWaitForMultipleObjects(1, &ei.hProcess, FALSE, INFINITE, QS_ALLINPUT);
                
                if (WAIT_OBJECT_0 == dwObject)
                {
                    break;
                }
                else if (WAIT_OBJECT_0+1 == dwObject)
                {
                    MSG msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                    {
                        if ( WM_QUIT == msg.message )
                        {
                            CloseHandle(ei.hProcess);
                            return fResult;
                        }
                        else
                        {
                            GetMessage(&msg, NULL, 0, 0);

                            // IsDialogMessage cannot understand the concept of ownerdraw default pushbuttons.  It treats
                            // these attributes as mutually exclusive.  As a result, we handle this ourselves.  We want
                            // whatever control has focus to act as the default pushbutton.
                            if ( (WM_KEYDOWN == msg.message) && (VK_RETURN == msg.wParam) )
                            {
                                HWND hwndFocus = GetFocus();
                                if ( hwndFocus )
                                {
                                    SendMessage(hwnd, WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndFocus), BN_CLICKED), (LPARAM)hwndFocus);
                                }
                                continue;
                            }

                            if ( IsDialogMessage(hwnd, &msg) )
                                continue;

                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                }
            }

            CloseHandle(ei.hProcess);
        }
    }

    return fResult;
}

// *******************************************************************************
// CDefItem
//
// 
// *******************************************************************************

CDefItem::CDefItem()
{
    m_uRotateSpeed = 30*1000;   // 30 seconds

    m_pszTitle = NULL;
    m_pdid = NULL;
    m_cItems = 0;
    m_iCurItem = -1;
    m_hwndAni = NULL;
    m_cyBottomPad = 0;
    m_bPositioned = FALSE;
    m_hkey = NULL;
    m_bActivePane = FALSE;
}

CDefItem::~CDefItem()
{
    for ( int i=0; i < m_cItems; i++ )
    {
        delete [] m_pdid[i].pszDescription;
        DeleteObject(m_pdid[i].hBitmap);
    }
    delete [] m_pdid;
    delete [] m_pszTitle;
    RegCloseKey(m_hkey);
}

int CDefItem::Init()
{
    TCHAR szBuffer[1024];
    HINSTANCE hInst = GetModuleHandle(NULL);

    LoadString( hInst, IDS_DEFTITLE, szBuffer, ARRAYSIZE(szBuffer) );
    m_pszTitle = new TCHAR[lstrlen(szBuffer)+1];
    if (!m_pszTitle)
        return FALSE;

    StrCpy(m_pszTitle, szBuffer);

    m_cItems = 6;
    m_pdid = new DEFITEMDATA[m_cItems];
    if (!m_pdid)
        return FALSE;

    for ( int i=0; i < m_cItems; i++ )
    {
        LoadString( hInst, IDS_TIPS0+i, szBuffer, ARRAYSIZE(szBuffer) );
        m_pdid[i].pszDescription = new TCHAR[lstrlen(szBuffer)+1];
        if ( !m_pdid[i].pszDescription )
            return FALSE;

        StrCpy(m_pdid[i].pszDescription, szBuffer);

        if ( 1 == i )
        {
            m_pdid[i].iAniCtrl = IDA_MOUSECLICK;
        }
        else
        {
            BITMAP bm;
            m_pdid[i].hBitmap = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_TIP0+i), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);
            
            if ( g_pdi->bHighContrast )
            {
                m_pdid[i].hBitMask = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_TIP0MASK+i), IMAGE_BITMAP, 0,0, LR_MONOCHROME);
            }

            GetObject(m_pdid[i].hBitmap,sizeof(bm),&bm);
            m_pdid[i].cxBitmap = bm.bmWidth;
            m_pdid[i].cyBitmap = bm.bmHeight;
        }
    }

    #ifdef WINNT
    const TCHAR WELCOMESETUP[]    = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome");
    #else
    const TCHAR WELCOMESETUP[]    = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Welcome");
    #endif
    if (ERROR_SUCCESS==RegOpenKeyEx(HKEY_CURRENT_USER, WELCOMESETUP, 0, KEY_READ|KEY_WRITE, &m_hkey))
    {
        DWORD dwType;
        DWORD dwSize = sizeof(m_iCurItem);
        RegQueryValueEx(m_hkey, CURITEMKEY, NULL, &dwType, (LPBYTE)&m_iCurItem, &dwSize);
        if ( (REG_DWORD != dwType) || (m_iCurItem < 0) || (m_iCurItem >= m_cItems) )
        {
            m_iCurItem = -1;
        }

        dwSize = sizeof(m_uRotateSpeed);
        RegQueryValueEx(m_hkey, ROTATESPEEDKEY, NULL, &dwType, (LPBYTE)&m_uRotateSpeed, &dwSize);
        if ( 500 > m_uRotateSpeed )
        {
            m_uRotateSpeed = 500;
        }
    }

    return TRUE;
}

int  CDefItem::GetMinHeight(HDC hdc, int cx, int cyBottomPad)
{
    int cyTallest = 0;
    int cyTitle = 0;
    RECT rect;

    m_cyBottomPad = cyBottomPad;

    rect.left = c_cxRightPanelPadding;
    rect.top =  c_cyBarToTitlePadding;
    rect.right = cx-c_cxRightPanelPadding;
    SelectObject(hdc,g_pdi->hfontTitle);
    DrawTextEx(hdc,m_pszTitle,-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);

    cyTitle = rect.bottom + c_cyTitleToBodyPadding;

    for ( int i=0; i<m_cItems; i++ )
    {
        rect.left = c_cxRightPanelPadding;
        rect.top = cyTitle;
        rect.right = cx-c_cxRightPanelPadding;
        SelectObject(hdc,g_pdi->hfontBody);
        DrawTextEx(hdc,m_pdid[i].pszDescription,-1,&rect,DT_CALCRECT|DT_WORDBREAK,NULL);

        if ( m_pdid[i].hBitmap )
        {
            rect.bottom += m_pdid[i].cyBitmap;
        }
        else
        {
            rect.bottom += c_cyAnimationControl;
        }

        if ( rect.bottom > cyTallest )
            cyTallest = rect.bottom;
    }

    return cyTallest + cyBottomPad;
}

bool CDefItem::DrawPane(HDC hdc, LPRECT prc)
{
    RECT rect = *prc;

    FillRect(hdc, prc, g_pdi->hbrRightPanel);

    // draw title text
    rect.top += c_cyBarToTitlePadding;
    rect.left += c_cxRightPanelPadding;
    rect.right -= c_cxRightPanelPadding;
    HFONT hfontOld = (HFONT)SelectObject(hdc,g_pdi->hfontTitle);
    SetTextColor(hdc,g_pdi->crTitleText);
    rect.top += c_cyTitleToBodyPadding +
        DrawTextEx(hdc,m_pszTitle,-1,&rect,DT_NOCLIP|DT_WORDBREAK,NULL);

    // draw body text
    SelectObject(hdc,g_pdi->hfontBody);
    SetTextColor(hdc,g_pdi->crNormalText);
    rect.top += DrawTextEx(hdc,m_pdid[m_iCurItem].pszDescription,-1,&rect,DT_NOCLIP|DT_WORDBREAK,NULL) + 5;

    // draw the bitmap/animation
    // first we check if the current item has an animation:
    if ( m_pdid[m_iCurItem].iAniCtrl )
    {
        if ( !m_bPositioned)
        {
            SetWindowPos(m_hwndAni, NULL, rect.left, rect.top, (rect.right-rect.left), (prc->bottom-m_cyBottomPad-rect.top), SWP_NOZORDER|SWP_SHOWWINDOW);
            Animate_Play(m_hwndAni, 0, -1, -1);
            m_bPositioned = TRUE;
            m_bActivePane = TRUE;
        }
        else if ( !m_bActivePane )
        {
            // we were not the active pane, that means our ANI window is hidden.  Show the window
            ShowWindow(m_hwndAni, SW_SHOW);
            Animate_Play(m_hwndAni, 0, -1, -1);
            m_bActivePane = TRUE;
        }
    }
    else if ( m_pdid[m_iCurItem].hBitmap )
    {
        if ( m_bActivePane )
        {
            // We were showing an active pane but now we have cycled to a non-active pane
            Deactivate();
        }

        HDC hdcBkgnd = CreateCompatibleDC(hdc);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBkgnd, m_pdid[m_iCurItem].hBitmap);
        if ( g_pdi->bHighContrast )
        {
            MaskBlt( hdc,
                    rect.left,
                    prc->bottom - (m_pdid[m_iCurItem].cyBitmap+m_cyBottomPad),
                    m_pdid[m_iCurItem].cxBitmap,
                    m_pdid[m_iCurItem].cyBitmap,
                    hdcBkgnd,0,0,
                    m_pdid[m_iCurItem].hBitMask, 0,0,
                    SRCCOPY );
        }
        else
        {
            BitBlt( hdc,
                    rect.left,
                    prc->bottom - (m_pdid[m_iCurItem].cyBitmap+m_cyBottomPad),
                    m_pdid[m_iCurItem].cxBitmap,
                    m_pdid[m_iCurItem].cyBitmap,
                    hdcBkgnd,0,0,
                    SRCCOPY );
        }

        SelectObject(hdcBkgnd,hbmOld);
        DeleteDC(hdcBkgnd);
    }

    SelectObject(hdc,hfontOld);
    return TRUE;
}

int CDefItem::Next(HWND hwnd)
{
    if ( ++m_iCurItem >= m_cItems )
    {
        m_iCurItem = 0;
    }

    RegSetValueEx(m_hkey, CURITEMKEY, NULL, REG_DWORD, (LPBYTE)&m_iCurItem, sizeof(m_iCurItem) );

    // Does the selected item have an animate control?  If yes, show it.
    if ( m_pdid[m_iCurItem].iAniCtrl )
    {
#ifdef WINNT
        // On terminal server skip the animated items.  This will break big time if all of the items
        // were to use AVIs, so don't do that :-).
        if ( g_pdi->bTerminalServer )
        {
            return Next(hwnd);
        }
#endif

        HINSTANCE hInst = GetModuleHandle(NULL);
        static int iLastAnimateControl = -1;

        // if we don't have a window, create one:
        if ( !m_hwndAni )
        {
            m_hwndAni = Animate_Create(hwnd, 0, WS_CHILD|ACS_CENTER|ACS_TRANSPARENT, hInst);
            // if we just created the window then it needs to be positioned
            m_bPositioned = FALSE;
        }

        // if our AVI image changed, load the new one:
        if ( m_pdid[m_iCurItem].iAniCtrl != iLastAnimateControl )
        {
            Animate_OpenEx( m_hwndAni, hInst, MAKEINTRESOURCE(m_pdid[m_iCurItem].iAniCtrl) );
            // we need to reposition the animation control every time we switch to a new animation
            m_bPositioned = FALSE;
            iLastAnimateControl = m_pdid[m_iCurItem].iAniCtrl;
        }
    }

    extern int g_iSelectedItem;
    if ( 0 > g_iSelectedItem )
    {
        // BUGBUG: Only invalidate the right pane
        InvalidateRect(hwnd, NULL, TRUE);
    }

    return m_iCurItem;
}

bool CDefItem::Deactivate()
{
    if ( m_bActivePane )
    {
        // We were showing an active pane but now we have cycled to a non-active pane
        ShowWindow(m_hwndAni, SW_HIDE);
        Animate_Stop(m_hwndAni);
        m_bActivePane = FALSE;
    }
    return TRUE;
}