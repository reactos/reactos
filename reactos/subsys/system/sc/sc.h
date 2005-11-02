#include <windows.h>
#include <stdio.h>
#include <tchar.h>

extern SC_HANDLE hSCManager; // declared in sc.c

//#define DBG

/* control functions */
BOOL Query(TCHAR **Args, BOOL bExtended);
BOOL Start(INT ArgCount, TCHAR **Args);
BOOL Create(TCHAR **Args);
BOOL Delete(TCHAR **Args);
BOOL Control(DWORD Control, TCHAR **Args);

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
