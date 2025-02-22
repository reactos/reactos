/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for GetDeviceDriverFileName & GetDeviceDriverBaseName
 * PROGRAMMER:      Pierre Schweitzer
 */

#include <apitest.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdio.h>

typedef struct
{
    LPVOID ImageBase;
    CHAR Path[255];
    DWORD Len;
} TEST_MODULE_INFO;

static LPVOID IntGetImageBase(LPCSTR Image)
{
    HANDLE Snap;
    MODULEENTRY32 Module;

    Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if (Snap == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    Module.dwSize = sizeof(MODULEENTRY32);
    if(!Module32First(Snap, &Module))
    {
        CloseHandle(Snap);
        return NULL;
    }

    do
    {
        if (lstrcmpiA(Module.szExePath, Image) == 0)
        {
            CloseHandle(Snap);
            return (LPVOID)Module.modBaseAddr;
        }
    } while(Module32Next(Snap, &Module));

    CloseHandle(Snap);
    return NULL;
}

static BOOLEAN IntGetModuleInformation(LPCSTR Module, BOOLEAN IsDriver, BOOLEAN IsProcMod, BOOLEAN BaseName, TEST_MODULE_INFO * Info)
{
    CHAR System[255];
    UINT Len;

    memset(Info, 0, sizeof(TEST_MODULE_INFO));

    /* Get system path */
    Len = GetSystemWindowsDirectory(System, 255);
    if (Len > 255 || Len == 0)
    {
        printf("GetSystemWindowsDirectory failed\n");
        return FALSE;
    }

    /* Make path to module */
    strcat(System, "\\system32\\");
    if (IsDriver) strcat(System, "drivers\\");
    strcat(System, Module);

    /* Get base address */
    if (IsProcMod)
    {
        Info->ImageBase = IntGetImageBase(System);
        if (!Info->ImageBase)
        {
            printf("IntGetImageBase failed\n");
            return FALSE;
        }
    }
    else
    {
        /* FIXME */
        printf("Not supported yet!\n");
        return FALSE;
    }

    if (BaseName)
    {
        strcpy(Info->Path, Module);
        Info->Len = lstrlenA(Info->Path);
    }
    else
    {
        /* Skip disk */
        strcpy(Info->Path, System + 2);
        Info->Len = lstrlenA(Info->Path);
    }

    return TRUE;
}

START_TEST(GetDeviceDriverFileName)
{
    DWORD Len;
    CHAR FileName[255];
    TEST_MODULE_INFO ModInfo;

    SetLastError(0xDEADBEEF);
    Len = GetDeviceDriverFileNameA(0, FileName, 255);
    ok(Len == 0, "Len: %lu\n", Len);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());

    if (IntGetModuleInformation("ntdll.dll", FALSE, TRUE, FALSE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverFileNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == ModInfo.Len, "Len: %lu\n", Len);
        ok(GetLastError() == 0xDEADBEEF, "Error: %lx\n", GetLastError());
        ok(lstrcmpiA(ModInfo.Path, FileName) == 0, "File name: %s\n", FileName);

        /* Test with too small buffer */
        SetLastError(0xDEADBEEF);
        ModInfo.Len--;
        ModInfo.Path[ModInfo.Len] = 0;
        FileName[ModInfo.Len] = 0;
        Len = GetDeviceDriverFileNameA(ModInfo.ImageBase, FileName, ModInfo.Len);
        ok(Len == ModInfo.Len, "Len: %lu\n", Len);
        ok(GetLastError() == 0xDEADBEEF, "Error: %lx\n", GetLastError());
        ok(lstrcmpiA(ModInfo.Path, FileName) == 0, "File name: %s\n", FileName);
    }
    else
    {
        skip("Couldn't find info about ntdll.dll\n");
    }

    if (IntGetModuleInformation("msvcrt.dll", FALSE, TRUE, FALSE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverFileNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == 0, "Len: %lu\n", Len);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());
    }
    else
    {
        skip("Couldn't find info about msvcrt.dll\n");
    }

    if (IntGetModuleInformation("psapi.dll", FALSE, TRUE, FALSE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverFileNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == 0, "Len: %lu\n", Len);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());
    }
    else
    {
        skip("Couldn't find info about psapi.dll\n");
    }
}

START_TEST(GetDeviceDriverBaseName)
{
    DWORD Len;
    CHAR FileName[255];
    TEST_MODULE_INFO ModInfo;

    SetLastError(0xDEADBEEF);
    Len = GetDeviceDriverBaseNameA(0, FileName, 255);
    ok(Len == 0, "Len: %lu\n", Len);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());

    if (IntGetModuleInformation("ntdll.dll", FALSE, TRUE, TRUE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverBaseNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == ModInfo.Len, "Len: %lu\n", Len);
        ok(GetLastError() == 0xDEADBEEF, "Error: %lx\n", GetLastError());
        ok(lstrcmpiA(ModInfo.Path, FileName) == 0, "File name: %s\n", FileName);

        /* Test with too small buffer */
        SetLastError(0xDEADBEEF);
        ModInfo.Len--;
        ModInfo.Path[ModInfo.Len] = 0;
        FileName[ModInfo.Len] = 0;
        Len = GetDeviceDriverBaseNameA(ModInfo.ImageBase, FileName, ModInfo.Len);
        ok(Len == ModInfo.Len, "Len: %lu\n", Len);
        ok(GetLastError() == 0xDEADBEEF, "Error: %lx\n", GetLastError());
        ok(lstrcmpiA(ModInfo.Path, FileName) == 0, "File name: %s\n", FileName);
    }
    else
    {
        skip("Couldn't find info about ntdll.dll\n");
    }

    if (IntGetModuleInformation("msvcrt.dll", FALSE, TRUE, TRUE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverBaseNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == 0, "Len: %lu\n", Len);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());
    }
    else
    {
        skip("Couldn't find info about msvcrt.dll\n");
    }

    if (IntGetModuleInformation("psapi.dll", FALSE, TRUE, TRUE, &ModInfo))
    {
        SetLastError(0xDEADBEEF);
        Len = GetDeviceDriverBaseNameA(ModInfo.ImageBase, FileName, 255);
        ok(Len == 0, "Len: %lu\n", Len);
        ok(GetLastError() == ERROR_INVALID_HANDLE, "Error: %lx\n", GetLastError());
    }
    else
    {
        skip("Couldn't find info about psapi.dll\n");
    }
}
