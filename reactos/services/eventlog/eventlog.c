/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2005 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/eventlog/eventlog.c
 * PURPOSE:          Event logging service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <stdio.h>

#include "eventlog.h"

#define NDEBUG
#include <debug.h>


VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv);


/* GLOBALS ******************************************************************/

SERVICE_TABLE_ENTRY ServiceTable[2] =
{
  {L"EventLog", (LPSERVICE_MAIN_FUNCTION)ServiceMain},
  {NULL, NULL}
};


/* FUNCTIONS *****************************************************************/


VOID CALLBACK
ServiceMain(DWORD argc, LPTSTR *argv)
{
  DPRINT("ServiceMain() called\n");

  if (StartPortThread() == FALSE)
    {
      DPRINT("StartPortThread() failed\n");
    }

  DPRINT("ServiceMain() done\n");
}


int
main(int argc, char *argv[])
{
  DPRINT("main() called\n");

  StartServiceCtrlDispatcher(ServiceTable);

  DPRINT("main() done\n");

  ExitThread(0);

  return 0;
}

/* EOF */
