/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell32 private helpers
 * COPYRIGHT:   Copyright 2022 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


// Share this with CDefaultContextMenu!!
DWORD
Clipboard_GetDropEffect(IShellFolder *pShellFolder)
{
    /* If the folder doesn't have a drop target we can't paste */
    CComPtr<IDropTarget> pdt;
    HRESULT hr = pShellFolder->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    if (FAILED(hr))
        return FALSE;

    CComPtr<IDataObject> pDataObj;
    hr = OleGetClipboard(&pDataObj);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    STGMEDIUM medium;
    FORMATETC formatetc;

    /* We can only paste if CFSTR_SHELLIDLIST is present in the clipboard */
    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    hr = pDataObj->GetData(&formatetc, &medium);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    ReleaseStgMedium(&medium);

    /* Check what we were asked to do */
    DWORD dwDropEffect;
    if (FAILED_UNEXPECTEDLY(DataObject_GetPreferredDropEffect(pDataObj, &dwDropEffect)))
    {
        dwDropEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;
    }
    /* Let's see what the drop target is allowing us to do */
    if (!FAILED_UNEXPECTEDLY(pdt->DragEnter(pDataObj, MK_RBUTTON, {0}, &dwDropEffect)))
    {
        FAILED_UNEXPECTEDLY(pdt->DragLeave());
    }
    else
    {
        dwDropEffect = DROPEFFECT_NONE;
    }

    return dwDropEffect;
}

// Adapted from https://devblogs.microsoft.com/oldnewthing/20041007-00/?p=37633
HRESULT
IContextMenu_HandleMenuMsg2(IContextMenu *pcm, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    CComPtr<IContextMenu2> pcm2;
    CComPtr<IContextMenu3> pcm3;
    HRESULT hr;
    if (SUCCEEDED(hr = pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &pcm3))))
    {
        hr = pcm3->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
    }
    else if (SUCCEEDED(hr = pcm->QueryInterface(IID_PPV_ARG(IContextMenu2, &pcm2))))
    {
        if (plResult)
            *plResult = 0;
        hr = pcm2->HandleMenuMsg(uMsg, wParam, lParam);
    }
    return hr;
}
