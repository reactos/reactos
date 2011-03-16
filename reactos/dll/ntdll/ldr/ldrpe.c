/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/ldrpe.c
 * PURPOSE:         Loader Functions dealing low-level PE Format structures
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
ULONG LdrpFatalHardErrorCount;
PVOID LdrpManifestProberRoutine;

/* PROTOTYPES ****************************************************************/

#define IMAGE_REL_BASED_HIGH3ADJ 11

NTSTATUS
NTAPI
LdrpLoadImportModule(IN PWSTR DllPath OPTIONAL,
                     IN LPSTR ImportName,
                     IN PVOID DllBase,
                     OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
                     OUT PBOOLEAN Existing);

/* FUNCTIONS *****************************************************************/

USHORT NTAPI
LdrpNameToOrdinal(LPSTR ImportName,
                  ULONG NumberOfNames,
                  PVOID ExportBase,
                  PULONG NameTable,
                  PUSHORT OrdinalTable)
{
    UNIMPLEMENTED;
    return 0;
}


NTSTATUS
NTAPI
LdrpSnapThunk(IN PVOID ExportBase,
              IN PVOID ImportBase,
              IN PIMAGE_THUNK_DATA OriginalThunk,
              IN OUT PIMAGE_THUNK_DATA Thunk,
              IN PIMAGE_EXPORT_DIRECTORY ExportEntry,
              IN ULONG ExportSize,
              IN BOOLEAN Static,
              IN LPSTR DllName)
{
    BOOLEAN IsOrdinal;
    USHORT Ordinal;
    ULONG OriginalOrdinal = 0;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    LPSTR ImportName;
    USHORT Hint;
    NTSTATUS Status;
    ULONG_PTR HardErrorParameters[3];
    UNICODE_STRING HardErrorDllName, HardErrorEntryPointName;
    ANSI_STRING TempString;
    ULONG Mask;
    ULONG Response;
    PULONG AddressOfFunctions;
    UNICODE_STRING TempUString;
    ANSI_STRING ForwarderName;
    PANSI_STRING ForwardName;
    PVOID ForwarderHandle;
    ULONG ForwardOrdinal;

    /* Check if the snap is by ordinal */
    if ((IsOrdinal = IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal)))
    {
        /* Get the ordinal number, and its normalized version */
        OriginalOrdinal = IMAGE_ORDINAL(OriginalThunk->u1.Ordinal);
        Ordinal = (USHORT)(OriginalOrdinal - ExportEntry->Base);
    }
    else
    {
        /* First get the data VA */
        AddressOfData = (PIMAGE_IMPORT_BY_NAME)
                        ((ULONG_PTR)ImportBase +
                        ((ULONG_PTR)OriginalThunk->u1.AddressOfData & 0xffffffff));

        /* Get the name */
        ImportName = (LPSTR)AddressOfData->Name;

        /* Now get the VA of the Name and Ordinal Tables */
        NameTable = (PULONG)((ULONG_PTR)ExportBase +
                             (ULONG_PTR)ExportEntry->AddressOfNames);
        OrdinalTable = (PUSHORT)((ULONG_PTR)ExportBase +
                                 (ULONG_PTR)ExportEntry->AddressOfNameOrdinals);

        /* Get the hint */
        Hint = AddressOfData->Hint;

        /* Try to get a match by using the hint */
        if (((ULONG)Hint < ExportEntry->NumberOfNames) &&
             (!strcmp(ImportName, ((LPSTR)((ULONG_PTR)ExportBase + NameTable[Hint])))))
        {
            /* We got a match, get the Ordinal from the hint */
            Ordinal = OrdinalTable[Hint];
        }
        else
        {
            /* Well bummer, hint didn't work, do it the long way */
            Ordinal = LdrpNameToOrdinal(ImportName,
                                        ExportEntry->NumberOfNames,
                                        ExportBase,
                                        NameTable,
                                        OrdinalTable);
        }
    }

    /* Check if the ordinal is invalid */
    if ((ULONG)Ordinal >= ExportEntry->NumberOfFunctions)
    {
FailurePath:
        /* Is this a static snap? */
        if (Static)
        {
            /* These are critical errors. Setup a string for the DLL name */
            RtlInitAnsiString(&TempString, DllName ? DllName : "Unknown");
            RtlAnsiStringToUnicodeString(&HardErrorDllName, &TempString, TRUE);

            /* Set it as the parameter */
            HardErrorParameters[1] = (ULONG_PTR)&HardErrorDllName;
            Mask = 2;

            /* Check if we have an ordinal */
            if (IsOrdinal)
            {
                /* Then set the ordinal as the 1st parameter */
                HardErrorParameters[0] = OriginalOrdinal;
            }
            else
            {
                /* We don't, use the entrypoint. Set up a string for it */
                RtlInitAnsiString(&TempString, ImportName);
                RtlAnsiStringToUnicodeString(&HardErrorEntryPointName,
                                             &TempString,
                                             TRUE);

                /* Set it as the parameter */
                HardErrorParameters[0] = (ULONG_PTR)&HardErrorEntryPointName;
                Mask = 3;
            }

            /* Raise the error */
            NtRaiseHardError(IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                                         STATUS_ENTRYPOINT_NOT_FOUND,
                             2,
                             Mask,
                             HardErrorParameters,
                             OptionOk,
                             &Response);

            /* Increase the error count */
            if (LdrpInLdrInit) LdrpFatalHardErrorCount++;

            /* Free our string */
            RtlFreeUnicodeString(&HardErrorDllName);
            if (!IsOrdinal)
            {
                /* Free our second string. Return entrypoint error */
                RtlFreeUnicodeString(&HardErrorEntryPointName);
                RtlRaiseStatus(STATUS_ENTRYPOINT_NOT_FOUND);
            }

            /* Return ordinal error */
            RtlRaiseStatus(STATUS_ORDINAL_NOT_FOUND);
        }

        /* Set this as a bad DLL */
        Thunk->u1.Function = (ULONG_PTR)0xffbadd11;

        /* Return the right error code */
        Status = IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                             STATUS_ENTRYPOINT_NOT_FOUND;
    }
    else
    {
        /* The ordinal seems correct, get the AddressOfFunctions VA */
        AddressOfFunctions = (PULONG)
                             ((ULONG_PTR)ExportBase +
                              (ULONG_PTR)ExportEntry->AddressOfFunctions);

        /* Write the function pointer*/
        Thunk->u1.Function = (ULONG_PTR)ExportBase + AddressOfFunctions[Ordinal];

        /* Make sure it's within the exports */
        if ((Thunk->u1.Function > (ULONG_PTR)ExportEntry) &&
            (Thunk->u1.Function < ((ULONG_PTR)ExportEntry + ExportSize)))
        {
            /* Get the Import and Forwarder Names */
            ImportName = (LPSTR)Thunk->u1.Function;
            ForwarderName.Buffer = ImportName;
            ForwarderName.Length = (USHORT)(strchr(ImportName, '.') - ImportName);
            ForwarderName.MaximumLength = ForwarderName.Length;
            Status = RtlAnsiStringToUnicodeString(&TempUString,
                                                  &ForwarderName,
                                                  TRUE);

            /* Make sure the conversion was OK */
            if (NT_SUCCESS(Status))
            {
                /* Load the forwarder, free the temp string */
                Status = LdrpLoadDll(FALSE,
                                     NULL,
                                     NULL,
                                     &TempUString,
                                     &ForwarderHandle,
                                     FALSE);
                RtlFreeUnicodeString(&TempUString);
            }

            /* If the load or conversion failed, use the failure path */
            if (!NT_SUCCESS(Status)) goto FailurePath;

            /* Now set up a name for the actual forwarder dll */
            RtlInitAnsiString(&ForwarderName,
                              ImportName + ForwarderName.Length + sizeof(CHAR));

            /* Check if it's an ordinal forward */
            if ((ForwarderName.Length > 1) && (*ForwarderName.Buffer == '#'))
            {
                /* We don't have an actual function name */
                ForwardName = NULL;

                /* Convert the string into an ordinal */
                Status = RtlCharToInteger(ForwarderName.Buffer + sizeof(CHAR),
                                          0,
                                          &ForwardOrdinal);

                /* If this fails, then error out */
                if (!NT_SUCCESS(Status)) goto FailurePath;
            }
            else
            {
                /* Import by name */
                ForwardName = &ForwarderName;
            }

            /* Get the pointer */
            Status = LdrpGetProcedureAddress(ForwarderHandle,
                                             ForwardName,
                                             ForwardOrdinal,
                                             (PVOID*)&Thunk->u1.Function,
                                             FALSE);
            /* If this fails, then error out */
            if (!NT_SUCCESS(Status)) goto FailurePath;
        }
        else
        {
            /* It's not within the exports, let's hope it's valid */
            if (!AddressOfFunctions[Ordinal]) goto FailurePath;
        }

        /* If we got here, then it's success */
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

/* EOF */
