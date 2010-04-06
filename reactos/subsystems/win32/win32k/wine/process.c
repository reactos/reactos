/*
 * Server-side process management
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#include <win32k.h>

#include <limits.h>
#include "object.h"
#include "request.h"
#include "handle.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

/* retrieve the process idle event */
DECL_HANDLER(get_process_idle_event)
{
    PEPROCESS process_obj;
    PPROCESSINFO process;
    NTSTATUS status;

    reply->event = 0;

    /* Reference the process */
    status = ObReferenceObjectByHandle((HANDLE)req->handle,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       KernelMode,
                                       (PVOID*)&process_obj,
                                       NULL);
DPRINT1("status 0x%08X\n", status);
    if (NT_SUCCESS(status))
    {
        process = PsGetProcessWin32Process(process_obj);

        if (process->idle_event && process_obj != PsGetCurrentProcess())
        {
            UNIMPLEMENTED;
            //reply->event = alloc_handle( PsGetCurrentProcess(), process->idle_event,
            //                             EVENT_ALL_ACCESS, 0 );
        }
        ObDereferenceObject( process_obj );
    }
}
