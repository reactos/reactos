/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Output system info
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include <udmihelp.h>
#include <winreg.h>
#include <reactos/buildno.h>

static const char* Exception2Str(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_GUARD_PAGE: return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_INVALID_HANDLE: return "EXCEPTION_INVALID_HANDLE";
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
    DWORD ExceptionCode = data.ExceptionInfo.ExceptionRecord.ExceptionCode;
    xfprintf(output, "    Exception number: 0x%8x (%s)" NEWLINE, ExceptionCode, Exception2Str(ExceptionCode));

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
