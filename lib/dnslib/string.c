/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/string.c
 * PURPOSE:     functions for string manipulation and conversion.
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

ULONG
WINAPI
Dns_StringCopy(OUT PVOID Destination,
               IN OUT PULONG DestinationSize,
               IN PVOID String,
               IN ULONG StringSize OPTIONAL,
               IN DWORD InputType,
               IN DWORD OutputType)
{
    ULONG DestSize;
    ULONG OutputSize = 0;

    /* Check if the caller already gave us the string size */
    if (!StringSize)
    {
        /* He didn't, get the input type */
        if (InputType == UnicodeString)
        {
            /* Unicode string, calculate the size */
            StringSize = (ULONG)wcslen((LPWSTR)String);
        }
        else
        {
            /* ANSI or UTF-8 sting, get the size */
            StringSize = (ULONG)strlen((LPSTR)String);
        }
    }

    /* Check if we have a limit on the desination size */
    if (DestinationSize)
    {
        /* Make sure that we can respect it */
        DestSize = Dns_GetBufferLengthForStringCopy(String,
                                                    StringSize,
                                                    InputType,
                                                    OutputType);
        if (*DestinationSize < DestSize)
        {
            /* Fail due to missing buffer space */
            SetLastError(ERROR_MORE_DATA);

            /* Return how much data we actually need */
            *DestinationSize = DestSize;
            return 0;
        }
        else if (!DestSize)
        {
            /* Fail due to invalid data */
            SetLastError(ERROR_INVALID_DATA);
            return 0;
        }

        /* Return how much data we actually need */
        *DestinationSize = DestSize;
    }

    /* Now check if this is a Unicode String as input */
    if (InputType == UnicodeString)
    {
        /* Check if the output is ANSI */
        if (OutputType == AnsiString)
        {
            /* Convert and return the final desination size */
            OutputSize = WideCharToMultiByte(CP_ACP,
                                             0,
                                             String,
                                             StringSize,
                                             Destination,
                                             -1,
                                             NULL,
                                             NULL) + 1;
        }
        else if (OutputType == UnicodeString)
        {
            /* Copy the string */
            StringSize = StringSize * sizeof(WCHAR);
            RtlMoveMemory(Destination, String, StringSize);

            /* Return output length */
            OutputSize = StringSize + 2;
        }
        else if (OutputType == Utf8String)
        {
            /* FIXME */
            OutputSize = 0;
        }
    }
    else if (InputType == AnsiString)
    {
        /* It's ANSI, is the output ansi too? */
        if (OutputType == AnsiString)
        {
            /* Copy the string */
            RtlMoveMemory(Destination, String, StringSize);

            /* Return output length */
            OutputSize = StringSize + 1;
        }
        else if (OutputType == UnicodeString)
        {
            /* Convert to Unicode and return size */
            OutputSize = MultiByteToWideChar(CP_ACP,
                                             0,
                                             String,
                                             StringSize,
                                             Destination,
                                             -1) * sizeof(WCHAR) + 2;
        }
        else if (OutputType == Utf8String)
        {
            /* FIXME */
            OutputSize = 0;
        }
    }
    else if (InputType == Utf8String)
    {
        /* FIXME */
        OutputSize = 0;
    }

    /* Return the output size */
    return OutputSize;
}

LPWSTR
WINAPI
Dns_CreateStringCopy_W(IN LPWSTR Name)
{
    SIZE_T StringLength;
    LPWSTR NameCopy;

    /* Make sure that we have a name */
    if (!Name)
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Find out the size of the string */
    StringLength = (wcslen(Name) + 1) * sizeof(WCHAR);

    /* Allocate space for the copy */
    NameCopy = Dns_AllocZero(StringLength);
    if (NameCopy)
    {
        /* Copy it */
        RtlCopyMemory(NameCopy, Name, StringLength);
    }
    else
    {
        /* Fail */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    /* Return the copy */
    return NameCopy;
}

ULONG
WINAPI
Dns_GetBufferLengthForStringCopy(IN PVOID String,
                                 IN ULONG Size OPTIONAL,
                                 IN DWORD InputType,
                                 IN DWORD OutputType)
{
    ULONG OutputSize = 0;

    /* Check what kind of string this is */
    if (InputType == UnicodeString)
    {
        /* Check if we have a size */
        if (!Size)
        {
            /* Get it ourselves */
            Size = (ULONG)wcslen(String);
        }

        /* Check the output type */
        if (OutputType == UnicodeString)
        {
            /* Convert the size to bytes */
            OutputSize = (Size + 1) * sizeof(WCHAR);
        }
        else if (OutputType == Utf8String)
        {
            /* FIXME */
            OutputSize = 0;
        }
        else
        {
            /* Find out how much it will be in ANSI bytes */
            OutputSize = WideCharToMultiByte(CP_ACP,
                                             0,
                                             String,
                                             Size,
                                             NULL,
                                             0,
                                             NULL,
                                             NULL) + 1;
        }
    }
    else if (InputType == AnsiString)
    {
        /* Check if we have a size */
        if (!Size)
        {
            /* Get it ourselves */
            Size = (ULONG)strlen(String);
        }

        /* Check the output type */
        if (OutputType == AnsiString)
        {
            /* Just add a byte for the null char */
            OutputSize = Size + 1;
        }
        else if (OutputType == UnicodeString)
        {
            /* Calculate the bytes for a Unicode string */
            OutputSize = (MultiByteToWideChar(CP_ACP,
                                              0,
                                              String,
                                              Size,
                                              NULL,
                                              0) + 1) * sizeof(WCHAR);
        }
        else if (OutputType == Utf8String)
        {
            /* FIXME */
            OutputSize = 0;
        }
    }
    else if (InputType == Utf8String)
    {
        /* FIXME */
        OutputSize = 0;
    }

    /* Return the size required */
    return OutputSize;
}

