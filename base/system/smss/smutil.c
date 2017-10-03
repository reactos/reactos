/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smutil.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#include <ndk/sefuncs.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

//
// Taken from an ASSERT
//
#define VALUE_BUFFER_SIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 512)

typedef struct _SMP_PRIVILEGE_STATE
{
    HANDLE TokenHandle;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    UCHAR OldBuffer[1024];
    TOKEN_PRIVILEGES NewBuffer;
} SMP_PRIVILEGE_STATE, *PSMP_PRIVILEGE_STATE;

UNICODE_STRING SmpDebugKeyword, SmpASyncKeyword, SmpAutoChkKeyword;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpAcquirePrivilege(IN ULONG Privilege,
                    OUT PVOID *PrivilegeState)
{
    PSMP_PRIVILEGE_STATE State;
    ULONG Size;
    NTSTATUS Status;

    /* Assume failure */
    *PrivilegeState = NULL;

    /* Acquire the state structure to hold everything we need */
    State = RtlAllocateHeap(SmpHeap,
                            0,
                            sizeof(SMP_PRIVILEGE_STATE) +
                            sizeof(TOKEN_PRIVILEGES) +
                            sizeof(LUID_AND_ATTRIBUTES));
    if (!State) return STATUS_NO_MEMORY;

    /* Open our token */
    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &State->TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(SmpHeap, 0, State);
        return Status;
    }

    /* Set one privilege in the enabled state */
    State->NewPrivileges = &State->NewBuffer;
    State->OldPrivileges = (PTOKEN_PRIVILEGES)&State->OldBuffer;
    State->NewPrivileges->PrivilegeCount = 1;
    State->NewPrivileges->Privileges[0].Luid = RtlConvertUlongToLuid(Privilege);
    State->NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    /* Adjust the privileges in the token */
    Size = sizeof(State->OldBuffer);
    Status = NtAdjustPrivilegesToken(State->TokenHandle,
                                     FALSE,
                                     State->NewPrivileges,
                                     Size,
                                     State->OldPrivileges,
                                     &Size);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Our static buffer is not big enough, allocate a bigger one */
        State->OldPrivileges = RtlAllocateHeap(SmpHeap, 0, Size);
        if (!State->OldPrivileges)
        {
            /* Out of memory, fail */
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Now try again */
        Status = NtAdjustPrivilegesToken(State->TokenHandle,
                                         FALSE,
                                         State->NewPrivileges,
                                         Size,
                                         State->OldPrivileges,
                                         &Size);
    }

    /* Normalize failure code and check for success */
    if (Status == STATUS_NOT_ALL_ASSIGNED) Status = STATUS_PRIVILEGE_NOT_HELD;
    if (NT_SUCCESS(Status))
    {
        /* We got the privilege, return */
        *PrivilegeState = State;
        return STATUS_SUCCESS;
    }

Quickie:
    /* Check if we used a dynamic buffer */
    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)&State->OldBuffer)
    {
        /* Free it */
        RtlFreeHeap(SmpHeap, 0, State->OldPrivileges);
    }

    /* Close the token handle and free the state structure */
    NtClose(State->TokenHandle);
    RtlFreeHeap(SmpHeap, 0, State);
    return Status;
}

VOID
NTAPI
SmpReleasePrivilege(IN PVOID PrivState)
{
    PSMP_PRIVILEGE_STATE State = (PSMP_PRIVILEGE_STATE)PrivState;

    /* Adjust the privileges in the token */
    NtAdjustPrivilegesToken(State->TokenHandle,
                            FALSE,
                            State->OldPrivileges,
                            0,
                            NULL,
                            NULL);

    /* Check if we used a dynamic buffer */
    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)&State->OldBuffer)
    {
        /* Free it */
        RtlFreeHeap(SmpHeap, 0, State->OldPrivileges);
    }

    /* Close the token handle and free the state structure */
    NtClose(State->TokenHandle);
    RtlFreeHeap(SmpHeap, 0, State);
}

NTSTATUS
NTAPI
SmpParseToken(IN PUNICODE_STRING Input,
              IN BOOLEAN SecondPass,
              OUT PUNICODE_STRING Token)
{
    PWCHAR p, pp;
    ULONG Length, TokenLength, InputLength;

    /* Initialize to NULL to start with */
    RtlInitUnicodeString(Token, NULL);

    /* Save the input length */
    InputLength = Input->Length;

    /* If the input string is empty, just return */
    if (InputLength == 0) return STATUS_SUCCESS;

    /* Parse the buffer until the first character */
    p = Input->Buffer;
    Length = 0;
    while (Length < InputLength)
    {
        if (*p > L' ' ) break;
        ++p;
        Length += sizeof(WCHAR);
    }

    /* Are we being called for argument pick-up? */
    if (SecondPass)
    {
        /* Then assume everything else is an argument */
        TokenLength = InputLength - Length * sizeof(WCHAR);
        pp = (PWSTR)((ULONG_PTR)p + TokenLength);
    }
    else
    {
        /* No -- so loop until the next space */
        pp = p;
        while (Length < InputLength)
        {
            if (*pp <= L' ' ) break;
            ++pp;
            Length += sizeof(WCHAR);
        }

        /* Now compute how long this token is, and loop until the next char */
        TokenLength = (ULONG_PTR)pp - (ULONG_PTR)p;
        while (Length < InputLength)
        {
            if (*pp > L' ' ) break;
            ++pp;
            Length += sizeof(WCHAR);
        }
    }

    /* Did we find a token? */
    if (TokenLength)
    {
        /* Allocate a buffer for it */
        Token->Buffer = RtlAllocateHeap(SmpHeap,
                                        SmBaseTag,
                                        TokenLength + sizeof(UNICODE_NULL));
        if (!Token->Buffer) return STATUS_NO_MEMORY;

        /* Fill in the unicode string to hold it */
        Token->MaximumLength = (USHORT)(TokenLength + sizeof(UNICODE_NULL));
        Token->Length = (USHORT)TokenLength;
        RtlCopyMemory(Token->Buffer, p, TokenLength);
        Token->Buffer[TokenLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Modify the input string with the position of where the next token begins */
    Input->Length -= (USHORT)((ULONG_PTR)pp - (ULONG_PTR)Input->Buffer);
    Input->Buffer = pp;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpParseCommandLine(IN PUNICODE_STRING CommandLine,
                    OUT PULONG Flags,
                    OUT PUNICODE_STRING FileName,
                    OUT PUNICODE_STRING Directory,
                    OUT PUNICODE_STRING Arguments)
{
    ULONG Length;
    UNICODE_STRING EnvString, PathString, CmdLineCopy, Token;
    WCHAR PathBuffer[MAX_PATH];
    PWCHAR FilePart;
    NTSTATUS Status;
    UNICODE_STRING FullPathString;

    /* Initialize output arguments to NULL */
    RtlInitUnicodeString(FileName, NULL);
    RtlInitUnicodeString(Arguments, NULL);
    if (Directory) RtlInitUnicodeString(Directory, NULL);

    /* Check if we haven't yet built a full default path or system root yet */
    if (!SmpSystemRoot.Length)
    {
        /* Initialize it based on shared user data string */
        RtlInitUnicodeString(&SmpSystemRoot, SharedUserData->NtSystemRoot);

        /* Allocate an empty string for the path */
        Length = SmpDefaultLibPath.MaximumLength + SmpSystemRoot.MaximumLength +
                 sizeof(L"\\system32;");
        RtlInitEmptyUnicodeString(&FullPathString,
                                  RtlAllocateHeap(SmpHeap, SmBaseTag, Length),
                                  (USHORT)Length);
        if (FullPathString.Buffer)
        {
            /* Append the root, system32;, and then the current library path */
            RtlAppendUnicodeStringToString(&FullPathString, &SmpSystemRoot);
            RtlAppendUnicodeToString(&FullPathString, L"\\system32;");
            RtlAppendUnicodeStringToString(&FullPathString, &SmpDefaultLibPath);
            RtlFreeHeap(SmpHeap, 0, SmpDefaultLibPath.Buffer);
            SmpDefaultLibPath = FullPathString;
        }
    }

    /* Consume the command line */
    CmdLineCopy = *CommandLine;
    while (TRUE)
    {
        /* Parse the first token and check for modifiers/specifiers */
        Status = SmpParseToken(&CmdLineCopy, FALSE, &Token);
        if (!(NT_SUCCESS(Status)) || !(Token.Buffer)) return STATUS_UNSUCCESSFUL;
        if (!Flags) break;

        /* Debug requested? */
        if (RtlEqualUnicodeString(&Token, &SmpDebugKeyword, TRUE))
        {
            /* Convert into a flag */
            *Flags |= SMP_DEBUG_FLAG;
        }
        else if (RtlEqualUnicodeString(&Token, &SmpASyncKeyword, TRUE))
        {
            /* Asynch requested, convert into a flag */
            *Flags |= SMP_ASYNC_FLAG;
        }
        else if (RtlEqualUnicodeString(&Token, &SmpAutoChkKeyword, TRUE))
        {
            /* Autochk requested, convert into a flag */
            *Flags |= SMP_AUTOCHK_FLAG;
        }
        else
        {
            /* No specifier found, keep going */
            break;
        }

        /* Get rid of this token and get the next */
        RtlFreeHeap(SmpHeap, 0, Token.Buffer);
    }

    /* Initialize a string to hold the current path */
    RtlInitUnicodeString(&EnvString, L"Path");
    Length = PAGE_SIZE;
    RtlInitEmptyUnicodeString(&PathString,
                              RtlAllocateHeap(SmpHeap, SmBaseTag, Length),
                              (USHORT)Length);
    if (!PathString.Buffer)
    {
        /* Fail if we have no memory for this */
        RtlFreeHeap(SmpHeap, 0, Token.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query the path from the environment */
    Status = RtlQueryEnvironmentVariable_U(SmpDefaultEnvironment,
                                           &EnvString,
                                           &PathString);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Our buffer was too small, free it */
        RtlFreeHeap(SmpHeap, 0, PathString.Buffer);

        /* And allocate one big enough */
        Length = PathString.Length + sizeof(UNICODE_NULL);
        RtlInitEmptyUnicodeString(&PathString,
                                  RtlAllocateHeap(SmpHeap, SmBaseTag, Length),
                                  (USHORT)Length);
        if (!PathString.Buffer)
        {
            /* Fail if we have no memory for this */
            RtlFreeHeap(SmpHeap, 0, Token.Buffer);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Now try again, this should work */
        Status = RtlQueryEnvironmentVariable_U(SmpDefaultEnvironment,
                                               &EnvString,
                                               &PathString);
    }
    if (!NT_SUCCESS(Status))
    {
        /* Another failure means that the kernel hasn't passed the path correctly */
        DPRINT1("SMSS: %wZ environment variable not defined.\n", &EnvString);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        /* Check if the caller expects any flags out of here */
        if (Flags)
        {
            /* We can return the image not found flag -- so does the image exist */
            if (!(RtlDosSearchPath_U(PathString.Buffer,
                                     Token.Buffer,
                                     L".exe",
                                     sizeof(PathBuffer),
                                     PathBuffer,
                                     &FilePart)) &&
                !(RtlDosSearchPath_U(SmpDefaultLibPath.Buffer,
                                     Token.Buffer,
                                     L".exe",
                                     sizeof(PathBuffer),
                                     PathBuffer,
                                     &FilePart)))
            {
                /* It doesn't, let the caller know about it and exit */
                *Flags |= SMP_INVALID_PATH;
                *FileName = Token;
                RtlFreeHeap(SmpHeap, 0, PathString.Buffer);
                return STATUS_SUCCESS;
            }
        }
        else
        {
            /* Caller doesn't want flags, probably wants the image itself */
            wcscpy(PathBuffer, Token.Buffer);
        }
    }

    /* Free tokens and such, all that's left is to convert the image name */
    RtlFreeHeap(SmpHeap, 0, Token.Buffer);
    RtlFreeHeap(SmpHeap, 0, PathString.Buffer);
    if (!NT_SUCCESS(Status)) return Status;

    /* Convert it and bail out if this failed */
    if (!RtlDosPathNameToNtPathName_U(PathBuffer, FileName, NULL, NULL))
    {
        DPRINT1("SMSS: Unable to translate %ws into an NT File Name\n",
                &PathBuffer);
        Status = STATUS_OBJECT_PATH_INVALID;
    }
    if (!NT_SUCCESS(Status)) return Status;

    /* Finally, check if the caller also wanted the directory */
    if (Directory)
    {
        /* Is the file a relative name with no directory? */
        if (FilePart <= PathBuffer)
        {
            /* Clear it */
            RtlInitUnicodeString(Directory, NULL);
        }
        else
        {
            /* There is a directory, and a filename -- separate those two */
            *--FilePart = UNICODE_NULL;
            RtlCreateUnicodeString(Directory, PathBuffer);
        }
    }

    /* We are done -- move on to the second pass to get the arguments */
    return SmpParseToken(&CmdLineCopy, TRUE, Arguments);
}

BOOLEAN
NTAPI
SmpQueryRegistrySosOption(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)ValueBuffer;
    ULONG Length;

    /* Open the key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't open control key: 0x%x\n", Status);
        return FALSE;
    }

    /* Query the value */
    RtlInitUnicodeString(&ValueName, L"SystemStartOptions");
    Status = NtQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(ValueBuffer),
                             &Length);
    ASSERT(Length < VALUE_BUFFER_SIZE);
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't query value key: 0x%x\n", Status);
        return FALSE;
    }

    /* Check if it's set to SOS or sos */
    if (!(wcsstr((PWCHAR)PartialInfo->Data, L"SOS")) ||
         (wcsstr((PWCHAR)PartialInfo->Data, L"sos")))
    {
        /* It's not set, return FALSE */
        return FALSE;
    }

    /* It's set, return TRUE */
    return TRUE;
}

BOOLEAN
NTAPI
SmpSaveAndClearBootStatusData(OUT PBOOLEAN BootOkay,
                              OUT PBOOLEAN ShutdownOkay)
{
    NTSTATUS Status;
    BOOLEAN Value = TRUE;
    PVOID BootStatusDataHandle;

    /* Assume failure */
    *BootOkay = FALSE;
    *ShutdownOkay = FALSE;

    /* Lock the BSD and fail if we couldn't */
    Status = RtlLockBootStatusData(&BootStatusDataHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Read the old settings */
    RtlGetSetBootStatusData(BootStatusDataHandle,
                            TRUE,
                            RtlBsdItemBootGood,
                            BootOkay,
                            sizeof(BOOLEAN),
                            NULL);
    RtlGetSetBootStatusData(BootStatusDataHandle,
                            TRUE,
                            RtlBsdItemBootShutdown,
                            ShutdownOkay,
                            sizeof(BOOLEAN),
                            NULL);

    /* Set new ones indicating we got at least this far */
    RtlGetSetBootStatusData(BootStatusDataHandle,
                            FALSE,
                            RtlBsdItemBootGood,
                            &Value,
                            sizeof(Value),
                            NULL);
    RtlGetSetBootStatusData(BootStatusDataHandle,
                            FALSE,
                            RtlBsdItemBootShutdown,
                            &Value,
                            sizeof(Value),
                            NULL);

    /* Unlock the BSD and return */
    RtlUnlockBootStatusData(BootStatusDataHandle);
    return TRUE;
}

VOID
NTAPI
SmpRestoreBootStatusData(IN BOOLEAN BootOkay,
                         IN BOOLEAN ShutdownOkay)
{
    NTSTATUS Status;
    PVOID BootState;

    /* Lock the BSD */
    Status = RtlLockBootStatusData(&BootState);
    if (NT_SUCCESS(Status))
    {
        /* Write the bootokay and bootshutdown values */
        RtlGetSetBootStatusData(BootState,
                                FALSE,
                                RtlBsdItemBootGood,
                                &BootOkay,
                                sizeof(BootOkay),
                                NULL);
        RtlGetSetBootStatusData(BootState,
                                FALSE,
                                RtlBsdItemBootShutdown,
                                &ShutdownOkay,
                                sizeof(ShutdownOkay),
                                NULL);

        /* Unlock the BSD and return */
        RtlUnlockBootStatusData(BootState);
    }
}
