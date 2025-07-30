/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
RtlSetEnvironmentStrings(_In_ PWCHAR NewEnvironment, _In_ ULONG NewEnvironmentSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateEnvironment(
    _In_ BOOLEAN Inherit,
    _Out_ PWSTR *OutEnvironment)
{
    MEMORY_BASIC_INFORMATION MemInfo;
    PVOID CurrentEnvironment, NewEnvironment = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T RegionSize = PAGE_SIZE;

    /* Check if we should inherit the current environment */
    if (Inherit)
    {
        /* In this case we need to lock the PEB */
        RtlAcquirePebLock();

        /* Get a pointer to the current Environment and check if it's not NULL */
        CurrentEnvironment = NtCurrentPeb()->ProcessParameters->Environment;
        if (CurrentEnvironment != NULL)
        {
            /* Query the size of the current environment allocation */
            Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                          CurrentEnvironment,
                                          MemoryBasicInformation,
                                          &MemInfo,
                                          sizeof(MEMORY_BASIC_INFORMATION),
                                          NULL);
            if (!NT_SUCCESS(Status))
            {
                RtlReleasePebLock();
                *OutEnvironment = NULL;
                return Status;
            }

            /* Allocate a new region of the same size */
            RegionSize = MemInfo.RegionSize;
            Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                             &NewEnvironment,
                                             0,
                                             &RegionSize,
                                             MEM_RESERVE | MEM_COMMIT,
                                             PAGE_READWRITE);
            if (!NT_SUCCESS(Status))
            {
                RtlReleasePebLock();
                *OutEnvironment = NULL;
                return Status;
            }

            /* Copy the current environment */
            RtlCopyMemory(NewEnvironment,
                          CurrentEnvironment,
                          MemInfo.RegionSize);
        }

        /* We are done with the PEB, release the lock */
        RtlReleasePebLock ();
    }

    /* Check if we still need an environment */
    if (NewEnvironment == NULL)
    {
        /* Allocate a new environment */
        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         &NewEnvironment,
                                         0,
                                         &RegionSize,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
        if (NT_SUCCESS(Status))
        {
            RtlZeroMemory(NewEnvironment, RegionSize);
        }
    }

    *OutEnvironment = NewEnvironment;

    return Status;
}


/*
 * @implemented
 */
VOID
NTAPI
RtlDestroyEnvironment(_In_ PWSTR Environment)
{
    SIZE_T Size = 0;

    NtFreeVirtualMemory(NtCurrentProcess(),
                        (PVOID)&Environment,
                        &Size,
                        MEM_RELEASE);
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings(
    _In_opt_ PVOID Environment,
    _In_reads_(SourceLength) PCWSTR SourceBuffer,
    _In_ SIZE_T SourceLength,
    _Out_writes_(DestMax) PWSTR Destination,
    _In_ SIZE_T DestMax,
    _Out_opt_ PSIZE_T Length)
{
    UNICODE_STRING Variable;
    UNICODE_STRING Value;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    NTSTATUS Status;
    PWSTR DestBuffer;
    PCWSTR CopyBuffer;
    PCWSTR VariableEnd;
    SIZE_T CopyLength;
    SIZE_T Tail;
    SIZE_T TotalLength = 1; /* for terminating NULL */

    DPRINT("RtlExpandEnvironmentStrings(%p, %S, %Iu, %p, %Iu, %p)\n",
           Environment, SourceBuffer, SourceLength, Destination, DestMax, Length);

    DestBuffer = Destination;

    while (SourceLength)
    {
        if (*SourceBuffer != L'%')
        {
            CopyBuffer = SourceBuffer;
            CopyLength = 0;
            while (SourceLength != 0 && *SourceBuffer != L'%')
            {
                SourceBuffer++;
                CopyLength++;
                SourceLength--;
            }
        }
        else
        {
           /* Process environment variable. */

            VariableEnd = SourceBuffer + 1;
            Tail = SourceLength - 1;
            while (*VariableEnd != L'%' && Tail != 0)
            {
                VariableEnd++;
                Tail--;
            }

            if (Tail != 0)
            {
                Variable.MaximumLength =
                    Variable.Length = (USHORT)(VariableEnd - (SourceBuffer + 1)) * sizeof(WCHAR);
                Variable.Buffer = (PWSTR)SourceBuffer + 1;

                Value.Length = 0;
                Value.MaximumLength = (USHORT)DestMax * sizeof(WCHAR);
                Value.Buffer = DestBuffer;

                Status = RtlQueryEnvironmentVariable_U(Environment, &Variable,
                                                       &Value);
                if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
                {
                    SourceBuffer = VariableEnd + 1;
                    SourceLength = Tail - 1;
                    TotalLength += Value.Length / sizeof(WCHAR);
                    if (Status != STATUS_BUFFER_TOO_SMALL)
                    {
                        DestBuffer += Value.Length / sizeof(WCHAR);
                        DestMax -= Value.Length / sizeof(WCHAR);
                    }
                    else
                    {
                        DestMax = 0;
                        ReturnStatus = STATUS_BUFFER_TOO_SMALL;
                    }
                    continue;
                }
                else
                {
                   /* Variable not found. */
                    CopyBuffer = SourceBuffer;
                    CopyLength = SourceLength - Tail + 1;
                    SourceLength -= CopyLength;
                    SourceBuffer += CopyLength;
                }
            }
            else
            {
               /* Unfinished variable name. */
                CopyBuffer = SourceBuffer;
                CopyLength = SourceLength;
                SourceLength = 0;
            }
        }

        TotalLength += CopyLength;
        if (DestMax)
        {
            if (DestMax < CopyLength)
            {
                CopyLength = DestMax;
                ReturnStatus = STATUS_BUFFER_TOO_SMALL;
            }
            RtlCopyMemory(DestBuffer, CopyBuffer, CopyLength * sizeof(WCHAR));
            DestMax -= CopyLength;
            DestBuffer += CopyLength;
        }
    }

    /* NULL-terminate the buffer. */
    if (DestMax)
        *DestBuffer = 0;
    else
        ReturnStatus = STATUS_BUFFER_TOO_SMALL;

    if (NT_SUCCESS(ReturnStatus))
    {
        ASSERT(TotalLength == (DestBuffer - Destination) + 1);
        //ASSERT(Destination[TotalLength - 1] == UNICODE_NULL);
    }

    if (Length != NULL)
        *Length = TotalLength;

    DPRINT("Destination %wZ\n", Destination);

    return ReturnStatus;
}

NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    _In_opt_ PWSTR Environment,
    _In_ PUNICODE_STRING SourceString,
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PULONG ReturnLength)
{
    SIZE_T ResultLength;
    NTSTATUS Status;

    DPRINT("RtlExpandEnvironmentStrings_U %p %wZ %p %p\n",
           Environment, SourceString, DestinationString, ReturnLength);

    /* Call the buffer version */
    Status = RtlExpandEnvironmentStrings(
        Environment,
        SourceString->Buffer,
        SourceString->Length / sizeof(WCHAR),
        DestinationString->Buffer,
        DestinationString->MaximumLength / sizeof(WCHAR),
        &ResultLength);

    /* Make sure the result length does not exceed the maximum */
    if (ResultLength > UNICODE_STRING_MAX_CHARS)
    {
        DPRINT("ReturnLength %lu exceeds maximum\n", ResultLength);
        Status = STATUS_UNSUCCESSFUL;
        ResultLength = 0;
    }

    if (NT_SUCCESS(Status))
    {
        DestinationString->Length =
            ((USHORT)ResultLength * sizeof(WCHAR)) - sizeof(UNICODE_NULL);
    }

    if (ReturnLength != NULL)
    {
        *ReturnLength = (ULONG)ResultLength * sizeof(WCHAR);
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlSetCurrentEnvironment(
    _In_ PWSTR NewEnvironment,
    _Out_opt_ PWSTR *OldEnvironment)
{
    PVOID EnvPtr;

    DPRINT("NewEnvironment 0x%p OldEnvironment 0x%p\n",
           NewEnvironment, OldEnvironment);

    RtlAcquirePebLock();

    EnvPtr = NtCurrentPeb()->ProcessParameters->Environment;
    NtCurrentPeb()->ProcessParameters->Environment = NewEnvironment;

    if (OldEnvironment != NULL)
        *OldEnvironment = EnvPtr;

    RtlReleasePebLock();
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    _In_z_ PWSTR *Environment,
    _In_ PUNICODE_STRING Name,
    _In_ PUNICODE_STRING Value)
{
    MEMORY_BASIC_INFORMATION mbi;
    UNICODE_STRING var;
    size_t hole_len, new_len, env_len = 0;
    WCHAR *new_env = 0, *env_end = 0, *wcs, *env, *val = 0, *tail = 0, *hole = 0;
    PWSTR head = NULL;
    SIZE_T size = 0, new_size;
    LONG f = 1;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("RtlSetEnvironmentVariable(Environment %p Name %wZ Value %wZ)\n",
           Environment, Name, Value);

    /* Variable name must not be empty */
    if (Name->Length < sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER;

     /* Variable names can't contain a '=' except as a first character. */
    for (wcs = Name->Buffer + 1;
         wcs < Name->Buffer + (Name->Length / sizeof(WCHAR));
         wcs++)
    {
        if (*wcs == L'=')
            return STATUS_INVALID_PARAMETER;
    }

    if (Environment)
    {
        env = *Environment;
    }
    else
    {
        RtlAcquirePebLock();
        env = NtCurrentPeb()->ProcessParameters->Environment;
    }

    if (env)
    {
       /* get environment length */
        wcs = env_end = env;
        do
        {
            env_end += wcslen(env_end) + 1;
        } while (*env_end);
        env_end++;
        env_len = env_end - env;
        DPRINT("environment length %lu characters\n", env_len);

        /* find where to insert */
        while (*wcs)
        {
            var.Buffer = wcs++;
            wcs = wcschr(wcs, L'=');
            if (wcs == NULL)
            {
                wcs = var.Buffer + wcslen(var.Buffer);
            }
            if (*wcs)
            {
                var.Length = (USHORT)(wcs - var.Buffer) * sizeof(WCHAR);
                var.MaximumLength = var.Length;
                val = ++wcs;
                wcs += wcslen(wcs);
                f = RtlCompareUnicodeString(&var, Name, TRUE);
                if (f >= 0)
                {
                    if (f) /* Insert before found */
                    {
                        hole = tail = var.Buffer;
                    }
                    else /* Exact match */
                    {
                        head = var.Buffer;
                        tail = ++wcs;
                        hole = val;
                    }
                    goto found;
                }
            }
            wcs++;
        }
        hole = tail = wcs; /* Append to environment */
    }

found:
    if (Value != NULL && Value->Buffer != NULL)
    {
        hole_len = tail - hole;
        /* calculate new environment size */
        new_size = Value->Length + sizeof(WCHAR);
        /* adding new variable */
        if (f)
            new_size += Name->Length + sizeof(WCHAR);
        new_len = new_size / sizeof(WCHAR);
        if (hole_len < new_len)
        {
           /* enlarge environment size */
           /* check the size of available memory */
            new_size += (env_len - hole_len) * sizeof(WCHAR);
            new_size = ROUND_UP(new_size, PAGE_SIZE);
            mbi.RegionSize = 0;
            DPRINT("new_size %lu\n", new_size);

            if (env)
            {
                Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                              env,
                                              MemoryBasicInformation,
                                              &mbi,
                                              sizeof(MEMORY_BASIC_INFORMATION),
                                              NULL);
                if (!NT_SUCCESS(Status))
                {
                    if (Environment == NULL)
                    {
                        RtlReleasePebLock();
                    }
                    return(Status);
                }
            }

            if (new_size > mbi.RegionSize)
            {
               /* reallocate memory area */
                Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                                 (PVOID)&new_env,
                                                 0,
                                                 &new_size,
                                                 MEM_RESERVE | MEM_COMMIT,
                                                 PAGE_READWRITE);
                if (!NT_SUCCESS(Status))
                {
                    if (Environment == NULL)
                    {
                        RtlReleasePebLock();
                    }
                    return(Status);
                }

                if (env)
                {
                    memmove(new_env,
                            env,
                            (hole - env) * sizeof(WCHAR));
                    hole = new_env + (hole - env);
                }
                else
                {
                   /* absolutely new environment */
                    tail = hole = new_env;
                    *hole = 0;
                    env_end = hole + 1;
                }
            }
        }

        /* move tail */
        memmove (hole + new_len, tail, (env_end - tail) * sizeof(WCHAR));

        if (new_env)
        {
           /* we reallocated environment, let's free the old one */
            if (Environment)
                *Environment = new_env;
            else
                NtCurrentPeb()->ProcessParameters->Environment = new_env;

            if (env)
            {
                size = 0;
                NtFreeVirtualMemory(NtCurrentProcess(),
                                    (PVOID)&env,
                                    &size,
                                    MEM_RELEASE);
            }
        }

        /* and now copy given stuff */
        if (f)
        {
           /* copy variable name and '=' character */
            memmove(hole,
                    Name->Buffer,
                    Name->Length);
            hole += Name->Length / sizeof(WCHAR);
            *hole++ = L'=';
        }

        /* copy value */
        memmove(hole,
                Value->Buffer,
                Value->Length);
        hole += Value->Length / sizeof(WCHAR);
        *hole = 0;
    }
    else
    {
       /* remove the environment variable */
        if (f == 0)
        {
            memmove(head,
                    tail,
                    (env_end - tail) * sizeof(WCHAR));
        }
    }

    if (Environment == NULL)
    {
        RtlReleasePebLock();
    }

    return(Status);
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_reads_(NameLength) PWSTR Name,
    _In_ SIZE_T NameLength,
    _Out_writes_(ValueLength) PWSTR Value,
    _In_ SIZE_T ValueLength,
    _Out_ PSIZE_T ReturnLength)
{
    NTSTATUS Status;
    PWSTR wcs;
    PWSTR varName;
    SIZE_T varNameLen;
    PWSTR val;
    BOOLEAN SysEnvUsed = FALSE;

    DPRINT("RtlQueryEnvironmentVariable_U Environment %p Variable %wZ Value %p\n",
           Environment, Name, Value);

    if (Environment == NULL)
    {
        PPEB Peb = RtlGetCurrentPeb();
        if (Peb)
        {
            RtlAcquirePebLock();
            Environment = Peb->ProcessParameters->Environment;
            SysEnvUsed = TRUE;
        }
    }

    if (Environment == NULL)
    {
        if (SysEnvUsed)
            RtlReleasePebLock();
        return STATUS_VARIABLE_NOT_FOUND;
    }

    *ReturnLength = 0;

    wcs = Environment;
    DPRINT("Starting search at :%p\n", wcs);
    while (*wcs)
    {
        varName = wcs++;
        wcs = wcschr(wcs, L'=');
        if (wcs == NULL)
        {
            wcs = varName + wcslen(varName);
            DPRINT("Search at :%S\n", wcs);
        }
        if (*wcs)
        {
            varNameLen = (wcs - varName);
            val = ++wcs;
            wcs += wcslen(wcs);
            DPRINT("Search at :%S\n", wcs);

            if ((varNameLen == NameLength) && _wcsnicmp(varName, Name, NameLength) == 0)
            {
                *ReturnLength = (wcs - val);
                if (*ReturnLength < ValueLength)
                {
                    memcpy(Value, val, (*ReturnLength + 1) * sizeof(WCHAR));
                    DPRINT("Value %S\n", val);
                    DPRINT("Return STATUS_SUCCESS\n");
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    if (ValueLength > 0)
                        Value[0] = UNICODE_NULL;
                    DPRINT("Return STATUS_BUFFER_TOO_SMALL\n");
                    *ReturnLength += 1;
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

                if (SysEnvUsed)
                    RtlReleasePebLock();

                return Status;
            }
        }

        wcs++;
    }

    if (SysEnvUsed)
        RtlReleasePebLock();

    DPRINT("Return STATUS_VARIABLE_NOT_FOUND: %wZ\n", Name);
    return STATUS_VARIABLE_NOT_FOUND;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    _In_opt_ PWSTR Environment,
    _In_ PCUNICODE_STRING Name,
    _Out_ PUNICODE_STRING Value)
{
    NTSTATUS Status;
    SIZE_T ReturnLength;

    Status = RtlQueryEnvironmentVariable(Environment,
                                         Name->Buffer,
                                         Name->Length / sizeof(WCHAR),
                                         Value->Buffer,
                                         Value->MaximumLength / sizeof(WCHAR),
                                         &ReturnLength);

    if (ReturnLength > UNICODE_STRING_MAX_CHARS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Correct for extra length returned by RtlQueryEnvironmentVariable */
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ReturnLength -= 1;
    }

    Value->Length = (USHORT)ReturnLength * sizeof(WCHAR);
    return Status;
}

/* EOF */
