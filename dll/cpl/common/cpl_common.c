/**
 * @file cpl_common.c
 * @brief Auxiliary functions for the realization
 *        of a one-instance cpl applet
 *
 * Copyright 2022 Raymond Czerny
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
 */

#include "cpl_common.h"

/** Structure for in and out data when
 *  search for the cpl dialog of first instance
 */
typedef struct tagUserData
{
    const WCHAR*    Title;
    HWND            Result;
} UserData;

static BOOL CALLBACK enumWinProc(
  _In_ HWND   hwnd,
  _In_ LPARAM lParam
)
{
    WCHAR szClassName[51];
    if(GetClassNameW(hwnd, szClassName, 50))
    {
        if(0 == wcscmp(L"#32770", szClassName))
        {
            WCHAR szCaption[255];
            ZeroMemory(szCaption, sizeof(szCaption));
            if(GetWindowTextW(hwnd, szCaption, sizeof(szCaption)))
            {
                UserData* pData = (UserData*)lParam;
                if(NULL != wcsstr(szCaption, pData->Title))
                {
                    pData->Result = hwnd;
                }
            }
        }
    }
    return TRUE;
}

HWND CPL_GetHWndByCaption(const WCHAR* Caption)
{
    UserData data = {Caption, NULL};
    EnumWindows(enumWinProc, (LPARAM)&data);
    return data.Result;
}

HWND CPL_GetHWndByResource(HINSTANCE hInstance, UINT uID)
{
    WCHAR szCaption[255];
    ZeroMemory(szCaption, sizeof(szCaption));
    LoadStringW(hInstance, uID, szCaption, 255);
    return CPL_GetHWndByCaption(szCaption);
}
