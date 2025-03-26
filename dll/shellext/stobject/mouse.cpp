/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Mouse keys notification icon handler
 * COPYRIGHT:   Copyright 2022 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

static MOUSEKEYS g_Mk;
static UINT g_MkState;
static HICON g_MkStateIcon;

HRESULT STDMETHODCALLTYPE
MouseKeys_Init(_In_ CSysTray *pSysTray)
{
    TRACE("MouseKeys_Init!\n");

    return MouseKeys_Update(pSysTray);
}

HRESULT STDMETHODCALLTYPE
MouseKeys_Shutdown(_In_ CSysTray *pSysTray)
{
    TRACE("MouseKeys_Shutdown!\n");

    if (g_MkStateIcon)
    {
        DestroyIcon(g_MkStateIcon);
        g_MkStateIcon = NULL;
    }

    if (g_MkState)
    {
        g_MkState = 0;
        pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_MOUSE, g_MkStateIcon, L"MouseKeys");
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
MouseKeys_Update(_In_ CSysTray *pSysTray)
{
    TRACE("MouseKeys_Update!\n");

    g_Mk.cbSize = sizeof(g_Mk);
    SystemParametersInfoW(SPI_GETMOUSEKEYS, sizeof(g_Mk), &g_Mk, 0);

    UINT state = 0;
    if ((g_Mk.dwFlags & (MKF_INDICATOR | MKF_MOUSEKEYSON)) == (MKF_INDICATOR | MKF_MOUSEKEYSON))
    {
        if (g_Mk.dwFlags & MKF_MOUSEMODE)
        {
            switch (g_Mk.dwFlags & (MKF_LEFTBUTTONDOWN | MKF_LEFTBUTTONSEL | MKF_RIGHTBUTTONDOWN | MKF_RIGHTBUTTONSEL))
            {
                case 0:
                default:
                    state = IDI_MOUSE_NOBTN;
                    break;
                case MKF_LEFTBUTTONSEL:
                    state = IDI_MOUSE_L_ACTIVE;
                    break;
                case MKF_LEFTBUTTONDOWN:
                case MKF_LEFTBUTTONDOWN | MKF_LEFTBUTTONSEL:
                    state = IDI_MOUSE_L_DOWN;
                    break;
                case MKF_RIGHTBUTTONSEL:
                    state = IDI_MOUSE_R_ACTIVE;
                    break;
                case MKF_RIGHTBUTTONDOWN:
                case MKF_RIGHTBUTTONDOWN | MKF_RIGHTBUTTONSEL:
                    state = IDI_MOUSE_R_DOWN;
                    break;
                case MKF_LEFTBUTTONSEL | MKF_RIGHTBUTTONSEL:
                    state = IDI_MOUSE_LR_ACTIVE;
                    break;
                case MKF_RIGHTBUTTONDOWN | MKF_LEFTBUTTONDOWN:
                case MKF_RIGHTBUTTONDOWN | MKF_LEFTBUTTONDOWN | MKF_LEFTBUTTONSEL:
                case MKF_RIGHTBUTTONDOWN | MKF_LEFTBUTTONDOWN | MKF_LEFTBUTTONSEL | MKF_RIGHTBUTTONSEL:
                case MKF_RIGHTBUTTONDOWN | MKF_LEFTBUTTONDOWN | MKF_RIGHTBUTTONSEL:
                    state = IDI_MOUSE_LR_DOWN;
                    break;
                case MKF_LEFTBUTTONSEL | MKF_RIGHTBUTTONDOWN:
                case MKF_LEFTBUTTONSEL | MKF_RIGHTBUTTONDOWN | MKF_RIGHTBUTTONSEL:
                    state = IDI_MOUSE_L_ACTIVE_R_DOWN;
                    break;
                case MKF_LEFTBUTTONDOWN | MKF_RIGHTBUTTONSEL:
                case MKF_LEFTBUTTONDOWN | MKF_RIGHTBUTTONSEL | MKF_LEFTBUTTONSEL:
                    state = IDI_MOUSE_R_ACTIVE_L_DOWN;
                    break;
            }
        }
        else
        {
            state = IDI_MOUSE_DISABLED;
        }
    }

    UINT uId = NIM_MODIFY;
    if (state != g_MkState)
    {
        if (g_MkStateIcon)
        {
            DestroyIcon(g_MkStateIcon);
            g_MkStateIcon = NULL;
        }

        if (g_MkState == 0)
            uId = NIM_ADD;

        g_MkState = state;
        if (g_MkState)
        {
            g_MkStateIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(g_MkState));
        }
    }

    if (g_MkState == 0)
    {
        uId = NIM_DELETE;
    }

    return pSysTray->NotifyIcon(uId, ID_ICON_MOUSE, g_MkStateIcon, L"MouseKeys");
}
