/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(WARNING);

/* FUNCTIONS ******************************************************************/

VOID __cdecl BootMain(IN PCCH CmdLine)
{
    CmdLineParse(CmdLine);
    MachInit(CmdLine);
    FsInit();

    /* Debugger pre-initialization */
    DebugInit(FALSE);

    TRACE("BootMain() called.\n");

    /* Check if the CPU is new enough */
    FrLdrCheckCpuCompatiblity();

    /* UI pre-initialization */
    if (!UiInitialize(FALSE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        goto Quit;
    }

    if (!MmInitializeMemoryManager())
    {
        UiMessageBoxCritical("Unable to initialize memory manager.");
        goto Quit;
    }

#ifdef _M_IX86
    HalpInitializePciStubs();
    HalpInitBusHandler();
#endif

    RunLoader();

Quit:
    /* If we reach this point, something went wrong before, therefore reboot */
    DiskStopFloppyMotor();
    Reboot();
}

// We need to emulate these, because the original ones don't work in freeldr
// These functions are here, because they need to be in the main compilation unit
// and cannot be in a library.
int __cdecl wctomb(char *mbchar, wchar_t wchar)
{
    *mbchar = (char)wchar;
    return 1;
}

int __cdecl mbtowc(wchar_t *wchar, const char *mbchar, size_t count)
{
    *wchar = (wchar_t)*mbchar;
    return 1;
}

// The wctype table is 144 KB, too much for poor freeldr
int __cdecl iswctype(wint_t wc, wctype_t wctypeFlags)
{
    return _isctype((char)wc, wctypeFlags);
}
