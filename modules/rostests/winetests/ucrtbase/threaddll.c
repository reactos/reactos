/*
 * Copyright 2021 Arkadiusz Hiler for CodeWeavers
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
#pragma makedep testdll
#endif

#include <windows.h>

#include "threaddll.h"

static HANDLE detach_event;

void CDECL _endthread(void);
void CDECL _endthreadex(unsigned int);

void WINAPI set_detach_event(HANDLE event)
{
    detach_event = event;
}

static unsigned internal_thread_proc(void *param)
{
    struct threaddll_args *args = param;
    SetEvent(args->confirm_running);
    WaitForSingleObject(args->past_free, INFINITE);

    if (args->exit_method == thread_exit_endthread)
        _endthread();
    else if (args->exit_method == thread_exit_endthreadex)
        _endthreadex(0);

    return 0;
}

unsigned WINAPI stdcall_thread_proc(void *param)
{
    return internal_thread_proc(param);
}

void CDECL cdecl_thread_proc(void *param)
{
    internal_thread_proc(param);
}

BOOL WINAPI DllMain(HINSTANCE instance_new, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_DETACH:
            if (detach_event) SetEvent(detach_event);
            break;
    }

    return TRUE;
}
