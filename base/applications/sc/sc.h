#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>

#define SCDBG

/* control functions */
BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount);
BOOL Create(LPCTSTR ServiceName, LPCTSTR *ServiceArgs);
BOOL Delete(LPCTSTR ServiceName);
BOOL Control(DWORD Control, LPCTSTR ServiceName, LPCTSTR *Args, INT ArgCount);
BOOL Query(LPCTSTR *ServiceArgs, DWORD ArgCount, BOOL bExtended);

LPSERVICE_STATUS_PROCESS QueryService(LPCTSTR ServiceName);

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
