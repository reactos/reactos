/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/sablob.c
 * PURPOSE:     Functions for the Saved Answer Blob Implementation
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PVOID
WINAPI
FlatBuf_Arg_ReserveAlignPointer(IN PVOID Position,
                                IN PSIZE_T FreeSize,
                                IN SIZE_T Size)
{
    /* Just a little helper that we use */
    return FlatBuf_Arg_Reserve(Position, FreeSize, Size, sizeof(PVOID));
}

PDNS_BLOB
WINAPI
SaBlob_Create(IN ULONG Count)
{
    PDNS_BLOB Blob;
    PDNS_ARRAY DnsAddrArray;

    /* Allocate the blob */
    Blob = Dns_AllocZero(sizeof(DNS_BLOB));
    if (Blob)
    {
        /* Check if it'll hold any addresses */
        if (Count)
        {
            /* Create the DNS Address Array */
            DnsAddrArray = DnsAddrArray_Create(Count);
            if (!DnsAddrArray)
            {
                /* Failure, free the blob */
                SaBlob_Free(Blob);
                SetLastError(ERROR_OUTOFMEMORY);
            }
            else
            {
                /* Link it with the blob */
                Blob->DnsAddrArray = DnsAddrArray;
            }
        }
    }

    /* Return the blob */
    return Blob;
}

PDNS_BLOB
WINAPI
SaBlob_CreateFromIp4(IN LPWSTR Name,
                     IN ULONG Count,
                     IN PIN_ADDR AddressArray)
{
    PDNS_BLOB Blob;
    LPWSTR NameCopy;
    ULONG i;

    /* Create the blob */
    Blob = SaBlob_Create(Count);
    if (!Blob) goto Quickie;

    /* If we have a name */
    if (Name)
    {
        /* Create a copy of it */
        NameCopy = Dns_CreateStringCopy_W(Name);
        if (!NameCopy) goto Quickie;

        /* Save the pointer to the name */
        Blob->Name = NameCopy;
    }

    /* Loop all the addresses */
    for (i = 0; i < Count; i++)
    {
        /* Add an entry for this address */
        DnsAddrArray_AddIp4(Blob->DnsAddrArray, AddressArray[i], IpV4Address);
    }

    /* Return the blob */
    return Blob;

Quickie:
    /* Free the blob, set error and fail */
    SaBlob_Free(Blob);
    SetLastError(ERROR_OUTOFMEMORY);
    return NULL;
}

VOID
WINAPI
SaBlob_Free(IN PDNS_BLOB Blob)
{
    /* Make sure we got a blob */
    if (Blob)
    {
        /* Free the name */
        Dns_Free(Blob->Name);

        /* Loop the aliases */
        while (Blob->AliasCount)
        {
            /* Free the alias */
            Dns_Free(Blob->Aliases[Blob->AliasCount]);

            /* Decrease number of aliases */
            Blob->AliasCount--;
        }

        /* Free the DNS Address Array */
        DnsAddrArray_Free(Blob->DnsAddrArray);

        /* Free the blob itself */
        Dns_Free(Blob);
    }
}

PHOSTENT
WINAPI
SaBlob_CreateHostent(IN OUT PULONG_PTR BufferPosition,
                     IN OUT PSIZE_T FreeBufferSpace,
                     IN OUT PSIZE_T HostEntrySize,
                     IN PDNS_BLOB Blob,
                     IN DWORD StringType,
                     IN BOOLEAN Relative,
                     IN BOOLEAN BufferAllocated)
{
    PDNS_ARRAY DnsAddrArray = Blob->DnsAddrArray;
    ULONG AliasCount = Blob->AliasCount;
    WORD AddressFamily = AF_UNSPEC;
    ULONG AddressCount = 0, AddressSize = 0, TotalSize, NamePointerSize;
    ULONG AliasPointerSize;
    PDNS_FAMILY_INFO FamilyInfo = NULL;
    ULONG StringLength = 0;
    ULONG i;
    ULONG HostentSize = 0;
    PHOSTENT Hostent = NULL;
    ULONG_PTR HostentPtr;
    PVOID CurrentAddress;

    /* Check if we actually have any addresses */
    if (DnsAddrArray)
    {
        /* Get the address family */
        AddressFamily = DnsAddrArray->Addresses[0].AddressFamily;

        /* Get family information */
        FamilyInfo = FamilyInfo_GetForFamily(AddressFamily);

        /* Save the current address count and their size */
        AddressCount = DnsAddrArray->UsedAddresses;
        AddressSize = FamilyInfo->AddressSize;
    }

    /* Calculate total size for all the addresses, and their pointers */
    TotalSize = AddressSize * AddressCount;
    NamePointerSize = AddressCount * sizeof(PVOID) + sizeof(PVOID);

    /* Check if we have a name */
    if (Blob->Name)
    {
        /* Find out the size we'll need for a copy */
        StringLength = (Dns_GetBufferLengthForStringCopy(Blob->Name,
                                                         0,
                                                         UnicodeString,
                                                         StringType) + 1) & ~1;
    }

    /*  Now do the same for the aliases */
    for (i = AliasCount; i; i--)
    {
        /* Find out the size we'll need for a copy */
        HostentSize += (Dns_GetBufferLengthForStringCopy(Blob->Aliases[i],
                                                         0,
                                                         UnicodeString,
                                                         StringType) + 1) & ~1;
    }

    /* Find out how much the pointers will take */
    AliasPointerSize = AliasCount * sizeof(PVOID) + sizeof(PVOID);

    /* Calculate Hostent Size */
    HostentSize += TotalSize +
                   NamePointerSize +
                   AliasPointerSize +
                   StringLength +
                   sizeof(HOSTENT);

    /* Check if we already have a buffer */
    if (!BufferAllocated)
    {
        /* We don't, allocate space ourselves */
        HostentPtr = (ULONG_PTR)Dns_AllocZero(HostentSize);
    }
    else
    {
        /* We do, so allocate space in the buffer */
        HostentPtr = (ULONG_PTR)FlatBuf_Arg_ReserveAlignPointer(BufferPosition,
                                                                FreeBufferSpace,
                                                                HostentSize);
    }

    /* Make sure we got space */
    if (HostentPtr)
    {
        /* Initialize it */
        Hostent = Hostent_Init((PVOID)&HostentPtr,
                               AddressFamily,
                               AddressSize,
                               AddressCount,
                               AliasCount);
    }

    /* Loop the addresses */
    for (i = 0; i < AddressCount; i++)
    {
        /* Get the pointer of the current address */
        CurrentAddress = (PVOID)((ULONG_PTR)&DnsAddrArray->Addresses[i] +
                                             FamilyInfo->AddressOffset);

        /* Write the pointer */
        Hostent->h_addr_list[i] = (PCHAR)HostentPtr;

        /* Copy the address */
        RtlCopyMemory((PVOID)HostentPtr, CurrentAddress, AddressSize);

        /* Advance the buffer */
        HostentPtr += AddressSize;
    }

    /* Check if we have a name */
    if (Blob->Name)
    {
        /* Align our current position */
        HostentPtr += 1 & ~1;

        /* Save our name here */
        Hostent->h_name = (LPSTR)HostentPtr;

        /* Now copy it in the blob */
        HostentPtr += Dns_StringCopy((PVOID)HostentPtr,
                                     NULL,
                                     Blob->Name,
                                     0,
                                     UnicodeString,
                                     StringType);
    }

    /* Loop the Aliases */
    for (i = AliasCount; i; i--)
    {
        /* Align our current position */
        HostentPtr += 1 & ~1;

        /* Save our alias here */
        Hostent->h_aliases[i] = (LPSTR)HostentPtr;

        /* Now copy it in the blob */
        HostentPtr += Dns_StringCopy((PVOID)HostentPtr,
                                     NULL,
                                     Blob->Aliases[i],
                                     0,
                                     UnicodeString,
                                     StringType);
    }

    /* Check if the caller didn't have a buffer */
    if (!BufferAllocated)
    {
        /* Return the size; not needed if we had a blob, since it's internal */
        *HostEntrySize = *BufferPosition - (ULONG_PTR)HostentPtr;
    }

    /* Convert to Offsets if requested */
    if(Relative) Hostent_ConvertToOffsets(Hostent);

    /* Return the full, complete, hostent */
    return Hostent;
}

INT
WINAPI
SaBlob_WriteNameOrAlias(IN PDNS_BLOB Blob,
                        IN LPWSTR String,
                        IN BOOLEAN IsAlias)
{
    /* Check if this is an alias */
    if (!IsAlias)
    {
        /* It's not. Simply create a copy of the string */
        Blob->Name = Dns_CreateStringCopy_W(String);
        if (!Blob->Name) return GetLastError();
    }
    else
    {
        /* Does it have a name, and less then 8 aliases? */
        if ((Blob->Name) && (Blob->AliasCount <= 8))
        {
            /* Yup, create a copy of the string and increase the alias count */
            Blob->Aliases[Blob->AliasCount] = Dns_CreateStringCopy_W(String);
            Blob->AliasCount++;
        }
        else
        {
            /* Invalid request! */
            return ERROR_MORE_DATA;
        }
    }

    /* Return Success */
    return ERROR_SUCCESS;
}

INT
WINAPI
SaBlob_WriteAddress(IN PDNS_BLOB Blob,
                    OUT PDNS_ADDRESS DnsAddr)
{
    /* Check if we have an array yet */
    if (!Blob->DnsAddrArray)
    {
        /* Allocate one! */
        Blob->DnsAddrArray = DnsAddrArray_Create(1);
        if (!Blob->DnsAddrArray) return ERROR_OUTOFMEMORY;
    }

    /* Add this address */
    return DnsAddrArray_AddAddr(Blob->DnsAddrArray, DnsAddr, AF_UNSPEC, 0) ?
           ERROR_SUCCESS:
           ERROR_MORE_DATA;
}

BOOLEAN
WINAPI
SaBlob_IsSupportedAddrType(WORD DnsType)
{
    /* Check for valid Types that we support */
    return (DnsType == DNS_TYPE_A ||
            DnsType == DNS_TYPE_ATMA ||
            DnsType == DNS_TYPE_AAAA);
}

INT
WINAPI
SaBlob_WriteRecords(OUT PDNS_BLOB Blob,
                    IN PDNS_RECORD DnsRecord,
                    IN BOOLEAN DoAlias)
{
    DNS_ADDRESS DnsAddress;
    INT ErrorCode = STATUS_INVALID_PARAMETER;
    BOOLEAN WroteOnce = FALSE;

    /* Zero out the Address */
    RtlZeroMemory(&DnsAddress, sizeof(DnsAddress));

    /* Loop through all the Records */
    while (DnsRecord)
    {
        /* Is this not an answer? */
        if (DnsRecord->Flags.S.Section != DNSREC_ANSWER)
        {
            /* Then simply move on to the next DNS Record */
            DnsRecord = DnsRecord->pNext;
            continue;
        }

        /* Check the type of thsi record */
        switch(DnsRecord->wType)
        {
            /* Regular IPv4, v6 or ATM Record */
            case DNS_TYPE_A:
            case DNS_TYPE_AAAA:
            case DNS_TYPE_ATMA:

                /* Create a DNS Address from the record */
                DnsAddr_BuildFromDnsRecord(DnsRecord, &DnsAddress);

                /* Add it to the DNS Blob */
                ErrorCode = SaBlob_WriteAddress(Blob, &DnsAddress);

                /* Add the name, if needed */
                if ((DoAlias) &&
                    (!WroteOnce) &&
                    (!Blob->Name) &&
                    (DnsRecord->pName))
                {
                    /* Write the name from the DNS Record */
                    ErrorCode = SaBlob_WriteNameOrAlias(Blob,
                                                        DnsRecord->pName,
                                                        FALSE);
                    WroteOnce = TRUE;
                }
                break;

            case DNS_TYPE_CNAME:

                /* Just write the alias name */
                ErrorCode = SaBlob_WriteNameOrAlias(Blob,
                                                    DnsRecord->pName,
                                                    TRUE);
                break;

            case DNS_TYPE_PTR:

                /* Check if we already have a name */
                if (Blob->Name)
                {
                    /* We don't, so add this as a name */
                    ErrorCode = SaBlob_WriteNameOrAlias(Blob,
                                                        DnsRecord->pName,
                                                        FALSE);
                }
                else
                {
                    /* We do, so add it as an alias */
                    ErrorCode = SaBlob_WriteNameOrAlias(Blob,
                                                        DnsRecord->pName,
                                                        TRUE);
                }
                break;
            default:
                break;
        }

        /* Next record */
        DnsRecord = DnsRecord->pNext;
    }

    /* Return error code */
    return ErrorCode;
}

PDNS_BLOB
WINAPI
SaBlob_CreateFromRecords(IN PDNS_RECORD DnsRecord,
                         IN BOOLEAN DoAliases,
                         IN DWORD DnsType)
{
    PDNS_RECORD LocalDnsRecord;
    ULONG ProcessedCount = 0;
    PDNS_BLOB DnsBlob;
    INT ErrorCode;
    DNS_ADDRESS DnsAddress;

    /* Find out how many DNS Addresses to allocate */
    LocalDnsRecord = DnsRecord;
    while (LocalDnsRecord)
    {
        /* Make sure this record is an answer */
        if ((LocalDnsRecord->Flags.S.Section == DNSREC_ANSWER) &&
            (SaBlob_IsSupportedAddrType(LocalDnsRecord->wType)))
        {
            /* Increase number of records to process */
            ProcessedCount++;
        }

        /* Move to the next record */
        LocalDnsRecord = LocalDnsRecord->pNext;
    }

    /* Create the DNS Blob */
    DnsBlob = SaBlob_Create(ProcessedCount);
    if (!DnsBlob)
    {
        /* Fail */
        ErrorCode = GetLastError();
        goto Quickie;
    }

    /* Write the record to the DNS Blob */
    ErrorCode = SaBlob_WriteRecords(DnsBlob, DnsRecord, TRUE);
    if (ErrorCode != NO_ERROR)
    {
        /* We failed... but do we still have valid data? */
        if ((DnsBlob->Name) || (DnsBlob->AliasCount))
        {
            /* We'll just assume success then */
            ErrorCode = NO_ERROR;
        }
        else
        {
            /* Ok, last chance..do you have a DNS Address Array? */
            if ((DnsBlob->DnsAddrArray) &&
                (DnsBlob->DnsAddrArray->UsedAddresses))
            {
                /* Boy are you lucky! */
                ErrorCode = NO_ERROR;
            }
        }

        /* Buh-bye! */
        goto Quickie;
    }

    /* Check if this is a PTR record */
    if ((DnsRecord->wType == DNS_TYPE_PTR) ||
        ((DnsType == DNS_TYPE_PTR) &&
         (DnsRecord->wType == DNS_TYPE_CNAME) &&
         (DnsRecord->Flags.S.Section == DNSREC_ANSWER)))
    {
        /* Get a DNS Address Structure */
        if (Dns_ReverseNameToDnsAddr_W(&DnsAddress, DnsRecord->pName))
        {
            /* Add it to the Blob */
            if (SaBlob_WriteAddress(DnsBlob, &DnsAddress)) ErrorCode = NO_ERROR;
        }
    }

    /* Ok...do we still not have a name? */
    if (!(DnsBlob->Name) && (DoAliases) && (LocalDnsRecord))
    {
        /* We have an local DNS Record, so just use it to write the name */
        ErrorCode = SaBlob_WriteNameOrAlias(DnsBlob,
                                            LocalDnsRecord->pName,
                                            FALSE);
    }

Quickie:
    /* Check error code */
    if (ErrorCode != NO_ERROR)
    {
        /* Free the blob and set the error */
        SaBlob_Free(DnsBlob);
        DnsBlob = NULL;
        SetLastError(ErrorCode);
    }

    /* Return */
    return DnsBlob;
}

PDNS_BLOB
WINAPI
SaBlob_Query(IN LPWSTR Name,
             IN WORD DnsType,
             IN ULONG Flags,
             IN PVOID *Reserved,
             IN DWORD AddressFamily)
{
    PDNS_RECORD DnsRecord = NULL;
    INT ErrorCode;
    PDNS_BLOB DnsBlob = NULL;
    LPWSTR LocalName, LocalNameCopy;

    /* If they want reserved data back, clear it out in case we fail */
    if (Reserved) *Reserved = NULL;

    /* Query DNS */
    ErrorCode = DnsQuery_W(Name,
                           DnsType,
                           Flags,
                           NULL,
                           &DnsRecord,
                           Reserved);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* We failed... did the caller use reserved data? */
        if (Reserved && *Reserved)
        {
            /* He did, and it was valid. Free it */
            DnsApiFree(*Reserved);
            *Reserved = NULL;
        }

        /* Normalize error code */
        if (ErrorCode == RPC_S_SERVER_UNAVAILABLE) ErrorCode = WSATRY_AGAIN;
        goto Quickie;
    }

    /* Now create the Blob from the DNS Records */
    DnsBlob = SaBlob_CreateFromRecords(DnsRecord, TRUE, DnsType);
    if (!DnsBlob)
    {
        /* Failed, get error code */
        ErrorCode = GetLastError();
        goto Quickie;
    }

    /* Make sure it has a name */
    if (!DnsBlob->Name)
    {
        /* It doesn't, fail */
        ErrorCode = DNS_INFO_NO_RECORDS;
        goto Quickie;
    }

    /* Check if the name is local or loopback */
    if (!(DnsNameCompare_W(DnsBlob->Name, L"localhost")) &&
        !(DnsNameCompare_W(DnsBlob->Name, L"loopback")))
    {
        /* Nothing left to do, exit! */
        goto Quickie;
    }

    /* This is a local name...query it */
    LocalName = DnsQueryConfigAllocEx(DnsConfigFullHostName_W, NULL, NULL);
    if (LocalName)
    {
        /* Create a copy for the caller */
        LocalNameCopy = Dns_CreateStringCopy_W(LocalName);
        if (LocalNameCopy)
        {
            /* Overwrite the one in the blob */
            DnsBlob->Name = LocalNameCopy;
        }
        else
        {
            /* We failed to make a copy, free memory */
            DnsApiFree(LocalName);
        }
    }

Quickie:
    /* Free the DNS Record if we have one */
    if (DnsRecord) DnsRecordListFree(DnsRecord, DnsFreeRecordList);

    /* Check if this is a failure path with an active blob */
    if ((ErrorCode != ERROR_SUCCESS) && (DnsBlob))
    {
        /* Free the blob */
        SaBlob_Free(DnsBlob);
        DnsBlob = NULL;
    }

    /* Set the last error and return */
    SetLastError(ErrorCode);
    return DnsBlob;
}

