#ifndef _MTPT_H
#define _MTPT_H

#include "rscchvr.h"

#include <dbt.h>

#include "rssdnobf.h"

/////////////////////////////////////////////////////////////////////////////
// Assumptions
/////////////////////////////////////////////////////////////////////////////
// 1- Floppies (3.5" and 5.25") are always FAT
// 2- FAT does not support compression
// 3- DRIVE_CDROM == CDFS or UDF for File system
// 4- CDFS or UDF does not support compression
//
/////////////////////////////////////////////////////////////////////////////

#define MTPT_LABEL_DEFAULT      1
#define MTPT_LABEL_NOFANCY      2

#define MTPT_INV_DRIVE          1
#define MTPT_INV_MEDIA          2
#define MTPT_INV_REFRESH        4
#define MTPT_INV_LABEL          8

// put in shell32\shellprv.h
#define TF_MOUNTPOINT       0x08000000

#define MAX_FILESYSNAME         30
#define MAX_LABEL               33  // MAX_LABEL_NTFS + 1
#define MAX_VOLUMEID            100
#define MAX_DISPLAYNAME         MAX_PATH
#define MAX_MTPTCOMMENT         MAX_PATH

// structure to put information from GetVolumeInformation
typedef struct _tagGVI
{
    TCHAR               szLabel[MAX_LABEL];
    DWORD               dwSerialNumber;
    DWORD               dwMaxlen;
    DWORD               dwFileSysFlags;
    TCHAR               szFileSysName[MAX_FILESYSNAME];
} GVI;

#include "pidl.h"

typedef struct
{
    IDDRIVE idd;
    USHORT  cbNext;
} DRIVE_IDLIST;

class CMountPoint;

typedef void (CMountPoint::*INVALIDATECB)();

class CMountPoint : public CSubDataProvider, protected CRSCacheVersion
{
///////////////////////////////////////////////////////////////////////////////
// Management (mtptmgmt.cpp)
///////////////////////////////////////////////////////////////////////////////
public:
    static CMountPoint* GetMountPoint(LPCTSTR pszMountPoint, BOOL fCreateNew = TRUE);
    static CMountPoint* GetMountPoint(int iDrive, BOOL fCreateNew = TRUE);

    static void InvalidateMountPoint(int iDrive, HDPA hdpaInvalidPaths = NULL,
        DWORD dwFlags = MTPT_INV_DRIVE);
    static void InvalidateMountPoint(LPCTSTR pszName,
        HDPA hdpaInvalidPaths = NULL, DWORD dwFlags = MTPT_INV_DRIVE);
    static void InvalidateAllMtPts();

    // uses the { MTPT_INV_DRIVE | MTPT_INV_MEDIA } flags
    static void SetHasNotif(int iDrive, DWORD dwInvFlags);
    static BOOL WantUI(int iDrive);
    static void SetWantUI(int iDrive);

    static void FinalCleanUp();
    static void FakeVolatileKeys();

    static void InitCriticalHDPA() { InitializeCriticalSection(&_csHDPA);
        _fCritSectHDPAInitialized = TRUE; }

private:
    static CMountPoint* _GetCachedMtPt(int iDrive);
    static int _GetCachedIcon(int iDrive);

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    HRESULT GetDisplayName(LPTSTR pszName, DWORD cchName);

    void GetTypeString(LPTSTR pszType, DWORD cchType);
    DWORD GetClusterSize();

    UINT GetIcon(LPTSTR pszModule, DWORD cchModule);

    int GetDRIVEType(BOOL fOKToHitNet);
    int GetDriveFlags();
    int GetVolumeFlags();

    BOOL GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName);
    BOOL GetFileSystemFlags(DWORD* pdwFlags);

    HKEY GetRegKey();

    BOOL IsRemovable();
    BOOL IsRemote();
    BOOL IsCDROM();
    BOOL IsAudioCD();
    BOOL IsAutoRun();
    BOOL IsDVD();

    BOOL IsNTFS();
    BOOL IsCompressible();
    BOOL IsCompressed();
    BOOL IsSupportingSparseFile();
    BOOL IsContentIndexed();

    // These fns replace the implementation of the old #defines in shlobjp.h
    // (DriveISXXX and IsXXXDrive)
    static BOOL IsSlow(int iDrive);
    static BOOL IsLFN(int iDrive);
    static BOOL IsAutoRun(int iDrive);
    static BOOL IsAutoOpen(int iDrive);
    static BOOL IsShellOpen(int iDrive);
    static BOOL IsAudioCD(int iDrive);
    static BOOL IsNetUnAvail(int iDrive);
    static BOOL IsSecure(int iDrive);
    static BOOL IsCompressed(int iDrive);
    static BOOL IsCompressible(int iDrive);
    static BOOL IsDVD(int iDrive);

    static BOOL IsCDROM(int iDrive);
    static BOOL IsRAMDrive(int iDrive);
    static BOOL IsRemovable(int iDrive);
    static BOOL IsRemote(int iDrive);

    // both
    virtual HRESULT GetLabel(LPTSTR pszLabel, DWORD cchLabel,
        DWORD dwFlags = MTPT_LABEL_DEFAULT) = 0;
    virtual int GetSHIDType(BOOL fOKToHitNet) = 0;
    virtual HRESULT SetLabel(LPCTSTR pszLabel) = 0;
    virtual HRESULT SetDriveLabel(LPCTSTR pszLabel) { return SetLabel(pszLabel); }
    virtual HRESULT ChangeNotifyRegisterAlias(void) = 0;

    // remote only
    virtual BOOL IsUnavailableNetDrive() { return FALSE; }
    virtual BOOL IsDisconnectedNetDrive() { return FALSE; }
    virtual DWORD GetPathSpeed() { return 0; }

    // local only
    virtual HRESULT Eject() { return E_FAIL; }
    virtual BOOL IsFloppy() { return FALSE; }
    virtual BOOL IsAutoRunDrive() { return FALSE; }
    virtual BOOL IsEjectable(BOOL fForceCDROM) { return FALSE; }
    virtual BOOL IsAudioDisc() { return FALSE; }

    static BOOL HandleWMDeviceChange(ULONG_PTR code, DEV_BROADCAST_HDR *pbh);

    static void GetTypeString(BYTE bFlags, LPTSTR pszType, DWORD cchType);

    static BOOL GetDriveIDList(int iDrive, DRIVE_IDLIST *piddl);
    static void SetDriveIDList(int iDrive, DRIVE_IDLIST *piddl);

    HRESULT GetComment(LPTSTR pszComment, DWORD cchComment);
    HRESULT GetHTMLInfoTipFile(LPTSTR pszHTMLInfoTipFile, DWORD cchHTMLInfoTipFile);
    
#ifndef WINNT
    BOOL WantToHide();
#endif

    ULONG AddRef();
    ULONG Release();

    static CRegSupport* GetRSStatic();

///////////////////////////////////////////////////////////////////////////////
// Autorun stuff
///////////////////////////////////////////////////////////////////////////////
private:
    BOOL _IsAutoRun();
    BOOL _IsAutoRunDrive();
    BOOL _ProcessAutoRunFile();

    // Helpers
    void _GetSection(LPCTSTR* ppszSection);
    void _QualifyCommandToDrive(LPTSTR pszCommand);

///////////////////////////////////////////////////////////////////////////////
// Default Icon and Label
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL _ProcessDefaultIconLabel(BOOL* pfDefaultIcon, BOOL* pfDefaultLabel);
    BOOL _HasDefaultIcon();
    BOOL _HasDefaultLabel();
    BOOL _GetDefaultLabel(LPTSTR pszLabel, DWORD cchLabel);

///////////////////////////////////////////////////////////////////////////////
// SubDataProvider call backs
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL _GetFileAttributesCB(PVOID pvData);
    BOOL _GetVolumeInformationCB(PVOID pvData);
    BOOL _AutorunCB(PVOID pvData);
    BOOL _DefaultIconLabelCB(PVOID pvData);
    BOOL _CommentFromDesktopINICB(PVOID pvData);
    BOOL _HTMLInfoTipFileFromDesktopINICB(PVOID pvData);

    virtual BOOL _GetDriveFlagsCB(PVOID pvData) = 0;

///////////////////////////////////////////////////////////////////////////////
// Drive cache validity
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL _IsValidDriveCache();
    virtual void _ResetDriveCache();
    virtual PBYTE _MakeUniqueBlob(DWORD* pcbSize) = 0;
    PBYTE _GetRegUniqueBlob(DWORD cbSize);
    void _ReleaseUniqueBlob(PBYTE pbUniqueBlob);

    void _RSCVDeleteRegCache();
    void _CleanupReg();

///////////////////////////////////////////////////////////////////////////////
// Management (mtptmgmt.cpp)
///////////////////////////////////////////////////////////////////////////////
private:
    //      Drive Letter (DL)
    static CMountPoint* _GetMountPointDL(int iDrive, BOOL fCreateNew);
    static CMountPoint* _GetStoredMtPtDL(int iDrive);
    static BOOL _StoreMtPtDL(CMountPoint** ppMtPt);

    //      Mounted On Folder (MOF)
    static CMountPoint* _GetMountPointMOF(LPTSTR pszPathWithBackslash,
            BOOL fCreateNew);
    static CMountPoint* _GetStoredMtPtMOF(LPTSTR pszPathWithBackslash);
    static BOOL _StoreMtPtMOF(CMountPoint** ppMtPt);
    static BOOL _IsMountedOnFolderMOF(LPCTSTR pszName);

    //      Common to DL and MOF
protected:
    virtual void _InvalidateDrive();
    void _InvalidateLabel();
    virtual void _InvalidateMedia();
    virtual void _InvalidateRefresh();
    BOOL _IsValid();

private:
    static CMountPoint* _CreateMtPt(LPTSTR pszPathWithBackslash,
                               BOOL fMountedOnDriveLetter);
    static void _HandleInvalidate(CMountPoint* pMtPtOriginal,
        INVALIDATECB fctInvalidateCB, HDPA hdpaInvalidPaths);

    //      Helpers
    static BOOL _StripToClosestMountPoint(LPCTSTR pszSource, LPTSTR pszDest,
        DWORD cchDest);
    static void _EnterCriticalHDPA() { if (!_fShuttingDown)
        EnterCriticalSection(&_csHDPA); }
    static void _LeaveCriticalHDPA() { if (!_fShuttingDown)
        LeaveCriticalSection(&_csHDPA); }

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
protected:
    HKEY _GetDriveKey();
    static BOOL _ShowUglyDriveNames();

    HRESULT _InitializeBase(LPCTSTR pszName, BOOL fMountedOnDriveLetter);
    virtual HRESULT _Initialize(LPCTSTR pszName,
        BOOL fMountedOnDriveLetter) = 0;
    virtual DWORD _GetExpirationInMilliSec() = 0;

    virtual LPCTSTR _GetNameForFctCall();

    void _SetKeyName();
    LPCTSTR _GetKeyName();
    static BOOL _GetMountPointKeyName(LPCTSTR pszName, LPTSTR pszKeyName,
                                        DWORD cchKeyName);
    static BOOL _GetMountPointKeyName(int iDrive, LPTSTR pszKeyName,
                                        DWORD cchKeyName);
    static void _CleanupMountPointKeyName();

    BOOL _HasNotif(DWORD dwInvFlags);

    BOOL _GetAttributes(DWORD* pdwAttrib);
    int _GetGVIDriveFlags();
    BOOL _GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel);
    BOOL _GetFileSystemFlags(DWORD* pdwFlags);

    virtual void _SetUniqueID() = 0;
    virtual LPCTSTR _GetVolumeGUID() { return NULL; }
    virtual LPCTSTR _GetUniqueID() = 0;
    TCHAR _GetNameFirstChar();
    LPTSTR _GetNameFirstXChar(LPTSTR pszBuffer, int c);
    LPCTSTR _GetName();

    HANDLE _GetIOCTLHandle(BOOL fFileSystem = FALSE);
    HANDLE _GetIOCTLHandle(DWORD dwDesiredAccess, DWORD dwShareMode);

    CMountPoint();
    virtual ULONG _ReleaseInternal();

#ifdef WINNT
protected:
#else
public:
#endif
    BOOL _DriveIOCTL(int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut, 
        BOOL fFileSystem = FALSE, HANDLE handle = INVALID_HANDLE_VALUE);
///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
protected:
    BOOL                        _fMountedOnDriveLetter;
    TCHAR                       _szName[MAX_PATH];
    TCHAR                       _szKeyName[12];

    BOOL                        _fDriveIDList;
    DRIVE_IDLIST                _DriveIDList;

    ////////////////////////////////////////////////////////////
    // SubData

    ////////////////////////////////////////////////////////////
    // { DRIVE_SHELLOPEN | DRIVE_AUTOOPEN | DRIVE_AUTORUN |     // both
    //   DRIVE_AUDIOCD | DRIVE_DVD |                            // local only 
    //   DRIVE_NETUNAVAIL | DRIVE_SLOW }                        // remote only
    UINT                        __uDriveFlags;

    CRSSubData                  _sdDriveFlags;
    
    ////////////////////////////////////////////////////////////
    // Attributes from the GetFileAttributes call
    DWORD                       __dwGFA;

    CRSSubData                  _sdGFA;

    ////////////////////////////////////////////////////////////
    // GetVolumeInformation
    GVI                         __gvi;

    CRSSubData                  _sdGVI;
    
    ////////////////////////////////////////////////////////////
    // Label from registry (for mix cap label on FAT and remote drives)
    TCHAR                       __szLabelFromReg[MAX_LABEL];

    CRSSubData                  _sdLabelFromReg;

    ////////////////////////////////////////////////////////////
    // Autorun stuff
    BOOL                        __fAutorun;

    CRSSubData                  _sdAutorun;

    ////////////////////////////////////////////////////////////
    // Default Icon and Label
    struct DEFICONLABEL
    {
        BOOL fDefaultIcon;
        BOOL fDefaultLabel;
    };

    DEFICONLABEL                __dil;

    CRSSubData                  _sdDefaultIconLabel;

    ////////////////////////////////////////////////////////////
    // Comment from DesktopINI for drive (from desktop.ini)
    CRSSubDataNoBuffer          _sdCommentFromDesktopINI;

    ////////////////////////////////////////////////////////////
    // HTMLInfoTipFile from DesktopINI for drive (from desktop.ini)
    CRSSubDataNoBuffer          _sdHTMLInfoTipFileFromDesktopINI;

    ////////////////////////////////////////////////////////////
    // { DRIVE_UNKNOWN | DRIVE_NO_ROOT_DIR | DRIVE_REMOVABLE |
    //   DRIVE_FIXED | DRIVE_REMOTE | DRIVE_CDROM | DRIVE_RAMDISK }
    UINT                        _uDriveType;

    ////////////////////////////////////////////////////////////
    // Internal states
    BOOL                        _fUseReg;

private:
    LONG                        _cRef;
    static CRITICAL_SECTION     _csHDPA;
    static BOOL                 _fShuttingDown;

    static HDPA                 _hdpaMountPoints;
    // optimization we have an array for the volumes mounted on drive letters
    static CMountPoint*         _rgMtPtDriveLetter[];
    static BOOL                 _fCritSectHDPAInitialized;

    static CRegSupportCached*   _prsStatic;

protected:
#ifndef WINNT
    static BOOL                 _fWipeVolatileOnWin9X;
#endif

#ifdef DEBUG
    static int                  _cMtPtDL;
    static int                  _cMtPtMOF;
#endif
};

#define DEVPB_DEVTYP_525_0360   0
#define DEVPB_DEVTYP_525_1200   1
#define DEVPB_DEVTYP_350_0720   2
#define DEVPB_DEVTYP_350_1440   7
#define DEVPB_DEVTYP_350_2880   9
#define DEVPB_DEVTYP_FIXED      5
#define DEVPB_DEVTYP_NECHACK    4       // for 3rd FE floppy
#define DEVPB_DEVTYP_350_120M   6

#ifndef WINNT

#define MAX_SEC_PER_TRACK       64
#pragma pack(1)
typedef struct {
    BYTE    SplFunctions;
    BYTE    devType;
    WORD    devAtt;
    WORD    NumCyls;
    BYTE    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    WORD    cbSec;          // Bytes per sector
    BYTE    secPerClus;     // Sectors per cluster
    WORD    cSecRes;                // Reserved sectors
    BYTE    cFAT;           // FATS
    WORD    cDir;           // Root Directory Entries
    WORD    cSec;           // Total number of sectors in image
    BYTE    bMedia;         // Media descriptor
    WORD    secPerFAT;              // Sectors per FAT
    WORD    secPerTrack;    // Sectors per track
    WORD    cHead;          // Heads
    WORD    cSecHidden;     // Hidden sectors
    WORD    cSecHidden_HiWord;      // The high word of no of hidden sectors
    DWORD   cTotalSectors;  // Total sectors, if BPB_cSec is zero
    BYTE    A_BPB_Reserved[6];                       // Unused 6 BPB bytes
    BYTE    TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
} DevPB;
#pragma pack()

#endif

#ifdef __cplusplus
extern "C" {
#endif

HRESULT MountPoint_RegisterChangeNotifyAlias(int iDrive);

#ifdef __cplusplus
}
#endif

#endif //_MTPT_H

/*
MountPointRootKey = HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints

Ex.: For drive "C:\"

    MountPointRootKey\
        + C
            + _Autorun
                + DefaultIcon
                + DefaultLabel
            + _CommentFromDesktopINI
            + _CS1 (remote only)
            + _CS2 (remote only)
            + DefaultIcon
            + DefaultLabel
            + _DriveFlags
            + _GFA
            + _GVI
            + _LabelFromDesktopINI (remote only)
            + _LabelFromReg (from user rename)
            + Shell
                -AutoRun
                + Autorun
                    -Auto&Play
                    + Command
                        -a:\notepad.exe
*/