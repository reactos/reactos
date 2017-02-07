/*
 *  SAM Server DLL
 *  Copyright (C) 2005 Eric Kohl
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

/* INCLUDES *****************************************************************/

#include <samsrv.h>

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
SamIInitialize(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("SamIInitialize() called\n");

    if (SampIsSetupRunning())
    {
        Status = SampInitializeRegistry();
        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Initialize the SAM database */
    Status = SampInitDatabase();
    if (!NT_SUCCESS(Status))
        return Status;

    /* Start the RPC server */
    SampStartRpcServer();

    return Status;
}


NTSTATUS
NTAPI
SampInitializeRegistry(VOID)
{
    TRACE("SampInitializeRegistry() called\n");

    SampInitializeSAM();

    return STATUS_SUCCESS;
}

/* EOF */
