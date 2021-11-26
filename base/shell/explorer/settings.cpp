/*
 * ReactOS Explorer
 *
 * Copyright 2013 - Edijs Kolesnikovics
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

TaskbarSettings g_TaskbarSettings;

BOOL TaskbarSettings::Save()
{
    SHSetValueW(hkExplorer, NULL, L"EnableAutotray", REG_DWORD, &bHideInactiveIcons, sizeof(bHideInactiveIcons));
    SHSetValueW(hkExplorer, L"Advanced", L"ShowSeconds", REG_DWORD, &bShowSeconds, sizeof(bShowSeconds));
    SHSetValueW(hkExplorer, L"Advanced", L"TaskbarGlomming", REG_DWORD, &bGroupButtons, sizeof(bGroupButtons));
    BOOL bAllowSizeMove = !bLock;
    SHSetValueW(hkExplorer, L"Advanced", L"TaskbarSizeMove", REG_DWORD, &bAllowSizeMove, sizeof(bAllowSizeMove));
    sr.cbSize = sizeof(sr);
    SHSetValueW(hkExplorer, L"StuckRects2", L"Settings", REG_BINARY, &sr, sizeof(sr));

    /* TODO: AutoHide writes something to HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Desktop\Components\0 figure out what and why */
    return TRUE;
}

BOOL TaskbarSettings::Load()
{
    DWORD dwRet, cbSize, dwValue = NULL;

    cbSize = sizeof(dwValue);
    dwRet = SHGetValueW(hkExplorer, L"Advanced", L"TaskbarSizeMove", NULL, &dwValue, &cbSize);
    bLock = (dwRet == ERROR_SUCCESS) ? (dwValue == 0) : TRUE;

    dwRet = SHGetValueW(hkExplorer, L"Advanced", L"ShowSeconds", NULL, &dwValue, &cbSize);
    bShowSeconds = (dwRet == ERROR_SUCCESS) ? (dwValue != 0) : FALSE;

    dwRet = SHGetValueW(hkExplorer, L"Advanced", L"TaskbarGlomming", NULL, &dwValue, &cbSize);
    bGroupButtons = (dwRet == ERROR_SUCCESS) ? (dwValue != 0) : FALSE;

    dwRet = SHGetValueW(hkExplorer, NULL, L"EnableAutotray", NULL, &dwValue, &cbSize);
    bHideInactiveIcons = (dwRet == ERROR_SUCCESS) ? (dwValue != 0) : FALSE;

    cbSize = sizeof(sr);
    dwRet = SHGetValueW(hkExplorer, L"StuckRects2", L"Settings", NULL, &sr, &cbSize);

    /* Make sure we have correct values here */
    if (dwRet != ERROR_SUCCESS || sr.cbSize != sizeof(sr) || cbSize != sizeof(sr))
    {
        sr.Position = ABE_BOTTOM;
        sr.AutoHide = FALSE;
        sr.AlwaysOnTop = TRUE;
        sr.SmallIcons = TRUE;
        sr.HideClock = FALSE;
        sr.Rect.left = sr.Rect.top = 0;
        sr.Rect.bottom = sr.Rect.right = 1;
        sr.Size.cx = sr.Size.cy = 0;
    }
    else
    {
        if (sr.Position > ABE_BOTTOM)
            sr.Position = ABE_BOTTOM;
    }

    return TRUE;
}

/* EOF */
