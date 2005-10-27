#include <windows.h>
#include <stdio.h>
#include <tchar.h>

DWORD ReportLastError(VOID);
VOID dprintf(TCHAR* fmt, ...);

INT MainUsage(VOID);
INT StartUsage(VOID);
INT PauseUsage(VOID);
INT InterrogateUsage(VOID);
INT ContinueUsage(VOID);
INT StopUsage(VOID);
INT ConfigUsage(VOID);
INT DescriptionUsage(VOID);

BOOL Query(TCHAR **Args, BOOL bExtended);
BOOL Start(INT ArgCount, TCHAR **Args);
BOOL Create(TCHAR **Args);
BOOL Delete(TCHAR **Args);
BOOL Control(DWORD Control, TCHAR **Args);
