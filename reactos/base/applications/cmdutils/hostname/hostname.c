/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : ReactOS/Win32 get host name
 * FILE      : subsys/system/hostname/hostname.c
 * PROGRAMMER: Emanuele Aliberti (ea@reactos.com)
 */

#include <conio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

int wmain(int argc, WCHAR* argv[])
{
    WCHAR Msg[100];

    if (1 == argc)
    {
        WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1] = L"";
        DWORD ComputerNameSize = sizeof(ComputerName) / sizeof(ComputerName[0]);

        if (!GetComputerName(ComputerName, &ComputerNameSize))
        {
            /* Fail in case of error */
            LoadStringW(GetModuleHandle(NULL), IDS_ERROR, Msg, 100);
            _cwprintf(L"%s %lu.\n", Msg, GetLastError());
            return 1;
        }

        /* Print out the computer's name */
        _cwprintf(L"%s\n", ComputerName);
    }
    else
    {
        if ((wcsicmp(argv[1], L"-s") == 0) || (wcsicmp(argv[1], L"/s") == 0))
        {
            /* The program doesn't allow the user to set the computer's name */
            LoadStringW(GetModuleHandle(NULL), IDS_NOSET, Msg, 100);
            _cwprintf(L"%s\n", Msg);
            return 1;
        }
        else
        {
            /* Let the user know what the program does */
            LoadStringW(GetModuleHandle(NULL), IDS_USAGE, Msg, 100);
            _cwprintf(L"\n%s\n\n", Msg);
        }
    }

    return 0;
}

/* EOF */
