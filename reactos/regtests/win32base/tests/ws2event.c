#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

#include "regtests.h"

static int RunTest(char *Buffer)
{
  const WSAEVENT* lphEvents;
  WORD wVersionRequested;
  WSAEVENT hEvent;
  WSADATA wsaData;
  DWORD ErrorCode;
  int startup;

  /* Require WinSock 2.0 or later */
  wVersionRequested = MAKEWORD(2, 0);
  startup = WSAStartup(wVersionRequested, &wsaData);
  if (startup != 0)
    {
      sprintf(Buffer, "WSAStartup() failed with status %d", startup);
      return TS_FAILED;
    }

  /* Check if the WinSock version is 2.0 */
  if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0)
    {
      strcpy(Buffer, "Winsock dll version is not 2.0 or higher");
      WSACleanup();
      return TS_FAILED;
    }

  /* Create an event */
  hEvent = WSACreateEvent();
  if (hEvent == WSA_INVALID_EVENT)
    {
      sprintf(Buffer, "WSACreateEvent() failed with status %d", WSAGetLastError());
      WSACleanup();
      return TS_FAILED;
    }

  /* Check that the state of the event defaults to non-signalled */
  lphEvents = &hEvent;
  ErrorCode = WSAWaitForMultipleEvents(1,
    lphEvents,
    FALSE,
    0,
    FALSE);
  if (ErrorCode != WSA_WAIT_TIMEOUT)
    {
      sprintf(Buffer, "WSAWaitForMultipleEvents() has bad status %ld (should be WSA_WAIT_TIMEOUT (%ld))",
        ErrorCode, WSA_WAIT_TIMEOUT);
      WSACleanup();
      return TS_FAILED;
    }

  if (!WSASetEvent(hEvent))
    {
      sprintf(Buffer, "WSASetEvent() failed with status %d", WSAGetLastError());
      WSACleanup();
      return TS_FAILED;
    }

  /* Check that the state of the event is now signalled */
  lphEvents = &hEvent;
  ErrorCode = WSAWaitForMultipleEvents(1,
    lphEvents,
    FALSE,
    0,
    FALSE);
  if (ErrorCode != WSA_WAIT_EVENT_0)
    {
      sprintf(Buffer, "WSAWaitForMultipleEvents() has bad status %ld (should be WSA_WAIT_EVENT_0 (%ld))",
        ErrorCode, WSA_WAIT_EVENT_0);
      WSACleanup();
      return TS_FAILED;
    }

  if (!WSAResetEvent(hEvent))
    {
      sprintf(Buffer, "WSAResetEvent() failed with status %d", WSAGetLastError());
      WSACleanup();
      return TS_FAILED;
    }

  /* Check that the state of the event is now non-signalled */
  lphEvents = &hEvent;
  ErrorCode = WSAWaitForMultipleEvents(1,
    lphEvents,
    FALSE,
    0,
    FALSE);
  if (ErrorCode != WSA_WAIT_TIMEOUT)
    {
      /*sprintf(Buffer, "WSAWaitForMultipleEvents() now has bad status %d (should be WSA_WAIT_TIMEOUT (%d))",
        ErrorCode, WSA_WAIT_TIMEOUT);*/
      WSACleanup();
      return TS_FAILED;
    }

  if (!WSACloseEvent(hEvent))
    {
      sprintf(Buffer, "WSACloseEvent() failed with status %d", WSAGetLastError());
      WSACleanup();
      return TS_FAILED;
    }

  WSACleanup();

  return TS_OK;
}

int
Ws2eventTest(int Command, char *Buffer)
{
  switch (Command)
    {
      case TESTCMD_RUN:
        return RunTest(Buffer);
      case TESTCMD_TESTNAME:
        strcpy(Buffer, "Winsock 2 event");
        return TS_OK;
      default:
        break;
    }
  return TS_FAILED;
}
