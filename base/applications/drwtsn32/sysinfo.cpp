/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Output system info
 * COPYRIGHT:   Copyright 2017 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"
#include <udmihelp.h>
#include <winreg.h>
#include <reactos/buildno.h>
#include <reactos/stubs.h>

static const char* Exception2Str(DWORD code)
{
    switch (code)
    {
#define EX_TO_STR(name)    case (DWORD)(name): return #name
        EX_TO_STR(EXCEPTION_ACCESS_VIOLATION);
        EX_TO_STR(EXCEPTION_DATATYPE_MISALIGNMENT);
        EX_TO_STR(EXCEPTION_BREAKPOINT);
        EX_TO_STR(EXCEPTION_SINGLE_STEP);
        EX_TO_STR(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
        EX_TO_STR(EXCEPTION_FLT_DENORMAL_OPERAND);
        EX_TO_STR(EXCEPTION_FLT_DIVIDE_BY_ZERO);
        EX_TO_STR(EXCEPTION_FLT_INEXACT_RESULT);
        EX_TO_STR(EXCEPTION_FLT_INVALID_OPERATION);
        EX_TO_STR(EXCEPTION_FLT_OVERFLOW);
        EX_TO_STR(EXCEPTION_FLT_STACK_CHECK);
        EX_TO_STR(EXCEPTION_FLT_UNDERFLOW);
        EX_TO_STR(EXCEPTION_INT_DIVIDE_BY_ZERO);
        EX_TO_STR(EXCEPTION_INT_OVERFLOW);
        EX_TO_STR(EXCEPTION_PRIV_INSTRUCTION);
        EX_TO_STR(EXCEPTION_IN_PAGE_ERROR);
        EX_TO_STR(EXCEPTION_ILLEGAL_INSTRUCTION);
        EX_TO_STR(EXCEPTION_NONCONTINUABLE_EXCEPTION);
        EX_TO_STR(EXCEPTION_STACK_OVERFLOW);
        EX_TO_STR(EXCEPTION_INVALID_DISPOSITION);
        EX_TO_STR(EXCEPTION_GUARD_PAGE);
        EX_TO_STR(EXCEPTION_INVALID_HANDLE);
        EX_TO_STR(EXCEPTION_WINE_STUB);
        EX_TO_STR(STATUS_ASSERTION_FAILURE);
    }

    return "--";
}

static void ReadKey(HKEY hKey, const char* ValueName, char* Buffer, DWORD size)
{
    DWORD dwType;
    LSTATUS ret = RegQueryValueExA(hKey, ValueName, NULL, &dwType, (LPBYTE)Buffer, &size);
    if (ret != ERROR_SUCCESS || dwType != REG_SZ)
        Buffer[0] = '\0';
}

void PrintSystemInfo(FILE* output, DumpData& data)
{
    SYSTEMTIME LocalTime;
    GetLocalTime(&LocalTime);
    xfprintf(output, NEWLINE "ReactOS " KERNEL_VERSION_STR " DrWtsn32" NEWLINE NEWLINE);
    xfprintf(output, "Application exception occurred:" NEWLINE);
    xfprintf(output, "    App: %ls (pid=%d, tid=0x%x)" NEWLINE, data.ProcessName.c_str(), data.ProcessID, data.ThreadID);
    xfprintf(output, "    When: %d/%d/%d @ %02d:%02d:%02d.%d" NEWLINE,
             LocalTime.wDay, LocalTime.wMonth, LocalTime.wYear,
             LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond, LocalTime.wMilliseconds);

    xfprintf(output, "    First chance: %u" NEWLINE, data.ExceptionInfo.dwFirstChance);
    EXCEPTION_RECORD& Record = data.ExceptionInfo.ExceptionRecord;
    xfprintf(output, "    Exception number: 0x%08x (%s)" NEWLINE, Record.ExceptionCode, Exception2Str(Record.ExceptionCode));
    xfprintf(output, "    Exception flags: 0x%08x" NEWLINE, Record.ExceptionFlags);
    xfprintf(output, "    Exception address: %p" NEWLINE, Record.ExceptionAddress);
    if (Record.NumberParameters)
    {
        xfprintf(output, "    Exception parameters: %u" NEWLINE, Record.NumberParameters);
        for (DWORD n = 0; n < std::min<DWORD>(EXCEPTION_MAXIMUM_PARAMETERS, Record.NumberParameters); ++n)
        {
            xfprintf(output, "      Parameter %u: 0x%p" NEWLINE, n, Record.ExceptionInformation[n]);
        }
    }

    char Buffer[MAX_PATH];
    DWORD count = sizeof(Buffer);
    xfprintf(output, NEWLINE "*----> System Information <----*" NEWLINE NEWLINE);
    if (GetComputerNameA(Buffer, &count))
        xfprintf(output, "    Computer Name: %s" NEWLINE, Buffer);
    count = sizeof(Buffer);
    if (GetUserNameA(Buffer, &count))
        xfprintf(output, "    User Name: %s" NEWLINE, Buffer);


    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };
    SMBiosBuf = LoadSMBiosData(DmiStrings);
    if (SMBiosBuf)
    {
        if (DmiStrings[BIOS_VENDOR])
            xfprintf(output, "    BIOS Vendor: %s" NEWLINE, DmiStrings[BIOS_VENDOR]);
        if (DmiStrings[BIOS_VERSION])
            xfprintf(output, "    BIOS Version: %s" NEWLINE, DmiStrings[BIOS_VERSION]);
        if (DmiStrings[BIOS_DATE])
            xfprintf(output, "    BIOS Date: %s" NEWLINE, DmiStrings[BIOS_DATE]);
        if (DmiStrings[SYS_VENDOR])
            xfprintf(output, "    System Manufacturer: %s" NEWLINE, DmiStrings[SYS_VENDOR]);
        if (DmiStrings[SYS_FAMILY])
            xfprintf(output, "    System Family: %s" NEWLINE, DmiStrings[SYS_FAMILY]);
        if (DmiStrings[SYS_PRODUCT])
            xfprintf(output, "    System Model: %s" NEWLINE, DmiStrings[SYS_PRODUCT]);
        if (DmiStrings[SYS_VERSION])
            xfprintf(output, "    System Version: %s" NEWLINE, DmiStrings[SYS_VERSION]);
        if (DmiStrings[SYS_SKU])
            xfprintf(output, "    System SKU: %s" NEWLINE, DmiStrings[SYS_SKU]);
        if (DmiStrings[BOARD_VENDOR])
            xfprintf(output, "    Baseboard Manufacturer: %s" NEWLINE, DmiStrings[BOARD_VENDOR]);
        if (DmiStrings[BOARD_NAME])
            xfprintf(output, "    Baseboard Model: %s" NEWLINE, DmiStrings[BOARD_NAME]);
        if (DmiStrings[BOARD_VERSION])
            xfprintf(output, "    Baseboard Version: %s" NEWLINE, DmiStrings[BOARD_VERSION]);
        FreeSMBiosData(SMBiosBuf);
    }

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    xfprintf(output, "    Number of Processors: %d" NEWLINE, info.dwNumberOfProcessors);

    HKEY hKey;
    LONG ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                             0, KEY_READ, &hKey);
    if (ret == ERROR_SUCCESS)
    {
        DWORD dwType;
        count = sizeof(Buffer);
        ret = RegQueryValueExA(hKey, "Identifier", NULL, &dwType, (LPBYTE)Buffer, &count);
        if (ret == ERROR_SUCCESS && dwType == REG_SZ)
        {
            Buffer[count] = '\0';
            xfprintf(output, "    Processor Type: %s" NEWLINE, Buffer);
        }
        RegCloseKey(hKey);
    }

    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
    if (ret == ERROR_SUCCESS)
    {
        char Version[50];
        ReadKey(hKey, "ProductName", Buffer, sizeof(Buffer));
        ReadKey(hKey, "CurrentVersion", Version, sizeof(Version));
        xfprintf(output, "    %s Version: %s" NEWLINE, Buffer, Version);
        ReadKey(hKey, "BuildLab", Buffer, sizeof(Buffer));
        xfprintf(output, "    BuildLab: %s" NEWLINE, Buffer);
        ReadKey(hKey, "CSDVersion", Buffer, sizeof(Buffer));
        if (Buffer[0])
            xfprintf(output, "    Service Pack: %s" NEWLINE, Buffer);
        ReadKey(hKey, "CurrentType", Buffer, sizeof(Buffer));
        xfprintf(output, "    Current Type: %s" NEWLINE, Buffer);
        ReadKey(hKey, "RegisteredOrganization", Buffer, sizeof(Buffer));
        xfprintf(output, "    Registered Organization: %s" NEWLINE, Buffer);
        ReadKey(hKey, "RegisteredOwner", Buffer, sizeof(Buffer));
        xfprintf(output, "    Registered Owner: %s" NEWLINE, Buffer);

        RegCloseKey(hKey);
    }
}
