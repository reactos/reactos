#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>

//#define SCDBG

VOID PrintService(LPCTSTR ServiceName, LPSERVICE_STATUS pStatus);
VOID PrintServiceEx(LPCTSTR ServiceName, LPSERVICE_STATUS_PROCESS pStatus);

/* control functions */
BOOL Query(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, BOOL bExtended);
BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount);
BOOL Create(LPCTSTR ServiceName, LPCTSTR *ServiceArgs);
BOOL Delete(LPCTSTR ServiceName);
BOOL Control(DWORD Control, LPCTSTR ServiceName, LPCTSTR *Args, INT ArgCount);

/* print and error functions */
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
