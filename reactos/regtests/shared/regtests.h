/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/shared/regtests.h
 * PURPOSE:         Regression testing
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <stdio.h>
#include <string.h>
#include <windows.h>

extern void SetupOnce();

#define _SetupOnce() \
void SetupOnce()

/* Valid values for Command parameter of TestRoutine */
#define TESTCMD_RUN       0   /* Buffer contains information about what failed */
#define TESTCMD_TESTNAME  1   /* Buffer contains description of test */

/* Valid values for return values of TestRoutine */
#define TS_EXCEPTION     -1
#define TS_OK             0
#define TS_FAILED         1

extern int _Result;
extern char *_Buffer;

/* Macros to simplify tests */
#define _Dispatcher(FunctionName, TestName) \
void \
FunctionName(int Command) \
{ \
  switch (Command) \
    { \
      case TESTCMD_RUN: \
        RunTest(); \
        break; \
      case TESTCMD_TESTNAME: \
        strcpy(_Buffer, TestName); \
        break; \
      default: \
        _Result = TS_FAILED; \
        break; \
    } \
}

static inline void
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
      sprintf(_message, "Expected %d/0x%.08x was %d/0x%.08x at %s:%d", \
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
  ULONG __Expected = (ULONG) (_Excepted); \
  ULONG __Actual = (ULONG) (_Actual); \
  if ((__Expected) == (__Actual)) \
    { \
      char _message[100]; \
      sprintf(_message, "Actual value expected to be different from %d/0x%.08x at %s:%d", \
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
typedef void STDCALL (*TestDriverMain)(TestOutputRoutine OutputRoutine, char *TestName);

typedef struct _ROS_TEST
{
  LIST_ENTRY ListEntry;
  TestRoutine Routine;
} ROS_TEST, *PROS_TEST;

extern LIST_ENTRY AllTests;

extern VOID InitializeTests();
extern VOID RegisterTests();
extern VOID PerformTests(TestOutputRoutine OutputRoutine, LPSTR TestName);


typedef struct _API_DESCRIPTION
{
  PCHAR FileName;
  PCHAR FunctionName;
  PCHAR ForwardedFunctionName;
  PVOID FunctionAddress;
  PVOID MockFunctionAddress;
} API_DESCRIPTION, *PAPI_DESCRIPTION;

extern API_DESCRIPTION ExternalDependencies[];
extern ULONG MaxExternalDependency;

HMODULE STDCALL
_GetModuleHandleA(LPCSTR lpModuleName);

FARPROC STDCALL
_GetProcAddress(HMODULE hModule,
  LPCSTR lpProcName);

HINSTANCE STDCALL
_LoadLibraryA(LPCSTR lpLibFileName);

VOID STDCALL
_ExitProcess(UINT uExitCode);

static inline PCHAR
FrameworkGetExportedFunctionNameInternal(PAPI_DESCRIPTION ApiDescription)
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

static inline PVOID
FrameworkGetFunction(PAPI_DESCRIPTION ApiDescription)
{
  HMODULE hModule;
  PVOID function;
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

static inline PVOID
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


static inline VOID
_SetHook(PCHAR name,
  PVOID address)
{
  PAPI_DESCRIPTION api;
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

typedef struct _HOOK
{
  PCHAR FunctionName;
  PVOID FunctionAddress;
} HOOK, *PHOOK;

static inline VOID
_SetHooks(PHOOK hookTable)
{
  PHOOK hook;

  hook = &hookTable[0];
  while (hook->FunctionName != NULL)
    {
      _SetHook(hook->FunctionName,
        hook->FunctionAddress);
      hook++;
    }
}

static inline VOID
_UnsetHooks(PHOOK hookTable)
{
  PHOOK hook;

  hook = &hookTable[0];
  while (hook->FunctionName != NULL)
    {
      _SetHook(hook->FunctionName,
        NULL);
      hook++;
    }
}

static inline VOID
_UnsetAllHooks()
{
  PAPI_DESCRIPTION api;
  ULONG index;

  for (index = 0; index <= MaxExternalDependency; index++)
    {
      api = &ExternalDependencies[index];
      api->MockFunctionAddress = NULL;
    }
}
