/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ping utility
 * FILE:        applications/cmdutils/gflags/gflags.c
 * PURPOSE:     Global Flags utility
 * PROGRAMMERS: Pierre Schweitzer <pierre@reactos.org>
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <stdio.h>
#include <stdlib.h>

static BOOL Set = FALSE;
static BOOL Unset = FALSE;
static BOOL Full = FALSE;
static PWSTR Image = NULL;
static WCHAR ImageExecOptionsString[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";

static DWORD ReagFlagsFromRegistry(HKEY SubKey, PVOID Buffer, PWSTR Value, DWORD MaxLen)
{
    DWORD Len, Flags, Type;

    Len = MaxLen;
    Flags = 0;
    if (RegQueryValueEx(SubKey, Value, NULL, &Type, Buffer, &Len) == ERROR_SUCCESS && Type == REG_SZ)
    {
        Flags = wcstoul(Buffer, NULL, 16);
    }

    return Flags;
}

static VOID ModifyStatus(VOID)
{
    LONG Ret;
    DWORD MaxLen, GlobalFlags;
    PVOID Buffer;
    HKEY HandleKey, HandleSubKey;

    Ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ImageExecOptionsString, 0, KEY_WRITE | KEY_READ, &HandleKey);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"MS: RegOpenKeyEx failed (%d)\n", Ret);
        return;
    }

    Ret = RegCreateKeyEx(HandleKey, Image, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &HandleSubKey, NULL);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"MS: RegCreateKeyEx failed (%d)\n", Ret);
        return;
    }

    Ret = RegQueryInfoKey(HandleSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &MaxLen, NULL, NULL);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"MS: RegQueryInfoKey failed (%d)\n", Ret);
        RegCloseKey(HandleSubKey);
        RegCloseKey(HandleKey);
        return;
    }

    MaxLen = max(MaxLen, 11 * sizeof(WCHAR));
    Buffer = HeapAlloc(GetProcessHeap(), 0, MaxLen);
    if (Buffer == NULL)
    {
        wprintf(L"MS: HeapAlloc failed\n");
        RegCloseKey(HandleSubKey);
        RegCloseKey(HandleKey);
        return;
    }

    GlobalFlags = ReagFlagsFromRegistry(HandleSubKey, Buffer, L"GlobalFlag", MaxLen);
    if (Set)
    {
        GlobalFlags |= 0x02000000;
    }
    else
    {
        GlobalFlags &= ~0x02000000;
    }

    if (GlobalFlags != 0)
    {
        wsprintf(Buffer, L"0x%08x", GlobalFlags);
        Ret = RegSetValueEx(HandleSubKey, L"GlobalFlag", 0, REG_SZ, Buffer, 11 * sizeof(WCHAR));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }
    else
    {
        Ret = RegDeleteValue(HandleSubKey, L"GlobalFlag");
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegDeleteValue failed (%d)\n", Ret);
        }
    }

    if (Unset)
    {
        Ret = RegDeleteValue(HandleSubKey, L"PageHeapFlags");
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegDeleteValue failed (%d)\n", Ret);
        }
    }
    else
    {
        DWORD PageHeapFlags;

        PageHeapFlags = ReagFlagsFromRegistry(HandleSubKey, Buffer, L"PageHeapFlags", MaxLen);
        PageHeapFlags &= ~3;

        if (Full)
        {
            PageHeapFlags |= 1;
        }
        PageHeapFlags |= 2;

        wsprintf(Buffer, L"0x%x", PageHeapFlags);
        Ret = RegSetValueEx(HandleSubKey, L"PageHeapFlags", 0, REG_SZ, Buffer, 11 * sizeof(WCHAR));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }

    if (Set)
    {
        DWORD Type, VerifierFlags, Len;

        VerifierFlags = 0;
        Len = MaxLen;
        if (RegQueryValueEx(HandleSubKey, L"VerifierFlags", NULL, &Type, Buffer, &Len) == ERROR_SUCCESS &&
            Type == REG_DWORD && Len == sizeof(DWORD))
        {
            VerifierFlags = ((DWORD *)Buffer)[0];
            VerifierFlags &= ~0x8001;
        }

        if (Full)
        {
            VerifierFlags |= 1;
        }
        else
        {
            VerifierFlags |= 0x8000;
        }

        Ret = RegSetValueEx(HandleSubKey, L"VerifierFlags", 0, REG_DWORD, (const BYTE *)&VerifierFlags, sizeof(DWORD));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }

    wprintf(L"path: %s\n", ImageExecOptionsString);
    wprintf(L"\t%s: page heap %s\n", Image, (Set ? L"enabled" : L"disabled"));

    HeapFree(GetProcessHeap(), 0, Buffer);
    RegCloseKey(HandleSubKey);
    RegCloseKey(HandleKey);
}

static BOOL DisplayImageInfo(HKEY HandleKey, PWSTR SubKey, PBOOL Header)
{
    LONG Ret;
    BOOL Handled;
    DWORD MaxLen, GlobalFlags;
    HKEY HandleSubKey;
    PVOID Buffer;

    Ret = RegOpenKeyEx(HandleKey, SubKey, 0, KEY_READ, &HandleSubKey);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DII: RegOpenKeyEx failed (%d)\n", Ret);
        return FALSE;
    }

    Ret = RegQueryInfoKey(HandleSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &MaxLen, NULL, NULL);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DII: RegQueryInfoKey failed (%d)\n", Ret);
        RegCloseKey(HandleSubKey);
        return FALSE;
    }

    Buffer = HeapAlloc(GetProcessHeap(), 0, MaxLen);
    if (Buffer == NULL)
    {
        wprintf(L"DII: HeapAlloc failed\n");
        RegCloseKey(HandleSubKey);
        return FALSE;
    }

    Handled = FALSE;
    GlobalFlags = ReagFlagsFromRegistry(HandleSubKey, Buffer, L"GlobalFlag", MaxLen);
    if (GlobalFlags & 0x02000000)
    {
        DWORD PageHeapFlags;

        if (Image == NULL)
        {
            if (!*Header)
            {
                wprintf(L"path: %s\n", ImageExecOptionsString);
                *Header = TRUE;
            }
            wprintf(L"\t%s: page heap enabled with flags (", SubKey);
        }
        else
        {
            wprintf(L"Page heap is enabled for %s with flags (", SubKey);
        }

        PageHeapFlags = ReagFlagsFromRegistry(HandleSubKey, Buffer, L"PageHeapFlags", MaxLen);
        if (PageHeapFlags & 0x1)
        {
            wprintf(L"full ");
        }

        if (PageHeapFlags & 0x2)
        {
            wprintf(L"traces");
        }

        wprintf(L")\n");

        Handled = TRUE;
    }

    HeapFree(GetProcessHeap(), 0, Buffer);
    RegCloseKey(HandleSubKey);

    return Handled;
}

static VOID DisplayStatus(VOID)
{
    LONG Ret;
    HKEY HandleKey;
    DWORD Index, MaxLen, Handled;
    TCHAR * SubKey;
    BOOL Header;

    Ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ImageExecOptionsString, 0, KEY_READ, &HandleKey);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DS: RegOpenKeyEx failed (%d)\n", Ret);
        return;
    }

    Ret = RegQueryInfoKey(HandleKey, NULL, NULL, NULL, NULL, &MaxLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DS: RegQueryInfoKey failed (%d)\n", Ret);
        RegCloseKey(HandleKey);
        return;
    }

    ++MaxLen; // NULL-char
    SubKey = HeapAlloc(GetProcessHeap(), 0, MaxLen * sizeof(TCHAR));
    if (SubKey == NULL)
    {
        wprintf(L"DS: HeapAlloc failed\n");
        RegCloseKey(HandleKey);
        return;
    }

    Index = 0;
    Handled = 0;
    Header = FALSE;
    do
    {
        Ret = RegEnumKey(HandleKey, Index, SubKey, MaxLen);
        if (Ret != ERROR_NO_MORE_ITEMS)
        {
            if (Image == NULL || wcscmp(SubKey, Image) == 0)
            {
                if (DisplayImageInfo(HandleKey, SubKey, &Header))
                {
                    ++Handled;
                }
            }

            ++Index;
        }
    } while (Ret != ERROR_NO_MORE_ITEMS);

    if (Handled == 0)
    {
        if (Image == NULL)
        {
            wprintf(L"No application has page heap enabled.\n");
        }
        else
        {
            wprintf(L"Page heap is not enabled for %s\n", Image);
        }
    }

    HeapFree(GetProcessHeap(), 0, SubKey);
    RegCloseKey(HandleKey);
}

static VOID Usage(VOID)
{
    // FIXME
    wprintf(L"Usage\n");
}

static BOOL ParseCmdline(int argc, LPWSTR argv[])
{
    INT i;
    BOOL UsePageHeap = FALSE;

    if (argc < 2)
    {
        wprintf(L"Not enough args!\n", argc);
        Usage();
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == L'/')
        {
            if (argv[i][1] == L'p' && argv[i][2] == UNICODE_NULL)
            {
                UsePageHeap = TRUE;
            }
            else if (argv[i][1] == L'p' && argv[i][2] != UNICODE_NULL)
            {
                wprintf(L"Invalid option: %s\n", argv[i]);
                Usage();
                return FALSE;
            }
            else
            {
                if (wcscmp(argv[i], L"/enable") == 0)
                {
                    Set = TRUE;
                }
                else if (wcscmp(argv[i], L"/disable") == 0)
                {
                    Unset = TRUE;
                }
                else if (wcscmp(argv[i], L"/full") == 0)
                {
                    Full = TRUE;
                }
            }
        }
        else if (Image == NULL)
        {
            Image = argv[i];
        }
        else
        {
            wprintf(L"Invalid option: %s\n", argv[i]);
            Usage();
            return FALSE;
        }
    }

    if (!UsePageHeap)
    {
        wprintf(L"Only page heap flags are supported\n");
        Usage();
        return FALSE;
    }

    if (Set && Unset)
    {
        wprintf(L"ENABLE and DISABLED cannot be set together\n");
        Usage();
        return FALSE;
    }

    if (Image == NULL && (Set || Unset || Full))
    {
        wprintf(L"Can't ENABLE or DISABLE with no image\n");
        Usage();
        return FALSE;
    }

    if (!Set && !Unset && Full)
    {
        wprintf(L"Cannot deal with full traces with no other indication\n");
        Usage();
        return FALSE;
    }

    return TRUE;
}

int wmain(int argc, LPWSTR argv[])
{
    if (!ParseCmdline(argc, argv))
    {
        return 1;
    }

    if (!Set && !Unset)
    {
        DisplayStatus();
    }
    else
    {
        ModifyStatus();
    }

    return 0;
}
