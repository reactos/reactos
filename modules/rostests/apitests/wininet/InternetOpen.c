/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for InternetOpen
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winsock2.h>
#include <wininet.h>

struct hostent *(WINAPI *pgethostbyname)(const char *);
int (WINAPI *pWSACancelBlockingCall)(void);
int (WINAPI *pWSAGetLastError)(void);

HINTERNET (WINAPI *pInternetOpen)(LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD);
BOOL (WINAPI *pInternetCloseHandle)(HINTERNET);

static
PVOID
GetProc(
    PCSTR FunctionName)
{
    HMODULE ModuleHandle;

    ModuleHandle = GetModuleHandleW(L"ws2_32");
    if (!ModuleHandle)
        return NULL;
    return GetProcAddress(ModuleHandle, FunctionName);
}

#define PROC(name) (p##name = GetProc(#name))

static
BOOLEAN
IsWinsockLoaded(VOID)
{
    return GetModuleHandleW(L"ws2_32") != NULL;
}

static
BOOLEAN
IsWinsockInitialized(VOID)
{
    struct hostent *Hostent;

    if (!PROC(gethostbyname) || !PROC(WSAGetLastError))
        return FALSE;

    Hostent = pgethostbyname("localhost");
    if (!Hostent)
        ok_dec(pWSAGetLastError(), WSANOTINITIALISED);
    return Hostent != NULL;
}

static
BOOLEAN
AreLegacyFunctionsSupported(VOID)
{
    int Error;

    if (!PROC(WSACancelBlockingCall) || !PROC(WSAGetLastError))
        return FALSE;

    Error = pWSACancelBlockingCall();
    ok(Error == SOCKET_ERROR, "Error = %d\n", Error);
    ok(pWSAGetLastError() == WSAEOPNOTSUPP ||
       pWSAGetLastError() == WSAEINVAL, "WSAGetLastError = %d\n", pWSAGetLastError());

    return pWSAGetLastError() != WSAEOPNOTSUPP;
}

START_TEST(InternetOpen)
{
    HMODULE ModuleHandle;
    HINTERNET InternetHandle;
    BOOL Success;

    ok(!IsWinsockLoaded(), "Winsock loaded on startup\n");
    ok(!IsWinsockInitialized(), "Winsock initialized on startup\n");

    ModuleHandle = GetModuleHandleW(L"wininet");
    ok_ptr(ModuleHandle, NULL);
    ModuleHandle = LoadLibraryW(L"wininet");
    ok(ModuleHandle != NULL, "LoadLibrary failed, error %lu\n", GetLastError());

    pInternetOpen = (PVOID)GetProcAddress(ModuleHandle, "InternetOpenW");
    pInternetCloseHandle = (PVOID)GetProcAddress(ModuleHandle, "InternetCloseHandle");

    ok(!IsWinsockLoaded(), "Winsock loaded after wininet load\n");
    ok(!IsWinsockInitialized(), "Winsock initialized after wininet load\n");

    InternetHandle = pInternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(InternetHandle != NULL, "InternetHandle = NULL\n");

    if (InternetHandle != NULL)
    {
        ok(IsWinsockLoaded(), "Winsock not loaded after InternetOpen\n");
        ok(IsWinsockInitialized(), "Winsock not initialized after InternetOpen\n");
        ok(!AreLegacyFunctionsSupported(), "Winsock initialized with version 1\n");
        Success = pInternetCloseHandle(InternetHandle);
        ok(Success, "InternetCloseHandle failed, error %lu\n", GetLastError());
    }

    ok(IsWinsockLoaded(), "Winsock unloaded after handle close\n");
    ok(IsWinsockInitialized(), "Winsock uninitialized after handle close\n");

    FreeLibrary(ModuleHandle);

    ok(IsWinsockLoaded(), "Winsock unloaded after wininet unload\n");
    trace("Winsock %sinitialized after wininet unload (should be uninitialized in 2003, still initialized in 7)\n",
          IsWinsockInitialized() ? "" : "un");
}
