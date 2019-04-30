/*
 * PROJECT:     Shim test application
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test application for the shim engine
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ntndk.h>
#include <winbase.h>

NTSYSAPI ULONG NTAPI vDbgPrintEx(_In_ ULONG ComponentId, _In_ ULONG Level, _In_z_ PCCH Format, _In_ va_list ap);
#define DPFLTR_ERROR_LEVEL 0

void xprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    vDbgPrintEx(-1, DPFLTR_ERROR_LEVEL, fmt, ap);
    va_end(ap);
}

void test_normal_imports()
{
    char buf[100];
    DWORD dwSize;
    HMODULE mod;
    BOOL (WINAPI *p_GetComputerNameA)(LPSTR lpBuffer, LPDWORD lpnSize);

    /* Call it directly */
    ZeroMemory(buf, sizeof(buf));
    dwSize = sizeof(buf);
    if (!GetComputerNameA(buf, &dwSize))
    {
        xprintf("GetComputerNameA failed: %d\n", GetLastError());
    }
    else
    {
        xprintf("GetComputerNameA returned: '%s'\n", buf);
    }

    /* Call it using GetProcAddress */
    mod = GetModuleHandleA("kernel32.dll");
    p_GetComputerNameA = (void*)GetProcAddress(mod, "GetComputerNameA");
    if (p_GetComputerNameA == NULL)
    {
        xprintf("GetProcAddress failed: %d\n", GetLastError());
    }
    else
    {
        ZeroMemory(buf, sizeof(buf));
        dwSize = sizeof(buf);
        if (!p_GetComputerNameA(buf, &dwSize))
        {
            xprintf("p_GetComputerNameA failed: %d\n", GetLastError());
        }
        else
        {
            xprintf("p_GetComputerNameA returned: '%s'\n", buf);
        }
    }
}

INT WINAPI SHStringFromGUIDA(REFGUID,LPSTR,INT);
void test_ordinal_imports()
{
    GUID guid = { 0x11223344, 0x5566, 0x7788 };
    char buf[100];
    int r;
    HMODULE mod;
    INT (WINAPI *p_SHStringFromGUIDA)(REFGUID guid, LPSTR lpszDest, INT cchMax);

    /* Call it directly */
    ZeroMemory(buf, sizeof(buf));
    r = SHStringFromGUIDA(&guid, buf, _countof(buf)-1);
    if (r)
    {
        xprintf("SHStringFromGUIDA failed (%d)\n", r);
    }
    else
    {
        xprintf("SHStringFromGUIDA returned: '%s'\n", buf);
    }

    /* Call it using GetProcAddress */
    mod = GetModuleHandleA("shlwapi.dll");
    p_SHStringFromGUIDA = (void*)GetProcAddress(mod, (LPCSTR)23);
    if (p_SHStringFromGUIDA == NULL)
    {
        xprintf("GetProcAddress failed: %d\n", GetLastError());
    }
    else
    {
        ZeroMemory(buf, sizeof(buf));
        r = p_SHStringFromGUIDA(&guid, buf, _countof(buf)-1);
        if (r)
        {
            xprintf("p_SHStringFromGUIDA failed (%d)\n", r);
        }
        else
        {
            xprintf("p_SHStringFromGUIDA returned: '%s'\n", buf);
        }
    }
}


int main(int argc, char* argv[])
{
    xprintf("Normal import (kernel32!GetComputerNameA)\n");
    test_normal_imports();
    xprintf("\nOrdinal import (shlwapi!#23)\n");
    test_ordinal_imports();

    return 0;
}
