/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/sound.c
 * PURPOSE:     ReactX diagnosis sound page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>

#if 0
BOOL
GetCatFileFromDriverPath(LPWSTR szFileName, LPWSTR szCatFileName)
{
    GUID VerifyGuid = DRIVER_ACTION_VERIFY;
    HANDLE hFile;
    DWORD dwHash;
    BYTE bHash[100];
    HCATINFO hCatInfo;
    HCATADMIN hActAdmin;
    BOOL bRet = FALSE;
    CATALOG_INFO CatInfo;

    /* attempt to open file */
    hFile = CreateFileW(szFileName, GENERIC_READ,  FILE_SHARE_READ, NULL,  OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

     /* calculate hash from file handle */
     dwHash = sizeof(bHash);
     if (!CryptCATAdminCalcHashFromFileHandle(hFile, &dwHash, bHash, 0))
     {
        CloseHandle(hFile);
        return FALSE;
     }

    /* try open the CAT admin */
    if (!CryptCATAdminAcquireContext(&hActAdmin, &VerifyGuid, 0))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    /* search catalog to find for catalog containing this hash */
    hCatInfo = CryptCATAdminEnumCatalogFromHash(hActAdmin, bHash, dwHash, 0, NULL);
    if (hCatInfo != NULL)
    {
        /* theres a catalog get the filename */
        bRet = CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0);
        if (bRet)
           wcscpy(szCatFileName, CatInfo.wszCatalogFile);
        CryptCATAdminReleaseCatalogContext(hActAdmin, hCatInfo, 0);
    }

    /* perform cleanup */
    CloseHandle(hFile);
    CryptCATAdminReleaseContext(hActAdmin, 0);
    return bRet;
}

BOOL
IsDriverWHQL(LPWSTR szFileName)
{
    WCHAR szCatFile[MAX_PATH];
    HANDLE hCat;
    BOOL bRet = FALSE;

    /* get the driver's cat file */
    if (!GetCatFileFromDriverPath(szFileName, szCatFile))
    {
        /* driver has no cat so it's definitely not WHQL signed */
        return FALSE;
    }

    /* open the CAT file */
    hCat = CryptCATOpen(szCatFile, CRYPTCAT_OPEN_EXISTING, 0, 0, 0);
    if (hCat == INVALID_HANDLE_VALUE)
    {
        /* couldnt open cat */
        return FALSE;
    }

    /* FIXME
     * build certificate chain with CertGetCertificateChain
     * verify certificate chain (WinVerifyTrust)
     * retrieve signer (WTHelperGetProvSignerFromChain)
     */


    /* close CAT file */
    CryptCATClose(hCat);
    return bRet;
}
#endif

static
void
SetDeviceDetails(HWND hwndDlg, LPCGUID classGUID, LPCWSTR lpcstrDescription)
{
    HDEVINFO hInfo;
    DWORD dwIndex = 0;
    SP_DEVINFO_DATA InfoData;
    WCHAR szText[30];
    HWND hDlgCtrls[3];
    WAVEOUTCAPSW waveOut;
    UINT numDev;
    MMRESULT errCode;


    /*  enumerate waveout devices */
    numDev = waveOutGetNumDevs();
    if (numDev)
    {
        do
        {
                ZeroMemory(&waveOut, sizeof(waveOut));
                errCode = waveOutGetDevCapsW(dwIndex++, &waveOut, sizeof(waveOut));
                if (!wcsncmp(lpcstrDescription, waveOut.szPname, min(MAXPNAMELEN, wcslen(waveOut.szPname))))
                {
                    /* set the product id */
                    SetDlgItemInt(hwndDlg, IDC_STATIC_DSOUND_PRODUCTID, waveOut.wPid, FALSE);
                    /* set the vendor id */
                    SetDlgItemInt(hwndDlg, IDC_STATIC_DSOUND_VENDORID, waveOut.wMid, FALSE);
                    /* check if it's a WDM audio driver */
                    if (waveOut.wPid == MM_MSFT_WDMAUDIO_WAVEOUT)
                        SendDlgItemMessageW(hwndDlg, IDC_STATIC_DSOUND_TYPE, WM_SETTEXT, 0, (LPARAM)L"WDM");

                    /* check if device is default device */
                    szText[0] = L'\0';
                    if (dwIndex - 1 == 0) /* FIXME assume default playback device is device 0 */
                        LoadStringW(hInst, IDS_OPTION_YES, szText, sizeof(szText)/sizeof(WCHAR));
                    else
                        LoadStringW(hInst, IDS_OPTION_NO, szText, sizeof(szText)/sizeof(WCHAR));

                    szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
                    /* set default device info */
                    SendDlgItemMessageW(hwndDlg, IDC_STATIC_DSOUND_STANDARD, WM_SETTEXT, 0, (LPARAM)szText);
                    break;
                }
                }while(errCode == MMSYSERR_NOERROR && dwIndex < numDev);
    }

    dwIndex = 0;
    /* create the setup list */
    hInfo = SetupDiGetClassDevsW(classGUID, NULL, NULL, DIGCF_PRESENT|DIGCF_PROFILE);
    if (hInfo == INVALID_HANDLE_VALUE)
        return;

    do
    {
        ZeroMemory(&InfoData, sizeof(InfoData));
        InfoData.cbSize = sizeof(InfoData);

        if (SetupDiEnumDeviceInfo(hInfo, dwIndex, &InfoData))
        {
            /* set device name */
            if (SetupDiGetDeviceInstanceId(hInfo, &InfoData, szText, sizeof(szText)/sizeof(WCHAR), NULL))
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_DSOUND_DEVICEID, WM_SETTEXT, 0, (LPARAM)szText);

            /* set the manufacturer name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_MFG, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_PROVIDER, WM_SETTEXT, 0, (LPARAM)szText);

            /* FIXME
             * we currently enumerate only the first adapter
             */
            hDlgCtrls[0] = GetDlgItem(hwndDlg, IDC_STATIC_DSOUND_DRIVER);
            hDlgCtrls[1] = GetDlgItem(hwndDlg, IDC_STATIC_DSOUND_VERSION);
            hDlgCtrls[2] = GetDlgItem(hwndDlg, IDC_STATIC_DSOUND_DATE);
            EnumerateDrivers(hDlgCtrls, hInfo, &InfoData);
            break;
        }

        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            break;

        dwIndex++;
    }while(TRUE);

    /* destroy the setup list */
    SetupDiDestroyDeviceInfoList(hInfo);
}



BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCWSTR lpcstrDescription, LPCWSTR lpcstrModule, LPVOID lpContext)
{
    PDXDIAG_CONTEXT pContext = (PDXDIAG_CONTEXT)lpContext;
    HWND * hDlgs;
    HWND hwndDlg;
    WCHAR szSound[20];
    WCHAR szText[30];
    IDirectSound8 *pObj;
    HRESULT hResult;
    DWORD dwCertified;

    if (!lpGuid)
        return TRUE;

    if (pContext->NumSoundAdapter)
        hDlgs = HeapReAlloc(GetProcessHeap(), 0, pContext->hSoundWnd, (pContext->NumSoundAdapter + 1) * sizeof(HWND));
    else
        hDlgs = HeapAlloc(GetProcessHeap(), 0, (pContext->NumSoundAdapter + 1) * sizeof(HWND));

    if (!hDlgs)
        return FALSE;

    pContext->hSoundWnd = hDlgs;
    hwndDlg = CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_SOUND_DIALOG), pContext->hMainDialog, SoundPageWndProc, (LPARAM)pContext); EnableDialogTheme(hwndDlg);
    if (!hwndDlg)
        return FALSE;

    hResult = DirectSoundCreate8(lpGuid, (LPDIRECTSOUND8*)&pObj, NULL);
    if (hResult == DS_OK)
    {
        szText[0] = L'\0';
        if (IDirectSound8_VerifyCertification(pObj, &dwCertified) == DS_OK)
        {
            if (dwCertified == DS_CERTIFIED)
                LoadStringW(hInst, IDS_OPTION_YES, szText, sizeof(szText)/sizeof(WCHAR));
            else if (dwCertified == DS_UNCERTIFIED)
                LoadStringW(hInst, IDS_NOT_APPLICABLE, szText, sizeof(szText)/sizeof(WCHAR));
        }
        else
        {
            LoadStringW(hInst, IDS_OPTION_NO, szText, sizeof(szText)/sizeof(WCHAR));
        }
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_DSOUND_LOGO, WM_SETTEXT, 0, (LPARAM)szText);
        IDirectSound8_Release(pObj);
    }

    /* set device name */
    SendDlgItemMessageW(hwndDlg, IDC_STATIC_DSOUND_NAME, WM_SETTEXT, 0, (LPARAM)lpcstrDescription);

    /* set range for slider */
    SendDlgItemMessageW(hwndDlg, IDC_SLIDER_DSOUND, TBM_SETRANGE, TRUE, MAKELONG(0, 3));

    /* FIXME set correct position */
    SendDlgItemMessageW(hwndDlg, IDC_SLIDER_DSOUND, TBM_SETSEL, FALSE, 0);

    /* set further device details */
    SetDeviceDetails(hwndDlg, &GUID_DEVCLASS_MEDIA, lpcstrDescription);

    /* load sound resource string */
    szSound[0] = L'\0';
    LoadStringW(hInst, IDS_SOUND_DIALOG, szSound, sizeof(szSound)/sizeof(WCHAR));
    szSound[(sizeof(szSound)/sizeof(WCHAR))-1] = L'\0';
    /* output the device id */
    wsprintfW (szText, L"%s %u", szSound, pContext->NumSoundAdapter + 1);
    /* insert it into general tab */
    InsertTabCtrlItem(pContext->hTabCtrl, pContext->NumDisplayAdapter + pContext->NumSoundAdapter + 1, szText);
    /* store dialog window */
    hDlgs[pContext->NumSoundAdapter] = hwndDlg;
    pContext->NumSoundAdapter++;
    return TRUE;
}


void InitializeDirectSoundPage(PDXDIAG_CONTEXT pContext)
{
    HRESULT hResult;


    /* create DSound object */

//    if (hResult != DS_OK)
//        return;
    hResult = DirectSoundEnumerateW(DSEnumCallback, pContext);

    /* release the DSound object */
//    pObj->lpVtbl->Release(pObj);
    (void)hResult;
}


INT_PTR CALLBACK
SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            return TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_BUTTON_TESTDSOUND)
            {
                return FALSE;
            }
            break;
        }
    }

    return FALSE;
}
