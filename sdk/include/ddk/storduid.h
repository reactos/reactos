#ifndef _STORDUID_H_
#define _STORDUID_H_

typedef enum _DUID_MATCH_STATUS
{
  DuidExactMatch = 0,
  DuidSubIdMatch,
  DuidNoMatch,
  DuidErrorGeneral = 100,
  DuidErrorMissingDuid,
  DuidErrorVersionMismatch,
  DuidErrorInvalidDuid,
  DuidErrorInvalidDeviceIdDescSize,
  DuidErrorInvalidDeviceDescSize,
  DuidErrorInvalidLayoutSigSize,
  DuidErrorInvalidLayoutSigVersion,
  DuidErrorMaximum
} DUID_MATCH_STATUS;

#define DUID_VERSION_1 1

#define DUID_HARDWARE_IDS_ONLY 0
#define DUID_INCLUDE_SOFTWARE_IDS 1

#define DUID_MATCH_ERROR(_duid_status) ((_duid_status) >= DuidErrorGeneral ? TRUE : FALSE)
#define DUID_MATCH_SUCCESS(_duid_status) ((_duid_status) < DuidErrorGeneral ? TRUE : FALSE)

typedef struct _STORAGE_DEVICE_UNIQUE_IDENTIFIER
{
  ULONG Version;
  ULONG Size;
  ULONG StorageDeviceIdOffset;
  ULONG StorageDeviceOffset;
  ULONG DriveLayoutSignatureOffset;
} STORAGE_DEVICE_UNIQUE_IDENTIFIER, *PSTORAGE_DEVICE_UNIQUE_IDENTIFIER;

typedef struct _STORAGE_DEVICE_LAYOUT_SIGNATURE
{
  ULONG Version;
  ULONG Size;
  BOOLEAN Mbr;
  union {
    ULONG MbrSignature;
    GUID GptDiskId;
  } DeviceSpecific;
} STORAGE_DEVICE_LAYOUT_SIGNATURE, *PSTORAGE_DEVICE_LAYOUT_SIGNATURE;

FORCEINLINE
DUID_MATCH_STATUS
CompareStorageDuids(
  _In_ PSTORAGE_DEVICE_UNIQUE_IDENTIFIER Duid1,
  _In_ PSTORAGE_DEVICE_UNIQUE_IDENTIFIER Duid2);

FORCEINLINE
DUID_MATCH_STATUS
CompareStorageDuids(
  _In_ PSTORAGE_DEVICE_UNIQUE_IDENTIFIER Duid1,
  _In_ PSTORAGE_DEVICE_UNIQUE_IDENTIFIER Duid2)
{
  if (!Duid1 || !Duid2)
  {
    return DuidErrorMissingDuid;
  }

  if (Duid1->Size < sizeof(STORAGE_DEVICE_UNIQUE_IDENTIFIER) ||
      Duid2->Size < sizeof(STORAGE_DEVICE_UNIQUE_IDENTIFIER))
  {
    return DuidErrorGeneral;
  }

  if (Duid1->Version != DUID_VERSION_1 || Duid2->Version != DUID_VERSION_1)
  {
    return DuidErrorVersionMismatch;
  }

  if (Duid1->StorageDeviceIdOffset == 0 && Duid1->StorageDeviceOffset == 0 &&
      Duid1->DriveLayoutSignatureOffset == 0)
  {
    return DuidErrorInvalidDuid;
  }

  if (Duid2->StorageDeviceIdOffset == 0 && Duid2->StorageDeviceOffset == 0 &&
      Duid2->DriveLayoutSignatureOffset == 0)
  {
    return DuidErrorInvalidDuid;
  }

  if (Duid1->Size == Duid2->Size)
  {
    if (memcmp(Duid1, Duid2, Duid1->Size) == 0)
    {
      return DuidExactMatch;
    }
  }

  if (Duid1->StorageDeviceIdOffset && Duid2->StorageDeviceIdOffset)
  {
    PSTORAGE_DEVICE_ID_DESCRIPTOR idDesc1;
    PSTORAGE_DEVICE_ID_DESCRIPTOR idDesc2;

    PSTORAGE_IDENTIFIER ident1;
    PSTORAGE_IDENTIFIER ident2;

    ULONG idx1;
    ULONG idx2;

    idDesc1 = (PSTORAGE_DEVICE_ID_DESCRIPTOR)((PUCHAR)Duid1 + Duid1->StorageDeviceIdOffset);
    idDesc2 = (PSTORAGE_DEVICE_ID_DESCRIPTOR)((PUCHAR)Duid2 + Duid2->StorageDeviceIdOffset);

    if (idDesc1->Size < sizeof(STORAGE_DEVICE_ID_DESCRIPTOR) ||
        idDesc2->Size < sizeof(STORAGE_DEVICE_ID_DESCRIPTOR))
    {
      return DuidErrorInvalidDeviceIdDescSize;
    }

    if (idDesc1->Size == idDesc2->Size)
    {
      if (memcmp(idDesc1, idDesc2, idDesc1->Size) == 0)
      {
        return DuidSubIdMatch;
      }
    }

    ident1 = (PSTORAGE_IDENTIFIER)(idDesc1->Identifiers);

    for (idx1 = 0; idx1 < idDesc1->NumberOfIdentifiers; idx1++)
    {
      if ((ident1->Type == StorageIdTypeScsiNameString || ident1->Type == StorageIdTypeFCPHName ||
           ident1->Type == StorageIdTypeEUI64 || ident1->Type == StorageIdTypeVendorId) &&
          (ident1->Association == StorageIdAssocPort) &&
          (ident1->CodeSet == StorageIdCodeSetUtf8 || ident1->CodeSet == StorageIdCodeSetAscii ||
           ident1->CodeSet == StorageIdCodeSetBinary))
      {
        ident2 = (PSTORAGE_IDENTIFIER)(idDesc2->Identifiers);

        for (idx2 = 0; idx2 < idDesc2->NumberOfIdentifiers; idx2++)
        {
          if (ident1->Type == ident2->Type && ident1->Association == ident2->Association &&
              ident1->CodeSet == ident2->CodeSet &&
              ident1->IdentifierSize == ident2->IdentifierSize &&
              (memcmp(ident1->Identifier, ident2->Identifier, ident1->IdentifierSize) == 0))
          {
            return DuidSubIdMatch;
          }

          ident2 = (PSTORAGE_IDENTIFIER)((PUCHAR)ident2 + ident2->NextOffset);
        }
      }

      ident1 = (PSTORAGE_IDENTIFIER)((PUCHAR)ident1 + ident1->NextOffset);
    }
  }

  if (Duid1->StorageDeviceOffset && Duid2->StorageDeviceOffset)
  {
    PSTORAGE_DEVICE_DESCRIPTOR desc1;
    PSTORAGE_DEVICE_DESCRIPTOR desc2;

    desc1 = (PSTORAGE_DEVICE_DESCRIPTOR)((PUCHAR)Duid1 + Duid1->StorageDeviceOffset);
    desc2 = (PSTORAGE_DEVICE_DESCRIPTOR)((PUCHAR)Duid2 + Duid2->StorageDeviceOffset);

    if (desc1->Size < sizeof(STORAGE_DEVICE_DESCRIPTOR) ||
        desc2->Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
    {
      return DuidErrorInvalidDeviceDescSize;
    }

    if (desc1->Size == desc2->Size)
    {
      if (memcmp(desc1, desc2, desc1->Size) == 0)
      {
        return DuidSubIdMatch;
      }
    }

    if (desc1->SerialNumberOffset && desc2->SerialNumberOffset)
    {
      const char *string1;
      const char *string2;

      string1 = (const char *)((PUCHAR)desc1 + desc1->SerialNumberOffset);
      string2 = (const char *)((PUCHAR)desc2 + desc2->SerialNumberOffset);

      if (strcmp(string1, string2) == 0)
      {
        if (desc1->VendorIdOffset && desc2->VendorIdOffset)
        {
          string1 = (const char *)((PUCHAR)desc1 + desc1->VendorIdOffset);
          string2 = (const char *)((PUCHAR)desc2 + desc2->VendorIdOffset);

          if (strcmp(string1, string2) != 0)
          {
            return DuidNoMatch;
          }
        }

        if (desc1->ProductIdOffset && desc2->ProductIdOffset)
        {
          string1 = (const char *)((PUCHAR)desc1 + desc1->ProductIdOffset);
          string2 = (const char *)((PUCHAR)desc2 + desc2->ProductIdOffset);

          if (strcmp(string1, string2) != 0)
          {
            return DuidNoMatch;
          }
        }

        return DuidSubIdMatch;
      }
    }
  }

  if (Duid1->DriveLayoutSignatureOffset && Duid2->DriveLayoutSignatureOffset)
  {
    PSTORAGE_DEVICE_LAYOUT_SIGNATURE sig1;
    PSTORAGE_DEVICE_LAYOUT_SIGNATURE sig2;

    sig1 = (PSTORAGE_DEVICE_LAYOUT_SIGNATURE)((PUCHAR)Duid1 + Duid1->DriveLayoutSignatureOffset);
    sig2 = (PSTORAGE_DEVICE_LAYOUT_SIGNATURE)((PUCHAR)Duid2 + Duid2->DriveLayoutSignatureOffset);

    if (sig1->Version != DUID_VERSION_1 && sig2->Version != DUID_VERSION_1)
    {
      return DuidErrorInvalidLayoutSigVersion;
    }

    if (sig1->Size < sizeof(STORAGE_DEVICE_LAYOUT_SIGNATURE) ||
        sig2->Size < sizeof(STORAGE_DEVICE_LAYOUT_SIGNATURE))
    {
      return DuidErrorInvalidLayoutSigSize;
    }

    if (memcmp(sig1, sig2, sizeof(STORAGE_DEVICE_LAYOUT_SIGNATURE)) == 0)
    {
      return DuidSubIdMatch;
    }
  }

  return DuidNoMatch;
}

#endif /* _STORDUID_H_ */
