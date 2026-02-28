/*
 * mainCRTStartup default entry point
 *
 * Copyright 2019 Jacek Caban for CodeWeavers
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
#include <stdlib.h>
#include <process.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"

int __cdecl main(int argc, char **argv, char **env);

static const IMAGE_NT_HEADERS *get_nt_header( void )
{
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)NtCurrentTeb()->Peb->ImageBaseAddress;
    return (const IMAGE_NT_HEADERS *)((char *)dos + dos->e_lfanew);
}

int __cdecl mainCRTStartup(void)
{
    int argc, ret;
    char **argv, **env;

#ifdef _UCRT
    _configure_narrow_argv(_crt_argv_unexpanded_arguments);
    _initialize_narrow_environment();
    argc = *__p___argc();
    argv = *__p___argv();
    env = _get_initial_narrow_environment();
#else
    int new_mode =  0;
    __getmainargs(&argc, &argv, &env, 0, &new_mode);
#endif
    _set_app_type(get_nt_header()->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ? _crt_gui_app : _crt_console_app);

    ret = main(argc, argv, env);

    exit(ret);
    return ret;
}
