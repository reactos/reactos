/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/alias.c
 * PURPOSE:     Alias specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


/* FUNCTIONS ***************************************************************/

NTSTATUS
SampOpenAliasObject(IN PSAM_DB_OBJECT DomainObject,
                    IN ULONG AliasId,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PSAM_DB_OBJECT *AliasObject)
{
    WCHAR szRid[9];

    TRACE("(%p %lu %lx %p)\n",
          DomainObject, AliasId, DesiredAccess, AliasObject);

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", AliasId);

    /* Create the user object */
    return SampOpenDbObject(DomainObject,
                            L"Aliases",
                            szRid,
                            AliasId,
                            SamDbAliasObject,
                            DesiredAccess,
                            AliasObject);
}

/* EOF */
