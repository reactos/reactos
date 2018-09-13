#ifndef _VOLINFO_H
#define _VOLINFO_H

#include "rscchvr.h"

#define MAX_FILESYSNAME         30
#define MAX_LABEL               33  // MAX_LABEL_NTFS + 1
#define MAX_VOLUMEID            100
#define MAX_DISPLAYNAME         MAX_PATH
#define MAX_MTPTCOMMENT         MAX_PATH

// put in shell32\shellprv.h
#define TF_MOUNTPOINT 0x02000000

// structure to put information from GetVolumeInformation
typedef struct _tagGVI
{
    TCHAR               szLabel[MAX_LABEL];
    DWORD               dwSerialNumber;
    DWORD               dwMaxlen;
    DWORD               dwFileSysFlags;
    TCHAR               szFileSysName[MAX_FILESYSNAME];
} GVI;

class CVolumeInfo : private CRSCacheVersion
{
public:
    CVolumeInfo();

    ULONG AddRef();
    ULONG Release();

public:
    BOOL GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName);
    BOOL GetFileSystemFlags(DWORD* pdwFlags);
    BOOL GetGVILabel(LPTSTR pszLabel, DWORD cchLabel);

    BOOL GetDriveFlags(int* piFlags);

    BOOL IsNTFS();
   
    static void InvalidateVolumeInfo(LPTSTR pszVolumeID);

    static CVolumeInfo* GetVolumeInfo(LPTSTR pszVolumeID, LPTSTR pszName,
            DWORD dwMaxAgeInMilliSec, GVI* pgvi = NULL);
    
    static void FinalCleanUp();

    HRESULT GetFromReg(LPCTSTR pszValue, LPTSTR psz, DWORD cch);
    HRESULT SetIntoReg(LPCTSTR pszValue, LPCTSTR psz);
    BOOL ExistInReg(LPCTSTR pszValue);

private:
    static CVolumeInfo* _GetVolumeInfoHelper(LPTSTR pszVolumeID, LPTSTR pszName,
            GVI* pgvi);
    HRESULT _Initialize(LPTSTR pszVolumeID, LPTSTR pszName, GVI* pgvi);
    HRESULT _InitializeGVI(GVI* pgvi = NULL);

    HRESULT _UpdateCache();
    HRESULT _UpdateCalcValue();
    void _UpdateRegCache();

    virtual void _RSCVDeleteRegCache();

    BOOL _IsExpired(DWORD dwMaxAgeInMilliSec);

    static void _EnterCriticalHDPA() { EnterCriticalSection(&_csHDPA); }
    static void _LeaveCriticalHDPA() { LeaveCriticalSection(&_csHDPA); }

    TCHAR                       _szName[MAX_PATH];
    TCHAR                       _szVolumeID[MAX_VOLUMEID];
    BOOL                        _fRegCacheOK;
    DWORD                       _dwCreationTick;
    BOOL                        _fGVIInitialized;

private:
    ////////////////////////////////////////////////////////////
    // CacheLevel2: Might hit the Net
    struct _tagCache
    {
        // From GetVolumeInformation call
        GVI                     gvi;
    } _Cache;

    ////////////////////////////////////////////////////////////
    // "Calculated" values
    struct _tagCalc
    {
        // { DRIVE_ISCOMPRESSIBLE | DRIVE_LFN | DRIVE_SECURITY }
        UINT                    uDriveFlags;
    } _Calc;

private:
    static HDPA                 _hdpaVolumeInfos;
    static HKEY                 _hkeyStaticRoot;
    static CRITICAL_SECTION     _csHDPA;
    static BOOL                 _fCritSectHDPAInitialized;
    static BOOL                 _fShuttingDown;

    LONG                        _cRef;
};

#endif //_VOLINFO_H