/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/infsupp.c
 * PURPOSE:         Interfacing with Setup* API .INF Files support functions
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "infsupp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/*
 * These externs should be defined by the user of this library.
 * They are kept there for reference and ease of usage.
 */
SPINF_EXPORTS SpInfExports = {NULL};

/* HELPER FUNCTIONS **********************************************************/

BOOLEAN
INF_GetDataField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT PCWSTR* Data)
{
#if 0

    BOOL Success;
    PWCHAR InfData;
    DWORD dwSize;

    *Data = NULL;

    Success = SpInfGetStringField(Context,
                                  FieldIndex,
                                  NULL,
                                  0,
                                  &dwSize);
    if (!Success)
        return FALSE;

    InfData = RtlAllocateHeap(ProcessHeap, 0, dwSize * sizeof(WCHAR));
    if (!InfData)
        return FALSE;

    Success = SpInfGetStringField(Context,
                                  FieldIndex,
                                  InfData,
                                  dwSize,
                                  NULL);
    if (!Success)
    {
        RtlFreeHeap(ProcessHeap, 0, InfData);
        return FALSE;
    }

    *Data = InfData;
    return TRUE;

#else

    *Data = SpInfGetField(Context, FieldIndex);
    return !!*Data;

#endif
}

BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PCWSTR* Key,
    OUT PCWSTR* Data)
{
    BOOL Success;
    PCWSTR InfData[2] = {NULL, NULL};

    if (Key)
        *Key = NULL;

    if (Data)
        *Data = NULL;

    /*
     * Verify that the INF file has only one value field, in addition to its key name.
     * Note that SpInfGetFieldCount() does not count the key name as a field.
     */
    if (SpInfGetFieldCount(Context) != 1)
    {
        DPRINT1("SpInfGetFieldCount != 1\n");
        return FALSE;
    }

    if (Key)
    {
        Success = INF_GetDataField(Context, 0, &InfData[0]);
        if (!Success)
            return FALSE;
    }

    if (Data)
    {
        Success = INF_GetDataField(Context, 1, &InfData[1]);
        if (!Success)
        {
            INF_FreeData(InfData[0]);
            return FALSE;
        }
    }

    if (Key)
        *Key = InfData[0];

    if (Data)
        *Data = InfData[1];

    return TRUE;
}

/* EOF */
