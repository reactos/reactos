/*
 * reactos/subsys/system/lsass/lsass.c
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * --------------------------------------------------------------------
 *
 * 	19990704 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#define WIN32_NO_STATUS
#define NTOS_MODE_USER

#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <lsass/lsasrv.h>
#include <samsrv/samsrv.h>

#define NDEBUG
#include <debug.h>

INT WINAPI
wWinMain(IN HINSTANCE hInstance,
         IN HINSTANCE hPrevInstance,
         IN LPWSTR lpCmdLine,
         IN INT nShowCmd)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("Local Security Authority Subsystem\n");
    DPRINT("  Initializing...\n");

    /* Make us critical */
    RtlSetProcessIsCritical(TRUE, NULL, TRUE);

    /* Initialize the LSA server DLL. */
    Status = LsapInitLsa();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LsapInitLsa() failed (Status 0x%08lX)\n", Status);
        goto ByeBye;
    }

    /* Start the Netlogon service. */
    Status = ServiceInit();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ServiceInit() failed (Status 0x%08lX)\n", Status);
        goto ByeBye;
    }

    /* Initialize the SAM server DLL. */
    Status = SamIInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamIInitialize() failed (Status 0x%08lX)\n", Status);
        goto ByeBye;
    }

    /* FIXME: More initialization */

    DPRINT("  Done...\n");

ByeBye:
    NtTerminateThread(NtCurrentThread(), Status);

    return 0;
}

/* EOF */
