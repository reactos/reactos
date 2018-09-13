#include "precomp.h"
#include "setcdcl.hxx"
#include "device.hxx"

/*****************************************************************\
*
* GetVerInfo
*
\*****************************************************************/

LPTSTR GetVerInfo(LPBYTE lpbVerInfo, LPTSTR pszKey) {

    TCHAR szVersionKey[MAX_PATH];
    LPTSTR pVerInfo;
    UINT cb = 0;

    //
    // Try to get info in the local language
    //

    if (lpbVerInfo)
    {
        wsprintf(szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\%ws"),
                LANGIDFROMLCID(GetUserDefaultLCID()), pszKey);

        VerQueryValue(lpbVerInfo, szVersionKey, (LPVOID*)&pVerInfo, &cb);

        if (cb == 0)
        {
            //
            // No local language, try US English
            //

            wsprintf(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\%ws"), pszKey);

            VerQueryValue(lpbVerInfo, szVersionKey, (LPVOID*)&pVerInfo, &cb);
        }
    }

    if (cb == 0)
    {
        pVerInfo = NULL;
    }

    return pVerInfo;
}



/*****************************************************************\
*
* CDLGCHGADAPTOR class
*
*        derived from CDIALOG
*
\*****************************************************************/
CDLGCHGADAPTOR::CDLGCHGADAPTOR() : iRet(RET_NO_CHANGE) {
    this->iDlgResID = DLG_SET_CHANGE_VID;
}

BOOL CDLGCHGADAPTOR::InitDlg(HWND hwndFocus) {
    LPDISPLAY_DEVICE pDisplayDevice = (LPDISPLAY_DEVICE) (this->lParam);
    //DLI: make it compile CREGVIDOBJ crvo((&(pDisplayDevice->DeviceName[0]));
#ifdef UNICODE
    CREGVIDOBJ crvo(&(pDisplayDevice->DeviceName[0]));
#else
    CREGVIDOBJ crvo((char *) &(pDisplayDevice->DeviceName[0]));
#endif
    LPTSTR psz;
    TCHAR noVersion[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    HDC hdc;
    ULONG DrivVer;
    HARDWARE_INFO hardwareInfo;
    DWORD i;
    DWORD cb;
    DWORD dwHandle;
    LPBYTE lpbVerInfo = NULL;
    LPTSTR lpInfo;


    LoadString(hInstance, IDS_NO_VERSION_INFO, noVersion, sizeof(noVersion));

    //
    // Get the installed driver names and put them in the dialog
    //

    psz = crvo.CloneDisplayFileNames(TRUE);

    if (psz) {

        this->SendCtlMsg(ID_ADP_CURFILES, WM_SETTEXT, 0, (LPARAM)psz);
        LocalFree(psz);

    }

    //
    // display the adaptor type
    //

    this->SendCtlMsg(ID_ADP_ADAPTOR,
                     WM_SETTEXT,
                     0,
                     (LPARAM) (&(pDisplayDevice->DeviceString[0])));

    //
    // Get the miniport driver path
    //

    wsprintf(szPath, TEXT("drivers\\%s.sys"), crvo.GetMiniPort());

    //
    // Open the file version resource for the driver
    //

    cb = GetFileVersionInfoSize(szPath, &dwHandle);

    if (cb != 0) {

        lpbVerInfo = (LPBYTE)LocalAlloc(LPTR, cb);

        if (lpbVerInfo != NULL) {

            GetFileVersionInfo(szPath, dwHandle, cb, lpbVerInfo);

        }
    }

    //
    // Get the company name and put it in the dialog
    //

    lpInfo = GetVerInfo(lpbVerInfo, TEXT("CompanyName"));

    this->SendCtlMsg(ID_ADP_MANUFACT,
                     WM_SETTEXT,
                     0,
                     (ULONG) (lpInfo ? lpInfo : noVersion));

    //
    // Get the version number from the miniport, and append "," and the
    // display driver version number.
    //

    hdc = GetDC(this->hwnd);
    DrivVer = GetDeviceCaps(hdc, DRIVERVERSION);
    ReleaseDC(this->hwnd, hdc);

    lpInfo = GetVerInfo(lpbVerInfo, TEXT("FileVersion"));

    wsprintf(szPath, TEXT("%s, %d.%d.%d"),
             lpInfo ? lpInfo : noVersion,
             (DrivVer >> 12) & 0xF,
             (DrivVer >> 8) & 0xF,
             DrivVer & 0xFF);

    this->SendCtlMsg(ID_ADP_VERSION, WM_SETTEXT, 0, (LPARAM)szPath);

    if (lpbVerInfo != NULL)
        LocalFree(lpbVerInfo);

    //
    // Now put in the hardware information.
    //

    crvo.GetHardwareInformation(&hardwareInfo);

    this->SendCtlMsg(ID_ADP_CHIP,       WM_SETTEXT, 0, (LPARAM)hardwareInfo.ChipType);
    this->SendCtlMsg(ID_ADP_DAC,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.DACType);
    this->SendCtlMsg(ID_ADP_MEM,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.MemSize);
    this->SendCtlMsg(ID_ADP_ADP_STRING, WM_SETTEXT, 0, (LPARAM)hardwareInfo.AdapString);
    this->SendCtlMsg(ID_ADP_BIOS_INFO,  WM_SETTEXT, 0, (LPARAM)hardwareInfo.BiosString);

    for (i=0; i < 5; i++) {

        if ( *(((LPTSTR *) (&hardwareInfo)) + i) != NULL) {

            LocalFree(*(((LPTSTR *) (&hardwareInfo)) + i));

        }
    }

    return TRUE;
}

BOOL CDLGCHGADAPTOR::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    return FALSE;

}

//
// DoNotify method
//
BOOL CDLGCHGADAPTOR::DoNotify(int idControl, NMHDR *lpnmh, UINT iNoteCode ) {

    switch (iNoteCode) {


    case PSN_APPLY:

        // use ID_PRESTARTWINDOWS to mean something has changed.
        //
        // NOTE: the PropSheet_RestartWindows() does NOT cause
        // windows to restart.  It just sets the return code
        // from the property sheet.
        //
        if (this->iRet != RET_NO_CHANGE)
            PropSheet_RestartWindows(this->hwndParent);
        //
        // NOTE: read the above comment!!!
        //

        SetWindowLong(this->hwnd, DWL_MSGRESULT, PSNRET_NOERROR);
        break;

#if 0
    case PSN_RESET:
        this->iRet = RET_NO_CHANGE;
        break;
#endif

    default:
        return FALSE;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//
// Start of "C" code!
//
///////////////////////////////////////////////////////////////////////////


/***************************************************************************\
* AdvAdaptorPageProc
*
*   Dialog Proc callable from PropertyPage code.
*
* History:
* 10-Oct-1996 JonPa Created it
\***************************************************************************/
BOOL CALLBACK AdvAdaptorPageProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCDLGCHGADAPTOR pddDialog;

    switch (msg) {
    case WM_INITDIALOG:

        pddDialog = new CDLGCHGADAPTOR;
        pddDialog->hwndParent = NULL;
        pddDialog->hmodModule = hInstance;
        pddDialog->lParam = ((LPPROPSHEETPAGE)lParam)->lParam;

        // dli -- moved to multimon.cpp
        //ghwndPropSheet = GetParent(hwnd);

        lParam =  (LPARAM)pddDialog;
        break;

    case WM_NCDESTROY:
        pddDialog = (PCDLGCHGADAPTOR)GetWindowLong(hwnd, DWL_USER);
        SetWindowLong(hwnd, DWL_USER, NULL);

        delete pddDialog;

        break;

    }

    return CDialogProc(hwnd, msg, wParam, lParam );
}


BOOL GetAdvAdapterPropPageParam(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam, LPARAM lparamPage) {
    // Create a property page and call lpfnAdd to add it in.

    HPROPSHEETPAGE page = NULL;
    PROPSHEETPAGE psp;
    BOOL fRet = FALSE;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;

    psp.pfnDlgProc = AdvAdaptorPageProc;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_SET_CHANGE_VID);
    psp.lParam = lparamPage;

    if ((page = CreatePropertySheetPage(&psp)) != NULL) {

        fRet = TRUE;

        if (!lpfnAdd(page, lparam)) {
            DestroyPropertySheetPage(page);
            page = NULL;
            fRet = FALSE;
        }
    }

    return fRet;

}
