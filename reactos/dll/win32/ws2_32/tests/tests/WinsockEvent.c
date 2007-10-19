#include <windows.h>
#include <winsock2.h>
#include "regtests.h"

#define TestHandle (HANDLE) 1

static BOOL CloseHandleSuccessCalled = FALSE;

static BOOL STDCALL
MockCloseHandleSuccess(HANDLE hObject)
{
  CloseHandleSuccessCalled = TRUE;
  _AssertEqualValue(TestHandle, hObject);
  return TRUE;
}

static HOOK HooksSuccess[] =
{
  {"CloseHandle", MockCloseHandleSuccess},
  {NULL, NULL}
};

static void
TestWSACloseEventSuccess()
{
  BOOL result;

  _SetHooks(HooksSuccess);
  result = WSACloseEvent(TestHandle);
  _AssertTrue(result);
  _AssertEqualValue(NO_ERROR, WSAGetLastError());
  _AssertTrue(CloseHandleSuccessCalled);
  _UnsetAllHooks();
}


static BOOL CloseHandleFailureCalled = FALSE;

static BOOL STDCALL
MockCloseHandleFailure(HANDLE hObject)
{
  CloseHandleFailureCalled = TRUE;
  return FALSE;
}

static HOOK HooksFailure[] =
{
  {"CloseHandle", MockCloseHandleFailure},
  {NULL, NULL}
};

static void
TestWSACloseEventFailure()
{
  BOOL result;

  _SetHooks(HooksFailure);
  result = WSACloseEvent(TestHandle);
  _AssertFalse(result);
  _AssertEqualValue(WSA_INVALID_HANDLE, WSAGetLastError());
  _AssertTrue(CloseHandleFailureCalled);
  _UnsetAllHooks();
}


static void
TestWSACloseEvent()
{
  TestWSACloseEventSuccess();
  TestWSACloseEventFailure();
}

static void
RunTest()
{
  WSADATA WSAData;

  WSAStartup(MAKEWORD(2, 0), &WSAData);
  TestWSACloseEvent();
  WSACleanup();
}

_Dispatcher(WinsockeventTest, "Winsock 2 event")
