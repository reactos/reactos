#include "shellprv.h"
#pragma  hdrstop

#include "mtpt.h"
#include "ids.h"
#include "shitemid.h"

#include "apithk.h"

#ifdef WINNT
#include <ntddcdrm.h>
#else
#define Not_VxD
#include <vwin32.h>     // DeviceIOCtl calls
#endif

#define REGSTR_MTPT_ROOTKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints")
#define FILE_ATTRIBUTE_INVALID 0xFFFFFFFF

CRegSupportCached* CMountPoint::_prsStatic = NULL;

#ifdef DEBUG
int CMountPoint::_cMtPtDL = 0;
int CMountPoint::_cMtPtMOF = 0;
#endif

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
// get the friendly name for a given drive thing
// for example:
//      Floppy (A:)
//      Volume Name (D:)
//      User on 'Pyrex' (V:)
//      Dist on Strike\sys\public (Netware case)
HRESULT CMountPoint::GetDisplayName(LPTSTR pszName, DWORD cchName)
{
    HRESULT hres = E_FAIL;
    TCHAR szDriveLabel[MAX_DISPLAYNAME];
    static BOOL s_fAllDriveLetterFirst = -1;
    static BOOL s_fRemoteDriveLetterFirst = -1;
    static BOOL s_fNoDriveLetter = -1;

    ASSERT(cchName > 0);
    *pszName = 0; // handle failure case

    // for s_fDriveLetterFirst, see bug 250899, that's a long story.
    if (-1 == s_fRemoteDriveLetterFirst)
    {
        DWORD dw;
        DWORD cb = sizeof(dw);

        s_fRemoteDriveLetterFirst = FALSE;
        s_fAllDriveLetterFirst = FALSE;
        s_fNoDriveLetter = FALSE;

        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), 
            TEXT("ShowDriveLettersFirst"), NULL, &dw, &cb))
        {
            if (1 == dw)
            {
                s_fRemoteDriveLetterFirst = TRUE;
            }
            else
            {
                if (2 == dw)
                {
                    s_fNoDriveLetter = TRUE;
                }
                else
                {
                    if (4 == dw)
                    {
                        s_fAllDriveLetterFirst = TRUE;
                    }
                }
            }
        }
    }

    hres = GetLabel(szDriveLabel, ARRAYSIZE(szDriveLabel));

    if (SUCCEEDED(hres))
    {
        if (s_fNoDriveLetter)
        {
            StrCpyN(pszName, szDriveLabel, cchName);
        }
        else
        {
            BOOL fDriveLetterFirst = ((DRIVE_REMOTE == _uDriveType) && s_fRemoteDriveLetterFirst) ||
                                        s_fAllDriveLetterFirst;

            // To return something like: "My Drive (c:)", we need a drive letter.
            // Fortunately for us this fct is only called for a drive mounted on a
            // letter (from drive implementation of IShellFolder), for volumes mounted
            // on folders, the folder impl  is called rather than the drive one.
            LPTSTR psz = ShellConstructMessageString(HINST_THISDLL, 
                        MAKEINTRESOURCE(fDriveLetterFirst ? IDS_VOL_FORMAT_LETTER_1ST : IDS_VOL_FORMAT),
                        szDriveLabel, _GetNameFirstChar());
            if (psz)
            {
                StrCpyN(pszName, psz, cchName);
                LocalFree(psz);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    return hres;
}

int CMountPoint::GetDRIVEType(BOOL fOKToHitNet)
{
    TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetDRIVEType: for '%s'", _GetName());

    return _uDriveType;
}

int CMountPoint::GetDriveFlags()
{
    int i = 0;

    if (_sdDriveFlags.Update())
    {
        i = __uDriveFlags;
    }

    return i;
}

// { DRIVE_ISCOMPRESSIBLE | DRIVE_COMPRESSED | DRIVE_LFN | DRIVE_SECURITY }
int CMountPoint::GetVolumeFlags()
{
    int iFlags = _GetGVIDriveFlags();

    // Try to avoid getting the attributes
    if (iFlags & DRIVE_ISCOMPRESSIBLE)
    {
        DWORD dwAttrib = FILE_ATTRIBUTE_INVALID;

        if (_GetAttributes(&dwAttrib))
        {
            if (dwAttrib & FILE_ATTRIBUTE_COMPRESSED)
            {
                iFlags |= DRIVE_COMPRESSED;
            }
        }
    }

    return iFlags;
}

const ICONMAP c_aicmpDrive[] = {
    { SHID_COMPUTER_DRIVE525 , II_DRIVE525      },
    { SHID_COMPUTER_DRIVE35  , II_DRIVE35       },
    { SHID_COMPUTER_FIXED    , II_DRIVEFIXED    },
    { SHID_COMPUTER_REMOTE   , II_DRIVEFIXED    },
    { SHID_COMPUTER_CDROM    , II_DRIVECD       },
    { SHID_COMPUTER_NETDRIVE , II_DRIVENET      },
    { SHID_COMPUTER_REMOVABLE, II_DRIVEREMOVE   },
    { SHID_COMPUTER_RAMDISK  , II_DRIVERAM      },    
};

const int c_nicmpDrives = ARRAYSIZE(c_aicmpDrive);

UINT CMountPoint::GetIcon(LPTSTR pszModule, DWORD cchModule)
{
    int bFlags = GetSHIDType(FALSE);

    UINT uType = (bFlags & SHID_TYPEMASK);
    UINT iIcon = -IDI_DRIVEUNKNOWN;

    *pszModule = 0;

    for (UINT i = 0; i < ARRAYSIZE(c_aicmpDrive); i++)
    {
        if (c_aicmpDrive[i].uType == uType)
        {
            iIcon = c_aicmpDrive[i].indexResource;

            if (IsAudioDisc())
                iIcon = II_CDAUDIO;
            break;
        }
    }

    if (_IsAutoRun() || _HasDefaultIcon())
    {
        if (RSGetTextValue(TEXT("DefaultIcon"), NULL, pszModule, &cchModule))
        {
            iIcon = PathParseIconLocation(pszModule);
        }
        else
        {
            *pszModule = 0;
        }
    }
    
    if (*pszModule)
        TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetIcon: for '%s', chose '%s', '%d'", _GetName(), pszModule, iIcon);
    else
        TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetIcon: for '%s', chose '%d'", _GetName(), iIcon);

    return iIcon;
}

DWORD CMountPoint::GetClusterSize()
{
    DWORD dwSecPerClus, dwBytesPerSec, dwClusters, dwTemp;

    // assume this, avoid div by zero
    DWORD dwRet = 512;

#ifndef WINNT
    BOOL bFound = FALSE;
    // Win95 FAT32 lies to GetDiskFreeSpace() because buggy apps can't
    // deal with large return results here. so we try this IOCTL instead
    // for drive based volumes

    DevPB devpb;
    devpb.SplFunctions = 0;     // get default (don't hit the drive!)

    if (_DriveIOCTL(0x860, NULL, 0, &devpb, SIZEOF(devpb)))
    {
        dwRet = devpb.cbSec * devpb.secPerClus;
        bFound = TRUE;
    }

    if (!bFound)
#endif // !WINNT
    {
        if (GetDiskFreeSpace(_GetNameForFctCall(), &dwSecPerClus, &dwBytesPerSec, &dwTemp, &dwClusters))
            dwRet = dwSecPerClus * dwBytesPerSec;
    }
    
    return dwRet;   
}

HRESULT CMountPoint::SetLabel(LPCTSTR pszLabel)
{
    HRESULT hres = E_FAIL;
    HDPA hdpaInvalidPath = DPA_Create(2);

    StrCpyN(__szLabelFromReg, pszLabel, ARRAYSIZE(__szLabelFromReg));

    hres = (_sdLabelFromReg.Propagate() ? S_OK : E_FAIL);

    CMountPoint::InvalidateMountPoint(_szName, hdpaInvalidPath, MTPT_INV_LABEL);

    if (hdpaInvalidPath && DPA_GetPtrCount(hdpaInvalidPath))
    {
        if (SUCCEEDED(hres))
        {
            int n = DPA_GetPtrCount(hdpaInvalidPath);

            for (int i = 0; i < n; ++i)
            {
                LPTSTR pszPath = (LPTSTR)DPA_GetPtr(hdpaInvalidPath, i);
    
                if (pszPath)
                {
                    SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, pszPath, pszPath);

                    LocalFree((HLOCAL)pszPath);
                }
            }
        }
    }
    else
    {
        if (SUCCEEDED(hres))
        {
            SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _GetName(), _GetName());
        }
    }

    if (hdpaInvalidPath)
        DPA_Destroy(hdpaInvalidPath);

    return hres;
}

const struct { BYTE bFlags; UINT uID; UINT uIDUgly; } c_drives_type[] = 
{
    { SHID_COMPUTER_REMOVABLE,  IDS_DRIVES_REMOVABLE , IDS_DRIVES_REMOVABLE },
    { SHID_COMPUTER_DRIVE525,   IDS_DRIVES_DRIVE525  , IDS_DRIVES_DRIVE525_UGLY },
    { SHID_COMPUTER_DRIVE35,    IDS_DRIVES_DRIVE35   , IDS_DRIVES_DRIVE35_UGLY  },
    { SHID_COMPUTER_FIXED,      IDS_DRIVES_FIXED     , IDS_DRIVES_FIXED     },
    { SHID_COMPUTER_REMOTE,     IDS_DRIVES_NETDRIVE  , IDS_DRIVES_NETDRIVE  },
    { SHID_COMPUTER_CDROM,      IDS_DRIVES_CDROM     , IDS_DRIVES_CDROM     },
    { SHID_COMPUTER_RAMDISK,    IDS_DRIVES_RAMDISK   , IDS_DRIVES_RAMDISK   },
    { SHID_COMPUTER_NETDRIVE,   IDS_DRIVES_NETDRIVE  , IDS_DRIVES_NETDRIVE  },
    { SHID_COMPUTER_NETUNAVAIL, IDS_DRIVES_NETUNAVAIL, IDS_DRIVES_NETUNAVAIL},
    { SHID_COMPUTER_REGITEM,    IDS_DRIVES_REGITEM   , IDS_DRIVES_REGITEM   },
};

//static
void CMountPoint::GetTypeString(BYTE bFlags, LPTSTR pszType, DWORD cchType)
{
    *pszType = 0;

    for (int i = 0; i < ARRAYSIZE(c_drives_type); ++i)
    {
        if (c_drives_type[i].bFlags == (bFlags & SHID_TYPEMASK))
        {
            LoadString(HINST_THISDLL, _ShowUglyDriveNames() ? 
                c_drives_type[i].uIDUgly : c_drives_type[i].uID, pszType, cchType);
            break;
        }
    }
}

void CMountPoint::GetTypeString(LPTSTR pszType, DWORD cchType)
{
    int iFlags = GetSHIDType(TRUE);

    GetTypeString((BYTE)iFlags, pszType, cchType);
}

//static
BOOL CMountPoint::GetDriveIDList(int iDrive, DRIVE_IDLIST *piddl)
{
    BOOL fRet = FALSE;

    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fRet = pMtPt->_fDriveIDList;

        if (fRet)
            memcpy(piddl, &pMtPt->_DriveIDList, sizeof(DRIVE_IDLIST));

        pMtPt->Release();
    }

    TraceMsg(TF_MOUNTPOINT, "static CMountPoint::GetDriveIDList: for '%d' -> %hs",
        iDrive, fRet ? "HIT" : "MISS");

    return fRet;
}

//static
void CMountPoint::SetDriveIDList(int iDrive, DRIVE_IDLIST *piddl)
{
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        memcpy(&pMtPt->_DriveIDList, piddl, sizeof(DRIVE_IDLIST));

        pMtPt->_fDriveIDList = TRUE;

        pMtPt->Release();
    }

    TraceMsg(TF_MOUNTPOINT, "static CMountPoint::SetDriveIDList: for '%d'", iDrive);
}

HKEY CMountPoint::GetRegKey()
{
    TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetRegKey: for '%s'", _GetName());

    return RSDuplicateRootKey();
}

#ifndef WINNT
BOOL CMountPoint::WantToHide()
{
    return _DriveIOCTL(0x872, NULL, 0, NULL, 0);
}
#endif

BOOL CMountPoint::GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName)
{
    BOOL fRet = _sdGVI.Update();

    if (fRet)
    {
        StrCpyN(pszFileSysName, __gvi.szFileSysName, cchFileSysName);
    }
    else
    {
        *pszFileSysName = 0;
    }

    return fRet;
}

BOOL CMountPoint::GetFileSystemFlags(DWORD* pdwFlags)
{
    return _GetFileSystemFlags(pdwFlags);
}

HRESULT CMountPoint::GetComment(LPTSTR pszComment, DWORD cchComment)
{
    HRESULT hres = E_FAIL;
    TCHAR szCommentFromDesktopINI[MAX_MTPTCOMMENT];

    _sdCommentFromDesktopINI.SetBuffer(szCommentFromDesktopINI,
        ARRAYSIZE(szCommentFromDesktopINI) * sizeof(TCHAR));

    if ((DRIVE_REMOVABLE != _uDriveType) && 
        (DRIVE_CDROM != _uDriveType) && 
        _sdCommentFromDesktopINI.Update())
    {
        StrCpyN(pszComment, szCommentFromDesktopINI, cchComment);

        hres = S_OK;
    }
    else
    {
        *pszComment = 0;
    }

    _sdCommentFromDesktopINI.ResetBuffer();

    TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetComment: for '%s' returned '%s'",
        _GetName(), pszComment);

    return hres;
}


HRESULT CMountPoint::GetHTMLInfoTipFile(LPTSTR pszHTMLInfoTipFile, DWORD cchHTMLInfoTipFile)
{
    HRESULT hres = E_FAIL;
    TCHAR szHTMLInfoTipFileFromDesktopINI[MAX_PATH];

    _sdHTMLInfoTipFileFromDesktopINI.SetBuffer(szHTMLInfoTipFileFromDesktopINI,
        ARRAYSIZE(szHTMLInfoTipFileFromDesktopINI) * sizeof(TCHAR));

    if ((DRIVE_REMOVABLE != _uDriveType) && 
        (DRIVE_CDROM != _uDriveType) && 
        _sdHTMLInfoTipFileFromDesktopINI.Update())
    {
        StrCpyN(pszHTMLInfoTipFile, szHTMLInfoTipFileFromDesktopINI, cchHTMLInfoTipFile);

        hres = S_OK;
    }
    else
    {
        *pszHTMLInfoTipFile = 0;
    }

    _sdHTMLInfoTipFileFromDesktopINI.ResetBuffer();

    TraceMsg(TF_MOUNTPOINT, "CMountPoint::GetHTMLInfoTipFile: for '%s' returned '%s'",
        _GetName(), pszHTMLInfoTipFile);

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// Misc
CMountPoint::CMountPoint() : _cRef(1), __dwGFA(FILE_ATTRIBUTE_INVALID)
{
}

ULONG CMountPoint::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CMountPoint::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

#ifdef DEBUG
    _fMountedOnDriveLetter ? --_cMtPtDL : --_cMtPtMOF;
#endif
    
    delete this;
    return 0;
}

//static
CRegSupport* CMountPoint::GetRSStatic()
{
    if (!_prsStatic)
    {
        _prsStatic = new CRegSupportCached();
    }

    return _prsStatic;
}
///////////////////////////////////////////////////////////////////////////////
// Call Backs
///////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::_GetFileAttributesCB(PVOID pvData)
{
    DWORD* pdw = (DWORD*)pvData;

    *pdw = GetFileAttributes(_GetNameForFctCall());

    return (*pdw != FILE_ATTRIBUTE_INVALID);
}

BOOL CMountPoint::_GetVolumeInformationCB(PVOID pvData)
{
    GVI* pGVI = (GVI*)pvData;

    ZeroMemory(pGVI, sizeof(*pGVI));

    return GetVolumeInformation(_GetNameForFctCall(), pGVI->szLabel, ARRAYSIZE(pGVI->szLabel),
            &pGVI->dwSerialNumber, &pGVI->dwMaxlen, &pGVI->dwFileSysFlags,
            pGVI->szFileSysName, ARRAYSIZE(pGVI->szFileSysName));
}

BOOL CMountPoint::_AutorunCB(PVOID pvData)
{
    BOOL* pf = (BOOL*)pvData;

    *pf = (_IsAutoRunDrive() && _ProcessAutoRunFile());

    // Are we autorun?
    if (!*pf)
    {
        // No, make sure to delete the shell key
        RSDeleteSubKey(TEXT("Shell"));
    }

    return TRUE;
}

BOOL CMountPoint::_DefaultIconLabelCB(PVOID pvData)
{
    DEFICONLABEL* pdil = (DEFICONLABEL*)pvData;

    return _ProcessDefaultIconLabel(&(pdil->fDefaultIcon), &(pdil->fDefaultLabel));
}

BOOL CMountPoint::_CommentFromDesktopINICB(PVOID pvData)
{
    BOOL fRet = TRUE;
    LPTSTR pszCommentFromDesktopINI = (LPTSTR)pvData;

    if (!GetShellClassInfoInfoTip(_GetName(), pszCommentFromDesktopINI, MAX_MTPTCOMMENT))
    {
         *pszCommentFromDesktopINI = 0;
         fRet = FALSE;
    }

    return fRet;
}

BOOL CMountPoint::_HTMLInfoTipFileFromDesktopINICB(PVOID pvData)
{
    BOOL fRet = TRUE;
    LPTSTR pszHTMLInfoTipFileFromDesktopINI = (LPTSTR)pvData;

    if (!GetShellClassInfoHTMLInfoTipFile(_GetName(), pszHTMLInfoTipFileFromDesktopINI, MAX_PATH))
    {
         *pszHTMLInfoTipFileFromDesktopINI = 0;
         fRet = FALSE;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::_GetAttributes(DWORD* pdwAttrib)
{
    if (_sdGFA.Update())
    {
        if (FILE_ATTRIBUTE_INVALID == __dwGFA)
        {
            *pdwAttrib = 0;
        }
        else
        {
            *pdwAttrib = __dwGFA;
        }
    }

    return (FILE_ATTRIBUTE_INVALID != __dwGFA);
}

// { DRIVE_ISCOMPRESSIBLE | DRIVE_LFN | DRIVE_SECURITY }
int CMountPoint::_GetGVIDriveFlags()
{
    int iFlags = 0;

    if (_sdGVI.Update())
    {
        if (__gvi.dwFileSysFlags & FS_FILE_COMPRESSION)
        {
            iFlags |= DRIVE_ISCOMPRESSIBLE;
        }

        // Volume supports long filename (greater than 8.3)?
        if (__gvi.dwMaxlen > 12)
            iFlags |= DRIVE_LFN;

        // Volume supports security?
        if (__gvi.dwFileSysFlags & FS_PERSISTENT_ACLS)
            iFlags |= DRIVE_SECURITY;
    }

    return iFlags;
}

BOOL CMountPoint::_GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet = FALSE;

    // Do we have a label from the GetVolumeInformation
    if (_sdGVI.Update() && *__gvi.szLabel)
    {
        LPTSTR pszFinalLabel = __gvi.szLabel;

        fRet = TRUE;

        // Do we already have a label from the registry for this volume?
        // (the user may have renamed this drive)
        if (_sdLabelFromReg.Update() && *__szLabelFromReg) 
        {
            // Yes
            // Do they match (only diff in case)
            if (lstrcmpi(__szLabelFromReg, __gvi.szLabel) == 0)
            {
                // Yes
                pszFinalLabel = __szLabelFromReg;
            }
        }

        lstrcpyn(pszLabel, pszFinalLabel, cchLabel);
    }
    else
    {
        *pszLabel = 0;
    }

    return fRet;
}

BOOL CMountPoint::_GetFileSystemFlags(DWORD* pdwFlags)
{
    BOOL fRet = _sdGVI.Update();

    if (fRet)
    {
        *pdwFlags = __gvi.dwFileSysFlags;
    }

    return fRet;
}

BOOL CMountPoint::_IsAutoRun()
{
    return _sdAutorun.Update() && __fAutorun;
}

///////////////////////////////////////////////////////////////////////////////
// Default Icon/Label
///////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::_ProcessDefaultIconLabel(BOOL* pfDefaultIcon, BOOL* pfDefaultLabel)
{
    BOOL fUseAutorunIcon = FALSE;
    BOOL fUseAutorunLabel = FALSE;

    TCHAR szIconLocation[MAX_PATH + 12];
    DWORD cchIconLocation = ARRAYSIZE(szIconLocation);
    TCHAR szLabel[MAX_LABEL];
    DWORD cchLabel = ARRAYSIZE(szLabel);

    RSDeleteSubKey(TEXT("DefaultIcon"));
    RSDeleteSubKey(TEXT("DefaultLabel"));
    
    // First let's update the autorun info, it might provide us with a default icon and/or label
    _sdAutorun.Update();

    szIconLocation[0] = 0;
    szLabel[0] = 0;

    // Is it an autorun drive?
    if (__fAutorun)
    {
        // Yes, does it provide a DefaultIcon?
        if (RSGetTextValue(TEXT("_Autorun\\DefaultIcon"), NULL, szIconLocation, &cchIconLocation))
        {
            // The Autorun provided icon has precedence
            fUseAutorunIcon = TRUE;
        }
        else
        {
            szIconLocation[0] = 0;
        }

        // Same for Label
        if (RSGetTextValue(TEXT("_Autorun\\DefaultLabel"), NULL, szLabel, &cchLabel))
        {
            // The Autorun provided icon has precedence
            fUseAutorunLabel = TRUE;
        }
        else
        {
            szLabel[0] = 0;
        }
    }

    if (!fUseAutorunIcon && _fMountedOnDriveLetter)
    {
        // Let's check if there is one at the following locations
        TCHAR szSubKey[MAX_PATH];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultIcon"),
            _GetNameFirstChar());

        if (!RegGetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, szIconLocation,
            ARRAYSIZE(szIconLocation) * sizeof(TCHAR)))
        {
            szIconLocation[0] = 0;

            // Let's try second location
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
                TEXT("Applications\\Explorer.exe\\Drives\\%c\\DefaultIcon"),
                _GetNameFirstChar());

            if (!RegGetValueString(HKEY_CLASSES_ROOT, szSubKey, NULL, szIconLocation,
                ARRAYSIZE(szIconLocation) * sizeof(TCHAR)))
            {
                szIconLocation[0] = 0;
            }
        }
    }

    if (szIconLocation[0])
    {
        // Let's copy the value under our drive key
        RSSetTextValue(TEXT("DefaultIcon"), NULL, szIconLocation, REG_OPTION_NON_VOLATILE);
        *pfDefaultIcon = TRUE;
    }

    if (!fUseAutorunLabel && _fMountedOnDriveLetter)
    {
        // Let's check if there is one at the following locations
        TCHAR szSubKey[MAX_PATH];

        wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DriveIcons\\%c\\DefaultLabel"),
            _GetNameFirstChar());

        if (!RegGetValueString(HKEY_LOCAL_MACHINE, szSubKey, NULL, szLabel,
            ARRAYSIZE(szLabel) * sizeof(TCHAR)))
        {
            szLabel[0] = 0;

            // Let's try second location.
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey),
                TEXT("Applications\\Explorer.exe\\Drives\\%c\\DefaultLabel"),
                _GetNameFirstChar());

            if (!RegGetValueString(HKEY_CLASSES_ROOT, szSubKey, NULL, szLabel,
                ARRAYSIZE(szLabel) * sizeof(TCHAR)))
            {
                szLabel[0] = 0;
            }
        }
    }

    if (szLabel[0])
    {
        // Let's copy the value under our drive key
        RSSetTextValue(TEXT("DefaultLabel"), NULL, szLabel, REG_OPTION_NON_VOLATILE);
        *pfDefaultLabel = TRUE;
    }

    return TRUE;
}

BOOL CMountPoint::_HasDefaultIcon()
{
    return _sdDefaultIconLabel.Update() && __dil.fDefaultIcon;
}

BOOL CMountPoint::_HasDefaultLabel()
{
    return _sdDefaultIconLabel.Update() && __dil.fDefaultLabel;
}

BOOL CMountPoint::_GetDefaultLabel(LPTSTR pszLabel, DWORD cchLabel)
{
    DWORD cchLabelLocal = cchLabel;

    return _sdDefaultIconLabel.Update() && 
        RSGetTextValue(TEXT("DefaultLabel"), NULL, pszLabel,
        &cchLabelLocal);
}
///////////////////////////////////////////////////////////////////////////////
// Drive cache
///////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::_IsValidDriveCache()
{
    BOOL fRet = FALSE;
    DWORD cbSize;

    PBYTE pbUniqueBlob = _MakeUniqueBlob(&cbSize);
    PBYTE pbRegUniqueBlob = _GetRegUniqueBlob(cbSize);

    if (pbUniqueBlob && pbRegUniqueBlob)
    {
        fRet = !memcmp(pbUniqueBlob, pbRegUniqueBlob, cbSize);
    }

    if (pbRegUniqueBlob)
    {
        _ReleaseUniqueBlob(pbRegUniqueBlob);
    }

    if (pbUniqueBlob)
    {
        _ReleaseUniqueBlob(pbUniqueBlob);
    }

    return fRet;
}

void CMountPoint::_ResetDriveCache()
{
    DWORD cbSize;

    RSDeleteValue(NULL, TEXT("_UB"));

    PBYTE pb = _MakeUniqueBlob(&cbSize);

    if (pb)
    {
        RSSetBinaryValue(NULL, TEXT("_UB"), pb, cbSize);

        _ReleaseUniqueBlob(pb);
    }

    _sdDriveFlags.WipeReg();
    _sdGFA.WipeReg();
    _sdGVI.WipeReg();

    _sdDefaultIconLabel.WipeReg();

    _sdAutorun.WipeReg();
    _sdCommentFromDesktopINI.WipeReg();
    _sdHTMLInfoTipFileFromDesktopINI.WipeReg();
}

///////////////////////////////////////////////////////////////////////////////
// Name fcts
///////////////////////////////////////////////////////////////////////////////
LPCTSTR CMountPoint::_GetNameForFctCall()
{
    return _szName;
}

TCHAR CMountPoint::_GetNameFirstChar()
{
    return _szName[0];
}

LPTSTR CMountPoint::_GetNameFirstXChar(LPTSTR pszBuffer, int c)
{
    StrCpyN(pszBuffer, _szName, c);

    return pszBuffer;
}

LPCTSTR CMountPoint::_GetName()
{
    return _szName;
}

///////////////////////////////////////////////////////////////////////////////
// Unique blob
///////////////////////////////////////////////////////////////////////////////
PBYTE CMountPoint::_GetRegUniqueBlob(DWORD cbSize)
{
    PBYTE pb = (PBYTE)LocalAlloc(LPTR, cbSize);

    if (!RSGetBinaryValue(NULL, TEXT("_UB"), pb, &cbSize))
    {
        // Either the value is not there, or the buffer is too small,
        // one way or the other the Blobs will not be the same

        LocalFree(pb);

        pb = NULL;
    }

    return pb;
}

void CMountPoint::_ReleaseUniqueBlob(PBYTE pbUniqueBlob)
{
    LocalFree(pbUniqueBlob);
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
HRESULT CMountPoint::_InitializeBase(LPCTSTR pszName, BOOL fMountedOnDriveLetter)
{
    lstrcpyn(_szName, pszName, ARRAYSIZE(_szName));
    PathAddBackslash(_szName);

    _fMountedOnDriveLetter = fMountedOnDriveLetter;

    _SetKeyName();

    RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _szKeyName, REG_OPTION_NON_VOLATILE);

    if (!_fMountedOnDriveLetter)
    {
        RSSetTextValue(NULL, NULL, _GetName());
    }	

    _sdDriveFlags.Init(this, (SUBDATACB)_GetDriveFlagsCB, &__uDriveFlags);
    _sdDriveFlags.InitExpiration(_GetExpirationInMilliSec());
    _sdDriveFlags.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_DriveFlags"),
        sizeof(__uDriveFlags), TRUE);

    _sdGFA.Init(this, (SUBDATACB)_GetFileAttributesCB, &__dwGFA);
    _sdGFA.InitExpiration(_GetExpirationInMilliSec());
    _sdGFA.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_GFA"),
        sizeof(__dwGFA), TRUE);

    _sdGVI.Init(this, (SUBDATACB)_GetVolumeInformationCB, &__gvi);
    _sdGVI.InitExpiration(_GetExpirationInMilliSec());
    _sdGVI.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_GVI"),
        sizeof(__gvi), TRUE);

    _sdLabelFromReg.Init(this, (SUBDATACB)NULL, &__szLabelFromReg);
    _sdLabelFromReg.InitExpiration(EXPIRATION_NEVER);
    _sdLabelFromReg.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_LabelFromReg"),
        ARRAYSIZE(__szLabelFromReg) * sizeof(TCHAR));

    _sdAutorun.Init(this, (SUBDATACB)_AutorunCB, &__fAutorun);
    _sdAutorun.InitExpiration(EXPIRATION_NEVER);
    _sdAutorun.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_Autorun"),
        sizeof(__fAutorun));

    _sdDefaultIconLabel.Init(this, (SUBDATACB)_DefaultIconLabelCB, &__dil);
    _sdDefaultIconLabel.InitExpiration(EXPIRATION_NEVER);
    _sdDefaultIconLabel.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(), TEXT("_DIL"),
        sizeof(__dil));

    _sdCommentFromDesktopINI.Init(this, (SUBDATACB)_CommentFromDesktopINICB, NULL);
    // InitExpiration called below (after GetDriveType())
    _sdCommentFromDesktopINI.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(),
        TEXT("_CommentFromDesktopINI"), 0);

    _sdHTMLInfoTipFileFromDesktopINI.Init(this, (SUBDATACB)_HTMLInfoTipFileFromDesktopINICB, NULL);
    // InitExpiration called below (after GetDriveType())
    _sdHTMLInfoTipFileFromDesktopINI.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(),
        TEXT("_HTMLInfoTipFileFromDesktopINI"), 0);

#ifdef DEBUG
    fMountedOnDriveLetter ? ++_cMtPtDL : ++_cMtPtMOF;
#endif

    _SetUniqueID();

    _uDriveType = GetDriveType(_GetNameForFctCall());

    if (DRIVE_NO_ROOT_DIR == _uDriveType)
    {
        // See comment in GetMountPoint(int iDrive, ...)
        // It should be the only way of getting here for a DRIVE_NO_ROOT_DIR
        _uDriveType = DRIVE_REMOTE;
    }

    if (!_IsValidDriveCache())
    {
        _ResetDriveCache();
    }

    RSSetTextValue(NULL, TEXT("BaseClass"), TEXT("Drive"));

    if (DRIVE_CDROM == _uDriveType)
    {
        RSSetTextValue(NULL, TEXT("_HasNotif"), TEXT(""));
    }

    if (DRIVE_FIXED == _uDriveType)
    {
        _sdCommentFromDesktopINI.InitExpiration(3000);
        _sdHTMLInfoTipFileFromDesktopINI.InitExpiration(3000);
    }
    else
    {
        _sdCommentFromDesktopINI.InitExpiration(EXPIRATION_NEVER);
        _sdHTMLInfoTipFileFromDesktopINI.InitExpiration(EXPIRATION_NEVER);
    }

    if ((DRIVE_FIXED != _uDriveType) &&
        (DRIVE_REMOVABLE != _uDriveType) && 
        (DRIVE_CDROM != _uDriveType))

    {
        if (!_sdCommentFromDesktopINI.ExistInReg())
        {
            TCHAR szCommentFromDesktopINI[MAX_MTPTCOMMENT];

            _sdCommentFromDesktopINI.SetBuffer(szCommentFromDesktopINI,
                ARRAYSIZE(szCommentFromDesktopINI) * sizeof(TCHAR));

            _sdCommentFromDesktopINI.Update();
    
            _sdCommentFromDesktopINI.ResetBuffer();
        }

        if (!_sdHTMLInfoTipFileFromDesktopINI.ExistInReg())
        {
            TCHAR szHTMLInfoTipFileFromDesktopINI[MAX_PATH];

            _sdHTMLInfoTipFileFromDesktopINI.SetBuffer(szHTMLInfoTipFileFromDesktopINI,
                    ARRAYSIZE(szHTMLInfoTipFileFromDesktopINI) * sizeof(TCHAR));

            _sdHTMLInfoTipFileFromDesktopINI.Update();

            _sdHTMLInfoTipFileFromDesktopINI.ResetBuffer();
        }
    }

    _sdDefaultIconLabel.Update();

    RSCVInitSubKey(NULL);
    _RSCVUpdateVersionOnCacheRead();

    return S_OK;
}

ULONG CMountPoint::_ReleaseInternal()
{
    // we don't want to write to the registry from now on
    //  this object is going to leave us soon

    _sdLabelFromReg.HoldUpdates();
    _sdCommentFromDesktopINI.HoldUpdates();
    _sdHTMLInfoTipFileFromDesktopINI.HoldUpdates();
    _sdDefaultIconLabel.HoldUpdates();
    _sdDriveFlags.HoldUpdates();
    _sdGFA.HoldUpdates();
    _sdGVI.HoldUpdates();
    _sdDefaultIconLabel.HoldUpdates();
    _sdAutorun.HoldUpdates();

#ifndef WINNT
    if (_fWipeVolatileOnWin9X)
    {
        _sdLabelFromReg.FakeVolatileOnWin9X();
        _sdCommentFromDesktopINI.FakeVolatileOnWin9X();
        _sdHTMLInfoTipFileFromDesktopINI.FakeVolatileOnWin9X();
        _sdDefaultIconLabel.FakeVolatileOnWin9X();
        _sdDriveFlags.FakeVolatileOnWin9X();
        _sdGFA.FakeVolatileOnWin9X();
        _sdGVI.FakeVolatileOnWin9X();
        _sdDefaultIconLabel.FakeVolatileOnWin9X();
        _sdAutorun.FakeVolatileOnWin9X();
    }
#endif

    return Release();
}
///////////////////////////////////////////////////////////////////////////////
// DeviceIOControl stuff
///////////////////////////////////////////////////////////////////////////////
BOOL CMountPoint::_DriveIOCTL(int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut, BOOL fFileSystem,
                          HANDLE handle)
{
    BOOL fHandlePassedIn = TRUE;
    BOOL fSuccess = FALSE;
    DWORD dwRead;

    TraceMsg(TF_MOUNTPOINT, 
        "    CMountPoint::_DriveIOCTL: >>>EXT<<< (DeviceIOControl) '%s'", _GetName());

    if (INVALID_HANDLE_VALUE == handle)
    {
        handle = _GetIOCTLHandle(fFileSystem);
        fHandlePassedIn = FALSE;
    }

    if (INVALID_HANDLE_VALUE != handle)
    {
#ifdef WINNT
        //
        // On NT, we issue DEVIOCTLs by cmd id.
        //
        fSuccess = DeviceIoControl(handle, cmd, pvIn, dwIn, pvOut, dwOut, &dwRead, NULL);
#else
        DIOC_REGISTERS reg;

        //
        // On non-NT, we talk to VWIN32, issuing reads (which are converted
        // internally to DEVIOCTLs)
        //
        //  BUGBUG: this is a real hack (talking to VWIN32) on NT we can just
        //  open the device, we dont have to go through VWIN32
        //
        reg.reg_EBX = LOWORD(_GetNameFirstChar()) + 1;   // make 1 based drive number
        reg.reg_EDX = (DWORD)pvOut; // out buffer
        reg.reg_ECX = cmd;              // device specific command code
        reg.reg_EAX = 0x440D;           // generic read ioctl
        reg.reg_Flags = 0x0001;     // flags, assume error (carry)

        DeviceIoControl(handle, VWIN32_DIOC_DOS_IOCTL, &reg, SIZEOF(reg), &reg, SIZEOF(reg), &dwRead, NULL);

        fSuccess = !(reg.reg_Flags & 0x0001);
#endif
        if (!fHandlePassedIn)
            CloseHandle(handle);
    }
    return fSuccess;
}

// returns INVALID_HANDLE_VALUE when failing

HANDLE CMountPoint::_GetIOCTLHandle(BOOL fFileSystem)
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;

#ifdef WINNT
    if (fFileSystem)
    {
        // Used by XXXAudioDisc on NT.
        // On NT, when use GENERIC_READ (as opposed to 0) in the CreateFile call, we
        // get a handle to the filesystem (CDFS), not the device itself.  But we can't
        // change DriveIOCTL to do this, since that causes the floppy disks to spin
        // up, and we don't want to do that.
        dwDesiredAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ;
    }
#else
    // Always use these hardcoded values
    dwDesiredAccess = GENERIC_READ;
    dwShareMode = FILE_SHARE_READ;
#endif
    handle = _GetIOCTLHandle(dwDesiredAccess, dwShareMode);

    return handle;
}

// returns INVALID_HANDLE_VALUE when failing
HANDLE CMountPoint::_GetIOCTLHandle(DWORD dwDesiredAccess, DWORD dwShareMode)
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    TCHAR szTmpName[MAX_PATH];
    DWORD dwFileAttributes = 0;

#ifdef WINNT
    // Go for VolumeGUID first
    if (GetVolumeNameForVolumeMountPoint(_GetName(), szTmpName, ARRAYSIZE(szTmpName)))
    {
        PathRemoveBackslash(szTmpName);
    }
    else
    {
        // Probably a floppy, which cannot be mounted on a folder
        lstrcpy(szTmpName, TEXT("\\\\.\\A:"));
        szTmpName[4] = _GetNameFirstChar();;
    }
#else
    // Always use these hardcoded values
    lstrcpyn(szTmpName, TEXT("\\\\.\\vwin32"), ARRAYSIZE(szTmpName));
    dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
#endif
    handle = CreateFile(szTmpName, dwDesiredAccess, dwShareMode, NULL, OPEN_EXISTING, dwFileAttributes, NULL);

    return handle;
}

///////////////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////////////
//static
BOOL CMountPoint::_ShowUglyDriveNames()
{
    static BOOL s_fShowUglyDriveNames = (BOOL)42;   // Preload some value to say lets calculate...

    if (s_fShowUglyDriveNames == (BOOL)42)
    {
        int iACP;
#ifdef WINNT
        TCHAR szTemp[MAX_PATH];     // Nice large buffer
        if (GetLocaleInfo(GetUserDefaultLCID(), LOCALE_IDEFAULTANSICODEPAGE, szTemp, ARRAYSIZE(szTemp)))
        {
            iACP = StrToInt(szTemp);
            // per Samer Arafeh, show ugly name for 1256 (Arabic ACP)
            if (iACP == 1252 || iACP == 1254 || iACP == 1255 || iACP == 1257 || iACP == 1258)
                goto TryLoadString;
            else
                s_fShowUglyDriveNames = TRUE;
        } else {
        TryLoadString:
            // All indications are that we can use pretty drive names.
            // Double-check that the localizers didn't corrupt the chars.
            LoadString(HINST_THISDLL, IDS_DRIVES_UGLY_TEST, szTemp, ARRAYSIZE(szTemp));

            // If the characters did not come through properly set ugly mode...
            s_fShowUglyDriveNames = (szTemp[0] != 0x00BC || szTemp[1] != 0x00BD);
        }
#else
        // on win98 the shell font can't change with user locale. Because ACP
        // is always same as system default, and all Ansi APIs are still just 
        // following ACP.
        // 
        iACP = GetACP();
        if (iACP == 1252 || iACP == 1254 || iACP == 1255 || iACP == 1257 || iACP == 1258)
            s_fShowUglyDriveNames = FALSE;
        else
            s_fShowUglyDriveNames = TRUE;
#endif
    }
    return s_fShowUglyDriveNames;
}

// Will return and icon index from the shell icon cache (-1 for failure)
// This should be called only from within HandleWMDeviceChange
//static
int CMountPoint::_GetCachedIcon(int iDrive)
{
    int iIcon = -1;
    CMountPoint* pMtPt = _GetCachedMtPt(iDrive);

    if (pMtPt)
    {
        TCHAR szModule[MAX_PATH];

        int iIconTmp = pMtPt->GetIcon(szModule, ARRAYSIZE(szModule));

        if (!szModule[0])
        {
            lstrcpyn(szModule, c_szShell32Dll, ARRAYSIZE(szModule));
        }

        iIcon = Shell_GetCachedImageIndex(szModule, iIconTmp, 0);

        pMtPt->Release();
    }

    return iIcon;
}

//
// External API for use by non-CPP modules.
//
HRESULT MountPoint_RegisterChangeNotifyAlias(int iDrive)
{    
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);
    if (pMtPt)
    {
        pMtPt->ChangeNotifyRegisterAlias();
        pMtPt->Release();
        hr = NOERROR;
    }
    return hr;
}    

