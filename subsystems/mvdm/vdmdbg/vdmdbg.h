#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

#define MAX_PATH16 255
#define MAX_MODULE_NAME 9

ULONG DbgPrint(PCCH Format,...);

typedef struct _CONTEXT VDMCONTEXT;
typedef VDMCONTEXT *LPVDMCONTEXT;

typedef struct _VDM_SEGINFO {
  WORD  Selector;
  WORD  SegNumber;
  DWORD Length;
  WORD  Type;
  CHAR  ModuleName[MAX_MODULE_NAME];
  CHAR  FileName[MAX_PATH16];
} VDM_SEGINFO;

typedef struct {
  DWORD  dwSize;
  char   szModule[MAX_MODULE_NAME+1];
  HANDLE hModule;
  WORD   wcUsage;
  char   szExePath[MAX_PATH16+1];
  WORD   wNext;
} MODULEENTRY, *LPMODULEENTRY;

typedef BOOL ( WINAPI *PROCESSENUMPROC )
(
  DWORD  dwProcessId,
  DWORD  dwAttributes,
  LPARAM lpUserDefined
);

typedef BOOL ( WINAPI *TASKENUMPROCEX )
(
  DWORD dwThreadId,
  WORD   hMod16,
  WORD   hTask16,
  PSZ    pszModName,
  PSZ    pszFileName,
  LPARAM lpUserDefined
);

typedef struct {
  DWORD   dwSize;
  DWORD   dwAddress;
  DWORD   dwBlockSize;
  HANDLE  hBlock;
  WORD    wcLock;
  WORD    wcPageLock;
  WORD    wFlags;
  BOOL    wHeapPresent;
  HANDLE  hOwner;
  WORD    wType;
  WORD    wData;
  DWORD   dwNext;
  DWORD   dwNextAlt;
} GLOBALENTRY, *LPGLOBALENTRY;

typedef DWORD ( CALLBACK* DEBUGEVENTPROC )
              ( LPDEBUG_EVENT, LPVOID );

typedef BOOL ( WINAPI *TASKENUMPROC )
             ( DWORD  dwThreadId,
               WORD   hMod16,
               WORD   hTask16,
               LPARAM lpUserDefined );

extern HINSTANCE hDllInstance;

/* EOF */
