/*
 * PROJECT:     Global Flags utility
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Global Flags utility entrypoint
 * COPYRIGHT:   Copyright 2017 Pierre Schweitzer (pierre@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "gflags.h"

static BOOL UsePageHeap = FALSE;


DWORD ReagFlagsFromRegistry(HKEY SubKey, PVOID Buffer, PWSTR Value, DWORD MaxLen)
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
        }
        else
        {
            wprintf(L"Invalid option: %s\n", argv[i]);
            return FALSE;
        }
    }

    if (!UsePageHeap)
    {
        wprintf(L"Only page heap flags are supported\n");
        return FALSE;
    }

    return TRUE;
}


int wmain(int argc, LPWSTR argv[])
{
    if (!ParseCmdline(argc, argv))
    {
        wprintf(L"Usage: gflags /p [image.exe] [/enable|/disable [/full]]\n"
                L"\timage.exe:\tImage you want to deal with\n"
                L"\t/enable:\tenable page heap for the image\n"
                L"\t/disable:\tdisable page heap for the image\n"
                L"\t/full:\t\tactivate full debug page heap\n");
        return 1;
    }

    if (UsePageHeap)
    {
        return PageHeap_Execute();
    }
    return 2;
}
