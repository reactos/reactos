#include "shellprv.h"
#pragma  hdrstop

#include "volinfo.h"

#define REGSTR_VOLINFO_ROOTKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeInfo")

HDPA                CVolumeInfo::_hdpaVolumeInfos = NULL;
HKEY                CVolumeInfo::_hkeyStaticRoot = NULL;
CRITICAL_SECTION    CVolumeInfo::_csHDPA = {0};
BOOL                CVolumeInfo::_fCritSectHDPAInitialized = FALSE;
BOOL                CVolumeInfo::_fShuttingDown = FALSE;

//static
CVolumeInfo* CVolumeInfo::GetVolumeInfo(LPTSTR pszVolumeID, LPTSTR pszName,
                                    DWORD dwMaxAgeInMilliSec, GVI* pgvi)
{
    CVolumeInfo* pVolInfo = _GetVolumeInfoHelper(pszVolumeID, pszName, pgvi);

    if (pVolInfo && (pVolInfo->_IsExpired(dwMaxAgeInMilliSec)))
    {
        TraceMsg(TF_MOUNTPOINT, 
            "CVolumeInfo::GetColumeInfo: Expired for Name='%s' and VolumeID='%s'", pszName, pszVolumeID);

        CVolumeInfo::InvalidateVolumeInfo(pszVolumeID);

        pVolInfo = _GetVolumeInfoHelper(pszVolumeID, pszName, pgvi);

        // we don't check the expiration this time to avoid infinite recursion,
        // if for any reason this operation is longer than dwMaxAgeInMilliSec
    }

    // We AddRef before giving it to caller.  This way we have "cRef >= 2".  When the caller
    // will release then we'll still have "cRef == 1", so that the object is not destroyed
    // and remains in the cache
    if (pVolInfo)
        pVolInfo->AddRef();

    return pVolInfo;
}

//static
void CVolumeInfo::InvalidateVolumeInfo(LPTSTR pszVolumeID)
{
    CVolumeInfo* pVolInfo = NULL;

    if (_hdpaVolumeInfos)
    {
        _EnterCriticalHDPA();

        int n = DPA_GetPtrCount(_hdpaVolumeInfos);

        for (int i = 0; i < n; ++i)
        {
            pVolInfo = (CVolumeInfo*)DPA_GetPtr(_hdpaVolumeInfos, i);
        
            if (pVolInfo)
            {
                if (!lstrcmpi(pVolInfo->_szVolumeID, pszVolumeID))
                {
                    // Doing this we increment the version number in the reg.  This
                    // way next time a GetVolumeInfo is done (in any process) the check
                    // of the version in the VolInfo object will fail and the object will
                    // be removed from the cache and a new one will be created
                    pVolInfo->_RSCVIncrementRegVersion();

                    break;
                }
            }
        }
        _LeaveCriticalHDPA();
    }
}

//static
void CVolumeInfo::FinalCleanUp()
{
    if (_fCritSectHDPAInitialized)
    {
        _EnterCriticalHDPA();

        if (_hdpaVolumeInfos)
        {
            int n = DPA_GetPtrCount(_hdpaVolumeInfos);

            for (int i = 0; i < n; ++i)
            {
                CVolumeInfo* pVolInfo = (CVolumeInfo*)DPA_GetPtr(_hdpaVolumeInfos, i);
        
                if (pVolInfo)
                {
                    pVolInfo->Release();
                }
            }

            DPA_Destroy(_hdpaVolumeInfos);

            _hdpaVolumeInfos = NULL;
        }

        _fShuttingDown = TRUE;

        _LeaveCriticalHDPA();

        DeleteCriticalSection(&_csHDPA);
    }
}

HRESULT CVolumeInfo::GetFromReg(LPCTSTR pszValue, LPTSTR psz, DWORD cch)
{
    HRESULT hres = S_OK;

    if(!RSGetTextValue(NULL, pszValue, psz, &cch))
    {
        if (cch > 0)
            *psz = 0;

        hres = E_FAIL;
    }

    return hres;
}

HRESULT CVolumeInfo::SetIntoReg(LPCTSTR pszValue, LPCTSTR psz)
{
    return (RSSetTextValue(NULL, pszValue, psz, REG_OPTION_NON_VOLATILE) ?
                S_OK : E_FAIL);
}

BOOL CVolumeInfo::ExistInReg(LPCTSTR pszValue)
{
    return RSValueExist(NULL, pszValue);
}

CVolumeInfo::CVolumeInfo() : _cRef(1)
{
}

ULONG CVolumeInfo::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CVolumeInfo::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Require GVI
/////////////////////////////////////////////////////////////////////////////
BOOL CVolumeInfo::GetGVILabel(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet = FALSE;

    if (SUCCEEDED(_InitializeGVI()))
    {
        StrCpyN(pszLabel, _Cache.gvi.szLabel, cchLabel);
        fRet = TRUE;
    }

    return fRet;
}

BOOL CVolumeInfo::IsNTFS()
{
    BOOL fRet = FALSE;

    if (SUCCEEDED(_InitializeGVI()))
    {
        fRet = BOOLFROMPTR(StrStr(TEXT("NTFS"), _Cache.gvi.szFileSysName));
    }

    return fRet;
}

BOOL CVolumeInfo::GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName)
{
    BOOL fRet = FALSE;

    if (SUCCEEDED(_InitializeGVI()))
    {
        lstrcpyn(pszFileSysName, _Cache.gvi.szFileSysName, cchFileSysName);
        fRet = TRUE;
    }

    return fRet;
}

BOOL CVolumeInfo::GetFileSystemFlags(DWORD* pdwFlags)
{
    BOOL fRet = FALSE;

    if (SUCCEEDED(_InitializeGVI()))
    {
        *pdwFlags = _Cache.gvi.dwFileSysFlags;
        fRet = TRUE;
    }

    return fRet;
}

BOOL CVolumeInfo::GetDriveFlags(int* piFlags)
{
    BOOL fRet = FALSE;    

    if (SUCCEEDED(_InitializeGVI()))
    {
        *piFlags = _Calc.uDriveFlags;
        fRet = TRUE;
    }

    return fRet;
}
/////////////////////////////////////////////////////////////////////////////
// 
/////////////////////////////////////////////////////////////////////////////
//static
CVolumeInfo* CVolumeInfo::_GetVolumeInfoHelper(LPTSTR pszVolumeID, LPTSTR pszName, GVI* pgvi)
{
    CVolumeInfo* pVolInfo = NULL;
    
    if (_hdpaVolumeInfos && !_fShuttingDown)
    {
         _EnterCriticalHDPA();

        int n = DPA_GetPtrCount(_hdpaVolumeInfos);

        for (int i = 0; i < n; ++i)
        {
            pVolInfo = (CVolumeInfo*)DPA_GetPtr(_hdpaVolumeInfos, i);
        
            if (pVolInfo)
            {
                if (!lstrcmpi(pVolInfo->_szVolumeID, pszVolumeID))
                {
                    if (!pVolInfo->_RSCVIsValidVersion())
                    {
                        DPA_DeletePtr(_hdpaVolumeInfos, i);

                        pVolInfo->Release();

                        pVolInfo = NULL;
                    }
                    break;
                }
                else
                    pVolInfo = NULL;
            }
        }
        _LeaveCriticalHDPA();
    }

    if (!pVolInfo)
    {
        TCHAR szPathWithBackslash[MAX_PATH];

        lstrcpyn(szPathWithBackslash, pszName, ARRAYSIZE(szPathWithBackslash));
        PathAddBackslash(szPathWithBackslash);

        pVolInfo = new CVolumeInfo();

        if (pVolInfo)
        {
            TraceMsg(TF_MOUNTPOINT,
                "    static CVolumeInfo::_GetVolumeInfoHelper: Created for Name='%s'",
                pszName);

            pVolInfo->_Initialize(pszVolumeID, pszName, pgvi);

            if (!_hdpaVolumeInfos)
            {
                if (!_fCritSectHDPAInitialized)
                {
                    InitializeCriticalSection(&_csHDPA);
                    _fCritSectHDPAInitialized = TRUE;
                }

                _EnterCriticalHDPA();

                if (!_hdpaVolumeInfos && !_fShuttingDown)
                    _hdpaVolumeInfos = DPA_Create(4);

                _LeaveCriticalHDPA();
            }

            if (!_hdpaVolumeInfos || (-1 == DPA_AppendPtr(_hdpaVolumeInfos, pVolInfo)))
            {
                pVolInfo->Release();

                pVolInfo = NULL;
            }
        }
    }

    return pVolInfo;
}

HRESULT CVolumeInfo::_UpdateCache()
{
    HRESULT hres = S_OK;

    TraceMsg(TF_MOUNTPOINT, "    CVolumeInfo::_UpdateCache: for Name='%s'", _szName);

    TraceMsg(TF_MOUNTPOINT, 
        "    CVolumeInfo::_UpdateCache: >>>EXT<<< (GetVolumeInformation) '%s'", _szName);

    if (!GetVolumeInformation(_szName, _Cache.gvi.szLabel, ARRAYSIZE(_Cache.gvi.szLabel),
            &_Cache.gvi.dwSerialNumber, &_Cache.gvi.dwMaxlen, &_Cache.gvi.dwFileSysFlags,
            _Cache.gvi.szFileSysName, ARRAYSIZE(_Cache.gvi.szFileSysName)))
    {
        hres = S_FALSE;
    }

    return hres;
}

HRESULT CVolumeInfo::_UpdateCalcValue()
{
    // If this is a drive which supports compression, we go off to find out
    // if the root is compressed

    TraceMsg(TF_MOUNTPOINT, "    CVolumeInfo::_UpdateCalcValue: for Name='%s'", _szName);

    // Volume supports compression?
    if (_Cache.gvi.dwFileSysFlags & FS_FILE_COMPRESSION)
    {
        _Calc.uDriveFlags |= DRIVE_ISCOMPRESSIBLE;
    }

    // Volume supports long filename (greater than 8.3)?
    if (_Cache.gvi.dwMaxlen > 12)
        _Calc.uDriveFlags |= DRIVE_LFN;

    // Volume supports security?
    if (_Cache.gvi.dwFileSysFlags & FS_PERSISTENT_ACLS)
        _Calc.uDriveFlags |= DRIVE_SECURITY;

    return S_OK;
}

HRESULT CVolumeInfo::_InitializeGVI(GVI* pgvi)
{
    if (!_fGVIInitialized)
    {
        if (pgvi)
        {
            // Yes, use it to update the registry
            _Cache.gvi = *pgvi;

            _UpdateRegCache();

            _fGVIInitialized = TRUE;
        }
        else
        {
            DWORD cb = sizeof(_Cache);

            // Do we have something in the registry?
            _RSCVUpdateVersionOnCacheRead();

            if (!RSGetBinaryValue(TEXT("Cache"), NULL, (PBYTE)&_Cache, &cb))
            {
                // No, then update the cache by calling the appropriate system fct
                _UpdateCache();

                _UpdateRegCache();
            }

            _fGVIInitialized = TRUE;
        }

        _UpdateCalcValue();
    }

    return S_OK;
}

HRESULT CVolumeInfo::_Initialize(LPTSTR pszVolumeID, LPTSTR pszName, GVI* pgvi)
{
    HRESULT hres = E_INVALIDARG;

    if (pszVolumeID && pszName)
    {
        TraceMsg(TF_MOUNTPOINT, 
            "    CVolumeInfo::_Initialize: for Name='%s' and VolumeID='%s'", pszName, pszVolumeID);

        lstrcpyn(_szName, pszName, ARRAYSIZE(_szName));
        PathAddBackslash(_szName);

        RSInitRoot(HKEY_CURRENT_USER, REGSTR_VOLINFO_ROOTKEY, pszVolumeID,
            REG_OPTION_NON_VOLATILE);

        lstrcpyn(_szVolumeID, pszVolumeID, ARRAYSIZE(_szVolumeID));

        // Did we receive a GVI struct, local mtpt need it to build volumeid
        if (pgvi)
            _InitializeGVI(pgvi);

        hres = S_OK;
    }

    return hres;
}

BOOL CVolumeInfo::_IsExpired(DWORD dwMaxAgeInMilliSec)
{
    DWORD dwCurrentTick = GetTickCount();
    BOOL fRet = FALSE;

    if ((0xFFFFFFFF != dwMaxAgeInMilliSec) && 
        (!_dwCreationTick ||
        (_dwCreationTick > dwCurrentTick) ||
        ((_dwCreationTick - dwCurrentTick) > dwMaxAgeInMilliSec)))
    {
        fRet = TRUE;
    }

    return fRet;
}

void CVolumeInfo::_RSCVDeleteRegCache()
{
    TraceMsg(TF_MOUNTPOINT, "    CVolumeInfo::_RSCVDeleteRegCache: Name='%s'", _szName);

    RSDeleteSubKey(TEXT("Cache"));
}

void CVolumeInfo::_UpdateRegCache()
{
    _RSCVUpdateVersionOnCacheWrite();

    RSSetBinaryValue(TEXT("Cache"), NULL, (PBYTE)&_Cache, sizeof(_Cache));

    _dwCreationTick = GetTickCount();

    RSSetDWORDValue(NULL, TEXT("CreationTick"), _dwCreationTick);
}

STDAPI_(void) CVolInfo_FinalCleanUp()
{
    CVolumeInfo::FinalCleanUp();
}
