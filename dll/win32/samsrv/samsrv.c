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

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
SamIConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN BOOLEAN Trusted)
{
    PSAM_DB_OBJECT ServerObject;
    NTSTATUS Status;

    TRACE("SamIConnect(%p %p %lx %ld)\n",
          ServerName, ServerHandle, DesiredAccess, Trusted);

    /* Map generic access rights */
    RtlMapGenericMask(&DesiredAccess,
                      pServerMapping);

    /* Open the Server Object */
    Status = SampOpenDbObject(NULL,
                              NULL,
                              L"SAM",
                              0,
                              SamDbServerObject,
                              DesiredAccess,
                              &ServerObject);
    if (NT_SUCCESS(Status))
    {
        ServerObject->Trusted = Trusted;
        *ServerHandle = (SAMPR_HANDLE)ServerObject;
    }

    TRACE("SamIConnect done (Status 0x%08lx)\n", Status);

    return Status;
}


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


VOID
NTAPI
SamIFree_SAMPR_PSID_ARRAY(PSAMPR_PSID_ARRAY Ptr)
{
    if (Ptr != NULL)
    {
        if (Ptr->Sids !=0)
        {
            MIDL_user_free(Ptr->Sids);
        }
    }
}


VOID
NTAPI
SamIFree_SAMPR_RETURNED_USTRING_ARRAY(PSAMPR_RETURNED_USTRING_ARRAY Ptr)
{
    ULONG i;

    if (Ptr != NULL)
    {
        if (Ptr->Element != NULL)
        {
            for (i = 0; i < Ptr->Count; i++)
            {
                if (Ptr->Element[i].Buffer != NULL)
                    MIDL_user_free(Ptr->Element[i].Buffer);
            }

            MIDL_user_free(Ptr->Element);
            Ptr->Element = NULL;
            Ptr->Count = 0;
        }
    }
}


VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(PSAMPR_ULONG_ARRAY Ptr)
{
    if (Ptr != NULL)
    {
        if (Ptr->Element != NULL)
        {
            MIDL_user_free(Ptr->Element);
            Ptr->Element = NULL;
            Ptr->Count = 0;
        }
    }

}

/* EOF */
