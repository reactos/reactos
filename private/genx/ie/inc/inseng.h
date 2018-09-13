//=--------------------------------------------------------------------------=
// inseng.h
//=--------------------------------------------------------------------------=
// Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
//
// interface declaration for the InstallEngine control.
//
#ifndef _INSENG_H_

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_ID_LENGTH            48
#define MAX_DISPLAYNAME_LENGTH  128


#define ICI_NOTINSTALLED          0
#define ICI_INSTALLED             1
#define ICI_NEWVERSIONAVAILABLE   2
#define ICI_UNKNOWN               3
#define ICI_OLDVERSIONAVAILABLE   4
#define ICI_NOTINITIALIZED        0xffffffff

#define ABORTINSTALL_NORMAL       0
#define ABORTINSTALL_IMMEADIATE   1


#define ENGINESTATUS_NOTREADY     0
#define ENGINESTATUS_LOADING      1
#define ENGINESTATUS_INSTALLING   2
#define ENGINESTATUS_READY        3

#define CDINSTALL                     1
#define WEBINSTALL                    2
#define WEBINSTALL_DIFFERENTMACHINE   3
#define NETWORKINSTALL                4
#define LOCALINSTALL                  5

#define DEP_NEVER_INSTALL   'N'
#define DEP_INSTALL         'I'


#define SETACTION_NONE            0x00000000
#define SETACTION_INSTALL         0x00000001

#define INSTALLOPTIONS_NOCACHE             0x00000001
#define INSTALLOPTIONS_DOWNLOAD            0x00000002
#define INSTALLOPTIONS_INSTALL             0x00000004
#define INSTALLOPTIONS_DONTALLOWXPLATFORM  0x00000008
#define INSTALLOPTIONS_FORCEDEPENDENCIES    0x00000010

#define EXECUTEJOB_SILENT              0x00000001
#define EXECUTEJOB_DELETE_JOB          0x00000002

#define EXECUTEJOB_VERIFYFILES         0x00000008
#define EXECUTEJOB_IGNORETRUST         0x00000010
#define EXECUTEJOB_IGNOREDOWNLOADERROR 0x00000020
#define EXECUTEJOB_DONTALLOWCANCEL     0x00000040


#define E_FILESMISSING             _HRESULT_TYPEDEF_(0x80100003L)



HRESULT WINAPI CheckTrust(LPCSTR pszFilename, HWND hwndForUI, BOOL bShowBadUI);
HRESULT WINAPI CheckTrustEx(LPCSTR szURL, LPCSTR szFilename, HWND hwndForUI, BOOL bShowBadUI, DWORD dwReserved);
HRESULT WINAPI PurgeDownloadDir(LPCSTR pszDir);
HRESULT WINAPI CheckForVersionConflict();


typedef struct
{
   DWORD cbSize;
   DWORD dwInstallSize;
   DWORD dwWinDriveSize;
   DWORD dwDownloadSize;
   DWORD dwDependancySize;
   DWORD dwInstallDriveReq;
   DWORD dwWinDriveReq;
   DWORD dwDownloadDriveReq;
   CHAR  chWinDrive;
   CHAR  chInstallDrive;
   CHAR  chDownloadDrive;
   DWORD dwTotalDownloadSize;
} COMPONENT_SIZES;

typedef struct
{
   DWORD cbSize;
   DWORD dwDownloadKBRemaining;
   DWORD dwInstallKBRemaining;
   DWORD dwDownloadSecsRemaining;
   DWORD dwInstallSecsRemaining;
} INSTALLPROGRESS;


enum InstallStatus
{
   INSTALLSTATUS_INITIALIZING,
   INSTALLSTATUS_DEPENDENCY,
   INSTALLSTATUS_DOWNLOADING,
   INSTALLSTATUS_COPYING,
   INSTALLSTATUS_RETRYING,
   INSTALLSTATUS_CHECKINGTRUST,
   INSTALLSTATUS_EXTRACTING,
   INSTALLSTATUS_RUNNING,
   INSTALLSTATUS_FINISHED,
   INSTALLSTATUS_DOWNLOADFINISHED
};

// defines for engine problems  (OnEngineProblem)
#define ENGINEPROBLEM_DOWNLOADFAIL   0x00000001


// Actions particular to ENGINEPROBLEM_DOWNLOAD
#define DOWNLOADFAIL_RETRY   0x00000001


#define STOPINSTALL_REBOOTNEEDED   0x00000001
#define STOPINSTALL_REBOOTREFUSED  0x00000002


DEFINE_GUID(IID_IInstallEngineCallback,0x6E449685L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngineCallback

DECLARE_INTERFACE_(IInstallEngineCallback, IUnknown)
{
   // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;


   STDMETHOD(OnEngineStatusChange)(THIS_ DWORD dwEngineStatus, DWORD substatus) PURE;
   STDMETHOD(OnStartInstall)(THIS_ DWORD dwDLSize, DWORD dwInstallSize) PURE;
   STDMETHOD(OnStartComponent)(THIS_ LPCSTR pszID, DWORD dwDLSize, DWORD dwInstallSize, LPCSTR pszString) PURE;
   STDMETHOD(OnComponentProgress)(THIS_ LPCSTR pszID, DWORD dwPhase, LPCSTR pszString, LPCSTR pszMsgString, ULONG progress, ULONG themax) PURE;
   STDMETHOD(OnStopComponent)(THIS_ LPCSTR pszID, HRESULT hError, DWORD dwPhase, LPCSTR pszString, DWORD dwStatus) PURE;
   STDMETHOD(OnStopInstall)(THIS_ HRESULT hrError, LPCSTR szError, DWORD dwStatus) PURE;
   STDMETHOD(OnEngineProblem)(THIS_ DWORD dwEngineProblem, LPDWORD dwAction) PURE;
};



DEFINE_GUID(IID_IInstallEngine,0x6E449684L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngine

DECLARE_INTERFACE_(IInstallEngine , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   // Methods to set engine up for install
   STDMETHOD(GetEngineStatus)(THIS_ DWORD *theenginestatus) PURE;
   STDMETHOD(SetCifFile)(THIS_ LPCSTR pszCabName, LPCSTR pszCifName) PURE;
   STDMETHOD(DownloadComponents)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(InstallComponents)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(EnumInstallIDs)(THIS_ UINT uIndex, LPSTR *ppszID) PURE;
   STDMETHOD(EnumDownloadIDs)(THIS_ UINT uIndex, LPSTR *ppszID) PURE;
   STDMETHOD(IsComponentInstalled)(THIS_ LPCSTR pszID, DWORD *pdwStatus) PURE;
   STDMETHOD(RegisterInstallEngineCallback)(THIS_ IInstallEngineCallback *pcb) PURE;
   STDMETHOD(UnregisterInstallEngineCallback)(THIS) PURE;
   STDMETHOD(SetAction)(THIS_ LPCSTR pszID, DWORD dwAction, DWORD dwPriority) PURE;
   STDMETHOD(GetSizes)(THIS_ LPCSTR pszID, COMPONENT_SIZES *pSizes) PURE;
   STDMETHOD(LaunchExtraCommand)(THIS_ LPCSTR pszInfName, LPCSTR pszSection) PURE;
   STDMETHOD(GetDisplayName)(THIS_ LPCSTR pszID, LPSTR *ppszName) PURE;

   // Info about the install (should be structure to fill in
   //   like GetBindInfo (GetInstallInfo)
   STDMETHOD(SetBaseUrl)(THIS_ LPCSTR pszBaseName) PURE;
   STDMETHOD(SetDownloadDir)(THIS_ LPCSTR pszDownloadDir) PURE;
   STDMETHOD(SetInstallDrive)(THIS_ CHAR chDrive) PURE;
   STDMETHOD(SetInstallOptions)(THIS_ DWORD dwInsFlag) PURE;
   STDMETHOD(SetHWND)(THIS_ HWND hForUI) PURE;
   STDMETHOD(SetIStream)(THIS_ IStream *pstm) PURE;


   // Engine control during installation (seperate interface?)
   STDMETHOD(Abort)(THIS_ DWORD dwFlags) PURE;
   STDMETHOD(Suspend)(THIS) PURE;
   STDMETHOD(Resume)(THIS) PURE;

};

DEFINE_GUID(IID_IInstallEngineTiming,0x6E449687L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngineTiming

DECLARE_INTERFACE_(IInstallEngineTiming , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(GetRates)(THIS_ DWORD *pdwDownload, DWORD *pdwInstall) PURE;
   STDMETHOD(GetInstallProgress)(THIS_ INSTALLPROGRESS *pinsprog) PURE;
};


DEFINE_GUID(CLSID_InstallEngine,0x6E449686L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);


//  The site manager interface

typedef struct
{
   UINT cbSize;
   LPSTR pszLang;
   LPSTR pszRegion;
} SITEQUERYPARAMS;

typedef struct
{
   UINT cbSize;
   LPSTR pszUrl;
   LPSTR pszFriendlyName;
   LPSTR pszLang;
   LPSTR pszRegion;
} DOWNLOADSITE;


// {BFC880F3-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(IID_IDownloadSite,
0xbfc880f3, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#undef INTERFACE
#define INTERFACE IDownloadSite

DECLARE_INTERFACE_(IDownloadSite , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(GetData)(THIS_ DOWNLOADSITE **pds) PURE;
};

// {BFC880F0-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(IID_IDownloadSiteMgr,
0xbfc880f0, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#undef INTERFACE
#define INTERFACE IDownloadSiteMgr

DECLARE_INTERFACE_(IDownloadSiteMgr , IUnknown)
{
     // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(Initialize)(THIS_ LPCSTR pszUrl, SITEQUERYPARAMS *pqp) PURE;
   STDMETHOD(EnumSites)(THIS_ DWORD dwIndex, IDownloadSite **pds) PURE;
};

// {BFC880F1-7484-11d0-8309-00AA00B6015C}
DEFINE_GUID(CLSID_DownloadSiteMgr,
0xbfc880f1, 0x7484, 0x11d0, 0x83, 0x9, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);


// defines for dwUrlFlags
#define URLF_DEFAULT                0x00000000
#define URLF_EXTRACT                0x00000001
#define URLF_RELATIVEURL            0x00000002
#define URLF_DELETE_AFTER_EXTRACT   0x00000004

// types of dependancies
#define DEP_NEVER_INSTALL   'N'
#define DEP_INSTALL         'I'

// platform defines
#define PLATFORM_WIN95              0x00000001
#define PLATFORM_WIN98              0x00000002
#define PLATFORM_NT4                0x00000004
#define PLATFORM_NT5                0x00000008
#define PLATFORM_NT4ALPHA           0x00000010
#define PLATFORM_NT5ALPHA           0x00000020
#define PLATFORM_ALL     PLATFORM_WIN95 | PLATFORM_WIN98 | PLATFORM_NT4 | PLATFORM_NT5 | PLATFORM_NT4ALPHA | PLATFORM_NT5ALPHA
               

// The action to be taken on this component ((Get)SetInstallQueueStatus, SetAction
enum ComponentAction { ActionNone, ActionInstall, ActionUninstall };

// Type for commands
                     //   0            1           2           3             4
enum CommandType     { InfCommand, WExtractExe, Win32Exe, InfExCommand, HRESULTWin32Exe };



#undef INTERFACE
#define INTERFACE ICifComponent

DECLARE_INTERFACE(ICifComponent)
{
   // for properties
   STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize)PURE;
   STDMETHOD(GetGUID)(THIS_ LPSTR pszGUID, DWORD dwSize)PURE;
   STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize)PURE;
   STDMETHOD(GetDetails)(THIS_ LPSTR pszDetails, DWORD dwSize) PURE;
   STDMETHOD(GetUrl)(THIS_ UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags)  PURE;
   STDMETHOD(GetFileExtractList)(THIS_ UINT uUrlNum, LPSTR pszExtract, DWORD dwSize)  PURE;
   STDMETHOD(GetUrlCheckRange)(THIS_ UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax)  PURE;
   STDMETHOD(GetCommand)(THIS_ UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches, 
                         DWORD dwSwitchSize, LPDWORD pdwType)  PURE;
   STDMETHOD(GetVersion)(THIS_ LPDWORD pdwVersion, LPDWORD pdwBuild)  PURE;
   STDMETHOD(GetLocale)(THIS_ LPSTR pszLocale, DWORD dwSize)  PURE;
   STDMETHOD(GetUninstallKey)(THIS_ LPSTR pszKey, DWORD dwSize)  PURE;
   STDMETHOD(GetInstalledSize)(THIS_ LPDWORD pdwWin, LPDWORD pdwApp)  PURE;
   STDMETHOD_(DWORD, GetDownloadSize)(THIS)  PURE;
   STDMETHOD_(DWORD, GetExtractSize)(THIS)  PURE;
   STDMETHOD(GetSuccessKey)(THIS_ LPSTR pszKey, DWORD dwSize)  PURE;
   STDMETHOD(GetProgressKeys)(THIS_ LPSTR pszProgress, DWORD dwProgSize, 
                              LPSTR pszCancel, DWORD dwCancelSize)  PURE;
   STDMETHOD(IsActiveSetupAware)(THIS)  PURE;
   STDMETHOD(IsRebootRequired)(THIS)  PURE;
   STDMETHOD(RequiresAdminRights)(THIS) PURE;
   STDMETHOD_(DWORD, GetPriority)(THIS)  PURE;
   STDMETHOD(GetDependency)(THIS_ UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild)  PURE;
   STDMETHOD_(DWORD, GetPlatform)(THIS)  PURE;
   STDMETHOD(GetMode)(THIS_ UINT uModeNum, LPSTR pszMode, DWORD dwSize)  PURE;
   STDMETHOD(GetGroup)(THIS_ LPSTR pszID, DWORD dwSize)  PURE;
   STDMETHOD(IsUIVisible)(THIS)  PURE;
   STDMETHOD(GetPatchID)(THIS_ LPSTR pszID, DWORD dwSize)  PURE;
   STDMETHOD(GetDetVersion)(THIS_ LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize) PURE;
   STDMETHOD(GetTreatAsOneComponents)(THIS_ UINT uNum, LPSTR pszID, DWORD dwBuf) PURE;
   STDMETHOD(GetCustomData)(LPSTR pszKey, LPSTR pszData, DWORD dwSize) PURE;

   // access to state
   STDMETHOD_(DWORD, IsComponentInstalled)(THIS)  PURE;
   STDMETHOD(IsComponentDownloaded)(THIS)  PURE;
   STDMETHOD_(DWORD, IsThisVersionInstalled)(THIS_ DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild) PURE;
   STDMETHOD_(DWORD, GetInstallQueueState)(THIS)  PURE;
   STDMETHOD(SetInstallQueueState)(THIS_ DWORD dwState)  PURE;
   STDMETHOD_(DWORD, GetActualDownloadSize)(THIS)  PURE;
   STDMETHOD_(DWORD, GetCurrentPriority)(THIS) PURE;
   STDMETHOD(SetCurrentPriority)(THIS_ DWORD dwPriority) PURE;
};

DECLARE_INTERFACE_(ICifRWComponent, ICifComponent)
{
   STDMETHOD(SetGUID)(THIS_ LPCSTR pszGUID)PURE;
   STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc)PURE;
   STDMETHOD(SetUrl)(THIS_ UINT uUrlNum, LPCSTR pszUrl, DWORD dwUrlFlags)  PURE;
   STDMETHOD(SetCommand)(THIS_ UINT uCmdNum, LPCSTR pszCmd, LPCSTR pszSwitches, DWORD dwType)  PURE;
   STDMETHOD(SetVersion)(THIS_ LPCSTR pszVersion)  PURE;
   STDMETHOD(SetUninstallKey)(THIS_ LPCSTR pszKey)  PURE;
   STDMETHOD(SetInstalledSize)(THIS_ DWORD dwWin, DWORD dwApp)  PURE;
   STDMETHOD(SetDownloadSize)(THIS_ DWORD)  PURE;
   STDMETHOD(SetExtractSize)(THIS_ DWORD)  PURE;
   STDMETHOD(DeleteDependency)(THIS_ LPCSTR pszID, char chType)  PURE;
   STDMETHOD(AddDependency)(THIS_ LPCSTR pszID, char chType)  PURE;
   STDMETHOD(SetUIVisible)(THIS_ BOOL)  PURE;
   STDMETHOD(SetGroup)(THIS_ LPCSTR pszID)  PURE;
   STDMETHOD(SetPlatform)(THIS_ DWORD)  PURE;
   STDMETHOD(SetPriority)(THIS_ DWORD)  PURE;
   STDMETHOD(SetReboot)(THIS_ BOOL)  PURE;
   
   STDMETHOD(DeleteFromModes)(THIS_ LPCSTR pszMode)  PURE;
   STDMETHOD(AddToMode)(THIS_ LPCSTR pszMode)  PURE;
   STDMETHOD(SetModes)(THIS_ LPCSTR pszMode)  PURE;
   STDMETHOD(CopyComponent)(THIS_ LPCSTR pszCifFile)  PURE;
   STDMETHOD(AddToTreatAsOne)(THIS_ LPCSTR pszCompID)  PURE;
   STDMETHOD(SetDetails)(THIS_ LPCSTR pszDesc) PURE;
};

DECLARE_INTERFACE_(IEnumCifComponents, IUnknown)
{
  // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;
   
  // enum methods
   STDMETHOD(Next)(THIS_ ICifComponent **) PURE;
   STDMETHOD(Reset)(THIS) PURE;
};

DECLARE_INTERFACE(ICifGroup)
{
  // for properties
   STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize) PURE;
   STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize) PURE;
   STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
  
   STDMETHOD(EnumComponents)(THIS_ IEnumCifComponents **, DWORD dwFilter, LPVOID pv) PURE;
   STDMETHOD_(DWORD, GetCurrentPriority)(THIS) PURE;

};

DECLARE_INTERFACE_(ICifRWGroup, ICifGroup)
{
   STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc) PURE;
   STDMETHOD(SetPriority)(THIS_ DWORD) PURE;
   STDMETHOD(SetDetails)(THIS_ LPCSTR pszDetails) PURE;
};

DECLARE_INTERFACE_(IEnumCifGroups, IUnknown)
{
  // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;
   
  // enum methods
   STDMETHOD(Next)(THIS_ ICifGroup **) PURE;
   STDMETHOD(Reset)(THIS) PURE;
};

DECLARE_INTERFACE(ICifMode)
{
  // for properties
   STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize) PURE;
   STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize) PURE;
   STDMETHOD(GetDetails)(THIS_ LPSTR pszDetails, DWORD dwSize) PURE;
  
   STDMETHOD(EnumComponents)(THIS_ IEnumCifComponents **, DWORD dwFilter, LPVOID pv) PURE;
};

DECLARE_INTERFACE_(ICifRWMode, ICifMode)
{
   STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc) PURE;
   STDMETHOD(SetDetails)(THIS_ LPCSTR pszDetails) PURE;
};

DECLARE_INTERFACE_(IEnumCifModes, IUnknown)
{
  // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;
   
  // enum methods
   STDMETHOD(Next)(THIS_ ICifMode **) PURE;
   STDMETHOD(Reset)(THIS) PURE;
};

DEFINE_GUID(IID_ICifFile,0x6E449688L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

DECLARE_INTERFACE_(ICifFile, IUnknown)
{
 // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;
 
   STDMETHOD(EnumComponents)(THIS_ IEnumCifComponents **, DWORD dwFilter, LPVOID pv) PURE;
   STDMETHOD(FindComponent)(THIS_ LPCSTR pszID, ICifComponent **p) PURE;

   STDMETHOD(EnumGroups)(THIS_ IEnumCifGroups **, DWORD dwFilter, LPVOID pv) PURE;
   STDMETHOD(FindGroup)(THIS_ LPCSTR pszID, ICifGroup **p) PURE;

   STDMETHOD(EnumModes)(THIS_ IEnumCifModes **, DWORD dwFilter, LPVOID pv) PURE;
   STDMETHOD(FindMode)(THIS_ LPCSTR pszID, ICifMode **p) PURE;

   STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize) PURE;
   STDMETHOD(GetDetDlls)(THIS_ LPSTR pszDlls, DWORD dwSize) PURE;

};

DECLARE_INTERFACE_(ICifRWFile, ICifFile)
{
   STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc) PURE;    
   STDMETHOD(CreateComponent)(THIS_ LPCSTR pszID, ICifRWComponent **p) PURE;
   STDMETHOD(CreateGroup)(THIS_ LPCSTR pszID, ICifRWGroup **p) PURE;
   STDMETHOD(CreateMode)(THIS_ LPCSTR pszID, ICifRWMode **p) PURE;
   STDMETHOD(DeleteComponent)(THIS_ LPCSTR pszID) PURE;
   STDMETHOD(DeleteGroup)(THIS_ LPCSTR pszID) PURE;
   STDMETHOD(DeleteMode)(THIS_ LPCSTR pszID) PURE;
   STDMETHOD(Flush)(THIS) PURE;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Detection DLL 

// Returns from DetectVersion
#define DET_NOTINSTALLED          0
#define DET_INSTALLED             1
#define DET_NEWVERSIONINSTALLED   2
#define DET_OLDVERSIONINSTALLED   3


   
// Function prototype
typedef struct
{
   DWORD          dwSize;
   LPDWORD        pdwInstalledVer;
   LPDWORD        pdwInstalledBuild;
   LPSTR          pszGUID;
   LPSTR          pszLocale;
   DWORD          dwAskVer;
   DWORD          dwAskBuild;
   ICifFile      *pCifFile; 
   ICifComponent *pCifComp; 
} DETECTION_STRUCT;


typedef DWORD (WINAPI *DETECTVERSION)(DETECTION_STRUCT *pDetectionStruct);

///////////////////////////////////////////////////////////////////////////////////////////////////////////



DEFINE_GUID(IID_IInstallEngine2,0x6E449689L,0xC509,0x11CF,0xAA,0xFA,0x00,0xAA,0x00,0xB6,0x01,0x5C);

#undef INTERFACE
#define INTERFACE IInstallEngine2

DECLARE_INTERFACE_(IInstallEngine2 , IInstallEngine)
{
   STDMETHOD(SetLocalCif)(THIS_ LPCSTR pszCif) PURE;
   STDMETHOD(GetICifFile)(THIS_ ICifFile **picif) PURE;
};

HRESULT WINAPI GetICifFileFromFile(ICifFile **, LPCSTR pszFile);

HRESULT WINAPI GetICifRWFileFromFile(ICifRWFile **, LPCSTR pszFile);

#ifdef __cplusplus
}
#endif

#define _INSENG_H_
#endif //
