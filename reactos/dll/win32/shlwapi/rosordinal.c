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
