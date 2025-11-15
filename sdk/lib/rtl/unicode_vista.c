/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Conversion Routines
 * FILE:              lib/rtl/unicode_vista.c
 * PROGRAMMER:        Alex Ionescu (alex@relsoft.net)
 *                    Emanuele Aliberti
 *                    Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#include <wine/unicode.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    _In_reads_(String1Length) PCWCH String1,
    _In_ SIZE_T String1Length,
    _In_reads_(String2Length) PCWCH String2,
    _In_ SIZE_T String2Length,
    _In_ BOOLEAN CaseInSensitive)
{
    LONG Result = 0;
    SIZE_T MinStringLength = min(String1Length, String2Length);
    SIZE_T Index;

    if (CaseInSensitive)
    {
        for (Index = 0; Index < MinStringLength; Index++)
        {
            WCHAR Char1 = RtlUpcaseUnicodeChar(String1[Index]);
            WCHAR Char2 = RtlUpcaseUnicodeChar(String2[Index]);
            Result = Char1 - Char2;
            if (Result != 0)
            {
                return Result;
            }
        }
    }
    else
    {
        for (Index = 0; Index < MinStringLength; Index++)
        {
            Result = String1[Index] - String2[Index];
            if (Result != 0)
            {
                return Result;
            }
        }
    }

    return String1Length - String2Length;
}
