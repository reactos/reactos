/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/sound.c
 * PURPOSE:     ReactX diagnosis sound page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCWSTR lpcstrDescription, LPCWSTR lpcstrModule, LPVOID lpContext)
{
    PDXDIAG_CONTEXT pContext = (PDXDIAG_CONTEXT)lpContext;
    HWND * hDlgs;
    HWND hwndDlg;
    WCHAR szSound[20];
    WCHAR szText[30];

    if (!lpGuid)
        return TRUE;

    if (pContext->NumSoundAdapter)
        hDlgs = HeapReAlloc(GetProcessHeap(), 0, pContext->hSoundWnd, (pContext->NumSoundAdapter + 1) * sizeof(HWND));
    else
        hDlgs = HeapAlloc(GetProcessHeap(), 0, (pContext->NumSoundAdapter + 1) * sizeof(HWND));

    if (!hDlgs)
        return FALSE;

    pContext->hSoundWnd = hDlgs;
	hwndDlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_SOUND_DIALOG), pContext->hTabCtrl, SoundPageWndProc, (LPARAM)pContext);
    if (!hwndDlg)
        return FALSE;

    szSound[0] = L'\0';
    LoadStringW(hInst, IDS_SOUND_DIALOG, szSound, sizeof(szSound)/sizeof(WCHAR));
    szSound[(sizeof(szSound)/sizeof(WCHAR))-1] = L'\0';

    swprintf (szText, L"%s %u", szSound, pContext->NumSoundAdapter + 1);


    InsertTabCtrlItem(pContext->hTabCtrl, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 1, szText);

    hDlgs[pContext->NumSoundAdapter] = hwndDlg;
    pContext->NumSoundAdapter++;

    return TRUE;
}


void InitializeDirectSoundPage(PDXDIAG_CONTEXT pContext)
{
    HRESULT hResult;
    //IDirectSound8 *pObj;

    /* create DSound object */
//    hResult = DirectSoundCreate8(NULL, (LPDIRECTSOUND8*)&pObj, NULL);
//    if (hResult != DS_OK)
//        return;
    hResult = DirectSoundEnumerateW(DSEnumCallback, pContext);

    /* release the DSound object */
//    pObj->lpVtbl->Release(pObj);
}


INT_PTR CALLBACK
SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message) {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            //InitializeDirectSoundPage(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}
