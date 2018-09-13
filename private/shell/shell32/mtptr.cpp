#include "shellprv.h"
#pragma  hdrstop

#include "shitemid.h"
#include "ids.h"

#include "mtptr.h"

#define REGSTR_MTPT_ROOTKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints")

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
int CMtPtRemote::GetSHIDType(BOOL fOKToHitNet)
{
    int iFlags = 0;

    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::GetSHIDType: for '%s'", _GetName());

    iFlags |= SHID_COMPUTER | _uDriveType;

    switch (iFlags & SHID_TYPEMASK)
    {
        case SHID_COMPUTER | DRIVE_REMOTE:
            iFlags = SHID_COMPUTER_NETDRIVE;
            break;

        // Invalid drive gets SHID_COMPUTER_MISC, which others must check for
        case SHID_COMPUTER | DRIVE_NO_ROOT_DIR:
        case SHID_COMPUTER | DRIVE_UNKNOWN:
        default:
            iFlags = SHID_COMPUTER_FIXED;
            break;
    }

    return iFlags;
}

HRESULT CMtPtRemote::SetLabel(LPCTSTR pszLabel)
{
    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::SetLabel: for '%s'", _GetName());

    return CMountPoint::SetLabel(pszLabel);
}

BOOL CMtPtRemote::IsDisconnectedNetDrive()
{
    return !_IsConnected();
}

BOOL CMtPtRemote::IsUnavailableNetDrive()
{
    return _sdConnectionStatus2.Update() && 
        (ERROR_CONNECTION_UNAVAIL == __dwConnectionStatus2);
}

DWORD CMtPtRemote::GetPathSpeed()
{
    return _dwSpeed;
}

HRESULT CMtPtRemote::GetLabel(LPTSTR pszLabel, DWORD cchLabel, DWORD dwFlags)
{
    HRESULT hres = S_OK;

    *pszLabel = 0;
        
    // Do we already have a label from the registry for this volume?
    // (the user may have renamed this drive)

    if (_sdLabelFromReg.Update() && *__szLabelFromReg)
    {
        // Yes
        lstrcpyn(pszLabel, __szLabelFromReg, cchLabel);
    }
    else
    {
        // No

        // Do we have a name from the server?
        if (_sdLabelFromDesktopINI.Update() && *__szLabelFromDesktopINI)
        {
            // Yes
            lstrcpyn(pszLabel, __szLabelFromDesktopINI, cchLabel);
        }
        else
        {
            // No
            // We should build up the display name ourselves
            _GetDefaultUNCDisplayName(pszLabel, cchLabel);

            if (!*pszLabel)
                hres = E_FAIL;
        }
    }

    if (FAILED(hres))
    {
        GetTypeString(pszLabel, cchLabel);
        hres = S_OK;
    }

    return hres;
}

HRESULT CMtPtRemote::_GetDefaultUNCDisplayName(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr = E_FAIL;
    LPTSTR pszShare, pszT;
    TCHAR szTempUNCPath[MAX_PATH];

    pszLabel[0] = TEXT('\0');
    if (PathIsUNC(_GetUNCName()))
    {
        // Now we need to handle 3 cases.
        // The normal case: \\pyrex\user
        // The Netware setting root: \\strike\sys\public\dist
        // The Netware CD?            \\stike\sys \public\dist
        lstrcpyn(szTempUNCPath, _GetUNCName(), ARRAYSIZE(szTempUNCPath));
        pszT = StrChr(szTempUNCPath, TEXT(' '));
        while (pszT)
        {
            pszT++;
            if (*pszT == TEXT('\\'))
            {
                // The netware case of \\strike\sys \public\dist
                *--pszT = 0;
                break;
            }
            pszT = StrChr(pszT, TEXT(' '));
        }

        pszShare = StrRChr(szTempUNCPath, NULL, TEXT('\\'));
        if (pszShare)
        {
            *pszShare++ = 0;
            PathMakePretty(pszShare);

            // pszServer should always start at char 2.
            if (szTempUNCPath[2])
            {
                LPTSTR pszServer, pszSlash;

                pszServer = &szTempUNCPath[2];
                for (pszT = pszServer; pszT != NULL; pszT = pszSlash)
                {
                    pszSlash = StrChr(pszT, TEXT('\\'));
                    if (pszSlash)
                        *pszSlash = 0;

                    PathMakePretty(pszT);
                    if (pszSlash)
                        *pszSlash++ = TEXT('\\');
                }
                LPTSTR pszLabel2 = ShellConstructMessageString(HINST_THISDLL,
                        MAKEINTRESOURCE(IDS_UNC_FORMAT), pszShare, pszServer);
                if (pszLabel2)
                {
                    lstrcpyn(pszLabel, pszLabel2, cchLabel);
                    LocalFree(pszLabel2);
                }
                else
                {
                    *pszLabel = TEXT('\0');
                }
                hr = S_OK;
            }
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Call Backs
///////////////////////////////////////////////////////////////////////////////
BOOL CMtPtRemote::_GetDriveFlagsCB(PVOID pvData)
{
    // By default every drive type is ShellOpen, except CD-ROMs
    __uDriveFlags = DRIVE_SHELLOPEN;

    if (_sdAutorun.Update() && __fAutorun)
    {
        __uDriveFlags |= DRIVE_AUTORUN;

        //BUGBUG should we set AUTOOPEN based on a flag in the AutoRun.inf???
        __uDriveFlags |= DRIVE_AUTOOPEN;
    }

    if (_IsConnected())
    {
        if ((0 != _dwSpeed) && (_dwSpeed <= SPEED_SLOW))
            __uDriveFlags |= DRIVE_SLOW;
    }

    return TRUE;
}

BOOL CMtPtRemote::_GetConnectionStatus1CB(PVOID pvData)
{
    CONNECTIONSTATUS* pcs = (CONNECTIONSTATUS*)pvData;
    DWORD dwSize = SIZEOF(pcs->wngcs);

    TCHAR szTmpName[MAX_PATH];
    int len;

    lstrcpyn(szTmpName, _GetName(), ARRAYSIZE(szTmpName));

    len = lstrlen(szTmpName) - 1;

    if (TEXT('\\') == szTmpName[len])
        szTmpName[len] = 0;

    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::_GetConnectionStatus1CB: for '%s'", _GetName());

    pcs->dwWNGC3ConnectionStatus = WNetGetConnection3(szTmpName, NULL, WNGC_INFOLEVEL_DISCONNECTED,
        &pcs->wngcs, &dwSize);

    return TRUE;
}

BOOL CMtPtRemote::_GetConnectionStatus2CB(PVOID pvData)
{
    DWORD* pdw = (DWORD*)pvData;

    TCHAR szRemoteName[MAX_PATH];
    DWORD cchRemoteName = ARRAYSIZE(szRemoteName);
    TCHAR szPath[3];

    *pdw = WNetGetConnection(_GetNameFirstXChar(szPath, 2 + 1), szRemoteName, &cchRemoteName);

    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::_GetConnectionStatus2CB: for '%s'", _GetName());

    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// Name fcts
///////////////////////////////////////////////////////////////////////////////
LPCTSTR CMtPtRemote::_GetUNCName()
{
    return _GetUniqueID();
}

LPCTSTR CMtPtRemote::_GetUniqueID()
{
    return _szUniqueID;
}

void CMtPtRemote::_SetUniqueID()
{
    DWORD cch = ARRAYSIZE(_szUniqueID);
    TCHAR szPath[3];

    // only copy the first two characters of szPath
    WNetGetConnection(_GetNameFirstXChar(szPath, 2 + 1), _szUniqueID, &cch);
}
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
HRESULT CMtPtRemote::_Initialize(LPCTSTR pszName, BOOL fMountedOnDriveLetter)
{
    HRESULT hres = E_FAIL;

    _sdLabelFromDesktopINI.Init(this, (SUBDATACB)NULL, &__szLabelFromDesktopINI);
    _sdLabelFromDesktopINI.InitExpiration(EXPIRATION_NEVER);
    _sdLabelFromDesktopINI.InitRegSupport(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY, _GetKeyName(),
        TEXT("_LabelFromDesktopINI"), ARRAYSIZE(__szLabelFromDesktopINI) * sizeof(TCHAR));

    // We cannot cache the connection status.  This is already cached at the redirector level.
    // When calling the WNetGetConnection fcts you get what's cache there, no check is actually
    // done on the network to see if this information is accurate (OK/Disconnected/Unavailable).
    // The information is updated only when the share is actually accessed (e.g: GetFileAttributes)
    // 
    // So we need to always do the calls (fortunately non-expensive) so that we get the most
    // up to date info.  Otherwise the following was occuring: A user double click a map drive
    // from the Explorer's Listview, WNetConnection gets called and we get the OK cached value 
    // from the redirector.  Some other code actually try to access the share, and the redirector 
    // realize that the share is not there and set its cache to Disconnected.  We are queried
    // again for the state of the connection to update the icon, if we cached this info we
    // return OK, if we ask for it (0.1 sec after the first call to WNetGetConnection) we get
    // Disconnected. (stephstm 06/02/99)

    _sdConnectionStatus1.Init(this, (SUBDATACB)_GetConnectionStatus1CB, &__cs1);
    _sdConnectionStatus1.InitExpiration(0);

    _sdConnectionStatus2.Init(this, (SUBDATACB)_GetConnectionStatus2CB, &__dwConnectionStatus2);
    _sdConnectionStatus2.InitExpiration(0);

    hres = _InitializeBase(pszName, fMountedOnDriveLetter);

    _CalcPathSpeed();

    if (!_sdLabelFromDesktopINI.ExistInReg())
    {
        GetShellClassInfo(_GetName(), TEXT("NetShareDisplayName"),
            __szLabelFromDesktopINI, ARRAYSIZE(__szLabelFromDesktopINI));

        _sdLabelFromDesktopINI.Propagate();
    }

    return hres;
}

ULONG CMtPtRemote::_ReleaseInternal()
{
    // we don't want to write to the registry from now on
    //  this object is going to leave us soon

    _sdLabelFromDesktopINI.HoldUpdates();
    _sdConnectionStatus1.HoldUpdates();
    _sdConnectionStatus2.HoldUpdates();

#ifndef WINNT
    if (_fWipeVolatileOnWin9X)
    {
        _sdLabelFromDesktopINI.FakeVolatileOnWin9X();
    }
#endif

    return CMountPoint::_ReleaseInternal();
}

DWORD CMtPtRemote::_GetExpirationInMilliSec()
{
    // we set this to 35 seconds to avoid people calling multiple times when
    // it takes a while because we are spinning up a drive.
    return 35000;
}

PBYTE CMtPtRemote::_MakeUniqueBlob(DWORD* pcbSize)
{
    DWORD dw;
    TCHAR szPath[3];
    DWORD cchRemoteName = MAX_PATH;
    LPTSTR pszRemoteName;

    *pcbSize = cchRemoteName * sizeof(TCHAR);
    pszRemoteName = (LPTSTR)LocalAlloc(LPTR, *pcbSize);

    if (pszRemoteName)
    {
        // wnet needs "c:", so get the first 2 chars of the path (+1 for null)
        dw = WNetGetConnection(_GetNameFirstXChar(szPath, 2 + 1), pszRemoteName, &cchRemoteName);

        if (NO_ERROR != dw)
        {
            LocalFree(pszRemoteName);

            pszRemoteName = NULL;
        }
    }

    return (PBYTE)pszRemoteName;
}

///////////////////////////////////////////////////////////////////////////////
// Invalidate
///////////////////////////////////////////////////////////////////////////////
void CMtPtRemote::_InvalidateRefresh()
{
    _sdConnectionStatus1.Invalidate();
    _sdConnectionStatus2.Invalidate();

    CMountPoint::_InvalidateRefresh();
}

void CMtPtRemote::_ResetDriveCache()
{
    _sdLabelFromDesktopINI.WipeReg();

    CMountPoint::_ResetDriveCache();
}

void CMtPtRemote::_InvalidateDrive()
{
    _sdLabelFromDesktopINI.WipeReg();    

    CMountPoint::_InvalidateDrive();
}

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////
void CMtPtRemote::_CalcPathSpeed()
{
    _dwSpeed = 0;

    NETCONNECTINFOSTRUCT nci;
    NETRESOURCE nr;
    TCHAR szPath[3];

    _fmemset(&nci, 0, SIZEOF(nci));
    nci.cbStructure = SIZEOF(nci);
    _fmemset(&nr, 0, SIZEOF(nr));

    // we are passing in a local drive and MPR does not like us to pass a
    // local name as Z:\ but only wants Z:
    _GetNameFirstXChar(szPath, 2 + 1);
    
    nr.lpLocalName = szPath;

    // dwSpeed is returned by MultinetGetConnectionPerformance
    MultinetGetConnectionPerformance(&nr, &nci);

    _dwSpeed = nci.dwSpeed;
}

BOOL CMtPtRemote::_IsConnected()
{
    BOOL fPrevConnected = _IsConnectedFromStateVar();
    BOOL fConnected = FALSE;

    if (_sdConnectionStatus1.Update() && _sdConnectionStatus2.Update())
    {
        fConnected = _IsConnectedFromStateVar();
    }

    if (fPrevConnected != fConnected)
    {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, _GetName(), NULL);
    }

    return fConnected;
}

BOOL CMtPtRemote::_IsConnectedFromStateVar()
{
    BOOL fNotConnected = ((NO_ERROR != __dwConnectionStatus2) ||
        ((WN_SUCCESS == __cs1.dwWNGC3ConnectionStatus) &&
        (WNGC_DISCONNECTED == __cs1.wngcs.dwState))) ? TRUE : FALSE;

    return !fNotConnected;

#if 0
    // This is the code needed if we want IsDisconnectedNetDrive to 
    // STRICTLY return IsDisconnected state.  For now the above implementation
    // return also IsDisconnected if the drive is Unavailable, which is not
    // strictly true.   This implementation was considered for bug 356309.
    // See comment in bug.  stephstm (07/09/99)
    BOOL fNotConnected = FALSE;

    // Is it "Unavailable"?
    // BTW: WN_CONNECTION_CLOSED == ERROR_CONNECTION_UNAVAIL
    if (WN_CONNECTION_CLOSED != __dwConnectionStatus2)
    {
        // No, thus it is Available

        // Is it disconnected?
        if ((NO_ERROR == __cs1.dwWNGC3ConnectionStatus) &&
            (WNGC_DISCONNECTED == __cs1.wngcs.dwState))
        {
            // Yes
            fNotConnected = TRUE;
        }
    }

    return !fNotConnected;
#endif
}

// Imported from fsnotify.c
STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);
//
// If a mount point is for a remote path (UNC), it needs to respond
// to shell changes identified by both UNC and local drive path (L:\).
// This function performs this registration.
//
HRESULT CMtPtRemote::ChangeNotifyRegisterAlias(void)
{
    HRESULT hr = E_FAIL;
    
    LPITEMIDLIST pidlLocal = SHSimpleIDListFromPath(_GetName());
    if (NULL != pidlLocal)
    {
        LPITEMIDLIST pidlUNC = SHSimpleIDListFromPath(_GetUNCName());
        if (NULL != pidlUNC)
        {
            SHChangeNotifyRegisterAlias(pidlUNC, pidlLocal);
            ILFree(pidlUNC);
            hr = NOERROR;
        }                
        ILFree(pidlLocal);
    }
    return hr;
}




