/*
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

/* INCLUDES *******************************************************************/

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
    ULONG Result = 0;
    USHORT UniSize;
    NTSTATUS Status;

    /* Initialize all the strings */
    RtlInitAnsiString(&VarName, lpName);
    RtlInitUnicodeString(&VarNameU, NULL);
    RtlInitUnicodeString(&VarValueU, NULL);
    Status = RtlAnsiStringToUnicodeString(&VarNameU, &VarName, TRUE);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Check if the size is too big to fit */
    UniSize = UNICODE_STRING_MAX_CHARS - 2;
    if (nSize <= UniSize)
    {
        /* It fits, but was there a string at all? */
        if (nSize)
        {
            /* Keep the given size, minus a NULL-char */
            UniSize = (USHORT)(nSize - 1);
        }
        else
        {
            /* No size */
            UniSize = 0;
        }
    }
    else
    {
        /* String is too big, so we need to return a NULL char as well */
        UniSize--;
    }

    /* Allocate the value string buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, UniSize * sizeof(WCHAR));
    if (!Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* And initialize its string */
    RtlInitEmptyUnicodeString(&VarValueU, Buffer, UniSize * sizeof(WCHAR));

    /* Acquire the PEB lock since we'll be querying variables now */
    RtlAcquirePebLock();

    /* Query the variable */
    Status = RtlQueryEnvironmentVariable_U(NULL, &VarNameU, &VarValueU);
    if ((NT_SUCCESS(Status)) && !(nSize)) Status = STATUS_BUFFER_TOO_SMALL;

    /* Check if we didn't have enough space */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Fixup the length that the API returned */
        VarValueU.MaximumLength = VarValueU.Length + sizeof(UNICODE_NULL);

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
            VarValueU.Buffer = NULL;
        }
    }
    else if (NT_SUCCESS(Status))
    {
        /* Check if the size is too big to fit */
        UniSize = UNICODE_STRING_MAX_BYTES - 1;
        if (nSize <= UniSize) UniSize = (USHORT)nSize;

        /* Check the size */
        Result = RtlUnicodeStringToAnsiSize(&VarValueU);
        if (Result <= UniSize)
        {
            /* Convert the string */
            RtlInitEmptyAnsiString(&VarValue, lpBuffer, UniSize);
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
    USHORT UniSize;

    if (nSize <= (UNICODE_STRING_MAX_CHARS - 1))
    {
        if (nSize)
        {
            UniSize = (USHORT)nSize * sizeof(WCHAR) - sizeof(UNICODE_NULL);
        }
        else
        {
            UniSize = 0;
        }
    }
    else
    {
        UniSize = UNICODE_STRING_MAX_BYTES - sizeof(UNICODE_NULL);
    }

    Status = RtlInitUnicodeStringEx(&VarName, lpName);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    RtlInitEmptyUnicodeString(&VarValue, lpBuffer, UniSize);

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
        p += wcslen(p) + 1;
    } while (*p);

    Length = p - Environment + 1;

    Status = RtlUnicodeToMultiByteSize(&Size, Environment, Length * sizeof(WCHAR));
    if (NT_SUCCESS(Status))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (Buffer)
        {
            Status = RtlUnicodeToOemN(Buffer, Size, 0, Environment, Length * sizeof(WCHAR));
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
        p += wcslen(p) + 1;
    } while (*p);

    Length = p - Environment + 1;

    p = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (p)
    {
        RtlCopyMemory(p, Environment, Length * sizeof(WCHAR));
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
    return RtlFreeHeap(RtlGetProcessHeap(), 0, EnvironmentStrings);
}

/*
 * @implemented
 */
BOOL
WINAPI
FreeEnvironmentStringsW(IN LPWSTR EnvironmentStrings)
{
    return RtlFreeHeap(RtlGetProcessHeap(), 0, EnvironmentStrings);
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
    ULONG Result = 0, Length;
    USHORT UniSize;
    NTSTATUS Status;

    /* Check if the size is too big to fit */
    UniSize = UNICODE_STRING_MAX_CHARS - 2;
    if (nSize <= UniSize) UniSize = (USHORT)nSize;

    /* Clear the input buffer */
    if (lpDst) *lpDst = ANSI_NULL;

    /* Initialize all the strings */
    RtlInitAnsiString(&Source, lpSrc);
    RtlInitUnicodeString(&SourceU, NULL);
    RtlInitUnicodeString(&DestU, NULL);
    Status = RtlAnsiStringToUnicodeString(&SourceU, &Source, TRUE);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* If the string fit in, make space for a NULL char */
    if (UniSize)
    {
        UniSize--;
    }
    else
    {
        /* No input size, so no string size */
        UniSize = 0;
    }

    /* Allocate the value string buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, UniSize * sizeof(WCHAR));
    if (!Buffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* And initialize its string */
    RtlInitEmptyUnicodeString(&DestU, Buffer, UniSize * sizeof(WCHAR));

    /* Query the variable */
    Length = 0;
    Status = RtlExpandEnvironmentStrings_U(NULL, &SourceU, &DestU, &Length);

    /* Check if we didn't have enough space */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Fixup the length that the API returned */
        DestU.MaximumLength = (SHORT)Length;

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
                /* Get the ASCII length of the variable, add a byte for NULL */
                Result = RtlUnicodeStringToAnsiSize(&DestU) + sizeof(ANSI_NULL);
            }
        }
        else
        {
            /* Set failure status */
            Status = STATUS_NO_MEMORY;
            DestU.Buffer = NULL;
        }
    }
    else if (NT_SUCCESS(Status))
    {
        /* Check if the size is too big to fit */
        UniSize = UNICODE_STRING_MAX_BYTES - 1;
        if (nSize <= UniSize) UniSize = (USHORT)nSize;

        /* Check the size */
        Result = RtlUnicodeStringToAnsiSize(&DestU);
        if (Result <= UniSize)
        {
            /* Convert the string */
            RtlInitEmptyAnsiString(&Dest, lpDst, UniSize);
            Status = RtlUnicodeStringToAnsiString(&Dest, &DestU, FALSE);

            /* Write a NULL-char in case of failure only */
            if (!NT_SUCCESS(Status)) *lpDst = ANSI_NULL;
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
    NTSTATUS Status;
    USHORT UniSize;

    UniSize = min(nSize, UNICODE_STRING_MAX_CHARS - 2);

    RtlInitUnicodeString(&Source, (LPWSTR)lpSrc);
    RtlInitEmptyUnicodeString(&Destination, lpDst, UniSize * sizeof(WCHAR));

    Status = RtlExpandEnvironmentStrings_U(NULL,
                                           &Source,
                                           &Destination,
                                           &nSize);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
    {
        return nSize / sizeof(WCHAR);
    }

    BaseSetLastNTError(Status);
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
