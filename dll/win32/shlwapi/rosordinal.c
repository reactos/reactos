/*
 * PROJECT:   ReactOS Shell
 * LICENSE:   GPL - See COPYING in the top level directory
 * PURPOSE:   Misc shell helper functions
 * COPYRIGHT: Copyright 2015 Giannis Adamopoulos
 */

#include "precomp.h"

/* http://undoc.airesoft.co.uk/shlwapi.dll/SHForwardContextMenuMsg.php */
HRESULT WINAPI SHForwardContextMenuMsg(IUnknown* pUnk, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam, LRESULT* pResult, BOOL useIContextMenu2)
{
    HRESULT hr;
    IContextMenu3* pcmenu3;
    IContextMenu2* pcmenu2;

    /* First try to use the IContextMenu3 interface */
    hr = IUnknown_QueryInterface(pUnk, &IID_IContextMenu3, (void**)&pcmenu3);
    if (SUCCEEDED(hr))
    {
        hr = IContextMenu3_HandleMenuMsg2(pcmenu3, uMsg, wParam, lParam, pResult);
        IContextMenu3_Release(pcmenu3);
        return hr;
    }

    /* Return failure if we can't use the IContextMenu2 interface */
    if(!useIContextMenu2)
        return hr;

    /* Now try to use the IContextMenu2 interface */
    hr = IUnknown_QueryInterface(pUnk, &IID_IContextMenu2, (void**)&pcmenu2);
    if (FAILED(hr))
        return hr;

    hr = IContextMenu2_HandleMenuMsg(pcmenu2, uMsg, wParam, lParam);
    IContextMenu2_Release(pcmenu2);
    return hr;
}

/* http://undoc.airesoft.co.uk/shlwapi.dll/SHAreIconsEqual.php */

BOOL WINAPI SHAreIconsEqual(HICON hIcon1, HICON hIcon2)
{
    ICONINFO iconInfo1, iconInfo2;
    BITMAP bm1, bm2;
    BOOL bSame = FALSE;

    if (!hIcon1 || !hIcon2)
        return FALSE;

    if (!GetIconInfo(hIcon1, &iconInfo1))
        return FALSE;

    if (!GetIconInfo(hIcon2, &iconInfo2))
    {
        DeleteObject(iconInfo1.hbmColor);
        DeleteObject(iconInfo1.hbmMask);
        return FALSE;
    }

    GetObjectW(iconInfo1.hbmColor, sizeof(bm1), &bm1);
    GetObjectW(iconInfo2.hbmColor, sizeof(bm2), &bm2);

    if (bm1.bmWidth == bm2.bmWidth && bm1.bmHeight == bm2.bmHeight)
    {
        BITMAPINFO bmi = { { sizeof(bmi), bm1.bmWidth, bm1.bmHeight, 1, 32 } };
        HDC hdc = GetDC(0);
        SIZE_T size = bm1.bmWidth * bm1.bmHeight * 4;
        BYTE *data1, *data2;

        data1 = HeapAlloc(GetProcessHeap(), 0, size);
        data2 = HeapAlloc(GetProcessHeap(), 0, size);

        if (data1 && data2)
        {
            if (GetDIBits(hdc, iconInfo1.hbmColor, 0, bm1.bmHeight, data1, &bmi, DIB_RGB_COLORS) &&
                GetDIBits(hdc, iconInfo2.hbmColor, 0, bm2.bmHeight, data2, &bmi, DIB_RGB_COLORS))
            {
                bSame = memcmp(data1, data2, size) == 0;
            }
        }
        HeapFree(GetProcessHeap(), 0, data1);
        HeapFree(GetProcessHeap(), 0, data2);

        ReleaseDC(NULL, hdc);
    }

    DeleteObject(iconInfo1.hbmColor);
    DeleteObject(iconInfo1.hbmMask);

    DeleteObject(iconInfo2.hbmColor);
    DeleteObject(iconInfo2.hbmMask);

    return bSame;
}
