#include <windows.h>
#include <wininet.h>

#define NDEBUG
#include <debug.h>

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}

BOOL WINAPI
InternetAutodial(DWORD Flags, DWORD /* FIXME: should be HWND */ Parent)
{
  DPRINT1("InternetAutodial not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

DWORD WINAPI
InternetAttemptConnect(DWORD Reserved)
{
  DPRINT1("InternetAttemptConnect not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

BOOL WINAPI
InternetGetConnectedState(LPDWORD Flags, DWORD Reserved)
{
  DPRINT1("InternetGetConnectedState not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

BOOL WINAPI
InternetAutodialHangup(DWORD Reserved)
{
  DPRINT1("InternetAutodialHangup not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}
