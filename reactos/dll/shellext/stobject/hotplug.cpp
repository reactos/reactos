/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/hotplug.cpp
 * PURPOSE:     Removable devices notification icon handler
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com> 
 */

#include "precomp.h"
#include <mmsystem.h>
#include <mmddk.h>
#include <atlstr.h>

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

static HICON g_hIconHotplug = NULL;
static LPWSTR g_strTooltip = L"Safely Remove Hardware and Eject Media";
static BOOL g_IsRunning = FALSE;

HRESULT STDMETHODCALLTYPE Hotplug_Init(_In_ CSysTray * pSysTray)
{ 
    TRACE("Hotplug_Init\n");
    g_hIconHotplug = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EXTRACT));
    g_IsRunning = TRUE;
    
    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_HOTPLUG, g_hIconHotplug, g_strTooltip);
}

HRESULT STDMETHODCALLTYPE Hotplug_Update(_In_ CSysTray * pSysTray)
{
    TRACE("Hotplug_Update\n");    
    //g_hIconHotplug = DynamicLoadIcon(g_hInstance);    

    return pSysTray->NotifyIcon(NIM_MODIFY, ID_ICON_HOTPLUG, g_hIconHotplug, g_strTooltip);
}

HRESULT STDMETHODCALLTYPE Hotplug_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Hotplug_Shutdown\n");
    g_IsRunning = FALSE;

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_HOTPLUG, NULL, NULL);
}

static void _RunHotplug()
{
    ShellExecuteW(NULL, NULL, L"hotplug.cpl", NULL, NULL, SW_SHOWNORMAL);
}

static void _ShowContextMenu(CSysTray * pSysTray)
{
    CString strOpen((LPCSTR)IDS_HOTPLUG_REMOVE_2);
    HMENU hPopup = CreatePopupMenu();      
    AppendMenuW(hPopup, MF_STRING, IDS_HOTPLUG_REMOVE_2, strOpen);    
    
    SetForegroundWindow(pSysTray->GetHWnd());
    DWORD flags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN;
    POINT pt;
    GetCursorPos(&pt);
    
    DWORD id = TrackPopupMenuEx(hPopup, flags,
        pt.x, pt.y,
        pSysTray->GetHWnd(), NULL);  

    switch (id)
    {
        case IDS_HOTPLUG_REMOVE_2:
            _RunHotplug();
            break;
    }
    DestroyMenu(hPopup);
}

HRESULT STDMETHODCALLTYPE Hotplug_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    TRACE("Hotplug_Message uMsg=%d, wParam=%x, lParam=%x\n", uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Hotplug_Message: WM_USER+220\n");
            if (wParam == 1)
            {
                if (lParam == FALSE)
                    return Hotplug_Init(pSysTray);
                else
                    return Hotplug_Shutdown(pSysTray);
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Hotplug_Message: WM_USER+221\n");
            if (wParam == 1)
            {
                lResult = (LRESULT)g_IsRunning;
                return S_OK;
            }
            return S_FALSE;

        case ID_ICON_HOTPLUG:
            Hotplug_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    break;

                case WM_LBUTTONUP:
                MessageBox(0, L"Safely Remove Hardware", L"Test", MB_OKCANCEL | MB_ICONINFORMATION);                    
                    break;

                case WM_LBUTTONDBLCLK:
                    _RunHotplug();
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
            TRACE("Hotplug_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}
