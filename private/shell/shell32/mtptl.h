#include "mtpt.h"

class CMtPtLocal : public CMountPoint
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    CMtPtLocal() {}

    // virtual override
    HRESULT Eject();
    BOOL IsFloppy();
    BOOL IsEjectable(BOOL fForceCDROM);
    BOOL IsAudioDisc();

    HRESULT GetLabel(LPTSTR pszLabel, DWORD cchLabel, DWORD dwFlags);
    HRESULT SetLabel(LPCTSTR pszLabel);
    HRESULT SetDriveLabel(LPCTSTR pszLabel);
    int GetSHIDType(BOOL fOKToHitNet);
    HRESULT ChangeNotifyRegisterAlias(void)
        { /* no-op */ return NOERROR; }

///////////////////////////////////////////////////////////////////////////////
// SubDataProvider call backs
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL _GetDriveFlagsCB(PVOID pvData);

///////////////////////////////////////////////////////////////////////////////
// Management
///////////////////////////////////////////////////////////////////////////////
protected:
    void _InvalidateMedia();
    void _InvalidateRefresh();

///////////////////////////////////////////////////////////////////////////////
// Drive cache validity
///////////////////////////////////////////////////////////////////////////////
private:
    PBYTE _MakeUniqueBlob(DWORD* pcbSize);

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
private:
    HRESULT _Initialize(LPCTSTR pszName, BOOL fMountedOnDriveLetter);

    LPCTSTR _GetUniqueID();
    void _SetUniqueID();

    LPCTSTR _GetNameForFctCall();
    LPCTSTR _GetVolumeGUID();

    DWORD _GetExpirationInMilliSec();

    BOOL _IsAudioDisc();
    BOOL _IsDVDDisc();
    BYTE _CalcFloppyType();

    HRESULT _Eject(LPTSTR pszMountPointNameForError);
#ifdef WINNT
    HANDLE _Lock(LPTSTR pszMountPointNameForError, BOOL* pfAborted,
        BOOL* pfFailed);
#endif

///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
private:
    // On NT4/Win9X: max will be "a:\"
    // On NT5 it's a Volume GUID (50)
    TCHAR                       _szUniqueID[50];

    BYTE                        _bFloppyType;
};
