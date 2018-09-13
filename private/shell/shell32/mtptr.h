#include "mtpt.h"

class CMtPtRemote : public CMountPoint
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    CMtPtRemote() {}

    // virtual override
    BOOL IsUnavailableNetDrive();
    BOOL IsDisconnectedNetDrive();
    DWORD GetPathSpeed();

    HRESULT GetLabel(LPTSTR pszLabel, DWORD cchLabel, DWORD dwFlags);
    HRESULT SetLabel(LPCTSTR pszLabel);
    int GetSHIDType(BOOL fOKToHitNet);
    HRESULT ChangeNotifyRegisterAlias(void);

///////////////////////////////////////////////////////////////////////////////
// SubDataProvider call backs
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL _GetDriveFlagsCB(PVOID pvData);
    BOOL _GetConnectionStatus1CB(PVOID pvData);
    BOOL _GetConnectionStatus2CB(PVOID pvData);

///////////////////////////////////////////////////////////////////////////////
// Drive cache validity
///////////////////////////////////////////////////////////////////////////////
private:
    void _ResetDriveCache();
    PBYTE _MakeUniqueBlob(DWORD* pcbSize);

///////////////////////////////////////////////////////////////////////////////
// Management
///////////////////////////////////////////////////////////////////////////////
protected:
    void _InvalidateDrive();
    void _InvalidateRefresh();

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
private:
    HRESULT _Initialize(LPCTSTR pszName, BOOL fMountedOnDriveLetter);
    ULONG _ReleaseInternal();

    DWORD _GetExpirationInMilliSec();                                                                                           

    void _CalcPathSpeed();
    HRESULT _GetDefaultUNCDisplayName(LPTSTR pszLabel, DWORD cchLabel);

    LPCTSTR _GetUniqueID();
    void _SetUniqueID();

    LPCTSTR _GetUNCName();
    BOOL _IsConnected();
    BOOL _IsConnectedFromStateVar();

///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
private:
    TCHAR                       _szUniqueID[MAX_PATH];

    ////////////////////////////////////////////////////////////
    // SubData

    // Label from DesktopINI for remote drive (from desktop.ini)
    TCHAR                       __szLabelFromDesktopINI[MAX_LABEL];

    CRSSubData                  _sdLabelFromDesktopINI;
    
    // Connection Status 1
    struct CONNECTIONSTATUS
    {
        DWORD                   dwWNGC3ConnectionStatus;
        WNGC_CONNECTION_STATE   wngcs;
    };

    CONNECTIONSTATUS            __cs1;

    // not a CRSSubData: we don't want to have the cache backed up by the registry
    CSubData                    _sdConnectionStatus1;

    // Connection Status 2
    DWORD                       __dwConnectionStatus2;

    // not a CRSSubData: we don't want to have the cache backed up by the registry
    CSubData                    _sdConnectionStatus2;

    ////////////////////////////////////////////////////////////
    // CacheLevel3: Remote drive stuff

    DWORD                       _dwSpeed;
};