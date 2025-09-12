/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnputil.c
 * PURPOSE:         PnP Utility Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PnpFreeUnicodeStringList(IN PUNICODE_STRING UnicodeStringList,
                         IN ULONG StringCount)
{
    ULONG i;

    /* Go through the list */
    if (UnicodeStringList)
    {
        /* Go through each string */
        for (i = 0; i < StringCount; i++)
        {
            /* Check if it exists */
            if (UnicodeStringList[i].Buffer)
            {
                /* Free it */
                ExFreePool(UnicodeStringList[i].Buffer);
            }
        }

        /* Free the whole list */
        ExFreePool(UnicodeStringList);
    }
}

NTSTATUS
NTAPI
PnpRegMultiSzToUnicodeStrings(IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
                              OUT PUNICODE_STRING *UnicodeStringList,
                              OUT PULONG UnicodeStringCount)
{
    PWCHAR p, pp, ps;
    ULONG i = 0;
    SIZE_T n;
    ULONG Count = 0;

    /* Validate the key information */
    if (KeyValueInformation->Type != REG_MULTI_SZ) return STATUS_INVALID_PARAMETER;

    /* Set the pointers */
    p = (PWCHAR)((ULONG_PTR)KeyValueInformation +
                 KeyValueInformation->DataOffset);
    pp = (PWCHAR)((ULONG_PTR)p + KeyValueInformation->DataLength);

    /* Loop the data */
    while (p != pp)
    {
        /* If we find a NULL, that means one string is done */
        if (!*p)
        {
            /* Add to our string count */
            Count++;

            /* Check for a double-NULL, which means we're done */
            if (((p + 1) == pp) || !(*(p + 1))) break;
        }

        /* Go to the next character */
        p++;
    }

    /* If we looped the whole list over, we missed increment a string, do it */
    if (p == pp) Count++;

    /* Allocate the list now that we know how big it is */
    *UnicodeStringList = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(UNICODE_STRING) * Count,
                                               'sUpP');
    if (!(*UnicodeStringList)) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set pointers for second loop */
    ps = p = (PWCHAR)((ULONG_PTR)KeyValueInformation +
                     KeyValueInformation->DataOffset);

    /* Loop again, to do the copy this time */
    while (p != pp)
    {
        /* If we find a NULL, that means one string is done */
        if (!*p)
        {
            /* Check how long this string is */
            n = (ULONG_PTR)p - (ULONG_PTR)ps + sizeof(UNICODE_NULL);

            /* Allocate the buffer */
            (*UnicodeStringList)[i].Buffer = ExAllocatePoolWithTag(PagedPool,
                                                                   n,
                                                                   'sUpP');
            if (!(*UnicodeStringList)[i].Buffer)
            {
                /* Back out of everything */
                PnpFreeUnicodeStringList(*UnicodeStringList, i);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Copy the string into the buffer */
            RtlCopyMemory((*UnicodeStringList)[i].Buffer, ps, n);

            /* Set the lengths */
            (*UnicodeStringList)[i].MaximumLength = (USHORT)n;
            (*UnicodeStringList)[i].Length = (USHORT)(n - sizeof(UNICODE_NULL));

            /* One more entry done */
            i++;

            /* Check for a double-NULL, which means we're done */
            if (((p + 1) == pp) || !(*(p + 1))) break;

            /* New string */
            ps = p + 1;
        }

        /* New string */
        p++;
    }

    /* Check if we've reached the last string */
    if (p == pp)
    {
        /* Calculate the string length */
        n = (ULONG_PTR)p - (ULONG_PTR)ps;

        /* Allocate the buffer for it */
        (*UnicodeStringList)[i].Buffer = ExAllocatePoolWithTag(PagedPool,
                                                               n +
                                                               sizeof(UNICODE_NULL),
                                                               'sUpP');
        if (!(*UnicodeStringList)[i].Buffer)
        {
            /* Back out of everything */
            PnpFreeUnicodeStringList(*UnicodeStringList, i);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Make sure there's an actual string here */
        if (n) RtlCopyMemory((*UnicodeStringList)[i].Buffer, ps, n);

        /* Null-terminate the string ourselves */
        (*UnicodeStringList)[i].Buffer[n / sizeof(WCHAR)] = UNICODE_NULL;

        /* Set the lengths */
        (*UnicodeStringList)[i].Length = (USHORT)n;
        (*UnicodeStringList)[i].MaximumLength = (USHORT)(n + sizeof(UNICODE_NULL));
    }

    /* And we're done */
    *UnicodeStringCount = Count;
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
PnpRegSzToString(IN PWCHAR RegSzData,
                 IN ULONG RegSzLength,
                 OUT PUSHORT StringLength OPTIONAL)
{
    PWCHAR p, pp;

    /* Find the end */
    pp = RegSzData + RegSzLength / sizeof(WCHAR);
    for (p = RegSzData; p < pp; p++)
    {
        if (!*p)
        {
            break;
        }
    }

    /* Return the length. Truncation can happen but is of no consequence. */
    if (StringLength)
    {
        *StringLength = (USHORT)(p - RegSzData) * sizeof(WCHAR);
    }
    return TRUE;
}

/* EOF */
