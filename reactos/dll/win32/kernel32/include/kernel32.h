#ifndef _KERNEL32_INCLUDE_KERNEL32_H
#define _KERNEL32_INCLUDE_KERNEL32_H

#include "baseheap.h"

#define BINARY_UNKNOWN	(0)
#define BINARY_PE_EXE32	(1)
#define BINARY_PE_DLL32	(2)
#define BINARY_PE_EXE64	(3)
#define BINARY_PE_DLL64	(4)
#define BINARY_WIN16	(5)
#define BINARY_OS216	(6)
#define BINARY_DOS	(7)
#define BINARY_UNIX_EXE	(8)
#define BINARY_UNIX_LIB	(9)

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld))
#endif

#define IsConsoleHandle(h) \
  (((((ULONG)h) & 0x10000003) == 0x3) ? TRUE : FALSE)

#define HANDLE_DETACHED_PROCESS    (HANDLE)-2
#define HANDLE_CREATE_NEW_CONSOLE  (HANDLE)-3
#define HANDLE_CREATE_NO_WINDOW    (HANDLE)-4

/* Undocumented CreateProcess flag */
#define STARTF_SHELLPRIVATE         0x400
  
#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))

typedef
DWORD
(*WaitForInputIdleType)(
    HANDLE hProcess,
    DWORD dwMilliseconds);

/* GLOBAL VARIABLES **********************************************************/

extern BOOL bIsFileApiAnsi;
extern HANDLE hProcessHeap;
extern HANDLE hBaseDir;
extern HMODULE hCurrentModule;

extern RTL_CRITICAL_SECTION DllLock;

extern UNICODE_STRING DllDirectory;

extern LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter;

/* FUNCTION PROTOTYPES *******************************************************/

BOOL STDCALL VerifyConsoleIoHandle(HANDLE Handle);

BOOL STDCALL CloseConsoleHandle(HANDLE Handle);

HANDLE STDCALL
GetConsoleInputWaitHandle (VOID);

HANDLE STDCALL OpenConsoleW (LPCWSTR wsName,
			     DWORD  dwDesiredAccess,
			     BOOL   bInheritHandle,
			     DWORD  dwShareMode);

PTEB GetTeb(VOID);

HANDLE FASTCALL TranslateStdHandle(HANDLE hHandle);

PWCHAR FilenameA2W(LPCSTR NameA, BOOL alloc);
DWORD FilenameW2A_N(LPSTR dest, INT destlen, LPCWSTR src, INT srclen);

DWORD FilenameW2A_FitOrFail(LPSTR  DestA, INT destLen, LPCWSTR SourceW, INT sourceLen);
DWORD FilenameU2A_FitOrFail(LPSTR  DestA, INT destLen, PUNICODE_STRING SourceU);

#define HeapAlloc RtlAllocateHeap
#define HeapReAlloc RtlReAllocateHeap
#define HeapFree RtlFreeHeap

POBJECT_ATTRIBUTES
STDCALL
BasepConvertObjectAttributes(OUT POBJECT_ATTRIBUTES ObjectAttributes,
                             IN PSECURITY_ATTRIBUTES SecurityAttributes OPTIONAL,
                             IN PUNICODE_STRING ObjectName);
                             
NTSTATUS
STDCALL
BasepCreateStack(HANDLE hProcess,
                 ULONG StackReserve,
                 ULONG StackCommit,
                 PINITIAL_TEB InitialTeb);
                 
VOID
STDCALL
BasepInitializeContext(IN PCONTEXT Context,
                       IN PVOID Parameter,
                       IN PVOID StartAddress,
                       IN PVOID StackAddress,
                       IN ULONG ContextType);
                
VOID
STDCALL
BaseThreadStartupThunk(VOID);

VOID
STDCALL
BaseProcessStartThunk(VOID);
        
__declspec(noreturn)
VOID
STDCALL
BaseThreadStartup(LPTHREAD_START_ROUTINE lpStartAddress,
                  LPVOID lpParameter);
                  
VOID
STDCALL
BasepFreeStack(HANDLE hProcess,
               PINITIAL_TEB InitialTeb);

__declspec(noreturn)
VOID
WINAPI
BaseFiberStartup(VOID);

typedef UINT (STDCALL *PPROCESS_START_ROUTINE)(VOID);

VOID
STDCALL
BaseProcessStartup(PPROCESS_START_ROUTINE lpStartAddress);
                  
BOOLEAN
STDCALL
BasepCheckRealTimePrivilege(VOID);

VOID
STDCALL
BasepAnsiStringToHeapUnicodeString(IN LPCSTR AnsiString,
                                   OUT LPWSTR* UnicodeString);
                                   
PUNICODE_STRING
STDCALL
Basep8BitStringToCachedUnicodeString(IN LPCSTR String);

NTSTATUS
STDCALL
Basep8BitStringToLiveUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                   IN LPCSTR String);

NTSTATUS
STDCALL
Basep8BitStringToHeapUnicodeString(OUT PUNICODE_STRING UnicodeString,
                                   IN LPCSTR String);

typedef NTSTATUS (NTAPI *PRTL_CONVERT_STRING)(IN PUNICODE_STRING UnicodeString,
                                              IN PANSI_STRING AnsiString,
                                              IN BOOLEAN AllocateMemory);
                                                
extern PRTL_CONVERT_STRING Basep8BitStringToUnicodeString;

NTSTATUS
STDCALL
BasepMapFile(IN LPCWSTR lpApplicationName,
             OUT PHANDLE hSection,
             IN PUNICODE_STRING ApplicationName);

#endif /* ndef _KERNEL32_INCLUDE_KERNEL32_H */

