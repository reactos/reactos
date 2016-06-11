#ifndef _SC_PCH_
#define _SC_PCH_

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <sddl.h>
#include <tchar.h>

#define SCDBG

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
BOOL QueryDescription(LPCTSTR ServiceName);
BOOL QueryFailure(LPCTSTR ServiceName);

/* print and error functions */
VOID PrintService(LPCTSTR ServiceName, LPSERVICE_STATUS_PROCESS pStatus, BOOL bExtended);
VOID ReportLastError(VOID);

/* usage functions */
VOID MainUsage(VOID);
VOID StartUsage(VOID);
VOID PauseUsage(VOID);
VOID InterrogateUsage(VOID);
VOID ContinueUsage(VOID);
VOID StopUsage(VOID);
VOID ConfigUsage(VOID);
VOID DescriptionUsage(VOID);
VOID DeleteUsage(VOID);
VOID CreateUsage(VOID);
VOID ControlUsage(VOID);
VOID SdShowUsage(VOID);
VOID SdSetUsage(VOID);
VOID QueryConfigUsage(VOID);
VOID QueryDescriptionUsage(VOID);
VOID QueryFailureUsage(VOID);

#endif /* _SC_PCH_ */
