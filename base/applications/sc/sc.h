#ifndef _SC_PCH_
#define _SC_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <sddl.h>
#include <tchar.h>

#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>


#define SCDBG

typedef struct
{
    LPCTSTR lpServiceName;
    LPCTSTR lpDisplayName;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    LPCTSTR lpBinaryPathName;
    LPCTSTR lpLoadOrderGroup;
    DWORD dwTagId;
    LPCTSTR lpDependencies;
    LPCTSTR lpServiceStartName;
    LPCTSTR lpPassword;

    BOOL bTagId;
} SERVICE_CREATE_INFO, *LPSERVICE_CREATE_INFO;


/* control functions */
BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount);
BOOL Create(LPCTSTR *ServiceArgs, INT ArgCount);
BOOL Delete(LPCTSTR ServiceName);
BOOL Control(DWORD Control, LPCTSTR ServiceName, LPCTSTR *Args, INT ArgCount);
BOOL Query(LPCTSTR *ServiceArgs, DWORD ArgCount, BOOL bExtended);

LPSERVICE_STATUS_PROCESS QueryService(LPCTSTR ServiceName);
BOOL SdShow(LPCTSTR ServiceName);
BOOL SdSet(LPCTSTR ServiceName, LPCTSTR SecurityDescriptor);
BOOL QueryConfig(LPCTSTR ServiceName);
BOOL SetConfig(LPCTSTR *ServiceArgs, INT ArgCount);
BOOL QueryDescription(LPCTSTR ServiceName);
BOOL SetDescription(LPCTSTR ServiceName, LPCTSTR Description);
BOOL QueryFailure(LPCTSTR ServiceName);
BOOL SetFailure(LPCTSTR *ServiceArgs, INT ArgCount);
BOOL GetDisplayName(LPCTSTR ServiceName);
BOOL GetKeyName(LPCTSTR ServiceName);
BOOL EnumDepend(LPCTSTR ServiceName);

/* print and error functions */
VOID PrintService(LPCTSTR ServiceName, LPSERVICE_STATUS_PROCESS pStatus, BOOL bExtended);
VOID PrintServiceStatus(LPSERVICE_STATUS pStatus);
VOID ReportLastError(VOID);

/* misc.c */
BOOL
ParseCreateConfigArguments(
    LPCTSTR *ServiceArgs,
    INT ArgCount,
    BOOL bChangeService,
    OUT LPSERVICE_CREATE_INFO lpServiceInfo);

BOOL
ParseFailureArguments(
    LPCTSTR *ServiceArgs,
    INT ArgCount,
    OUT LPCTSTR *ppServiceName,
    OUT LPSERVICE_FAILURE_ACTIONS pFailureActions);

/* usage functions */
VOID MainUsage(VOID);
VOID StartUsage(VOID);
VOID PauseUsage(VOID);
VOID InterrogateUsage(VOID);
VOID ContinueUsage(VOID);
VOID StopUsage(VOID);
VOID DeleteUsage(VOID);
VOID CreateUsage(VOID);
VOID ControlUsage(VOID);
VOID SdShowUsage(VOID);
VOID SdSetUsage(VOID);
VOID QueryConfigUsage(VOID);
VOID QueryDescriptionUsage(VOID);
VOID QueryFailureUsage(VOID);
VOID SetDescriptionUsage(VOID);
VOID SetConfigUsage(VOID);
VOID SetFailureUsage(VOID);
VOID GetDisplayNameUsage(VOID);
VOID GetKeyNameUsage(VOID);
VOID EnumDependUsage(VOID);

#endif /* _SC_PCH_ */
