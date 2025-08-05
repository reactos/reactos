/*
 * main default entry point for exe files
 *
 * Copyright 2005 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if 0
#pragma makedep implib
#endif

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"

int __cdecl main( int argc, char *argv[] )
{
    STARTUPINFOA info;
    char *cmdline = GetCommandLineA();
    int bcount = 0;
    BOOL in_quotes = FALSE;

    while (*cmdline)
    {
        if ((*cmdline == '\t' || *cmdline == ' ') && !in_quotes) break;
        else if (*cmdline == '\\') bcount++;
        else if (*cmdline == '\"')
        {
            if (!(bcount & 1)) in_quotes = !in_quotes;
            bcount = 0;
        }
        else bcount = 0;
        cmdline++;
    }
    while (*cmdline == '\t' || *cmdline == ' ') cmdline++;

    GetStartupInfoA( &info );
    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = SW_SHOWNORMAL;
    return WinMain( GetModuleHandleA(0), 0, cmdline, info.wShowWindow );
}
