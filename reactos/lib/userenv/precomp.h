#ifndef __USERENV_PRECOMP_H
#define __USERENV_PRECOMP_H

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <userenv.h>
#include <tchar.h>
#include <stdio.h>

/* debug.h */
void
DebugPrint (char* fmt,...);

#define DPRINT1 DebugPrint("(%s:%d) ",__FILE__,__LINE__), DebugPrint
#define CHECKPOINT1 do { DebugPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT
#endif	/* __GNUC__ */
#define CHECKPOINT

/* directory.c */
BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath);

BOOL
CreateDirectoryPath (LPCWSTR lpPathName,
		     LPSECURITY_ATTRIBUTES lpSecurityAttributes);

BOOL
RemoveDirectoryPath (LPCWSTR lpPathName);

/* misc.c */
typedef struct _DYN_FUNCS
{
  HMODULE hModule;
  union
  {
    PVOID foo;
    struct
    {
      HRESULT (STDCALL *CoInitialize)(LPVOID pvReserved);
      HRESULT (STDCALL *CoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);
      HRESULT (STDCALL *CoUninitialize)(VOID);
    };
  } fn;
} DYN_FUNCS, *PDYN_FUNCS;

typedef struct _DYN_MODULE
{
  LPWSTR Library;    /* dll file name */
  LPSTR Functions[]; /* function names */
} DYN_MODULE, *PDYN_MODULE;

extern DYN_MODULE DynOle32;

BOOL
LoadDynamicImports(PDYN_MODULE Module, PDYN_FUNCS DynFuncs);

VOID
UnloadDynamicImports(PDYN_FUNCS DynFuncs);

LPWSTR
AppendBackslash (LPWSTR String);

BOOL
GetUserSidFromToken (HANDLE hToken,
		     PUNICODE_STRING SidString);

/* profile.c */
BOOL
AppendSystemPostfix (LPWSTR lpName,
		     DWORD dwMaxLength);

/* registry.c */
BOOL
CreateUserHive (LPCWSTR lpKeyName);

#endif /* __USERENV_PRECOMP_H */
