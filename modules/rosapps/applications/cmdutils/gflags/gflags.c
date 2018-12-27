/*
 * PROJECT:     Global Flags utility
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Global Flags utility entrypoint
 * COPYRIGHT:   Copyright 2017 Pierre Schweitzer (pierre@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "gflags.h"

static BOOL UsePageHeap = FALSE;
static BOOL UseImageFile = FALSE;

const WCHAR ImageExecOptionsString[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";

BOOL OpenImageFileExecOptions(IN REGSAM SamDesired, IN OPTIONAL PCWSTR ImageName, OUT HKEY* Key)
{
    LONG Ret;
    HKEY HandleKey, HandleSubKey;

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ImageExecOptionsString, 0, SamDesired, &HandleKey);
    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"OpenIFEO: RegOpenKeyEx failed (%d)\n", Ret);
        return FALSE;
    }

    if (ImageName == NULL)
    {
        *Key = HandleKey;
        return TRUE;
    }

    Ret = RegCreateKeyExW(HandleKey, ImageName, 0, NULL, REG_OPTION_NON_VOLATILE, SamDesired, NULL, &HandleSubKey, NULL);
    CloseHandle(HandleKey);

    if (Ret != ERROR_SUCCESS)
    {
        wprintf(L"OpenIFEO: RegCreateKeyEx failed (%d)\n", Ret);
        return FALSE;
    }
    *Key = HandleSubKey;
    return TRUE;
}


DWORD ReadSZFlagsFromRegistry(HKEY SubKey, PWSTR Value)
{
    WCHAR Buffer[20] = { 0 };
    DWORD Len, Flags, Type;

    Len = sizeof(Buffer) - sizeof(WCHAR);
    Flags = 0;
    if (RegQueryValueExW(SubKey, Value, NULL, &Type, (BYTE*)Buffer, &Len) == ERROR_SUCCESS && Type == REG_SZ)
    {
        Flags = wcstoul(Buffer, NULL, 16);
    }

    return Flags;
}

static BOOL ParseCmdline(int argc, LPWSTR argv[])
{
    INT i;

    if (argc < 2)
    {
        wprintf(L"Not enough args!\n");
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == L'/')
        {
            if (argv[i][1] == L'p' && argv[i][2] == UNICODE_NULL)
            {
                UsePageHeap = TRUE;
                return PageHeap_ParseCmdline(i + 1, argc, argv);
            }
            if (argv[i][1] == L'i' && argv[i][2] == UNICODE_NULL)
            {
                UseImageFile = TRUE;
                return ImageFile_ParseCmdline(i + 1, argc, argv);
            }
        }
        else
        {
            wprintf(L"Invalid option: %s\n", argv[i]);
            return FALSE;
        }
    }

    if (!UsePageHeap && !UseImageFile)
    {
        wprintf(L"Only page heap / image file flags are supported\n");
        return FALSE;
    }

    return TRUE;
}


int wmain(int argc, LPWSTR argv[])
{
    if (!ParseCmdline(argc, argv))
    {
        wprintf(L"Usage: gflags [/p [image.exe] [/enable|/disable [/full]]]\n"
                L"              [/i <image.exe> [<Flags>]]\n"
                L"    image.exe:  Image you want to deal with\n"
                L"    /enable:    enable page heap for the image\n"
                L"    /disable:   disable page heap for the image\n"
                L"    /full:      activate full debug page heap\n"
                L"    <Flags>:    A 32 bit hex number (0x00000001) that specifies\n"
                L"                one or more global flags to set.\n"
                L"                Without any flags, the current settings are shown.\n"
                L"                Specify FFFFFFFF to delete the GlobalFlags entry.\n"
                L"                Additionally, instead of a single hex number,\n"
                L"                specify a list of abbreviations prefixed with\n"
                L"                a '+' to add, and '-' to remove a bit.\n"
                L"                Valid abbreviations:\n");
        PrintFlags(~0, DEST_IMAGE);
        return 1;
    }

    if (UsePageHeap)
    {
        return PageHeap_Execute();
    }
    else if (UseImageFile)
    {
        return ImageFile_Execute();
    }
    return 2;
}
