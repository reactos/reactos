//=--------------------------------------------------------------------------=
// jobexec.h
//=--------------------------------------------------------------------------=
// Copyright 1996-1998 Microsoft Corporation.  All Rights Reserved.
//
//
// interface declaration for the JobExecuter object.
//
#ifndef _JOBEXEC_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct
{
   DWORD cbSize;
   DWORD dwKBReqWinDrive;
   DWORD dwKBAvailWinDrive;
   DWORD dwKBReqInstallDrive;
   DWORD dwKBAvailInstallDrive;
   DWORD dwKBReqDownloadDrive;
   DWORD dwKBAvailDownloadDrive;
} CHECKDISKSPACE;

// {7279B780-2867-11d0-A066-00AA00B6015C}
DEFINE_GUID(CLSID_JobExecuter, 0x7279b780, 0x2867, 0x11d0, 0xa0, 0x66, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

// {7279B781-2867-11d0-A066-00AA00B6015C}
DEFINE_GUID(IID_IJobExecuter, 0x7279b782, 0x2867, 0x11d0, 0xa0, 0x66, 0x0, 0xaa, 0x0, 0xb6, 0x1, 0x5c);

#undef INTERFACE
#define INTERFACE IJobExecuter

#define E_PARTIAL                   _HRESULT_TYPEDEF_(0x80100001L)
#define E_ABORTPARTIAL              _HRESULT_TYPEDEF_(0x80100002L)


DECLARE_INTERFACE_(IJobExecuter, IUnknown)
{
   // *** IUnknown methods ***
   STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
   STDMETHOD_(ULONG,Release) (THIS) PURE;

   STDMETHOD(InitializeJob)(THIS_ LPCSTR pszJobName, char chMode) PURE;
   STDMETHOD(SetDownloadDir)(THIS_ LPCSTR pszDownloadDir) PURE;
   STDMETHOD(SetBaseUrls)(THIS_ LPCSTR pszBaseUrl) PURE;
   STDMETHOD(SetInstallDrive)(THIS_ CHAR chDrive) PURE;
   STDMETHOD(SetInstallMode)(THIS_ CHAR chMode) PURE;
   STDMETHOD(SetInstallOptions)(THIS_ DWORD dwInsFlag) PURE;
   STDMETHOD(CheckFreeSpace)(THIS_  CHECKDISKSPACE *pcds) PURE;

   STDMETHOD(DownloadJob)(THIS_ DWORD dwFlags, LPSTR *ppszSuccessStr, LPSTR *ppszErrorStr) PURE;
   STDMETHOD(InstallJob)(THIS_ DWORD dwFlags, DWORD *dwStatus,
                               LPSTR *ppszSuccessStr, LPSTR *ppszErrorStr) PURE;



   STDMETHOD(SetHWND)(THIS_ HWND hForUI) PURE;
   STDMETHOD(SetIStream)(THIS_ IStream *pstm) PURE;
};

// The job key, off of HKCU
#define KEY_JOB  "Software\\Microsoft\\Active Setup\\Jobs"

#define VALUE_BASEURL      "BaseUrl"
#define VALUE_CIFNAME      "InstallList"
#define VALUE_CABNAME      "CabName"
#define VALUE_MODELIST     "InstallModes"
#define VALUE_DEFAULTMODE  "DefaultMode"
#define VALUE_QUIETMODE    "QuietMode"
#define VALUE_LOCALURL     "LocalUrl"

// for support of suspend,resume,abort from outside jobexec.dll
#define VALUE_STATUS      "Status"
#define DATA_SUSPEND       1
#define DATA_RESUME        2
#define DATA_ABORT         3

#ifdef __cplusplus
}
#endif

#define _JOBEXEC_H_
#endif //
