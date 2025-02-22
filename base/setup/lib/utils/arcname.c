/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ARC path to-and-from NT path resolver.
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */
/*
 * References:
 *
 * - ARC Specification v1.2: http://netbsd.org./docs/Hardware/Machines/ARC/riscspec.pdf
 * - "Setup and Startup", MSDN article: https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-2000-server/cc977184(v=technet.10)
 * - Answer for "How do I determine the ARC path for a particular drive letter in Windows?": https://serverfault.com/questions/5910/how-do-i-determine-the-arc-path-for-a-particular-drive-letter-in-windows/5929#5929
 * - ARC - LinuxMIPS: https://web.archive.org/web/20230922043211/https://www.linux-mips.org/wiki/ARC
 * - ARCLoad - LinuxMIPS: https://web.archive.org/web/20221002210224/https://www.linux-mips.org/wiki/ARCLoad
 * - Inside Windows 2000 Server: https://books.google.fr/books?id=kYT7gKnwUQ8C&pg=PA71&lpg=PA71&dq=nt+arc+path&source=bl&ots=K8I1F_KQ_u&sig=EJq5t-v2qQk-QB7gNSREFj7pTVo&hl=en&sa=X&redir_esc=y#v=onepage&q=nt%20arc%20path&f=false
 * - Inside Windows Server 2003: https://books.google.fr/books?id=zayrcM9ZYdAC&pg=PA61&lpg=PA61&dq=arc+path+to+nt+path&source=bl&ots=x2JSWfp2MA&sig=g9mufN6TCOrPejDov6Rjp0Jrldo&hl=en&sa=X&redir_esc=y#v=onepage&q=arc%20path%20to%20nt%20path&f=false
 *
 * Stuff to read: http://www.adminxp.com/windows2000/index.php?aid=46 and https://web.archive.org/web/20170923151458/http://www.trcb.com/Computers-and-Technology/Windows-XP/Windows-XP-ARC-Naming-Conventions-1432.htm
 * concerning which values of disk() or rdisk() are valid when either scsi() or multi() adapters are specified.
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "filesup.h"
#include "partlist.h"
#include "arcname.h"

#define NDEBUG
#include <debug.h>


/* TYPEDEFS *****************************************************************/

/* Supported adapter types */
typedef enum _ADAPTER_TYPE
{
    EisaAdapter,
    ScsiAdapter,
    MultiAdapter,
    NetAdapter,
    RamdiskAdapter,
    AdapterTypeMax
} ADAPTER_TYPE, *PADAPTER_TYPE;
const PCSTR AdapterTypes_A[] =
{
    "eisa",
    "scsi",
    "multi",
    "net",
    "ramdisk",
    NULL
};
const PCWSTR AdapterTypes_U[] =
{
    L"eisa",
    L"scsi",
    L"multi",
    L"net",
    L"ramdisk",
    NULL
};

/* Supported controller types */
typedef enum _CONTROLLER_TYPE
{
    DiskController,
    CdRomController,
    ControllerTypeMax
} CONTROLLER_TYPE, *PCONTROLLER_TYPE;
const PCSTR ControllerTypes_A[] =
{
    "disk",
    "cdrom",
    NULL
};
const PCWSTR ControllerTypes_U[] =
{
    L"disk",
    L"cdrom",
    NULL
};

/* Supported peripheral types */
typedef enum _PERIPHERAL_TYPE
{
//  VDiskPeripheral,
    RDiskPeripheral,
    FDiskPeripheral,
    CdRomPeripheral,
    PeripheralTypeMax
} PERIPHERAL_TYPE, *PPERIPHERAL_TYPE;
const PCSTR PeripheralTypes_A[] =
{
//  "vdisk", // Enable this when we'll support boot from virtual disks!
    "rdisk",
    "fdisk",
    "cdrom",
    NULL
};
const PCWSTR PeripheralTypes_U[] =
{
//  L"vdisk", // Enable this when we'll support boot from virtual disks!
    L"rdisk",
    L"fdisk",
    L"cdrom",
    NULL
};


/* FUNCTIONS ****************************************************************/

/* static */ PCSTR
ArcGetNextTokenA(
    IN  PCSTR ArcPath,
    OUT PANSI_STRING TokenSpecifier,
    OUT PULONG Key)
{
    NTSTATUS Status;
    PCSTR p = ArcPath;
    SIZE_T SpecifierLength;
    ULONG KeyValue;

    /*
     * We must have a valid "specifier(key)" string, where 'specifier'
     * cannot be the empty string, and is followed by '('.
     */
    p = strchr(p, '(');
    if (p == NULL)
        return NULL; /* No '(' found */
    if (p == ArcPath)
        return NULL; /* Path starts with '(' and is thus invalid */

    SpecifierLength = (p - ArcPath) * sizeof(CHAR);
    if (SpecifierLength > MAXUSHORT)
    {
        return NULL;
    }

    /*
     * The strtoul function skips any leading whitespace.
     *
     * Note that if the token is "specifier()" then strtoul won't perform
     * any conversion and return 0, therefore effectively making the token
     * equivalent to "specifier(0)", as it should be.
     */
    // KeyValue = atoi(p);
    KeyValue = strtoul(p, (PSTR*)&p, 10);

    /* Skip any trailing whitespace */
    while (isspace(*p)) ++p;

    /* The token must terminate with ')' */
    if (*p != ')')
        return NULL;
#if 0
    p = strchr(p, ')');
    if (p == NULL)
        return NULL;
#endif

    /* We should have succeeded, copy the token specifier in the buffer */
    Status = RtlStringCbCopyNA(TokenSpecifier->Buffer,
                               TokenSpecifier->MaximumLength,
                               ArcPath, SpecifierLength);
    if (!NT_SUCCESS(Status))
        return NULL;

    TokenSpecifier->Length = (USHORT)SpecifierLength;

    /* We succeeded, return the token key value */
    *Key = KeyValue;

    /* Next token starts just after */
    return ++p;
}

static PCWSTR
ArcGetNextTokenU(
    IN  PCWSTR ArcPath,
    OUT PUNICODE_STRING TokenSpecifier,
    OUT PULONG Key)
{
    NTSTATUS Status;
    PCWSTR p = ArcPath;
    SIZE_T SpecifierLength;
    ULONG KeyValue;

    /*
     * We must have a valid "specifier(key)" string, where 'specifier'
     * cannot be the empty string, and is followed by '('.
     */
    p = wcschr(p, L'(');
    if (p == NULL)
        return NULL; /* No '(' found */
    if (p == ArcPath)
        return NULL; /* Path starts with '(' and is thus invalid */

    SpecifierLength = (p - ArcPath) * sizeof(WCHAR);
    if (SpecifierLength > UNICODE_STRING_MAX_BYTES)
    {
        return NULL;
    }

    ++p;

    /*
     * The strtoul function skips any leading whitespace.
     *
     * Note that if the token is "specifier()" then strtoul won't perform
     * any conversion and return 0, therefore effectively making the token
     * equivalent to "specifier(0)", as it should be.
     */
    // KeyValue = _wtoi(p);
    KeyValue = wcstoul(p, (PWSTR*)&p, 10);

    /* Skip any trailing whitespace */
    while (iswspace(*p)) ++p;

    /* The token must terminate with ')' */
    if (*p != L')')
        return NULL;
#if 0
    p = wcschr(p, L')');
    if (p == NULL)
        return NULL;
#endif

    /* We should have succeeded, copy the token specifier in the buffer */
    Status = RtlStringCbCopyNW(TokenSpecifier->Buffer,
                               TokenSpecifier->MaximumLength,
                               ArcPath, SpecifierLength);
    if (!NT_SUCCESS(Status))
        return NULL;

    TokenSpecifier->Length = (USHORT)SpecifierLength;

    /* We succeeded, return the token key value */
    *Key = KeyValue;

    /* Next token starts just after */
    return ++p;
}


/* static */ ULONG
ArcMatchTokenA(
    IN PCSTR CandidateToken,
    IN const PCSTR* TokenTable)
{
    ULONG Index = 0;

    while (TokenTable[Index] && _stricmp(CandidateToken, TokenTable[Index]) != 0)
    {
        ++Index;
    }

    return Index;
}

/* static */ ULONG
ArcMatchTokenU(
    IN PCWSTR CandidateToken,
    IN const PCWSTR* TokenTable)
{
    ULONG Index = 0;

    while (TokenTable[Index] && _wcsicmp(CandidateToken, TokenTable[Index]) != 0)
    {
        ++Index;
    }

    return Index;
}

static ULONG
ArcMatchToken_UStr(
    IN PCUNICODE_STRING CandidateToken,
    IN const PCWSTR* TokenTable)
{
    ULONG Index = 0;
#if 0
    SIZE_T Length;
#else
    UNICODE_STRING Token;
#endif

    while (TokenTable[Index])
    {
#if 0
        Length = wcslen(TokenTable[Index]);
        if ((Length == CandidateToken->Length / sizeof(WCHAR)) &&
            (_wcsnicmp(CandidateToken->Buffer, TokenTable[Index], Length) == 0))
        {
            break;
        }
#else
        RtlInitUnicodeString(&Token, TokenTable[Index]);
        if (RtlEqualUnicodeString(CandidateToken, &Token, TRUE))
            break;
#endif

        ++Index;
    }

    return Index;
}


BOOLEAN
ArcPathNormalize(
    OUT PUNICODE_STRING NormalizedArcPath,
    IN  PCWSTR ArcPath)
{
    NTSTATUS Status;
    PCWSTR EndOfArcName;
    PCWSTR p;
    SIZE_T PathLength;

    if (NormalizedArcPath->MaximumLength < sizeof(UNICODE_NULL))
        return FALSE;

    *NormalizedArcPath->Buffer = UNICODE_NULL;
    NormalizedArcPath->Length = 0;

    EndOfArcName = wcschr(ArcPath, OBJ_NAME_PATH_SEPARATOR);
    if (!EndOfArcName)
        EndOfArcName = ArcPath + wcslen(ArcPath);

    while ((p = wcsstr(ArcPath, L"()")) && (p < EndOfArcName))
    {
#if 0
        Status = RtlStringCbCopyNW(NormalizedArcPath->Buffer,
                                   NormalizedArcPath->MaximumLength,
                                   ArcPath, (p - ArcPath) * sizeof(WCHAR));
#else
        Status = RtlStringCbCatNW(NormalizedArcPath->Buffer,
                                  NormalizedArcPath->MaximumLength,
                                  ArcPath, (p - ArcPath) * sizeof(WCHAR));
#endif
        if (!NT_SUCCESS(Status))
            return FALSE;

        Status = RtlStringCbCatW(NormalizedArcPath->Buffer,
                                 NormalizedArcPath->MaximumLength,
                                 L"(0)");
        if (!NT_SUCCESS(Status))
            return FALSE;
#if 0
        NormalizedArcPath->Buffer += wcslen(NormalizedArcPath->Buffer);
#endif
        ArcPath = p + 2;
    }

    Status = RtlStringCbCatW(NormalizedArcPath->Buffer,
                             NormalizedArcPath->MaximumLength,
                             ArcPath);
    if (!NT_SUCCESS(Status))
        return FALSE;

    PathLength = wcslen(NormalizedArcPath->Buffer);
    if (PathLength > UNICODE_STRING_MAX_CHARS)
    {
        return FALSE;
    }

    NormalizedArcPath->Length = (USHORT)PathLength * sizeof(WCHAR);
    return TRUE;
}


/*
 * ArcNamePath:
 *      In input, pointer to an ARC path (NULL-terminated) starting by an
 *      ARC name to be parsed into its different components.
 *      In output, ArcNamePath points to the beginning of the path after
 *      the ARC name part.
 */
static NTSTATUS
ParseArcName(
    IN OUT PCWSTR* ArcNamePath,
    OUT PULONG pAdapterKey,
    OUT PULONG pControllerKey,
    OUT PULONG pPeripheralKey,
    OUT PULONG pPartitionNumber,
    OUT PADAPTER_TYPE pAdapterType,
    OUT PCONTROLLER_TYPE pControllerType,
    OUT PPERIPHERAL_TYPE pPeripheralType,
    OUT PBOOLEAN pUseSignature)
{
    // NTSTATUS Status;
    WCHAR TokenBuffer[50];
    UNICODE_STRING Token;
    PCWSTR p, q;
    ULONG AdapterKey = 0;
    ULONG ControllerKey = 0;
    ULONG PeripheralKey = 0;
    ULONG PartitionNumber = 0;
    ADAPTER_TYPE AdapterType = AdapterTypeMax;
    CONTROLLER_TYPE ControllerType = ControllerTypeMax;
    PERIPHERAL_TYPE PeripheralType = PeripheralTypeMax;
    BOOLEAN UseSignature = FALSE;

    /*
     * The format of ArcName is:
     *    adapter(www)[controller(xxx)peripheral(yyy)[partition(zzz)][filepath]] ,
     * where the [filepath] part is not being parsed.
     */

    RtlInitEmptyUnicodeString(&Token, TokenBuffer, sizeof(TokenBuffer));

    p = *ArcNamePath;

    /* Retrieve the adapter */
    p = ArcGetNextTokenU(p, &Token, &AdapterKey);
    if (!p)
    {
        DPRINT1("No adapter specified!\n");
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    /* Check for the 'signature()' pseudo-adapter, introduced in Windows 2000 */
    if (_wcsicmp(Token.Buffer, L"signature") == 0)
    {
        /*
         * We've got a signature! Remember this for later, and set the adapter type to SCSI.
         * We however check that the rest of the ARC path is valid by parsing the other tokens.
         * AdapterKey stores the disk signature value (that holds in a ULONG).
         */
        UseSignature = TRUE;
        AdapterType = ScsiAdapter;
    }
    else
    {
        /* Check for regular adapters */
        // ArcMatchTokenU(Token.Buffer, AdapterTypes_U);
        AdapterType = (ADAPTER_TYPE)ArcMatchToken_UStr(&Token, AdapterTypes_U);
        if (AdapterType >= AdapterTypeMax)
        {
            DPRINT1("Invalid adapter type %wZ\n", &Token);
            return STATUS_OBJECT_NAME_INVALID;
        }

        /* Check for adapters that don't take any extra controller or peripheral nodes */
        if (AdapterType == NetAdapter || AdapterType == RamdiskAdapter)
        {
            // if (*p)
            //     return STATUS_OBJECT_PATH_SYNTAX_BAD;

            if (AdapterType == NetAdapter)
            {
                DPRINT1("%S(%lu) path is not supported!\n", AdapterTypes_U[AdapterType], AdapterKey);
                return STATUS_NOT_SUPPORTED;
            }

            goto Quit;
        }
    }

    /* Here, we have either an 'eisa', a 'scsi/signature', or a 'multi' adapter */

    /* Check for a valid controller */
    p = ArcGetNextTokenU(p, &Token, &ControllerKey);
    if (!p)
    {
        DPRINT1("%S(%lu) adapter doesn't have a controller!\n", AdapterTypes_U[AdapterType], AdapterKey);
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }
    // ArcMatchTokenU(Token.Buffer, ControllerTypes_U);
    ControllerType = (CONTROLLER_TYPE)ArcMatchToken_UStr(&Token, ControllerTypes_U);
    if (ControllerType >= ControllerTypeMax)
    {
        DPRINT1("Invalid controller type %wZ\n", &Token);
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* Here the controller can only be either a disk or a CDROM */

    /*
     * Ignore the controller in case we have a 'multi' adapter.
     * I guess a similar condition holds for the 'eisa' adapter too...
     *
     * For SignatureAdapter, as similar for ScsiAdapter, the controller key corresponds
     * to the disk target ID. Note that actually, the implementation just ignores the
     * target ID, as well as the LUN, and just loops over all the available disks and
     * searches for the one having the correct signature.
     */
    if ((AdapterType == MultiAdapter /* || AdapterType == EisaAdapter */) && ControllerKey != 0)
    {
        DPRINT1("%S(%lu) adapter with %S(%lu non-zero), ignored!\n",
               AdapterTypes_U[AdapterType], AdapterKey,
               ControllerTypes_U[ControllerType], ControllerKey);
        ControllerKey = 0;
    }

    /*
     * Only the 'scsi' adapter supports a direct 'cdrom' controller.
     * For the others, we need a 'disk' controller to which a 'cdrom' peripheral can talk to.
     */
    if ((AdapterType != ScsiAdapter) && (ControllerType == CdRomController))
    {
        DPRINT1("%S(%lu) adapter cannot have a CDROM controller!\n", AdapterTypes_U[AdapterType], AdapterKey);
        return STATUS_OBJECT_PATH_INVALID;
    }

    /* Check for a valid peripheral */
    p = ArcGetNextTokenU(p, &Token, &PeripheralKey);
    if (!p)
    {
        DPRINT1("%S(%lu)%S(%lu) adapter-controller doesn't have a peripheral!\n",
               AdapterTypes_U[AdapterType], AdapterKey,
               ControllerTypes_U[ControllerType], ControllerKey);
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }
    // ArcMatchTokenU(Token.Buffer, PeripheralTypes_U);
    PeripheralType = (PERIPHERAL_TYPE)ArcMatchToken_UStr(&Token, PeripheralTypes_U);
    if (PeripheralType >= PeripheralTypeMax)
    {
        DPRINT1("Invalid peripheral type %wZ\n", &Token);
        return STATUS_OBJECT_NAME_INVALID;
    }

    /*
     * If we had a 'cdrom' controller already, the corresponding peripheral can only be 'fdisk'
     * (see for example the ARC syntax for SCSI CD-ROMs: scsi(x)cdrom(y)fdisk(z) where z == 0).
     */
    if ((ControllerType == CdRomController) && (PeripheralType != FDiskPeripheral))
    {
        DPRINT1("%S(%lu) controller cannot have a %S(%lu) peripheral! (note that we haven't check whether the adapter was SCSI or not)\n",
               ControllerTypes_U[ControllerType], ControllerKey,
               PeripheralTypes_U[PeripheralType], PeripheralKey);
        return STATUS_OBJECT_PATH_INVALID;
    }

    /* For a 'scsi' adapter, the possible peripherals are only 'rdisk' or 'fdisk' */
    if (AdapterType == ScsiAdapter && !(PeripheralType == RDiskPeripheral || PeripheralType == FDiskPeripheral))
    {
        DPRINT1("%S(%lu)%S(%lu) SCSI adapter-controller has an invalid peripheral %S(%lu) !\n",
               AdapterTypes_U[AdapterType], AdapterKey,
               ControllerTypes_U[ControllerType], ControllerKey,
               PeripheralTypes_U[PeripheralType], PeripheralKey);
        return STATUS_OBJECT_PATH_INVALID;
    }

#if 0
    if (AdapterType == SignatureAdapter && PeripheralKey != 0)
    {
        DPRINT1("%S(%lu) adapter with %S(%lu non-zero), ignored!\n",
               AdapterTypes_U[AdapterType], AdapterKey,
               PeripheralTypes_U[PeripheralType], PeripheralKey);
        PeripheralKey = 0;
    }
#endif

    /* Check for the optional 'partition' specifier */
    q = ArcGetNextTokenU(p, &Token, &PartitionNumber);
    if (q && _wcsicmp(Token.Buffer, L"partition") == 0)
    {
        /* We've got a partition! */
        p = q;
    }
    else
    {
        /*
         * Either no other ARC token was found, or we've got something else
         * (possibly invalid or not)...
         */
        PartitionNumber = 0;
    }

    // TODO: Check the partition number in case of fdisks and cdroms??

Quit:
    /* Return the results */
    *ArcNamePath      = p;
    *pAdapterKey      = AdapterKey;
    *pControllerKey   = ControllerKey;
    *pPeripheralKey   = PeripheralKey;
    *pPartitionNumber = PartitionNumber;
    *pAdapterType     = AdapterType;
    *pControllerType  = ControllerType;
    *pPeripheralType  = PeripheralType;
    *pUseSignature    = UseSignature;

    return STATUS_SUCCESS;
}

/*
 * ArcName:
 *      ARC name (counted string) to be resolved into a NT device name.
 *      The caller should have already delimited it from within an ARC path
 *      (usually by finding where the first path separator appears in the path).
 *
 * NtName:
 *      Receives the resolved NT name. The buffer is NULL-terminated.
 */
static NTSTATUS
ResolveArcNameNtSymLink(
    OUT PUNICODE_STRING NtName,
    IN  PUNICODE_STRING ArcName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle, LinkHandle;
    UNICODE_STRING ArcNameDir;

    if (NtName->MaximumLength < sizeof(UNICODE_NULL))
        return STATUS_BUFFER_TOO_SMALL;

    /* Open the \ArcName object directory */
    RtlInitUnicodeString(&ArcNameDir, L"\\ArcName");
    InitializeObjectAttributes(&ObjectAttributes,
                               &ArcNameDir,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_ALL_ACCESS,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenDirectoryObject(%wZ) failed, Status 0x%08lx\n", &ArcNameDir, Status);
        return Status;
    }

    /* Open the ARC name link */
    InitializeObjectAttributes(&ObjectAttributes,
                               ArcName,
                               OBJ_CASE_INSENSITIVE,
                               DirectoryHandle,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);

    /* Close the \ArcName object directory handle */
    NtClose(DirectoryHandle);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenSymbolicLinkObject(%wZ) failed, Status 0x%08lx\n", ArcName, Status);
        return Status;
    }

    /* Reserve one WCHAR for the NULL-termination */
    NtName->MaximumLength -= sizeof(UNICODE_NULL);

    /* Resolve the link and close its handle */
    Status = NtQuerySymbolicLinkObject(LinkHandle, NtName, NULL);
    NtClose(LinkHandle);

    /* Restore the NULL-termination */
    NtName->MaximumLength += sizeof(UNICODE_NULL);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        /* We failed, don't touch NtName */
        DPRINT1("NtQuerySymbolicLinkObject(%wZ) failed, Status 0x%08lx\n", ArcName, Status);
    }
    else
    {
        /* We succeeded, NULL-terminate NtName */
        NtName->Buffer[NtName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    return Status;
}

/*
 * ArcNamePath:
 *      In input, pointer to an ARC path (NULL-terminated) starting by an
 *      ARC name to be resolved into a NT device name.
 *      In opposition to ResolveArcNameNtSymLink(), the caller does not have
 *      to delimit the ARC name from within an ARC path. The real ARC name is
 *      deduced after parsing the ARC path, and, in output, ArcNamePath points
 *      to the beginning of the path after the ARC name part.
 *
 * NtName:
 *      Receives the resolved NT name. The buffer is NULL-terminated.
 *
 * PartList:
 *      (Optional) partition list that helps in resolving the paths pointing
 *      to hard disks.
 */
static NTSTATUS
ResolveArcNameManually(
    OUT PUNICODE_STRING NtName,
    IN OUT PCWSTR* ArcNamePath,
    IN  PPARTLIST PartList)
{
    NTSTATUS Status;
    ULONG AdapterKey;
    ULONG ControllerKey;
    ULONG PeripheralKey;
    ULONG PartitionNumber;
    ADAPTER_TYPE AdapterType;
    CONTROLLER_TYPE ControllerType;
    PERIPHERAL_TYPE PeripheralType;
    BOOLEAN UseSignature;
    SIZE_T NameLength;

    if (NtName->MaximumLength < sizeof(UNICODE_NULL))
        return STATUS_BUFFER_TOO_SMALL;

    /* Parse the ARC path */
    Status = ParseArcName(ArcNamePath,
                          &AdapterKey,
                          &ControllerKey,
                          &PeripheralKey,
                          &PartitionNumber,
                          &AdapterType,
                          &ControllerType,
                          &PeripheralType,
                          &UseSignature);
    if (!NT_SUCCESS(Status))
        return Status;

    // TODO: Check the partition number in case of fdisks and cdroms??

    /* Check for adapters that don't take any extra controller or peripheral node */
    if (AdapterType == NetAdapter || AdapterType == RamdiskAdapter)
    {
        if (AdapterType == NetAdapter)
        {
            DPRINT1("%S(%lu) path is not supported!\n", AdapterTypes_U[AdapterType], AdapterKey);
            return STATUS_NOT_SUPPORTED;
        }

        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\Ramdisk%lu", AdapterKey);
    }
    else
    if (ControllerType == CdRomController) // and so, AdapterType == ScsiAdapter and PeripheralType == FDiskPeripheral
    {
        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\Scsi\\CdRom%lu", ControllerKey);
    }
    else
    /* Now, ControllerType == DiskController */
    if (PeripheralType == CdRomPeripheral)
    {
        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\CdRom%lu", PeripheralKey);
    }
    else
    if (PeripheralType == FDiskPeripheral)
    {
        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\Floppy%lu", PeripheralKey);
    }
    else
    if (PeripheralType == RDiskPeripheral)
    {
        PDISKENTRY DiskEntry;
        PPARTENTRY PartEntry = NULL;

        if (UseSignature)
        {
            /* The disk signature is stored in AdapterKey */
            DiskEntry = GetDiskBySignature(PartList, AdapterKey);
        }
        else
        {
            DiskEntry = GetDiskBySCSI(PartList, AdapterKey,
                                      ControllerKey, PeripheralKey);
        }
        if (!DiskEntry)
            return STATUS_OBJECT_PATH_NOT_FOUND; // STATUS_NOT_FOUND;

        if (PartitionNumber != 0)
        {
            PartEntry = GetPartition(DiskEntry, PartitionNumber);
            if (!PartEntry)
                return STATUS_OBJECT_PATH_NOT_FOUND; // STATUS_DEVICE_NOT_PARTITIONED;
            ASSERT(PartEntry->DiskEntry == DiskEntry);
        }

        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\Harddisk%lu\\Partition%lu",
                                    DiskEntry->DiskNumber, PartitionNumber);
    }
#if 0 // FIXME: Not implemented yet!
    else
    if (PeripheralType == VDiskPeripheral)
    {
        // TODO: Check how Win 7+ deals with virtual disks.
        Status = RtlStringCbPrintfW(NtName->Buffer, NtName->MaximumLength,
                                    L"\\Device\\VirtualHarddisk%lu\\Partition%lu",
                                    PeripheralKey, PartitionNumber);
    }
#endif

    if (!NT_SUCCESS(Status))
    {
        /* Returned NtName is invalid, so zero it out */
        *NtName->Buffer = UNICODE_NULL;
        NtName->Length = 0;

        return Status;
    }

    /* Update NtName length */
    NameLength = wcslen(NtName->Buffer);
    if (NameLength > UNICODE_STRING_MAX_CHARS)
    {
        return STATUS_NAME_TOO_LONG;
    }

    NtName->Length = (USHORT)NameLength * sizeof(WCHAR);

    return STATUS_SUCCESS;
}


BOOLEAN
ArcPathToNtPath(
    OUT PUNICODE_STRING NtPath,
    IN  PCWSTR ArcPath,
    IN  PPARTLIST PartList OPTIONAL)
{
    NTSTATUS Status;
    PCWSTR BeginOfPath;
    UNICODE_STRING ArcName;
    SIZE_T PathLength;

    /* TODO: We should "normalize" the path, i.e. expand all the xxx() into xxx(0) */

    if (NtPath->MaximumLength < sizeof(UNICODE_NULL))
        return FALSE;

    *NtPath->Buffer = UNICODE_NULL;
    NtPath->Length = 0;

    /*
     * - First, check whether the ARC path is already inside \\ArcName
     *   and if so, map it to the corresponding NT path.
     * - Only then, if we haven't found any ArcName, try to build a
     *   NT path by deconstructing the ARC path, using its disk and
     *   partition numbers. We may use here our disk/partition list.
     *
     * See also freeldr/arcname.c
     *
     * Note that it would be nice to maintain a cache of these mappings.
     */

    /*
     * Initialize the ARC name to resolve, by cutting the ARC path at the first
     * NT path separator. The ARC name therefore ends where the NT path part starts.
     */
    RtlInitUnicodeString(&ArcName, ArcPath);
    BeginOfPath = wcschr(ArcName.Buffer, OBJ_NAME_PATH_SEPARATOR);
    if (BeginOfPath)
        ArcName.Length = (ULONG_PTR)BeginOfPath - (ULONG_PTR)ArcName.Buffer;

    /* Resolve the ARC name via NT SymLinks. Note that NtPath is returned NULL-terminated. */
    Status = ResolveArcNameNtSymLink(NtPath, &ArcName);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, attempt a manual resolution */
        DPRINT1("ResolveArcNameNtSymLink(ArcName = '%wZ') for ArcPath = '%S' failed, Status 0x%08lx\n", &ArcName, ArcPath, Status);

        /*
         * We failed at directly resolving the ARC path, and we cannot perform
         * a manual resolution because we don't have any disk/partition list,
         * we therefore fail here.
         */
        if (!PartList)
        {
            DPRINT1("PartList == NULL, cannot perform a manual resolution\n");
            return FALSE;
        }

        *NtPath->Buffer = UNICODE_NULL;
        NtPath->Length = 0;

        BeginOfPath = ArcPath;
        Status = ResolveArcNameManually(NtPath, &BeginOfPath, PartList);
        if (!NT_SUCCESS(Status))
        {
            /* We really failed this time, bail out */
            DPRINT1("ResolveArcNameManually(ArcPath = '%S') failed, Status 0x%08lx\n", ArcPath, Status);
            return FALSE;
        }
    }

    /*
     * We succeeded. Concatenate the rest of the system-specific path. We know the path is going
     * to be inside the NT namespace, therefore we can use the path string concatenation function
     * that uses '\\' as the path separator.
     */
    if (BeginOfPath && *BeginOfPath)
    {
        Status = ConcatPaths(NtPath->Buffer, NtPath->MaximumLength / sizeof(WCHAR), 1, BeginOfPath);
        if (!NT_SUCCESS(Status))
        {
            /* Buffer not large enough, or whatever...: just bail out */
            return FALSE;
        }
    }

    PathLength = wcslen(NtPath->Buffer);
    if (PathLength > UNICODE_STRING_MAX_CHARS)
    {
        return FALSE;
    }

    NtPath->Length = (USHORT)PathLength * sizeof(WCHAR);

    return TRUE;
}

#if 0 // FIXME: Not implemented yet!
PWSTR
NtPathToArcPath(
    IN PWSTR NtPath)
{
    /*
     * - First, check whether any of the ARC paths inside \\ArcName
     *   map to the corresponding NT path. If so, we are OK.
     * - Only then, if we haven't found any ArcName, try to build an
     *   ARC path by deconstructing the NT path, using its disk and
     *   partition numbers. We may use here our disk/partition list.
     *
     * See also freeldr/arcname.c
     *
     * Note that it would be nice to maintain a cache of these mappings.
     */
}
#endif

/* EOF */
