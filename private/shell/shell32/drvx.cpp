#include "shellprv.h"
#pragma  hdrstop

extern "C"
{
#include <hwtab.h>
#include "fstreex.h"
#include "views.h"
#include "drives.h"
#include "propsht.h"
#include "infotip.h"
#include "datautil.h"
#include "netview.h"
#include "bitbuck.h"
#include "drawpie.h"
#include "shitemid.h"
#include "devguid.h"
#include "ids.h"
}

#ifndef WINNT
#define Not_VxD
#include <vwin32.h>     // DeviceIOCtl calls
#endif

#include "mtpt.h"

// from mtpt.cpp
STDAPI_(void) CMtPt_InvalidateDriveType(int iDrive);

const TCHAR c_szOptReg[]            = TEXT("MyComputer\\defragpath");

#define DRIVEID_TCHAR(szPath)   ((szPath[0] - TEXT('A')) & 31)

// Internal function prototype
BOOL SetInitialDriveAttributes(DRIVEPROPSHEETPAGE* pdpsp);

///////////////////////////////////////////////////////////////////////////////
// Begin: Old C fct required externally
///////////////////////////////////////////////////////////////////////////////
STDAPI_(int) RealDriveTypeFlags(int iDrive, BOOL fOKToHitNet)
{
    int iType = DRIVE_NO_ROOT_DIR;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        iType = pMtPt->GetDRIVEType(fOKToHitNet);
        iType |= pMtPt->GetDriveFlags();
        iType |= pMtPt->GetVolumeFlags();
        pMtPt->Release();
    }

    return iType;    
}

STDAPI_(int) RealDriveType(int iDrive, BOOL fOKToHitNet)
{
    int iType = DRIVE_NO_ROOT_DIR;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        iType = pMtPt->GetDRIVEType(fOKToHitNet);
        pMtPt->Release();
    }

    return iType & DRIVE_TYPE;    
}

STDAPI_(int) DriveType(int iDrive)
{
    return RealDriveType(iDrive, TRUE);
}

STDAPI_(DWORD) PathGetClusterSize(LPCTSTR pszPath)
{
    DWORD dwSize = 512;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pszPath);

    if (pMtPt)
    {
        dwSize = pMtPt->GetClusterSize();
        pMtPt->Release();
    }
    return dwSize;
}

STDAPI_(UINT) GetMountedVolumeIcon(LPCTSTR pszMountPoint, LPTSTR pszModule, DWORD cchModule)
{
    UINT iIcon = II_FOLDER;
#ifdef WINNT
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pszMountPoint);

    if (pMtPt)
    {
        iIcon = pMtPt->GetIcon(pszModule, cchModule);
        pMtPt->Release();
    }
#endif //WINNT
    return iIcon;
}


STDAPI_(BOOL) IsDisconnectedNetDrive(int iDrive)
{
    BOOL fDisc = 0;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fDisc = pMtPt->IsDisconnectedNetDrive();
        pMtPt->Release();
    }
    return fDisc;
}

STDAPI_(BOOL) IsUnavailableNetDrive(int iDrive)
{
    BOOL fUnAvail = 0;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fUnAvail = pMtPt->IsUnavailableNetDrive();
        pMtPt->Release();
    }

    return fUnAvail;

}

STDMETHODIMP SetDriveLabel(HWND hwnd, IUnknown* punkEnableModless, int iDrive, LPCTSTR pszDriveLabel)
{
    //BUGBUG: need to use hwnd?
    HRESULT hres = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hres = pMtPt->SetDriveLabel(pszDriveLabel);
        pMtPt->Release();
    }

    return hres;
}

STDMETHODIMP GetDriveComment(int iDrive, LPTSTR pszComment, int cchComment)
{
    HRESULT hres = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hres = pMtPt->GetComment(pszComment, cchComment);
        pMtPt->Release();
    }

    return hres;
}

STDMETHODIMP GetDriveHTMLInfoTipFile(int iDrive, LPTSTR pszHTMLInfoTipFile,
                                     int cchHTMLInfoTipFile)
{
    HRESULT hres = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hres = pMtPt->GetHTMLInfoTipFile(pszHTMLInfoTipFile, cchHTMLInfoTipFile);
        pMtPt->Release();
    }

    return hres;
}

STDAPI_(BOOL) GetDiskCleanupPath(LPTSTR pszBuf, UINT cbSize)
{
    TCHAR szPathToCleanupExe[MAX_PATH];
    BOOL bFoundCleanMgr = FALSE;
    HKEY hkExp;

    if(pszBuf)
       *pszBuf = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, &hkExp) == ERROR_SUCCESS)
    {
        LONG cbLen = SIZEOF(szPathToCleanupExe);
        if (SHRegQueryValue(hkExp, TEXT("MyComputer\\cleanuppath"), szPathToCleanupExe, &cbLen) == ERROR_SUCCESS)
        {
            bFoundCleanMgr = TRUE;
        }
        RegCloseKey(hkExp);
    }

    if (bFoundCleanMgr && pszBuf)
       lstrcpyn(pszBuf, szPathToCleanupExe, cbSize);

    return bFoundCleanMgr;
}

STDAPI_(void) LaunchDiskCleanup(HWND hwnd, int iDrive)
{
    TCHAR szPathToCleanupExe[MAX_PATH], szCmdLine[MAX_PATH + 20];   // pad 20 for command line args

    if (GetDiskCleanupPath(szPathToCleanupExe, ARRAYSIZE(szPathToCleanupExe)))
    {
        //BUGBUG: don't use iDrive
        wsprintf(szCmdLine, szPathToCleanupExe, TEXT('A') + iDrive);

        if (!ShellExecCmdLine(NULL, szCmdLine, NULL, SW_SHOWNORMAL, NULL, 
            SECL_USEFULLPATHDIR | SECL_NO_UI))
        {
            ShellMessageBox(HINST_THISDLL, NULL, 
                        MAKEINTRESOURCE(IDS_NO_CLEANMGR_APP), 
                        NULL, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        }
    }
}

#ifndef WINNT
// _DriveIOCtl function callable from 16 bit thunk side...
STDAPI_(BOOL) SH16To32DriveIOCTL(int iDrive, int cmd, void *pv)
{
    BOOL fRet = FALSE;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fRet = pMtPt->_DriveIOCTL(cmd, NULL, 0, pv, 0);
        pMtPt->Release();
    }

    return fRet;
}

// This function allows the 16 bit side to do int26 releasing win16 lock...

#pragma pack(1)
typedef struct I256PBLK {
    DWORD   strtsec;
    WORD    count;
    DWORD   BufPtr;
} I256PBLK;
#pragma pack()

STDAPI_(int) SH16To32Int2526(int iDrive, int iInt, void *pv, WORD count, DWORD ssector)
{
    DIOC_REGISTERS reg;
    I256PBLK       i256ParmBlk;
    HANDLE         hVolume = INVALID_HANDLE_VALUE;
    static unsigned char g_DoNewINT2526 = 0xFF;

    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (g_DoNewINT2526 != 0)
    {

        i256ParmBlk.strtsec = ssector;
        i256ParmBlk.count   = count;
        i256ParmBlk.BufPtr  = (DWORD)pv;

        reg.reg_ESI = (iInt == 25)? 0 : 0x0001;
        reg.reg_EDX = iDrive + 1;       // 1 based drive number
        reg.reg_ECX = 0xFFFFFFFF;
        reg.reg_EBX = (DWORD)&i256ParmBlk;
        reg.reg_EAX = 0x7305;
        reg.reg_Flags = 0x0001;        // flags, assume error (carry)

        if (pMtPt)
        {
            if (pMtPt->_DriveIOCTL(VWIN32_DIOC_DOS_DRIVEINFO,
                &reg, sizeof(reg), &reg, sizeof(reg)))
            {
                if (g_DoNewINT2526 != 0xFF)
                {
                    if (pMtPt)
                        pMtPt->Release();

                    return (reg.reg_Flags & 0x0001) ? (int)reg.reg_EAX : 0;
                } 
                else 
                {
                    if(reg.reg_Flags & 0x0001)
                    {
                        if((int)reg.reg_EAX == 0x0001)
                        {
                            g_DoNewINT2526 = 0;
                        }
                    } 
                    else 
                    {
                        g_DoNewINT2526 = 1;

                        if (pMtPt)
                            pMtPt->Release();

                        return 0;
                    }
                }
            }
        }
    }

    reg.reg_EAX = iDrive;           // 0 based drive number
    reg.reg_ECX = count;            // Count of sectors
    reg.reg_EDX = (DWORD)ssector;   // which sector to write to
    reg.reg_EBX = (DWORD)pv;        // pointer to buffer to output
    reg.reg_Flags = 0x0001;         // flags, assume error (carry)

    if (pMtPt)
    {
        pMtPt->_DriveIOCTL((iInt == 25)? VWIN32_DIOC_DOS_INT25 : VWIN32_DIOC_DOS_INT26,
            &reg, sizeof(reg), &reg, sizeof(reg));
        pMtPt->Release();
    }

    return (reg.reg_Flags & 0x0001) ? (int)reg.reg_EAX : 0;
}
#endif
///////////////////////////////////////////////////////////////////////////////
// End:   Old C fct required externally
///////////////////////////////////////////////////////////////////////////////

//
// fDoIt -- TRUE, if we make connections; FALSE, if just querying.
//
BOOL _MakeConnection(IDataObject *pDataObj, BOOL fDoIt)
{
    STGMEDIUM medium;
    FORMATETC fmte = {g_cfNetResource, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BOOL fAnyConnectable = FALSE;

    if (SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
    {
        LPNETRESOURCE pnr = (LPNETRESOURCE)LocalAlloc(LPTR, 1024);
        if (pnr)
        {
            UINT iItem, cItems = SHGetNetResource(medium.hGlobal, (UINT)-1, NULL, 0);
            for (iItem = 0; iItem < cItems; iItem++)
            {
                if (SHGetNetResource(medium.hGlobal, iItem, pnr, 1024) &&
                    pnr->dwUsage & RESOURCEUSAGE_CONNECTABLE &&
                    !(pnr->dwType & RESOURCETYPE_PRINT))
                {
                    fAnyConnectable = TRUE;
                    if (fDoIt)
                    {
                        SHNetConnectionDialog(NULL, pnr->lpRemoteName, pnr->dwType);
                        SHChangeNotifyHandleEvents();
                    }
                    else
                    {
                        break;  // We are just querying.
                    }
                }
            }
            LocalFree(pnr);
        }
        ReleaseStgMedium(&medium);
    }

    return fAnyConnectable;
}

//
// the entry of "make connection thread"
//
DWORD WINAPI CDrives_MakeConnection(void *pv)
{
    IDataObject *pdtobj;
    if (SUCCEEDED(CoGetInterfaceAndReleaseStream((IStream *)pv, IID_IDataObject, (void **)&pdtobj)))
    {
        _MakeConnection(pdtobj, TRUE);
        pdtobj->Release();
    }

    return 0;
}

//
// puts DROPEFFECT_LINK in *pdwEffect, only if the data object
// contains one or more net resource.
//
STDMETHODIMP CDrivesIDLDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *This = IToClass(CIDLDropTarget, dropt, pdropt);

    // Call the base class first.
    CIDLDropTarget_DragEnter(pdropt, pDataObj, grfKeyState, pt, pdwEffect);

    *pdwEffect &= _MakeConnection(pDataObj, FALSE) ? DROPEFFECT_LINK : DROPEFFECT_NONE;

    This->dwEffectLastReturned = *pdwEffect;

    return NOERROR;     // Notes: we should NOT return hres as it.
}

//
// creates a connection to a dropped net resource object.
//
STDMETHODIMP CDrivesIDLDropTarget_Drop(IDropTarget * pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *This = IToClass(CIDLDropTarget, dropt, pdropt);
    HRESULT hres;

    if (This->dwData & DTID_NETRES)
    {
        *pdwEffect = DROPEFFECT_LINK;

        hres = CIDLDropTarget_DragDropMenu(This, DROPEFFECT_LINK, pDataObj,
            pt, pdwEffect, NULL, NULL, POPUP_DRIVES_NONDEFAULTDD, grfKeyState);

        if (hres == S_FALSE)
        {
            // we create another thread to avoid blocking the source thread.
            IStream *pstm;
            if (S_OK == CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObj, &pstm))
            {
                if (SHCreateThread(CDrives_MakeConnection, pstm, CTF_COINIT, NULL))
                {
                    hres = NOERROR;
                }
                else
                {
                    pstm->Release();
                    hres = E_OUTOFMEMORY;
                }
            }
        }
    }
    else
    {
        //
        // Because QueryGetData() failed, we don't call CIDLDropTarget_
        // DragDropMenu(). Therefore, we must call this explicitly.
        //
        DAD_DragLeave();
        hres = E_FAIL;
    }

    CIDLDropTarget_DragLeave(pdropt);

    return hres;
}

STDAPI CDrives_InvokeCommand(HWND hwnd, WPARAM wParam)
{
    HRESULT hres = S_OK;
    switch(wParam)
    {
    case FSIDM_SORTBYNAME:
        ShellFolderView_ReArrange(hwnd, 0);
        break;

    case FSIDM_SORTBYTYPE:
        ShellFolderView_ReArrange(hwnd, 1);
        break;

    case FSIDM_SORTBYSIZE:
        ShellFolderView_ReArrange(hwnd, 2);
        break;

    case FSIDM_SORTBYDATE:
        ShellFolderView_ReArrange(hwnd, 3);
        break;

    case FSIDM_PROPERTIESBG:
        SHRunControlPanel(TEXT("SYSDM.CPL"), hwnd);
        break;

    default:
        // This is one of view menu items, use the default code.
        hres = S_FALSE;
        break;
    }
    return hres;
}

STDAPI CDrives_GetHelpText(UINT offset, BOOL bWide, LPARAM lParam, UINT cch)
{
    UINT idRes = IDS_MH_FSIDM_FIRST + offset;
    if (idRes == (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYDATE))
        idRes = IDS_MH_SORTBYFREESPACE;

    if (bWide)
        LoadStringW(HINST_THISDLL, idRes, (LPWSTR)lParam, cch);
    else
        LoadStringA(HINST_THISDLL, idRes, (LPSTR)lParam, cch);

    return S_OK;
}


// context menu for a list of items (drive pidls)

STDAPI CDrives_DFMCallBackBG(IShellFolder *psf, HWND hwnd,
                             IDataObject *pdtobj, UINT uMsg, 
                             WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch(uMsg) {
    case DFM_MERGECONTEXTMENU:
        {
            LPQCMINFO pqcm = (LPQCMINFO)lParam;
            if (!(wParam & CMF_DVFILE)) //In the case of the file menu
            {
                CDefFolderMenu_MergeMenu(HINST_THISDLL, 
                    POPUP_DRIVES_BACKGROUND, POPUP_DRIVES_POPUPMERGE, pqcm);
            }
        }
        break;

    case DFM_GETHELPTEXT:
    case DFM_GETHELPTEXTW:
        hres = CDrives_GetHelpText(LOWORD(wParam), uMsg == DFM_GETHELPTEXTW, lParam, HIWORD(wParam));
        break;

    case DFM_INVOKECOMMAND:
        hres = CDrives_InvokeCommand(hwnd, wParam);
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}

STDAPI_(DWORD) _CDrives_PropertiesThread(PROPSTUFF *pps)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);

#ifdef WINNT
    BOOL bMountedDriveInfo = FALSE;

    // Were we able to get data for a HIDA?
    if (!pida)
    {
        // No, pida is first choice, but if not present check for mounteddrive info
        FORMATETC fmte;

        fmte.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
        fmte.ptd = NULL;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.lindex = -1;
        fmte.tymed = TYMED_HGLOBAL;

        // Is data available for the MountedVolume format?
        if (SUCCEEDED(pps->pdtobj->GetData(&fmte, &medium)))
            // Yes
            bMountedDriveInfo = TRUE;
    }

    // Do we have data for a HIDA or a mountedvolume?
    if (pida || bMountedDriveInfo)
#else //WINNT
    if (pida)
#endif //WINNT
    {
        // Yes
        HKEY ahkeys[3] = { NULL, NULL, NULL };
#ifdef WINNT
        TCHAR szCaption[MAX_PATH];
#endif
        LPTSTR pszCaption = NULL;

        if (pida)
        {
            pszCaption = SHGetCaption(medium.hGlobal);
        }
#ifdef WINNT
        else
        {
            TCHAR szMountPoint[MAX_PATH];
            TCHAR szVolumeGUID[MAX_PATH];

            DragQueryFile((HDROP)medium.hGlobal, 0, szMountPoint, ARRAYSIZE(szMountPoint));
            
            GetVolumeNameForVolumeMountPoint(szMountPoint, szVolumeGUID, ARRAYSIZE(szVolumeGUID));
            szCaption[0] = TEXT('\0');
            GetVolumeInformation(szVolumeGUID, szCaption, ARRAYSIZE(szCaption), NULL, NULL, NULL, NULL, 0);

            if (!(*szCaption))
                LoadString(HINST_THISDLL, IDS_UNLABELEDVOLUME, szCaption, ARRAYSIZE(szCaption));        

            PathRemoveBackslash(szMountPoint);

            // Fix 330388
            // If the szMountPoint is not a valid local path, do not
            // display it in the properties dialog title:
            if (-1 != PathGetDriveNumber(szMountPoint))
            {
                int nCaptionLength = lstrlen(szCaption) ;
                wnsprintf(szCaption + nCaptionLength, ARRAYSIZE(szCaption) - nCaptionLength, TEXT(" (%s)"), szMountPoint);
            }
            				
            pszCaption = szCaption;
        }
#endif        
        // Get the hkeyProgID and hkeyBaseProgID from the first item.
        CDrives_GetKeys(NULL, ahkeys);

        SHOpenPropSheet(pszCaption, ahkeys, ARRAYSIZE(ahkeys),
                        &CLSID_ShellDrvDefExt, pps->pdtobj, NULL, pps->pStartPage);

        SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));

        if (pida && pszCaption)
            SHFree(pszCaption);

        if (pida)
            HIDA_ReleaseStgMedium(pida, &medium);
        else
            ReleaseStgMedium(&medium);

    }
    else
    {
        TraceMsg(DM_TRACE, "no HIDA in data obj nor Mounted drive info");
    }
    return 0;
}

//
// To be called back from within CDefFolderMenu
//
STDAPI CDrives_DFMCallBack(IShellFolder *psf, HWND hwnd,
                           IDataObject *pdtobj, UINT uMsg, 
                           WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (pdtobj)
        {
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

            // Check if only file system objects are selected.
            if (pdtobj->QueryGetData(&fmte) == NOERROR)
            {
                #define pqcm ((LPQCMINFO)lParam)

                STGMEDIUM medium;
                // Yes, only file system objects are selected.
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                LPIDDRIVE pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, 0);
                int iDrive = DRIVEID(pidd->cName);
                UINT idCmdBase = pqcm->idCmdFirst;   // store it away

                BOOL fIsEjectable = FALSE;

                CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DRIVES_ITEM, 0, pqcm);

                if (pidd->bFlags != SHID_COMPUTER_NETDRIVE ||
                    SHRestricted( REST_NONETCONNECTDISCONNECT ))
                    DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_DISCONNECT, MF_BYCOMMAND);

                if ((pida->cidl != 1) ||
                    (pidd->bFlags != SHID_COMPUTER_REMOVABLE &&
                     pidd->bFlags != SHID_COMPUTER_FIXED &&
                     pidd->bFlags != SHID_COMPUTER_DRIVE525 &&
                     pidd->bFlags != SHID_COMPUTER_DRIVE35))
                {
                    // Don't even try to format more than one disk
                    // Or a net drive, or a CD-ROM, or a RAM drive ...
                    // Note we are going to show the Format command on the
                    // boot drive, Windows drive, System drive, compressed
                    // drives, etc.  An appropriate error should be shown
                    // after the user chooses this command
                    DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_FORMAT, MF_BYCOMMAND);
                }

                if (pMtPt)
                {
                    fIsEjectable = pMtPt->IsEjectable(TRUE);
                    pMtPt->Release();
                }
                if ((pida->cidl != 1) || (iDrive < 0) || !fIsEjectable)
                    DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_EJECT, MF_BYCOMMAND);

                HIDA_ReleaseStgMedium(pida, &medium);    

                #undef pqcm
            }
        }
        // Note that we always return NOERROR from this function so that
        // default processing of menu items will occur
        ASSERT(hres == NOERROR);
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_MAPCOMMANDNAME:
        if (lstrcmpi((LPCTSTR)lParam, TEXT("eject")) == 0)
            *(int *)wParam = FSIDM_EJECT;
        else if (lstrcmpi((LPCTSTR)lParam, TEXT("format")) == 0)
            *(int *)wParam = FSIDM_FORMAT;
        else
            hres = E_FAIL;  // command not found
        break;

    case DFM_INVOKECOMMAND:
        switch(wParam)
        {
        case DFM_CMD_PROPERTIES:
            // lParam contains the page name to open
            SHLaunchPropSheet((LPTHREAD_START_ROUTINE)_CDrives_PropertiesThread, pdtobj,
                (LPCTSTR)lParam, NULL, NULL);
            break;

        case FSIDM_EJECT:
        case FSIDM_FORMAT:
        {
            LPIDDRIVE pidd;
            UINT iDrive;
            STGMEDIUM medium;

            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

            ASSERT(HIDA_GetCount(medium.hGlobal) == 1);

            pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, 0);

            iDrive = DRIVEID(pidd->cName);

            ASSERT(iDrive >= 0);

            switch (wParam) {
            case FSIDM_FORMAT:
                SHFormatDriveAsync(hwnd, iDrive, SHFMT_ID_DEFAULT, 0);
                break;

            case FSIDM_EJECT:
                {
                    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

                    if (pMtPt)
                    {
                        pMtPt->Eject();
                        pMtPt->Release();
                    }
                    break;
                }
            }

            HIDA_ReleaseStgMedium(pida, &medium);
            break;
        }

        case FSIDM_DISCONNECT:

            if (pdtobj)
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                if (pida)
                {
                    DISCDLGSTRUCT discd = {
                        SIZEOF(DISCDLGSTRUCT),  // cbStructure
                        hwnd,               // hwndOwner
                        NULL,                   // lpLocalName
                        NULL,                   // lpRemoteName
                        DISC_UPDATE_PROFILE // dwFlags
                    };
                    UINT iidl;
                    for (iidl = 0 ; iidl < pida->cidl ; iidl++)
                    {
                        LPIDDRIVE pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, iidl);
                        if (pidd->bFlags == SHID_COMPUTER_NETDRIVE)
                        {
                            BOOL fUnavailable = IsUnavailableNetDrive(DRIVEID(pidd->cName));

                            TCHAR szPath[4], szDrive[4];

                            SHAnsiToTChar(pidd->cName, szPath,  ARRAYSIZE(szPath));
                            SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));
#ifdef WINNT
                            szDrive[2] = 0; // remove slash
#endif // WINNT
                            discd.lpLocalName = szDrive;

                            if (SHWNetDisconnectDialog1(&discd) == WN_SUCCESS)
                            {
                                // If it is a unavailable drive we get no
                                // file system notification and as such
                                // the drive will not disappear, so lets
                                // set up to do it ourself...
                                if (fUnavailable)
                                    SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, szPath, NULL);
                            }
                        }
                    }

                    // flush them altogether
                    SHChangeNotifyHandleEvents();
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
            }
            break;

        case FSIDM_CONNECT_PRN:
            SHNetConnectionDialog(hwnd, NULL, RESOURCETYPE_PRINT);
            break;

        case FSIDM_DISCONNECT_PRN:
            WNetDisconnectDialog(hwnd, RESOURCETYPE_PRINT);
            break;

        default:
            // This is one of view menu items, use the default code.
            hres = S_FALSE;
            break;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}

#define REGSTR_LASTUNCHASH  TEXT("LastUNCHash")

STDMETHODIMP GetUNCPathHash(HKEY hkDrives, LPCTSTR pszUNCPath, LPTSTR pszUNCPathHash, int cchUNCPathHash)
{
    HRESULT hr = E_FAIL;
    DWORD dwLastUNCHash = 0;
    SHQueryValueEx(hkDrives, REGSTR_LASTUNCHASH, NULL, NULL, (LPBYTE)&dwLastUNCHash, NULL);
    dwLastUNCHash++;
    if (RegSetValueEx(hkDrives, REGSTR_LASTUNCHASH, 0, REG_DWORD, (LPCBYTE)&dwLastUNCHash, SIZEOF(dwLastUNCHash)) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }
    wnsprintf(pszUNCPathHash, cchUNCPathHash, TEXT("%lu"), (ULONG)dwLastUNCHash);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: _DrvPrshtUpdateSpaceValues
//
// DESCRIPTION:
//    Updates the Used space, Free space and Capacity values on the drive
//    general property page..
//
// NOTE:
//    This function was separated from _DrvPrshtInit because drive space values
//    must be updated after a compression/uncompression operation as well as
//    during dialog initialization.
///////////////////////////////////////////////////////////////////////////////
void _DrvPrshtUpdateSpaceValues(DRIVEPROPSHEETPAGE *pdpsp)
{
   BOOL fResult  = FALSE;
   _int64 qwTot  = 0;
   _int64 qwFree = 0;
   ULARGE_INTEGER qwFreeUser, qwTotal, qwTotalFree;
   TCHAR szTemp[80];
   TCHAR szFormat[30];

   fResult = SHGetDiskFreeSpaceEx(pdpsp->szDrive, &qwFreeUser, &qwTotal, &qwTotalFree);

   if (fResult)
   {
      qwTot = qwTotal.QuadPart;
      qwFree = qwFreeUser.QuadPart;

      // Save away to use when drawing pie, could probably clean up differently..
      pdpsp->qwTot = qwTotal.QuadPart;
      pdpsp->qwFree = qwFreeUser.QuadPart;
   }
   else
   {
      CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);

      // If network drive show the type as unavalable if
      // the drive fails to get free space...
#ifdef DEBUG
      DWORD dwError = GetLastError();
      // see if we should special case the different bugs that
      // might come here.  Currently when it is unshared it looks
      // like ERROR_INVALID_DRIVE is returned...
#endif

      // Clear these for use when drawing the pie.
      pdpsp->qwTot = pdpsp->qwFree = 0;

      // relies on expression evaluation order
      if (!pMtPt || pMtPt->IsUnavailableNetDrive())
      {
          LoadString(HINST_THISDLL, IDS_DRIVES_NETUNAVAIL, szTemp, ARRAYSIZE(szTemp));
          SetDlgItemText(pdpsp->hDlg, IDC_DRV_TYPE, szTemp);
      }

      if (pMtPt)
          pMtPt->Release();

      qwTot  = 0;
      qwFree = 0;
   }

   if (LoadString(HINST_THISDLL, IDS_BYTES, szFormat, ARRAYSIZE(szFormat)))
   {
       TCHAR szTemp2[30];

      // NT must be able to display 64-bit numbers; at least as much
      // as is realistic.  We've made the decision
      // that volumes up to 100 Terrabytes will display the byte value
      // and the short-format value.  Volumes of greater size will display
      // "---" in the byte field and the short-format value.  Note that the
      // short format is always displayed.
      //
      const _int64 MaxDisplayNumber = 99999999999999; // 100TB - 1.
      if (qwTot-qwFree <= MaxDisplayNumber)
      {
          wsprintf(szTemp, szFormat, AddCommas64(qwTot - qwFree, szTemp2));
          SetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDBYTES, szTemp);
      }

      if (qwFree <= MaxDisplayNumber)
      {
          wsprintf(szTemp, szFormat, AddCommas64(qwFree, szTemp2));
          SetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEBYTES, szTemp);
      }

      if (qwTot <= MaxDisplayNumber)
      {
          wsprintf(szTemp, szFormat, AddCommas64(qwTot, szTemp2));
          SetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTBYTES, szTemp);
      }
   }

   ShortSizeFormat64(qwTot-qwFree, szTemp);
   SetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDMB, szTemp);

   ShortSizeFormat64(qwFree, szTemp);
   SetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEMB, szTemp);

   ShortSizeFormat64(qwTot, szTemp);
   SetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTMB, szTemp);
}

void _DrvPrshtInit(DRIVEPROPSHEETPAGE * pdpsp)
{
    HWND hCtl;
    TCHAR szFormat[30];
    TCHAR szTemp[80];
    TCHAR szRoot[MAX_PATH];  //now can contain a folder name as a mounting point
    TCHAR szLabel[MAX_LABEL_NTFS + 1];
    HCURSOR hcOld;
    HDC hDC;
    SIZE size;
    HICON hiconT;
    HRESULT hres = E_FAIL;
    UINT uIcon = 0;
    HICON hIcon = NULL;
    CMountPoint* pMtPt = NULL;
    TCHAR szModule[MAX_PATH];

    hcOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hDC = GetDC(pdpsp->hDlg);
    GetTextExtentPoint(hDC, TEXT("W"), 1, &size);
    pdpsp->dwPieShadowHgt = size.cy*2/3;
    ReleaseDC(pdpsp->hDlg, hDC);

    if (pdpsp->fMountedDrive)
        lstrcpyn(szRoot, pdpsp->szDrive, ARRAYSIZE(szRoot));
    else
        PathBuildRoot(szRoot, pdpsp->iDrive);

    CMountPoint::InvalidateMountPoint(szRoot, NULL, MTPT_INV_MEDIA);

    pMtPt = CMountPoint::GetMountPoint(szRoot);

    if (pMtPt)
        uIcon = pMtPt->GetIcon(szModule, ARRAYSIZE(szModule));

    if (uIcon)
    {
        HIMAGELIST hIL = NULL;

        Shell_GetImageLists(&hIL, NULL);

        if (hIL)
        {
            int iIndex = Shell_GetCachedImageIndex(szModule[0] ? szModule : c_szShell32Dll,
                uIcon, 0);
            hIcon = ImageList_ExtractIcon(g_hinst, hIL, iIndex);
        }
    }

    if (hIcon)
    {
        hiconT = Static_SetIcon(GetDlgItem(pdpsp->hDlg, IDC_DRV_ICON), hIcon);
        if (hiconT)
            DestroyIcon(hiconT);
    }

    // check to see if the drive is initially compressed or content indexed
    SetInitialDriveAttributes(pdpsp);

    hCtl = GetDlgItem(pdpsp->hDlg, IDC_DRV_LABEL);
    {
        if (pMtPt)
            hres = pMtPt->GetLabel(szLabel, ARRAYSIZE(szLabel), MTPT_LABEL_NOFANCY);

        if (SUCCEEDED(hres))
        {
            TCHAR szFileSystem[64];
            UINT cchLabel;

            cchLabel = MAX_LABEL_FAT;

            if (pMtPt->GetFileSystemName(szFileSystem, ARRAYSIZE(szFileSystem)) && 
                *szFileSystem)
            {
                if (pMtPt->IsNTFS())
                    cchLabel = MAX_LABEL_NTFS;
            }
#ifdef WINNT    // IDS_FMT_MEDIA0 is defined only for NT
            else
            {
                LoadString(HINST_THISDLL, IDS_FMT_MEDIA0, szFileSystem, ARRAYSIZE(szFileSystem));
            }
#endif
            Edit_LimitText(hCtl, cchLabel);

            SetDlgItemText(pdpsp->hDlg, IDC_DRV_FS, szFileSystem);
        }
        else
        {
            Edit_SetReadOnly(hCtl, TRUE);
        }
        SetWindowText(hCtl, szLabel);
    }

    Edit_SetModify(hCtl, FALSE);

    if (pMtPt)
    {
        if (pMtPt->GetDRIVEType(FALSE) == DRIVE_CDROM)
            Edit_SetReadOnly(hCtl, TRUE);

        pMtPt->GetTypeString(szTemp, ARRAYSIZE(szTemp));
    }

    SetDlgItemText(pdpsp->hDlg, IDC_DRV_TYPE, szTemp);

    _DrvPrshtUpdateSpaceValues(pdpsp);

    if (pdpsp->fMountedDrive)
    {
        LoadString(HINST_THISDLL, IDS_VOLUMELABEL, szFormat, ARRAYSIZE(szFormat));
        wsprintf(szTemp, szFormat, szLabel);
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_LETTER, szTemp);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_DRIVELETTER, szFormat, ARRAYSIZE(szFormat));
        wsprintf(szTemp, szFormat, pdpsp->iDrive + TEXT('A'));
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_LETTER, szTemp);
    }

    //
    // Set the inital state of the compression / content index checkboxes
    //
    if (!pdpsp->fIsCompressionAvailable)
    {
        // file-based compression is not supported
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDD_COMPRESS));
    }
    else
    {
        CheckDlgButton(pdpsp->hDlg, IDD_COMPRESS, pdpsp->asInitial.fCompress);
    }

    if (!pdpsp->fIsIndexAvailable)
    {
        // content index is only supported on NTFS 5 volumes
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDD_INDEX));
    }
    else
    {
        CheckDlgButton(pdpsp->hDlg, IDD_INDEX, pdpsp->asInitial.fIndex);
    }

    // if we have a cleanup path in the registry, 
    // turn on the Disk Cleanup button
    // BUGBUG: Put it off for mounted volume until it can handle it
    // BUGBUG: part II, the resource does not hide the control so this is useless
    if (!pdpsp->fMountedDrive && GetDiskCleanupPath(NULL, 0) && IsBitBucketableDrive(pdpsp->iDrive))
    {
        ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), SW_SHOW);
        EnableWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), TRUE);
    }
    else
    {
        ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), SW_HIDE);
        EnableWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), FALSE);
    }

    SetCursor(hcOld);

    if (pMtPt)
        pMtPt->Release();
}

const COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),      // Blue
    RGB(255,   0, 255),      // Red-Blue
    RGB(  0,   0, 128),      // 1/2 Blue
    RGB(128,   0, 128),      // 1/2 Red-Blue
};

STDAPI Draw3dPie(HDC hdc, RECT *prc, DWORD dwPer1000, const COLORREF *lpColors);
        
void DrawColorRect(HDC hdc, COLORREF crDraw, const RECT *prc)
{
    HBRUSH hbDraw = CreateSolidBrush(crDraw);
    if (hbDraw)
    {
        HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbDraw);
        if (hbOld)
        {
            PatBlt(hdc, prc->left, prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                PATCOPY);
            
            SelectObject(hdc, hbOld);
        }
        
        DeleteObject(hbDraw);
    }
}

void _DrvPrshtDrawItem(DRIVEPROPSHEETPAGE *pdpsp, const DRAWITEMSTRUCT * lpdi)
{
    switch (lpdi->CtlID)
    {
    case IDC_DRV_PIE:
        {
            DWORD dwPctX10 = pdpsp->qwTot ?
                (DWORD)((__int64)1000 * (pdpsp->qwTot - pdpsp->qwFree) / pdpsp->qwTot) : 
                1000;
#if 1
            DrawPie(lpdi->hDC, &lpdi->rcItem,
                dwPctX10, pdpsp->qwFree==0 || pdpsp->qwFree==pdpsp->qwTot,
                pdpsp->dwPieShadowHgt, c_crPieColors);
#else
            {
                RECT rcTemp = lpdi->rcItem;
                Draw3dPie(lpdi->hDC, &rcTemp, dwPctX10, c_crPieColors);
            }
#endif
        }
        break;
        
    case IDC_DRV_USEDCOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_USEDCOLOR], &lpdi->rcItem);
        break;
        
    case IDC_DRV_FREECOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_FREECOLOR], &lpdi->rcItem);
        break;
        
    default:
        break;
    }
}

BOOL_PTR CALLBACK DriveAttribsDlgProc(HWND hDlgRecurse, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE* pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlgRecurse, DWLP_USER);
    
    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            TCHAR szTemp[MAX_PATH];
            TCHAR szAttribsToApply[MAX_PATH];
            TCHAR szDriveText[MAX_PATH];
            TCHAR szFormatString[MAX_PATH];
            TCHAR szDlgText[MAX_PATH];
            int iLength;

            SetWindowLongPtr(hDlgRecurse, DWLP_USER, lParam);
            pdpsp = (DRIVEPROPSHEETPAGE *)lParam;

            // set the initial state of the radio button
            CheckDlgButton(hDlgRecurse, IDD_NOTRECURSIVE, TRUE);
            
            szAttribsToApply[0] = TEXT('\0');

            // set the IDD_ATTRIBSTOAPPLY based on what attribs we are applying
            if (pdpsp->asInitial.fIndex != pdpsp->asCurrent.fIndex)
            {
                if (pdpsp->asCurrent.fIndex)
                    LoadString(HINST_THISDLL, IDS_INDEX, szTemp, ARRAYSIZE(szTemp)); 
                else
                    LoadString(HINST_THISDLL, IDS_DISABLEINDEX, szTemp, ARRAYSIZE(szTemp)); 

                lstrcatn(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
            }

            if (pdpsp->asInitial.fCompress != pdpsp->asCurrent.fCompress)
            {
                if (pdpsp->asCurrent.fCompress)
                    LoadString(HINST_THISDLL, IDS_COMPRESS, szTemp, ARRAYSIZE(szTemp)); 
                else
                    LoadString(HINST_THISDLL, IDS_UNCOMPRESS, szTemp, ARRAYSIZE(szTemp)); 

                lstrcatn(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
            }

            // remove the trailing ", "
            iLength = lstrlen(szAttribsToApply);
            ASSERT(iLength >= 3);
            szAttribsToApply[iLength - 2] = TEXT('\0');

            SetDlgItemText(hDlgRecurse, IDD_ATTRIBSTOAPPLY, szAttribsToApply);

            // this dialog was only designed for nice short paths like "c:\" not "\\?\Volume{GUID}\" paths
            if (lstrlen(pdpsp->szDrive) > 3)
            {
                // get the lame-ass default string
                LoadString(HINST_THISDLL, IDS_THISVOLUME, szDriveText, ARRAYSIZE(szDriveText));
            }
            else
            {
                // Create the string "C:\"
                lstrcpyn(szDriveText, pdpsp->szDrive, ARRAYSIZE(szDriveText));
                PathAddBackslash(szDriveText);

                // sanity check; this better be a drive root!
                ASSERT(PathIsRoot(szDriveText));
            }
            
            // set the IDD_RECURSIVE_TXT text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szDlgText);

            // set the IDD_NOTRECURSIVE raido button text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szDlgText);

            // set the IDD_RECURSIVE raido button text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szDlgText);

            return TRUE;
        }

        case WM_COMMAND:
        {
            WORD wID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (wID)
            {
                case IDOK:
                    pdpsp->fRecursive = (IsDlgButtonChecked(hDlgRecurse, IDD_RECURSIVE) == BST_CHECKED);
                    // fall through
                case IDCANCEL:
                    EndDialog(hDlgRecurse, (wID == IDCANCEL) ? FALSE : TRUE);
                    break;
            }
        }

        default:
            return FALSE;
    }
}


BOOL _DrvPrshtApply(DRIVEPROPSHEETPAGE* pdpsp)
{
    BOOL bFctRet;
    HWND hCtl;

    // take care of compression / content indexing first
    pdpsp->asCurrent.fCompress = (IsDlgButtonChecked(pdpsp->hDlg, IDD_COMPRESS) == BST_CHECKED);
    pdpsp->asCurrent.fIndex = (IsDlgButtonChecked(pdpsp->hDlg, IDD_INDEX) == BST_CHECKED);

    // check to see if something has changed before applying attribs
    if (memcmp(&pdpsp->asInitial, &pdpsp->asCurrent, SIZEOF(pdpsp->asInitial)) != 0)
    {
        BOOL_PTR bRet = TRUE;

        // the user toggled the attributes, so ask them if they want to recurse
        bRet = DialogBoxParam(HINST_THISDLL, 
                              MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                              pdpsp->hDlg,
                              DriveAttribsDlgProc,
                              (LPARAM)pdpsp);
        if (bRet)
        {
            FILEPROPSHEETPAGE fpsp = {0};

            // we cook up a fpsp and call ApplySingleFileAttributes instead of 
            // rewriting the apply attributes code
#ifdef WINNT
            if (pdpsp->fMountedDrive)
            {
                GetVolumeNameForVolumeMountPoint(pdpsp->szDrive, fpsp.szPath, ARRAYSIZE(fpsp.szPath));
            }
            else
#endif
            {
                lstrcpyn(fpsp.szPath, pdpsp->szDrive, ARRAYSIZE(fpsp.szPath));
                PathAddBackslash(fpsp.szPath);
            }

            fpsp.hDlg = pdpsp->hDlg;
            fpsp.asInitial = pdpsp->asInitial;
            fpsp.asCurrent = pdpsp->asCurrent;
            fpsp.fIsCompressionAvailable = pdpsp->fIsCompressionAvailable;
            fpsp.fIsIndexAvailable = pdpsp->fIsIndexAvailable;
            fpsp.ulTotalNumberOfBytes.QuadPart = pdpsp->qwTot - pdpsp->qwFree; // for progress calculations
            fpsp.fRecursive = pdpsp->fRecursive;
            fpsp.fIsDirectory = TRUE;
            
            bRet = ApplySingleFileAttributes(&fpsp);

            // update the free/used space after applying attribs because something could
            // have changed (eg compression)
            _DrvPrshtUpdateSpaceValues(pdpsp);

            // update the initial attributes to reflect the ones we just applied, regardless
            // if the operation was sucessful or not. If they hit cancel, then the volume
            // root was most likely still changed so we need to update.
            pdpsp->asInitial = pdpsp->asCurrent;
        }

        if (!bRet)
        {
            // the user hit cancel somewhere
            return FALSE;
        }
    }

    hCtl = GetDlgItem(pdpsp->hDlg, IDC_DRV_LABEL);

    bFctRet = TRUE;

    if (Edit_GetModify(hCtl))
    {
        TCHAR szRoot[MAX_PATH];
        HRESULT hres = E_FAIL;
        TCHAR szLabel[MAX_LABEL_NTFS + 1];
        CMountPoint* pMtPt = NULL;

        GetWindowText(hCtl, szLabel, ARRAYSIZE(szLabel));

        if (pdpsp->fMountedDrive)
            lstrcpyn(szRoot, pdpsp->szDrive, ARRAYSIZE(szRoot));
        else
            PathBuildRoot(szRoot, pdpsp->iDrive);

        pMtPt = CMountPoint::GetMountPoint(szRoot);

        if (pMtPt)
        {
            hres = pMtPt->SetLabel(szLabel);
            pMtPt->Release();
        }
        bFctRet = SUCCEEDED(hres);
    }

    HDPA hdpaInvalidPath = DPA_Create(2);

    if (pdpsp->fMountedDrive)
        CMountPoint::InvalidateMountPoint(pdpsp->szDrive, hdpaInvalidPath, MTPT_INV_MEDIA);
    else
        CMountPoint::InvalidateMountPoint(pdpsp->iDrive, hdpaInvalidPath, MTPT_INV_MEDIA);

    if (hdpaInvalidPath)
    {
        int n = DPA_GetPtrCount(hdpaInvalidPath);

        for (int i = 0; i < n; ++i)
        {
            LPTSTR pszPath = (LPTSTR)DPA_GetPtr(hdpaInvalidPath, i);

            if (pszPath)
            {
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pszPath, NULL);
                LocalFree((HLOCAL)pszPath);
            }
        }
        
        DPA_Destroy(hdpaInvalidPath);
    }

    return bFctRet;
}

const static DWORD aDrvPrshtHelpIDs[] = {  // Context Help IDs
    IDC_DRV_ICON,          IDH_FCAB_DRV_ICON,
    IDC_DRV_LABEL,         IDH_FCAB_DRV_LABEL,
    IDC_DRV_TYPE_TXT,      IDH_FCAB_DRV_TYPE,
    IDC_DRV_TYPE,          IDH_FCAB_DRV_TYPE,
#ifdef WINNT
    IDC_DRV_FS_TXT,        IDH_FCAB_DRV_FS,
    IDC_DRV_FS,            IDH_FCAB_DRV_FS,
#endif
    IDC_DRV_USEDCOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREECOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_TOTSEP,        NO_HELP,
    IDC_DRV_TOTBYTES_TXT,  IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTBYTES,      IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTMB,         IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_PIE,           IDH_FCAB_DRV_PIE,
    IDC_DRV_LETTER,        IDH_FCAB_DRV_LETTER,
    IDC_DRV_CLEANUP,       IDH_FCAB_DRV_CLEANUP,
    IDD_COMPRESS,          IDH_FCAB_DRV_COMPRESS,
    IDD_INDEX,             IDH_FCAB_DRV_INDEX,
    0, 0
};

//
// Descriptions:
//   This is the dialog procedure for the "general" page of a property sheet.
//
BOOL_PTR CALLBACK _DrvGeneralDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) {
    case WM_INITDIALOG:
        // REVIEW, we should store more state info here, for example
        // the hIcon being displayed and the FILEINFO pointer, not just
        // the file name ptr
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pdpsp = (DRIVEPROPSHEETPAGE *)lParam;
        pdpsp->hDlg = hDlg;

        _DrvPrshtInit(pdpsp);
        break;

    case WM_DESTROY:
        {
        HICON hIcon = Static_GetIcon(GetDlgItem(hDlg, IDC_DRV_ICON), NULL);
        if (hIcon)
            DestroyIcon(hIcon);
        break;
        }

    case WM_DRAWITEM:
        _DrvPrshtDrawItem(pdpsp, (DRAWITEMSTRUCT *)lParam);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aDrvPrshtHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aDrvPrshtHelpIDs);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_DRV_LABEL:
                if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE)
                    break;
                // else, fall through
            case IDD_COMPRESS:
            case IDD_INDEX:
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                break;

                break;

            // handle disk cleanup button      
            case IDC_DRV_CLEANUP:
                LaunchDiskCleanup(hDlg, pdpsp->iDrive);
                break;

            default:
                return TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            if (!_DrvPrshtApply(pdpsp))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            }
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

int _GetDaysDelta(HKEY hkRoot, LPCTSTR pszSubKey, LPCTSTR pszValName)
{
    int nDays = -1;
    SYSTEMTIME lastst, nowst;
    DWORD dwType, dwSize;

    if (RegOpenKey(hkRoot, pszSubKey, &hkRoot) != ERROR_SUCCESS)
    {
        return -1;
    }

    dwSize = SIZEOF(lastst);
    if (SHQueryValueEx(hkRoot, pszValName, 0, &dwType, (LPBYTE)&lastst, &dwSize) == ERROR_SUCCESS
            && dwType == REG_BINARY && dwSize == SIZEOF(lastst))
    {
        FILETIME nowftU, nowft, lastft;
        LARGE_INTEGER liLast, liNow;

        GetSystemTime(&nowst);

        SystemTimeToFileTime(&nowst, &nowftU);
        FileTimeToLocalFileTime(&nowftU, &nowft);
        SystemTimeToFileTime(&lastst, &lastft);

        liLast.LowPart = lastft.dwLowDateTime;
        liLast.HighPart = lastft.dwHighDateTime;
  
        liNow.LowPart = nowft.dwLowDateTime;
        liNow.HighPart = nowft.dwHighDateTime;

        liNow.QuadPart -= liLast.QuadPart;

        liNow.QuadPart = liNow.QuadPart / 10000000 / 60 / 60 / 24;

        // Cast to int: if the number of days does not fit in an "int", then probably
        // nobody will be around to complain.  If a bug is opened, assign to me :)
        // Put "Y4G" in the title. (stephstm)
        nDays = (int)liNow.QuadPart;
    }

    // Note this is not the root passed in
    RegCloseKey(hkRoot);

    return nDays;
}


void CDrives_ShowDays(DRIVEPROPSHEETPAGE * pdpsp, UINT idCtl,
        LPCTSTR pszRegKey, UINT idsDays, UINT idsUnknown)
{
    TCHAR szFormat[256], szTitle[256];
    int nDays;

    szTitle[0] = TEXT('A') + pdpsp->iDrive;
    szTitle[1] = 0;

    if (RealDriveType(pdpsp->iDrive, FALSE /* fOKToHitNet */) == DRIVE_FIXED
            && (nDays = _GetDaysDelta(HKEY_LOCAL_MACHINE, pszRegKey, szTitle)) >= 0)
    {
        LoadString(HINST_THISDLL, idsDays, szFormat, ARRAYSIZE(szFormat));
        wsprintf(szTitle, szFormat, nDays);
    }
    else
    {
        LoadString(HINST_THISDLL, idsUnknown, szTitle, ARRAYSIZE(szTitle));
    }
    SetDlgItemText(pdpsp->hDlg, idCtl, szTitle);
}


void _DiskToolsPrshtInit(DRIVEPROPSHEETPAGE * pdpsp)
{
#ifdef WINNT
    HKEY hkExp;
    BOOL bFoundFmt = FALSE;
    
    //
    // Several things separate NT from Win95 here.
    // 1. NT doesn't currently provide a defragmentation utility.
    //    If there isn't a 3rd party one identified in the registry,
    //    we disable the Defrag button and display an appropriate message.
    // 2. The NT Check Disk and Backup utilities don't write the
    //    "last time run" information into the registry.  Therefore
    //    we can't display a meaningful "last time..." message.
    //    We replace the "last time..." message with generic feature
    //    description. i.e. "This option will..."
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, &hkExp) == ERROR_SUCCESS)
    {
        TCHAR szFmt[MAX_PATH + 20];
        LONG cbLen = SIZEOF(szFmt);
        
        if ((SHRegQueryValue(hkExp, c_szOptReg, szFmt, &cbLen) == ERROR_SUCCESS) &&
            szFmt[0])
        {
            bFoundFmt = TRUE;
        }
        RegCloseKey(hkExp);
    }
    
    //
    // If no defrag utility is installed, replace the default defrag text with
    // the "No defrag installed" message.  Also grey out the "defrag now" button.
    //
    if (!bFoundFmt)
    {
        TCHAR szMessage[50];  // WARNING:  IDS_DRIVES_NOOPTINSTALLED is currently 47
        //           characters long.  Resize this buffer if
        //           the string resource is lengthened.
        
        LoadString(HINST_THISDLL, IDS_DRIVES_NOOPTINSTALLED, szMessage, ARRAYSIZE(szMessage));
        SetDlgItemText(pdpsp->hDlg, IDC_DISKTOOLS_OPTDAYS, szMessage);
        Button_Enable(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_OPTNOW), FALSE);
    }

#else
    CDrives_ShowDays(pdpsp, IDC_DISKTOOLS_CHKDAYS, REGSTR_PATH_LASTCHECK,
            IDS_DRIVES_LASTCHECKDAYS, IDS_DRIVES_LASTCHECKUNK);
    CDrives_ShowDays(pdpsp, IDC_DISKTOOLS_BKPDAYS, REGSTR_PATH_LASTBACKUP,
            IDS_DRIVES_LASTBACKUPDAYS, IDS_DRIVES_LASTBACKUPUNK);
    CDrives_ShowDays(pdpsp, IDC_DISKTOOLS_OPTDAYS, REGSTR_PATH_LASTOPTIMIZE,
            IDS_DRIVES_LASTOPTIMIZEDAYS, IDS_DRIVES_LASTOPTIMIZEUNK);
#endif
}


const static DWORD aDiskToolsHelpIDs[] = {  // Context Help IDs
    IDC_DISKTOOLS_TRLIGHT,    IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_CHKDAYS,    IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_CHKNOW,     IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_BKPTXT,     IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_BKPDAYS,    IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_BKPNOW,     IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_OPTDAYS,    IDH_FCAB_DISKTOOLS_OPTNOW,
    IDC_DISKTOOLS_OPTNOW,     IDH_FCAB_DISKTOOLS_OPTNOW,

    0, 0
};

BOOL _DiskToolsCommand(DRIVEPROPSHEETPAGE * pdpsp, WPARAM wParam, LPARAM lParam)
{
    // Add 20 for extra formatting
    TCHAR szFmt[MAX_PATH + 20];
    TCHAR szCmd[MAX_PATH + 20];
    LPCTSTR pszRegName, pszDefFmt;
    HKEY hkExp;
    BOOL bFoundFmt;
    int nErrMsg = 0;

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case IDC_DISKTOOLS_CHKNOW:
#ifdef WINNT
        if (pdpsp->fMountedDrive)
        {
            SHChkDskDriveEx(pdpsp->hDlg, pdpsp->szDrive);
        }
        else
        {
            WCHAR szPath[4];
            lstrcpyW(szPath, TEXT("A:\\"));
            ASSERT((pdpsp->iDrive < 26) && (pdpsp->iDrive >= 0));
            szPath[0] += (WCHAR) pdpsp->iDrive;

            SHChkDskDriveEx(pdpsp->hDlg, szPath);
        }

        return FALSE;
#else
        pszRegName = TEXT("MyComputer\\chkdskpath");
        pszDefFmt = TEXT("scandskw.exe %c:");
#endif

        nErrMsg = IDS_NO_DISKCHECK_APP;
        break;

    case IDC_DISKTOOLS_OPTNOW:
        pszRegName = c_szOptReg;
        if (pdpsp->fMountedDrive)
            pszDefFmt = TEXT("defrag.exe");
        else
            pszDefFmt = TEXT("defrag.exe %c:");

        nErrMsg =  IDS_NO_OPTIMISE_APP;
        break;

    case IDC_DISKTOOLS_BKPNOW:
        pszRegName = TEXT("MyComputer\\backuppath");
#ifdef WINNT
        pszDefFmt = TEXT("ntbackup.exe");
#else
        pszDefFmt = TEXT("backup.exe");
#endif
        nErrMsg = IDS_NO_BACKUP_APP;
        break;

    default:
        return FALSE;
    }

    bFoundFmt = FALSE;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, &hkExp) == ERROR_SUCCESS)
    {
        LONG cbLen = SIZEOF(szFmt);
        if (SHRegQueryValue(hkExp, pszRegName, szFmt, &cbLen) == ERROR_SUCCESS)
        {
            bFoundFmt = TRUE;
        }
        RegCloseKey(hkExp);
    }

    if (!bFoundFmt)
        lstrcpy(szFmt, pszDefFmt);

    // Plug in the drive letter in case they want it
    wsprintf(szCmd, szFmt, pdpsp->iDrive + TEXT('A'));

    if (!ShellExecCmdLine(pdpsp->hDlg, szCmd, NULL, SW_SHOWNORMAL,
            NULL, SECL_USEFULLPATHDIR | SECL_NO_UI))
    {
        // Something went wrong - app's probably not installed.
        if (nErrMsg)
        {
            ShellMessageBox(HINST_THISDLL, pdpsp->hDlg,
                    MAKEINTRESOURCE(nErrMsg), NULL,
                    MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        }
        return FALSE;
    }

    return TRUE;
}

//
// Descriptions:
//   This is the dialog procedure for the "Tools" page of a property sheet.
//
BOOL_PTR CALLBACK _DiskToolsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) {
    case WM_INITDIALOG:
        // REVIEW, we should store more state info here, for example
        // the hIcon being displayed and the FILEINFO pointer, not just
        // the file name ptr
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pdpsp = (DRIVEPROPSHEETPAGE *)lParam;
        pdpsp->hDlg = hDlg;

        _DiskToolsPrshtInit(pdpsp);

        break;

    case WM_ACTIVATE:
        if (GET_WM_ACTIVATE_STATE(wParam, lParam)!=WA_INACTIVE && pdpsp)
        {
            _DiskToolsPrshtInit(pdpsp);
        }

        // Let DefDlgProc know we did not handle this
        return FALSE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aDiskToolsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aDiskToolsHelpIDs);
        break;

    case WM_COMMAND:
        return(_DiskToolsCommand(pdpsp, wParam, lParam));

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            return TRUE;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//
// This is the dialog procedure for the "Hardware" page.
//

const GUID c_rgguidDevMgr[] = {
{ 0x4d36e967L, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_DISKDRIVE
{ 0x4d36e980L, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_FLOPPYDISK
{ 0x4d36e965L, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_CDROM
};

BOOL_PTR CALLBACK _DriveHWDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage) {
    case WM_INITDIALOG:
        {
            DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)lParam;

            HWND hwndHW = DeviceCreateHardwarePageEx(hDlg, c_rgguidDevMgr, ARRAYSIZE(c_rgguidDevMgr), HWTAB_LARGELIST);
            if (hwndHW) 
            {
                TCHAR szBuf[MAX_PATH];
                LoadString(HINST_THISDLL, IDS_DRIVETSHOOT, szBuf, ARRAYSIZE(szBuf));
                SetWindowText(hwndHW, szBuf);

                LoadString(HINST_THISDLL, IDS_THESEDRIVES, szBuf, ARRAYSIZE(szBuf));
                SetDlgItemText(hwndHW, IDC_HWTAB_LVSTATIC, szBuf);
            } 
            else 
            {
                DestroyWindow(hDlg); // catastrophic failure
            }
        }
        return FALSE;
    }
    return FALSE;
}



//
// this function sets the inital drive attribute state
//
BOOL SetInitialDriveAttributes(DRIVEPROPSHEETPAGE* pdpsp)
{
    BOOL bRet = TRUE;
#ifdef WINNT
    DWORD dwFileSystemFlags;
    DWORD dwDriveAttribs = 0;

    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);

    if (pMtPt)    
    {
        bRet = pMtPt->GetFileSystemFlags(&dwFileSystemFlags);

        if (bRet)
        {
            if (pMtPt->IsCompressible())
            {
                // file-based compression is supported (must be NTFS)
                pdpsp->fIsCompressionAvailable = TRUE;
            
                if (pMtPt->IsCompressed())
                {
                    // the volume root is compressed
                    pdpsp->asInitial.fCompress = TRUE;

                    // if its compressed, compression better be available
                    ASSERT(pdpsp->fIsCompressionAvailable);
                }
            }

            //
            // BUGBUG (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we 
            // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
            // appeared first on NTFS5 volumes, at the same time sparse file support 
            // was implemented.
            //
            if (pMtPt->IsSupportingSparseFile())
            {
                // yup, we are on NTFS 5 or greater
                pdpsp->fIsIndexAvailable = TRUE;

                if (pMtPt->IsContentIndexed())
                {
                    pdpsp->asInitial.fIndex = TRUE;
                }
            }
        }
        pMtPt->Release();
    }
#endif 
    return TRUE;
}

BOOL CDrives_AddPage(LPPROPSHEETPAGE ppsp, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hpage;
    BOOL fSuccess;

    hpage = CreatePropertySheetPage(ppsp);
    if (hpage)
    {
        fSuccess = pfnAddPage(hpage, lParam);
        if (!fSuccess)
        {   // Couldn't add page
            DestroyPropertySheetPage(hpage);
            fSuccess = FALSE;
        }
    }
    else
    {   // Couldn't create page
        fSuccess = FALSE;
    }
    return fSuccess;
}

HRESULT CDrives_AddPagesHelper(DRIVEPROPSHEETPAGE* pdpsp, int iType,
                               LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    // Add the disk tools page if applicable...
    switch(iType)
    {
    case DRIVE_NO_ROOT_DIR:     // Error in getting type bail out of here!
    case DRIVE_REMOTE:
        break;

    default:
        pdpsp->psp.pszTemplate = MAKEINTRESOURCE(DLG_DISKTOOLS);
        pdpsp->psp.pfnDlgProc  = _DiskToolsDlgProc;

        CDrives_AddPage(&pdpsp->psp, pfnAddPage, lParam);
        // fall through

    case DRIVE_CDROM:
        if (g_bRunOnNT5 && !SHRestricted(REST_NOHARDWARETAB))
        {
            pdpsp->psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_HWTAB);
            pdpsp->psp.pfnDlgProc  = _DriveHWDlgProc;
            CDrives_AddPage(&pdpsp->psp, pfnAddPage, lParam);
        }
        break;
    }

    return NOERROR;
}

//
// We check if any of the IDList's points to a drive root.  If so, we use the
// drives property page.
// Note that drives should not be mixed with folders and files, even in a
// search window.
//
HRESULT CDrives_AddPages(void *lp, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    IDataObject *pdtobj = (IDataObject *)lp;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        TCHAR szPath[MAX_PATH];
        int i, cItems = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);

        for (i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
        {
            DRIVEPROPSHEETPAGE dpsp = {0};
            TCHAR szTitle[80];

            if (lstrlen(szPath) > 3)
                continue;               // can't be a drive letter
            
            dpsp.psp.dwSize      = SIZEOF(dpsp);    // extra data
            dpsp.psp.dwFlags     = PSP_DEFAULT;
            dpsp.psp.hInstance   = HINST_THISDLL;
            dpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_GENERAL);
            dpsp.psp.pfnDlgProc  = _DrvGeneralDlgProc,
            lstrcpyn(dpsp.szDrive, szPath, ARRAYSIZE(dpsp.szDrive));
            dpsp.iDrive          = DRIVEID_TCHAR(szPath);

            // if more than one drive selected give each tab the title of the drive
            // otherwise use "General"

            if (cItems > 1)
            {
                CMountPoint* pMtPt = CMountPoint::GetMountPoint(dpsp.iDrive);
                if (pMtPt)
                {
                    dpsp.psp.dwFlags = PSP_USETITLE;
                    dpsp.psp.pszTitle = szTitle;

                    pMtPt->GetDisplayName(szTitle, ARRAYSIZE(szTitle));

                    pMtPt->Release();
                }
            }

            if (!CDrives_AddPage(&dpsp.psp, pfnAddPage, lParam))
                break;

            // if only one property page added add the disk tools
            // and Hardware tab too...
            if (cItems == 1)
            {
                CDrives_AddPagesHelper(&dpsp, RealDriveType(dpsp.iDrive, FALSE /* fOKToHitNet */),
                               pfnAddPage, lParam);
            }
        }
        ReleaseStgMedium(&medium);
    }
#ifdef WINNT
    else
    {
        // try mounteddrive
        fmte.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);

        // Can we retrieve the MountedVolume format?
        if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
        {
            // Yes
            DRIVEPROPSHEETPAGE dpsp = {0};
            HPROPSHEETPAGE hpage;
            TCHAR szMountPoint[MAX_PATH];

            dpsp.psp.dwSize      = SIZEOF(dpsp);    // extra data
            dpsp.psp.dwFlags     = PSP_DEFAULT;
            dpsp.psp.hInstance   = HINST_THISDLL;
            dpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_GENERAL);
            dpsp.psp.pfnDlgProc  = _DrvGeneralDlgProc,
            dpsp.iDrive          = -1;
            dpsp.fMountedDrive   = TRUE;

            DragQueryFile((HDROP)medium.hGlobal, 0, szMountPoint, ARRAYSIZE(szMountPoint));

            lstrcpyn(dpsp.szDrive, szMountPoint, ARRAYSIZE(dpsp.szDrive));

            hpage = CreatePropertySheetPage(&dpsp.psp);
            if (hpage)
            {
                if (!pfnAddPage(hpage, lParam))
                {
                    DestroyPropertySheetPage(hpage);
                }
            }

            //
            // Disk tools page
            //
            CMountPoint* pMtPt = CMountPoint::GetMountPoint(szMountPoint);
            if (pMtPt)
            {
                CDrives_AddPagesHelper(&dpsp, pMtPt->GetDRIVEType(FALSE),
                               pfnAddPage, lParam);
                // BUGBUG: add this line when mtpt::Release is checked in
                pMtPt->Release();
            }

            ReleaseStgMedium(&medium);
        }
    }
#endif //WINNT
    return NOERROR;
}
