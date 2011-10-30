/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Emanuele Aliberti
 *                  Thomas Weidenmueller
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
GetEnvironmentVariableA(IN LPCSTR lpName,
                        IN LPSTR lpBuffer,
                        IN DWORD nSize)
{
    ANSI_STRING VarName, VarValue;
    UNICODE_STRING VarNameU, VarValueU;
    PWSTR Buffer;
    ULONG Result = 0, UniSize = 0;
    NTSTATUS Status;

    /* Initialize all the strings */
    RtlInitAnsiString(&VarName, lpName);
    RtlInitUnicodeString(&VarNameU, NULL);
    RtlInitUnicodeString(&VarValueU, NULL);
    Status = RtlAnsiStringToUnicodeString(&VarNameU, &VarName, TRUE);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Check if the size is too big to fit */
    if (nSize <= UNICODE_STRING_MAX_BYTES)
    {
        /* Keep the given size, minus a NULL-char */
        if (nSize) UniSize = nSize * sizeof(WCHAR) - sizeof(UNICODE_NULL);
    }
    else
    {
        /* Set the maximum possible */
        UniSize = UNICODE_STRING_MAX_BYTES - sizeof(UNICODE_NULL);
    }

    /* Allocate the value string buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, UniSize);
    if (!Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* And initialize its string */
    RtlInitEmptyUnicodeString(&VarValueU, Buffer, UniSize);

    /* Acquire the PEB lock since we'll be querying variables now */
    RtlAcquirePebLock();

    /* Query the variable */
    Status = RtlQueryEnvironmentVariable_U(NULL, &VarNameU, &VarValueU);
    if ((NT_SUCCESS(Status)) && !(nSize)) Status = STATUS_BUFFER_TOO_SMALL;

    /* Check if we didn't have enough space */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Fixup the length that the API returned */
        VarValueU.MaximumLength = VarValueU.Length + 2;

        /* Free old Unicode buffer */
        RtlFreeHeap(RtlGetProcessHeap(), 0, VarValueU.Buffer);

        /* Allocate new one */
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VarValueU.MaximumLength);
        if (Buffer)
        {
            /* Query the variable so we can know its size */
            VarValueU.Buffer = Buffer;
            Status = RtlQueryEnvironmentVariable_U(NULL, &VarNameU, &VarValueU);
            if (NT_SUCCESS(Status))
            {
                /* Get the ASCII length of the variable */
                Result = RtlUnicodeStringToAnsiSize(&VarValueU);
            }
        }
        else
        {
            /* Set failure status */
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        /* Check if the size is too big to fit */
        if (nSize <= MAXULONG)
        {
            /* Keep the given size, minus a NULL-char */
            if (nSize) nSize = nSize - sizeof(ANSI_NULL);
        }
        else
        {
            /* Set the maximum possible */
            nSize = MAXULONG - sizeof(ANSI_NULL);
        }

        /* Check the size */
        Result = RtlUnicodeStringToAnsiSize(&VarValueU);
        if (Result <= nSize)
        {
            /* Convert the string */
            RtlInitEmptyAnsiString(&VarValue, lpBuffer, nSize);
            Status = RtlUnicodeStringToAnsiString(&VarValue, &VarValueU, FALSE);
            if (NT_SUCCESS(Status))
            {
                /* NULL-terminate and set the final length */
                lpBuffer[VarValue.Length] = ANSI_NULL;
                Result = VarValue.Length;
            }
        }
    }

    /* Release the lock */
    RtlReleasePebLock();

Quickie:
    /* Free the strings */
    RtlFreeUnicodeString(&VarNameU);
    if (VarValueU.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, VarValueU.Buffer);

    /* Check if we suceeded */
    if (!NT_SUCCESS(Status))
    {
        /* We did not, clear the result and set the error code */
        BaseSetLastNTError(Status);
        Result = 0;
    }

    /* Return the result */
    return Result;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetEnvironmentVariableW(IN LPCWSTR lpName,
                        IN LPWSTR lpBuffer,
                        IN DWORD nSize)
{
    UNICODE_STRING VarName, VarValue;
    NTSTATUS Status;

    if (nSize <= UNICODE_STRING_MAX_BYTES)
    {
        if (nSize) nSize = nSize * sizeof(WCHAR) - sizeof(UNICODE_NULL);
    }
    else
    {
        nSize = UNICODE_STRING_MAX_BYTES - sizeof(UNICODE_NULL);
    }

    Status = RtlInitUnicodeStringEx(&VarName, lpName);
    if (NT_SUCCESS(Status))
    {
        RtlInitEmptyUnicodeString(&VarValue, lpBuffer, nSize);

        Status = RtlQueryEnvironmentVariable_U(NULL, &VarName, &VarValue);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                return (VarValue.Length / sizeof(WCHAR)) + sizeof(ANSI_NULL);
            }
            BaseSetLastNTError (Status);
            return 0;
        }

        lpBuffer[VarValue.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    return (VarValue.Length / sizeof(WCHAR));
}

/*
 * @implemented
 */
BOOL
WINAPI
SetEnvironmentVariableA(IN LPCSTR lpName,
                        IN LPCSTR lpValue)
{
    ANSI_STRING VarName, VarValue;
    UNICODE_STRING VarNameU, VarValueU;
    NTSTATUS Status;

    RtlInitAnsiString(&VarName, (LPSTR)lpName);
    Status = RtlAnsiStringToUnicodeString(&VarNameU, &VarName, TRUE);
    if (NT_SUCCESS(Status))
    {
        if (lpValue)
        {
            RtlInitAnsiString(&VarValue, (LPSTR)lpValue);
            Status = RtlAnsiStringToUnicodeString(&VarValueU, &VarValue, TRUE);
            if (NT_SUCCESS(Status))
            {
                Status = RtlSetEnvironmentVariable(NULL, &VarNameU, &VarValueU);
                RtlFreeUnicodeString(&VarValueU);
            }
        }
        else
        {
            Status = RtlSetEnvironmentVariable(NULL, &VarNameU, NULL);
        }

        RtlFreeUnicodeString(&VarNameU);

        if (NT_SUCCESS(Status)) return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetEnvironmentVariableW(IN LPCWSTR lpName,
                        IN LPCWSTR lpValue)
{
    UNICODE_STRING VarName, VarValue;
    NTSTATUS Status;

    Status = RtlInitUnicodeStringEx(&VarName, lpName);
    if (NT_SUCCESS(Status))
    {
        if (lpValue)
        {
            Status = RtlInitUnicodeStringEx(&VarValue, lpValue);
            if (NT_SUCCESS(Status))
            {
                Status = RtlSetEnvironmentVariable(NULL, &VarName, &VarValue);
            }
        }
        else
        {
            Status = RtlSetEnvironmentVariable(NULL, &VarName, NULL);
        }

        if (NT_SUCCESS(Status)) return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
LPSTR
WINAPI
GetEnvironmentStringsA(VOID)
{
    ULONG Length, Size;
    NTSTATUS Status;
    PWCHAR Environment, p;
    PCHAR Buffer = NULL;

    RtlAcquirePebLock();
    p = Environment = NtCurrentPeb()->ProcessParameters->Environment;

    do
    {
        p += wcslen(Environment) + 1;
    } while (*p);

    Length = p - Environment + sizeof(UNICODE_NULL);

    Status = RtlUnicodeToMultiByteSize(&Size, Environment, Length);
    if (NT_SUCCESS(Status))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (Buffer)
        {
            Status = RtlUnicodeToOemN(Buffer, Size, 0, Environment, Length);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
                Buffer = NULL;

                BaseSetLastNTError(Status);
            }
        }
        else
        {
            BaseSetLastNTError(STATUS_NO_MEMORY);
        }
    }
    else
    {
        BaseSetLastNTError(Status);
    }

    RtlReleasePebLock();
    return Buffer;
}

/*
 * @implemented
 */
LPWSTR
WINAPI
GetEnvironmentStringsW(VOID)
{
    PWCHAR Environment, p;
    ULONG Length;

    RtlAcquirePebLock();

    p = Environment = NtCurrentPeb()->ProcessParameters->Environment;

    do
    {
        p += wcslen(Environment) + 1;
    } while (*p);

    Length = p - Environment + sizeof(UNICODE_NULL);

    p = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (p)
    {
        RtlCopyMemory(p, Environment, Length);
    }
    else
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
    }

    RtlReleasePebLock();
    return p;
}

/*
 * @implemented
 */
BOOL
WINAPI
FreeEnvironmentStringsA(IN LPSTR EnvironmentStrings)
{
    return (BOOL)RtlFreeHeap(RtlGetProcessHeap(), 0, EnvironmentStrings);
}

/*
 * @implemented
 */
BOOL
WINAPI
FreeEnvironmentStringsW(IN LPWSTR EnvironmentStrings)
{
    return (BOOL)RtlFreeHeap(RtlGetProcessHeap(), 0, EnvironmentStrings);
}

/*
 * @implemented
 */
DWORD
WINAPI
ExpandEnvironmentStringsA(IN LPCSTR lpSrc,
                          IN LPSTR lpDst,
                          IN DWORD nSize)
{
    ANSI_STRING Source, Dest;
    UNICODE_STRING SourceU, DestU;
    PWSTR Buffer;
    ULONG Result = 0, UniSize = 0, Length;
    NTSTATUS Status;

    /* Initialize all the strings */
    RtlInitAnsiString(&Source, lpSrc);
    RtlInitUnicodeString(&SourceU, NULL);
    RtlInitUnicodeString(&DestU, NULL);
    Status = RtlAnsiStringToUnicodeString(&SourceU, &Source, TRUE);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Check if the size is too big to fit */
    if (nSize <= UNICODE_STRING_MAX_BYTES)
    {
        /* Keep the given size, minus a NULL-char */
        if (nSize) UniSize = nSize * sizeof(WCHAR) - sizeof(UNICODE_NULL);
    }
    else
    {
        /* Set the maximum possible */
        UniSize = UNICODE_STRING_MAX_BYTES - sizeof(UNICODE_NULL);
    }

    /* Allocate the value string buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, UniSize);
    if (!Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* And initialize its string */
    RtlInitEmptyUnicodeString(&DestU, Buffer, UniSize);

    /* Query the variable */
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U(NULL, &SourceU, &DestU, &Length);

    /* Check if we didn't have enough space */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Fixup the length that the API returned */
        DestU.MaximumLength = Length;

        /* Free old Unicode buffer */
        RtlFreeHeap(RtlGetProcessHeap(), 0, DestU.Buffer);

        /* Allocate new one */
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
        if (Buffer)
        {
            /* Query the variable so we can know its size */
            DestU.Buffer = Buffer;
            Status = RtlExpandEnvironmentStrings_U(NULL, &SourceU, &DestU, &Length);
            if (NT_SUCCESS(Status))
            {
                /* Get the ASCII length of the variable */
                Result = RtlUnicodeStringToAnsiSize(&DestU);
            }
        }
        else
        {
            /* Set failure status */
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        /* Check if the size is too big to fit */
        if (nSize <= MAXULONG)
        {
            /* Keep the given size, minus a NULL-char */
            if (nSize) nSize = nSize - sizeof(ANSI_NULL);
        }
        else
        {
            /* Set the maximum possible */
            nSize = MAXULONG - sizeof(ANSI_NULL);
        }

        /* Check the size */
        Result = RtlUnicodeStringToAnsiSize(&DestU);
        if (Result <= nSize)
        {
            /* Convert the string */
            RtlInitEmptyAnsiString(&Dest, lpDst, nSize);
            Status = RtlUnicodeStringToAnsiString(&Dest, &DestU, FALSE);
            if (!NT_SUCCESS(Status))
            {
                /* Clear the destination */
                *lpDst = ANSI_NULL;
            }
        }
    }
Quickie:
    /* Free the strings */
    RtlFreeUnicodeString(&SourceU);
    if (DestU.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, DestU.Buffer);

    /* Check if we suceeded */
    if (!NT_SUCCESS(Status))
    {
        /* We did not, clear the result and set the error code */
        BaseSetLastNTError(Status);
        Result = 0;
    }

    /* Return the result */
    return Result;
}

/*
 * @implemented
 */
DWORD
WINAPI
ExpandEnvironmentStringsW(IN LPCWSTR lpSrc,
                          IN LPWSTR lpDst,
                          IN DWORD nSize)
{
    UNICODE_STRING Source, Destination;
    NTSTATUS Status;;

    RtlInitUnicodeString(&Source, (LPWSTR)lpSrc);

    /* make sure we don't overflow the maximum UNICODE_STRING size */
    if (nSize > UNICODE_STRING_MAX_BYTES) nSize = UNICODE_STRING_MAX_BYTES;

    RtlInitEmptyUnicodeString(&Destination, lpDst, nSize * sizeof(WCHAR));
    Status = RtlExpandEnvironmentStrings_U(NULL,
                                           &Source,
                                           &Destination,
                                           &nSize);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
    {
        return nSize / sizeof(WCHAR);
    }

    BaseSetLastNTError (Status);
    return 0;

}

/*
 * @implemented
 */
BOOL
WINAPI
SetEnvironmentStringsA(IN LPCH NewEnvironment)
{
    STUB;
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetEnvironmentStringsW(IN LPWCH NewEnvironment)
{
    STUB;
    return FALSE;
}

/* EOF */
