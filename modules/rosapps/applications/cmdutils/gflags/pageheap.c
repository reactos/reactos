/*
 * PROJECT:     Global Flags utility
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Global Flags utility page heap options
 * COPYRIGHT:   Copyright 2017 Pierre Schweitzer (pierre@reactos.org)
 */

#include "gflags.h"

static BOOL Set = FALSE;
static BOOL Unset = FALSE;
static BOOL Full = FALSE;
static PWSTR Image = NULL;


static VOID ModifyStatus(VOID)
{
    LONG Ret;
    DWORD GlobalFlags;
    HKEY IFEOKey;
    WCHAR Buffer[11];

    if (!OpenImageFileExecOptions(KEY_WRITE | KEY_READ, Image, &IFEOKey))
    {
        return;
    }

    GlobalFlags = ReadSZFlagsFromRegistry(IFEOKey, L"GlobalFlag");
    if (Set)
    {
        GlobalFlags |= FLG_HEAP_PAGE_ALLOCS;
    }
    else
    {
        GlobalFlags &= ~FLG_HEAP_PAGE_ALLOCS;
    }

    if (GlobalFlags != 0)
    {
        wsprintf(Buffer, L"0x%08x", GlobalFlags);
        Ret = RegSetValueExW(IFEOKey, L"GlobalFlag", 0, REG_SZ, (BYTE*)Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }
    else
    {
        Ret = RegDeleteValueW(IFEOKey, L"GlobalFlag");
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegDeleteValue failed (%d)\n", Ret);
        }
    }

    if (Unset)
    {
        Ret = RegDeleteValueW(IFEOKey, L"PageHeapFlags");
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegDeleteValue failed (%d)\n", Ret);
        }
    }
    else
    {
        DWORD PageHeapFlags;

        PageHeapFlags = ReadSZFlagsFromRegistry(IFEOKey, L"PageHeapFlags");
        PageHeapFlags &= ~3;

        if (Full)
        {
            PageHeapFlags |= 1;
        }
        PageHeapFlags |= 2;

        wsprintf(Buffer, L"0x%x", PageHeapFlags);
        Ret = RegSetValueExW(IFEOKey, L"PageHeapFlags", 0, REG_SZ, (BYTE*)Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }

    if (Set)
    {
        DWORD Type, VerifierFlags, Len;

        VerifierFlags = 0;
        Len = VerifierFlags;
        if (RegQueryValueExW(IFEOKey, L"VerifierFlags", NULL, &Type, (BYTE *)&VerifierFlags, &Len) == ERROR_SUCCESS &&
            Type == REG_DWORD && Len == sizeof(DWORD))
        {
            VerifierFlags &= ~0x8001;   /* RTL_VRF_FLG_FAST_FILL_HEAP | RTL_VRF_FLG_FULL_PAGE_HEAP */
        }
        else
        {
            VerifierFlags = 0;
        }

        if (Full)
        {
            VerifierFlags |= 1; /* RTL_VRF_FLG_FULL_PAGE_HEAP */
        }
        else
        {
            VerifierFlags |= 0x8000;    /* RTL_VRF_FLG_FAST_FILL_HEAP */
        }

        Ret = RegSetValueExW(IFEOKey, L"VerifierFlags", 0, REG_DWORD, (const BYTE *)&VerifierFlags, sizeof(DWORD));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
    }

    wprintf(L"path: %s\n", ImageExecOptionsString);
    wprintf(L"\t%s: page heap %s\n", Image, (Set ? L"enabled" : L"disabled"));

    HeapFree(GetProcessHeap(), 0, Buffer);
    RegCloseKey(IFEOKey);
}

static BOOL DisplayImageInfo(HKEY HandleKey, PWSTR SubKey, PBOOL Header)
{
    LONG Ret;
    BOOL Handled;
    DWORD GlobalFlags;
    HKEY HandleSubKey;

    Ret = RegOpenKeyExW(HandleKey, SubKey, 0, KEY_READ, &HandleSubKey);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DII: RegOpenKeyEx failed (%d)\n", Ret);
        return FALSE;
    }

    Handled = FALSE;
    GlobalFlags = ReadSZFlagsFromRegistry(HandleSubKey, L"GlobalFlag");
    if (GlobalFlags & FLG_HEAP_PAGE_ALLOCS)
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

        PageHeapFlags = ReadSZFlagsFromRegistry(HandleSubKey, L"PageHeapFlags");
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

    if (!OpenImageFileExecOptions(KEY_READ, NULL, &HandleKey))
    {
        return;
    }

    Ret = RegQueryInfoKeyW(HandleKey, NULL, NULL, NULL, NULL, &MaxLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"DS: RegQueryInfoKey failed (%d)\n", Ret);
        RegCloseKey(HandleKey);
        return;
    }

    ++MaxLen; // NULL-char
    SubKey = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MaxLen * sizeof(WCHAR));
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
        Ret = RegEnumKeyW(HandleKey, Index, SubKey, MaxLen);
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

BOOL PageHeap_ParseCmdline(INT i, int argc, LPWSTR argv[])
{
    for (; i < argc; i++)
    {
        if (argv[i][0] == L'/')
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
        else if (Image == NULL)
        {
            Image = argv[i];
        }
        else
        {
            wprintf(L"Invalid option: %s\n", argv[i]);
            return FALSE;
        }
    }

    if (Set && Unset)
    {
        wprintf(L"ENABLE and DISABLED cannot be set together\n");
        return FALSE;
    }

    if (Image == NULL && (Set || Unset || Full))
    {
        wprintf(L"Can't ENABLE or DISABLE with no image\n");
        return FALSE;
    }

    if (!Set && !Unset && Full)
    {
        wprintf(L"Cannot deal with full traces with no other indication\n");
        return FALSE;
    }

    return TRUE;
}

INT PageHeap_Execute()
{
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
