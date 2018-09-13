/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Sep.c

Abstract:

    This Module implements the private security routine that are defined
    in sep.h

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989

Environment:

    Kernel Mode

Revision History:

--*/

#include "sep.h"
#include "seopaque.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepCheckAcl)
#endif



BOOLEAN
SepCheckAcl (
    IN PACL Acl,
    IN ULONG Length
    )

/*++

Routine Description:

    This is a private routine that checks that an acl is well formed.

Arguments:

    Acl - Supplies the acl to check

    Length - Supplies the real size of the acl.  The internal acl size
        must agree.

Return Value:

    BOOLEAN - TRUE if the acl is well formed and FALSE otherwise

--*/
{
    if ((Length < sizeof(ACL)) || (Length != Acl->AclSize)) {
        return FALSE;
    }
    return RtlValidAcl( Acl );
}
