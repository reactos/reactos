/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.h
 * PURPOSE:         Regression testing
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#ifndef __REGTESTS_H
#define __REGTESTS_H
#include <stdio.h>
#include <string.h>

typedef DWORD (STDCALL _LPTHREAD_START_ROUTINE)(LPVOID lpParameter);

typedef struct __FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} _FILETIME, *_PFILETIME, *_LPFILETIME;

extern void SetupOnce();

#define _SetupOnce() \
void SetupOnce()

/* Valid values for Command parameter of TestRoutine */
#define TESTCMD_RUN       0   /* Buffer contains information about what failed */
#define TESTCMD_TESTTYPE  1   /* Buffer contains type of test */
#define TESTCMD_TESTNAME  2   /* Buffer contains description of test */
#define TESTCMD_TIMEOUT   3   /* Buffer contains timeout for test (DWORD, default is 5000 ms) */

/* Test types */
#define TT_NORMAL         0

/* Valid values for return values of TestRoutine */
#define TS_TIMEDOUT      ((DWORD)-2)
#define TS_EXCEPTION     ((DWORD)-1)
#define TS_OK             0
#define TS_FAILED         1

extern int _Result;
extern char *_Buffer;

/* Macros to simplify tests */
#define _DispatcherTypeTimeout(FunctionName, TestName, TestType, TimeOut) \
void \
FunctionName(int Command) \
{ \
  switch (Command) \
    { \
      case TESTCMD_RUN: \
        RunTest(); \
        break; \
      case TESTCMD_TESTTYPE: \
        *(PDWORD)_Buffer = (DWORD)TestType; \
        break; \
      case TESTCMD_TESTNAME: \
        strcpy(_Buffer, TestName); \
        break; \
      case TESTCMD_TIMEOUT: \
        *(PDWORD)_Buffer = (DWORD)TimeOut; \
        break; \
      default: \
        _Result = TS_FAILED; \
        break; \
    } \
}

#define _DispatcherTimeout(FunctionName, TestName, TimeOut) \
  _DispatcherTypeTimeout(FunctionName, TestName, TT_NORMAL, TimeOut)

#define _DispatcherType(FunctionName, TestName, TestType) \
  _DispatcherTypeTimeout(FunctionName, TestName, TestType, 5000)

#define _Dispatcher(FunctionName, TestName) \
  _DispatcherTimeout(FunctionName, TestName, 5000)

static __inline void
AppendAssertion(char *message)
{
  if (strlen(_Buffer) != 0)
    strcat(_Buffer, "\n");
  strcat(_Buffer, message);
  _Result = TS_FAILED;
}

#define _AssertTrue(_Condition) \
{ \
  if (!(_Condition)) \
    { \
      char _message[100]; \
      sprintf(_message, "Condition was not true at %s:%d", \
        __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertFalse(_Condition) \
{ \
  if (_Condition) \
    { \
      char _message[100]; \
      sprintf(_message, "Condition was not false at %s:%d", \
        __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertEqualValue(_Expected, _Actual) \
{ \
  ULONG __Expected = (ULONG) (_Expected); \
  ULONG __Actual = (ULONG) (_Actual); \
  if ((__Expected) != (__Actual)) \
    { \
      char _message[100]; \
      sprintf(_message, "Expected %ld/0x%.08lx was %ld/0x%.08lx at %s:%d", \
        (__Expected), (__Expected), (__Actual), (__Actual), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertEqualWideString(_Expected, _Actual) \
{ \
  LPWSTR __Expected = (LPWSTR) (_Expected); \
  LPWSTR __Actual = (LPWSTR) (_Actual); \
  if (wcscmp((__Expected), (__Actual)) != 0) \
    { \
      char _message[100]; \
      sprintf(_message, "Expected %S was %S at %s:%d", \
        (__Expected), (__Actual), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertNotEqualValue(_Expected, _Actual) \
{ \
  ULONG __Expected = (ULONG) (_Expected); \
  ULONG __Actual = (ULONG) (_Actual); \
  if ((__Expected) == (__Actual)) \
    { \
      char _message[100]; \
      sprintf(_message, "Actual value expected to be different from %ld/0x%.08lx at %s:%d", \
        (__Expected), (__Expected), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}


/*
 * Test routine prototype
 * Command - The command to process
 */
typedef void (*TestRoutine)(int Command);

/*
 * Test output routine prototype
 * Buffer - Address of buffer with text to output
 */
typedef void (*TestOutputRoutine)(char *Buffer);

/*
 * Test driver entry routine.
*  OutputRoutine - Output routine.
 * TestName - If NULL all tests are run. If non-NULL specifies the test to be run
 */
typedef void (STDCALL *TestDriverMain)(TestOutputRoutine OutputRoutine, char *TestName);

typedef struct __TEST
{
  LIST_ENTRY ListEntry;
  TestRoutine Routine;
} _TEST, *_PTEST;

extern VOID InitializeTests();
extern VOID RegisterTests();
extern VOID PerformTests(TestOutputRoutine OutputRoutine, LPSTR TestName);


typedef struct __API_DESCRIPTION
{
  PCHAR FileName;
  PCHAR FunctionName;
  PCHAR ForwardedFunctionName;
  PVOID FunctionAddress;
  PVOID MockFunctionAddress;
} _API_DESCRIPTION, *_PAPI_DESCRIPTION;

extern _API_DESCRIPTION ExternalDependencies[];
extern ULONG MaxExternalDependency;

HANDLE STDCALL
_GetModuleHandleA(LPCSTR lpModuleName);

PVOID STDCALL
_GetProcAddress(HANDLE hModule,
  LPCSTR lpProcName);

HANDLE STDCALL
_LoadLibraryA(LPCSTR lpLibFileName);

VOID STDCALL
_ExitProcess(UINT uExitCode);

HANDLE STDCALL
_CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
              _LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
              DWORD dwCreationFlags, LPDWORD lpThreadId);

BOOL STDCALL
_TerminateThread(HANDLE hThread, DWORD dwExitCode);

DWORD STDCALL
_WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

DWORD STDCALL
_GetLastError();

VOID STDCALL
_CloseHandle(HANDLE handle);

BOOL STDCALL
_GetThreadTimes(HANDLE hThread,
                _LPFILETIME lpCreationTime,
                _LPFILETIME lpExitTime,
                _LPFILETIME lpKernelTime,
                _LPFILETIME lpUserTime);

BOOL STDCALL
_SetPriorityClass(HANDLE hProcess, DWORD dwPriorityClass);

BOOL STDCALL
_SetThreadPriority(HANDLE hThread, int nPriority);

HANDLE STDCALL
_GetCurrentProcess();

HANDLE STDCALL
_GetCurrentThread();

VOID STDCALL
_Sleep(DWORD dwMilliseconds);


static __inline PCHAR
FrameworkGetExportedFunctionNameInternal(_PAPI_DESCRIPTION ApiDescription)
{
  if (ApiDescription->ForwardedFunctionName != NULL)
    {
      return ApiDescription->ForwardedFunctionName;
    }
  else
    {
      return ApiDescription->FunctionName;
    }
}

static __inline PVOID
FrameworkGetFunction(_PAPI_DESCRIPTION ApiDescription)
{
  HANDLE hModule;
  PVOID function = NULL;
  PCHAR exportedFunctionName;

  exportedFunctionName = FrameworkGetExportedFunctionNameInternal(ApiDescription);

  hModule = _GetModuleHandleA(ApiDescription->FileName);
  if (hModule != NULL)
    {
      function = _GetProcAddress(hModule, exportedFunctionName);
    }
  else
	  {
      hModule = _LoadLibraryA(ApiDescription->FileName);
      if (hModule != NULL)
        {
          function = _GetProcAddress(hModule, exportedFunctionName);
          //FreeLibrary(hModule);
        }
    }
  return function;
}

static __inline PVOID
FrameworkGetHookInternal(ULONG index)
{
  PVOID address;
  PCHAR exportedFunctionName;

  if (index > MaxExternalDependency)
    return NULL;

  if (ExternalDependencies[index].MockFunctionAddress != NULL)
    return ExternalDependencies[index].MockFunctionAddress;

  if (ExternalDependencies[index].FunctionAddress != NULL)
    return ExternalDependencies[index].FunctionAddress;

  exportedFunctionName = FrameworkGetExportedFunctionNameInternal(&ExternalDependencies[index]);

  printf("Calling function '%s' in DLL '%s'.\n",
    exportedFunctionName,
    ExternalDependencies[index].FileName);

  address = FrameworkGetFunction(&ExternalDependencies[index]);

  if (address == NULL)
    {
      printf("Function '%s' not found in DLL '%s'.\n",
        exportedFunctionName,
        ExternalDependencies[index].FileName);
    }
  ExternalDependencies[index].FunctionAddress = address;

  return address;
}


static __inline VOID
_SetHook(PCHAR name,
  PVOID address)
{
  _PAPI_DESCRIPTION api;
  ULONG index;

  for (index = 0; index <= MaxExternalDependency; index++)
    {
      api = &ExternalDependencies[index];
      if (strcmp(api->FunctionName, name) == 0)
        {
          api->MockFunctionAddress = address;
          return;
        }
    }
}

typedef struct __HOOK
{
  PCHAR FunctionName;
  PVOID FunctionAddress;
} _HOOK, *_PHOOK;

static __inline VOID
_SetHooks(_PHOOK hookTable)
{
  _PHOOK hook;

  hook = &hookTable[0];
  while (hook->FunctionName != NULL)
    {
      _SetHook(hook->FunctionName,
        hook->FunctionAddress);
      hook++;
    }
}

static __inline VOID
_UnsetHooks(_PHOOK hookTable)
{
  _PHOOK hook;

  hook = &hookTable[0];
  while (hook->FunctionName != NULL)
    {
      _SetHook(hook->FunctionName,
        NULL);
      hook++;
    }
}

static __inline VOID
_UnsetAllHooks()
{
  _PAPI_DESCRIPTION api;
  ULONG index;

  for (index = 0; index <= MaxExternalDependency; index++)
    {
      api = &ExternalDependencies[index];
      api->MockFunctionAddress = NULL;
    }
}

#endif /* __REGTESTS_H */
