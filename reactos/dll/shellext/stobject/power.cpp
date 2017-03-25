/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/power.cpp
 * PURPOSE:     Power notification icon handler
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 *              David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"
#include "powrprof.h"

#include <mmsystem.h>
#include <mmddk.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

typedef struct _PWRSCHEMECONTEXT
{
    HMENU hPopup;
    UINT uiFirst;
    UINT uiLast;
} PWRSCHEMECONTEXT, *PPWRSCHEMECONTEXT;

//static HICON g_hIconBattery = NULL;
static HICON g_hIconAC = NULL;

static BOOL g_IsRunning = FALSE;


HRESULT STDMETHODCALLTYPE Power_Init(_In_ CSysTray * pSysTray)
{
    WCHAR strTooltip[128];

    TRACE("Power_Init\n");

//    g_hIconBattery = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_BATTERY));
    g_hIconAC = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_BATTERY));


    HICON icon;
//    if (g_IsMute)
//        icon = g_hIconBattery;
//    else
        icon = g_hIconAC;

    LoadStringW(g_hInstance, IDS_PWR_AC, strTooltip, _countof(strTooltip));

    g_IsRunning = TRUE;

    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_POWER, icon, strTooltip);
}

HRESULT STDMETHODCALLTYPE Power_Update(_In_ CSysTray * pSysTray)
{
//    BOOL PrevState;

    TRACE("Power_Update\n");

#if 0
    PrevState = g_IsMute;
    Volume_IsMute();

    if (PrevState != g_IsMute)
    {
        WCHAR strTooltip[128];
        HICON icon;
        if (g_IsMute) {
            icon = g_hIconMute;
            LoadStringW(g_hInstance, IDS_VOL_MUTED, strTooltip, _countof(strTooltip));
        }
        else {
            icon = g_hIconVolume;
            LoadStringW(g_hInstance, IDS_VOL_VOLUME, strTooltip, _countof(strTooltip));
        }

        return pSysTray->NotifyIcon(NIM_MODIFY, ID_ICON_POWER, icon, strTooltip);
    }
    else
    {
        return S_OK;
    }
#endif
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Power_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Power_Shutdown\n");

    g_IsRunning = FALSE;

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_POWER, NULL, NULL);
}

static void _RunPower()
{
    ShellExecuteW(NULL, NULL, L"powercfg.cpl", NULL, NULL, SW_SHOWNORMAL);
}

static void _ShowContextMenu(CSysTray * pSysTray)
{
    WCHAR strOpen[128];

    LoadStringW(g_hInstance, IDS_PWR_PROPERTIES, strOpen, _countof(strOpen));

    HMENU hPopup = CreatePopupMenu();
    AppendMenuW(hPopup, MF_STRING, IDS_PWR_PROPERTIES, strOpen);

    DWORD flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN;
    DWORD msgPos = GetMessagePos();

    SetForegroundWindow(pSysTray->GetHWnd());
    DWORD id = TrackPopupMenuEx(hPopup, flags,
        GET_X_LPARAM(msgPos), GET_Y_LPARAM(msgPos),
        pSysTray->GetHWnd(), NULL);

    DestroyMenu(hPopup);

    switch (id)
    {
        case IDS_PWR_PROPERTIES:
            _RunPower();
            break;
    }
}

static
BOOLEAN
CALLBACK
PowerSchemesEnumProc(
    UINT uiIndex,
    DWORD dwName,
    LPWSTR sName,
    DWORD dwDesc,
    LPWSTR sDesc,
    PPOWER_POLICY pp,
    LPARAM lParam)
{
    PPWRSCHEMECONTEXT PowerSchemeContext = (PPWRSCHEMECONTEXT)lParam;

    if (AppendMenuW(PowerSchemeContext->hPopup, MF_STRING, uiIndex + 1, sName))
    {
        if (PowerSchemeContext->uiFirst == 0)
            PowerSchemeContext->uiFirst = uiIndex + 1;

        PowerSchemeContext->uiLast = uiIndex + 1;
    }

    return TRUE;
}

static
VOID
ShowPowerSchemesPopupMenu(
    CSysTray *pSysTray)
{
    PWRSCHEMECONTEXT PowerSchemeContext = {NULL, 0, 0};
    UINT uiActiveScheme;
    DWORD id, msgPos;

    PowerSchemeContext.hPopup = CreatePopupMenu();
    EnumPwrSchemes(PowerSchemesEnumProc, (LPARAM)&PowerSchemeContext);

    if (GetActivePwrScheme(&uiActiveScheme))
    {
        CheckMenuRadioItem(PowerSchemeContext.hPopup,
                           PowerSchemeContext.uiFirst,
                           PowerSchemeContext.uiLast,
                           uiActiveScheme + 1,
                           MF_BYCOMMAND);
    }

    msgPos = GetMessagePos();

    SetForegroundWindow(pSysTray->GetHWnd());
    id = TrackPopupMenuEx(PowerSchemeContext.hPopup,
                          TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                          GET_X_LPARAM(msgPos),
                          GET_Y_LPARAM(msgPos),
                          pSysTray->GetHWnd(),
                          NULL);

    DestroyMenu(PowerSchemeContext.hPopup);

    if (id != 0)
        SetActivePwrScheme(id - 1, NULL, NULL);
}

HRESULT STDMETHODCALLTYPE Power_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    TRACE("Power_Message uMsg=%d, wParam=%x, lParam=%x\n", uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Power_Message: WM_USER+220\n");
            if (wParam == 1)
            {
                if (lParam == FALSE)
                    return Power_Init(pSysTray);
                else
                    return Power_Shutdown(pSysTray);
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Power_Message: WM_USER+221\n");
            if (wParam == 1)
            {
                lResult = (LRESULT)g_IsRunning;
                return S_OK;
            }
            return S_FALSE;

        case ID_ICON_POWER:
            Power_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    break;

                case WM_LBUTTONUP:
                    ShowPowerSchemesPopupMenu(pSysTray);
                    break;

                case WM_LBUTTONDBLCLK:
                    _RunPower();
                    break;

                case WM_RBUTTONDOWN:
                    break;

                case WM_RBUTTONUP:
                    _ShowContextMenu(pSysTray);
                    break;

                case WM_RBUTTONDBLCLK:
                    break;

                case WM_MOUSEMOVE:
                    break;
            }
            return S_OK;

        default:
            TRACE("Power_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}
