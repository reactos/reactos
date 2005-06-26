/* $Id$
 *
 * reactos/subsys/system/lsass/lsass.c
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 *
 * 	19990704 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <windows.h>
#include NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <lsass/lsasrv.h>

#define NDEBUG
#include <debug.h>


int STDCALL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("Local Security Authority Subsystem\n");
  DPRINT("  Initializing...\n");

  Status = LsapInitLsa();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("LsapInitLsa() failed (Status 0x%08lX)\n", Status);
    goto ByeBye;
  }

  /* FIXME: More initialization */

ByeBye:
  NtTerminateThread(NtCurrentThread(), Status);

  return 0;
}

/* EOF */
