#include <k32.h>

#ifdef __GNUC__
#include "regtests.h"
#define NDEBUG
#include "debug.h"

#define TestFilename L"C:\\File"
#define TestExpectedFilename L"\\??\\" TestFilename
#define TestHandle (HANDLE) 1

typedef struct
{
  LPCWSTR lpFileName;
  DWORD dwDesiredAccess;
  DWORD dwShareMode;
  LPSECURITY_ATTRIBUTES lpSecurityAttributes;
  DWORD dwCreationDisposition;
  DWORD dwFlagsAndAttributes;
  HANDLE hTemplateFile;
} CreateFile_PARAMETERS;

typedef struct
{
  PWCHAR ObjectName;
  ACCESS_MASK DesiredAccess;
  ULONG FileAttributes;
  ULONG ShareAccess;
  ULONG CreateDisposition;
  ULONG CreateOptions;
  PVOID EaBuffer;
  ULONG EaLength;
} NtCreateFile_PARAMETERS;

typedef struct
{
  CreateFile_PARAMETERS CreateFileParameters;
  NtCreateFile_PARAMETERS NtCreateFileParameters;
} CreateFileTest_Parameters;

static CreateFileTest_Parameters CreateFileTests[] =
{
  {
    CreateFileParameters:
    {
      lpFileName: TestFilename,
      dwDesiredAccess: GENERIC_ALL,
    	dwShareMode: FILE_SHARE_WRITE,
    	lpSecurityAttributes: NULL,
    	dwCreationDisposition: CREATE_ALWAYS,
    	dwFlagsAndAttributes: 0,
    	hTemplateFile: NULL
    },
    NtCreateFileParameters:
    {
      ObjectName: TestExpectedFilename,
      DesiredAccess: GENERIC_ALL|SYNCHRONIZE|FILE_READ_ATTRIBUTES,
      FileAttributes: 0,
      ShareAccess: FILE_SHARE_WRITE,
      CreateDisposition: FILE_OVERWRITE_IF,
      CreateOptions: FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT,
      NULL,
      0
    }
  }
};

static CreateFileTest_Parameters *CurrentTest;

static NTSTATUS STDCALL
MockNtCreateFile(PHANDLE FileHandle,
  ACCESS_MASK DesiredAccess,
  POBJECT_ATTRIBUTES ObjectAttributes,
  PIO_STATUS_BLOCK IoStatusBlock,
  PLARGE_INTEGER AllocateSize,
  ULONG FileAttributes,
  ULONG ShareAccess,
  ULONG CreateDisposition,
  ULONG CreateOptions,
  PVOID EaBuffer,
  ULONG EaLength)
{
  _AssertEqualWideString(CurrentTest->NtCreateFileParameters.ObjectName,
    ObjectAttributes->ObjectName->Buffer);
  _AssertEqualValue(CurrentTest->NtCreateFileParameters.DesiredAccess, DesiredAccess);
  _AssertEqualValue(CurrentTest->NtCreateFileParameters.FileAttributes, FileAttributes);
  _AssertEqualValue(CurrentTest->NtCreateFileParameters.ShareAccess, ShareAccess);
  _AssertEqualValue(CurrentTest->NtCreateFileParameters.CreateDisposition, CreateDisposition);
  _AssertEqualValue(CurrentTest->NtCreateFileParameters.CreateOptions, CreateOptions);
  *FileHandle = TestHandle;
  return STATUS_SUCCESS;
}

static _HOOK NtCreateFileHooks[] =
{
  {"NtCreateFile", MockNtCreateFile},
  {NULL, NULL}
};

static void TestFile()
{
  HANDLE FileHandle;
  int index;

  _SetHooks(NtCreateFileHooks);
  for (index = 0; index < sizeof(CreateFileTests) / sizeof(CreateFileTests[0]); index++)
    {
      CurrentTest = &CreateFileTests[index];
      FileHandle = CreateFileW(CurrentTest->CreateFileParameters.lpFileName,
        CurrentTest->CreateFileParameters.dwDesiredAccess,
        CurrentTest->CreateFileParameters.dwShareMode,
        CurrentTest->CreateFileParameters.lpSecurityAttributes,
        CurrentTest->CreateFileParameters.dwCreationDisposition,
        CurrentTest->CreateFileParameters.dwFlagsAndAttributes,
    		CurrentTest->CreateFileParameters.hTemplateFile);
      _AssertEqualValue(NO_ERROR, GetLastError());
      _AssertEqualValue(TestHandle, FileHandle);
    }
  _UnsetAllHooks();
}

static void RunTest()
{
  TestFile();
}

_Dispatcher(CreatefileTest, "CreateFileW")

#endif
