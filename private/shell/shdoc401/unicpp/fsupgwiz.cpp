//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: fsupgwiz.c
//
//  File system upgrade wizard.
//
// History:
//  06-25-97 ccteng     Created.
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#ifdef FEATURE_FS_UPGRADE

extern "C" {
//#include <shellp.h>
//#include <devioctl.h>
//#include <ntdddisk.h>
//#include "shobjprv.h"
//#include "ids.h"
//#include "ole2dup.h"

extern const TCHAR c_szNTFS[];

BOOL DriveIOCTL(int iDrive, int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut);
LPCTSTR GetDriveDeviceName(LPTSTR pszName, int iDrive);
};

#ifdef WINNT

const UINT MAX_FSUW_PAGES = DLG_FSUW_LAST - DLG_FSUW_FIRST + 1;

#define MAX_DRIVES  26

#define FSUW_REMOVABLE          0x0001
#define FSUW_SELECTED           0x0002
#define FSUW_BUSY               0x0004
#define FSUW_LOCKED             0x0008
#define FSUW_UPGRADENOW         0x0010
#define FSUW_UPGRADE_SCHEDULED  0x0020

#define iImgDrvFixed    Shell_GetCachedImageIndex(c_szShell32Dll, II_DRIVEFIXED, 0)
#define iImgDrvRemove   Shell_GetCachedImageIndex(c_szShell32Dll, II_DRIVEREMOVE, 0)

class CWizData
{
public:
    IShellFolder *_psfDrv;
    IEnumIDList *_penumDrv;
    HWND _hwndParent;
    int _idPrevPage;               /* ID of page before Finish */
    DWORD _dwDrvFlags[MAX_DRIVES];
    LPITEMIDLIST _pidlDrv[MAX_DRIVES];
    HANDLE _hDrv[MAX_DRIVES];
    int _cSelected;
    int _cBusy;
    int _cUpgradeNow;

    BOOL _bRefreshUpgradeList : 1;

    CWizData(HWND hwndParent);
    ~CWizData();

    BOOL DrvCanUpgrade(LPITEMIDLIST pidl);
    BOOL DrvEnumUpgradable();
    BOOL DrvIsBusy(int iDrive);
    BOOL LockDrive(int iDrive, BOOL bLock);
    void DrvUpgradeNow(int iDrive);
};


CWizData::CWizData(HWND hwndParent):
    _hwndParent(hwndParent),
    _psfDrv(0),
    _penumDrv(0),
    _cSelected(0)
{
    HRESULT hres;
    
    ZeroMemory(_dwDrvFlags, sizeof(_dwDrvFlags));
    ZeroMemory(_pidlDrv, sizeof(_pidlDrv));
    ZeroMemory(_hDrv, sizeof(_hDrv));

    hres = CDrives_CreateInstance(NULL, IID_IShellFolder, (LPVOID*)&_psfDrv);
    if (SUCCEEDED(hres))
    {
        _psfDrv->EnumObjects(_hwndParent, SHCONTF_FOLDERS, &_penumDrv);
    }
}


CWizData::~CWizData()
{
    for (int i = 0; i < MAX_DRIVES; i++)
    {
        if (_pidlDrv[i])
            SHFree(_pidlDrv[i]);

        if (_dwDrvFlags[i] & FSUW_LOCKED)
            LockDrive(i, FALSE);

        if (_hDrv[i])
            CloseHandle(_hDrv[i]);
    }

    if (_penumDrv)
        _penumDrv->Release();
        
    if (_psfDrv)
        _psfDrv->Release();
}


void
CWizData::DrvUpgradeNow(
    int iDrive
)
{
    // try to lock again.

    DrvIsBusy(iDrive);

    // dismount. unlock. close handle.

    DWORD bytesRead;
    if (!DeviceIoControl(_hDrv[iDrive], FSCTL_DISMOUNT_VOLUME,
            NULL, 0, NULL, 0, &bytesRead, NULL))
    {
        DebugMsg(TF_WARNING, TEXT("FSCTL_DISMOUNT_VOLUME failed with error %d."), GetLastError());
    }

    if (_dwDrvFlags[iDrive] & FSUW_LOCKED)
        LockDrive(iDrive, FALSE);

    CloseHandle(_hDrv[iDrive]);
    _hDrv[iDrive] = NULL;

    // remount.

    TCHAR szDevName[7];
    _hDrv[iDrive] = CreateFile(GetDriveDeviceName(szDevName, iDrive),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);
}


// Lock/Unlock the volume.
// We need to save the volume handle when we lock so that we can unlock it later.
//
BOOL
CWizData::LockDrive(
    int iDrive,
    BOOL bLock
)
{
    DWORD bytesRead;
    HANDLE h;

    h = _hDrv[iDrive];

    if (!h || (h == INVALID_HANDLE_VALUE))
        return FALSE;

    if (DeviceIoControl(h,
        bLock ? FSCTL_LOCK_VOLUME : FSCTL_UNLOCK_VOLUME,
        NULL, 0, NULL, 0, &bytesRead, NULL))
    {
        if (bLock)
            _dwDrvFlags[iDrive] |= FSUW_LOCKED;
        else
            _dwDrvFlags[iDrive] &= ~FSUW_LOCKED;

        return TRUE;
    }

    return FALSE;
}


// Check if the volume is busy (in use).
//
BOOL
CWizData::DrvIsBusy(
    int iDrive
)
{
    if (_dwDrvFlags[iDrive] & FSUW_LOCKED)
        return FALSE;

    if (LockDrive(iDrive, TRUE))
        return FALSE;

    DebugMsg(TF_WARNING, TEXT("FSCTL_LOCK_VOLUME failed with error %d."), GetLastError());

    return TRUE;
}


BOOL
CWizData::DrvCanUpgrade(
    LPITEMIDLIST pidl
)
{
    STRRET strret;
    HRESULT hres;
    WCHAR wszBuf[MAX_PATH];
    ULONG ulAttrib;
    int iDrive = MAX_DRIVES;

    // skip if it's not filesystem.

    ulAttrib = SFGAO_FILESYSTEM;

    hres = _psfDrv->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &ulAttrib);
    if (FAILED(hres) || 
        !(ulAttrib & SFGAO_FILESYSTEM))
    {
        return FALSE;
    }

    // skip net drive and floppy.

    hres = _psfDrv->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
    if (SUCCEEDED(hres) &&
        StrRetToStrW(wszBuf, ARRAYSIZE(wszBuf), &strret, pidl))
    {
#ifndef UNICODE
        BUGBUG ! This module is unicode only.
#endif

        iDrive = DRIVEID(wszBuf);

        if (IsRemoteDrive(iDrive))
            return FALSE;

        DISK_GEOMETRY disk_g;

        if (DriveIOCTL(iDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL, 0, &disk_g, SIZEOF(disk_g)))
        {
            switch(disk_g.MediaType)
            {
                case RemovableMedia:
                    _dwDrvFlags[iDrive] |= FSUW_REMOVABLE;

                    // fall through

                case FixedMedia:
                    break;

                default:
                    // must be floppy in this case.

                    return FALSE;
            }
        }
    }
    else
        return FALSE;

    // skip if it's readonly, and check it after we eliminate the net drives
    // and floppy, as they could be very slow.

    ulAttrib = SFGAO_READONLY;

    hres = _psfDrv->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &ulAttrib);
    if (FAILED(hres) || 
        (ulAttrib & SFGAO_READONLY))
    {
        return FALSE;
    }

    // check volume filesystem and version.

    NTSTATUS err;
    UCHAR MajorVer, MinorVer;
    TCHAR szFileSystem[MAX_FILE_SYSTEM_FORMAT_NAME_LEN];
    
    if (QueryFileSystemName(wszBuf, szFileSystem, &MajorVer, &MinorVer, &err))
    {
        if (lstrcmpi(szFileSystem, c_szNTFS))
        {
            // not NTFS, fail.

            return FALSE;
        }

        // check NTFS version.

        // BUGBUG ???
        // Do we only compare major version ?
        // Or should we compare minor version as well ?

        UCHAR FSMajorVer, FSMinorVer;

        if (QueryLatestFileSystemVersion(szFileSystem, &FSMajorVer, &FSMinorVer))
        {
            if (MajorVer < FSMajorVer)
            {
                // THIS IS THE ONLY PLACE WHERE WE RETURN TRUE.
                // save the pidl.
            
                if (iDrive < MAX_DRIVES)
                {
                    HANDLE h = CreateFile(GetDriveDeviceName(wszBuf, iDrive),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, NULL);

                    if (h != INVALID_HANDLE_VALUE)
                    {
                        _hDrv[iDrive] = h;
                        _pidlDrv[iDrive] = pidl;
                        return TRUE;
                    }
                }
            }
        }
        else
        {
            DebugMsg(TF_WARNING, TEXT("QueryLatestFileSystemVersion failed with error %d."), GetLastError());
        }
    }
    else
    {
        DebugMsg(TF_WARNING, TEXT("QueryFileSystemName failed with error %d."), GetLastError());
    }

    return FALSE;
}


BOOL
CWizData::DrvEnumUpgradable()
{
    LPITEMIDLIST pidlT;
    ULONG celt;

    _penumDrv->Reset();

    while ((_penumDrv->Next(1, &pidlT, &celt) == NOERROR) && (celt==1))
    {
        if (!DrvCanUpgrade(pidlT))
        {
            SHFree(pidlT);
        }
    }

    return TRUE;
}


void AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    LPVOID pwd,
    DWORD dwFlags,
    UINT idsTitle,
    UINT idsSub
)
{
    if (ppsh->nPages < MAX_FSUW_PAGES)
    {
        PROPSHEETPAGE psp;

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = dwFlags;
        psp.hInstance   = MLGetHinst();
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc  = pfn;
        psp.lParam      = (LPARAM)pwd;
        psp.pszHeaderTitle      = MAKEINTRESOURCE(idsTitle);
        psp.pszHeaderSubTitle   = MAKEINTRESOURCE(idsSub);

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
            ppsh->nPages++;
    }
}  // AddPage


void GoToPage(HWND hDlg, int idPage)
{
    SetWindowLong(hDlg, DWL_MSGRESULT, idPage);
}


void InitWizDataPtr(HWND hDlg, LPARAM lParam)
{
    CWizData *pwd = (CWizData *)(((LPPROPSHEETPAGE)lParam)->lParam);
    SetWindowLong(hDlg, DWL_USER, (LPARAM)pwd);
}


void InitDrvListView(HWND hwndList)
{
    HIMAGELIST himlLarge, himlSmall;
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;

    DWORD   dwExStyle = ListView_GetExtendedListViewStyle(hwndList);
    ListView_SetExtendedListViewStyle(hwndList,  dwExStyle | LVS_EX_CHECKBOXES);
    
    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(hwndList, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);

    SetWindowLong(hwndList, GWL_EXSTYLE,
            GetWindowLong(hwndList, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    GetClientRect(hwndList, &rc);
    col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
            - 4 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(hwndList, 0, &col);
}


void DrvListViewRemoveAll(HWND hwndList)
{
    int iCount;

    iCount = ListView_GetItemCount(hwndList);
    while (iCount--)
    {
        ListView_DeleteItem(hwndList, iCount);
    }
}


BOOL CALLBACK WelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            break;

        default:
            // not handled
            return FALSE;
        }

        return TRUE;

    } // end of switch on message

    return FALSE;
}


BOOL CALLBACK VolumesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;
    NM_LISTVIEW *plvn;
    CWizData *pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);
    HWND hwndList;
    LV_ITEM item;
    int i, iCount;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        plvn = (NM_LISTVIEW *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            if (pwd->_cSelected > 0)
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            else
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

            break;

        case PSN_WIZNEXT:
            // check which drive(s) are selected to upgrade.
            // and check if the drive is busy.

            if (!pwd)
                return FALSE;

            pwd->_cBusy = 0;
            pwd->_cUpgradeNow = 0;

            hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
            iCount = ListView_GetItemCount(hwndList);
            for (i = 0; i < iCount; i++)
            {
                item.mask = LVIF_PARAM;
                item.iItem = i;
                item.iSubItem = 0;
                item.stateMask = LVIS_SELECTED;
    
                if (ListView_GetItem(hwndList, &item) &&
                    (item.lParam < MAX_DRIVES))
                {
                    int iDrive = item.lParam;
                    
                    // reset the flags.

                    pwd->_dwDrvFlags[iDrive] &= 
                        ~(FSUW_BUSY | FSUW_UPGRADENOW);

                    if (pwd->_dwDrvFlags[iDrive] & FSUW_SELECTED)
                    {
                        // check if the drive is busy.

                        if (pwd->DrvIsBusy(iDrive))
                        {
                            pwd->_cBusy++;
                            pwd->_dwDrvFlags[iDrive] |= FSUW_BUSY;
                        }
                        else
                        {
                            pwd->_cUpgradeNow++;
                            pwd->_dwDrvFlags[iDrive] |= FSUW_UPGRADENOW;
                        }
                    }
                }
            }

            pwd->_bRefreshUpgradeList = TRUE;

            // If if volumes selected are not busy, skip the options pages and
            // go directly to the progress page.

            if (pwd->_cBusy == 0)
            {
                GoToPage(hDlg, DLG_FSUW_STATUSBAR_UPGRADING);
                break;
            }

            return FALSE;

        case LVN_ITEMCHANGED:
            // We only care about STATECHANGE messages

            if (!(plvn->uChanged & LVIF_STATE))
                return FALSE;

            hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
            i = plvn->lParam;

            DWORD dwFlags;

            dwFlags = ListView_GetCheckState(hwndList, plvn->iItem) ? FSUW_SELECTED : 0;

            if ((dwFlags ^ pwd->_dwDrvFlags[i]) & FSUW_SELECTED)
            {
                if (dwFlags & FSUW_SELECTED)
                {
                    pwd->_cSelected++;
                    pwd->_dwDrvFlags[i] |= FSUW_SELECTED;
                }
                else
                {
                    pwd->_cSelected--;
                    pwd->_dwDrvFlags[i] &= ~FSUW_SELECTED;
                }

                if (pwd->_cSelected > 0)
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                else
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            }

            break;
    
        default:
            // not handled.
            return FALSE;
        }

        return TRUE;

    case WM_INITDIALOG:
        // Enum the upgradable drives and fill the list box.

        InitWizDataPtr(hDlg, lParam);
        pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);
        hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
        InitDrvListView(hwndList);

        if (pwd)
        {
            // IMPORTANT: ONLY DO THIS IN THIS PAGE.

            HCURSOR hcSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

            pwd->DrvEnumUpgradable();

            SetCursor(hcSave);
    
            for (int i = 0; i < MAX_DRIVES; i++)
            {
                STRRET strret;
                HRESULT hres;
                WCHAR wszBuf[MAX_PATH];
                LPITEMIDLIST pidlT = pwd->_pidlDrv[i];

                if (!pidlT)
                    continue;

                hres = pwd->_psfDrv->GetDisplayNameOf(pidlT, SHGDN_INFOLDER | SHGDN_NORMAL, &strret);
                if (SUCCEEDED(hres))
                {
                    StrRetToStrW(wszBuf, ARRAYSIZE(wszBuf), &strret, pidlT);

#ifndef UNICODE
                    BUGBUG ! This module is unicode only.
#endif

                    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                    item.iItem = INT_MAX;
                    item.iSubItem = 0;
                    item.stateMask = LVIS_STATEIMAGEMASK;
                    item.state = 0;
                    item.iImage = (pwd->_dwDrvFlags[i] & FSUW_REMOVABLE) ? iImgDrvRemove : iImgDrvFixed;
                    item.pszText = wszBuf;
                    item.lParam = i;

#if 1
                    ULONG ulFlags;
                    
                    // Check the current state of the upgrade bit by
                    // calling DriveIoCtl(FS_IS_VOLUME_DIRRY)
                    // FSCTL_IS_VOLUME_DIRTY returns a ULONG and check this bit,
                    // #define VOLUME_UPGRADE_SCHEDULED         (0x00000002)
                    //
                    // Then init the check box accordingly

                    if (DriveIOCTL(i, FSCTL_IS_VOLUME_DIRTY,
                        NULL, 0, &ulFlags, SIZEOF(ulFlags)))
                    {
                        item.state = ulFlags & VOLUME_UPGRADE_SCHEDULED ? 0x00002000 : 0x00001000;

                        if (ulFlags & VOLUME_UPGRADE_SCHEDULED)
                            pwd->_dwDrvFlags[i] |= FSUW_UPGRADE_SCHEDULED;
                    }
#endif

                    ListView_InsertItem(hwndList, &item);

                    // BUGBUG: There is a problem with the listview implementation because of which
                    // the check box is not displayed even though lvi.state is set correctly.
                    //  We have to find the item and set it again.
                    ListView_SetItemState(hwndList,
                        ListView_FindItem(hwndList, -1, &item),
                        ulFlags & VOLUME_UPGRADE_SCHEDULED ? 0x00002000 : 0x00001000,
                        LVIS_STATEIMAGEMASK);
                }
            }
        }

        return TRUE;
        
    } // end of switch on message

    // not handled.
    return FALSE;
}


BOOL CALLBACK OptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;
    NM_LISTVIEW *plvn;
    CWizData *pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);
    HWND hwndList;
    LV_ITEM item;
    int i, iCount;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        plvn = (NM_LISTVIEW *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

            // rebuild the list if needed.

            if (pwd && pwd->_bRefreshUpgradeList)
            {
                hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
                DrvListViewRemoveAll(hwndList);
                
                for (i = 0; i < MAX_DRIVES; i++)
                {
                    STRRET strret;
                    HRESULT hres;
                    WCHAR wszBuf[MAX_PATH];
                    LPITEMIDLIST pidlT = pwd->_pidlDrv[i];
    
                    // only list the drives that are busy.
    
                    if (!pidlT || !(pwd->_dwDrvFlags[i] & FSUW_BUSY))
                        continue;

                    hres = pwd->_psfDrv->GetDisplayNameOf(pidlT, SHGDN_INFOLDER | SHGDN_NORMAL, &strret);
                    if (SUCCEEDED(hres))
                    {
                        StrRetToStrW(wszBuf, ARRAYSIZE(wszBuf), &strret, pidlT);
    
#ifndef UNICODE
                        BUGBUG ! This module is unicode only.
#endif

                        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                        item.iItem = INT_MAX;
                        item.iSubItem = 0;
                        item.state = 0;
                        item.iImage = (pwd->_dwDrvFlags[i] & FSUW_REMOVABLE) ? iImgDrvRemove : iImgDrvFixed;
                        item.pszText = wszBuf;
                        item.lParam = i;
                        ListView_InsertItem(hwndList, &item);
                    }
                }

                pwd->_bRefreshUpgradeList = FALSE;
            }

            break;

        case PSN_WIZNEXT:
            // go through the listview and see which busy volumes are
            // selected to be upgraded now.

            hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
            iCount = ListView_GetItemCount(hwndList);
            for (i = 0; i < iCount; i++)
            {
                item.mask = LVIF_PARAM | LVIF_STATE;
                item.iItem = i;
                item.iSubItem = 0;
                item.stateMask = LVIS_SELECTED;
    
                if (ListView_GetItem(hwndList, &item) &&
                    (item.lParam < MAX_DRIVES))
                {
                    int iDrive = item.lParam;
                    
                    if (item.state & LVIS_SELECTED)
                        pwd->_dwDrvFlags[iDrive] |= FSUW_UPGRADENOW;
                    else
                        pwd->_dwDrvFlags[iDrive] &= ~FSUW_UPGRADENOW;
                }
            }

            return FALSE;

        case LVN_ITEMCHANGED:
            // We only care about STATECHANGE messages

            if (!(plvn->uChanged & LVIF_STATE))
                return FALSE;

            hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
            i = plvn->lParam;

            DWORD dwFlags;

            dwFlags = ListView_GetCheckState(hwndList, plvn->iItem) ? FSUW_UPGRADENOW : 0;

            if ((dwFlags ^ pwd->_dwDrvFlags[i]) & FSUW_UPGRADENOW)
            {
                if (dwFlags & FSUW_UPGRADENOW)
                {
                    pwd->_cUpgradeNow++;
                    pwd->_dwDrvFlags[i] |= FSUW_UPGRADENOW;
                }
                else
                {
                    pwd->_cUpgradeNow--;
                    pwd->_dwDrvFlags[i] &= ~FSUW_UPGRADENOW;
                }
            }

            break;
    
        default:
            // not handled.
            return FALSE;
        }

        return TRUE;

    case WM_INITDIALOG:
        InitWizDataPtr(hDlg, lParam);
        pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);
        hwndList = GetDlgItem(hDlg, IDC_FSUW_VOLUME_LIST);
        InitDrvListView(hwndList);

        return TRUE;
        
    } // end of switch on message

    return FALSE;
}


#define FSUW_TIMER_ID   1234

BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;
    CWizData *pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);
    int i;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            // disable BACK & NEXT.

            PropSheet_SetWizButtons(GetParent(hDlg), 0);
            SetTimer(hDlg, FSUW_TIMER_ID, 200, NULL);

            break;

        default:
            // not handled.
            return FALSE;
        }

        return TRUE;

    case WM_TIMER:
        if (wParam == FSUW_TIMER_ID)
        {
            KillTimer(hDlg, FSUW_TIMER_ID);

            // send FSCTL_ENABLE_UPGRADE to all the selected drives.

// BUGBUG !!!
// TMP HACK UNTIL we have the new NT5 build that handles the new
// FSCTL_ENABLE_UPGRADE, use the old value.
#ifdef FSCTL_ENABLE_UPGRADE
#undef FSCTL_ENABLE_UPGRADE
#define FSCTL_ENABLE_UPGRADE CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 52, METHOD_NEITHER, FILE_WRITE_DATA)
#endif

            for (i = 0; i < MAX_DRIVES; i++)
            {
                if (pwd->_dwDrvFlags[i] & FSUW_SELECTED)
                {
                    DWORD bytesRead;

                    // must unlock before FSCTL_ENABLE_UPGRADE.

                    if (pwd->_dwDrvFlags[i] & FSUW_LOCKED)
                    {
                        pwd->LockDrive(i, FALSE);

                        // FSCTL_ENABLE_UPGRADE will fail unless we create
                        // a new handle.

                        CloseHandle(pwd->_hDrv[i]);
                        TCHAR szDevName[7];
                        pwd->_hDrv[i] = CreateFile(GetDriveDeviceName(szDevName, i),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL, OPEN_EXISTING, 0, NULL);
                    }

                    if (!DeviceIoControl(pwd->_hDrv[i], FSCTL_ENABLE_UPGRADE,
                            NULL, 0, NULL, 0, &bytesRead, NULL))
                    {
                        DebugMsg(TF_WARNING, TEXT("FSCTL_ENABLE_UPGRADE failed with error %d."), GetLastError());
                    }
                }
                else if (pwd->_dwDrvFlags[i] & FSUW_UPGRADE_SCHEDULED)
                {
                    // cancel the scheduled upgrade.

                    ULONG ulUpgrade = 0;

                    if (!DriveIOCTL(i, FSCTL_ENABLE_UPGRADE,
                            &ulUpgrade, SIZEOF(ulUpgrade), NULL, 0))
                    {
                        DebugMsg(TF_WARNING, TEXT("FSCTL_ENABLE_UPGRADE failed with error %d."), GetLastError());
                    }
                }
            }

            // if we don't have any volumes to upgrade now,
            // go directly to the finish page.

            if (pwd->_cUpgradeNow != 0)
            {
                // go through the ones that we have already locked,
                // and the ones that we decide to upgrade now anyway,
                // and do the dismount/remount.

                int iPercent = 100/pwd->_cUpgradeNow;
                int iProgress = 0;
                TCHAR szTemp[32];
                TCHAR szText[MAX_PATH];

                MLLoadString(IDS_FSUW_UPGRADING, szTemp, ARRAYSIZE(szTemp));

                for (i = 0; i < MAX_DRIVES; i++)
                {
                    STRRET strret;
                    HRESULT hres;
                    WCHAR wszBuf[MAX_PATH];
                    LPITEMIDLIST pidlT = pwd->_pidlDrv[i];

                    if (pidlT && (pwd->_dwDrvFlags[i] & FSUW_UPGRADENOW))
                    {
                        // update the status text.

                        hres = pwd->_psfDrv->GetDisplayNameOf(pidlT, SHGDN_INFOLDER | SHGDN_NORMAL, &strret);
                        if (SUCCEEDED(hres))
                        {
                            StrRetToStrW(wszBuf, ARRAYSIZE(wszBuf), &strret, pidlT);

#ifndef UNICODE
                            BUGBUG ! This module is unicode only.
#endif

                            wsprintf(szText, szTemp, wszBuf);
                            SetDlgItemText(hDlg, IDC_FSUW_UPGRADE_VOLUME, szText);
                        }

                        Sleep(500);
                        pwd->DrvUpgradeNow(i);

                        // update progress bar.

                        iProgress += iPercent;
                        SendDlgItemMessage(hDlg, IDC_FSUW_PROGRESS, PBM_SETPOS, iProgress, 0);
                    }
                }

                Sleep(500);
            }
            else
                SendDlgItemMessage(hDlg, IDC_FSUW_PROGRESS, PBM_SETPOS, 100, 0);

            PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
        }

        break;

    case WM_INITDIALOG:
        InitWizDataPtr(hDlg, lParam);
        pwd = (CWizData *)GetWindowLong(hDlg, DWL_USER);

        return TRUE;

    } // end of switch on message

    return FALSE;
}


BOOL CALLBACK FinishDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;

    switch(message)
    {
    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        switch(lpnm->code)
        {
        case PSN_SETACTIVE:
            // BUGBUG !!!
            // we should probably disable CANCEL button since
            // it doesn't really cancel anything.

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
            break;

        case PSN_WIZFINISH:
        default:
            // not handled.
            return FALSE;
        }

        return TRUE;

    } // end of switch on message

    return FALSE;
}


extern "C" int
FSUpgradeWizard(
    HWND hwndParent
)
{
    CWizData wd(hwndParent);
    LPPROPSHEETHEADER ppsh;
    int ret;

    // Allocate the property sheet header
    //
    if ((ppsh = (LPPROPSHEETHEADER)LocalAlloc(LPTR, sizeof(PROPSHEETHEADER)+
                (MAX_FSUW_PAGES * sizeof(HPROPSHEETPAGE)))) != NULL)
    {
        ppsh->dwSize     = sizeof(*ppsh);
        ppsh->dwFlags    = PSH_WIZARD97IE4 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
        ppsh->hwndParent = hwndParent;
        ppsh->hInstance  = MLGetHinst();
        ppsh->pszCaption = NULL;
        ppsh->nPages     = 0;
        ppsh->nStartPage = 0;
        ppsh->phpage     = (HPROPSHEETPAGE *)(ppsh+1);
        ppsh->pszbmWatermark = MAKEINTRESOURCE(IDB_FSUW_BACKGROUND);
        ppsh->pszbmHeader = MAKEINTRESOURCE(IDB_FSUW_HEADER); 

        AddPage(ppsh, DLG_FSUW_WELCOME, WelcomeDlgProc, &wd,
            PSP_HIDEHEADER, 0, 0);
        AddPage(ppsh, DLG_FSUW_CHOOSE_VOLUMES, VolumesDlgProc, &wd,
            PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, IDS_FSUW_VOLUMES_TITLE, IDS_FSUW_VOLUMES_SUB);
        AddPage(ppsh, DLG_FSUW_SELECT_OPTION, OptionsDlgProc, &wd,
            PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, IDS_FSUW_OPTIONS_TITLE, IDS_FSUW_OPTIONS_SUB);
        AddPage(ppsh, DLG_FSUW_STATUSBAR_UPGRADING, ProgressDlgProc, &wd,
            PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, IDS_FSUW_PROGRESS_TITLE, IDS_FSUW_PROGRESS_SUB);
        AddPage(ppsh, DLG_FSUW_COMPLETION_PAGE, FinishDlgProc, &wd,
            PSP_HIDEHEADER, 0, 0);

        ret = MLPropertySheet(ppsh);

        LocalFree((HLOCAL)ppsh);
    }

    return ret;
}


#endif // WINNT
#endif // FEATURE_FS_UPGRADE
