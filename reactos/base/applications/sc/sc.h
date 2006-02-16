#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>

extern SC_HANDLE hSCManager; // declared in sc.c

//#define SCDBG

/* control functions */
BOOL Query(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, BOOL bExtended);
BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount);
BOOL Create(LPCTSTR ServiceName, LPCTSTR *ServiceArgs);
BOOL Delete(LPCTSTR ServiceName);
BOOL Control(DWORD Control, LPCTSTR ServiceName, LPCTSTR *Args);

/* print and error functions */
DWORD ReportLastError(VOID);

/* usage functions */
INT MainUsage(VOID);
INT StartUsage(VOID);
INT PauseUsage(VOID);
INT InterrogateUsage(VOID);
INT ContinueUsage(VOID);
INT StopUsage(VOID);
INT ConfigUsage(VOID);
INT DescriptionUsage(VOID);
INT DeleteUsage(VOID);
INT CreateUsage(VOID);
