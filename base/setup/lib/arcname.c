/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/arcname.c
 * PURPOSE:         ARC path to-and-from NT path resolver.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */
/*
 * References:
 *
 * - ARC Specification v1.2: http://netbsd.org./docs/Hardware/Machines/ARC/riscspec.pdf
 * - "Setup and Startup", MSDN article: https://technet.microsoft.com/en-us/library/cc977184.aspx
 * - Answer for "How do I determine the ARC path for a particular drive letter in Windows?": https://serverfault.com/a/5929
 * - ARC - LinuxMIPS: https://www.linux-mips.org/wiki/ARC
 * - ARCLoad - LinuxMIPS: https://www.linux-mips.org/wiki/ARCLoad
 * - Inside Windows 2000 Server: https://books.google.fr/books?id=kYT7gKnwUQ8C&pg=PA71&lpg=PA71&dq=nt+arc+path&source=bl&ots=K8I1F_KQ_u&sig=EJq5t-v2qQk-QB7gNSREFj7pTVo&hl=en&sa=X&redir_esc=y#v=onepage&q=nt%20arc%20path&f=false
 * - Inside Windows Server 2003: https://books.google.fr/books?id=zayrcM9ZYdAC&pg=PA61&lpg=PA61&dq=arc+path+to+nt+path&source=bl&ots=x2JSWfp2MA&sig=g9mufN6TCOrPejDov6Rjp0Jrldo&hl=en&sa=X&redir_esc=y#v=onepage&q=arc%20path%20to%20nt%20path&f=false
 *
 * Stuff to read: http://www.adminxp.com/windows2000/index.php?aid=46 and http://www.trcb.com/Computers-and-Technology/Windows-XP/Windows-XP-ARC-Naming-Conventions-1432.htm
 * concerning which values of disk() or rdisk() are valid when either scsi() or multi() adapters are specified.
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "partlist.h"
#include "arcname.h"

#define NDEBUG
#include <debug.h>


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

PCSTR
ArcGetNextTokenA(
    IN  PCSTR ArcPath,
    OUT PANSI_STRING TokenSpecifier,
    OUT PULONG Key)
{
    HRESULT hr;
    PCSTR p = ArcPath;
    ULONG SpecifierLength;
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
    while (*p && isspace(*p)) ++p;

    /* The token must terminate with ')' */
    if (*p != ')')
        return NULL;
#if 0
    p = strchr(p, ')');
    if (p == NULL)
        return NULL;
#endif

    /* We should have succeeded, copy the token specifier in the buffer */
    hr = StringCbCopyNA(TokenSpecifier->Buffer, TokenSpecifier->MaximumLength,
                        ArcPath, SpecifierLength);
    if (FAILED(hr))
        return NULL;

    TokenSpecifier->Length = strlen(TokenSpecifier->Buffer) * sizeof(CHAR);

    /* We succeeded, return the token key value */
    *Key = KeyValue;

    /* Next token starts just after */
    return ++p;
}

PCWSTR
ArcGetNextTokenU(
    IN  PCWSTR ArcPath,
    OUT PUNICODE_STRING TokenSpecifier,
    OUT PULONG Key)
{
    HRESULT hr;
    PCWSTR p = ArcPath;
    ULONG SpecifierLength;
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
    ASSERT(p);

    /* Skip any trailing whitespace */
    while (*p && iswspace(*p)) ++p;

    /* The token must terminate with ')' */
    if (*p != L')')
        return NULL;
#if 0
    p = wcschr(p, L')');
    if (p == NULL)
        return NULL;
#endif

    /* We should have succeeded, copy the token specifier in the buffer */
    hr = StringCbCopyNW(TokenSpecifier->Buffer, TokenSpecifier->MaximumLength,
                        ArcPath, SpecifierLength);
    if (FAILED(hr))
        return NULL;

    TokenSpecifier->Length = wcslen(TokenSpecifier->Buffer) * sizeof(WCHAR);

    /* We succeeded, return the token key value */
    *Key = KeyValue;

    /* Next token starts just after */
    return ++p;
}


ULONG
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

ULONG
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

ULONG
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
        Length = wcslen(TokenTable[Index])*sizeof(WCHAR);
        if (RtlCompareMemory(CandidateToken->Buffer, TokenTable[Index], Length) == Length)
            break;
#else
        RtlInitUnicodeString(&Token, TokenTable[Index]);
        // if (RtlCompareUnicodeString(CandidateToken, &Token, TRUE) == 0)
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
    HRESULT hr;
    PCWSTR EndOfArcName;
    PCWSTR p;

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
        hr = StringCbCopyNW(NormalizedArcPath->Buffer, NormalizedArcPath->MaximumLength,
                            ArcPath, (p - ArcPath) * sizeof(WCHAR));
#else
        hr = StringCbCatNW(NormalizedArcPath->Buffer, NormalizedArcPath->MaximumLength,
                           ArcPath, (p - ArcPath) * sizeof(WCHAR));
#endif
        if (FAILED(hr))
            return FALSE;
        hr = StringCbCatW(NormalizedArcPath->Buffer, NormalizedArcPath->MaximumLength, L"(0)");
        if (FAILED(hr))
            return FALSE;
#if 0
        NormalizedArcPath->Buffer += wcslen(NormalizedArcPath->Buffer);
#endif
        ArcPath = p + 2;
    }
    hr = StringCbCatW(NormalizedArcPath->Buffer, NormalizedArcPath->MaximumLength, ArcPath);
    if (FAILED(hr))
        return FALSE;

    NormalizedArcPath->Length = wcslen(NormalizedArcPath->Buffer) * sizeof(WCHAR);
    return TRUE;
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

#if 0
    *NtName->Buffer = UNICODE_NULL;
    NtName->Length = 0;
#endif

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
                                      SYMBOLIC_LINK_ALL_ACCESS,
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

    /* Resolve the link */
    Status = NtQuerySymbolicLinkObject(LinkHandle, NtName, NULL);

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

    NtClose(LinkHandle);
    return Status;
}

/*
 * ArcNamePath:
 *      In input, pointer to an ARC path (NULL-terminated) starting by an ARC name
 *      to be resolved into a NT device name.
 *      In opposition to ResolveArcNameNtSymLink(), the caller does not have to
 *      delimit the ARC name from within an ARC path. The real ARC name is deduced
 *      after parsing the ARC path, and, in output, ArcNamePath points to the
 *      beginning of the path after the ARC name part.
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
    IN  PPARTLIST PartList OPTIONAL)
{
    HRESULT hr;
    WCHAR TokenBuffer[50];
    UNICODE_STRING Token;
    PCWSTR p, q;
    ULONG AdapterKey;
    ULONG ControllerKey;
    ULONG PeripheralKey;
    ULONG PartitionNumber;
    ADAPTER_TYPE AdapterType;
    CONTROLLER_TYPE ControllerType;
    PERIPHERAL_TYPE PeripheralType;
    BOOLEAN UseSignature = FALSE;

    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry = NULL;

    if (NtName->MaximumLength < sizeof(UNICODE_NULL))
        return STATUS_BUFFER_TOO_SMALL;

#if 0
    *NtName->Buffer = UNICODE_NULL;
    NtName->Length = 0;
#endif

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
        AdapterType = (ADAPTER_TYPE)/*ArcMatchTokenU*/ArcMatchToken_UStr(/*Token.Buffer*/&Token, AdapterTypes_U);
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

            hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\Ramdisk%lu", AdapterKey);
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
    ControllerType = (CONTROLLER_TYPE)/*ArcMatchTokenU*/ArcMatchToken_UStr(/*Token.Buffer*/&Token, ControllerTypes_U);
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
    PeripheralType = (PERIPHERAL_TYPE)/*ArcMatchTokenU*/ArcMatchToken_UStr(/*Token.Buffer*/&Token, PeripheralTypes_U);
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


    if (ControllerType == CdRomController) // and so, AdapterType == ScsiAdapter and PeripheralType == FDiskPeripheral
        hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\Scsi\\CdRom%lu", ControllerKey);
    else
    /* Now, ControllerType == DiskController */
    if (PeripheralType == CdRomPeripheral)
        hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\CdRom%lu", PeripheralKey);
    else
    if (PeripheralType == FDiskPeripheral)
        hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\Floppy%lu", PeripheralKey);
    else
    if (PeripheralType == RDiskPeripheral)
    {
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

        hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\Harddisk%lu\\Partition%lu",
                             DiskEntry->DiskNumber, PartitionNumber);
    }
#if 0
    else
    if (PeripheralType == VDiskPeripheral)
    {
        // TODO: Check how Win 7+ deals with virtual disks.
        hr = StringCbPrintfW(NtName->Buffer, NtName->MaximumLength, L"\\Device\\VirtualHarddisk%lu\\Partition%lu",
                             PeripheralKey, PartitionNumber);
    }
#endif

Quit:
    if (FAILED(hr))
    {
        /*
         * We can directly cast the HRESULTs into NTSTATUS since the error codes
         * returned by StringCbPrintfW:
         *    STRSAFE_E_INVALID_PARAMETER   == 0x80070057,
         *    STRSAFE_E_INSUFFICIENT_BUFFER == 0x8007007a,
         * do not have assigned values in the NTSTATUS space.
         */
        return (NTSTATUS)hr;
    }

    *ArcNamePath = p;
    return STATUS_SUCCESS;
}


/**** FIXME: Redundant with filesup.c ! ****\
|** (but filesup.c is not yet included in **|
\**    setuplib, hence this code copy)    **/

static
HRESULT
ConcatPaths(
    IN OUT PWSTR PathElem1,
    IN SIZE_T cchPathSize,
    IN PCWSTR PathElem2 OPTIONAL)
{
    HRESULT hr;
    SIZE_T cchPathLen;

    if (!PathElem2)
        return S_OK;
    if (cchPathSize <= 1)
        return S_OK;

    cchPathLen = min(cchPathSize, wcslen(PathElem1));

    if (PathElem2[0] != L'\\' && cchPathLen > 0 && PathElem1[cchPathLen-1] != L'\\')
    {
        /* PathElem2 does not start with '\' and PathElem1 does not end with '\' */
        hr = StringCchCatW(PathElem1, cchPathSize, L"\\");
        if (FAILED(hr))
            return hr;
    }
    else if (PathElem2[0] == L'\\' && cchPathLen > 0 && PathElem1[cchPathLen-1] == L'\\')
    {
        /* PathElem2 starts with '\' and PathElem1 ends with '\' */
        while (*PathElem2 == L'\\')
            ++PathElem2; // Skip any backslash
    }
    hr = StringCchCatW(PathElem1, cchPathSize, PathElem2);
    return hr;
}

/*******************************************/


BOOLEAN
ArcPathToNtPath(
    OUT PUNICODE_STRING NtPath,
    IN  PCWSTR ArcPath,
    IN  PPARTLIST PartList OPTIONAL)
{
    NTSTATUS Status;
    PCWSTR BeginOfPath;
    UNICODE_STRING ArcName;

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
        HRESULT hr;
        hr = ConcatPaths(NtPath->Buffer, NtPath->MaximumLength / sizeof(WCHAR), BeginOfPath);
        if (FAILED(hr))
        {
            /* Buffer not large enough, or whatever...: just bail out */
            return FALSE;
        }
    }
    NtPath->Length = wcslen(NtPath->Buffer) * sizeof(WCHAR);
    return TRUE;
}

#if 0
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
