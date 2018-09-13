#include "shellprv.h"
#pragma  hdrstop

#include "shitemid.h"
#include "ids.h"
#include "oemhard.h" //for IsNEC_9800
#ifdef WINNT
#include <ntddcdrm.h>
#else
#define Not_VxD
#include <vwin32.h>     // DeviceIOCtl calls
#endif

#include "mtptl.h"

TCHAR const c_szAudioCDShell[] = TEXT("AudioCD\\shell");
TCHAR const c_szDVDShell[] = TEXT("DVD\\shell");

#pragma pack(1)
// from dos\inc\ioctl.inc
typedef struct {
    TCHAR dmiAllocationLength;          // db   ?       ; length of the buffer provided by caller
    TCHAR dmiInfoLength;                        // db   ?       ; length of information returned
    TCHAR dmiFlags;                     // db   ?       ; DRIVE_MAP_INFO flags
    TCHAR dmiInt13Unit;                 // db   ?       ; int 13 drive number.  FFh if the drive
                                        //              ; does not map to an int 13 drive
    DWORD dmiAssociatedDriveMap;        // dd   ?       ; bit map of logical drive numbers that
                                        //              ; are associated with the given drive
                                        //              ; (i.e. parent/child volumes of compressed
                                        //              ; volume files)
    DWORD dmiPartitionStartRBA[2];      // dq   ?       ; starting RBA offset of the given
                                        //              ; partition
} DRIVE_MAP_INFO;
#pragma pack()

#define PROT_MODE_EJECT                 0x08    //      ; indicates a protect mode drive
                                                //      ; supports electronic eject

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Label
HRESULT CMtPtLocal::SetLabel(LPCTSTR pszLabel)
{
    HRESULT hres = E_FAIL;

    TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::SetLabel: for '%s'", _GetName());

    if (SetVolumeLabel(_GetNameForFctCall(), pszLabel))
    {
        TraceMsg(TF_MOUNTPOINT, "   'SetVolumeLabel' succeeded");

        hres = CMountPoint::SetLabel(pszLabel);
    }
    else
    {
        DWORD dwErr;
    
        dwErr = GetLastError();
    
#ifdef WINNT
        switch (dwErr)
        {
            case NOERROR:
                break;

            case ERROR_ACCESS_DENIED:
        
                hres = S_FALSE;	// don't have permission, shouldn't put them back into editing mode

                ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_ACCESSDENIED ),
                    MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                break;
            
            case ERROR_WRITE_PROTECT:
                hres = S_FALSE; // can't write, shouldn't put them back into editing mode
                ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_WRITEPROTECTED ),
                    MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                break;

            case ERROR_LABEL_TOO_LONG:
                ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_ERR_VOLUMELABELBAD ),
                    MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                break;

            default:
                ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_BADLABEL ),
                    MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                break;
        }
#else
        ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_ERR_VOLUMELABELBAD ),
            MAKEINTRESOURCE( IDS_TITLE_VOLUMELABELBAD ),
            MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
#endif

        TraceMsg(TF_MOUNTPOINT, "   'SetVolumeLabel' failed");
    }

    return hres;
}

HRESULT CMtPtLocal::SetDriveLabel(LPCTSTR pszLabel)
{
    HRESULT hres = E_FAIL;

    if (DRIVE_REMOVABLE == _uDriveType)
    {
        // we rename the drive not the media
        TCHAR szSubKey[MAX_PATH];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultLabel"),
            _GetNameFirstChar());

        hres = RegSetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, pszLabel) ? S_OK : E_FAIL;

        if (SUCCEEDED(hres))
        {
	    CMountPoint::InvalidateMountPoint(_GetName(), NULL, MTPT_INV_LABEL);
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _GetName(), _GetName());
        }
    }
    else
    {
        hres = SetLabel(pszLabel);
    }

    return hres;
}

HRESULT CMtPtLocal::GetLabel(LPTSTR pszLabel, DWORD cchLabel, DWORD dwFlags)
{
    HRESULT hres = S_OK;

    switch(_uDriveType)
    {
        case DRIVE_REMOVABLE:
        {
            // Do we want fancy name?
            if (MTPT_LABEL_NOFANCY != dwFlags)
            {
                // Yes
                if (_HasDefaultLabel())
                {
                    hres = (_GetDefaultLabel(pszLabel, cchLabel) ? S_OK : E_FAIL);
                }
                else
                {
                    UINT ids = IDS_UNK_FLOPPY_DRIVE;

                    switch (_bFloppyType)
                    {
                        case DEVPB_DEVTYP_525_0360:
                        case DEVPB_DEVTYP_525_1200:
                            ids = _ShowUglyDriveNames() ? IDS_525_FLOPPY_DRIVE_UGLY : IDS_525_FLOPPY_DRIVE;
                            break;

                        case DEVPB_DEVTYP_350_0720:
                        case DEVPB_DEVTYP_350_1440:
                        case DEVPB_DEVTYP_350_2880:
                        case DEVPB_DEVTYP_350_120M:
                            ids = _ShowUglyDriveNames() ? IDS_35_FLOPPY_DRIVE_UGLY : IDS_35_FLOPPY_DRIVE;
                            break;

                        case DEVPB_DEVTYP_NECHACK:
                            ids = IDS_NECUNK_FLOPPY_DRIVE;
                            break;
                    }

                    LoadString(HINST_THISDLL, ids, pszLabel, cchLabel);
                }
            }
            else
            {
                // No, just the raw albel from GetVolumeInforamtion
                if (!_GetGVILabelOrMixedCaseFromReg(pszLabel, cchLabel))
                {
                    *pszLabel = 0;
                    hres = E_FAIL;
                }
            }

            break;
        }

        // We need to handle both connected and unconnected volumes. However,
        // for unconncected volumes we have to be careful as a real drive
        // may have taken it's spot (a new CDROM for example).
        case DRIVE_NO_ROOT_DIR:
        case DRIVE_UNKNOWN:
            break;

        default:
        {
            // Not a removable

            if (!_GetGVILabelOrMixedCaseFromReg(pszLabel, cchLabel))
            {
                *pszLabel = 0;
                hres = E_FAIL;
            }
            else
            {
                if (!*pszLabel)
                {
                    // We have an empty string for the label, let's see if we have a default label
                    if (_HasDefaultLabel())
                    {
                        hres = (_GetDefaultLabel(pszLabel, cchLabel) ? S_OK : E_FAIL);           
                    }
                }
            }

            break;
        }
    }

    if (FAILED(hres))
    {
        if (MTPT_LABEL_NOFANCY == dwFlags)
        {
            *pszLabel = 0;
            hres = S_OK;
        }
        else
        {
            if (_HasDefaultLabel())
            {
                hres = (_GetDefaultLabel(pszLabel, cchLabel) ? S_OK : E_FAIL);           
            }

            if (FAILED(hres))
            {
               GetTypeString(pszLabel, cchLabel);
            }
            hres = S_OK;
        }
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// 
HRESULT CMtPtLocal::Eject()
{
    TCHAR szNameForError[MAX_DISPLAYNAME];
    HRESULT hres = E_FAIL;

    GetDisplayName(szNameForError, ARRAYSIZE(szNameForError));

    hres = _Eject(szNameForError);

    InvalidateMountPoint(_GetName());

    return hres;
}

BOOL CMtPtLocal::IsEjectable(BOOL fForceCDROM)
{
    BOOL fIsEjectable = FALSE;

    if (fForceCDROM && (DRIVE_CDROM == _uDriveType))
    {
        fIsEjectable = TRUE;
    }
    else
    {
#ifdef WINNT
        DISK_GEOMETRY disk_g;
        //
        // Call down to get the drive's geometry.  We can then see if it's a
        // removeable (or ejectable) drive.  We can't just use GetDriveType
        // because it tells us that floppies are removeable, which they aren't
        // (by software).  Doing it this way gives us the information we need
        // to make the correct determination
        //
        if (_DriveIOCTL(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL, 0, &disk_g, SIZEOF(disk_g)))
        {
            fIsEjectable = (disk_g.MediaType == RemovableMedia);

            // NT5 bug 140923 - if the media was (1) the boot media, or
            // (2) the pagefile, then ejecting the drive hangs the system
            // hard.  Don't allow eject in these scenarios.
            if (fIsEjectable)
            {
                // BUGBUG: Unfortunately there's no IOCTL to query this.
                // Comment added to bug and resolved for NT5.  Maybe we'll
                // figure out how to do this at some later date.
                //
                // Hey, we could check if this is the WINDOWS drive and
                // not allow eject in that case!  Still misses the PAGEFILE case tho...
            }
        }
#else
        DRIVE_MAP_INFO dmi;
        dmi.dmiAllocationLength = SIZEOF(dmi);

        fIsEjectable = _DriveIOCTL(0x86F, NULL, 0, &dmi, SIZEOF(dmi)) && (dmi.dmiFlags & PROT_MODE_EJECT);

        // avoid disk hit and say all removable media are ejectable
        // return IsRemovableDrive(iDrive) || (LockUnlockDrive(iDrive, STATUS) == 0);
#endif
    }

    return fIsEjectable;
}

BOOL CMtPtLocal::IsFloppy()
{
    BOOL bRet = FALSE;

    if (DRIVE_REMOVABLE == _uDriveType)
    {
        switch (_bFloppyType)
        {
            case DEVPB_DEVTYP_525_0360:
            case DEVPB_DEVTYP_525_1200:
            case DEVPB_DEVTYP_350_0720:
            case DEVPB_DEVTYP_350_1440:
            case DEVPB_DEVTYP_350_2880:
            case DEVPB_DEVTYP_350_120M:
            case DEVPB_DEVTYP_NECHACK:
                bRet = TRUE;
                break;
        }
    }
    return bRet;
}

BOOL CMtPtLocal::IsAudioDisc()
{
    return _sdDriveFlags.Update() && (DRIVE_AUDIOCD & __uDriveFlags);
}

int CMtPtLocal::GetSHIDType(BOOL fOKToHitNet)
{
    int iFlags = 0;

    TraceMsg(TF_MOUNTPOINT, "CMtPtLocal::GetSHIDType: for '%s'", _GetName());

    iFlags |= SHID_COMPUTER | _uDriveType;

    switch (iFlags & SHID_TYPEMASK)
    {
        case SHID_COMPUTER_REMOVABLE:
        {
            switch (_bFloppyType)
            {
                case DEVPB_DEVTYP_525_0360:
                case DEVPB_DEVTYP_525_1200:
                case DEVPB_DEVTYP_NECHACK:
                    iFlags = SHID_COMPUTER_DRIVE525;
                    break;

                case DEVPB_DEVTYP_350_0720:
                case DEVPB_DEVTYP_350_1440:
                case DEVPB_DEVTYP_350_2880:
                case DEVPB_DEVTYP_350_120M:
                    iFlags = SHID_COMPUTER_DRIVE35;
                    break;
            }
        }
        break;

        case SHID_COMPUTER | DRIVE_CDROM:
        case SHID_COMPUTER | DRIVE_FIXED:

            break;

        // Invalid drive gets SHID_COMPUTER_MISC, which others must check for
        case SHID_COMPUTER | DRIVE_NO_ROOT_DIR:
            iFlags = SHID_COMPUTER_MISC;
            break;

        case SHID_COMPUTER | DRIVE_UNKNOWN:
        default:
            iFlags = SHID_COMPUTER_FIXED;
            break;
    }

    return iFlags;
}

///////////////////////////////////////////////////////////////////////////////
// Name fcts
///////////////////////////////////////////////////////////////////////////////
LPCTSTR CMtPtLocal::_GetNameForFctCall()
{
    return _szUniqueID;
}

LPCTSTR CMtPtLocal::_GetVolumeGUID()
{
    return _GetNameForFctCall();
}

void CMtPtLocal::_InvalidateMedia()
{
    CMountPoint::_InvalidateMedia();
}

void CMtPtLocal::_InvalidateRefresh()
{
    _sdCommentFromDesktopINI.Invalidate();
    _sdHTMLInfoTipFileFromDesktopINI.Invalidate();

    CMountPoint::_InvalidateRefresh();
}

LPCTSTR CMtPtLocal::_GetUniqueID()
{
    return _szUniqueID;
}

void CMtPtLocal::_SetUniqueID()
{
    BOOL fDone = FALSE;

    _szUniqueID[0] = 0;

#ifdef WINNT
    if (g_bRunOnNT5 && GetVolumeNameForVolumeMountPoint(_szName, _szUniqueID,
        ARRAYSIZE(_szUniqueID)))
    {
        fDone = TRUE;
    }
#endif
    // On Win9X and NT4 we cannot mount a drive on a folder, so the drive letter is a unique ID

    if (!fDone)
    {
        lstrcpyn(_szUniqueID, _szName, ARRAYSIZE(_szUniqueID));
    }
}
///////////////////////////////////////////////////////////////////////////////
// Call Backs
///////////////////////////////////////////////////////////////////////////////
BOOL CMtPtLocal::_GetDriveFlagsCB(PVOID pvData)
{
    __uDriveFlags = 0;

    // Is this a CD/DVD of some sort?
    if (DRIVE_CDROM == _uDriveType)
    {
        // Yes
        LPCTSTR pszSubKey = NULL;
        BOOL fAudio = _IsAudioDisc();
        BOOL fDVD = _IsDVDDisc();

        if (fAudio)
        {
            __uDriveFlags |= DRIVE_AUDIOCD;
            pszSubKey = c_szAudioCDShell;
        }
        else
        {
            if (fDVD)
            {
                __uDriveFlags |= DRIVE_DVD;
                pszSubKey = c_szDVDShell;
            }
        }

        // Set the AutoOpen stuff, if applicable
        if (pszSubKey)
        {
            TCHAR ach[80];
            LONG cb = SIZEOF(ach);
            ach[0] = 0;

            // get the default verb for Audio CD/DVD
            SHRegQueryValue(HKEY_CLASSES_ROOT, pszSubKey, ach, &cb);

            // we should only set AUTOOPEN if there is a default verb on Audio CD/DVD
            if (ach[0])
                __uDriveFlags |= DRIVE_AUTOOPEN;
        }
    }
    else
    {
        // No, by default every drive type is ShellOpen, except CD-ROMs
        __uDriveFlags |= DRIVE_SHELLOPEN;
    }

    if (_sdAutorun.Update() && __fAutorun)
    {
        __uDriveFlags |= DRIVE_AUTORUN;

        //BUGBUG should we set AUTOOPEN based on a flag in the AutoRun.inf???
        __uDriveFlags |= DRIVE_AUTOOPEN;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
PBYTE CMtPtLocal::_MakeUniqueBlob(DWORD* pcbSize)
{
    PBYTE pb = NULL;
    * pcbSize = 0;

    if ((DRIVE_REMOVABLE != _uDriveType) && (DRIVE_CDROM != _uDriveType))
    {
        GVI* pGVI;

        *pcbSize = sizeof(GVI);

        pGVI = (GVI*)LocalAlloc(LPTR, *pcbSize);

        if (pGVI)
        {
            if (!GetVolumeInformation(_GetNameForFctCall(), pGVI->szLabel, ARRAYSIZE(pGVI->szLabel),
                    &pGVI->dwSerialNumber, &pGVI->dwMaxlen, &pGVI->dwFileSysFlags,
                    pGVI->szFileSysName, ARRAYSIZE(pGVI->szFileSysName)))
            {
                LocalFree(pGVI);

                pGVI = NULL;
            }
        }

        pb = (PBYTE)pGVI;
    }
    else
    {
        UINT* pu;

        *pcbSize = sizeof(UINT);

        pu = (UINT*)LocalAlloc(LPTR, *pcbSize);

        if (pu)
        {
            *pu = _uDriveType;
        }

        pb = (PBYTE)pu;
    }

    return pb;
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
HRESULT CMtPtLocal::_Initialize(LPCTSTR pszName, BOOL fMountedOnDriveLetter)
{
    HRESULT hres = _InitializeBase(pszName, fMountedOnDriveLetter);

    if (DRIVE_REMOVABLE == _uDriveType)
    {
        _bFloppyType = _CalcFloppyType();
    }

    return hres;
}

DWORD CMtPtLocal::_GetExpirationInMilliSec()
{
    DWORD dwMaxAgeInMilliSec = EXPIRATION_NEVER;
    // Special case for floppy with old VolumeInfo ( > 3 sec)?
    if ((DRIVE_REMOVABLE == _uDriveType))
    {
        if (IsFloppy())
            dwMaxAgeInMilliSec = 3000;
        else
            dwMaxAgeInMilliSec = 5000;
    }

    return dwMaxAgeInMilliSec;
}
///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////
typedef MCIERROR  (WINAPI *MCISENDSTRINGAFN)(LPCTSTR lpstrCommand, LPTSTR lpstrReturnString,
                                             UINT uReturnLength, HWND hwndCallback);
STDAPI_(HMODULE) LoadMM();

HRESULT CMtPtLocal::_Eject(LPTSTR pszMountPointNameForError)
{
    HRESULT hres = E_FAIL;

    if (IsEjectable(FALSE))
    {
        // it is a protect mode drive that we can tell directly...
#ifdef WINNT
        if (DRIVE_CDROM == _uDriveType)
        {
            _DriveIOCTL(IOCTL_DISK_EJECT_MEDIA, NULL, 0, NULL, 0);
            
            hres = S_OK;
        }
        else
        {
            // For removable drives, we want to do all the calls on a single
            // handle, thus we can't do lots of calls to _DriveIOCTL.  Instead,
            // use the helper routines to do our work...
            
            // Don't bring up any error message if the user already chose to abort.

            BOOL fAborted;
            BOOL fFailed;
            HANDLE hDeviceIOCtl = _Lock(pszMountPointNameForError, &fAborted, &fFailed);

            if (!fAborted)
            {
                if (fFailed)
                {
                    ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_UNMOUNT_TEXT ),
                            MAKEINTRESOURCE( IDS_UNMOUNT_TITLE ),
                            MB_OK | MB_ICONSTOP | MB_SETFOREGROUND, pszMountPointNameForError);
                }
                else
                {
                    PREVENT_MEDIA_REMOVAL pmr;

                    pmr.PreventMediaRemoval = FALSE;

                    // tell the drive to allow removal, then eject
                    if (!_DriveIOCTL(g_bRunOnNT5 ? IOCTL_STORAGE_MEDIA_REMOVAL : IOCTL_DISK_MEDIA_REMOVAL,
                            &pmr, SIZEOF(pmr), NULL, 0, FALSE, hDeviceIOCtl) ||
                        !_DriveIOCTL(g_bRunOnNT5 ? IOCTL_STORAGE_EJECT_MEDIA : IOCTL_DISK_EJECT_MEDIA,
                            NULL, 0, NULL, 0, FALSE, hDeviceIOCtl))
                    {
                        ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE( IDS_EJECT_TEXT ),
                                MAKEINTRESOURCE( IDS_EJECT_TITLE ),
                                MB_OK | MB_ICONSTOP | MB_SETFOREGROUND, pszMountPointNameForError);
                    }
                    else
                        hres = S_OK;

                    CloseHandle(hDeviceIOCtl);
                }
            }
        }
#else
        _DriveIOCTL(0x849, NULL, 0, NULL, 0);
        hres = S_OK;
#endif
    }
    else
    {
        // else we will let MCICDA try to eject it for us...
        HMODULE hMM = LoadMM();

        if (hMM)
        {
#ifdef UNICODE
            MCISENDSTRINGAFN pfnMciSendString = (MCISENDSTRINGAFN)GetProcAddress(hMM, "mciSendStringW");
#else
            MCISENDSTRINGAFN pfnMciSendString = (MCISENDSTRINGAFN)GetProcAddress(hMM, "mciSendStringA");
#endif
            if (pfnMciSendString)
            {
                //BUGBUG: (stephstm) only works for drive mounted on letter
                TCHAR szMCI[128];

                wsprintf(szMCI, TEXT("Open %c: type cdaudio alias foo shareable"), _GetNameFirstChar());

                if (pfnMciSendString(szMCI, NULL, 0, 0L) == MMSYSERR_NOERROR)
                {
                    pfnMciSendString(TEXT("set foo door open"), NULL, 0, 0L);
                    pfnMciSendString(TEXT("close foo"), NULL, 0, 0L);
                    hres = S_OK;
                }
            }
        }
    }
    return hres;
}

#ifdef WINNT

#ifndef IDTRYAGAIN
#define IDTRYAGAIN      10
#define IDCONTINUE      11
#define MB_CANCELTRYCONTINUE        0x00000006L
#endif /* WINVER >= 0x0500 */

HANDLE CMtPtLocal::_Lock(LPTSTR pszMountPointNameForError, BOOL* pfAborted, BOOL* pfFailed)
{
    *pfAborted = FALSE;
    *pfFailed = FALSE;
    HANDLE hDeviceIOCtl = NULL;
    
_RETRY_LOCK_VOLUME_:

    hDeviceIOCtl = _GetIOCTLHandle(GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);

    if (INVALID_HANDLE_VALUE != hDeviceIOCtl)
    {
        //
        // Now try to lock the drive...
        //
        // In theory, if no filesystem was mounted, the IOCtl command could go
        // to the device, which would fail with ERROR_INVALID_FUNCTION.  If that
        // occured, we would still want to proceed, since the device might still
        // be able to handle the IOCTL_DISK_EJECT_MEDIA command below.
        //
        if (!_DriveIOCTL(FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, FALSE, hDeviceIOCtl) ||
            (GetLastError() == ERROR_INVALID_FUNCTION))
        {
            // So we can't lock the drive.
            // If we're running on NT4, just fail the Eject.
            // If we're running on NT5, bring up a message box to see if user
            // wants to
            //  1. Abort.
            //  2. Retry to lock the drive.
            //  3. Dismount anyway.
    
            if (g_bRunOnNT5)
            {
                int iRet = ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_RETRY_UNMOUNT_TEXT),
                        MAKEINTRESOURCE(IDS_RETRY_UNMOUNT_TITLE),
                        MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_SETFOREGROUND, pszMountPointNameForError);
                
                switch (iRet)
                {
                    case IDCANCEL:
                        *pfAborted = TRUE;
                        break;
            
                    case IDCONTINUE:
                        // send FSCTL_DISMOUNT_VOLUME
                        if (!_DriveIOCTL(FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, FALSE, hDeviceIOCtl))
                        {
                            TraceMsg(TF_WARNING, "FSCTL_DISMOUNT_VOLUME failed with error %d.", GetLastError());
                            *pfFailed = TRUE;
                            break;
                        }
                        // We sucessfully dismounted the volume, so the h we have is not valid anymore. We
                        // therefore close it and start the process all over again, hoping to lock the volume before
                        // anyone re-opens a handle to it
                        //
                        // (fall through)
                    case IDTRYAGAIN:
                        CloseHandle(hDeviceIOCtl);
                        goto _RETRY_LOCK_VOLUME_;
                }
            }
            else
                *pfFailed = TRUE;
        }
    }
    else
        *pfFailed = TRUE;

    if (*pfAborted || *pfFailed)
    {
        if (INVALID_HANDLE_VALUE != hDeviceIOCtl)
        {
            CloseHandle(hDeviceIOCtl);
            hDeviceIOCtl = INVALID_HANDLE_VALUE;
        }
    }

    return hDeviceIOCtl;
}
#endif


// type stuff
#ifdef WINNT
BYTE _GetSpecialTypeByte(MEDIA_TYPE MediaType)
{
    BYTE byte;
    switch(MediaType)
    {
        case F5_1Pt2_512:       byte = DEVPB_DEVTYP_525_1200; break;
        case F3_1Pt44_512:      byte = DEVPB_DEVTYP_350_1440; break;
        case F3_2Pt88_512:      byte = DEVPB_DEVTYP_350_2880; break;
        case F3_20Pt8_512:      byte = DEVPB_DEVTYP_350_2880; break;   // Hack
        case F3_720_512:        byte = DEVPB_DEVTYP_350_0720; break;
        case F5_360_512:        byte = DEVPB_DEVTYP_525_0360; break;
        case F5_320_512:        byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case F5_320_1024:       byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case F5_180_512:        byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case F5_160_512:        byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case RemovableMedia:    byte = 0xFF;                  break;
        case FixedMedia:        byte = DEVPB_DEVTYP_FIXED;    break;
        case F3_120M_512:       byte = DEVPB_DEVTYP_350_120M; break;
        //
        // Japanese specific device types from here.
        //
        case F3_640_512:        byte = DEVPB_DEVTYP_350_0720; break;   // Hack
        case F5_640_512:        byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case F5_720_512:        byte = DEVPB_DEVTYP_525_0360; break;   // Hack
        case F3_1Pt2_512:       byte = DEVPB_DEVTYP_350_1440; break;   // Hack
        case F3_1Pt23_1024:     byte = DEVPB_DEVTYP_350_1440; break;   // Hack
        case F5_1Pt23_1024:     byte = DEVPB_DEVTYP_525_1200; break;   // Hack
        case F3_128Mb_512:      byte = 0xFF;                  break;   // Hack
        case F3_230Mb_512:      byte = 0xFF;                  break;   // Hack
        default:                byte = 0xFF;                  break;
    }
    return byte;
}
#endif

BYTE CMtPtLocal::_CalcFloppyType()
{
    BYTE byte;
#ifdef WINNT
    int  i;
    DISK_GEOMETRY   SupportedGeometry[30];      // s/b big enough for all

    if (IsNEC_PC9800())
        for(i = 0; i < 30; i++)
            SupportedGeometry[i].MediaType = (MEDIA_TYPE)0xff;

    if (_DriveIOCTL(IOCTL_DISK_GET_MEDIA_TYPES, NULL, 0,
                          SupportedGeometry, SIZEOF(SupportedGeometry)))
    {
        if (IsNEC_PC9800())
        {
            // PC98's floppy.sys supports both 5inch FDD and 3.5inch FDD.
            // we have to distinguish which one the machine has by
            // going through the entire array to see if ioctl has returned 
            // 1.44mb or not.
            //
            byte = 0xff;
            for (i = 0; i < 30; i++) 
            {
                if(SupportedGeometry[i].MediaType == F3_1Pt44_512)
                {
                    byte = DEVPB_DEVTYP_350_1440;
                    break;
                }
            }
            // now go through the array 
            for (i=0; i< 30 && byte == 0xff; i++)
                byte = _GetSpecialTypeByte(SupportedGeometry[i].MediaType);

        }
        else
        {
            byte = _GetSpecialTypeByte(SupportedGeometry[0].MediaType);
        }
    }
    else
    {
        byte = 0xFF;
    }
#else
    DevPB devpb;
    devpb.SplFunctions = 0;     // get default (don't hit the drive!)

    if (!_DriveIOCTL(0x860, NULL, 0, &devpb, SIZEOF(devpb)))
        byte = 0xFF;

    byte = devpb.devType;
#endif
    return byte;
}

BOOL CMtPtLocal::_IsAudioDisc()
{
    BOOL fAudio = FALSE;
#ifdef WINNT

#define TRACK_TYPE_MASK 0x04
#define AUDIO_TRACK     0x00
#define DATA_TRACK      0x04

    // To be compatible with Win95, we'll only return TRUE from this
    // function if the disc has ONLY audio tracks (and NO data tracks).

    // BUGBUG: Post NT-SUR beta 1, we should consider adding a new
    // DriveType flag for "contains data tracks" and revamp the commands
    // available on a CD-ROM drive.  The current code doesn't handle
    // mixed audio/data and audio/autorun discs very usefully. --JonBe

    // First try the new IOCTL which gives us a ULONG with bits indicating
    // the presence of either/both data & audio tracks

    CDROM_DISK_DATA data;

    if (_DriveIOCTL(IOCTL_CDROM_DISK_TYPE, NULL, 0, &data, SIZEOF(data), TRUE))
    {
        if ((data.DiskData & CDROM_DISK_AUDIO_TRACK) && !(data.DiskData & CDROM_DISK_DATA_TRACK))
            fAudio = TRUE;
        else
            fAudio = FALSE;
    }
    else
    {
        // else that failed, so try to look for audio tracks the old way, by
        // looking throught the table of contents manually.  Note that data tracks
        // are supposed to be hidden in the TOC by CDFS now on mixed audio/data
        // discs (at least if the data tracks follow the audio tracks).

        PCDROM_TOC  pTOC = (PCDROM_TOC)LocalAlloc(LPTR, SIZEOF(CDROM_TOC));

        if (pTOC)
        {
            if (!_DriveIOCTL(IOCTL_CDROM_READ_TOC, NULL, 0, pTOC, SIZEOF(*pTOC), TRUE))
            {
                SUB_Q_CHANNEL_DATA subq;
                CDROM_SUB_Q_DATA_FORMAT df;

                LocalFree(pTOC);

                //
                // We might not have been able to read the TOC because the drive
                // was busy playing audio.  Lets try querying the audio position.
                //
                df.Format = IOCTL_CDROM_CURRENT_POSITION;
                fAudio = _DriveIOCTL(IOCTL_CDROM_READ_Q_CHANNEL, &df, SIZEOF(df), &subq, SIZEOF(subq));
            }
            else
            {
                INT nTracks = (pTOC->LastTrack - pTOC->FirstTrack) + 1;
                INT iTrack = 0;
                // Now iterate through the tracks looking for Audio data

                while (iTrack < nTracks)
                {
                    if ((pTOC->TrackData[iTrack].Control & TRACK_TYPE_MASK) == AUDIO_TRACK)
                        fAudio = TRUE;
                    else
                        if ((pTOC->TrackData[iTrack].Control & TRACK_TYPE_MASK) == DATA_TRACK)
                        {
                            fAudio = FALSE;
                            break;
                        }

                    ++iTrack;
                }
                LocalFree(pTOC);
            }
        }
    }
#else

#pragma pack(1)
typedef struct {
    WORD    InfoLevel;      // information level
    DWORD   SerialNum;      // serial number
    TCHAR    VolLabel[11];   // ASCII volume label
    TCHAR    FileSysType[8]; // file system type
}   MediaID;
#pragma pack()
    MediaID mid;

    ASSERT(GetDRIVEType(FALSE) == DRIVE_CDROM);

    mid.FileSysType[0] = 0;
    _DriveIOCTL(0x0866, NULL, 0, &mid, SIZEOF(mid));
    mid.FileSysType[7] = 0;

    fAudio = (lstrcmp(mid.FileSysType, TEXT("CDAUDIO")) == 0) ? TRUE : FALSE;
#endif

    TraceMsg(TF_MOUNTPOINT, "    CMtPtLocal::_CalcAudioDisc: for '%s', result is '%hs'",
        _GetName(), (fAudio ? "TRUE" : "FALSE"));

    return fAudio;
}

BOOL CMtPtLocal::_IsDVDDisc()
{
    TCHAR szDVDFile[MAX_PATH];
    UINT err;

    BOOL fDVD = FALSE;
    
    // build abs path to AutoRun.inf
    lstrcpyn(szDVDFile, _GetName(), ARRAYSIZE(szDVDFile));

    PathAddBackslash(szDVDFile);
    lstrcatn(szDVDFile, TEXT("video_ts\\video_ts.ifo"), ARRAYSIZE(szDVDFile));

    err = SetErrorMode(SEM_FAILCRITICALERRORS);

//    fDVD = PathFileExistsAndAttributes(szDVDFile, NULL);
    fDVD = PathFileExists(szDVDFile);

    SetErrorMode(err);

    return fDVD;
}

