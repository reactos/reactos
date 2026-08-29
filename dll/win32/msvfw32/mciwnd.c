/*
 * Copyright 2000 Eric Pouech
 * Copyright 2003 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME:
 * Add support for all remaining MCI_ commands and MCIWNDM_ messages.
 * Add support for MCIWNDF_RECORD.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "vfw.h"
#include "digitalv.h"
#include "commctrl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mci);

extern HMODULE MSVFW32_hModule;
static const WCHAR mciWndClassW[] = {'M','C','I','W','n','d','C','l','a','s','s',0};
static const WCHAR mciWndNameW[] = {'M','C','I','W','n','d','C','r','e','a','t','e',
                                    'W','i','n','e','I','n','t','e','r','n','a','l', 0};

typedef struct
{
    DWORD       dwStyle;
    MCIDEVICEID mci;
    HDRVR       hdrv;
    int         alias;
    UINT        dev_type;
    UINT        mode;
    LONG        position;
    SIZE        size; /* size of the original frame rect */
    int         zoom;
    LPWSTR      lpName;
    HWND        hWnd, hwndOwner;
    UINT        uTimer;
    MCIERROR    lasterror;
    WCHAR       return_string[128];
    WORD        active_timer, inactive_timer;
} MCIWndInfo;

static LRESULT WINAPI MCIWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#define CTL_PLAYSTOP    0x3200
#define CTL_MENU        0x3201
#define CTL_TRACKBAR    0x3202

/***********************************************************************
 *                MCIWndRegisterClass                [MSVFW32.@]
 *
 * NOTE: Native always uses its own hInstance
 */
BOOL VFWAPIV MCIWndRegisterClass(void)
{
    WNDCLASSW wc;

    /* Since we are going to register a class belonging to MSVFW32
     * and later we will create windows with a different hInstance
     * CS_GLOBALCLASS is needed. And because the second attempt
     * to register a global class will fail we need to test whether
     * the class was already registered.
     */
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC | CS_GLOBALCLASS;
    wc.lpfnWndProc = MCIWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(MCIWndInfo*);
    wc.hInstance = MSVFW32_hModule;
    wc.hIcon = 0;
    wc.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = mciWndClassW;

    if (RegisterClassW(&wc)) return TRUE;
    if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) return TRUE;

    return FALSE;
}

/***********************************************************************
 *                MCIWndCreateW                                [MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
                           DWORD dwStyle, LPCWSTR szFile)
{
    TRACE("%p %p %lx %s\n", hwndParent, hInstance, dwStyle, debugstr_w(szFile));

    MCIWndRegisterClass();

    if (!hInstance) hInstance = GetModuleHandleW(0);

    if (hwndParent)
        dwStyle |= WS_VISIBLE | WS_BORDER /*| WS_CHILD*/;
    else
        dwStyle |= WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

    return CreateWindowExW(0, mciWndClassW, mciWndNameW,
                           dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                           0, 0, 300, 0,
                           hwndParent, 0, hInstance, (LPVOID)szFile);
}

/***********************************************************************
 *                MCIWndCreate                [MSVFW32.@]
 *                MCIWndCreateA                [MSVFW32.@]
 */
HWND VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
                           DWORD dwStyle, LPCSTR szFile)
{
    HWND ret;
    UNICODE_STRING fileW;

    if (szFile)
        RtlCreateUnicodeStringFromAsciiz(&fileW, szFile);
    else
        fileW.Buffer = NULL;

    ret = MCIWndCreateW(hwndParent, hInstance, dwStyle, fileW.Buffer);

    RtlFreeUnicodeString(&fileW);
    return ret;
}

static inline void MCIWND_notify_mode(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & MCIWNDF_NOTIFYMODE)
    {
        UINT new_mode = SendMessageW(mwi->hWnd, MCIWNDM_GETMODEW, 0, 0);
        if (new_mode != mwi->mode)
        {
            mwi->mode = new_mode;
            SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMODE, (WPARAM)mwi->hWnd, new_mode);
        }
    }
}

static inline void MCIWND_notify_pos(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & MCIWNDF_NOTIFYPOS)
    {
        LONG new_pos = SendMessageW(mwi->hWnd, MCIWNDM_GETPOSITIONW, 0, 0);
        if (new_pos != mwi->position)
        {
            mwi->position = new_pos;
            SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYPOS, (WPARAM)mwi->hWnd, new_pos);
        }
    }
}

static inline void MCIWND_notify_size(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & MCIWNDF_NOTIFYSIZE)
        SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYSIZE, (WPARAM)mwi->hWnd, 0);
}

static inline void MCIWND_notify_error(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & MCIWNDF_NOTIFYERROR)
        SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYERROR, (WPARAM)mwi->hWnd, (LPARAM)mwi->lasterror);
}

static void MCIWND_UpdateState(MCIWndInfo *mwi)
{
    WCHAR buffer[1024];

    if (!mwi->mci)
    {
        /* FIXME: get this from resources */
        static const WCHAR no_deviceW[] = {'N','o',' ','D','e','v','i','c','e',0};
        SetWindowTextW(mwi->hWnd, no_deviceW);
        return;
    }

    MCIWND_notify_pos(mwi);

    if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
        SendDlgItemMessageW(mwi->hWnd, CTL_TRACKBAR, TBM_SETPOS, TRUE, mwi->position);

    if (!(mwi->dwStyle & MCIWNDF_SHOWALL))
        return;

    if ((mwi->dwStyle & MCIWNDF_SHOWNAME) && mwi->lpName)
        lstrcpyW(buffer, mwi->lpName);
    else
        *buffer = 0;

    if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR spaceW[] = {' ',0};
        static const WCHAR l_braceW[] = {'(',0};

        if (*buffer) lstrcatW(buffer, spaceW);
        lstrcatW(buffer, l_braceW);
    }

    if (mwi->dwStyle & MCIWNDF_SHOWPOS)
    {
        WCHAR posW[64];

        posW[0] = 0;
        SendMessageW(mwi->hWnd, MCIWNDM_GETPOSITIONW, 64, (LPARAM)posW);
        lstrcatW(buffer, posW);
    }

    if ((mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE)) == (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR dashW[] = {' ','-',' ',0};
        lstrcatW(buffer, dashW);
    }

    if (mwi->dwStyle & MCIWNDF_SHOWMODE)
    {
        WCHAR modeW[64];

        modeW[0] = 0;
        SendMessageW(mwi->hWnd, MCIWNDM_GETMODEW, 64, (LPARAM)modeW);
        lstrcatW(buffer, modeW);
    }

    if (mwi->dwStyle & (MCIWNDF_SHOWPOS|MCIWNDF_SHOWMODE))
    {
        static const WCHAR r_braceW[] = {')',0};
        lstrcatW(buffer, r_braceW);
    }

    TRACE("=> %s\n", debugstr_w(buffer));
    SetWindowTextW(mwi->hWnd, buffer);
}

static LRESULT MCIWND_Create(HWND hWnd, LPCREATESTRUCTW cs)
{
    HWND hChld;
    MCIWndInfo *mwi;
    static const WCHAR buttonW[] = {'b','u','t','t','o','n',0};

    mwi = calloc(1, sizeof(*mwi));
    if (!mwi) return -1;

    SetWindowLongPtrW(hWnd, 0, (LPARAM)mwi);

    mwi->dwStyle = cs->style;
    /* There is no need to show stats if there is no caption */
    if ((mwi->dwStyle & WS_CAPTION) != WS_CAPTION)
        mwi->dwStyle &= ~MCIWNDF_SHOWALL;

    mwi->hWnd = hWnd;
    mwi->hwndOwner = cs->hwndParent;
    mwi->active_timer = 500;
    mwi->inactive_timer = 2000;
    mwi->mode = MCI_MODE_NOT_READY;
    mwi->position = -1;
    mwi->zoom = 100;

    if (!(mwi->dwStyle & MCIWNDF_NOMENU))
    {
        static const WCHAR menuW[] = {'M','e','n','u',0};

        hChld = CreateWindowExW(0, buttonW, menuW, WS_CHILD|WS_VISIBLE, 32, cs->cy, 32, 32,
                                hWnd, (HMENU)CTL_MENU, cs->hInstance, 0L);
        TRACE("Get Button2: %p\n", hChld);
    }

    if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
    {
        INITCOMMONCONTROLSEX init;
        static const WCHAR playW[] = {'P','l','a','y',0};

        /* adding the other elements: play/stop button, menu button, status */
        hChld = CreateWindowExW(0, buttonW, playW, WS_CHILD|WS_VISIBLE, 0, cs->cy, 32, 32,
                                hWnd, (HMENU)CTL_PLAYSTOP, cs->hInstance, 0L);
        TRACE("Get Button1: %p\n", hChld);

        init.dwSize = sizeof(init);
        init.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&init);

        hChld = CreateWindowExW(0, TRACKBAR_CLASSW, NULL, WS_CHILD|WS_VISIBLE, 64, cs->cy, cs->cx - 64, 32,
                                hWnd, (HMENU)CTL_TRACKBAR, cs->hInstance, 0L);
        TRACE("Get status: %p\n", hChld);
    }

    /* This sets the default window size */
    SendMessageW(hWnd, MCI_CLOSE, 0, 0);

    if (cs->lpCreateParams)
    {
        LPARAM lParam;

        /* MCI wnd class is prepared to be embedded as an MDI child window */
        if (cs->dwExStyle & WS_EX_MDICHILD)
        {
            MDICREATESTRUCTW *mdics = cs->lpCreateParams;
            lParam = mdics->lParam;
        }
        else
            lParam = (LPARAM)cs->lpCreateParams;

        /* If it's our internal window name, we are being called from MCIWndCreateA/W,
         * so file name is a unicode string */
        if (!lstrcmpW(cs->lpszName, mciWndNameW))
            SendMessageW(hWnd, MCIWNDM_OPENW, 0, lParam);
        else
        {
            /* Otherwise let's try to figure out what string format is used */
            HWND parent = cs->hwndParent;
            if (!parent) parent = GetWindow(hWnd, GW_OWNER);

            SendMessageW(hWnd, IsWindowUnicode(parent) ? MCIWNDM_OPENW : MCIWNDM_OPENA, 0, lParam);
        }
    }

    return 0;
}

static void MCIWND_ToggleState(MCIWndInfo *mwi)
{
    switch (SendMessageW(mwi->hWnd, MCIWNDM_GETMODEW, 0, 0))
    {
    case MCI_MODE_NOT_READY:
    case MCI_MODE_RECORD:
    case MCI_MODE_SEEK:
    case MCI_MODE_OPEN:
        TRACE("Cannot do much...\n");
        break;

    case MCI_MODE_PAUSE:
        SendMessageW(mwi->hWnd, MCI_RESUME, 0, 0);
        break;

    case MCI_MODE_PLAY:
        SendMessageW(mwi->hWnd, MCI_PAUSE, 0, 0);
        break;

    case MCI_MODE_STOP:
        SendMessageW(mwi->hWnd, MCI_STOP, 0, 0);
        break;
    }
}

static LRESULT MCIWND_Command(MCIWndInfo *mwi, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case CTL_PLAYSTOP: MCIWND_ToggleState(mwi); break;
    case CTL_MENU:
    case CTL_TRACKBAR:
    default:
        FIXME("support for command %04x not implement yet\n", LOWORD(wParam));
    }
    return 0L;
}

static void MCIWND_notify_media(MCIWndInfo *mwi)
{
    if (mwi->dwStyle & (MCIWNDF_NOTIFYMEDIAA | MCIWNDF_NOTIFYMEDIAW))
    {
        if (!mwi->lpName)
        {
            static const WCHAR empty_str[1];
            SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)empty_str);
        }
        else
        {
            if (mwi->dwStyle & MCIWNDF_NOTIFYANSI)
            {
                char *ansi_name;
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, mwi->lpName, -1, NULL, 0, NULL, NULL);
                ansi_name = malloc(len);
                WideCharToMultiByte(CP_ACP, 0, mwi->lpName, -1, ansi_name, len, NULL, NULL);

                SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)ansi_name);

                free(ansi_name);
            }
            else
                SendMessageW(mwi->hwndOwner, MCIWNDM_NOTIFYMEDIA, (WPARAM)mwi->hWnd, (LPARAM)mwi->lpName);
        }
    }
}

static MCIERROR mci_generic_command(MCIWndInfo *mwi, UINT cmd)
{
    MCI_GENERIC_PARMS mci_generic;

    mci_generic.dwCallback = 0;
    mwi->lasterror = mciSendCommandW(mwi->mci, cmd, 0, (DWORD_PTR)&mci_generic);

    if (mwi->lasterror)
        return mwi->lasterror;

    MCIWND_notify_mode(mwi);
    MCIWND_UpdateState(mwi);
    return 0;
}

static LRESULT mci_get_devcaps(MCIWndInfo *mwi, UINT cap)
{
    MCI_GETDEVCAPS_PARMS mci_devcaps;

    mci_devcaps.dwItem = cap;
    mwi->lasterror = mciSendCommandW(mwi->mci, MCI_GETDEVCAPS,
                                   MCI_GETDEVCAPS_ITEM,
                                   (DWORD_PTR)&mci_devcaps);
    if (mwi->lasterror)
        return 0;

    return mci_devcaps.dwReturn;
}

static LRESULT MCIWND_KeyDown(MCIWndInfo *mwi, UINT key)
{
    TRACE("%p, key %04x\n", mwi->hWnd, key);

    switch(key)
    {
    case VK_ESCAPE:
        SendMessageW(mwi->hWnd, MCI_STOP, 0, 0);
        return 0;

    default:
        return 0;
    }
}

static LRESULT WINAPI MCIWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    MCIWndInfo *mwi;

    TRACE("%p %04x %08Ix %08Ix\n", hWnd, wMsg, wParam, lParam);

    mwi = (MCIWndInfo*)GetWindowLongPtrW(hWnd, 0);
    if (!mwi && wMsg != WM_CREATE)
        return DefWindowProcW(hWnd, wMsg, wParam, lParam);

    switch (wMsg)
    {
    case WM_CREATE:
        MCIWND_Create(hWnd, (CREATESTRUCTW *)lParam);
        break;

    case WM_DESTROY:
        if (mwi->uTimer)
            KillTimer(hWnd, mwi->uTimer);

        if (mwi->mci)
            SendMessageW(hWnd, MCI_CLOSE, 0, 0);

        free(mwi);

        DestroyWindow(GetDlgItem(hWnd, CTL_MENU));
        DestroyWindow(GetDlgItem(hWnd, CTL_PLAYSTOP));
        DestroyWindow(GetDlgItem(hWnd, CTL_TRACKBAR));
        break;

    case WM_PAINT:
        {
            MCI_DGV_UPDATE_PARMS mci_update;
            PAINTSTRUCT ps;

            mci_update.hDC = (wParam) ? (HDC)wParam : BeginPaint(hWnd, &ps);

            mciSendCommandW(mwi->mci, MCI_UPDATE,
                            MCI_DGV_UPDATE_HDC | MCI_DGV_UPDATE_PAINT,
                            (DWORD_PTR)&mci_update);

            if (!wParam) EndPaint(hWnd, &ps);
            return 1;
        }

    case WM_COMMAND:
        return MCIWND_Command(mwi, wParam, lParam);

    case WM_KEYDOWN:
        return MCIWND_KeyDown(mwi, wParam);

    case WM_NCACTIVATE:
        if (mwi->uTimer)
        {
            KillTimer(hWnd, mwi->uTimer);
            mwi->uTimer = SetTimer(hWnd, 1, wParam ? mwi->active_timer : mwi->inactive_timer, NULL);
        }
        break;

    case WM_TIMER:
        MCIWND_UpdateState(mwi);
        return 0;

    case WM_SIZE:
        SetWindowPos(GetDlgItem(hWnd, CTL_PLAYSTOP), 0, 0, HIWORD(lParam) - 32, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hWnd, CTL_MENU), 0, 32, HIWORD(lParam) - 32, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hWnd, CTL_TRACKBAR), 0, 64, HIWORD(lParam) - 32, LOWORD(lParam) - 64, 32, SWP_NOACTIVATE);

        if (!(mwi->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE))
        {
            RECT rc;

            rc.left = rc.top = 0;
            rc.right = LOWORD(lParam);
            rc.bottom = HIWORD(lParam);
            if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
                rc.bottom -= 32; /* subtract the height of the playbar */
            SendMessageW(hWnd, MCIWNDM_PUT_DEST, 0, (LPARAM)&rc);
        }
        MCIWND_notify_size(mwi);
        break;

    case MM_MCINOTIFY:
        MCIWND_notify_mode(mwi);
        MCIWND_UpdateState(mwi);
        return 0;

    case MCIWNDM_OPENA:
        {
            UNICODE_STRING nameW;
            TRACE("MCIWNDM_OPENA %s\n", debugstr_a((LPSTR)lParam));
            RtlCreateUnicodeStringFromAsciiz(&nameW, (LPCSTR)lParam);
            lParam = (LPARAM)nameW.Buffer;
        }
        /* fall through */
    case MCIWNDM_OPENW:
        {
            RECT rc;
            HCURSOR hCursor;
            MCI_OPEN_PARMSW mci_open;
            MCI_GETDEVCAPS_PARMS mci_devcaps;
            WCHAR aliasW[64];
            WCHAR drv_name[MAX_PATH];
            static const WCHAR formatW[] = {'%','d',0};
            static const WCHAR mci32W[] = {'m','c','i','3','2',0};
            static const WCHAR system_iniW[] = {'s','y','s','t','e','m','.','i','n','i',0};

            TRACE("MCIWNDM_OPENW %s\n", debugstr_w((LPWSTR)lParam));

            if (wParam == MCIWNDOPENF_NEW)
            {
                SendMessageW(hWnd, MCIWNDM_NEWW, 0, lParam);
                goto end_of_mci_open;
            }

            if (mwi->uTimer)
            {
                KillTimer(hWnd, mwi->uTimer);
                mwi->uTimer = 0;
            }

            hCursor = LoadCursorW(0, (LPWSTR)IDC_WAIT);
            hCursor = SetCursor(hCursor);

            mci_open.lpstrElementName = (LPWSTR)lParam;
            wsprintfW(aliasW, formatW, HandleToLong(hWnd) + 1);
            mci_open.lpstrAlias = aliasW;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_OPEN,
                                             MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS | MCI_WAIT,
                                             (DWORD_PTR)&mci_open);
            SetCursor(hCursor);

            if (mwi->lasterror && !(mwi->dwStyle & MCIWNDF_NOERRORDLG))
            {
                /* FIXME: get the caption from resources */
                static const WCHAR caption[] = {'M','C','I',' ','E','r','r','o','r',0};
                WCHAR error_str[MAXERRORLENGTH];

                mciGetErrorStringW(mwi->lasterror, error_str, MAXERRORLENGTH);
                MessageBoxW(hWnd, error_str, caption, MB_ICONEXCLAMATION | MB_OK);
                MCIWND_notify_error(mwi);
                goto end_of_mci_open;
            }

            mwi->mci = mci_open.wDeviceID;
            mwi->alias = HandleToLong(hWnd) + 1;
            mwi->lpName = wcsdup((WCHAR *)lParam);

            MCIWND_UpdateState(mwi);

            mci_devcaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_GETDEVCAPS,
                                             MCI_GETDEVCAPS_ITEM,
                                             (DWORD_PTR)&mci_devcaps);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                goto end_of_mci_open;
            }

            mwi->dev_type = mci_devcaps.dwReturn;

            drv_name[0] = 0;
            SendMessageW(hWnd, MCIWNDM_GETDEVICEW, 256, (LPARAM)drv_name);
            if (drv_name[0] && GetPrivateProfileStringW(mci32W, drv_name, NULL,
                                            drv_name, MAX_PATH, system_iniW))
                mwi->hdrv = OpenDriver(drv_name, NULL, 0);

            if (mwi->dev_type == MCI_DEVTYPE_DIGITAL_VIDEO)
            {
                MCI_DGV_WINDOW_PARMSW mci_window;

                mci_window.hWnd = hWnd;
                mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WINDOW,
                                                 MCI_DGV_WINDOW_HWND,
                                                 (DWORD_PTR)&mci_window);
                if (mwi->lasterror)
                {
                    MCIWND_notify_error(mwi);
                    goto end_of_mci_open;
                }
            }

            if (SendMessageW(hWnd, MCIWNDM_GET_DEST, 0, (LPARAM)&rc) == 0)
            {
                mwi->size.cx = rc.right - rc.left;
                mwi->size.cy = rc.bottom - rc.top;

                rc.right = MulDiv(mwi->size.cx, mwi->zoom, 100);
                rc.bottom = MulDiv(mwi->size.cy, mwi->zoom, 100);
                SendMessageW(hWnd, MCIWNDM_PUT_DEST, 0, (LPARAM)&rc);
            }
            else
            {
                GetClientRect(hWnd, &rc);
                rc.bottom = rc.top;
            }

            if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
                rc.bottom += 32; /* add the height of the playbar */
            AdjustWindowRect(&rc, GetWindowLongW(hWnd, GWL_STYLE), FALSE);
            SetWindowPos(hWnd, 0, 0, 0, rc.right - rc.left,
                         rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            SendDlgItemMessageW(hWnd, CTL_TRACKBAR, TBM_SETRANGEMIN, 0L, 0L);
            SendDlgItemMessageW(hWnd, CTL_TRACKBAR, TBM_SETRANGEMAX, 1,
                                SendMessageW(hWnd, MCIWNDM_GETLENGTH, 0, 0));
            mwi->uTimer = SetTimer(hWnd, 1, mwi->active_timer, NULL);

            MCIWND_notify_media(mwi);

end_of_mci_open:
            if (wMsg == MCIWNDM_OPENA)
                free((void *)lParam);
            return mwi->lasterror;
        }

    case MCIWNDM_GETDEVICEID:
        TRACE("MCIWNDM_GETDEVICEID\n");
        return mwi->mci;

    case MCIWNDM_GETALIAS:
        TRACE("MCIWNDM_GETALIAS\n");
        return mwi->alias;

    case MCIWNDM_GET_SOURCE:
        {
            MCI_DGV_RECT_PARMS mci_rect;

            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WHERE,
                                             MCI_DGV_WHERE_SOURCE,
                                             (DWORD_PTR)&mci_rect);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }
            *(RECT *)lParam = mci_rect.rc;
            TRACE("MCIWNDM_GET_SOURCE: %s\n", wine_dbgstr_rect(&mci_rect.rc));
            return 0;
        }

    case MCIWNDM_GET_DEST:
        {
            MCI_DGV_RECT_PARMS mci_rect;

            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_WHERE,
                                             MCI_DGV_WHERE_DESTINATION,
                                             (DWORD_PTR)&mci_rect);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }
            *(RECT *)lParam = mci_rect.rc;
            TRACE("MCIWNDM_GET_DEST: %s\n", wine_dbgstr_rect(&mci_rect.rc));
            return 0;
        }

    case MCIWNDM_PUT_SOURCE:
        {
            MCI_DGV_PUT_PARMS mci_put;

            mci_put.rc = *(RECT *)lParam;
            TRACE("MCIWNDM_PUT_SOURCE: %s\n", wine_dbgstr_rect(&mci_put.rc));
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PUT,
                                             MCI_DGV_PUT_SOURCE,
                                             (DWORD_PTR)&mci_put);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCIWNDM_PUT_DEST:
        {
            MCI_DGV_PUT_PARMS mci_put;

            mci_put.rc = *(RECT *)lParam;
            TRACE("MCIWNDM_PUT_DEST: %s\n", wine_dbgstr_rect(&mci_put.rc));

            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PUT,
                                             MCI_DGV_PUT_DESTINATION | MCI_DGV_RECT,
                                             (DWORD_PTR)&mci_put);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }
            return 0;
        }

    case MCIWNDM_GETLENGTH:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_LENGTH;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return 0;
            }
            TRACE("MCIWNDM_GETLENGTH: %Id\n", mci_status.dwReturn);
            return mci_status.dwReturn;
        }

    case MCIWNDM_GETSTART:
        {
            MCI_STATUS_PARMS mci_status;

            mci_status.dwItem = MCI_STATUS_POSITION;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM | MCI_STATUS_START,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return 0;
            }
            TRACE("MCIWNDM_GETSTART: %Id\n", mci_status.dwReturn);
            return mci_status.dwReturn;
        }

    case MCIWNDM_GETEND:
        {
            LRESULT start, length;

            start = SendMessageW(hWnd, MCIWNDM_GETSTART, 0, 0);
            length = SendMessageW(hWnd, MCIWNDM_GETLENGTH, 0, 0);
            TRACE("MCIWNDM_GETEND: %Id\n", start + length);
            return (start + length);
        }

    case MCIWNDM_GETPOSITIONA:
    case MCIWNDM_GETPOSITIONW:
        {
            MCI_STATUS_PARMS mci_status;

            TRACE("MCIWNDM_GETPOSITION\n");

            /* get position string if requested */
            if (wParam && lParam)
            {
                if (wMsg == MCIWNDM_GETPOSITIONA)
                {
                    char cmd[64];

                    wsprintfA(cmd, "status %d position", mwi->alias);
                    mwi->lasterror = mciSendStringA(cmd, (LPSTR)lParam, wParam, 0);
                }
                else
                {

                    WCHAR cmdW[64];
                    static const WCHAR formatW[] = {'s','t','a','t','u','s',' ','%','d',' ','p','o','s','i','t','i','o','n',0};

                    wsprintfW(cmdW, formatW, mwi->alias);
                    mwi->lasterror = mciSendStringW(cmdW, (LPWSTR)lParam, wParam, 0);
                }

                if (mwi->lasterror)
                    return 0;
            }

            mci_status.dwItem = MCI_STATUS_POSITION;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
                return 0;

            return mci_status.dwReturn;
        }

    case MCIWNDM_GETMODEA:
    case MCIWNDM_GETMODEW:
        {
            MCI_STATUS_PARMS mci_status;

            TRACE("MCIWNDM_GETMODE\n");

            if (!mwi->mci)
                return MCI_MODE_NOT_READY;

            /* get mode string if requested */
            if (wParam && lParam)
            {
                if (wMsg == MCIWNDM_GETMODEA)
                {
                    char cmd[64];

                    wsprintfA(cmd, "status %d mode", mwi->alias);
                    mwi->lasterror = mciSendStringA(cmd, (LPSTR)lParam, wParam, 0);
                }
                else
                {

                    WCHAR cmdW[64];
                    static const WCHAR formatW[] = {'s','t','a','t','u','s',' ','%','d',' ','m','o','d','e',0};

                    wsprintfW(cmdW, formatW, mwi->alias);
                    mwi->lasterror = mciSendStringW(cmdW, (LPWSTR)lParam, wParam, 0);
                }

                if (mwi->lasterror)
                    return MCI_MODE_NOT_READY;
            }

            mci_status.dwItem = MCI_STATUS_MODE;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
                return MCI_MODE_NOT_READY;

            return mci_status.dwReturn;
        }

    case MCIWNDM_PLAYFROM:
        {
            MCI_PLAY_PARMS mci_play;

            TRACE("MCIWNDM_PLAYFROM %08Ix\n", lParam);

            mci_play.dwCallback = (DWORD_PTR)hWnd;
            mci_play.dwFrom = lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PLAY,
                                             MCI_FROM | MCI_NOTIFY,
                                             (DWORD_PTR)&mci_play);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }

            MCIWND_notify_mode(mwi);
            MCIWND_UpdateState(mwi);
            return 0;
        }

    case MCIWNDM_PLAYTO:
        {
            MCI_PLAY_PARMS mci_play;

            TRACE("MCIWNDM_PLAYTO %08Ix\n", lParam);

            mci_play.dwCallback = (DWORD_PTR)hWnd;
            mci_play.dwTo = lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PLAY,
                                             MCI_TO | MCI_NOTIFY,
                                             (DWORD_PTR)&mci_play);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }

            MCIWND_notify_mode(mwi);
            MCIWND_UpdateState(mwi);
            return 0;
        }

    case MCIWNDM_PLAYREVERSE:
        {
            MCI_PLAY_PARMS mci_play;
            DWORD flags = MCI_NOTIFY;

            TRACE("MCIWNDM_PLAYREVERSE %08Ix\n", lParam);

            mci_play.dwCallback = (DWORD_PTR)hWnd;
            mci_play.dwFrom = lParam;
            switch (mwi->dev_type)
            {
            default:
            case MCI_DEVTYPE_ANIMATION:
                flags |= MCI_ANIM_PLAY_REVERSE;
                break;

            case MCI_DEVTYPE_DIGITAL_VIDEO:
                flags |= MCI_DGV_PLAY_REVERSE;
                break;

#ifdef MCI_VCR_PLAY_REVERSE
            case MCI_DEVTYPE_VCR:
                flags |= MCI_VCR_PLAY_REVERSE;
                break;
#endif

            case MCI_DEVTYPE_VIDEODISC:
                flags |= MCI_VD_PLAY_REVERSE;
                break;

            }
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_PLAY,
                                             flags, (DWORD_PTR)&mci_play);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }

            MCIWND_notify_mode(mwi);
            MCIWND_UpdateState(mwi);
            return 0;
        }

    case MCIWNDM_GETERRORA:
        mciGetErrorStringA(mwi->lasterror, (LPSTR)lParam, wParam);
        TRACE("MCIWNDM_GETERRORA: %s\n", debugstr_an((LPSTR)lParam, wParam));
        return mwi->lasterror;

    case MCIWNDM_GETERRORW:
        mciGetErrorStringW(mwi->lasterror, (LPWSTR)lParam, wParam);
        TRACE("MCIWNDM_GETERRORW: %s\n", debugstr_wn((LPWSTR)lParam, wParam));
        return mwi->lasterror;

    case MCIWNDM_SETOWNER:
        TRACE("MCIWNDM_SETOWNER %p\n", (HWND)wParam);
        mwi->hwndOwner = (HWND)wParam;
        return 0;

    case MCIWNDM_SENDSTRINGA:
        {
            UNICODE_STRING stringW;

            TRACE("MCIWNDM_SENDSTRINGA %s\n", debugstr_a((LPCSTR)lParam));

            RtlCreateUnicodeStringFromAsciiz(&stringW, (LPCSTR)lParam);
            lParam = (LPARAM)stringW.Buffer;
        }
        /* fall through */
    case MCIWNDM_SENDSTRINGW:
        {
            WCHAR *cmdW, *p;

            TRACE("MCIWNDM_SENDSTRINGW %s\n", debugstr_w((LPCWSTR)lParam));

            p = wcschr((LPCWSTR)lParam, ' ');
            if (p)
            {
                static const WCHAR formatW[] = {'%','d',' ',0};
                int len, pos;

                pos = p - (WCHAR *)lParam + 1;
                len = lstrlenW((LPCWSTR)lParam) + 64;

                cmdW = malloc(len * sizeof(WCHAR));

                memcpy(cmdW, (void *)lParam, pos * sizeof(WCHAR));
                wsprintfW(cmdW + pos, formatW, mwi->alias);
                lstrcatW(cmdW, (WCHAR *)lParam + pos);
            }
            else
                cmdW = (LPWSTR)lParam;

            mwi->lasterror = mciSendStringW(cmdW, mwi->return_string,
                                            ARRAY_SIZE(mwi->return_string), 0);
            if (mwi->lasterror)
                MCIWND_notify_error(mwi);

            if (cmdW != (LPWSTR)lParam)
                free(cmdW);

            if (wMsg == MCIWNDM_SENDSTRINGA)
                free((void *)lParam);

            MCIWND_UpdateState(mwi);
            return mwi->lasterror;
        }

    case MCIWNDM_RETURNSTRINGA:
        WideCharToMultiByte(CP_ACP, 0, mwi->return_string, -1, (LPSTR)lParam, wParam, NULL, NULL);
        TRACE("MCIWNDM_RETURNTRINGA %s\n", debugstr_an((LPSTR)lParam, wParam));
        return mwi->lasterror;

    case MCIWNDM_RETURNSTRINGW:
        lstrcpynW((LPWSTR)lParam, mwi->return_string, wParam);
        TRACE("MCIWNDM_RETURNTRINGW %s\n", debugstr_wn((LPWSTR)lParam, wParam));
        return mwi->lasterror;

    case MCIWNDM_SETTIMERS:
        TRACE("MCIWNDM_SETTIMERS active %d ms, inactive %d ms\n", (int)wParam, (int)lParam);
        mwi->active_timer = (WORD)wParam;
        mwi->inactive_timer = (WORD)lParam;
        return 0;

    case MCIWNDM_SETACTIVETIMER:
        TRACE("MCIWNDM_SETACTIVETIMER %d ms\n", (int)wParam);
        mwi->active_timer = (WORD)wParam;
        return 0;

    case MCIWNDM_SETINACTIVETIMER:
        TRACE("MCIWNDM_SETINACTIVETIMER %d ms\n", (int)wParam);
        mwi->inactive_timer = (WORD)wParam;
        return 0;

    case MCIWNDM_GETACTIVETIMER:
        TRACE("MCIWNDM_GETACTIVETIMER: %d ms\n", mwi->active_timer);
        return mwi->active_timer;

    case MCIWNDM_GETINACTIVETIMER:
        TRACE("MCIWNDM_GETINACTIVETIMER: %d ms\n", mwi->inactive_timer);
        return mwi->inactive_timer;

    case MCIWNDM_CHANGESTYLES:
        TRACE("MCIWNDM_CHANGESTYLES mask %08Ix, set %08Ix\n", wParam, lParam);
        /* FIXME: update the visual window state as well:
         * add/remove trackbar, autosize, etc.
         */
        mwi->dwStyle &= ~wParam;
        mwi->dwStyle |= lParam & wParam;
        return 0;

    case MCIWNDM_GETSTYLES:
        TRACE("MCIWNDM_GETSTYLES: %08lx\n", mwi->dwStyle & 0xffff);
        return mwi->dwStyle & 0xffff;

    case MCIWNDM_GETDEVICEA:
        {
            int len = 0;
            char *str = (char *)lParam;
            MCI_SYSINFO_PARMSA mci_sysinfo;

            mci_sysinfo.lpstrReturn = str;
            mci_sysinfo.dwRetSize = wParam;
            mwi->lasterror = mciSendCommandA(mwi->mci, MCI_SYSINFO,
                                             MCI_SYSINFO_INSTALLNAME,
                                             (DWORD_PTR)&mci_sysinfo);
            while(len < wParam && str[len]) len++;
            TRACE("MCIWNDM_GETDEVICEA: %s\n", debugstr_an(str, len));
            return 0;
        }

    case MCIWNDM_GETDEVICEW:
        {
            int len = 0;
            WCHAR *str = (WCHAR *)lParam;
            MCI_SYSINFO_PARMSW mci_sysinfo;

            mci_sysinfo.lpstrReturn = str;
            mci_sysinfo.dwRetSize = wParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_SYSINFO,
                                             MCI_SYSINFO_INSTALLNAME,
                                             (DWORD_PTR)&mci_sysinfo);
            while(len < wParam && str[len]) len++;
            TRACE("MCIWNDM_GETDEVICEW: %s\n", debugstr_wn(str, len));
            return 0;
        }

    case MCIWNDM_VALIDATEMEDIA:
        TRACE("MCIWNDM_VALIDATEMEDIA\n");
        if (mwi->mci)
        {
            SendMessageW(hWnd, MCIWNDM_GETSTART, 0, 0);
            SendMessageW(hWnd, MCIWNDM_GETLENGTH, 0, 0);
        }
        return 0;

    case MCIWNDM_GETFILENAMEA:
        TRACE("MCIWNDM_GETFILENAMEA: %s\n", debugstr_w(mwi->lpName));
        if (mwi->lpName)
            WideCharToMultiByte(CP_ACP, 0, mwi->lpName, -1, (LPSTR)lParam, wParam, NULL, NULL);
        return 0;

    case MCIWNDM_GETFILENAMEW:
        TRACE("MCIWNDM_GETFILENAMEW: %s\n", debugstr_w(mwi->lpName));
        if (mwi->lpName)
            lstrcpynW((LPWSTR)lParam, mwi->lpName, wParam);
        return 0;

    case MCIWNDM_GETTIMEFORMATA:
    case MCIWNDM_GETTIMEFORMATW:
        {
            MCI_STATUS_PARMS mci_status;

            TRACE("MCIWNDM_GETTIMEFORMAT %08Ix %08Ix\n", wParam, lParam);

            /* get format string if requested */
            if (wParam && lParam)
            {
                if (wMsg == MCIWNDM_GETTIMEFORMATA)
                {
                    char cmd[64];

                    wsprintfA(cmd, "status %d time format", mwi->alias);
                    mwi->lasterror = mciSendStringA(cmd, (LPSTR)lParam, wParam, 0);
                    if (mwi->lasterror)
                        return 0;
                }
                else
                {
                    WCHAR cmdW[64];
                    static const WCHAR formatW[] = {'s','t','a','t','u','s',' ','%','d',' ','t','i','m','e',' ','f','o','r','m','a','t',0};

                    wsprintfW(cmdW, formatW, mwi->alias);
                    mwi->lasterror = mciSendStringW(cmdW, (LPWSTR)lParam, wParam, 0);
                    if (mwi->lasterror)
                        return 0;
                }
            }

            mci_status.dwItem = MCI_STATUS_TIME_FORMAT ;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_STATUS,
                                             MCI_STATUS_ITEM,
                                             (DWORD_PTR)&mci_status);
            if (mwi->lasterror)
                return 0;

            return mci_status.dwReturn;
        }

    case MCIWNDM_SETTIMEFORMATA:
        {
            UNICODE_STRING stringW;

            TRACE("MCIWNDM_SETTIMEFORMATA %s\n", debugstr_a((LPSTR)lParam));

            RtlCreateUnicodeStringFromAsciiz(&stringW, (LPCSTR)lParam);
            lParam = (LPARAM)stringW.Buffer;
        }
        /* fall through */
    case MCIWNDM_SETTIMEFORMATW:
        {
            static const WCHAR formatW[] = {'s','e','t',' ','%','d',' ','t','i','m','e',' ','f','o','r','m','a','t',' ',0};
            WCHAR *cmdW;

            TRACE("MCIWNDM_SETTIMEFORMATW %s\n", debugstr_w((LPWSTR)lParam));

            if (mwi->mci)
            {
                cmdW = malloc((wcslen((WCHAR *)lParam) + 64) * sizeof(WCHAR));
                wsprintfW(cmdW, formatW, mwi->alias);
                lstrcatW(cmdW, (WCHAR *)lParam);

                mwi->lasterror = mciSendStringW(cmdW, NULL, 0, 0);

                /* fix the range tracking according to the new time format */
                if (!mwi->lasterror)
                    SendDlgItemMessageW(hWnd, CTL_TRACKBAR, TBM_SETRANGEMAX, 1,
                                        SendMessageW(hWnd, MCIWNDM_GETLENGTH, 0, 0));

                free(cmdW);
            }

            if (wMsg == MCIWNDM_SETTIMEFORMATA)
                free((void *)lParam);

            return 0;
        }

    case MCIWNDM_CAN_PLAY:
        TRACE("MCIWNDM_CAN_PLAY\n");
        if (mwi->mci)
            return mci_get_devcaps(mwi, MCI_GETDEVCAPS_CAN_PLAY);
        return 0;

    case MCIWNDM_CAN_RECORD:
        TRACE("MCIWNDM_CAN_RECORD\n");
        if (mwi->mci)
            return mci_get_devcaps(mwi, MCI_GETDEVCAPS_CAN_RECORD);
        return 0;

    case MCIWNDM_CAN_SAVE:
        TRACE("MCIWNDM_CAN_SAVE\n");
        if (mwi->mci)
            return mci_get_devcaps(mwi, MCI_GETDEVCAPS_CAN_SAVE);
        return 0;

    case MCIWNDM_CAN_EJECT:
        TRACE("MCIWNDM_CAN_EJECT\n");
        if (mwi->mci)
            return mci_get_devcaps(mwi, MCI_GETDEVCAPS_CAN_EJECT);
        return 0;

    case MCIWNDM_CAN_WINDOW:
        TRACE("MCIWNDM_CAN_WINDOW\n");
        switch (mwi->dev_type)
        {
        case MCI_DEVTYPE_ANIMATION:
        case MCI_DEVTYPE_DIGITAL_VIDEO:
        case MCI_DEVTYPE_OVERLAY:
            return 1;
        }
        return 0;

    case MCIWNDM_CAN_CONFIG:
        TRACE("MCIWNDM_CAN_CONFIG\n");
        if (mwi->hdrv)
            return SendDriverMessage(mwi->hdrv, DRV_QUERYCONFIGURE, 0, 0);
        return 0;

    case MCIWNDM_SETZOOM:
        TRACE("MCIWNDM_SETZOOM %Id\n", lParam);
        mwi->zoom = lParam;

        if (mwi->mci && !(mwi->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW))
        {
            RECT rc;

            rc.left = rc.top = 0;
            rc.right = MulDiv(mwi->size.cx, mwi->zoom, 100);
            rc.bottom = MulDiv(mwi->size.cy, mwi->zoom, 100);

            if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
                rc.bottom += 32; /* add the height of the playbar */
            AdjustWindowRect(&rc, GetWindowLongW(hWnd, GWL_STYLE), FALSE);
            SetWindowPos(hWnd, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;

    case MCIWNDM_GETZOOM:
        TRACE("MCIWNDM_GETZOOM: %d\n", mwi->zoom);
        return mwi->zoom;

    case MCIWNDM_EJECT:
        {
            MCI_SET_PARMS mci_set;

            TRACE("MCIWNDM_EJECT\n");

            mci_set.dwCallback = (DWORD_PTR)hWnd;
            mwi->lasterror = mciSendCommandW(mwi->mci, MCI_SET,
                                             MCI_SET_DOOR_OPEN | MCI_NOTIFY,
                                             (DWORD_PTR)&mci_set);
            MCIWND_notify_mode(mwi);
            MCIWND_UpdateState(mwi);
            return mwi->lasterror;
        }

    case MCIWNDM_SETVOLUME:
    case MCIWNDM_GETVOLUME:
    case MCIWNDM_SETSPEED:
    case MCIWNDM_GETSPEED:
    case MCIWNDM_SETREPEAT:
    case MCIWNDM_GETREPEAT:
    case MCIWNDM_REALIZE:
    case MCIWNDM_GETPALETTE:
    case MCIWNDM_SETPALETTE:
    case MCIWNDM_NEWA:
    case MCIWNDM_NEWW:
    case MCIWNDM_PALETTEKICK:
    case MCIWNDM_OPENINTERFACE:
        FIXME("support for MCIWNDM_ message WM_USER+%d not implemented\n", wMsg - WM_USER);
        return 0;

    case MCI_PLAY:
        {
            LRESULT end = SendMessageW(hWnd, MCIWNDM_GETEND, 0, 0);
            return SendMessageW(hWnd, MCIWNDM_PLAYTO, 0, end);
        }

    case MCI_SEEK:
    case MCI_STEP:
        {
            MCI_SEEK_PARMS mci_seek; /* Layout is usable as MCI_XYZ_STEP_PARMS */
            DWORD flags = MCI_STEP == wMsg ? 0 :
                          MCIWND_START == lParam ? MCI_SEEK_TO_START :
                          MCIWND_END   == lParam ? MCI_SEEK_TO_END : MCI_TO;

            mci_seek.dwTo = lParam;
            mwi->lasterror = mciSendCommandW(mwi->mci, wMsg,
                                             flags, (DWORD_PTR)&mci_seek);
            if (mwi->lasterror)
            {
                MCIWND_notify_error(mwi);
                return mwi->lasterror;
            }
            /* update window to reflect the state */
            else InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }

    case MCI_CLOSE:
        {
            RECT rc;
            MCI_GENERIC_PARMS mci_generic;

            if (mwi->hdrv)
            {
                CloseDriver(mwi->hdrv, 0, 0);
                mwi->hdrv = 0;
            }

            if (mwi->mci)
            {
                mci_generic.dwCallback = 0;
                mwi->lasterror = mciSendCommandW(mwi->mci, MCI_CLOSE,
                                                 0, (DWORD_PTR)&mci_generic);
                mwi->mci = 0;
            }

            mwi->mode = MCI_MODE_NOT_READY;
            mwi->position = -1;

            free(mwi->lpName);
            mwi->lpName = NULL;
            MCIWND_UpdateState(mwi);

            GetClientRect(hWnd, &rc);
            rc.bottom = rc.top;
            if (!(mwi->dwStyle & MCIWNDF_NOPLAYBAR))
                rc.bottom += 32; /* add the height of the playbar */
            AdjustWindowRect(&rc, GetWindowLongW(hWnd, GWL_STYLE), FALSE);
            SetWindowPos(hWnd, 0, 0, 0, rc.right - rc.left,
                         rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            MCIWND_notify_media(mwi);
            return 0;
        }

    case MCI_PAUSE:
    case MCI_STOP:
    case MCI_RESUME:
        mci_generic_command(mwi, wMsg);
        return mwi->lasterror;

    case MCI_CONFIGURE:
        if (mwi->hdrv)
            SendDriverMessage(mwi->hdrv, DRV_CONFIGURE, (LPARAM)hWnd, 0);
        return 0;

    case MCI_BREAK:
    case MCI_CAPTURE:
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_ESCAPE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
    /*case MCI_INDEX:*/
    case MCI_INFO:
    case MCI_LIST:
    case MCI_LOAD:
    /*case MCI_MARK:*/
    case MCI_MONITOR:
    case MCI_OPEN:
    case MCI_PASTE:
    case MCI_PUT:
    case MCI_QUALITY:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESERVE:
    case MCI_RESTORE:
    case MCI_SAVE:
    case MCI_SET:
    case MCI_SETAUDIO:
    /*case MCI_SETTIMECODE:*/
    /*case MCI_SETTUNER:*/
    case MCI_SETVIDEO:
    case MCI_SIGNAL:
    case MCI_SPIN:
    case MCI_STATUS:
    case MCI_SYSINFO:
    case MCI_UNDO:
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
    case MCI_WINDOW:
        FIXME("support for MCI_ command %04x not implemented\n", wMsg);
        return 0;
    }

    if (wMsg >= WM_USER)
    {
        FIXME("support for MCIWNDM_ message WM_USER+%d not implemented\n", wMsg - WM_USER);
        return 0;
    }

    if (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
        return DefMDIChildProcW(hWnd, wMsg, wParam, lParam);

    return DefWindowProcW(hWnd, wMsg, wParam, lParam);
}
