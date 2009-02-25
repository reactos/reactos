/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            include/ddk/ntstrsafe.h
 * PURPOSE:         Safe String Library for NT Code (Native/Kernel)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTSTRSAFE_H_INCLUDED_
#define _NTSTRSAFE_H_INCLUDED_

//
// Dependencies
//
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

//
// Maximum limits: allow overriding the maximum
//
#ifndef NTSTRSAFE_MAX_CCH
#define NTSTRSAFE_MAX_CCH       2147483647
#endif
#define NTSTRSAFE_MAX_LENGTH    (NTSTRSAFE_MAX_CCH - 1)

//
// Typedefs
//
typedef unsigned long DWORD;

/* PRIVATE FUNCTIONS *********************************************************/

FORCEINLINE
NTSTATUS
NTAPI
RtlStringLengthWorkerA(IN PCHAR String,
                       IN SIZE_T MaxLength,
                       OUT PSIZE_T ReturnLength OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T LocalMax = MaxLength;

    while (MaxLength && (*String != ANSI_NULL))
    {
        String++;
        MaxLength--;
    }

    if (!MaxLength) Status = STATUS_INVALID_PARAMETER;

    if (ReturnLength)
    {
        if (NT_SUCCESS(Status))
        {
            *ReturnLength = LocalMax - MaxLength;
        }
        else
        {
            *ReturnLength = 0;
        }
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringValidateDestA(IN PCHAR Destination,
                       IN SIZE_T Length,
                       OUT PSIZE_T ReturnLength OPTIONAL,
                       IN SIZE_T MaxLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!(Length) || (Length > MaxLength)) Status = STATUS_INVALID_PARAMETER;

    if (ReturnLength)
    {
        if (NT_SUCCESS(Status))
        {
            Status = RtlStringLengthWorkerA(Destination,
                                            Length,
                                            ReturnLength);
        }
        else
        {
            *ReturnLength = 0;
        }
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringExValidateDestA(IN OUT PCHAR *Destination,
                         IN OUT PSIZE_T DestinationLength,
                         OUT PSIZE_T ReturnLength OPTIONAL,
                         IN SIZE_T MaxLength,
                         IN DWORD Flags)
{
    ASSERTMSG("We don't support Extended Flags yet!\n", Flags == 0);
    return RtlStringValidateDestA(*Destination,
                                  *DestinationLength,
                                  ReturnLength,
                                  MaxLength);
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringExValidateSrcA(IN OUT PCCHAR *Source OPTIONAL,
                        IN OUT PSIZE_T ReturnLength OPTIONAL,
                        IN SIZE_T MaxLength,
                        IN DWORD Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERTMSG("We don't support Extended Flags yet!\n", Flags == 0);

    if ((ReturnLength) && (*ReturnLength >= MaxLength))
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringVPrintfWorkerA(OUT PCHAR Destination,
                        IN SIZE_T Length,
                        OUT PSIZE_T NewLength OPTIONAL,
                        IN PCCHAR Format,
                        IN va_list argList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG Return;
    SIZE_T MaxLength, LocalNewLength = 0;

    MaxLength = Length - 1;

    Return = _vsnprintf(Destination, MaxLength, Format, argList);
    if ((Return < 0) || ((SIZE_T)Return > MaxLength))
    {
        Destination += MaxLength;
        *Destination = ANSI_NULL;

        LocalNewLength = MaxLength;

        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if ((SIZE_T)Return == MaxLength)
    {
        Destination += MaxLength;
        *Destination = ANSI_NULL;

        LocalNewLength = MaxLength;
    }
    else
    {
        LocalNewLength = Return;
    }

    if (NewLength) *NewLength = LocalNewLength;
    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringCopyWorkerA(OUT PCHAR Destination,
                     IN SIZE_T Length,
                     OUT PSIZE_T NewLength OPTIONAL,
                     IN PCCHAR Source,
                     IN SIZE_T CopyLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T LocalNewLength = 0;

    while ((Length) && (CopyLength) && (*Source != ANSI_NULL))
    {
        *Destination++ = *Source++;
        Length--;
        CopyLength--;

        LocalNewLength++;
    }

    if (!Length)
    {
        Destination--;
        LocalNewLength--;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    *Destination = ANSI_NULL;

    if (NewLength) *NewLength = LocalNewLength;
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
RtlStringCbPrintfA(OUT PCHAR Destination,
                   IN SIZE_T Length,
                   IN PCHAR Format,
                   ...)
{
    NTSTATUS Status;
    SIZE_T CharLength = Length / sizeof(CHAR);
    va_list argList;

    Status = RtlStringValidateDestA(Destination,
                                    CharLength,
                                    NULL,
                                    NTSTRSAFE_MAX_CCH);
    if (NT_SUCCESS(Status))
    {
        va_start(argList, Format);
        Status = RtlStringVPrintfWorkerA(Destination,
                                         CharLength,
                                         NULL,
                                         Format,
                                         argList);
        va_end(argList);
    }

    return Status;
}

NTSTATUS
NTAPI
RtlStringCbPrintfExA(OUT PCHAR Destination,
                     IN SIZE_T Length,
                     OUT PCHAR *DestinationEnd OPTIONAL,
                     OUT PSIZE_T RemainingSize OPTIONAL,
                     IN DWORD Flags,
                     IN PCCHAR Format,
                     ...)
{
    NTSTATUS Status;
    SIZE_T CharLength = Length / sizeof(CHAR), Remaining, LocalNewLength = 0;
    PCHAR LocalDestinationEnd;
    va_list argList;
    ASSERTMSG("We don't support Extended Flags yet!\n", Flags == 0);

    Status = RtlStringExValidateDestA(&Destination,
                                      &CharLength,
                                      NULL,
                                      NTSTRSAFE_MAX_CCH,
                                      Flags);
    if (NT_SUCCESS(Status))
    {
        LocalDestinationEnd = Destination;
        Remaining = CharLength;

        Status = RtlStringExValidateSrcA(&Format,
                                         NULL,
                                         NTSTRSAFE_MAX_CCH,
                                         Flags);
        if (NT_SUCCESS(Status))
        {
            if (!Length)
            {
                if (*Format != ANSI_NULL)
                {
                    if (!Destination)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                va_start(argList, Format);
                Status = RtlStringVPrintfWorkerA(Destination,
                                                 CharLength,
                                                 &LocalNewLength,
                                                 Format,
                                                 argList);
                va_end(argList);

                LocalDestinationEnd = Destination + LocalNewLength;
                Remaining = CharLength - LocalNewLength;
            }
        }
        else
        {
            if (Length) *Destination = ANSI_NULL;
        }

        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
        {
            if (DestinationEnd) *DestinationEnd = LocalDestinationEnd;

            if (RemainingSize)
            {
                *RemainingSize = (Remaining * sizeof(CHAR)) +
                                 (Length % sizeof(CHAR));
            }
        }
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringCbCopyExA(OUT PCHAR Destination,
                   IN SIZE_T Length,
                   IN PCCHAR Source,
                   OUT PCHAR *DestinationEnd OPTIONAL,
                   OUT PSIZE_T RemainingSize OPTIONAL,
                   IN DWORD Flags)
{
    NTSTATUS Status;
    SIZE_T CharLength = Length / sizeof(CHAR), Copied = 0, Remaining;
    PCHAR LocalDestinationEnd;
    ASSERTMSG("We don't support Extended Flags yet!\n", Flags == 0);

    Status = RtlStringExValidateDestA(&Destination,
                                      &Length,
                                      NULL,
                                      NTSTRSAFE_MAX_CCH,
                                      Flags);
    if (NT_SUCCESS(Status))
    {
        LocalDestinationEnd = Destination;
        Remaining = CharLength;

        Status = RtlStringExValidateSrcA(&Source,
                                         NULL,
                                         NTSTRSAFE_MAX_CCH,
                                         Flags);
        if (NT_SUCCESS(Status))
        {
            if (!CharLength)
            {
                if (*Source != ANSI_NULL)
                {
                    if (!Destination)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                Status = RtlStringCopyWorkerA(Destination,
                                              CharLength,
                                              &Copied,
                                              Source,
                                              NTSTRSAFE_MAX_LENGTH);

                LocalDestinationEnd = Destination + Copied;
                Remaining = CharLength - Copied;
            }
        }
        else
        {
            if (CharLength) *Destination = ANSI_NULL;
        }

        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
        {
            if (DestinationEnd) *DestinationEnd = LocalDestinationEnd;

            if (RemainingSize)
            {
                *RemainingSize = (Remaining * sizeof(CHAR)) +
                                 (Length % sizeof(CHAR));
            }
        }
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringCbCatExA(IN OUT PCHAR Destination,
                  IN SIZE_T Length,
                  IN PCCHAR Source,
                  OUT PCHAR *DestinationEnd OPTIONAL,
                  OUT PSIZE_T RemainingSize OPTIONAL,
                  IN DWORD Flags)
{
    NTSTATUS Status;
    SIZE_T CharLength = Length / sizeof(CHAR);
    SIZE_T DestinationLength, Remaining, Copied = 0;
    PCHAR LocalDestinationEnd;
    ASSERTMSG("We don't support Extended Flags yet!\n", Flags == 0);

    Status = RtlStringExValidateDestA(&Destination,
                                      &CharLength,
                                      &DestinationLength,
                                      NTSTRSAFE_MAX_CCH,
                                      Flags);
    if (NT_SUCCESS(Status))
    {
        LocalDestinationEnd = Destination + DestinationLength;
        Remaining = CharLength - DestinationLength;

        Status = RtlStringExValidateSrcA(&Source,
                                         NULL,
                                         NTSTRSAFE_MAX_CCH,
                                         Flags);
        if (NT_SUCCESS(Status))
        {
            if (Remaining <= 1)
            {
                if (*Source != ANSI_NULL)
                {
                    if (!Destination)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                Status = RtlStringCopyWorkerA(LocalDestinationEnd,
                                              Remaining,
                                              &Copied,
                                              Source,
                                              NTSTRSAFE_MAX_LENGTH);

                LocalDestinationEnd = LocalDestinationEnd + Copied;
                Remaining = Remaining - Copied;
            }
        }

        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
        {
            if (DestinationEnd) *DestinationEnd = LocalDestinationEnd;

            if (RemainingSize)
            {
                *RemainingSize = (Remaining * sizeof(CHAR)) +
                                 (Length % sizeof(CHAR));
            }
        }
    }

    return Status;
}

FORCEINLINE
NTSTATUS
NTAPI
RtlStringCbCopyA(OUT PCHAR Destination,
                 IN SIZE_T Length,
                 IN PCCHAR Source)
{
    NTSTATUS Status;
    SIZE_T CharLength = Length / sizeof(CHAR);

    Status = RtlStringValidateDestA(Destination,
                                    CharLength,
                                    NULL,
                                    NTSTRSAFE_MAX_CCH);
    if (NT_SUCCESS(Status))
    {
        Status = RtlStringCopyWorkerA(Destination,
                                      CharLength,
                                      NULL,
                                      Source,
                                      NTSTRSAFE_MAX_LENGTH);
    }

    return Status;
}

#endif
