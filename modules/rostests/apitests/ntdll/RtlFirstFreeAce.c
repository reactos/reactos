/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlFirstFreeAce
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlFirstFreeAce)
{
    PACL Acl;
    PACE FirstAce;
    BOOLEAN Found;

    Acl = MakeAcl(0);
    if (Acl)
    {
        /* Simple empty ACL */
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == (PACE)(Acl + 1), "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Not enough space */
        Acl->AclSize = sizeof(ACL) - 1;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Invalid values for all the other fields */
        Acl->AclRevision = 76;
        Acl->Sbz1 = 0x55;
        Acl->AclSize = sizeof(ACL);
        Acl->Sbz2 = 0x55;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == (PACE)(Acl + 1), "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        FreeGuarded(Acl);
    }

    Acl = MakeAcl(1, (int)sizeof(ACE_HEADER));
    if (Acl)
    {
        /* ACL with one ACE */
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == (PACE)((PACE_HEADER)(Acl + 1) + 1), "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* The one ACE doesn't actually fit */
        Acl->AclSize = sizeof(ACL);
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == FALSE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Only the first byte fits */
        Acl->AclSize = sizeof(ACL) + 1;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Until we cover the whole size we get NULL */
        Acl->AclSize = sizeof(ACL) + sizeof(ACE_HEADER) - 1;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        FreeGuarded(Acl);
    }

    /* Same but bigger */
    Acl = MakeAcl(1, (int)sizeof(ACE_HEADER) + 4);
    if (Acl)
    {
        /* ACL with one ACE */
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == (PACE)((PCHAR)(Acl + 1) + sizeof(ACE_HEADER) + 4), "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* The one ACE doesn't actually fit */
        Acl->AclSize = sizeof(ACL);
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == FALSE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Only the first byte fits */
        Acl->AclSize = sizeof(ACL) + 1;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* Until we cover the whole size we get NULL */
        Acl->AclSize = sizeof(ACL) + sizeof(ACE_HEADER) - 3;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        FreeGuarded(Acl);
    }

    Acl = MakeAcl(4, (int)sizeof(ACE_HEADER), (int)sizeof(ACE_HEADER), (int)sizeof(ACCESS_ALLOWED_ACE), (int)sizeof(ACCESS_ALLOWED_ACE));
    if (Acl)
    {
        /* ACL with one ACE */
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == (PACE)((PCHAR)(Acl + 1) + 2 * sizeof(ACE_HEADER) + 2 * sizeof(ACCESS_ALLOWED_ACE)), "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* One less gives us NULL */
        Acl->AclSize = sizeof(ACL) + 2 * sizeof(ACE_HEADER) + 2 * sizeof(ACCESS_ALLOWED_ACE) - 1;
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == TRUE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        /* One ACE less also gives us FALSE */
        Acl->AclSize = sizeof(ACL) + 2 * sizeof(ACE_HEADER) + sizeof(ACCESS_ALLOWED_ACE);
        FirstAce = InvalidPointer;
        Found = RtlFirstFreeAce(Acl, &FirstAce);
        ok(Found == FALSE, "Found = %u\n", Found);
        ok(FirstAce == NULL, "FirstAce = %p (Acl was %p)\n", FirstAce, Acl);

        FreeGuarded(Acl);
    }
}
