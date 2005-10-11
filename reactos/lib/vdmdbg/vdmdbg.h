#include <windows.h>

ULONG DbgPrint(PCH Format,...);

typedef BOOL ( WINAPI *PROCESSENUMPROC )
(
  DWORD dwProcessId, 
  DWORD dwAttributes,
  LPARAM lpUserDefined
);

typedef BOOL ( WINAPI *TASKENUMPROCEX )
(
  DWORD dwThreadId,
  WORD hMod16,
  WORD hTask16,
  PSZ pszModName,
  PSZ pszFileName,
  LPARAM lpUserDefined
);

extern HINSTANCE hDllInstance;

/* EOF */
