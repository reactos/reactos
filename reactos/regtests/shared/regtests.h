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

/* Valid values for Command parameter of TestRoutine */
#define TESTCMD_RUN       0   /* Buffer contains information about what failed */
#define TESTCMD_TESTNAME  1   /* Buffer contains description of test */

/* Valid values for return values of TestRoutine */
#define TS_EXCEPTION     -1
#define TS_OK             0
#define TS_FAILED         1

static int _Result;
static char *_Buffer;

/* Macros to simplify tests */
#define DISPATCHER(FunctionName, TestName) \
int \
FunctionName(int Command, \
  char *Buffer) \
{ \
  switch (Command) \
    { \
    case TESTCMD_RUN: \
      _Result = TS_OK; \
      _Buffer = Buffer; \
      RunTest(); \
      return _Result; \
    case TESTCMD_TESTNAME: \
      strcpy(Buffer, TestName); \
      return TS_OK; \
    default: \
      break; \
    } \
  return TS_FAILED; \
}

#define FAIL(ErrorMessage) \
  sprintf(Buffer, "%s\n", ErrorMessage); \
  return TS_FAILED;

#define FAIL_IF_NULL(GivenValue, ErrorMessage)                     if (GivenValue == NULL) { FAIL(ErrorMessage); }
#define FAIL_IF_TRUE(GivenValue, ErrorMessage)                     if (GivenValue == TRUE) { FAIL(ErrorMessage); }
#define FAIL_IF_FALSE(GivenValue, ErrorMessage)                    if (GivenValue == FALSE) { FAIL(ErrorMessage); }
#define FAIL_IF_EQUAL(GivenValue, FailValue, ErrorMessage)         if (GivenValue == FailValue) { FAIL(ErrorMessage); }
#define FAIL_IF_NOT_EQUAL(GivenValue, FailValue, ErrorMessage)     if (GivenValue != FailValue) { FAIL(ErrorMessage); }
#define FAIL_IF_LESS_EQUAL(GivenValue, FailValue, ErrorMessage)    if (GivenValue <= FailValue) { FAIL(ErrorMessage); }
#define FAIL_IF_GREATER_EQUAL(GivenValue, FailValue, ErrorMessage) if (GivenValue >= FailValue) { FAIL(ErrorMessage); }

static inline void AppendAssertion(char *message)
{
  if (strlen(_Buffer) != 0)
    strcat(_Buffer, "\n");
  strcat(_Buffer, message);
  _Result = TS_FAILED;
}

#define _AssertEqualValue(_Expected, _Actual) \
{ \
  if ((_Expected) != (_Actual)) \
    { \
      char _message[100]; \
      sprintf(_message, "Expected %d/0x%.08x was %d/0x%.08x at %s:%d", \
        (_Expected), (_Expected), (_Actual), (_Actual), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertEqualWideString(_Expected, _Actual) \
{ \
  if (wcscmp((_Expected), (_Actual)) != 0) \
    { \
      char _message[100]; \
      sprintf(_message, "Expected %S was %S at %s:%d", \
        (_Expected), (_Actual), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}

#define _AssertNotEqualValue(_Expected, _Actual) \
{ \
  if ((_Expected) == (_Actual)) \
    { \
      char _message[100]; \
      sprintf(_message, "Actual value expected to be different from %d/0x%.08x at %s:%d", \
        (_Expected), (_Expected), __FILE__, __LINE__); \
      AppendAssertion(_message); \
    } \
}


/*
 * Test routine prototype
 * Command - The command to process
 * Buffer - Pointer to buffer in which to return context information
 */
typedef int (*TestRoutine)(int Command, char *Buffer);

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

/* Routines provided by the driver */
extern PVOID AllocateMemory(ULONG Size);
extern VOID FreeMemory(PVOID Base);


typedef struct _API_DESCRIPTION
{
  PCHAR FileName;
  PCHAR FunctionName;
  PVOID FunctionAddress;
  PVOID MockFunctionAddress;
} API_DESCRIPTION, *PAPI_DESCRIPTION;

extern API_DESCRIPTION ExternalDependencies[];
extern ULONG MaxExternalDependency;

static inline PVOID
FrameworkGetFunction(PAPI_DESCRIPTION ApiDescription)
{
  HMODULE hModule;
  PVOID Function;

  hModule = GetModuleHandleA(ApiDescription->FileName);
  if (hModule != NULL) 
    {
      Function = GetProcAddress(hModule, ApiDescription->FunctionName);
    }
  else
	  {
      hModule = LoadLibraryA(ApiDescription->FileName);
      if (hModule != NULL)
        {
          Function = GetProcAddress(hModule, ApiDescription->FunctionName);
          //FreeLibrary(hModule);
        }
    }
  return Function;
}

static inline PVOID STDCALL
FrameworkGetHookInternal(ULONG index)
{
  PVOID address;

  if (index > MaxExternalDependency)
    return NULL;

  if (ExternalDependencies[index].MockFunctionAddress != NULL)
    return ExternalDependencies[index].MockFunctionAddress;

  if (ExternalDependencies[index].FunctionAddress != NULL)
    return ExternalDependencies[index].FunctionAddress;

  printf("Calling function '%s' in DLL '%s'.\n",
    ExternalDependencies[index].FunctionName,
    ExternalDependencies[index].FileName);

  address = FrameworkGetFunction(&ExternalDependencies[index]);
 
  if (address == NULL)
    {
      printf("Function '%s' not found in DLL '%s'.\n",
        ExternalDependencies[index].FunctionName,
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

  for (index = 0; index < MaxExternalDependency; index++)
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
_ResetAllHooks()
{
  PAPI_DESCRIPTION api;
  ULONG index;

  for (index = 0; index < MaxExternalDependency; index++)
    {
      api = &ExternalDependencies[index];
      api->MockFunctionAddress = NULL;
    }
}
