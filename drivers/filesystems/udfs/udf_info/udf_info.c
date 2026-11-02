////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
        Module name:

   udf_info.cpp

        Abstract:

   This file contains filesystem-specific routines

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO

static const uint16 CrcTable[256] = {
    0x0000U, 0x1021U, 0x2042U, 0x3063U, 0x4084U, 0x50a5U, 0x60c6U, 0x70e7U,
    0x8108U, 0x9129U, 0xa14aU, 0xb16bU, 0xc18cU, 0xd1adU, 0xe1ceU, 0xf1efU,
    0x1231U, 0x0210U, 0x3273U, 0x2252U, 0x52b5U, 0x4294U, 0x72f7U, 0x62d6U,
    0x9339U, 0x8318U, 0xb37bU, 0xa35aU, 0xd3bdU, 0xc39cU, 0xf3ffU, 0xe3deU,
    0x2462U, 0x3443U, 0x0420U, 0x1401U, 0x64e6U, 0x74c7U, 0x44a4U, 0x5485U,
    0xa56aU, 0xb54bU, 0x8528U, 0x9509U, 0xe5eeU, 0xf5cfU, 0xc5acU, 0xd58dU,
    0x3653U, 0x2672U, 0x1611U, 0x0630U, 0x76d7U, 0x66f6U, 0x5695U, 0x46b4U,
    0xb75bU, 0xa77aU, 0x9719U, 0x8738U, 0xf7dfU, 0xe7feU, 0xd79dU, 0xc7bcU,
    0x48c4U, 0x58e5U, 0x6886U, 0x78a7U, 0x0840U, 0x1861U, 0x2802U, 0x3823U,
    0xc9ccU, 0xd9edU, 0xe98eU, 0xf9afU, 0x8948U, 0x9969U, 0xa90aU, 0xb92bU,
    0x5af5U, 0x4ad4U, 0x7ab7U, 0x6a96U, 0x1a71U, 0x0a50U, 0x3a33U, 0x2a12U,
    0xdbfdU, 0xcbdcU, 0xfbbfU, 0xeb9eU, 0x9b79U, 0x8b58U, 0xbb3bU, 0xab1aU,
    0x6ca6U, 0x7c87U, 0x4ce4U, 0x5cc5U, 0x2c22U, 0x3c03U, 0x0c60U, 0x1c41U,
    0xedaeU, 0xfd8fU, 0xcdecU, 0xddcdU, 0xad2aU, 0xbd0bU, 0x8d68U, 0x9d49U,
    0x7e97U, 0x6eb6U, 0x5ed5U, 0x4ef4U, 0x3e13U, 0x2e32U, 0x1e51U, 0x0e70U,
    0xff9fU, 0xefbeU, 0xdfddU, 0xcffcU, 0xbf1bU, 0xaf3aU, 0x9f59U, 0x8f78U,
    0x9188U, 0x81a9U, 0xb1caU, 0xa1ebU, 0xd10cU, 0xc12dU, 0xf14eU, 0xe16fU,
    0x1080U, 0x00a1U, 0x30c2U, 0x20e3U, 0x5004U, 0x4025U, 0x7046U, 0x6067U,
    0x83b9U, 0x9398U, 0xa3fbU, 0xb3daU, 0xc33dU, 0xd31cU, 0xe37fU, 0xf35eU,
    0x02b1U, 0x1290U, 0x22f3U, 0x32d2U, 0x4235U, 0x5214U, 0x6277U, 0x7256U,
    0xb5eaU, 0xa5cbU, 0x95a8U, 0x8589U, 0xf56eU, 0xe54fU, 0xd52cU, 0xc50dU,
    0x34e2U, 0x24c3U, 0x14a0U, 0x0481U, 0x7466U, 0x6447U, 0x5424U, 0x4405U,
    0xa7dbU, 0xb7faU, 0x8799U, 0x97b8U, 0xe75fU, 0xf77eU, 0xc71dU, 0xd73cU,
    0x26d3U, 0x36f2U, 0x0691U, 0x16b0U, 0x6657U, 0x7676U, 0x4615U, 0x5634U,
    0xd94cU, 0xc96dU, 0xf90eU, 0xe92fU, 0x99c8U, 0x89e9U, 0xb98aU, 0xa9abU,
    0x5844U, 0x4865U, 0x7806U, 0x6827U, 0x18c0U, 0x08e1U, 0x3882U, 0x28a3U,
    0xcb7dU, 0xdb5cU, 0xeb3fU, 0xfb1eU, 0x8bf9U, 0x9bd8U, 0xabbbU, 0xbb9aU,
    0x4a75U, 0x5a54U, 0x6a37U, 0x7a16U, 0x0af1U, 0x1ad0U, 0x2ab3U, 0x3a92U,
    0xfd2eU, 0xed0fU, 0xdd6cU, 0xcd4dU, 0xbdaaU, 0xad8bU, 0x9de8U, 0x8dc9U,
    0x7c26U, 0x6c07U, 0x5c64U, 0x4c45U, 0x3ca2U, 0x2c83U, 0x1ce0U, 0x0cc1U,
    0xef1fU, 0xff3eU, 0xcf5dU, 0xdf7cU, 0xaf9bU, 0xbfbaU, 0x8fd9U, 0x9ff8U,
    0x6e17U, 0x7e36U, 0x4e55U, 0x5e74U, 0x2e93U, 0x3eb2U, 0x0ed1U, 0x1ef0U
};

static const uint32 crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
};

/*
    This routine converts compressed Unicode to standard
 */
VOID
UDFDecompressUnicode(
    IN OUT PUNICODE_STRING UName,
    IN uint8* CS0,
    IN SIZE_T Length,
    OUT uint16* valueCRC
    )
{
    uint16 compID = CS0[0];
    uint32 unicodeIndex = 0;
    uint32 byteIndex = 1;
    PWCHAR buff;
    uint8* _CS0 = CS0+1;

    if (!Length) goto return_empty_str;
    // First check for valid compID.
    switch(compID) {
    case UDF_COMP_ID_8: {

        buff = (PWCHAR)MyAllocatePoolTag__(UDF_FILENAME_MT, (Length)*sizeof(WCHAR), MEM_FNAME_TAG);
        if (!buff) goto return_empty_str;
        UName->Buffer = buff;

        // Loop through all the bytes.
        while (byteIndex < Length) {
            (*buff) = (*_CS0);
            _CS0++;
            byteIndex++;
            buff++;
        }
        unicodeIndex = byteIndex-1;
        break;
    }
    case UDF_COMP_ID_16: {

        buff = (PWCHAR)MyAllocatePoolTag__(UDF_FILENAME_MT, (Length-1)+sizeof(WCHAR), MEM_FNAME16_TAG);
        if (!buff) goto return_empty_str;
        UName->Buffer = buff;

        // Loop through all the bytes.
        while (byteIndex < Length) {
            // Move the first byte to the high bits of the unicode char.
            *buff = ((*_CS0) << 8) | (*(_CS0+1));
            _CS0+=2;
            byteIndex+=2;
            unicodeIndex++;
            buff++;
            ASSERT(byteIndex <= Length);
        }
        break;
    }
    default: {
return_empty_str:
        UName->Buffer = NULL;
        UName->MaximumLength =
        UName->Length = 0;
        return;
    }
    }
    UName->MaximumLength = (UName->Length = (((uint16)unicodeIndex)*sizeof(WCHAR))) + sizeof(WCHAR);
    UName->Buffer[unicodeIndex] = 0;
    if (valueCRC) {
        *valueCRC = UDFCrc(CS0 + 1, Length - 1, 0);
    }
} // end UDFDecompressUnicode()

/*
    This routine converts standard Unicode to compressed
 */
void
__fastcall
UDFCompressUnicode(
    IN PUNICODE_STRING UName,
    IN OUT uint8** _CS0,
    IN OUT PSIZE_T Length
    )
{
    uint8* CS0;
    uint8 compID;
    uint16 unicodeIndex;
    uint32 i, len;
    PWCHAR Buff;

    len = (UName->Length) / sizeof(WCHAR);
    compID = (!len) ? 0 : UDF_COMP_ID_8;
    // check for uncompressable characters
    Buff = UName->Buffer;
    for(i=0; i<len; i++, Buff++) {
        if ((*Buff) & 0xff00) {
            compID = UDF_COMP_ID_16;
            break;
        }
    }

    CS0 = (uint8*)MyAllocatePool__(NonPagedPool, *Length = (((compID==UDF_COMP_ID_8) ? 1 : 2)*len + 1) );
    if (!CS0) return;

    CS0[0] = compID;
    *_CS0 = CS0;
    // init loop
    CS0++;
    unicodeIndex = 0;
    Buff = UName->Buffer;
    if (compID == UDF_COMP_ID_16) {
        // Loop through all the bytes.
        while (unicodeIndex < len) {
            // Move the 2nd byte to the low bits of the compressed unicode char.
            *CS0 = (uint8)((*Buff) >> 8);
            CS0++;
            *CS0 = (uint8)(*Buff);
            CS0++;
            Buff++;
            unicodeIndex++;
        }
    } else {
        // Loop through all the bytes.
        while (unicodeIndex < len) {
            *CS0 = (uint8)(*Buff);
            CS0++;
            Buff++;
            unicodeIndex++;
        }
    }
} // end UDFCompressUnicode()

/*
NTSTATUS UDFFindFile__(IN PVCB Vcb,
                                IN BOOLEAN IgnoreCase,
                                IN PUNICODE_STRING Name,
                                IN PUDF_FILE_INFO DirInfo)
          see 'Udf_info.h'
*/

/*
    This routine reads (Extended)FileEntry according to FileDesc
 */
NTSTATUS
UDFReadFileEntry(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN long_ad* Icb,
    IN OUT PFILE_ENTRY FileEntry, // here we can also get ExtendedFileEntry
    IN OUT uint16* Ident
    )
{
    NTSTATUS status;

    if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, (int8*)FileEntry,
                         UDFPartLbaToPhys(Vcb,&(Icb->extLocation)),
                         Icb->extLocation.logicalBlockNum,
                         Ident))) return status;
    if ((FileEntry->descTag.tagIdent != TID_FILE_ENTRY) &&
       (FileEntry->descTag.tagIdent != TID_EXTENDED_FILE_ENTRY)) {
        UDFPrint(("  Not a FileEntry (lbn=%x, tag=%x)\n", Icb->extLocation.logicalBlockNum, FileEntry->descTag.tagIdent));
        return STATUS_FILE_CORRUPT_ERROR;
    }
    return STATUS_SUCCESS;
} // UDFReadFileEntry()

/*
    This routine initializes Tag structure. It must be called after all
    manual settings to generate valid CRC & Checksum
 */
void
UDFSetUpTag(
    IN PVCB Vcb,
    IN tag* Tag,
    IN uint16 DataLen,  // total length of descriptor _including_ icbTag
    IN uint32 TagLoc,
    IN uint16 skip)
{
    uint32 i;
    int8* tb;

    AdPrint(("UDF: SetTag Loc=%x(%x), tagIdent=%x, skip=%x\n", TagLoc, Tag->tagLocation, Tag->tagIdent, skip));

    AdPrint(
        ("Vcb->NSRDesc=%x, DataLen=%x, sizeof(tag)=%x descCRCLength=%x\n", Vcb->NSRDesc, DataLen, sizeof(tag),
         Tag->descCRCLength));
    if (DataLen)
        DataLen -= sizeof(tag); // Don't do this here (not all DataLen include the tag)
    //    int8* Data = ((int8*)Tag) + sizeof(tag);
    // Ecma-167 states, that all implementations
    // shall set this field to '3' even if
    // disc contains descriptors recorded with
    // value '2'
    // But we should ignore this to make happy othe UDF implementations :(
    Tag->descVersion = (Vcb->NSRDesc & VRS_NSR03_FOUND) ? 3 : 2;
    Tag->tagLocation = TagLoc;
    Tag->tagSerialNum = (uint16)(Vcb->SerialNumber);
    Tag->descCRCLength = DataLen;
    Tag->descCRC = UDFCrc((uint8*)(Tag + 1 + skip), DataLen - skip, (skip ? Tag->descCRC : 0));
    Tag->tagChecksum = 0;
    tb = ((int8*)Tag);
    for (i=0; i<sizeof(tag); i++,tb++)
        Tag->tagChecksum += (i!=4) ? (*tb) : 0;
    AdPrint(
        ("UDF tag: descVersion=%x tagSerialNum=%x descCRCLength=%x descCRC=%x tagChecksum=%x\n", Tag->descVersion,
         Tag->tagSerialNum, Tag->descCRCLength, Tag->descCRC, Tag->tagChecksum));
} // end UDFSetUpTag()

/*
    This routine builds FileEntry & associated AllocDescs for specified
    extent.
 */
NTSTATUS
UDFBuildFileEntry(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum,
    IN uint16 AllocMode, // short/long/ext/in-icb
    IN uint32 ExtAttrSz,
    IN BOOLEAN Extended
    )
{
    PFILE_ENTRY FileEntry;
    NTSTATUS status;
    EntityID* eID;
    uint32 l;
    EXTENT_INFO _FEExtInfo;
    uint16* lcp;

    ASSERT(PartNum < Vcb->PartitionMaps);
    ASSERT(!ExtAttrSz);
    // calculate the length required
    l = (Extended ? sizeof(EXTENDED_FILE_ENTRY) : sizeof(FILE_ENTRY)) + ExtAttrSz;
    if (l > Vcb->SectorSize) return STATUS_INVALID_PARAMETER;
    // allocate block for FE
    if (!NT_SUCCESS(status = UDFAllocateFESpace(IrpContext, Vcb, DirInfo, PartNum, &_FEExtInfo, l) ))
        return status;
    // Allocate buffer for full sector to avoid stale in-ICB data
    // when FE block is reused after FreeFESpace
    uint32 feAllocLen = max(l, Vcb->SectorSize);
    // remember FE location for future hard link creation
#ifdef UDF_DBG
    // Must acquire DlocResource before calling UDFFindDloc to prevent race conditions
    UDFAcquireResourceExclusive(&Vcb->DlocResource, TRUE);
    ASSERT(UDFFindDloc(Vcb, _FEExtInfo.Mapping[0].extLocation) == (-1));
    UDFReleaseResource(&Vcb->DlocResource);
#endif
    if (!NT_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, _FEExtInfo.Mapping[0].extLocation))) {
        ASSERT(status != STATUS_SHARING_PAUSED);
        UDFFreeFESpace(Vcb, DirInfo, &_FEExtInfo); // free
        MyFreePool__(_FEExtInfo.Mapping);
        return status;
    }
    FileEntry = (PFILE_ENTRY)MyAllocatePoolTag__(NonPagedPool, feAllocLen, MEM_FE_TAG);
    if (!FileEntry) {
        UDFRemoveDloc(Vcb, FileInfo->Dloc);
        FileInfo->Dloc = NULL;
        UDFFreeFESpace(Vcb, DirInfo, &_FEExtInfo); // free
        MyFreePool__(_FEExtInfo.Mapping);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    FileInfo->Dloc->FELoc = _FEExtInfo;

    RtlZeroMemory((int8*)FileEntry, feAllocLen);
    // set up in-memory FE structure
    FileEntry->icbTag.flags = AllocMode;
    FileEntry->icbTag.fileType = UDF_FILE_TYPE_REGULAR;
    FileEntry->icbTag.numEntries = 1;
//    if (DirInfo && DirInfo->Dloc && DirInfo->Dloc
    FileEntry->icbTag.strategyType = 4;
//    FileEntry->icbTag.strategyParameter = 0;
    FileEntry->descTag.tagIdent = Extended ? TID_EXTENDED_FILE_ENTRY : TID_FILE_ENTRY;
    FileEntry->descTag.tagLocation = UDFPhysLbaToPart(Vcb, PartNum, _FEExtInfo.Mapping[0].extLocation);
    FileEntry->uid = -1;
    FileEntry->gid = -1;

    if (Extended) {
        eID = &(((PEXTENDED_FILE_ENTRY)FileEntry)->impIdent);
        lcp = &(((PEXTENDED_FILE_ENTRY)FileEntry)->fileLinkCount);
        ((PEXTENDED_FILE_ENTRY)FileEntry)->checkpoint = 1;
    } else {
        eID = &(FileEntry->impIdent);
        lcp = &(FileEntry->fileLinkCount);
        ((PFILE_ENTRY)FileEntry)->checkpoint = 1;
    }

    UDFSetEntityID_imp(eID, UDF_ID_DEVELOPER);

    *lcp = 0;

    FileInfo->Dloc->FileEntry = (tag*)FileEntry;
    FileInfo->Dloc->FileEntryLen = l;

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

    return STATUS_SUCCESS;
} // end UDFBuildFileEntry()

/*
    This routine builds ExtentInfo for specified (Ex)FileEntry & associated
    AllocDescs
 */
NTSTATUS
UDFLoadExtInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_ENTRY fe,
    IN PLONG_AD fe_loc,
    IN OUT PEXTENT_INFO FExtInfo,  // user data
    IN OUT PEXTENT_INFO AExtInfo   // alloc descs
    )
{
    EXTENT_AD TmpExt;

    UDFPrint(("  UDFLoadExtInfo:\n"));
    FExtInfo->Mapping = UDFReadMappingFromXEntry(IrpContext, Vcb, fe_loc->extLocation.partitionReferenceNum,
                                       (tag*)fe, &FExtInfo->Offset, AExtInfo);
    if (!(FExtInfo->Mapping)) {
        if (!(FExtInfo->Offset))
            return STATUS_UNSUCCESSFUL;
        TmpExt.extLength = fe_loc->extLength;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &(fe_loc->extLocation));
        if (TmpExt.extLocation == LBA_OUT_OF_EXTENT)
            return STATUS_FILE_CORRUPT_ERROR;
        FExtInfo->Mapping = UDFExtentToMapping(&TmpExt);
    }
    if (fe->descTag.tagIdent == TID_FILE_ENTRY) {
//        UDFPrint(("Standard FileEntry\n"));
        FExtInfo->Length = fe->informationLength;
    } else /*if (fe->descTag.tagIdent == TID_EXTENDED_FILE_ENTRY) */ {
        FExtInfo->Length = ((PEXTENDED_FILE_ENTRY)fe)->informationLength;
    }
    UDFPrint(("  FExtInfo->Length %x\n", FExtInfo->Length));
    ASSERT(FExtInfo->Length <= UDFGetExtentLength(FExtInfo->Mapping));
    FExtInfo->Modified = FALSE;

    return STATUS_SUCCESS;
} // end UDFLoadExtInfo()

/*
    This routine builds FileIdent for specified FileEntry.
    We shall call UDFSetUpTag after all other initializations
    This structure is a precise copy of on-disk FileIdent
    structure. All modifications of it (including memory block
    size) are reflected on Directory extent. This, allocation of
    too long block (without changes in ImpUseLen) will lead to
    unreadable Directory
 */
NTSTATUS
UDFBuildFileIdent(
    IN PVCB Vcb,
    IN PUNICODE_STRING fn,
    IN PLONG_AD FileEntryIcb,       // virtual address of FileEntry
    IN uint32 ImpUseLen,
    OUT PFILE_IDENT_DESC* _FileId,
    OUT uint32* FileIdLen
    )
{
    PFILE_IDENT_DESC FileId;
    uint8* CS0;
    SIZE_T Nlen;
    uint32 l;
    // prepare filename
    UDFCompressUnicode(fn, &CS0, &Nlen);
    if (!CS0) return STATUS_INSUFFICIENT_RESOURCES;
    if (Nlen < 2) {
        Nlen = 0;
    } else
    if (Nlen > UDF_NAME_LEN) {
        if (CS0) MyFreePool__(CS0);
        return STATUS_OBJECT_NAME_INVALID;
    }
    // allocate memory for FI
    l = (sizeof(FILE_IDENT_DESC) + Nlen + ImpUseLen + 3) & ~((uint32)3);
    FileId = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, l, MEM_FID_TAG);
    if (!FileId) {
        if (CS0) MyFreePool__(CS0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // fill FI structure
    RtlZeroMemory( (int8*)FileId, l);
    RtlCopyMemory( ((int8*)(FileId+1))+ImpUseLen, CS0, Nlen);
    FileId->descTag.tagIdent = TID_FILE_IDENT_DESC;
    FileId->fileVersionNum = 1;
    FileId->lengthOfImpUse = (uint16)ImpUseLen;
    FileId->lengthFileIdent = (uint8)Nlen;
    FileId->icb = *FileEntryIcb;
    *_FileId = FileId;
    *FileIdLen = l;

    if (CS0) MyFreePool__(CS0);
    return STATUS_SUCCESS;
} // end UDFBuildFileIdent()

static
void
UDFAddToStreamOwnerSize(
    IN PUDF_FILE_INFO StreamInfo,
    IN int64 Delta
)
{
    if (!Delta || 
        !UDFIsAStream(StreamInfo) || 
        !StreamInfo->ParentFile->ParentFile
     )
        return;

    PUDF_DATALOC_INFO OwnerDloc = StreamInfo->ParentFile->ParentFile->Dloc;
    if (OwnerDloc && OwnerDloc->FileEntry &&
        (OwnerDloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY))
    {
        ((PEXTENDED_FILE_ENTRY)(OwnerDloc->FileEntry))->objectSize += Delta;
        OwnerDloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    }
}

/*
    This routine sets informationLength field in (Ext)FileEntry
 */
void
UDFSetFileSize(
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Size
    )
{
    int64 StreamsSize;
    uint16 Ident;
//    PDIR_INDEX_ITEM DirIndex;

    ValidateFileInfo(FileInfo);
    AdPrint(("UDFSetFileSize: %I64x, FI %x\n", Size, FileInfo));

    //AdPrint(("  Dloc %x\n", FileInfo->Dloc));
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    //AdPrint(("  FileEntry %x\n", FileInfo->Dloc->FileEntry));
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    //AdPrint(("  Ident %x\n", Ident));
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        //AdPrint(("  fe %x\n", fe));
        fe->informationLength = Size;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        //AdPrint(("  ext-fe %x\n", fe));
        UDFPrint(("informationLength %x objectSize %x\n", Size, fe->objectSize));
        StreamsSize = (fe->objectSize > fe->informationLength) ?
                            (fe->objectSize - fe->informationLength) : 0;
        UDFAddToStreamOwnerSize(FileInfo, Size - fe->informationLength);
        fe->informationLength = Size;
        fe->objectSize = fe->informationLength + StreamsSize;
    }
/*    if (DirIndex = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index) ) {
        DirIndex->FileSize = Size;
    }*/
    //AdPrint(("UDFSetFileSize: ok\n"));
    return;
} // end UDFSetFileSize()

void
UDFSetFileSizeInDirNdx(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64* ASize
    )
{
    uint16 Ident;
    PDIR_INDEX_ITEM DirIndex;

    ValidateFileInfo(FileInfo);
    if (ASize) {
        AdPrint(("UDFSetFileSizeInDirNdx: %I64x\n", *ASize));
    } else {
        AdPrint(("UDFSetFileSizeInDirNdx: sync\n"));
    }

    DirIndex = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index);
    if (!DirIndex)
       return;

    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        DirIndex->FileSize = fe->informationLength;
        if (ASize) {
            DirIndex->AllocationSize = *ASize;
//        } else {
//            DirIndex->AllocationSize = (fe->informationLength + Vcb->LBlockSize - 1) & ~(Vcb->LBlockSize - 1);
        }
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        DirIndex->FileSize = fe->informationLength;
        if (ASize) {
            DirIndex->AllocationSize = *ASize;
//        } else {
//            DirIndex->AllocationSize = (fe->informationLength + Vcb->LBlockSize - 1) & ~(Vcb->LBlockSize - 1);
        }
    }
    return;
} // end UDFSetFileSizeInDirNdx()

/*
    This routine gets informationLength field in (Ext)FileEntry
 */
int64
UDFGetFileSize(
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        return fe->informationLength;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        return fe->informationLength;
    }
    return (-1);
} // end UDFGetFileSize()

int64
UDFGetFileSizeFromDirNdx(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    PDIR_INDEX_ITEM DirIndex;

    ValidateFileInfo(FileInfo);

    DirIndex = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index);
    if (!DirIndex)
       return -1;

    return DirIndex->FileSize;
} // end UDFGetFileSizeFromDirNdx()

/*
    This routine sets lengthAllocDesc field in (Ext)FileEntry
 */
/* Length of the allocation descriptors stored in the FE block itself. When
 * the sequence continues in an Allocation Extent Descriptor, the FE block
 * ends with padding that is too short for another descriptor. Keep it out
 * of L_AD, which must be a whole number of descriptors. */
static uint32
UDFInFEAllocDescLen(
    IN PUDF_FILE_INFO FileInfo,
    IN uint16 IcbFlags
    )
{
    PUDF_DATALOC_INFO Dloc = FileInfo->Dloc;
    uint32 Space, AdSize;

    if (!Dloc->AllocLoc.Length)
        return (uint32)(Dloc->DataLoc.Length);
    Space = Dloc->AllocLoc.Mapping[0].extLength - Dloc->AllocLoc.Offset;
    if ((uint64)Dloc->AllocLoc.Length <= Space)
        return (uint32)(Dloc->AllocLoc.Length);
    switch (IcbFlags & ICB_FLAG_ALLOC_MASK) {
    case ICB_FLAG_AD_SHORT: AdSize = sizeof(SHORT_AD); break;
    case ICB_FLAG_AD_LONG:  AdSize = sizeof(LONG_AD); break;
    case ICB_FLAG_AD_EXTENDED: AdSize = sizeof(EXT_AD); break;
    default: return Space;
    }
    return Space - (Space % AdSize);
}

void
UDFSetAllocDescLen(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);
    UNREFERENCED_PARAMETER(Vcb);

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->lengthAllocDescs = UDFInFEAllocDescLen(FileInfo, fe->icbTag.flags);
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->lengthAllocDescs = UDFInFEAllocDescLen(FileInfo, fe->icbTag.flags);
    }
} // end UDFSetAllocDescLen()

/*
    This routine changes fileLinkCount field in (Ext)FileEntry
 */
void
UDFChangeFileLinkCount(
    IN PUDF_FILE_INFO FileInfo,
    IN BOOLEAN Increase
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if (Increase) {
            fe->fileLinkCount++;
        } else {
            fe->fileLinkCount--;
        }
        if (fe->fileLinkCount & 0x8000)
            fe->fileLinkCount = 0xffff;
        return;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if (Increase) {
            fe->fileLinkCount++;
        } else {
            fe->fileLinkCount--;
        }
        if (fe->fileLinkCount & 0x8000)
            fe->fileLinkCount = 0xffff;
        return;
    }
    return;
} // end UDFChangeFileLinkCount()

/*
    This routine gets fileLinkCount field from (Ext)FileEntry
 */
uint16
UDFGetFileLinkCount(
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    if (!FileInfo->Dloc->FileEntry)
        return 1;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        return ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        return ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount;
    }
    return UDF_INVALID_LINK_COUNT;
} // end UDFGetFileLinkCount()

uint16
UDFGetFileNameLinkCount(
    IN PUDF_FILE_INFO FileInfo
)
{
    uint16 LinkCount;
    
    LinkCount = UDFGetFileLinkCount(FileInfo);

    if (LinkCount && (LinkCount != UDF_INVALID_LINK_COUNT) &&
        !UDFIsAStreamDir(FileInfo) && 
        UDFHasAStreamDir(FileInfo) &&
        !UDFIsSDirDeleted(FileInfo->Dloc->SDirInfo)
      ) {
        LinkCount--;
    }

    return LinkCount;
}

#ifdef UDF_CHECK_UTIL
/*
    This routine sets fileLinkCount field in (Ext)FileEntry
 */
void
UDFSetFileLinkCount(
    IN PUDF_FILE_INFO FileInfo,
    uint16 LinkCount
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    if (!FileInfo->Dloc->FileEntry)
        return;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount = LinkCount;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount = LinkCount;
    }
    return;
} // end UDFSetFileLinkCount()
#endif //UDF_CHECK_UTIL

/*
    This routine gets lengthExtendedAttr field in (Ext)FileEntry
 */
uint32
UDFGetFileEALength(
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    if (!FileInfo->Dloc->FileEntry)
        return 1;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;

    if (Ident == TID_FILE_ENTRY) {
        return ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->lengthExtendedAttr;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        return ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->lengthExtendedAttr;
    }
    return 0;
} // end UDFGetFileEALength()

/*
    This routine sets UniqueID field in (Ext)FileEntry
 */
NTSTATUS
UDFAllocateFileUID(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PULONGLONG UniqueId
    )
{
    ULONGLONG next, after, oldEnd, newEnd;
    NTSTATUS status;

    *UniqueId = 0;
    /* Make the volume's OPEN checkpoint durable before an ID can reach an
     * FE or FID.  The bitmap resource also serializes the 64-bit counter on
     * i386 and all LVID writers, including the clean-close path. */
    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status))
        return status;

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    next = Vcb->NextUniqueId;
    if ((uint32)next < 16)
        next += 16 - (uint32)next;
    if (next == ~(ULONGLONG)0) {
        status = STATUS_INTEGER_OVERFLOW;
        goto release;
    }
    after = next + 1;
    if ((uint32)after == 0)
        after += 16;

    if (Vcb->LvidJournalState == 1 &&
        after > Vcb->UniqueIdReservedEnd) {
        oldEnd = Vcb->UniqueIdReservedEnd;
        newEnd = after > ~(ULONGLONG)0 - 4096 ?
                 ~(ULONGLONG)0 : after + 4096;
        Vcb->UniqueIdReservedEnd = newEnd;
        status = UDFUpdateLogicalVolInt(IrpContext, Vcb, FALSE);
        if (NT_SUCCESS(status))
            status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (!NT_SUCCESS(status)) {
            Vcb->UniqueIdReservedEnd = oldEnd;
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            goto release;
        }
    }
    Vcb->NextUniqueId = after;
    *UniqueId = next;
    status = STATUS_SUCCESS;

release:
    UDFReleaseResource(&(Vcb->BitMapResource1));
    return status;
}

void
UDFSetFileUID(
    IN PUDF_FILE_INFO FileInfo,
    IN ULONGLONG UniqueId
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

/*    UID = FileInfo->Dloc->FELoc.Mapping[0].extLocation |
          ( FileInfo->ParentFile ? (((int64)(FileInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation)) << 32) : 0);*/

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->uniqueID = UniqueId;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->uniqueID = UniqueId;
    }
    if (FileInfo->FileIdent)
        ((struct FidADImpUse*)&(FileInfo->FileIdent->icb.impUse))->uniqueID = (uint32)UniqueId;
    return;
} // end UDFSetFileUID()

/*
    This routine gets UniqueID field in (Ext)FileEntry
 */
__inline
int64
UDFGetFileUID_(
    IN tag* FileEntry
    )
{
    uint16 Ident;

    Ident = FileEntry->tagIdent;
    if (Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileEntry);
        return fe->uniqueID;
    } else if (Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileEntry);
        return fe->uniqueID;
    }
    return (-1);
} // end UDFGetFileUID()

int64
UDFGetFileUID(
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    return UDFGetFileUID_(FileInfo->Dloc->FileEntry);
} // end UDFGetFileUID()

void
UDFChangeFileCounter(
    IN PVCB Vcb,
    IN BOOLEAN FileCounter,
    IN BOOLEAN Increase
    )
{
    uint32* counter;

    counter = FileCounter ?
        &(Vcb->numFiles) :
        &(Vcb->numDirs);
    if (*counter == (ULONG)-1)
        return;
    if (Increase) {
        InterlockedIncrement((int32*)counter);
    } else {
        InterlockedDecrement((int32*)counter);
    }

} // end UDFChangeFileCounter()

void
UDFSetEntityID_imp_(
    IN EntityID* eID,
    IN uint8* Str,
    IN uint32 Len
    )
{
    impIdentSuffix* iis;

    RtlZeroMemory(eID, sizeof(EntityID));
    RtlCopyMemory( (int8*)&(eID->ident), Str, min(Len, sizeof(eID->ident)) );

    iis = (impIdentSuffix*)&(eID->identSuffix);
    iis->OSClass = UDF_OS_CLASS_WINNT;
    iis->OSIdent = UDF_OS_ID_WINNT;

} // end UDFSetEntityID_imp_()

void
UDFReadEntityID_Domain(
    PVCB Vcb,
    EntityID* eID
    )
{
    domainIdentSuffix* dis;

    dis = (domainIdentSuffix*)&(eID->identSuffix);

    UDFPrint(("UDF: Entity Id:\n"));
    UDFPrint(("flags: %x\n", eID->flags));
    UDFPrint(("ident[0]: %x\n", eID->ident[0]));
    UDFPrint(("ident[1]: %x\n", eID->ident[1]));
    UDFPrint(("ident[2]: %x\n", eID->ident[2]));
    UDFPrint(("ident[3]: %x\n", eID->ident[3]));
    UDFPrint(("UDF: Entity Id Domain:\n"));
    // Get current UDF revision
    Vcb->UdfRevision = max(dis->currentRev, Vcb->UdfRevision);
    UDFPrint(("Effective Revision: %x\n", Vcb->UdfRevision));
    if ((Vcb->NSRDesc & VRS_NSR02_FOUND) && (Vcb->NSRDesc & VRS_NSR03_FOUND))
    {
        if (Vcb->UdfRevision >= 0x200)
        {
            Vcb->NSRDesc = VRS_NSR03_FOUND;
        }
        else
        {
            Vcb->NSRDesc = VRS_NSR02_FOUND;
        }
    }

    // Get Read-Only flags

    if (FlagOn(dis->flags, ENTITYID_FLAGS_HARD_RO | ENTITYID_FLAGS_SOFT_RO)) {

        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    }

} // end UDFReadEntityID_Domain()

static
NTSTATUS
UDFWriteFileData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
);

/*
    This routine writes data to file & increases it if necessary.
    In case of increasing AllocDescs will be rebuilt & flushed to disc
    (via driver's cache, of cource). Free space map will be updated only
    durring global media flush.
 */
NTSTATUS
UDFWriteFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
    )
{
    int64 t, elen;
    NTSTATUS status;
    int8* OldInIcb = NULL;
    ValidateFileInfo(FileInfo);
    SIZE_T _WrittenBytes;
    PUDF_DATALOC_INFO Dloc;
    // unwind staff
    BOOLEAN WasInIcb = FALSE;
    uint64 OldLen;

//    ASSERT(FileInfo->RefCount >= 1);

    Dloc = FileInfo->Dloc;
    ASSERT(Dloc->FELoc.Mapping[0].extLocation);
    uint32 PartNum = UDFGetRefPartNumByPhysLba(Vcb, Dloc->FELoc.Mapping[0].extLocation);
    uint32 DataPartNum = PartNum;
    if (Vcb->MetadataFileInfo && PartNum == Vcb->MetadataRef &&
        !UDFIsADirectory(FileInfo))
        DataPartNum = Vcb->MetadataPhysRef;
    (*WrittenBytes) = 0;

    {
        BOOLEAN handled;
        status = UDFTryDirectoryAppend(IrpContext, Vcb, FileInfo, Offset,
                                      Length, Direct, Buffer, WrittenBytes, &handled);
        if (!NT_SUCCESS(status) || handled)
            return status;
    }

    AdPrint(("UDFWriteFile__ FE %x, FileInfo %x, ExtInfo %x, Mapping %x\n",
        Dloc->FELoc.Mapping[0].extLocation, FileInfo, &(Dloc->DataLoc), Dloc->DataLoc.Mapping));

    t = Offset + Length;
//    UDFUpdateModifyTime(Vcb, FileInfo);
    if (t <= Dloc->DataLoc.Length) {
        // write Alloc-Rec area
        ExtPrint(("  WAlloc-Rec: %I64x <= %I64x\n", t, Dloc->DataLoc.Length));
        status = UDFWriteFileData(IrpContext, Vcb, FileInfo, Offset, Length, Direct, Buffer, WrittenBytes);
        return status;
    }
    elen = UDFGetExtentLength(Dloc->DataLoc.Mapping);
    ExtPrint(("  DataLoc Offs %x, Len %I64x\n",
        Dloc->DataLoc.Offset, Dloc->DataLoc.Length));
    if (t <= (elen - Dloc->DataLoc.Offset)) {
        // write Alloc-Not-Rec area
        ExtPrint(("  WAlloc-Not-Rec: %I64x <= %I64x (%I64x - %I64x)\n",
            t, elen - Dloc->DataLoc.Offset - Dloc->DataLoc.Length,
            elen - Dloc->DataLoc.Offset,
            Dloc->DataLoc.Length));
        UDFSetFileSize(FileInfo, t);
        Dloc->DataLoc.Modified = TRUE;
        Dloc->DataLoc.Length = t;
        return UDFWriteFileData(IrpContext, Vcb, FileInfo, Offset, Length, Direct, Buffer, WrittenBytes);
    }
    // We should not get here if Direct=TRUE
    if (Direct) return STATUS_INVALID_PARAMETER;
    OldLen = Dloc->DataLoc.Length;
    if (Dloc->DataLoc.Offset && Dloc->DataLoc.Length) {
        // read in-icb data. it'll be replaced after resize
        ExtPrint(("  read in-icb data\n"));
        OldInIcb = (int8*)MyAllocatePool__(NonPagedPool, (uint32)(Dloc->DataLoc.Length));
        if (!OldInIcb) return STATUS_INSUFFICIENT_RESOURCES;
        status = UDFReadFile__(IrpContext, Vcb, FileInfo, 0, (uint32)OldLen, FALSE, OldInIcb);
        if (!NT_SUCCESS(status)) {
            MyFreePool__(OldInIcb);
            return status;
        }
    }
    // init Alloc mode
    ExtPrint(("  init Alloc mode\n"));
    if ((((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |=
            (DataPartNum == PartNum) ? ICB_FLAG_AD_SHORT : ICB_FLAG_AD_LONG;
        WasInIcb = TRUE;
        // Clear embedded data flag since file is no longer in ICB mode
        // Fcb may be NULL during directory operations (FCB not yet created)
        if (FileInfo->Fcb) {
            FileInfo->Fcb->FcbState &= ~UDF_FCB_EMBEDDED_DATA;
        }
    }
    // increase extent
    ExtPrint(("  %s %s %s\n",
        UDFIsADirectory(FileInfo) ? "DIR" : "FILE",
        WasInIcb ? "In-Icb" : "",
        Vcb->LowFreeSpace ? "LowSpace" : ""));
    status = UDFResizeExtent(IrpContext, Vcb, DataPartNum, t, FALSE, &(Dloc->DataLoc));
    ExtPrint(("  DataLoc Offs %x, Len %I64x\n",
        Dloc->DataLoc.Offset, Dloc->DataLoc.Length));
    AdPrint(("UDFWriteFile__ (2) FileInfo %x, ExtInfo %x, Mapping %x\n", FileInfo, &(Dloc->DataLoc), Dloc->DataLoc.Mapping));
    if (!NT_SUCCESS(status)) {
        // rollback
        ExtPrint(("  err -> rollback\n"));
        if (WasInIcb) {
            // restore Alloc mode
            ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
            ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |= ICB_FLAG_AD_IN_ICB;
            if (Dloc->AllocLoc.Mapping) {
                MyFreePool__(Dloc->AllocLoc.Mapping);
                Dloc->AllocLoc.Mapping = NULL;
            }
        }
        if (OldInIcb) {
            UDFWriteFileData(IrpContext, Vcb, FileInfo, 0, (uint32)OldLen, FALSE, OldInIcb, &_WrittenBytes);
            MyFreePool__(OldInIcb);
        }
        if ((int64)OldLen != Dloc->DataLoc.Length) {
            // restore file size
            AdPrint(("  restore alloc\n"));
            FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
            UDFResizeExtent(IrpContext, Vcb, DataPartNum, OldLen, FALSE, &Dloc->DataLoc);
            FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_CUT_PREALLOCATED;
        }
        return status;
    }
    if (OldInIcb) {
        // replace data from ICB (if any) & free buffer
        ExtPrint(("  write old in-icd data\n"));
        status = UDFWriteFileData(IrpContext, Vcb, FileInfo, 0, (uint32)OldLen, FALSE, OldInIcb, &_WrittenBytes);
        MyFreePool__(OldInIcb);
        if (!NT_SUCCESS(status))
            return status;
    }
    // ufff...
    // & now we'll write out data to well prepared extent...
    // ... like all normal people do...
    ExtPrint(("  write user data\n"));
    if (!NT_SUCCESS(status = UDFWriteFileData(IrpContext, Vcb, FileInfo, Offset, Length, FALSE, Buffer, WrittenBytes)))
        return status;
    UDFSetFileSize(FileInfo, t);
    Dloc->DataLoc.Modified = TRUE;
    ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
    return STATUS_SUCCESS;
} // end UDFWriteFile__()

/* Keep the disk's prepared FREE generation visible across ordinary bitmap
 * writeback while a deleted file's FCB still pins its RAM allocation.  The
 * BitMapResource protects this list and every bitmap serialization below. */
NTSTATUS
UDFOverlayDeletePrefreePhysical(PVCB Vcb, int8* Unallocated,
                                int8* Freed, uint32 BitCount)
{
    PUDF_DELETE_PREFREE record;
    uint32 i, j;
    for (record = Vcb->DeletePrefreeList; record; record = record->Next) {
        for (i = 0; i < record->RangeCount; ++i) {
            PUDF_DELETE_PREFREE_RANGE range = &record->Ranges[i];
            if (range->Metadata) continue;
            if (range->FirstBit >= BitCount ||
                range->BitCount > BitCount - range->FirstBit)
                return STATUS_FILE_CORRUPT_ERROR;
            for (j = 0; j < range->BitCount; ++j) {
                if (Unallocated)
                    UDFSetFreeBit(Unallocated, range->FirstBit + j);
                if (Freed)
                    UDFSetFreeBit(Freed, range->FirstBit + j);
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
UDFOverlayDeletePrefreeMetadata(PVCB Vcb, int8* Bitmap,
                                uint32 BitCount)
{
    PUDF_DELETE_PREFREE record;
    uint32 i, j;
    for (record = Vcb->DeletePrefreeList; record; record = record->Next) {
        for (i = 0; i < record->RangeCount; ++i) {
            PUDF_DELETE_PREFREE_RANGE range = &record->Ranges[i];
            if (!range->Metadata) continue;
            if (range->FirstBit >= BitCount ||
                range->BitCount > BitCount - range->FirstBit)
                return STATUS_FILE_CORRUPT_ERROR;
            for (j = 0; j < range->BitCount; ++j)
                Bitmap[(range->FirstBit + j) >> 3] |=
                    (int8)(1U << ((range->FirstBit + j) & 7));
        }
    }
    return STATUS_SUCCESS;
}

static VOID
UDFDetachDeletePrefree(PVCB Vcb, PUDF_DATALOC_INFO Dloc)
{
    PUDF_DELETE_PREFREE* cursor = &Vcb->DeletePrefreeList;
    PUDF_DELETE_PREFREE record = Dloc->DeletePrefree;
    if (!record) return;
    while (*cursor && *cursor != record)
        cursor = &(*cursor)->Next;
    ASSERT(*cursor == record && Vcb->DeletePrefreeCount);
    if (*cursor == record) {
        *cursor = record->Next;
        Vcb->DeletePrefreeCount--;
    }
    Dloc->DeletePrefree = NULL;
    MyFreePool__(record);
}

VOID
UDFAbandonDeletePrefreeDloc(PVCB Vcb, PUDF_DATALOC_INFO Dloc)
{
    if (!Dloc || !Dloc->DeletePrefree) return;
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    if (Dloc->DeletePrefree) {
        /* An unexpected teardown must not leave a VCB list pointing at a
         * freed Dloc or resume allocation from an uncommitted generation. */
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        UDFDetachDeletePrefree(Vcb, Dloc);
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
}

VOID
UDFReleaseDeletePrefreeRecords(PVCB Vcb)
{
    PUDF_DELETE_PREFREE record, next;
    /* Mount can call VCB cleanup before the bitmap resource exists.  A
     * prefree record is created only after a successful writable mount. */
    if (!Vcb->DeletePrefreeList) return;
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    record = Vcb->DeletePrefreeList;
    Vcb->DeletePrefreeList = NULL;
    Vcb->DeletePrefreeCount = 0;
    while (record) {
        next = record->Next;
        if (record->Dloc && record->Dloc->DeletePrefree == record)
            record->Dloc->DeletePrefree = NULL;
        MyFreePool__(record);
        record = next;
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
}

static BOOLEAN
UDFDeleteRangeOverlapsMap(PEXTENT_MAP Map, uint32 Lba, uint32 Blocks)
{
    uint32 i;
    uint64 first = Lba, end = first + Blocks;
    for (i = 0; Map && i < 4096 &&
                (Map[i].extLength & UDF_EXTENT_LENGTH_MASK); ++i) {
        uint32 bytes = Map[i].extLength & UDF_EXTENT_LENGTH_MASK;
        uint64 mapFirst = Map[i].extLocation;
        uint64 mapEnd = mapFirst + ((uint64)bytes + 511) / 512;
        if ((Map[i].extLength >> 30) !=
                EXTENT_NOT_RECORDED_NOT_ALLOCATED &&
            first < mapEnd && mapFirst < end)
            return TRUE;
    }
    return i == 4096;
}

/* Only the exact dense, 512-byte, external-SBD case is prepared here. Other
 * layouts retain the older delete path and still fail closed at recovery if
 * a cut exposes an orphan; they are not claimed as crash-safe by this patch. */
static NTSTATUS
UDFBuildDeletePrefree(PVCB Vcb, PUDF_FILE_INFO FileInfo,
                      PUDF_DELETE_PREFREE* Prepared,
                      uint32* PhysicalBitmapLba,
                      uint32* MetadataBitmapLba)
{
    PUDF_DATALOC_INFO dloc = FileInfo->Dloc;
    PEXTENT_MAP map, alloc, feMap, bitmapMap;
    PUDF_DELETE_PREFREE record;
    uint32 i, count = 1, feLba, metadataLbn, partRoot, blocks;
    SIZE_T bytes;
    BOOLEAN embedded;

    *Prepared = NULL;
    if (Vcb->Cow || Vcb->CDR_Mode || Vcb->BitmapFcb || Vcb->SectorSize != 512 ||
        !Vcb->FSBM_Bitmap || !Vcb->FSBM_OldBitmap ||
        !Vcb->MetadataBitmap || !Vcb->MetadataFileInfo ||
        !Vcb->MetadataBitmapFileInfo ||
        !dloc || dloc->DeletePrefree ||
        !Vcb->PartitionMaps || !Vcb->FSBM_ByteCount ||
        Vcb->MetadataPhysRef != 0 ||
        Vcb->Partitions[0].PartitionType == UDF_METADATA_MAP25 ||
        Vcb->Partitions[0].UspaceBitmap == 0xFFFFFFFF ||
        Vcb->Partitions[0].UspaceBitmap >=
            Vcb->Partitions[0].PartitionLen)
        return STATUS_NOT_SUPPORTED;
    partRoot = Vcb->Partitions[0].PartitionRoot;
    feMap = dloc->FELoc.Mapping;
    if (!feMap || feMap[0].extLength != 512 ||
        (feMap[1].extLength & UDF_EXTENT_LENGTH_MASK))
        return STATUS_NOT_SUPPORTED;
    feLba = feMap[0].extLocation;
    embedded = dloc->DataLoc.Offset != 0;
    if (embedded) {
        /* In-ICB data occupies the FE sector itself.  Only its metadata
         * bitmap bit is released; it has no separate physical data blocks.
         * Reject a transition or extra allocation extent that this bounded
         * transaction cannot describe. */
        map = dloc->DataLoc.Mapping;
        if (!dloc->FileEntry || !map ||
            dloc->DataLoc.Offset != dloc->FileEntryLen ||
            dloc->DataLoc.Length < 0 ||
            (uint64)dloc->DataLoc.Offset +
                (uint64)dloc->DataLoc.Length > Vcb->SectorSize ||
            (((PFILE_ENTRY)dloc->FileEntry)->icbTag.flags &
                ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB ||
            dloc->AllocLoc.Mapping || dloc->AllocLoc.Length ||
            map[0].extLocation != feLba || map[0].extLength != 512 ||
            (map[1].extLength & UDF_EXTENT_LENGTH_MASK))
            return STATUS_NOT_SUPPORTED;
    }
    metadataLbn = UDFPhysLbaToMetadata(Vcb, feLba);
    if (metadataLbn == LBA_OUT_OF_EXTENT ||
        metadataLbn >= Vcb->MetadataBitCount ||
        UDFMetadataLbaToPhys(Vcb, metadataLbn, NULL) != feLba)
        return STATUS_NOT_SUPPORTED;

    bitmapMap = Vcb->MetadataBitmapFileInfo->Dloc->DataLoc.Mapping;
    if (Vcb->MetadataBitmapFileInfo->Dloc->DataLoc.Offset ||
        !bitmapMap || (bitmapMap[0].extLength >> 30) !=
            EXTENT_RECORDED_ALLOCATED ||
        (bitmapMap[0].extLength & UDF_EXTENT_LENGTH_MASK) <
            sizeof(SPACE_BITMAP_DESC) + Vcb->MetadataBitmapByteCount ||
        (bitmapMap[1].extLength & UDF_EXTENT_LENGTH_MASK))
        return STATUS_NOT_SUPPORTED;
    *PhysicalBitmapLba = partRoot + Vcb->Partitions[0].UspaceBitmap;
    if (Vcb->FSBM_ByteCount > Vcb->Partitions[0].PartitionLen * 512ULL ||
        (uint64)Vcb->Partitions[0].UspaceBitmap +
            ((uint64)sizeof(SPACE_BITMAP_DESC) +
             Vcb->FSBM_ByteCount + 511) / 512 >
            Vcb->Partitions[0].PartitionLen)
        return STATUS_NOT_SUPPORTED;
    *MetadataBitmapLba = bitmapMap[0].extLocation;
    if (UDFPhysLbaToMetadata(Vcb, *MetadataBitmapLba) != LBA_OUT_OF_EXTENT)
        return STATUS_NOT_SUPPORTED;

    alloc = dloc->AllocLoc.Mapping;
    for (i = 0; alloc && i < 4096 &&
                (alloc[i].extLength & UDF_EXTENT_LENGTH_MASK); ++i) {
        if ((alloc[i].extLength >> 30) !=
                EXTENT_NOT_RECORDED_NOT_ALLOCATED &&
            (alloc[i].extLocation != feLba ||
             (alloc[i].extLength & UDF_EXTENT_LENGTH_MASK) > 512))
            return STATUS_NOT_SUPPORTED;
    }
    if (i == 4096) return STATUS_NOT_SUPPORTED;
    map = embedded ? NULL : dloc->DataLoc.Mapping;
    for (i = 0; map && i < 4096 &&
                (map[i].extLength & UDF_EXTENT_LENGTH_MASK); ++i) {
        uint32 length = map[i].extLength & UDF_EXTENT_LENGTH_MASK;
        uint32 kind = map[i].extLength >> 30;
        if (kind == EXTENT_NOT_RECORDED_NOT_ALLOCATED) continue;
        if (kind != EXTENT_RECORDED_ALLOCATED &&
            kind != EXTENT_NOT_RECORDED_ALLOCATED)
            return STATUS_NOT_SUPPORTED;
        blocks = (length + 511) / 512;
        if (!blocks || !map[i].extLocation ||
            map[i].extLocation < partRoot ||
            map[i].extLocation - partRoot >= Vcb->FSBM_BitCount ||
            blocks > Vcb->FSBM_BitCount -
                     (map[i].extLocation - partRoot) ||
            UDFDeleteRangeOverlapsMap(Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping,
                                      map[i].extLocation, blocks) ||
            (Vcb->MetadataMirrorFileInfo &&
             UDFDeleteRangeOverlapsMap(
                 Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping,
                 map[i].extLocation, blocks)))
            return STATUS_NOT_SUPPORTED;
        count++;
    }
    if (i == 4096) return STATUS_NOT_SUPPORTED;
    bytes = sizeof(UDF_DELETE_PREFREE) +
            (SIZE_T)(count - 1) * sizeof(UDF_DELETE_PREFREE_RANGE);
    record = (PUDF_DELETE_PREFREE)MyAllocatePool__(NonPagedPool, bytes);
    if (!record) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(record, bytes);
    record->Dloc = dloc;
    record->Ranges[0].FirstBit = metadataLbn;
    record->Ranges[0].BitCount = 1;
    record->Ranges[0].Metadata = TRUE;
    record->RangeCount = 1;
    for (i = 0; map && (map[i].extLength & UDF_EXTENT_LENGTH_MASK); ++i) {
        uint32 length = map[i].extLength & UDF_EXTENT_LENGTH_MASK;
        if ((map[i].extLength >> 30) ==
                EXTENT_NOT_RECORDED_NOT_ALLOCATED) continue;
        record->Ranges[record->RangeCount].FirstBit =
            map[i].extLocation - partRoot;
        record->Ranges[record->RangeCount].BitCount =
            (length + 511) / 512;
        record->RangeCount++;
    }
    ASSERT(record->RangeCount == count);
    *Prepared = record;
    return STATUS_SUCCESS;
}

static NTSTATUS
UDFValidateDeletePrefreeBitmaps(PIRP_CONTEXT IrpContext, PVCB Vcb,
                                uint32 PhysicalLba, uint32 MetadataLba,
                                int8* Sector)
{
    PSPACE_BITMAP_DESC desc;
    uint16 ident;
    NTSTATUS status;
    status = UDFReadTagged(IrpContext, Vcb, Sector, PhysicalLba,
                           Vcb->Partitions[0].UspaceBitmap, &ident);
    if (!NT_SUCCESS(status)) return status;
    desc = (PSPACE_BITMAP_DESC)Sector;
    if (ident != TID_SPACE_BITMAP_DESC ||
        desc->descTag.descCRCLength >
            sizeof(SPACE_BITMAP_DESC) - sizeof(tag) ||
        desc->numOfBits != Vcb->FSBM_BitCount ||
        desc->numOfBytes < Vcb->FSBM_ByteCount)
        return STATUS_NOT_SUPPORTED;
    status = UDFReadTagged(IrpContext, Vcb, Sector, MetadataLba,
                           UDFPhysLbaToPart(Vcb, Vcb->MetadataPhysRef,
                                            MetadataLba), &ident);
    if (!NT_SUCCESS(status)) return status;
    desc = (PSPACE_BITMAP_DESC)Sector;
    if (ident != TID_SPACE_BITMAP_DESC ||
        desc->descTag.descCRCLength >
            sizeof(SPACE_BITMAP_DESC) - sizeof(tag) ||
        desc->numOfBits != Vcb->MetadataBitCount ||
        desc->numOfBytes < Vcb->MetadataBitmapByteCount)
        return STATUS_NOT_SUPPORTED;
    return STATUS_SUCCESS;
}

static NTSTATUS
UDFWriteDeletePrefreeBitmaps(PIRP_CONTEXT IrpContext, PVCB Vcb,
                             PUDF_DELETE_PREFREE Record,
                             uint32 PhysicalLba, uint32 MetadataLba,
                             int8* Sector, PBOOLEAN PossiblyModified)
{
    uint32 i;
    for (i = 0; i < Record->RangeCount; ++i) {
        PUDF_DELETE_PREFREE_RANGE range = &Record->Ranges[i];
        uint32 bit = range->FirstBit, left = range->BitCount;
        uint32 base = range->Metadata ? MetadataLba : PhysicalLba;
        while (left) {
            uint32 byteOffset = sizeof(SPACE_BITMAP_DESC) + (bit >> 3);
            uint32 sectorOffset = byteOffset % 512;
            uint32 bits = min(left, (512 - sectorOffset) * 8 - (bit & 7));
            uint32 j;
            SIZE_T written = 0;
            BOOLEAN changed = FALSE;
            NTSTATUS status = UDFReadSectors(IrpContext, Vcb, FALSE,
                                    base + byteOffset / 512, 1, FALSE, Sector);
            if (!NT_SUCCESS(status)) return status;
            for (j = 0; j < bits; ++j) {
                uint32 current = bit + j;
                uint8* byte = (uint8*)Sector +
                    (sizeof(SPACE_BITMAP_DESC) + (current >> 3)) % 512;
                uint8 mask = (uint8)(1U << (current & 7));
                /* The RAM allocator must still reserve every target bit.
                 * A freshly allocated block may have a durable FE before
                 * its bitmap generation is published. Accept an on-disk
                 * FREE bit only if this exact bit has an unflushed allocation
                 * recorded under BitMapResource1; unrelated dirty bits do
                 * not establish provenance. */
                if (range->Metadata) {
                    if (current >= Vcb->MetadataBitCount ||
                        !Vcb->MetadataBitmap || !Vcb->MetadataPendingAlloc ||
                        UDFGetFreeBit(Vcb->MetadataBitmap, current))
                        return STATUS_FILE_CORRUPT_ERROR;
                    if (*byte & mask) {
                        if (!(Vcb->MetadataPendingAlloc[current >> 3] & mask))
                            return STATUS_FILE_CORRUPT_ERROR;
                        continue;
                    }
                } else {
                    if (current >= Vcb->FSBM_BitCount ||
                        !Vcb->FSBM_Bitmap || !Vcb->FSBM_OldBitmap ||
                        UDFGetFreeBit(Vcb->FSBM_Bitmap, current))
                        return STATUS_FILE_CORRUPT_ERROR;
                    if (*byte & mask) {
                        if (!UDFGetFreeBit(Vcb->FSBM_OldBitmap, current))
                            return STATUS_FILE_CORRUPT_ERROR;
                        continue;
                    }
                }
                *byte |= mask;
                changed = TRUE;
            }
            if (changed) {
                *PossiblyModified = TRUE;
                status = UDFWriteSectors(IrpContext, Vcb, FALSE,
                                         base + byteOffset / 512, 1, FALSE,
                                         Sector, &written);
                if (!NT_SUCCESS(status)) return status;
                if (written != 512) return STATUS_IO_DEVICE_ERROR;
            }
            bit += bits;
            left -= bits;
        }
    }
    return STATUS_SUCCESS;
}

static VOID
UDFReleasePrefreedAllocation(PVCB Vcb, PUDF_FILE_INFO DirInfo,
                             PUDF_FILE_INFO FileInfo)
{
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    _SEH2_TRY {
        UDFFreeFileAllocation(Vcb, DirInfo, FileInfo);
        UDFDetachDeletePrefree(Vcb, FileInfo->Dloc);
    } _SEH2_FINALLY {
        if (_SEH2_AbnormalTermination())
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        UDFReleaseResource(&(Vcb->BitMapResource1));
    } _SEH2_END;
}

/* The directory transaction is deliberately narrower than the ordinary-file
 * one: its parent FE and child FID must be distinct, mirrored 512-byte
 * sectors.  Recovery must be able to identify either copy after a cut. */
static BOOLEAN
UDFDeleteHasIndependentMetadataMirror(PVCB Vcb, uint32 MainLba)
{
    PEXTENT_MAP mirror;
    uint32 lbn, mirrorLba, flags;
    SIZE_T available;

    if (!(Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) ||
        !Vcb->MetadataMirrorFileInfo ||
        !Vcb->MetadataMirrorFileInfo->Dloc ||
        !(mirror = Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping))
        return FALSE;
    lbn = UDFPhysLbaToMetadata(Vcb, MainLba);
    if (lbn == LBA_OUT_OF_EXTENT || lbn >= Vcb->MetadataBitCount ||
        UDFMetadataLbaToPhys(Vcb, lbn, NULL) != MainLba)
        return FALSE;
    mirrorLba = UDFExtentOffsetToLba(Vcb, mirror, ((int64)lbn) << 9,
                                    NULL, &available, &flags, NULL);
    return mirrorLba != LBA_OUT_OF_EXTENT && mirrorLba != MainLba &&
           flags == EXTENT_RECORDED_ALLOCATED && available >= 512;
}

static BOOLEAN
UDFDeleteParentHasOtherDirtyFid(PDIR_INDEX_HDR Index, uint_di Target)
{
    PDIR_INDEX_ITEM item;
    uint_di i;
    for (i = 2; (item = UDFDirIndex(Index, i)); ++i)
        if (i != Target && (item->FI_Flags & UDF_FI_FLAG_FI_MODIFIED))
            return TRUE;
    return FALSE;
}

/* A single empty, inline directory has only its FE metadata bit to release.
 * Keep that bit free on disk but reserved in RAM until the parent FE and FID
 * have both passed lower-device barriers.  On any uncertain write, retain
 * OPEN LVID and make this mount read-only; the bounded recovery graph decides
 * whether another mount can safely roll the transaction forward. */
static NTSTATUS
UDFTryPreparedEmptyDirectoryUnlink(PIRP_CONTEXT IrpContext, PVCB Vcb,
                                   PUDF_FILE_INFO FileInfo,
                                   PBOOLEAN Handled)
{
    PUDF_FILE_INFO parent;
    PUDF_DATALOC_INFO dloc;
    PDIR_INDEX_HDR parentIndex;
    PDIR_INDEX_ITEM entry;
    PUDF_DELETE_PREFREE record = NULL;
    int8 *sector = NULL, *parentBefore = NULL, *fidBefore = NULL;
    uint32 bitmapPhysical = 0, bitmapMetadata = 0;
    uint32 childPart, parentPart, fidLba, childFeLba, parentFeLba;
    uint32 parentFeFlags;
    uint8 oldFlags, oldCharacteristics;
    NTSTATUS status;
    BOOLEAN bitmapHeld = FALSE, attached = FALSE;
    BOOLEAN parentTouched = FALSE, fidTouched = FALSE;
    BOOLEAN uncertain = FALSE, committed = FALSE;
    FILE_ID removedId;

    *Handled = FALSE;
    if (!IrpContext || IrpContext->MajorFunction != IRP_MJ_CLEANUP ||
        Vcb->CDR_Mode || Vcb->SectorSize != 512 ||
        !Vcb->LVid ||
        Vcb->origIntegrityType != INTEGRITY_TYPE_CLOSE ||
        Vcb->VcbCondition != VcbMounted ||
        Vcb->FsDeviceType != FILE_DEVICE_DISK_FILE_SYSTEM ||
        (Vcb->VcbState & (VCB_STATE_VOLUME_READ_ONLY |
                           VCB_STATE_REMOVABLE_MEDIA)) ||
        !FileInfo || !FileInfo->Fcb ||
        FileInfo->Fcb->FcbCleanup ||
        !UDFIsADirectory(FileInfo) || UDFIsAStreamDir(FileInfo) ||
        UDFHasAStreamDir(FileInfo) || !FileInfo->FileIdent ||
        !(dloc = FileInfo->Dloc) ||
        dloc->CommonFcb != FileInfo->Fcb ||
        !dloc->DirIndex || !UDFIsDirEmpty(dloc->DirIndex) ||
        UDFGetFileLinkCount(FileInfo) != 1 ||
        !dloc->DataLoc.Offset || dloc->DataLoc.Length != 40 ||
        !dloc->FELoc.Mapping ||
        dloc->FELoc.Mapping[0].extLength != 512 ||
        (dloc->FELoc.Mapping[1].extLength & UDF_EXTENT_LENGTH_MASK) ||
        (dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
        dloc->DataLoc.Modified || dloc->AllocLoc.Modified ||
        dloc->FELoc.Modified ||
        !(parent = FileInfo->ParentFile) ||
        !parent->Dloc || !parent->Dloc->FileEntry ||
        !parent->Dloc->FELoc.Mapping ||
        parent->Dloc->FELoc.Mapping[0].extLength != 512 ||
        (parent->Dloc->FELoc.Mapping[1].extLength &
            UDF_EXTENT_LENGTH_MASK) ||
        parent->Dloc->DataLoc.Offset ||
        !parent->Dloc->DataLoc.Mapping ||
        !(parentIndex = parent->Dloc->DirIndex) ||
        FileInfo->Index < 2 ||
        !(entry = UDFDirIndex(parentIndex, FileInfo->Index)) ||
        !(entry->FileCharacteristics & FILE_DIRECTORY) ||
        (entry->FileCharacteristics & FILE_DELETED) ||
        !(FileInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY) ||
        (FileInfo->FileIdent->fileCharacteristics & FILE_DELETED) ||
        UDFDeleteParentHasOtherDirtyFid(parentIndex, FileInfo->Index) ||
        entry->Length != FileInfo->FileIdentLen ||
        !entry->Length || entry->Length > 512 ||
        (entry->Offset & 511) > 512 - entry->Length ||
        parent->Dloc->DataLoc.Length < 0 ||
        (uint64)entry->Offset + entry->Length >
            (uint64)parent->Dloc->DataLoc.Length ||
        UDFGetFileLinkCount(parent) < 2 ||
        UDFGetFileLinkCount(parent) == UDF_INVALID_LINK_COUNT)
        return STATUS_NOT_SUPPORTED;

    childFeLba = dloc->FELoc.Mapping[0].extLocation;
    parentFeLba = parent->Dloc->FELoc.Mapping[0].extLocation;
    childPart = UDFGetRefPartNumByPhysLba(Vcb, childFeLba);
    parentPart = UDFGetRefPartNumByPhysLba(Vcb, parentFeLba);
    fidLba = UDFExtentOffsetToLba(Vcb, parent->Dloc->DataLoc.Mapping,
                                  entry->Offset, NULL, NULL, NULL, NULL);
    if (childPart == (uint32)-1 || parentPart == (uint32)-1 ||
        fidLba == LBA_OUT_OF_EXTENT ||
        !UDFDeleteHasIndependentMetadataMirror(Vcb, childFeLba) ||
        !UDFDeleteHasIndependentMetadataMirror(Vcb, parentFeLba) ||
        !UDFDeleteHasIndependentMetadataMirror(Vcb, fidLba))
        return STATUS_NOT_SUPPORTED;

    status = UDFBuildDeletePrefree(Vcb, FileInfo, &record,
                                   &bitmapPhysical, &bitmapMetadata);
    if (status == STATUS_NOT_SUPPORTED)
        return status;
    *Handled = TRUE;
    if (!NT_SUCCESS(status))
        return status;
    if (record->RangeCount != 1 || !record->Ranges[0].Metadata ||
        record->Ranges[0].BitCount != 1) {
        MyFreePool__(record);
        *Handled = FALSE;
        return STATUS_NOT_SUPPORTED;
    }
    sector = (int8*)MyAllocatePool__(NonPagedPool, 512);
    parentBefore = (int8*)MyAllocatePool__(NonPagedPool, 512);
    fidBefore = (int8*)MyAllocatePool__(NonPagedPool,
                                        FileInfo->FileIdentLen);
    if (!sector || !parentBefore || !fidBefore) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status)) goto cleanup;
    status = UDFFlushFile__(IrpContext, Vcb, parent, 0);
    if (!NT_SUCCESS(status)) {
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        goto cleanup;
    }
    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
    if (!NT_SUCCESS(status)) {
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        goto cleanup;
    }
    /* A prior append or retag may have changed the parent's mapping while
     * preflushing it.  This transaction cannot use a stale FID address. */
    parentIndex = parent->Dloc->DirIndex;
    entry = parentIndex ? UDFDirIndex(parentIndex, FileInfo->Index) : NULL;
    if (parent->Dloc->DataLoc.Modified ||
        parent->Dloc->AllocLoc.Modified ||
        parent->Dloc->FELoc.Modified ||
        (parent->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
        parent->Dloc->DataLoc.Offset ||
        !entry || entry->Length != FileInfo->FileIdentLen ||
        (entry->FileCharacteristics & FILE_DELETED) ||
        UDFDeleteParentHasOtherDirtyFid(parentIndex, FileInfo->Index) ||
        fidLba != UDFExtentOffsetToLba(Vcb,
                       parent->Dloc->DataLoc.Mapping, entry->Offset,
                       NULL, NULL, NULL, NULL) ||
        !UDFDeleteHasIndependentMetadataMirror(Vcb, parentFeLba) ||
        !UDFDeleteHasIndependentMetadataMirror(Vcb, fidLba)) {
        status = STATUS_CANNOT_DELETE;
        goto cleanup;
    }
    parentFeFlags = parent->Dloc->FE_Flags;
    oldFlags = entry->FI_Flags;
    oldCharacteristics = entry->FileCharacteristics;
    RtlCopyMemory(parentBefore, parent->Dloc->FileEntry, 512);
    RtlCopyMemory(fidBefore, FileInfo->FileIdent,
                  FileInfo->FileIdentLen);
    removedId = UDFGetNTFileId(Vcb, FileInfo);
    UDFPrint(("UDFS empty-dir prefree start FE %lx parentFE %lx FID %lx\n",
              (ULONG)childFeLba, (ULONG)parentFeLba, (ULONG)fidLba));

    _SEH2_TRY {
        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
        bitmapHeld = TRUE;
        status = UDFValidateDeletePrefreeBitmaps(IrpContext, Vcb,
                     bitmapPhysical, bitmapMetadata, sector);
        if (NT_SUCCESS(status)) {
            record->Next = Vcb->DeletePrefreeList;
            Vcb->DeletePrefreeList = record;
            Vcb->DeletePrefreeCount++;
            dloc->DeletePrefree = record;
            attached = TRUE;
            uncertain = TRUE;
            status = UDFWriteDeletePrefreeBitmaps(IrpContext, Vcb, record,
                         bitmapPhysical, bitmapMetadata, sector,
                         &uncertain);
        }
        if (NT_SUCCESS(status))
            status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        UDFReleaseResource(&(Vcb->BitMapResource1));
        bitmapHeld = FALSE;

        if (NT_SUCCESS(status)) {
            UDFDecFileLinkCount(parent);
            parentTouched = TRUE;
            status = UDFFlushFE(IrpContext, Vcb, parent, parentPart);
        }
        if (NT_SUCCESS(status))
            status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (NT_SUCCESS(status)) {
            entry->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            entry->FileCharacteristics |= FILE_DELETED;
            FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
            RtlZeroMemory(&FileInfo->FileIdent->icb, sizeof(long_ad));
            fidTouched = TRUE;
            status = UDFFlushFI(IrpContext, Vcb, FileInfo, childPart);
        }
        if (NT_SUCCESS(status))
            status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (NT_SUCCESS(status)) {
            committed = TRUE;
            parentIndex->DelCount++;
            UDFChangeFileCounter(Vcb, FALSE, FALSE);
            UDFDecFileLinkCount(FileInfo);
            dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
            dloc->FE_Flags |= UDF_FE_FLAG_FREE_DEFERRED;
            UDFRemoveFileId(Vcb, removedId);
            UDFPrint(("UDFS empty-dir prefree committed FE %lx\n",
                      (ULONG)childFeLba));
            /* Cleanup owns the FCB until teardown. The overlay keeps its
             * freed on-disk FE bit unavailable to RAM allocation meanwhile. */
        }
    } _SEH2_FINALLY {
        if (_SEH2_AbnormalTermination() || !NT_SUCCESS(status)) {
            if (uncertain || _SEH2_AbnormalTermination())
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            if (!committed && fidTouched) {
                RtlCopyMemory(FileInfo->FileIdent, fidBefore,
                              FileInfo->FileIdentLen);
                entry->FileCharacteristics = oldCharacteristics;
                entry->FI_Flags = oldFlags;
            }
            if (!committed && parentTouched) {
                RtlCopyMemory(parent->Dloc->FileEntry, parentBefore, 512);
                parent->Dloc->FE_Flags = parentFeFlags;
            }
            if (!committed && attached) {
                if (!bitmapHeld) {
                    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),
                                                TRUE);
                    bitmapHeld = TRUE;
                }
                UDFDetachDeletePrefree(Vcb, dloc);
                record = NULL;
                attached = FALSE;
            }
        }
        if (bitmapHeld)
            UDFReleaseResource(&(Vcb->BitMapResource1));
    } _SEH2_END;

cleanup:
    if (fidBefore) MyFreePool__(fidBefore);
    if (parentBefore) MyFreePool__(parentBefore);
    if (sector) MyFreePool__(sector);
    if (record && !attached) MyFreePool__(record);
    return status;
}

/*
    This routine marks file as deleted & decrements file link counter.
    It can optionaly free allocation space
 */
/* OpenCount counts references held by children, not user handles of a stream.
 * FcbCleanup is decremented when that stream's last FILE_OBJECT is cleaned up.
 * Check the shared Dloc's FCB because a directory-index FI may be a parallel
 * FI rather than the one attached to the open handle. */
BOOLEAN
UDFStreamDirectoryHasLiveHandles(
    IN PUDF_FILE_INFO SDirInfo
    )
{
    PDIR_INDEX_ITEM Entry;
    PUDF_FILE_INFO StreamInfo;
    PFCB StreamFcb;
    uint_di Index;

    if (!SDirInfo || !SDirInfo->Dloc || !SDirInfo->Dloc->DirIndex)
        return FALSE;

    for (Index = 2;
         (Entry = UDFDirIndex(SDirInfo->Dloc->DirIndex, Index));
         Index++) {
        StreamInfo = Entry->FileInfo;
        if (!StreamInfo)
            continue;
        StreamFcb = StreamInfo->Dloc ? StreamInfo->Dloc->CommonFcb : NULL;
        if (!StreamFcb)
            StreamFcb = StreamInfo->Fcb;
        if (StreamFcb && StreamFcb->FcbCleanup)
            return TRUE;
    }
    return FALSE;
}

NTSTATUS
UDFUnlinkFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN BOOLEAN FreeSpace
    )
{
    uint_di Index;   // index of file to be deleted
    uint16 lc;
    PUDF_DATALOC_INFO Dloc;
    PUDF_FILE_INFO DirInfo;
    PUDF_FILE_INFO SDirInfo;
    PDIR_INDEX_HDR hDirNdx;
    PDIR_INDEX_HDR hCurDirNdx;
    PDIR_INDEX_ITEM DirNdx;
    NTSTATUS status;
    BOOLEAN IsSDir;
    BOOLEAN FinalFileLink;
    uint8 OldCharacteristics = 0;
    uint8 OldFidCharacteristics = 0;
    uint8 OldFiFlags = 0;
    uint32 OldFeFlags = 0;
    long_ad OldIcb;
    FILE_ID RemovedFileId;
    BOOLEAN RemoveFileId;
    BOOLEAN CommitBarrierFailed = FALSE;
    BOOLEAN PrefreeModified = FALSE;
    BOOLEAN PrefreeLockHeld = FALSE;
    BOOLEAN PrefreeAttached = FALSE;
    BOOLEAN DirectoryHandled = FALSE;
    PUDF_DELETE_PREFREE Prefree = NULL;
    int8* PrefreeSector = NULL;
    uint32 PhysicalBitmapLba = 0, MetadataBitmapLba = 0;
    NTSTATUS RestoreStatus;

    AdPrint(("UDFUnlinkFile__:\n"));
    if (!FileInfo) return STATUS_SUCCESS;

    ValidateFileInfo(FileInfo);
    RemoveFileId = FileInfo->Fcb != NULL;
    if (RemoveFileId)
        RemovedFileId = UDFGetNTFileId(Vcb, FileInfo);

    // check references
    Dloc = FileInfo->Dloc;
    // Original check was too strict: (OpenCount) || (RefCount > 1)
    // Problem: When we open a file via UDFOpenFile__ to delete it during rename,
    // that adds +1 to RefCount. If there's also a delayed-close reference,
    // RefCount becomes 2 and unlink fails even though OpenCount=0.
    // Fix: Only check OpenCount. RefCount will be properly managed by Close/CleanUp.
    // The key insight is: OpenCount tracks actual user handles, RefCount tracks
    // internal references. If no user has the file open (OpenCount=0), we should
    // be able to unlink it.
    // When FreeSpace=FALSE (rename/move), we only unlink the FID from the
    // directory — the file data stays intact. OpenCount/RefCount/SDirInfo checks
    // are not relevant because the file remains alive under a new name.
    if (FreeSpace) {
        if (UDFIsAStreamDir(FileInfo) || UDFHasAStreamDir(FileInfo) ||
            UDFIsADirectory(FileInfo)) {
            // Skip OpenCount/RefCount checks for:
            // - SDir: delayed-close streams may hold OpenCount > 0.
            // - File with SDir: SDir + stream LCBs contribute to counts.
            // - Directory: delayed-close children (already unlinked) may
            //   hold OpenCount > 0 via FCB reference chain. Directory
            //   emptiness is verified by UDFIsDirEmpty below.
        } else {
            if (FileInfo->OpenCount) {
                AdPrint(("UDFUnlinkFile__: OpenCount=%d, cannot delete\n", FileInfo->OpenCount));
                return STATUS_CANNOT_DELETE;
            }
            if (FileInfo->RefCount > 2) {
                AdPrint(("UDFUnlinkFile__: RefCount=%d > 2, cannot delete\n", FileInfo->RefCount));
                return STATUS_CANNOT_DELETE;
            }
        }
    }
    ASSERT(FileInfo->RefCount >= 1);
    DirInfo = FileInfo->ParentFile;
    // root dir or self
    if (!DirInfo || ((FileInfo->Index < 2) && !UDFIsAStreamDir(FileInfo))) return STATUS_CANNOT_DELETE;
    hDirNdx = DirInfo->Dloc->DirIndex;
    Index = FileInfo->Index;
    // we can't delete modified file
    // it should be closed & reopened (or flushed) before deletion
    DirNdx = UDFDirIndex(hDirNdx,Index);
#if defined UDF_DBG
    if (DirNdx && DirNdx->FName.Buffer) {
        AdPrint(("Unlink: %wZ\n", &DirNdx->FName));
    }
#endif // UDF_DBG
    if (FreeSpace &&
       ((Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
         Dloc->DataLoc.Modified ||
         Dloc->AllocLoc.Modified ||
         Dloc->FELoc.Modified ||
        (DirNdx && (DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) )) {
        return STATUS_CANNOT_DELETE;
    }
    /* A live named-stream handle must retain the stream directory and its
     * owner until cleanup.  This check precedes every FID/link-count change;
     * returning STATUS_CANNOT_DELETE later would leave a dirty parent FID. */
    if (FreeSpace &&
        UDFStreamDirectoryHasLiveHandles(UDFIsAStreamDir(FileInfo) ?
            FileInfo : (UDFHasAStreamDir(FileInfo) ? Dloc->SDirInfo : NULL)))
        return STATUS_CANNOT_DELETE;
    if (FreeSpace && UDFIsADirectory(FileInfo)) {
        status = UDFTryPreparedEmptyDirectoryUnlink(IrpContext, Vcb,
                                                     FileInfo,
                                                     &DirectoryHandled);
        if (DirectoryHandled)
            return status;
    }
    /* An ordinary final-link delete may fail while writing its FID. Save the
     * state that must be restored before cleanup can flush the parent. */
    if (FreeSpace && UDFGetFileLinkCount(FileInfo) == 1 &&
        !UDFIsADirectory(FileInfo) && !UDFIsAStream(FileInfo) &&
        !UDFHasAStreamDir(FileInfo) && FileInfo->FileIdent) {
        OldCharacteristics = DirNdx->FileCharacteristics;
        OldFidCharacteristics = FileInfo->FileIdent->fileCharacteristics;
        OldFiFlags = DirNdx->FI_Flags;
        OldFeFlags = Dloc->FE_Flags;
        RtlCopyMemory(&OldIcb, &FileInfo->FileIdent->icb, sizeof(OldIcb));
    }
    SDirInfo = Dloc->SDirInfo;
/*    if (FreeSpace && SDirInfo) {
        UDFPrint(("Unlink: SDirInfo should be NULL !!!\n"));
        BrutePoint();
        return STATUS_CANNOT_DELETE;
    }*/
    // stream directory can be deleted even being not empty
    // otherwise we should perform some checks
    if (!(IsSDir = UDFIsAStreamDir(FileInfo))) {
        // check if not empty direcory
        if ((DirNdx->FileCharacteristics & FILE_DIRECTORY) &&
           (hCurDirNdx = Dloc->DirIndex) &&
            FreeSpace) {
            if (!UDFIsDirEmpty(hCurDirNdx))
                return STATUS_DIRECTORY_NOT_EMPTY;
        }
        DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
        DirNdx->FileCharacteristics |= FILE_DELETED;
        FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
        hDirNdx->DelCount++;
        if (!UDFIsAStream(FileInfo))
            UDFChangeFileCounter(Vcb, !UDFIsADirectory(FileInfo), FALSE);
        else if (FreeSpace)
            UDFAddToStreamOwnerSize(FileInfo, -UDFGetFileSize(FileInfo));
        if (UDFIsADirectory(FileInfo) && FileInfo->ParentFile) {
            
            // FID of subdir parent counts as a link to the parent
            UDFDecFileLinkCount(FileInfo->ParentFile);
        }
    } else { // FID of stream dir parent link to owning file
        if (FileInfo->ParentFile) {

            UDFDecFileLinkCount(FileInfo->ParentFile);
        }
    }
    lc = UDFGetFileLinkCount(FileInfo);
    FinalFileLink = (lc == 1);

    if (lc) {

        UDFDecFileLinkCount(FileInfo);
        lc--;
    }

    if (!IsSDir && 
        (lc == 1) && 
        UDFHasAStreamDir(FileInfo) && 
        !UDFIsSDirDeleted(Dloc->SDirInfo)
    ) {
        lc = 0;
    }

    if (DirNdx) {
        if (FreeSpace) {
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            DirNdx->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
        }
        // Root Files (Root/SDir/Vat/etc.) has no FileIdent...
        if (FileInfo->FileIdent)
            RtlZeroMemory(&(FileInfo->FileIdent->icb), sizeof(long_ad));
    }
    // caller wishes to free allocation, but we can't do it due to
    // alive links. In this case we should just remove reference
    if (FreeSpace && lc) {
        ((icbtag*)(Dloc->FileEntry+1))->parentICBLocation.logicalBlockNum = 0;
        ((icbtag*)(Dloc->FileEntry+1))->parentICBLocation.partitionReferenceNum = 0;
        Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    } else
    // if caller wishes to free file allocation &
    // there are no more references(links) to this file, lets do it >;->
    if (FreeSpace && !lc) {
        if (UDFHasAStreamDir(FileInfo) &&
           !UDFIsSDirDeleted(Dloc->SDirInfo) ) {
            // we have a Stream Dir associated...
            PUDF_FILE_INFO SFileInfo;
            if (!Dloc->SDirInfo) {
                // open SDir from disk
                status = UDFOpenStreamDir__(IrpContext, Vcb, FileInfo, &(Dloc->SDirInfo));
                if (!NT_SUCCESS(status)) {
                    // abort Unlink on error
                    SFileInfo = Dloc->SDirInfo;
cleanup_SDir:
                    UDFCleanUpFile__(Vcb, SFileInfo);
                    if (SFileInfo) MyFreePool__(SFileInfo);
                    UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
                    return status;
                }
            }  else {
                UDFReferenceFile__(Dloc->SDirInfo);
            }
            SDirInfo = Dloc->SDirInfo;
            // try to perform deltree for Streams
            status = UDFUnlinkAllFilesInDir(IrpContext, Vcb, SDirInfo);
            if (!NT_SUCCESS(status)) {
                // abort Unlink on error
                UDFCloseFile__(IrpContext, Vcb, SDirInfo);
                SFileInfo = SDirInfo;
                BrutePoint();
                goto cleanup_SDir;
            }
            // delete SDir
            UDFFlushFile__(IrpContext, Vcb, SDirInfo, 0);
            AdPrint(("  "));
            UDFUnlinkFile__(IrpContext, Vcb, SDirInfo, TRUE);
            // Mark SDir FCB as deleted so delayed close skips re-unlink
            if (SDirInfo->Fcb) {
                SDirInfo->Fcb->FcbState |= UDF_FCB_DELETED;
                SDirInfo->Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETE_PARENT);
            }
            // close SDir
            UDFCloseFile__(IrpContext, Vcb, SDirInfo);
            if (UDFCleanUpFile__(Vcb, SDirInfo)) {
                MyFreePool__(SDirInfo);
            }
            // If UDFCleanUpFile__ returns UDF_FREE_NOTHING, SDir has
            // delayed-close stream references. TeardownStructures
            // handles cleanup when those delayed closes fire.
            // update FileInfo
            ASSERT(Dloc->FileEntry);
            RtlZeroMemory( &(((PEXTENDED_FILE_ENTRY)(Dloc->FileEntry))->streamDirectoryICB), sizeof(long_ad));
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        } else
        if (IsSDir) {
            // do deltree for Streams
            status = UDFUnlinkAllFilesInDir(IrpContext, Vcb, FileInfo);
            if (!NT_SUCCESS(status)) {
                UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
                return status;
            }
            // update parent FileInfo
            ASSERT(FileInfo->ParentFile->Dloc->FileEntry);
            RtlZeroMemory( &(((PEXTENDED_FILE_ENTRY)(FileInfo->ParentFile->Dloc->FileEntry))->streamDirectoryICB), sizeof(long_ad));
            FileInfo->ParentFile->Dloc->FE_Flags &= ~UDF_FE_FLAG_HAS_SDIR;
            FileInfo->ParentFile->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED |
                                                     UDF_FE_FLAG_HAS_DEL_SDIR);
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_IS_DEL_SDIR;
        }
        // The last name of an ordinary file is being removed. Commit its
        // deleted FID before freeing any blocks; the now unreachable FE need
        // not be rewritten solely to record a zero link count. Its dirty
        // allocation state was rejected above, and open FCBs still defer the
        // physical free until they leave FcbTable.
        // Keep stream, directory, sequential-media and unmount paths intact.
        uint32 PartNum;
        if (FinalFileLink &&
            !UDFIsADirectory(FileInfo) &&
            !UDFIsAStream(FileInfo) &&
            !UDFHasAStreamDir(FileInfo) &&
            FileInfo->FileIdent &&
            !Vcb->CDR_Mode &&
            Vcb->VcbCondition == VcbMounted &&
            Vcb->FsDeviceType == FILE_DEVICE_DISK_FILE_SYSTEM &&
            !(Vcb->VcbState & VCB_STATE_REMOVABLE_MEDIA) &&
            ((PartNum = UDFGetRefPartNumByPhysLba(
                Vcb, Dloc->FELoc.Mapping[0].extLocation)) != (uint32)-1)) {
            /* A supported final-link delete pre-frees its exact on-disk
             * bitmap bits while RAM still reserves them, barriers that
             * generation, then barriers the FID tombstone.  Both crash
             * sides are repairable by the existing complete owner graph. */
            if (UDFExtentOffsetToLba(Vcb,
                    DirInfo->Dloc->DataLoc.Mapping, DirNdx->Offset,
                    NULL, NULL, NULL, NULL) != LBA_OUT_OF_EXTENT)
                status = UDFBuildDeletePrefree(Vcb, FileInfo, &Prefree,
                                &PhysicalBitmapLba, &MetadataBitmapLba);
            else
                status = STATUS_NOT_SUPPORTED;
            if (NT_SUCCESS(status)) {
                _SEH2_TRY {
                    PrefreeSector = (int8*)MyAllocatePool__(NonPagedPool, 512);
                    if (!PrefreeSector)
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    if (NT_SUCCESS(status))
                        status = UDFOpenIntegrityForWrite(IrpContext, Vcb,
                                                          FALSE);
                    if (NT_SUCCESS(status)) {
                        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),
                                                     TRUE);
                        PrefreeLockHeld = TRUE;
                        status = UDFValidateDeletePrefreeBitmaps(IrpContext,
                            Vcb, PhysicalBitmapLba, MetadataBitmapLba,
                            PrefreeSector);
                        if (NT_SUCCESS(status)) {
                            Prefree->Next = Vcb->DeletePrefreeList;
                            Vcb->DeletePrefreeList = Prefree;
                            Vcb->DeletePrefreeCount++;
                            Dloc->DeletePrefree = Prefree;
                            PrefreeAttached = TRUE;
                            status = UDFWriteDeletePrefreeBitmaps(IrpContext,
                                Vcb, Prefree, PhysicalBitmapLba,
                                MetadataBitmapLba, PrefreeSector,
                                &PrefreeModified);
                            if (NT_SUCCESS(status))
                                status = UDFBarrierFlushDevice(
                                    Vcb->TargetDeviceObject);
                            if (NT_SUCCESS(status)) {
                                uint32 rangeIndex;
                                /* The dense bitmap's old copy tracks the
                                 * actual disk generation.  Without this,
                                 * free then reallocate can leave RAM equal
                                 * to the old copy, so the next volume flush
                                 * skips writing the now-used disk bit. */
                                for (rangeIndex = 0;
                                     rangeIndex < Prefree->RangeCount;
                                     ++rangeIndex) {
                                    PUDF_DELETE_PREFREE_RANGE range =
                                        &Prefree->Ranges[rangeIndex];
                                    if (!range->Metadata)
                                        UDFSetFreeBits(Vcb->FSBM_OldBitmap,
                                                       range->FirstBit,
                                                       range->BitCount);
                                }
                            }
                            if (NT_SUCCESS(status))
                                status = UDFFlushFI(IrpContext, Vcb,
                                                    FileInfo, PartNum);
                            if (NT_SUCCESS(status))
                                status = UDFBarrierFlushDevice(
                                    Vcb->TargetDeviceObject);
                            if (!NT_SUCCESS(status)) {
                                /* Any failed write or barrier leaves an
                                 * uncertain durable generation. */
                                CommitBarrierFailed = TRUE;
                                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                                UDFDetachDeletePrefree(Vcb, Dloc);
                                Prefree = NULL;
                                PrefreeAttached = FALSE;
                            }
                        }
                    }
                } _SEH2_FINALLY {
                    if (_SEH2_AbnormalTermination()) {
                        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                        if (PrefreeAttached) {
                            UDFDetachDeletePrefree(Vcb, Dloc);
                            Prefree = NULL;
                        }
                    }
                    if (PrefreeLockHeld)
                        UDFReleaseResource(&(Vcb->BitMapResource1));
                    if (PrefreeSector)
                        MyFreePool__(PrefreeSector);
                    if (Prefree && (!NT_SUCCESS(status) ||
                                    _SEH2_AbnormalTermination()))
                        MyFreePool__(Prefree);
                } _SEH2_END;
                if (NT_SUCCESS(status))
                    UDFPrint(("UDFS unlink prefree: FE %x, ranges %u\n",
                        Dloc->FELoc.Mapping[0].extLocation,
                        Dloc->DeletePrefree->RangeCount));
            } else if (status == STATUS_NOT_SUPPORTED) {
                /* Preserve ordinary deletion on layouts not yet covered by
                 * the bounded prefree protocol.  R250 still orders any
                 * subsequent reuse behind the tombstone. */
                status = UDFFlushFI(IrpContext, Vcb, FileInfo, PartNum);
                if (NT_SUCCESS(status)) {
                    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
                    if (!NT_SUCCESS(status)) {
                        CommitBarrierFailed = TRUE;
                        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                    }
                }
            }
            if (!NT_SUCCESS(status)) {
                /* The FID was changed in memory before UDFFlushFI. A later
                 * parent flush must not turn this failed unlink into a delete.
                 * Keep the FE and its allocation reachable, and retry writing
                 * the original FID in case the failed write was partial. */
                DirNdx->FileCharacteristics = OldCharacteristics;
                FileInfo->FileIdent->fileCharacteristics = OldFidCharacteristics;
                RtlCopyMemory(&FileInfo->FileIdent->icb, &OldIcb, sizeof(OldIcb));
                DirNdx->FI_Flags = OldFiFlags | UDF_FI_FLAG_FI_MODIFIED;
                hDirNdx->DelCount--;
                UDFChangeFileCounter(Vcb, TRUE, TRUE);
                UDFIncFileLinkCount(FileInfo);
                Dloc->FE_Flags = OldFeFlags;
                if (!CommitBarrierFailed) {
                    RestoreStatus = UDFFlushFI(IrpContext, Vcb, FileInfo,
                                               PartNum);
                    if (NT_SUCCESS(RestoreStatus))
                        RestoreStatus = UDFBarrierFlushDevice(
                            Vcb->TargetDeviceObject);
                    if (NT_SUCCESS(RestoreStatus))
                        DirNdx->FI_Flags = OldFiFlags;
                    else
                        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                }
                return status;
            }
            Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
        } else {
            UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
        }

        if ((Dloc->FE_Flags & UDF_FE_FLAG_FREE_DEFERRED) || Dloc->CommonFcb) {

            // Defer block freeing while an FCB still owns this Dloc.
            // Dloc->CommonFcb != NULL means this LBA is still keyed in
            // Vcb->FcbTable. Freeing the FE block now would let a freshly
            // created file reuse the LBA while the stale FCB lingers (the
            // cleanup->close window, or delayed close), so the reused FCB
            // accumulates parent LCBs from several incarnations and trips the
            // multi-parent ASSERT(!Recursive) in UDFTeardownStructures.
            // Keep the block allocated and the Dloc in cache; UDFCleanUpFile__
            // performs the actual free at the last Dloc reference (KeepDloc=FALSE),
            // which is exactly when the owning FCB is torn down (CommonFcb cleared).
            Dloc->FE_Flags |= UDF_FE_FLAG_FREE_DEFERRED;

        } else {

            // Remove the Dloc lookup before its FE bit becomes allocatable.
            // The overlay is independent of the lookup and stays present
            // until RAM release finishes under the bitmap lock.
            UDFUnlinkDloc(Vcb, Dloc);
            if (Dloc->DeletePrefree)
                UDFReleasePrefreedAllocation(Vcb, DirInfo, FileInfo);
            else
                UDFFreeFileAllocation(Vcb, DirInfo, FileInfo);
            ASSERT(!(FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED));
            FileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
        }
    }

    /* A refused or failed unlink leaves the namespace entry reachable.
     * Drop its by-ID path cache only after the namespace change succeeds. */
    if (RemoveFileId)
        UDFRemoveFileId(Vcb, RemovedFileId);
    return STATUS_SUCCESS;
} // end UDFUnlinkFile__()

/*
    Drop the stream directory (and all named streams) of a file that itself stays
    alive — used on overwrite/supersede. Unlike the full-delete path (where the base
    file is torn down synchronously with its SDir), here the base survives, so we must
    explicitly detach the in-memory base<->SDir prefix LCB; otherwise a later teardown
    of the open base would follow a link to a freed SDir (use-after-free).

    We do NOT free the SDir / stream FileInfo or FCB here: they stay on the
    delayed-close path and are released there. We only:
      - delete the streams and the SDir on disk (UDFUnlinkFile__ on the SDir, which
        also clears the base file's HAS_SDIR flag and streamDirectoryICB),
      - remove the base<->SDir prefix LCB and balance the base references,
      - detach Dloc->SDirInfo from the base.

    Requires exclusive Vcb access.
*/
NTSTATUS
UDFDeleteAllStreams(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    NTSTATUS status;
    PUDF_FILE_INFO SDirInfo;
    PUDF_DATALOC_INFO Dloc;
    PFCB SDirFcb;

    ValidateFileInfo(FileInfo);
    Dloc = FileInfo->Dloc;

    if (!UDFHasAStreamDir(FileInfo) || UDFIsSDirDeleted(Dloc->SDirInfo)) {
        return STATUS_SUCCESS;
    }

    if (!Dloc->SDirInfo) {
        status = UDFOpenStreamDir__(IrpContext, Vcb, FileInfo, &(Dloc->SDirInfo));
        if (!NT_SUCCESS(status)) {
            if (Dloc->SDirInfo) {
                UDFCleanUpFile__(Vcb, Dloc->SDirInfo);
                MyFreePool__(Dloc->SDirInfo);
                Dloc->SDirInfo = NULL;
            }
            return status;
        }
    }
    SDirInfo = Dloc->SDirInfo;

    // Delete the streams and the stream directory on disk. UDFUnlinkFile__ on an
    // SDir runs the deltree of its streams and clears the base file's HAS_SDIR /
    // streamDirectoryICB (its IsSDir branch). Data blocks are freed there (or
    // deferred while an FCB still owns the Dloc).
    status = UDFFlushFile__(IrpContext, Vcb, SDirInfo, 0);
    if (!NT_SUCCESS(status)) {
        /* The stream directory remains attached to the base file. A failed
         * metadata flush must not be followed by namespace deletion. */
        UDFCloseFile__(IrpContext, Vcb, SDirInfo);
        return status;
    }
    status = UDFUnlinkFile__(IrpContext, Vcb, SDirInfo, TRUE);
    if (!NT_SUCCESS(status)) {
        UDFCloseFile__(IrpContext, Vcb, SDirInfo);
        return status;
    }

    // Detach the in-memory base<->SDir prefix LCB(s). The base file survives the
    // overwrite, so this link must go now — otherwise teardown of the base would
    // later follow it to a freed SDir. Balance the base references exactly as
    // UDFTeardownStructures does when it removes a prefix.
    SDirFcb = SDirInfo->Fcb;
    if (SDirFcb) {

        SDirFcb->FcbState |= UDF_FCB_DELETED;
        SDirFcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETE_PARENT);

        UDFLockVcb(IrpContext, Vcb);
        while (!IsListEmpty(&SDirFcb->ParentLcbQueue)) {

            PLCB Lcb = CONTAINING_RECORD(SDirFcb->ParentLcbQueue.Flink, struct LCB, ChildFcbLinks);
            PFCB ParentFcb = Lcb->ParentFcb;

            // A create may be holding this prefix; leave it to the normal path.
            if (Lcb->Reference != 0) {
                break;
            }

            UDFRemovePrefix(IrpContext, Lcb);

            if (ParentFcb) {
                if (ParentFcb->FileInfo) {
                    UDFCloseFile__(IrpContext, Vcb, ParentFcb->FileInfo);
                }
                InterlockedDecrement((PLONG)&ParentFcb->FcbReference);
            }
        }
        UDFUnlockVcb(IrpContext, Vcb);
    }

    // Fully release the SDir's hold on the (surviving) base file. The SDir is a
    // child of the base (SDir->ParentFile == base), so each of its references
    // contributes to the base's OpenCount. In the delete path the leftover would
    // be resolved by tearing down the base; here the base survives, so we must
    // drop the whole SDir->base linkage now, otherwise the base keeps a stray
    // OpenCount and can no longer be deleted. The SDir FCB stays alive (FcbRef>0,
    // delayed close), so UDFCleanUpFile__ will not free it — no use-after-free.
    while (SDirInfo->RefCount) {
        UDFCloseFile__(IrpContext, Vcb, SDirInfo);
    }
    Dloc->SDirInfo = NULL;
    Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

    UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
    return STATUS_SUCCESS;
} // end UDFDeleteAllStreams()

NTSTATUS
UDFUnlinkAllFilesInDir(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    )
{
    PDIR_INDEX_HDR hCurDirNdx;
    PDIR_INDEX_ITEM CurDirNdx;
    PUDF_FILE_INFO FileInfo;
    NTSTATUS status;
    uint_di i;

    hCurDirNdx = DirInfo->Dloc->DirIndex;
    // Retain the stream directory while a child still has a live handle.
    // Callers normally preflight before changing their own FID as well.
    if (UDFStreamDirectoryHasLiveHandles(DirInfo))
        return STATUS_CANNOT_DELETE;
    // start deletion
    for(i=2; (CurDirNdx = UDFDirIndex(hCurDirNdx,i)); i++) {

        FileInfo = NULL;

        // try to open Stream
        status = UDFOpenFile__(IrpContext, Vcb, FALSE, TRUE, NULL, DirInfo, &FileInfo, &i);

        if (status == STATUS_FILE_DELETED) {
            goto skip_del_stream;
        } else if (!NT_SUCCESS(status)) {
            // Error :(((
            UDFCleanUpFile__(Vcb, FileInfo);
            if (FileInfo)
                MyFreePool__(FileInfo);
            return status;
        }

        UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
        AdPrint(("    "));
        UDFUnlinkFile__(IrpContext, Vcb, FileInfo, TRUE);
        // Mark stream FCB as deleted so delayed close skips re-unlink/flush
        if (FileInfo->Fcb) {
            FileInfo->Fcb->FcbState |= UDF_FCB_DELETED;
            FileInfo->Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETE_PARENT);
        }
        UDFCloseFile__(IrpContext, Vcb, FileInfo);
skip_del_stream:
        if (FileInfo && UDFCleanUpFile__(Vcb, FileInfo)) {
            MyFreePool__(FileInfo);
        }
    }
    return STATUS_SUCCESS;
} // end UDFUnlinkAllFilesInDir()

/*
    This routine inits UDF_FILE_INFO structure for specified file
    If it returns status != STATUS_SUCCESS caller should call UDFCleanUpFile__
    for returned pointer *WITHOUT* using UDFCloseFile__
 */
NTSTATUS
UDFOpenFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN NotDeleted,
    IN PUNICODE_STRING fn,
    IN PUDF_FILE_INFO DirInfo,
    OUT PUDF_FILE_INFO* _FileInfo,// this is to be filled & doesn't contain
                                // any pointers
    IN uint_di* IndexToOpen
    )
{
    NTSTATUS status;
    uint_di i=0;
    EXTENT_AD FEExt;
    uint16 Ident;
    PDIR_INDEX_HDR hDirNdx = DirInfo->Dloc->DirIndex;
    PDIR_INDEX_ITEM DirNdx;
    PUDF_FILE_INFO FileInfo;
    PUDF_FILE_INFO ParFileInfo;
    *_FileInfo = NULL;
    if (!hDirNdx) return STATUS_NOT_A_DIRECTORY;

    // find specified file in directory index
    // if it is already known, skip this foolish code
    if (IndexToOpen) {
        i=*IndexToOpen;
    } else
        if (!NT_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, NotDeleted, fn, DirInfo, &i)))
            return status;
    // do this check for OpenByIndex
    // some routines may send invalid Index
    if (!(DirNdx = UDFDirIndex(hDirNdx,i)))
        return STATUS_OBJECT_NAME_NOT_FOUND;
    if ((FileInfo = DirNdx->FileInfo)) {
        // file is already opened.
        if ((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
            AdPrint(("  FILE_DELETED on open\n"));
            return STATUS_FILE_DELETED;
        }
        if ((FileInfo->ParentFile != DirInfo) &&
           (FileInfo->Index >= 2)) {
            ParFileInfo = UDFLocateParallelFI(DirInfo, i, FileInfo);
            BrutePoint();
            if (ParFileInfo->ParentFile != DirInfo) {
                FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
                *_FileInfo = FileInfo;
                if (!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
                RtlCopyMemory(FileInfo, DirNdx->FileInfo, sizeof(UDF_FILE_INFO));
    //          FileInfo->NextLinkedFile = DirNdx->FileInfo->NextLinkedFile; // is already done
                UDFInsertLinkedFile(FileInfo, DirNdx->FileInfo);
                DirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
                FileInfo->RefCount = 0;
                FileInfo->ParentFile = DirInfo;
                FileInfo->Fcb = NULL;
            } else {
                FileInfo = ParFileInfo;
            }
        }
        // Just increase some counters & exit
        UDFReferenceFile__(FileInfo);

        ASSERT(FileInfo->ParentFile == DirInfo);
        ValidateFileInfo(FileInfo);

        *_FileInfo = FileInfo;
        return STATUS_SUCCESS;
    } else
    if (IndexToOpen) {
        if ((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
            AdPrint(("  FILE_DELETED on open (2)\n"));
            return STATUS_FILE_DELETED;
        }
    }
    FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
    *_FileInfo = FileInfo;
    if (!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // read FileIdent
    FileInfo->FileIdent = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, DirNdx->Length, MEM_FID_TAG);
    if (!(FileInfo->FileIdent)) return STATUS_INSUFFICIENT_RESOURCES;
    FileInfo->FileIdentLen = DirNdx->Length;
    if (!NT_SUCCESS(status = UDFReadFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset,
                             DirNdx->Length, FALSE, (int8*)(FileInfo->FileIdent)) ))
        return status;
    if (FileInfo->FileIdent->descTag.tagIdent != TID_FILE_IDENT_DESC) {
        BrutePoint();
        return STATUS_FILE_CORRUPT_ERROR;
    }
    // check for opened links
    if (!NT_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation)))))
        return status;
    // init pointer to parent object
    FileInfo->Index = i;
    FileInfo->ParentFile = DirInfo;
    // init pointers to linked files (if any)
    if (FileInfo->Dloc->LinkedFileInfo != FileInfo)
        UDFInsertLinkedFile(FileInfo, FileInfo->Dloc->LinkedFileInfo);
    if (FileInfo->Dloc->FileEntry) {
        // Dloc cached in UDFOpenFile__ — skip FE read from disk
        goto init_tree_entry;
    }
    // read (Ex)FileEntry
    FileInfo->Dloc->FileEntry = (tag*)MyAllocatePoolTag__(NonPagedPool, Vcb->SectorSize, MEM_FE_TAG);
    if (!(FileInfo->Dloc->FileEntry)) return STATUS_INSUFFICIENT_RESOURCES;
    status = UDFReadFileEntry(IrpContext, Vcb, &FileInfo->FileIdent->icb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &Ident);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // build mappings for Data & AllocDescs
    if (!FileInfo->Dloc->AllocLoc.Mapping) {
        FEExt.extLength = FileInfo->FileIdent->icb.extLength;
        FEExt.extLocation = UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation) );
        if (FEExt.extLocation == LBA_OUT_OF_EXTENT) {
            return STATUS_FILE_CORRUPT_ERROR;
        }
        FileInfo->Dloc->AllocLoc.Mapping = UDFExtentToMapping(&FEExt);
        if (!(FileInfo->Dloc->AllocLoc.Mapping))
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    // read location info
    status = UDFLoadExtInfo(IrpContext, Vcb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &FileInfo->FileIdent->icb,
                           &FileInfo->Dloc->DataLoc, &FileInfo->Dloc->AllocLoc);
    if (!NT_SUCCESS(status))
        return status;
    // init (Ex)FileEntry mapping
    FileInfo->Dloc->FELoc.Length = (FileInfo->Dloc->DataLoc.Offset) ? FileInfo->Dloc->DataLoc.Offset :
                                                          FileInfo->Dloc->AllocLoc.Offset;
//    FileInfo->Dloc->FELoc.Offset = 0;
    FileInfo->Dloc->FELoc.Mapping = UDFExtentToMapping(&FEExt);
    FileInfo->Dloc->FileEntryLen = (uint32)(FileInfo->Dloc->FELoc.Length);
    // we get here immediately when opened link encountered
init_tree_entry:
    // init back pointer from parent object
    ASSERT(!DirNdx->FileInfo);
    DirNdx->FileInfo = FileInfo;
    // init DirIndex
    if (UDFGetFileLinkCount(FileInfo) > 1) {
        DirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    } else {
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_LINKED;
    }
    // Keep FE buffer at SectorSize — FlushFE may write full sector for in-ICB mode
    // check if this file has a SDir
    if ((FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) &&
       ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength )
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    if (!(FileInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY)) {
        UDFReferenceFile__(FileInfo);
        ASSERT(FileInfo->ParentFile == DirInfo);
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        return STATUS_SUCCESS;
    }

    UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used

    // build index for directories
    if (!FileInfo->Dloc->DirIndex) {
        status = UDFIndexDirectory(IrpContext, Vcb, FileInfo);
        if (!NT_SUCCESS(status))
            return status;
    }
    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);
    ASSERT(FileInfo->ParentFile == DirInfo);

    return status;
} // end UDFOpenFile__()


/*
    Open file from directory context.
    This is called after UDFFindDirEntry to open the found file.
 */
NTSTATUS
UDFOpenFileInfoFromDirContext(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN NotDeleted,
    OUT PUDF_FILE_INFO* _FileInfo
    )
{
    NTSTATUS status;
    EXTENT_AD FEExt;
    uint16 Ident;
    PDIR_INDEX_ITEM DirNdx = DirContext->DirNdx;
    PUDF_FILE_INFO DirInfo = DirContext->ParentInfo;
    PUDF_FILE_INFO FileInfo;
    PUDF_FILE_INFO ParFileInfo;
    uint_di i = DirContext->Index;

    *_FileInfo = NULL;

    if (!DirNdx) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if ((FileInfo = DirNdx->FileInfo)) {
        // file is already opened
        if ((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
            AdPrint(("  FILE_DELETED on open\n"));
            return STATUS_FILE_DELETED;
        }
        if ((FileInfo->ParentFile != DirInfo) &&
           (FileInfo->Index >= 2)) {
            ParFileInfo = UDFLocateParallelFI(DirInfo, i, FileInfo);
            BrutePoint();
            if (ParFileInfo->ParentFile != DirInfo) {
                FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
                *_FileInfo = FileInfo;
                if (!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
                RtlCopyMemory(FileInfo, DirNdx->FileInfo, sizeof(UDF_FILE_INFO));
                UDFInsertLinkedFile(FileInfo, DirNdx->FileInfo);
                DirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
                FileInfo->RefCount = 0;
                FileInfo->ParentFile = DirInfo;
                FileInfo->Fcb = NULL;
            } else {
                FileInfo = ParFileInfo;
            }
        }
        // Just increase some counters & exit
        UDFReferenceFile__(FileInfo);

        ASSERT(FileInfo->ParentFile == DirInfo);
        ValidateFileInfo(FileInfo);

        *_FileInfo = FileInfo;
        return STATUS_SUCCESS;
    }

    // Check deleted for new open
    if ((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
        AdPrint(("  FILE_DELETED on open (2)\n"));
        return STATUS_FILE_DELETED;
    }

    FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
    *_FileInfo = FileInfo;
    if (!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // read FileIdent
    FileInfo->FileIdent = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, DirNdx->Length, MEM_FID_TAG);
    if (!(FileInfo->FileIdent)) return STATUS_INSUFFICIENT_RESOURCES;
    FileInfo->FileIdentLen = DirNdx->Length;
    if (!NT_SUCCESS(status = UDFReadFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset,
                             DirNdx->Length, FALSE, (int8*)(FileInfo->FileIdent))))
        return status;
    if (FileInfo->FileIdent->descTag.tagIdent != TID_FILE_IDENT_DESC) {
        BrutePoint();
        return STATUS_FILE_CORRUPT_ERROR;
    }
    // check for opened links
    if (!NT_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation)))))
        return status;
    // init pointer to parent object
    FileInfo->Index = i;
    FileInfo->ParentFile = DirInfo;
    // init pointers to linked files (if any)
    if (FileInfo->Dloc->LinkedFileInfo != FileInfo)
        UDFInsertLinkedFile(FileInfo, FileInfo->Dloc->LinkedFileInfo);
    if (FileInfo->Dloc->FileEntry)
        goto init_tree_entry;
    // read (Ex)FileEntry
    FileInfo->Dloc->FileEntry = (tag*)MyAllocatePoolTag__(NonPagedPool, Vcb->SectorSize, MEM_FE_TAG);
    if (!(FileInfo->Dloc->FileEntry)) return STATUS_INSUFFICIENT_RESOURCES;
    status = UDFReadFileEntry(IrpContext, Vcb, &FileInfo->FileIdent->icb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &Ident);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    // build mappings for Data & AllocDescs
    if (!FileInfo->Dloc->AllocLoc.Mapping) {
        FEExt.extLength = FileInfo->FileIdent->icb.extLength;
        FEExt.extLocation = UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation));
        if (FEExt.extLocation == LBA_OUT_OF_EXTENT)
            return STATUS_FILE_CORRUPT_ERROR;
        FileInfo->Dloc->AllocLoc.Mapping = UDFExtentToMapping(&FEExt);
        if (!(FileInfo->Dloc->AllocLoc.Mapping))
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    // read location info
    status = UDFLoadExtInfo(IrpContext, Vcb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &FileInfo->FileIdent->icb,
                           &FileInfo->Dloc->DataLoc, &FileInfo->Dloc->AllocLoc);
    if (!NT_SUCCESS(status))
        return status;
    // init (Ex)FileEntry mapping
    FileInfo->Dloc->FELoc.Length = (FileInfo->Dloc->DataLoc.Offset) ? FileInfo->Dloc->DataLoc.Offset :
                                                          FileInfo->Dloc->AllocLoc.Offset;
    FileInfo->Dloc->FELoc.Mapping = UDFExtentToMapping(&FEExt);
    FileInfo->Dloc->FileEntryLen = (uint32)(FileInfo->Dloc->FELoc.Length);
    // we get here immediately when opened link encountered
init_tree_entry:
    // init back pointer from parent object
    ASSERT(!DirNdx->FileInfo);
    DirNdx->FileInfo = FileInfo;
    // init DirIndex
    if (UDFGetFileLinkCount(FileInfo) > 1) {
        DirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    } else {
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_LINKED;
    }
    // Keep FE buffer at SectorSize — FlushFE may write full sector for in-ICB mode
    // check if this file has a SDir
    if ((FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) &&
       ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength)
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    if (!(FileInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY)) {
        UDFReferenceFile__(FileInfo);
        ASSERT(FileInfo->ParentFile == DirInfo);
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        return STATUS_SUCCESS;
    }

    UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED);

    // build index for directories
    if (!FileInfo->Dloc->DirIndex) {
        status = UDFIndexDirectory(IrpContext, Vcb, FileInfo);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);
    ASSERT(FileInfo->ParentFile == DirInfo);

    return status;
} // end UDFOpenFileInfoFromDirContext()


/*
    This routine inits UDF_FILE_INFO structure for root directory
 */
NTSTATUS
UDFOpenRootFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN lb_addr* RootLoc,
    OUT PUDF_FILE_INFO FileInfo
    )
{
    uint32 RootLBA;
    NTSTATUS status;
//    uint32 PartNum = RootLoc->partitionReferenceNum;
    uint32 LBS = Vcb->SectorSize;
    uint16 Ident;
    LONG_AD FELoc;
    EXTENT_AD FEExt;
    uint8 FileType;

    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    RootLBA = UDFPartLbaToPhys(Vcb,RootLoc);
    if (RootLBA == LBA_OUT_OF_EXTENT)
        return STATUS_FILE_CORRUPT_ERROR;
    FELoc.extLocation = *RootLoc;
    FELoc.extLength = LBS;
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // check for opened links
    if (!NT_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, RootLBA)))
        return status;
    if (FileInfo->Dloc->FileEntry)
        goto init_tree_entry;
    // read (Ex)FileEntry
    FileInfo->Dloc->FileEntry = (tag*)MyAllocatePoolTag__(NonPagedPool, LBS, MEM_FE_TAG);
    if (!(FileInfo->Dloc->FileEntry)) return STATUS_INSUFFICIENT_RESOURCES;

    if (!NT_SUCCESS(status = UDFReadFileEntry(IrpContext, Vcb, &FELoc, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &Ident)))
        return status;
    // build mappings for Data & AllocDescs
    FEExt.extLength = LBS;
    FEExt.extLocation = UDFPartLbaToPhys(Vcb, &(FELoc.extLocation) );
    if (FEExt.extLocation == LBA_OUT_OF_EXTENT)
        return STATUS_FILE_CORRUPT_ERROR;
    FileInfo->Dloc->FELoc.Mapping = UDFExtentToMapping(&FEExt);
    if (!(FileInfo->Dloc->FELoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    // build mappings for AllocDescs
    if (!FileInfo->Dloc->AllocLoc.Mapping) {
        FileInfo->Dloc->AllocLoc.Mapping = UDFExtentToMapping(&FEExt);
        if (!(FileInfo->Dloc->AllocLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(status = UDFLoadExtInfo(IrpContext, Vcb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &FELoc,
                       &FileInfo->Dloc->DataLoc, &FileInfo->Dloc->AllocLoc)))
        return status;
    FileInfo->Dloc->FileEntryLen = (uint32)
    (FileInfo->Dloc->FELoc.Length = (FileInfo->Dloc->DataLoc.Offset) ? FileInfo->Dloc->DataLoc.Offset :
                                                          FileInfo->Dloc->AllocLoc.Offset);
init_tree_entry:
    // Keep FE buffer at SectorSize — FlushFE may write full sector for in-ICB mode
    // init DirIndex
    if ( (FileType = ((icbtag*)((FileInfo->Dloc->FileEntry)+1))->fileType) != UDF_FILE_TYPE_DIRECTORY &&
        (FileType != UDF_FILE_TYPE_STREAMDIR) ) {
        UDFReferenceFile__(FileInfo);
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        return STATUS_SUCCESS;
    }
    // build index for directories
    if (!FileInfo->Dloc->DirIndex) {
        status = UDFIndexDirectory(IrpContext, Vcb, FileInfo);
        if (!NT_SUCCESS(status))
            return status;

    }
    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);

    return status;
} // end UDFOpenRootFile__()

/*
    This routine frees all memory blocks referenced by given FileInfo
 */
uint32
UDFCleanUpFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    PUDF_DATALOC_INFO Dloc;
    uint32 lc = 0;
    BOOLEAN IsASDir;
    BOOLEAN KeepDloc;
    PDIR_INDEX_ITEM DirNdx = NULL, DirNdx2;
    BOOLEAN Parallel = FALSE;
    BOOLEAN Linked = FALSE;
#ifdef UDF_DBG
    BOOLEAN Modified = FALSE;
    PDIR_INDEX_HDR hDirNdx;
    uint_di Index;
    PUDF_FILE_INFO DirInfo;
#endif // UDF_DBG

    if (!FileInfo) return UDF_FREE_FILEINFO;

    ValidateFileInfo(FileInfo);

    if (FileInfo->OpenCount || FileInfo->RefCount) {
        UDFPrint(("UDF: not all references are closed\n"));
        UDFPrint(("     Skipping cleanup\n"));
        UDFPrint(("UDF: OpenCount = %x, RefCount = %x, LinkRefCount = %x\n",
                              FileInfo->OpenCount,FileInfo->RefCount,FileInfo->Dloc->LinkRefCount));
        return UDF_FREE_NOTHING;
    }
    if (FileInfo->Fcb) {
        UDFPrint(("Operating System still has references to this file\n"));
        UDFPrint(("     Skipping cleanup\n"));
//        BrutePoint();
        return UDF_FREE_NOTHING;
    }

    /* A failed append flush still owns its entire generation. Keep the FI
     * reachable from the parent scan for explicit retry instead of freeing
     * its mapping or discarding its buffer during last-close teardown. */
    if (UDFDirectoryAppendPending(FileInfo) ||
        (FileInfo->Dloc && (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_CACHED_DATA_PENDING)))
        return UDF_FREE_NOTHING;

    IsASDir = UDFIsAStreamDir(FileInfo);

    if ((Dloc = FileInfo->Dloc)) {

#ifdef UDF_DBG
        DirInfo = FileInfo->ParentFile;
        if (DirInfo) {
            hDirNdx = DirInfo->Dloc->DirIndex;
            Index = FileInfo->Index;
            // we can't delete modified file
            // it should be closed & reopened (or flushed) before deletion
            DirNdx = UDFDirIndex(hDirNdx,Index);
            UDFPrint(("Cleanup Mod: %s%s%s%s%s%s\n",
                                 (Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ? "FE "       : "",
                                 (Dloc->DataLoc.Modified)                   ? "DataLoc "  : "",
                                 (Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) ? "Data-PreAlloc " : "",
                                 (Dloc->AllocLoc.Modified)                  ? "AllocLoc " : "",
                                 (Dloc->FELoc.Modified)                     ? "FELoc "    : "",
                                 (DirNdx && (DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) ? "FI " : ""
                                 ));
            Modified = ((Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
                         Dloc->DataLoc.Modified ||
                        (Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) ||
                         Dloc->AllocLoc.Modified ||
                         Dloc->FELoc.Modified ||
                        (DirNdx && (DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) );
        }
#endif // UDF_DBG

        PUDF_FILE_INFO ParFileInfo = UDFLocateAnyParallelFI(FileInfo);

        Parallel = (ParFileInfo != NULL);
        Linked = (FileInfo->NextLinkedFile != FileInfo);

//        Parallel = (FileInfo->NextLinkedFile != FileInfo);
        ASSERT(FileInfo->NextLinkedFile);
//        ASSERT(!Parallel);
        KeepDloc = (Dloc->LinkRefCount ||
                    Dloc->CommonFcb ||
                    Linked ) ?
                    TRUE : FALSE;

        if (Dloc->DirIndex) {
            uint_di i;
            for(i=2; (DirNdx = UDFDirIndex(Dloc->DirIndex,i)); i++) {
                if (DirNdx->FileInfo) {
                    if (!KeepDloc) {
                        // Orphaned FileInfo in DirIndex: loaded by
                        // UDFOpenFileInfoFromDirContext during path traversal
                        // but never opened as final component (no LCB).
                        // Recursively clean up instead of ASSERTing.
                        UDFPrint(("UDF: Cleaning orphaned FileInfo %x in DirIndex[%d]\n",
                                  DirNdx->FileInfo, i));
                        PUDF_FILE_INFO OrphanFi = DirNdx->FileInfo;
                        /* Do not discard a live cached-data retry owner as
                         * an orphan, even when an earlier lifetime defect
                         * left it outside the normal FCB/LCB hold graph. */
                        if (OrphanFi->Dloc &&
                            (OrphanFi->Dloc->FE_Flags & UDF_FE_FLAG_CACHED_DATA_PENDING))
                            return UDF_FREE_NOTHING;
                        // Orphan must have a valid Dloc (loaded by UDFOpenFileInfoFromDirContext)
                        ASSERT(OrphanFi->Dloc);
                        // Orphan should be self-linked (not part of a parallel/linked chain)
                        ASSERT(OrphanFi->NextLinkedFile == OrphanFi);
                        // ParentFile must point to the FileInfo we're currently cleaning up
                        ASSERT(OrphanFi->ParentFile == FileInfo);
                        // Clear CommonFcb to ensure KeepDloc=FALSE in recursive call
                        // (floating FCB may have set CommonFcb != NULL).
                        // The floating FCB itself will leak until unmount — acceptable.
                        if (OrphanFi->Dloc->CommonFcb) {
                            UDFPrint(("UDF: Orphan %x has floating CommonFcb %x\n",
                                      OrphanFi, OrphanFi->Dloc->CommonFcb));
                            OrphanFi->Dloc->CommonFcb = NULL;
                        }
                        // Clear Fcb so UDFCleanUpFile__ doesn't bail at line 2465
                        if (OrphanFi->Fcb) {
                            UDFPrint(("UDF: Orphan %x has floating Fcb %x\n",
                                      OrphanFi, OrphanFi->Fcb));
                            OrphanFi->Fcb->FileInfo = NULL;
                            OrphanFi->Fcb = NULL;
                        }
                        // Log original counts for diagnostics
                        UDFPrint(("UDF: Orphan counts: RefCount=%d, LinkRefCount=%d, OpenCount=%d\n",
                                  OrphanFi->RefCount,
                                  OrphanFi->Dloc->LinkRefCount,
                                  OrphanFi->OpenCount));
                        // Reset OpenCount — orphan's children (also orphans) may
                        // have incremented it via UDFReferenceFile__. These children
                        // will be recursively cleaned up by UDFCleanUpFile__.
                        OrphanFi->OpenCount = 0;
                        // Reset RefCount/LinkRefCount — may be non-zero if
                        // UDFCloseFile__ was never called for this orphan
                        OrphanFi->RefCount = 0;
                        OrphanFi->Dloc->LinkRefCount = 0;
                        // Clear Modified flags — orphan is being discarded along
                        // with the parent directory's DirIndex, so unflushed
                        // modifications would be lost anyway. Without this,
                        // recursive UDFCleanUpFile__ asserts !Modified when
                        // KeepDloc=FALSE (e.g., after rename-replace sets
                        // FI_MODIFIED on directory entries during UDFUnlinkFile__).
                        DirNdx->FI_Flags &= ~UDF_FI_FLAG_FI_MODIFIED;
                        OrphanFi->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
                        OrphanFi->Dloc->DataLoc.Modified = FALSE;
                        OrphanFi->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_PREALLOCATED;
                        OrphanFi->Dloc->AllocLoc.Modified = FALSE;
                        OrphanFi->Dloc->FELoc.Modified = FALSE;
                        // UDFCleanUpFile__ handles Dloc cleanup (frees via
                        // UDFRemoveDloc/UDFFreeDloc) and sets DirNdx->FileInfo=NULL
                        // in parent's DirIndex. We also free the FileInfo struct.
                        ULONG cleanupResult = UDFCleanUpFile__(Vcb, OrphanFi);
                        // Cleanup must have freed the Dloc (KeepDloc=FALSE path)
                        ASSERT(OrphanFi->Dloc == NULL);
                        ASSERT(cleanupResult & UDF_FREE_FILEINFO);
                        // Only free the FileInfo struct itself.
                        MyFreePool__(OrphanFi);
                        DirNdx->FileInfo = NULL;
                        continue;
                    }
                    // The file being cleaned up may have not closed Dirs
                    // (linked Dir). In this case each of them may have
                    // reference to FileInfo in DirIndex[1]
                    // Here we'll check it and change for valid value if
                    // necessary (Update Child Objects - I)
                    if (DirNdx->FileInfo->Dloc) {
                        // we can get here only when (Parallel == TRUE)
                        DirNdx2 = UDFDirIndex(DirNdx->FileInfo->Dloc->DirIndex, 1);
                        // It is enough to check DirNdx2->FileInfo only.
                        // If one of Parallel FI's has reference (and equal)
                        // to the FI being removed, it'll be removed from
                        // the chain & nothing wrong will happen.
                        if (DirNdx2 && (DirNdx2->FileInfo == FileInfo)) {
                            if (FileInfo->PrevLinkedFile == FileInfo) {
                                BrutePoint();
                                DirNdx2->FileInfo = NULL;
                            } else {
                                DirNdx2->FileInfo = Parallel ?
                                    ParFileInfo : FileInfo->PrevLinkedFile;
                            }
                            ASSERT(!DirNdx2->FileInfo->RefCount);
                        }
                    }
                }
            }
        }
        if (Dloc->SDirInfo) {
            UDFPrint(("UDF: Found not cleaned up reference (SDir).\n"));

            // (Update Child Objects - II)
            if (Dloc->SDirInfo->ParentFile == FileInfo) {
                BrutePoint();
                ASSERT(ParFileInfo);
                Dloc->SDirInfo->ParentFile = ParFileInfo;
            }
            // We should break Cleanup process if alive reference detected
            // and there is no possibility to store pointer in some other
            // place (in parallel object)
            if (!KeepDloc) {
                BrutePoint();
                UDFPrint(("     Skipping cleanup\n"));
                return UDF_FREE_NOTHING;
            }

            if (!UDFIsSDirDeleted(Dloc->SDirInfo) &&
               Dloc->SDirInfo->Dloc) {
                DirNdx2 = UDFDirIndex(Dloc->SDirInfo->Dloc->DirIndex, 1);
                if (DirNdx2 && (DirNdx2->FileInfo == FileInfo)) {
                    DirNdx2->FileInfo =
                        Parallel ? ParFileInfo : NULL;
                    ASSERT(!DirNdx2->FileInfo->RefCount);
                }
            }
        }

        if (!KeepDloc) {

            // Execute deferred block freeing — blocks were kept allocated
            // during cleanup to prevent reuse while FCB was in the table.
            // Safe to free now: this is the last Dloc reference (KeepDloc=FALSE).
            if (Dloc->FE_Flags & UDF_FE_FLAG_FREE_DEFERRED) {
                UDFUnlinkDloc(Vcb, Dloc);
                if (Dloc->DeletePrefree)
                    UDFReleasePrefreedAllocation(Vcb, NULL, FileInfo);
                else
                    UDFFreeFileAllocation(Vcb, NULL, FileInfo);
                Dloc->FE_Flags &= ~(UDF_FE_FLAG_FREE_DEFERRED | UDF_FE_FLAG_FE_MODIFIED);
            }

#ifdef UDF_DBG
            ASSERT(!Modified);
            ASSERT(!(Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED));
            ASSERT(!Dloc->DataLoc.Modified);
            ASSERT(!(Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED));
            ASSERT(!Dloc->AllocLoc.Modified);
            ASSERT(!Dloc->FELoc.Modified);
            ASSERT(!(DirNdx && (DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)));
#endif

#ifndef UDF_TRACK_ONDISK_ALLOCATION
            if (Dloc->DataLoc.Mapping)  MyFreePool__(Dloc->DataLoc.Mapping);
            if (Dloc->AllocLoc.Mapping) MyFreePool__(Dloc->AllocLoc.Mapping);
            if (Dloc->FELoc.Mapping)    MyFreePool__(Dloc->FELoc.Mapping);
            if (Dloc->FileEntry) {
                // plain file
                lc = UDFGetFileLinkCount(FileInfo);
                MyFreePool__(Dloc->FileEntry);
                Dloc->FileEntry = NULL;
            } else if (FileInfo->Index >= 2) {
                // error durring open operation
                lc = UDF_INVALID_LINK_COUNT;
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION
            if (FileInfo->Dloc->DirIndex) {
                uint_di i;
                for(i=2; (DirNdx = UDFDirIndex(Dloc->DirIndex,i)); i++) {
                    ASSERT(!DirNdx->FileInfo);
                    if (DirNdx->FName.Buffer)
                        MyFreePool__(DirNdx->FName.Buffer);
                }
                UDFDirIndexFree(Dloc->DirIndex);
                Dloc->DirIndex = NULL;
#ifdef UDF_TRACK_ONDISK_ALLOCATION
                UDFIndexDirectory(Vcb, FileInfo);
                if (FileInfo->Dloc->DirIndex) {
                    for(i=2; DirNdx = UDFDirIndex(Dloc->DirIndex,i); i++) {
                        ASSERT(!DirNdx->FileInfo);
                        if (DirNdx->FName.Buffer)
                            MyFreePool__(DirNdx->FName.Buffer);
                    }
                    UDFDirIndexFree(Dloc->DirIndex);
                    Dloc->DirIndex = NULL;
                }
#endif //UDF_TRACK_ONDISK_ALLOCATION
            }

#ifdef UDF_TRACK_ONDISK_ALLOCATION
            if (Dloc->AllocLoc.Mapping) MyFreePool__(Dloc->AllocLoc.Mapping);
            if (Dloc->FELoc.Mapping)    MyFreePool__(Dloc->FELoc.Mapping);
            if (Dloc->FileEntry) {
                // plain file
                lc = UDFGetFileLinkCount(FileInfo);
                MyFreePool__(Dloc->FileEntry);
                Dloc->FileEntry = NULL;
            } else if (FileInfo->Index >= 2) {
                // error durring open operation
                lc = UDF_INVALID_LINK_COUNT;
            }
            if (Dloc->DataLoc.Mapping) {
                if (lc && (lc != UDF_INVALID_LINK_COUNT)) {
                    UDFCheckSpaceAllocation(Vcb, 0, Dloc->DataLoc.Mapping, AS_USED); // check if used
                } else {
                    UDFCheckSpaceAllocation(Vcb, 0, Dloc->DataLoc.Mapping, AS_FREE); // check if free
                }
                MyFreePool__(Dloc->DataLoc.Mapping);
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION

            if (lc && (lc != UDF_INVALID_LINK_COUNT)) {
                UDFRemoveDloc(Vcb, Dloc);
            } else {
                UDFFreeDloc(Vcb, Dloc);
            }
        } else // KeepDloc cannot be FALSE if (Linked == TRUE)
        if (Linked) {
//            BrutePoint();
            // Update pointers in ParentObject (if any)
            if (FileInfo->ParentFile->Dloc->SDirInfo == FileInfo)
                FileInfo->ParentFile->Dloc->SDirInfo = FileInfo->PrevLinkedFile;
            DirNdx = UDFDirIndex(FileInfo->Dloc->DirIndex, 0);
            if (DirNdx && (DirNdx->FileInfo == FileInfo))
                DirNdx->FileInfo = FileInfo->PrevLinkedFile;
            DirNdx = UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex, FileInfo->Index);
            if (DirNdx && (DirNdx->FileInfo == FileInfo))
                DirNdx->FileInfo = ParFileInfo;
            // remove from linked chain
            FileInfo->NextLinkedFile->PrevLinkedFile = FileInfo->PrevLinkedFile;
            FileInfo->PrevLinkedFile->NextLinkedFile = FileInfo->NextLinkedFile;
            // update pointer in Dloc
            if (FileInfo->Dloc->LinkedFileInfo == FileInfo)
                FileInfo->Dloc->LinkedFileInfo = FileInfo->PrevLinkedFile;
        }
        FileInfo->Dloc = NULL;
    } else {
        KeepDloc = FALSE;
    }

    // Cleanup pointers in ParentObject (if any)
    if (IsASDir) {
        if (FileInfo->ParentFile->Dloc->SDirInfo == FileInfo) {
            ASSERT(!Linked);
            FileInfo->ParentFile->Dloc->SDirInfo = NULL;
            FileInfo->ParentFile->Dloc->FE_Flags &= ~UDF_FE_FLAG_HAS_DEL_SDIR;
        }
    } else
    if (FileInfo->ParentFile) {
        ASSERT(FileInfo->ParentFile->Dloc);
        DirNdx = UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex, FileInfo->Index);
        ASSERT(DirNdx);
#ifdef UDF_DBG
        PUDF_FILE_INFO OldFI;
        if (Parallel) {
            ASSERT(!DirNdx || !(OldFI = DirNdx->FileInfo) ||
                    !(OldFI == FileInfo));
        } else {
            ASSERT(!DirNdx || !(OldFI = DirNdx->FileInfo) ||
                     (OldFI == FileInfo));
        }
#endif
        if ( DirNdx && (DirNdx->FileInfo == FileInfo) ) {
            if (!Parallel)
                DirNdx->FileInfo = NULL;
#ifdef UDF_DBG
        } else {
            // We can get here after incomplete Open
            if (!Parallel && DirNdx->FileInfo)
                BrutePoint();
#endif
        }
#ifdef UDF_DBG
    } else {
//        BrutePoint();
#endif
    }

    if (!Parallel && FileInfo->FileIdent)
        MyFreePool__(FileInfo->FileIdent);
    FileInfo->FileIdent = NULL;
    // Kill reference to parent object
    FileInfo->ParentFile = NULL;
    // Kill references to parallel object(s) since it has no reference to
    // this one now
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    if (FileInfo->ListPtr)
        FileInfo->ListPtr->FileInfo = NULL;;
    return KeepDloc ? UDF_FREE_FILEINFO : (UDF_FREE_FILEINFO | UDF_FREE_DLOC);
} // end UDFCleanUpFile__()

/*
    This routine creates FileIdent record in destination directory &
    allocates FileEntry with in-ICB zero-sized data
    If it returns status != STATUS_SUCCESS caller should call UDFCleanUpFile__
    for returned pointer *WITHOUT* using UDFCloseFile__
 */
/* An appended name can reserve a deleted, reference-free FID before its
 * initial FE is written. This prevents later growth from exposing stale
 * bytes in the new slot. UNDER_INIT and then the existing FI/FCB owner
 * retain the FE; scalar/batched publication commits it before the valid FID.
 * Keep the immediate initial write when the reservation cannot be owned.
 */
static BOOLEAN
UDFMayDeferInitialFE(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, uint32 PartNum)
{
    PUDF_DATALOC_INFO dloc, parentDloc;
    PUDF_FILE_INFO parent;
    PFCB parentFcb;
    PDIR_INDEX_HDR header;
    PDIR_INDEX_ITEM index;
    uint32 frame, rel;

    if (!UDFMayDeferNewCreate(IrpContext, Vcb) ||
        FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH) ||
        FlagOn(Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA) ||
        !FileInfo || !(dloc = FileInfo->Dloc) || FileInfo->Fcb ||
        FileInfo->RefCount || FileInfo->OpenCount || dloc->LinkRefCount ||
        dloc->CommonFcb ||
        !FlagOn(dloc->FE_Flags, UDF_FE_FLAG_UNDER_INIT) ||
        !FlagOn(dloc->FE_Flags, UDF_FE_FLAG_FE_MODIFIED) ||
        !dloc->FileEntry || !UDFCanStageFileEntry(Vcb, FileInfo) ||
        dloc->DataLoc.Length || dloc->DataLoc.Offset != dloc->FileEntryLen ||
        dloc->FileEntryLen > Vcb->SectorSize ||
        ((icbtag*)(dloc->FileEntry + 1))->fileType != UDF_FILE_TYPE_REGULAR ||
        (((icbtag*)(dloc->FileEntry + 1))->flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB ||
        FileInfo->NextLinkedFile != FileInfo || FileInfo->PrevLinkedFile != FileInfo ||
        !(parent = FileInfo->ParentFile) || !(parentDloc = parent->Dloc) ||
        !(parentFcb = parent->Fcb) || parentDloc->CommonFcb != parentFcb ||
        parentFcb->FileInfo != parent || parentFcb->FcbReference <= 0 ||
        !FlagOn(parentFcb->FcbState, FCB_STATE_IN_FCB_TABLE) ||
        !FlagOn(parentFcb->FcbState, UDF_FCB_DIRECTORY | UDF_FCB_ROOT_DIRECTORY) ||
        FlagOn(parentFcb->FcbState, UDF_FCB_DELETED | UDF_FCB_DELETE_ON_CLOSE) ||
        UDFIsAStreamDir(parent) || !(header = parentDloc->DirIndex) ||
        parentDloc->DataLoc.Length < 0 ||
        !parentDloc->DirectoryCommitKnown || parentDloc->DirectoryCommittedLength < 0 ||
        parentDloc->DirectoryCommittedLength > parentDloc->DataLoc.Length ||
        FileInfo->Index < 2 || !FileInfo->FileIdent ||
        FileInfo->FileIdentLen < sizeof(FILE_IDENT_DESC) ||
        FileInfo->FileIdentLen > Vcb->SectorSize || PartNum >= Vcb->PartitionMaps)
        return FALSE;
    if (parentDloc->DataLoc.Offset &&
        (!UDFCanStageFileEntry(Vcb, parent) ||
         parentDloc->DataLoc.Offset != parentDloc->FileEntryLen ||
         !parentDloc->FileEntry ||
         (((icbtag*)(parentDloc->FileEntry + 1))->flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB ||
         (uint64)parentDloc->FileEntryLen + (uint64)parentDloc->DataLoc.Length > Vcb->SectorSize))
        return FALSE;
    frame = FileInfo->Index >> UDF_DIR_INDEX_FRAME_SH;
    rel = FileInfo->Index & (UDF_DIR_INDEX_FRAME - 1);
    if (frame >= header->FrameCount ||
        (frame + 1 == header->FrameCount && rel >= header->LastFrameCount))
        return FALSE;
    index = UDFDirIndex(header, FileInfo->Index);
    return index && index->FileInfo == FileInfo &&
        FlagOn(index->FI_Flags, UDF_FI_FLAG_FI_MODIFIED) &&
        index->Length == FileInfo->FileIdentLen &&
        (uint64)index->Offset == (uint64)parentDloc->DataLoc.Length &&
        (uint64)index->Offset >= (uint64)parentDloc->DirectoryCommittedLength &&
        !(index->FileCharacteristics & (FILE_DELETED | FILE_PARENT | FILE_DIRECTORY | FILE_METADATA)) &&
        FileInfo->FileIdent->descTag.tagIdent == TID_FILE_IDENT_DESC &&
        FileInfo->FileIdent->fileCharacteristics == index->FileCharacteristics &&
        FileInfo->FileIdent->icb.extLocation.partitionReferenceNum == PartNum &&
        FileInfo->FileIdent->icb.extLocation.logicalBlockNum ==
            UDFPhysLbaToPart(Vcb, PartNum, dloc->FELoc.Mapping[0].extLocation);
}

static NTSTATUS
UDFReserveInitialFid(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, uint32 PartNum, PBOOLEAN Deferred)
{
    PUDF_FILE_INFO parent = FileInfo->ParentFile;
    PDIR_INDEX_ITEM index;
    PFILE_IDENT_DESC tombstone;
    SIZE_T written = 0;
    uint32 lba, parentPart;
    NTSTATUS status = STATUS_SUCCESS;

    *Deferred = FALSE;
    if (!UDFMayDeferInitialFE(IrpContext, Vcb, FileInfo, PartNum))
        return STATUS_SUCCESS;
    index = UDFDirIndex(parent->Dloc->DirIndex, FileInfo->Index);
    lba = UDFExtentOffsetToLba(Vcb, parent->Dloc->DataLoc.Mapping,
                              index->Offset, NULL, NULL, NULL, NULL);
    parentPart = UDFGetRefPartNumByPhysLba(Vcb,
        parent->Dloc->FELoc.Mapping[0].extLocation);
    if (parentPart >= Vcb->PartitionMaps)
        return STATUS_FILE_CORRUPT_ERROR;
    tombstone = MyAllocatePool__(NonPagedPool, FileInfo->FileIdentLen);
    if (!tombstone)
        return STATUS_SUCCESS; // No new state accepted; immediate FE fallback.
    _SEH2_TRY {
        RtlCopyMemory(tombstone, FileInfo->FileIdent, FileInfo->FileIdentLen);
        tombstone->fileCharacteristics = FILE_DELETED;
        RtlZeroMemory(&tombstone->icb, sizeof(tombstone->icb));
        UDFSetUpTag(Vcb, &tombstone->descTag, (uint16)FileInfo->FileIdentLen,
                    lba == LBA_OUT_OF_EXTENT ? 0 :
                        UDFPhysLbaToPart(Vcb, parentPart, lba), 0);
        status = UDFReserveDirectoryFid(IrpContext, Vcb, parent, index->Offset,
            FileInfo->FileIdentLen, (int8*)tombstone, &written, Deferred);
        if (NT_SUCCESS(status) && *Deferred && written != FileInfo->FileIdentLen) {
            *Deferred = FALSE;
            status = STATUS_IO_DEVICE_ERROR;
        }
    } _SEH2_FINALLY {
        MyFreePool__(tombstone);
    } _SEH2_END;
    return status;
}

NTSTATUS
UDFCreateFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
//    IN uint16 AllocMode, // short/long/ext/in-icb  // always in-ICB
    IN BOOLEAN IgnoreCase,
    IN PUNICODE_STRING _fn,
    IN uint32 ExtAttrSz,
    IN uint32 ImpUseLen,
    IN BOOLEAN Extended,
    IN BOOLEAN CreateNew,
 IN OUT PUDF_FILE_INFO DirInfo,
    OUT PUDF_FILE_INFO* _FileInfo
    )
{
    uint32 l, d;
    uint_di i, j;
    NTSTATUS status;
    LONG_AD FEicb;
    UDF_DIR_SCAN_CONTEXT ScanContext;
    PDIR_INDEX_HDR hDirNdx = DirInfo->Dloc->DirIndex;
    PDIR_INDEX_ITEM DirNdx;
    uint32 LBS = Vcb->SectorSize;
    PUDF_FILE_INFO FileInfo;
    *_FileInfo = NULL;
    BOOLEAN undel = FALSE;
//    BOOLEAN PackDir = FALSE;
    BOOLEAN FEAllocated = FALSE;
    BOOLEAN NewNameAppend = FALSE;
    BOOLEAN InitialFEDeferred = FALSE;
    ULONGLONG NewUniqueId = 0;
    WCHAR AliasBuffer[12];
    UNICODE_STRING Alias;
    BOOLEAN AliasStored = FALSE;

    Alias.Buffer = AliasBuffer;
    Alias.Length = 0;
    Alias.MaximumLength = sizeof(AliasBuffer);

    ValidateFileInfo(DirInfo);
    *_FileInfo = NULL;

    ASSERT(DirInfo->Dloc->FELoc.Mapping[0].extLocation);
    uint32 PartNum = UDFGetRefPartNumByPhysLba(Vcb, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
    if (!hDirNdx) return STATUS_NOT_A_DIRECTORY;
    i = 0;

    _SEH2_TRY {

        // check if exists
        status = UDFFindFile(Vcb, IgnoreCase, FALSE, _fn, DirInfo, &i);
        DirNdx = UDFDirIndex(hDirNdx,i);
        if (NT_SUCCESS(status)) {
            // file is a Cur(Parent)Dir
            if (i<2) try_return (status = STATUS_ACCESS_DENIED);
            // file deleted
            if (UDFIsDeleted(DirNdx)) {
                j=0;
                if (NT_SUCCESS(UDFFindFile(Vcb, IgnoreCase, TRUE, _fn, DirInfo, &j))) {
                   i=j;
                   DirNdx = UDFDirIndex(hDirNdx,i);
                   goto CreateBothFound;
                }
                // we needn't allocating new FileIdent inside Dir stream
                // perform 'undel'
                if (DirNdx->FileInfo) {
    //                BrutePoint();
                    status = UDFPretendFileDeleted__(Vcb, DirNdx->FileInfo);
                    if (!NT_SUCCESS(status))
                        try_return (status = STATUS_FILE_DELETED);
                } else {
                    undel = TRUE;
                }
    //            BrutePoint();
                goto CreateUndel;
            }
CreateBothFound:
            // file already exists
            if (CreateNew) {
                try_return (status = STATUS_ACCESS_DENIED);
            }
            // try to open it
            BrutePoint();
            status = UDFOpenFile__(IrpContext, Vcb, IgnoreCase, TRUE, _fn, DirInfo, _FileInfo, &i);
    //        *_FileInfo = FileInfo; // OpenFile__ has already done it, so update it...
            DirNdx = UDFDirIndex(hDirNdx,i);
            DirNdx->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
            FileInfo = *_FileInfo;
            if (!NT_SUCCESS(status)) {
                // :(( can't open....
                if (FileInfo && UDFCleanUpFile__(Vcb, FileInfo)) {
                    MyFreePool__(FileInfo);
                    *_FileInfo = NULL;
                }
                BrutePoint();
                try_return (status);
            }
            // check if we can delete this file
            if (FileInfo->OpenCount || (FileInfo->RefCount > 1)) {
                BrutePoint();
                UDFCloseFile__(IrpContext, Vcb, FileInfo);
                try_return (status = STATUS_CANNOT_DELETE);
            }
            BrutePoint();
            // remove DIRECTORY flag
            DirNdx->FileCharacteristics &= ~FILE_DIRECTORY;
            FileInfo->FileIdent->fileCharacteristics &= ~FILE_DIRECTORY;
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            // truncate file size to ZERO
            status = UDFResizeFile__(IrpContext, Vcb, FileInfo, 0);
            if (!NT_SUCCESS(status)) {
                BrutePoint();
                UDFCloseFile__(IrpContext, Vcb, FileInfo);
            }
            // set NORMAL flag
            FileInfo->FileIdent->fileCharacteristics = 0;
            DirNdx->FileCharacteristics = 0;
            // update DeletedFiles counter in Directory... (for PackDir)
            if (undel && NT_SUCCESS(status))
                hDirNdx->DelCount--;
            try_return (status);
        }

CreateUndel:
        /* Reserve the ID before allocating or publishing a new FE/FID.
         * A failed journal write must leave the namespace untouched. */
        if (UDFIsAStreamDir(DirInfo))
            NewUniqueId = (ULONGLONG)UDFGetFileUID(DirInfo);
        else {
            status = UDFAllocateFileUID(IrpContext, Vcb, &NewUniqueId);
            if (!NT_SUCCESS(status))
                try_return(status);
        }

        // allocate FileInfo
        FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FE_TAG);
        *_FileInfo = FileInfo;
        if (!FileInfo)
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        /* Windows-style 8.3 alias, unique in this directory and stored in
         * the FID so it never changes. A failure only leaves the name
         * without a stored alias; it does not fail the create. */
        if (!UDFIsAStreamDir(DirInfo) &&
            NT_SUCCESS(UDFGenerateUniqueShortName(Vcb, DirInfo, _fn, &Alias)) &&
            Alias.Length && ImpUseLen < UDF_SHORT_NAME_IMPUSE_LEN)
            ImpUseLen = UDF_SHORT_NAME_IMPUSE_LEN;
        ImpUseLen = (ImpUseLen + 3) & ~((uint16)3);

        RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
        // init horizontal links
        FileInfo->NextLinkedFile =
        FileInfo->PrevLinkedFile = FileInfo;
        // allocate space for FileEntry
        if (!NT_SUCCESS(status =
            UDFBuildFileEntry(IrpContext, Vcb, DirInfo, FileInfo, PartNum, ICB_FLAG_AD_IN_ICB, ExtAttrSz, Extended) )) {
            BrutePoint();
            try_return (status);
        }
        FEAllocated = TRUE;
        FEicb.extLength = LBS;
        ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
        RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));

        if (!undel) {
            // build FileIdent
            if (!NT_SUCCESS(status =
                UDFBuildFileIdent(Vcb, _fn, &FEicb, ImpUseLen,
                        &(FileInfo->FileIdent), &(FileInfo->FileIdentLen)) ))
                try_return (status);
        } else {
            // read FileIdent
            FileInfo->FileIdent = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, DirNdx->Length, MEM_FID_TAG);
            if (!(FileInfo->FileIdent)) try_return (status = STATUS_INSUFFICIENT_RESOURCES);
            FileInfo->FileIdentLen = DirNdx->Length;
            if (!NT_SUCCESS(status = UDFReadFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset,
                                     DirNdx->Length, FALSE, (int8*)(FileInfo->FileIdent))))
                try_return (status);
            FileInfo->FileIdent->fileCharacteristics = 0;
            FileInfo->FileIdent->icb = FEicb;
            ImpUseLen = FileInfo->FileIdent->lengthOfImpUse;
            DirNdx->FileCharacteristics = 0;
        }
        if (Alias.Length)
            AliasStored = UDFWriteShortNameImpUse(FileInfo->FileIdent, &Alias);

        // try to find suitable unused FileIdent in DirIndex
        l = FileInfo->FileIdentLen;
        if (undel) goto CrF__2;
        // search for suitable unused (deleted) entry to reuse
        if (UDFDirIndexInitScan(DirInfo, &ScanContext, 2)) {
            while((DirNdx = UDFDirIndexScan(&ScanContext, NULL))) {
                if ((DirNdx->Length == l) && UDFIsDeleted(DirNdx) &&
                   !DirNdx->FileInfo ) {
                    // free unicode-buffer with old name
                    if (DirNdx->FName.Buffer) {
                        MyFreePool__(DirNdx->FName.Buffer);
                        DirNdx->FName.Buffer = NULL;
                    }
                    i = ScanContext.i;
                    goto CrF__1;
                }
            }
        }
        // no suitable deleted entry found, append new one
        i = UDFDirIndexGetLastIndex(hDirNdx);

        // append entry
        if (!NT_SUCCESS(status = UDFDirIndexGrow(&(DirInfo->Dloc->DirIndex), 1))) {
            try_return (status);
        }

        NewNameAppend = TRUE;
        // init offset of new FileIdent in directory Data extent
        hDirNdx = DirInfo->Dloc->DirIndex;
        if (i-1) {
            DirNdx = UDFDirIndex(hDirNdx,i-1);
            UDFDirIndex(hDirNdx,i)->Offset = DirNdx->Offset + DirNdx->Length;
            DirNdx = UDFDirIndex(hDirNdx,i);
        } else {
            DirNdx = UDFDirIndex(hDirNdx,i);
            DirNdx->Offset = 0;
        }
        // new terminator is recorded by UDFDirIndexGrow()
        if ( ((d = ((LBS - (DirNdx->Offset + l + DirInfo->Dloc->DataLoc.Offset)) & (LBS-1) )) < sizeof(FILE_IDENT_DESC)) &&
              d ) {
            // insufficient space at the end of last sector for
            // next FileIdent's tag. fill it with ImpUse data

            // generally, all data should be DWORD-aligned, but if it is not so
            // this opearation will help us to avoid glitches
            d = (d+3) & ~((uint32)3);

            uint32 IUl = FileInfo->FileIdent->lengthOfImpUse;
            uint32 FIl = FileInfo->FileIdent->lengthFileIdent;
            uint32 NewIUl = max(IUl + d, 32);
            d = NewIUl - IUl;
            if (!MyReallocPool__((PCHAR)(FileInfo->FileIdent), l, (PCHAR*)&(FileInfo->FileIdent), l+d )) {
                try_return (status = STATUS_INSUFFICIENT_RESOURCES);
            }

            l += d;
            
            RtlMoveMemory(((int8*)(FileInfo->FileIdent+1))+NewIUl,
                          ((int8*)(FileInfo->FileIdent+1))+IUl, FIl);

            RtlZeroMemory(((int8*)(FileInfo->FileIdent+1))+IUl, d);

            RtlZeroMemory(((int8*)(FileInfo->FileIdent+1))+NewIUl+FIl,
                          l - sizeof(FILE_IDENT_DESC) - NewIUl - FIl);

            if (!IUl) {
                UDFSetEntityID_imp((EntityID*)(FileInfo->FileIdent+1), UDF_ID_DEVELOPER);
            }

            FileInfo->FileIdent->lengthOfImpUse = (uint16)NewIUl;
            FileInfo->FileIdentLen = l;
        }
        DirNdx->Length = l;
CrF__1:
        // clone unicode string
        // it  **<<MUST>>**  be allocated with internal memory manager
        DirNdx->FName.Buffer = (PWCHAR)MyAllocatePoolTag__(UDF_FILENAME_MT, (DirNdx->FName.MaximumLength = _fn->Length + sizeof(WCHAR)), MEM_FNAMECPY_TAG);
        DirNdx->FName.Length = _fn->Length;
        if (!DirNdx->FName.Buffer)
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        RtlCopyMemory(DirNdx->FName.Buffer, _fn->Buffer, _fn->Length);
        DirNdx->FName.Buffer[_fn->Length/sizeof(WCHAR)] = 0;
CrF__2:
        // Reset all flags: stale FI_INTERNAL from deleted entry would
        // hide the new file from enumeration.
        DirNdx->FI_Flags = UDFBuildHashEntry(Vcb, &(DirNdx->FName), &(DirNdx->hashes), HASH_ALL)
                         | UDF_FI_FLAG_FI_MODIFIED;
        if (AliasStored)
            UDFSetDirNdxShortName(DirNdx, &Alias);
        else if (!UDFNeedsShortName(&DirNdx->FName))
            UDFSetDirNdxShortName(DirNdx, NULL);
        // we get here immediately when 'undel' occured
        FileInfo->Index = i;
        ASSERT(!DirNdx->FileInfo);
        DirNdx->FileInfo = FileInfo;
        DirNdx->FileEntryLoc = FEicb.extLocation;
        // mark file as 'deleted' for now
        DirNdx->FileCharacteristics = FILE_DELETED;
        FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
        FileInfo->Dloc->DataLoc.Mapping = UDFExtentToMapping(&(FileInfo->Dloc->FELoc.Mapping[0]));
        if (!(FileInfo->Dloc->DataLoc.Mapping)) {
            UDFFlushFI(IrpContext, Vcb, FileInfo, PartNum);
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        }
        FileInfo->Dloc->DataLoc.Length = 0;
        FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
        FileInfo->ParentFile = DirInfo;
        // init FileEntry
        UDFSetFileUID(FileInfo, NewUniqueId);
        UDFSetFileSize(FileInfo, 0);
        UDFIncFileLinkCount(FileInfo); // increase to 1
        if (UDFIsAStreamDir(DirInfo))
            ((icbtag*)(FileInfo->Dloc->FileEntry+1))->flags |= ICB_FLAG_STREAM;
        UDFUpdateCreateTime(Vcb, FileInfo);
        UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index),
                             FileInfo->Dloc->FileEntry, Vcb->DefaultAttr);
        FileInfo->Dloc->DataLoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
        FileInfo->Dloc->DataLoc.Modified = TRUE;
        FileInfo->Dloc->FELoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
        /* UDFBuildFileEntry zeroes its sector-sized buffer.  UDFFlushFE below
           writes the complete in-ICB FE sector, including its zero tail, so
           a separate zero-sector write here is immediately overwritten. */
#if 0
        if ((i >= 2) && (DirNdx->FName.Buffer[0] == L'.')) {
            BrutePoint();
        }
#endif

#ifdef UDF_CHECK_DISK_ALLOCATION
        if (  /*FileInfo->Fcb &&*/
             (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot))) {

            if (!FileInfo->FileIdent ||
               !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
                AdPrint(("Flushing to Discarded block %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
                BrutePoint();
            }
        }
#endif // UDF_CHECK_DISK_ALLOCATION

        // make FileIdent valid
        FileInfo->FileIdent->fileCharacteristics = 0;
        DirNdx->FileCharacteristics = 0;
        /* A deferred initial FE remains owned and dirty until the existing
         * scalar or batch FE-before-FID publication boundary. No valid FID
         * may be emitted merely because this private buffer was initialized.
         * Reused slots and unsupported ownership retain the immediate write. */
        status = STATUS_SUCCESS;
        if (NewNameAppend && !undel)
            status = UDFReserveInitialFid(IrpContext, Vcb, FileInfo,
                                          PartNum, &InitialFEDeferred);
        if (NT_SUCCESS(status) && InitialFEDeferred) {
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_NEW_UNPUBLISHED;
            UDF_METADATA_COUNT(Vcb,60);
        } else if (NT_SUCCESS(status)) {
            status = UDFFlushFE(IrpContext, Vcb, FileInfo, PartNum);
        }
        if (NT_SUCCESS(status) && NewNameAppend && !undel &&
            !UDFIsAStreamDir(DirInfo) && UDFMayDeferNewCreate(IrpContext, Vcb))
        {
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_NEW_UNPUBLISHED;
            UDF_METADATA_COUNT(Vcb,30);
        }
        if (!NT_SUCCESS(status)) {
            /* The new FE was not committed.  Keep its directory entry a
               tombstone before FINALLY releases the FE allocation. */
            FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
            DirNdx->FileCharacteristics |= FILE_DELETED;
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            UDFFlushFI(IrpContext, Vcb, FileInfo, PartNum);
            try_return (status);
        }
        UDFReferenceFile__(FileInfo);
        if (undel)
            hDirNdx->DelCount--;
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        if (!UDFIsAStreamDir(DirInfo))
            UDFIncFileCounter(Vcb);

        UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used

        try_return (status = STATUS_SUCCESS);

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        if (!NT_SUCCESS(status)) {
            if (FEAllocated && FileInfo->Dloc) {
                // Free FE space first (needs Dloc->FELoc), then remove Dloc entry
                UDFFreeFESpace(Vcb, DirInfo, &(FileInfo->Dloc->FELoc));
                UDFRemoveDloc(Vcb, FileInfo->Dloc);
                FileInfo->Dloc = NULL;
            }
        }
    } _SEH2_END
    return status;

} // end UDFCreateFile__()

/*
    This routine reads data from file described by FileInfo
 */
/*__inline
NTSTATUS
UDFReadFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    OUT int8* Buffer,
    OUT PSIZE_T ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFReadExtent(Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, Buffer, ReadBytes);
} // end UDFReadFile__()*/

/*
    This routine zeros data in file described by FileInfo
 */
__inline
NTSTATUS
UDFZeroFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    OUT PSIZE_T ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFZeroExtent__(IrpContext, Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, ReadBytes);
} // end UDFZeroFile__()*/

/*
    This routine makes sparse area in file described by FileInfo
 */
__inline
NTSTATUS
UDFSparseFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    OUT PSIZE_T ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFSparseExtent__(IrpContext, Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, ReadBytes);
} // end UDFSparseFile__()*/

/*
    This routine fills tails of the last sector in extent with ZEROs
 */
NTSTATUS
UDFPadLastSector(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo
    )
{
    if (!ExtInfo || !(ExtInfo->Mapping) || !(ExtInfo->Length)) return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    SIZE_T to_write, WrittenBytes;
    uint32 Lba, sect_offs, flags;
    NTSTATUS status;
    // Length should not be zero
    int64 Offset = ExtInfo->Length + ExtInfo->Offset;
    // data is sector-size-aligned, we needn't any padding
    if (Offset && !((uint32)Offset & (Vcb->SectorSize-1) )) return STATUS_SUCCESS;
    // get Lba of the last sector
    Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
    // EOF check. If we have valid ExtInfo this will not happen, but who knows..
    if ((Lba == (uint32)-1) ||
       (flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED))
        return STATUS_END_OF_FILE;
    // write tail
    status = UDFWriteData(IrpContext, Vcb, TRUE, (((int64)Lba) << Vcb->SectorShift) + sect_offs, to_write, FALSE, Vcb->ZBuffer, &WrittenBytes);
    return status;
} // UDFPadLastSector()

/*
    This routine updates AllocDesc sequence, FileIdent & FileEntry
    for given file
 */
static NTSTATUS
UDFCloseFile__Internal(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    if (!FileInfo) return STATUS_SUCCESS;
    if (FileInfo->Index<2 && (FileInfo->ParentFile) && !UDFIsAStreamDir(FileInfo)) {
        UDFPrint(("Closing Current or Parent Directory... :-\\\n"));
        if (FileInfo->RefCount) {
            InterlockedDecrement((PLONG)&FileInfo->RefCount);
            ASSERT(FileInfo->Dloc);
            if (FileInfo->Dloc)
                InterlockedDecrement((PLONG)&FileInfo->Dloc->LinkRefCount);
#ifdef UDF_DBG
        } else {
            BrutePoint();
            UDFPrint(("ERROR: Closing unreferenced file!\n"));
#endif // UDF_DBG
        }
        if (FileInfo->ParentFile->OpenCount) {
            InterlockedDecrement((PLONG)&FileInfo->ParentFile->OpenCount);
#ifdef UDF_DBG
        } else {
            BrutePoint();
            UDFPrint(("ERROR: Closing unopened file!\n"));
#endif // UDF_DBG
        }
        return STATUS_SUCCESS;
    }
    PUDF_FILE_INFO DirInfo = FileInfo->ParentFile;
    NTSTATUS status;
    uint32 PartNum;
    if (FileInfo->RefCount) {
        InterlockedDecrement((PLONG)&FileInfo->RefCount);
        ASSERT(FileInfo->Dloc);
        if (FileInfo->Dloc)
            InterlockedDecrement((PLONG)&FileInfo->Dloc->LinkRefCount);
#ifdef UDF_DBG
    } else {
        BrutePoint();
        UDFPrint(("ERROR: Closing unreferenced file!\n"));
#endif // UDF_DBG
    }
    if (DirInfo) {
        // validate DirInfo
        ValidateFileInfo(DirInfo);

        if (DirInfo->OpenCount) {
            InterlockedDecrement((PLONG)&DirInfo->OpenCount);
#ifdef UDF_DBG
        } else {
            BrutePoint();
            UDFPrint(("ERROR: Closing unopened file!\n"));
#endif // UDF_DBG
        }
    }
    // If the file has gone (unlinked) we should return STATUS_SUCCESS here.
    if (!FileInfo->Dloc) return STATUS_SUCCESS;

    if (FileInfo->RefCount ||
       FileInfo->OpenCount ||
       !(FileInfo->Dloc->FELoc.Mapping)) return STATUS_SUCCESS;
    if (UDFDeferNewFileMetadata(IrpContext, Vcb, FileInfo))
        return STATUS_SUCCESS;
    /* Cleanup already consumed this FI reference. Its FILE_OBJECT still
     * holds the indexed FCB until IRP_CLOSE; do not immediately undo a
     * proved attribute-only cleanup exception. Final close/flush stay eager. */
    if (UDFCanRetainDirectoryAttributesClose(IrpContext, Vcb, FileInfo))
        return STATUS_SUCCESS;
    status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status))
        return status;
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = UDFGetRefPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if (PartNum == (uint32)-1) {
        UDFPrint(("  Is DELETED ?\n"));
        if (DirInfo) {
            PartNum = UDFGetRefPartNumByPhysLba(Vcb, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
        } else {
            BrutePoint();
        }
    }
#ifdef UDF_CHECK_DISK_ALLOCATION
    if (  FileInfo->Fcb &&
         (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot))) {

        //ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        if (UDFIsAStreamDir(FileInfo)) {
            if (!UDFIsSDirDeleted(FileInfo)) {
                UDFPrint(("  Not DELETED SDir\n"));
                BrutePoint();
            }
            ASSERT(!FileInfo->Dloc->FELoc.Modified);
        } else
        if (!FileInfo->FileIdent ||
           !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
            if (!FileInfo->FileIdent) {
                AdPrint(("  No FileIdent\n"));
            }
            if (FileInfo->FileIdent &&
               !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED))
                AdPrint(("  Not DELETED\n"));
            ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
            AdPrint(("Flushing to Discarded block %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
            BrutePoint();
        } else {
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_FREE); // check if free
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->FELoc.Mapping, AS_FREE); // check if free
        }
    } else {
        if (!FileInfo->Dloc->FELoc.Mapping[0].extLocation ||
            (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot))) {
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_FREE); // check if free
        } else {
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used
        }
    }
#endif // UDF_CHECK_DISK_ALLOCATION

    // we needn't flushing FE & Allocs untill all links are closed...
    if (!FileInfo->Dloc->LinkRefCount) {

        // flush pre-allocation charge for directories
        if (FileInfo->Dloc &&
           FileInfo->Dloc->DirIndex) {

            if (FileInfo->Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) {
                FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
                status = UDFResizeExtent(IrpContext, Vcb, PartNum, UDFGetFileSize(FileInfo), FALSE, &(FileInfo->Dloc->DataLoc));
                FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_CUT_PREALLOCATED;
                if (!NT_SUCCESS(status))
                    return status;
                FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_PREALLOCATED;
                AdPrint(("Dir pre-alloc truncated (Close)\n"));
                FileInfo->Dloc->DataLoc.Modified = TRUE;
            }
        }

        if (!NT_SUCCESS(status = UDFFlushFE(IrpContext, Vcb, FileInfo, PartNum))) {
            UDFPrint(("Error flushing FE\n"));
            /* A failed FE write is retryable.  Turning its FID into a
             * deletion tombstone would make the file disappear on remount. */
            return status;
        }
    }
    // ... but FI must be updated (if any)
    if (!NT_SUCCESS(status = UDFFlushFI(IrpContext, Vcb, FileInfo, PartNum))) {
        UDFPrint(("Error flushing FI\n"));
        return status;
    }
#ifdef UDF_DBG
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if ((FileInfo->Dloc->FileEntry->descVersion != 2) &&
       (FileInfo->Dloc->FileEntry->descVersion != 3)) {
        ASSERT((Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot)));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFCloseFile__Internal()

NTSTATUS
UDFCloseFile__(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_FILE_INFO FileInfo)
{
    NTSTATUS status = UDFCloseFile__Internal(IrpContext, Vcb, FileInfo);
    UDFRememberMetadataWritebackError(IrpContext, Vcb, FileInfo, status);
    return status;
}

/*
    Helper function to handle parent FID updates
*/
static
NTSTATUS
UDFUpdateParentFID(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_IDENT_DESC fid = NULL;
    PDIR_INDEX_ITEM DirNdx;
    int8* Buf = NULL;
    uint32 PartNum, ParentLBA;
    SIZE_T WrittenBytes = 0;

    DirNdx = UDFDirIndex(DirInfo->Dloc->DirIndex, 1);
    if (!DirNdx)
        return STATUS_FILE_CORRUPT_ERROR;

    Buf = (int8*)MyAllocatePool__(NonPagedPool, DirNdx->Length);
    if (!Buf)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = UDFReadFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset, DirNdx->Length, FALSE, Buf);
    if (!NT_SUCCESS(status))
        goto cleanup;

    fid = (PFILE_IDENT_DESC)Buf;
    if (!(fid->fileCharacteristics & FILE_PARENT)) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto cleanup;
    }
    
    ParentLBA = DirInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation;
    ASSERT(ParentLBA);
    PartNum = UDFGetRefPartNumByPhysLba(Vcb, ParentLBA);
    fid->icb.extLength = Vcb->SectorSize;
    fid->icb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, ParentLBA);
    fid->icb.extLocation.partitionReferenceNum = (uint16)PartNum;
    RtlZeroMemory(&(fid->icb.impUse), sizeof(fid->icb.impUse));

    ((struct FidADImpUse*)&(fid->icb.impUse))->uniqueID = 
        (uint32)UDFGetFileUID(DirInfo->ParentFile);

    UDFSetUpTag(Vcb, &fid->descTag, (uint16)DirNdx->Length, fid->descTag.tagLocation, 0);

    status = UDFWriteFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset, DirNdx->Length, FALSE, Buf, &WrittenBytes);
    if (!NT_SUCCESS(status))
        goto cleanup;

    DirNdx->FileInfo = DirInfo->ParentFile;
    DirNdx->FileEntryLoc = fid->icb.extLocation;

cleanup:
    MyFreePool__(Buf);
    return status;
}

/*
    This routine moves file from DirInfo1 to DirInfo2 & renames it to fn
 */
NTSTATUS
UDFRenameMoveFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN IgnoreCase,
    IN OUT BOOLEAN* Replace,        // replace if destination file exists
    IN PUNICODE_STRING fn,       // destination
    IN OUT PUDF_FILE_INFO DirInfo1,
    IN OUT PUDF_FILE_INFO DirInfo2,
    IN OUT PUDF_FILE_INFO FileInfo  // source (opened)
    )
{
    PUDF_FILE_INFO FileInfo2;
    NTSTATUS status;
    PDIR_INDEX_ITEM DirNdx1;
    PDIR_INDEX_ITEM DirNdx2;
    uint_di i,j;
    BOOLEAN Recovery = FALSE;
    BOOLEAN SameFE = FALSE;
    uint32 NTAttr = 0;
    long_ad SrcIcb;

    // validate FileInfo
    ValidateFileInfo(DirInfo1);
    ValidateFileInfo(DirInfo2);
    ValidateFileInfo(FileInfo);
    if (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) {
        status = UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
        if (!NT_SUCCESS(status))
            return status;
    }


    i = j = 0;
    if (DirInfo1 == DirInfo2) {
        if (NT_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &j)) &&
           (j==FileInfo->Index) ) {
            // case-only rename
            uint8* CS0;
            SIZE_T Nlen, /* l, FIXME ReactOS */ IUl;

            // prepare filename
            UDFCompressUnicode(fn, &CS0, &Nlen);
            if (!CS0) return STATUS_INSUFFICIENT_RESOURCES;
/*            if (Nlen > UDF_NAME_LEN) {
                if (CS0) MyFreePool__(CS0);
                return STATUS_OBJECT_NAME_INVALID;
            }*/
            // allocate memory for FI
            DirNdx2 = UDFDirIndex(DirInfo2->Dloc->DirIndex,j);
            IUl = DirNdx2->FileInfo->FileIdent->lengthOfImpUse;
#if 0
            l = (sizeof(FILE_IDENT_DESC) + Nlen + IUl + 3) & ~((uint32)3);
#endif

            RtlCopyMemory( ((uint8*)(DirNdx2->FileInfo->FileIdent+1))+IUl, CS0, Nlen);
            RtlCopyMemory(DirNdx2->FName.Buffer, fn->Buffer, fn->Length);

            if (CS0) MyFreePool__(CS0);

            DirNdx2->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            UDFBuildHashEntry(Vcb, &(DirNdx2->FName), &(DirNdx2->hashes), HASH_ALL);
            /* A case change keeps the 8.3 alias; restore its hash. */
            if ((DirNdx2->FI_Flags & UDF_FI_FLAG_SHORT_NAME) && DirNdx2->ShortNameLength) {
                WCHAR Kept[12];
                UNICODE_STRING KeptAlias;
                RtlCopyMemory(Kept, DirNdx2->ShortName, DirNdx2->ShortNameLength);
                KeptAlias.Buffer = Kept;
                KeptAlias.Length = KeptAlias.MaximumLength = DirNdx2->ShortNameLength;
                UDFSetDirNdxShortName(DirNdx2, &KeptAlias);
            }
            return STATUS_SUCCESS;
/*        } else
        if (!NT_SUCCESS(status) && (fn->Length == UDFDirIndex(DirInfo2->Dloc->DirIndex, j=FileInfo->Index)->FName.Length)) {
            // target file doesn't exist, but name lengthes are equal
            RtlCopyMemory((DirNdx1 = UDFDirIndex(DirInfo1->Dloc->DirIndex,j))->FName.Buffer, fn->Buffer, fn->Length);
            DirNdx1->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            UDFBuildHashEntry(Vcb, &(DirNdx1->FName), &(DirNdx1->hashes), HASH_ALL);
            return STATUS_SUCCESS;*/
        }
    }

    // PHASE 0
    // try to create new FileIdent & FileEntry in Dir2

RenameRetry:
    if (!NT_SUCCESS(status = UDFCreateFile__(IrpContext, Vcb, IgnoreCase, fn, UDFGetFileEALength(FileInfo),
                    0, (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY),
                    TRUE, DirInfo2, &FileInfo2))) {
        UDFCleanUpFile__(Vcb, FileInfo2);
        if (FileInfo2) MyFreePool__(FileInfo2);
        if (status == STATUS_ACCESS_DENIED) {
            // try to recover >;->
            if ((*Replace) && !Recovery) {
                Recovery = TRUE;
                status = UDFOpenFile__(IrpContext, Vcb, IgnoreCase, TRUE, fn, DirInfo2, &FileInfo2, NULL);
                if (!NT_SUCCESS(status)) {

                    goto cleanup_and_abort_rename;
                }
                // file opened successfully
;
                status = UDFDoesOSAllowFileToBeTargetForRename__(FileInfo2);

                if (!NT_SUCCESS(status)) {

                    UDFCloseFile__(IrpContext, Vcb, FileInfo2);
                    goto cleanup_and_abort_rename;
                }

                // TODO: Refactor — don't use UDFCreateFile__ in rename path.
                // Use direct FID unlink + create link instead.
                // Quick fix: if target's Dloc == source's Dloc, the internal open got the
                // wrong file (shared Dloc bug). Unlink would destroy the source data.
                if (FileInfo2->Dloc == FileInfo->Dloc) {

                    UDFCloseFile__(IrpContext, Vcb, FileInfo2);
                    status = STATUS_ACCESS_DENIED;
                    goto cleanup_and_abort_rename;
                }
                status = UDFUnlinkFile__(IrpContext, Vcb, FileInfo2, TRUE);

                // Mark FCB as deleted to prevent it from going into delayed close queue
                if (FileInfo2->Fcb) {
 
                    FileInfo2->Fcb->FcbState |= UDF_FCB_DELETED;
                }
                UDFCloseFile__(IrpContext, Vcb, FileInfo2);

                if (UDFCleanUpFile__(Vcb, FileInfo2)) {

                    MyFreePool__(FileInfo2);
                    FileInfo2 = NULL;
                    if (SameFE)
                        return status;
                } else {
                    // we get here if the FileInfo has associated
                    // system-specific Fcb
                    // Such fact means that not all system references
                    // has already gone (except Linked file case)

                    // Note: After UDFUnlinkFile__ link count is 0, which is expected
                    // Only return ACCESS_DENIED if unlink itself failed
                    if (!NT_SUCCESS(status))
                        status = STATUS_ACCESS_DENIED;
                }

                if (NT_SUCCESS(status)) goto RenameRetry;
cleanup_and_abort_rename:

                if (FileInfo2 && UDFCleanUpFile__(Vcb, FileInfo2)) {
                    MyFreePool__(FileInfo2);
                    FileInfo2 = NULL;
                }
            } else {
                status = STATUS_OBJECT_NAME_COLLISION;
            }
        }
        return status;
    }
    // update pointers
    DirNdx1 = UDFDirIndex(DirInfo1->Dloc->DirIndex, i = FileInfo->Index);
    DirNdx2 = UDFDirIndex(DirInfo2->Dloc->DirIndex, j = FileInfo2->Index);

    // copy file attributes to newly created FileIdent
    NTAttr = UDFAttributesToNT(DirNdx1, FileInfo->Dloc->FileEntry);
    FileInfo2->FileIdent->fileVersionNum = FileInfo->FileIdent->fileVersionNum;
    SrcIcb = FileInfo->FileIdent->icb;
    // unlink source FileIdent
    if (!NT_SUCCESS(status = UDFUnlinkFile__(IrpContext, Vcb, FileInfo, FALSE))) {
        // kill newly created entry
        UDFFlushFile__(IrpContext, Vcb, FileInfo2, 0);
        UDFUnlinkFile__(IrpContext, Vcb, FileInfo2, TRUE);
        UDFCloseFile__(IrpContext, Vcb, FileInfo2);
        UDFCleanUpFile__(Vcb, FileInfo2);
        MyFreePool__(FileInfo2);
        return status;
    }

    // PHASE 1
    // drop all unnecessary info from FileInfo & flush FI

    DirNdx1->FileInfo = NULL;
    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    UDFFlushFI(IrpContext, Vcb, FileInfo, UDFGetRefPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation));
    InterlockedExchangeAdd((PLONG)&(DirInfo1->OpenCount),
                          -((LONG)(FileInfo->RefCount)));
    // PHASE 2
    // copy all necessary info from FileInfo to FileInfo2

    FileInfo2->FileIdent->icb = SrcIcb;
    FileInfo2->FileIdent->fileCharacteristics = FileInfo->FileIdent->fileCharacteristics;
    FileInfo2->FileIdent->fileVersionNum = FileInfo->FileIdent->fileVersionNum;
    MyFreePool__(FileInfo->FileIdent);
    FileInfo->FileIdent = NULL;

    // PHASE 3
    // copy all necessary info from FileInfo2 to FileInfo

    DirNdx2->FileInfo = FileInfo;
    DirNdx2->FileCharacteristics = DirNdx1->FileCharacteristics & ~FILE_DELETED;
    DirNdx2->FileEntryLoc = DirNdx1->FileEntryLoc;
    DirNdx2->FI_Flags = (DirNdx1->FI_Flags & ~UDF_FI_FLAG_SYS_ATTR) | UDF_FI_FLAG_FI_MODIFIED;
    InterlockedExchangeAdd((PLONG)&(DirInfo2->OpenCount),
                           FileInfo->RefCount - FileInfo2->RefCount);

    UDFAttributesToUDF(DirNdx2, FileInfo2->Dloc->FileEntry, NTAttr);

    FileInfo->Index = j;
    FileInfo->FileIdent = FileInfo2->FileIdent;
    FileInfo->FileIdentLen = FileInfo2->FileIdentLen;
    FileInfo->ParentFile = DirInfo2;
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

    ((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation =
        ((icbtag*)(FileInfo2->Dloc->FileEntry+1))->parentICBLocation;

    UDFIncFileLinkCount(FileInfo); // increase to 1

    // UDFCreateFile__ counts every new entry like a file but,
    // UDFUnlinkFile__ removes dir and old parent link
    if (UDFIsADirectory(FileInfo)) {
        UDFDecFileCounter(Vcb);
        UDFIncDirCounter(Vcb);
        UDFIncFileLinkCount(DirInfo2);

        if (DirInfo1 != DirInfo2) {
            status = UDFUpdateParentFID(IrpContext, Vcb, FileInfo);
            if(!NT_SUCCESS(status)) {
                
                UDFPrint(("UDFRenameMoveFile__: Updating parent FID failed: %08lx\n", status));
            }
        }
    }

//    UDFUpdateModifyTime(Vcb, FileInfo);

    // PHASE 4
    // drop all unnecessary info from FileInfo2

    // Skip if UDFUnlinkFile__ already freed the FE via free allocation path
    if (FileInfo2->Dloc->FELoc.Mapping[0].extLocation) {
        UDFFreeFESpace(Vcb, DirInfo2, &(FileInfo2->Dloc->FELoc));
    }
    UDFUnlinkDloc(Vcb, FileInfo2->Dloc);
    UDFDecFileLinkCount(FileInfo2);

    FileInfo2->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
/*    MyFreePool__(FileInfo2->Dloc->FileEntry);
    FileInfo2->Dloc->FileEntry = NULL;*/
    FileInfo2->ParentFile = NULL;
    FileInfo2->FileIdent = NULL;
    FileInfo2->RefCount = 0;
    FileInfo2->Dloc->LinkRefCount = 0;
    ASSERT(FileInfo2->Dloc->DataLoc.Mapping);
    FileInfo2->Dloc->DataLoc.Mapping[0].extLocation = 0;
    FileInfo2->Dloc->DataLoc.Mapping[0].extLength = 0;

    UDFCleanUpFile__(Vcb, FileInfo2);
    MyFreePool__(FileInfo2);

    // return 'delete target' status
    (*Replace) = Recovery;

    return STATUS_SUCCESS;
} // end UDFRenameMoveFile__()

/*
    This routine transforms zero-sized file to directory
 */
NTSTATUS
UDFRecordDirectory__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PUDF_FILE_INFO DirInfo   // source (opened)
    )
{
    NTSTATUS status;
    LONG_AD FEicb;
    UDF_FILE_INFO FileInfo;
    UDF_DATALOC_INFO Dloc;
    UNICODE_STRING PName;
    uint32 PartNum;
    SIZE_T WrittenBytes;
    PDIR_INDEX_ITEM CurDirNdx;
    uint32 lba, ParentLBA, TagLoc;

    // validate DirInfo
    ValidateFileInfo(DirInfo);
    if (DirInfo->ParentFile && UDFIsAStreamDir(DirInfo->ParentFile))
        return STATUS_ACCESS_DENIED;
    // file should be empty
    if (UDFGetFileSize(DirInfo)) {
        if ( DirInfo->FileIdent &&
           (DirInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY)) return STATUS_FILE_IS_A_DIRECTORY;
        return STATUS_NOT_A_DIRECTORY;
    }
    if (DirInfo->Dloc->DirIndex) return STATUS_FILE_IS_A_DIRECTORY;
    // create empty DirIndex
    if (DirInfo->FileIdent) DirInfo->FileIdent->fileCharacteristics |= FILE_DIRECTORY;
    // FID of subdir parent counts as a link to the parent
    UDFIncFileLinkCount(DirInfo->ParentFile);
    if ((CurDirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(DirInfo),DirInfo->Index)))
        CurDirNdx->FileCharacteristics |= FILE_DIRECTORY;
    ((icbtag*)(DirInfo->Dloc->FileEntry+1))->fileType = UDF_FILE_TYPE_DIRECTORY;
    // init temporary FileInfo
    RtlZeroMemory(&FileInfo, sizeof(UDF_FILE_INFO));
    RtlZeroMemory(&Dloc, sizeof(Dloc));
    FileInfo.Dloc = &Dloc;
    FileInfo.Dloc->FileEntry = DirInfo->ParentFile->Dloc->FileEntry;
    FileInfo.Dloc->FileEntryLen = DirInfo->ParentFile->Dloc->FileEntryLen;
    FileInfo.Dloc->DataLoc = DirInfo->Dloc->DataLoc;
    FileInfo.Dloc->FELoc = DirInfo->Dloc->FELoc;
    FileInfo.ParentFile = DirInfo;
    // prepare FileIdent for 'parent Dir'
    lba = DirInfo->Dloc->FELoc.Mapping[0].extLocation;
    ASSERT(lba);
    PartNum = UDFGetRefPartNumByPhysLba(Vcb, lba);
    TagLoc = UDFPhysLbaToPart(Vcb, PartNum, lba);
    ParentLBA = DirInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation;
    ASSERT(ParentLBA);
    PartNum = UDFGetRefPartNumByPhysLba(Vcb, ParentLBA);
    FEicb.extLength = Vcb->SectorSize;
    FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, ParentLBA);
    FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
    RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));
    ((struct FidADImpUse*)&(FEicb.impUse))->uniqueID 
        = (uint32)UDFGetFileUID(DirInfo->ParentFile);
    PName.Buffer = (PWCH)L"";
    PName.Length = (PName.MaximumLength = sizeof(L"")) - sizeof(WCHAR);
    if (!NT_SUCCESS(status =
        UDFBuildFileIdent(Vcb, &PName, &FEicb, 0,
                &(FileInfo.FileIdent), &(FileInfo.FileIdentLen)) ))
        return status;
    FileInfo.FileIdent->fileCharacteristics |= FILE_PARENT;
    if (!UDFIsAStreamDir(DirInfo)) {
        FileInfo.FileIdent->fileCharacteristics |= FILE_DIRECTORY;
    }
    UDFDecFileCounter(Vcb);
    UDFIncDirCounter(Vcb);
    // init structure
    UDFSetUpTag(
        Vcb, &(FileInfo.FileIdent->descTag), (uint16)(FileInfo.FileIdentLen), TagLoc, 0);
    FileInfo.Dloc->DataLoc.Flags |= EXTENT_FLAG_VERIFY; // for metadata
    // flush
    status = UDFWriteFile__(IrpContext, Vcb, DirInfo, 0, FileInfo.FileIdentLen, FALSE, (int8*)(FileInfo.FileIdent), &WrittenBytes);
//    status = UDFFlushFI(Vcb, &FileInfo, PartNum);

    ASSERT(UDFGetFileSize(DirInfo) <= UDFGetExtentLength(DirInfo->Dloc->DataLoc.Mapping));

    MyFreePool__(FileInfo.FileIdent);
    if (!NT_SUCCESS(status)) return status;
    if (CurDirNdx) CurDirNdx->FileCharacteristics =
        DirInfo->FileIdent->fileCharacteristics;
    return UDFIndexDirectory(IrpContext, Vcb, DirInfo);
} // end UDFRecordDirectory__()

/* The dense physical bitmap is normally checkpointed after the namespace.
 * Truncation is the exception: before making an old extent allocatable in
 * RAM, persist FREE for exactly its released bits.  While the bitmap lock is
 * held, write and barrier the shortened FE.  A cut on either side of that FE
 * then has either an old owner with a referenced-free bit (repairable from a
 * complete graph), or no old owner.  Never publish unrelated RAM allocations
 * early by flushing the whole bitmap here. */
static NTSTATUS
UDFTruncatePrefreeRange(PIRP_CONTEXT IrpContext, PVCB Vcb,
                       uint32 BitmapLba, uint32 FirstLbn, uint32 Blocks,
                       int8* Sector, PBOOLEAN PossiblyModified)
{
    const uint32 SectorBytes = Vcb->SectorSize;
    const uint32 BitmapBytes = Vcb->FSBM_ByteCount;
    const uint32 DataOffset = sizeof(SPACE_BITMAP_DESC);
    NTSTATUS status;
    SIZE_T written;

    if (FirstLbn >= Vcb->FSBM_BitCount ||
        Blocks > Vcb->FSBM_BitCount - FirstLbn)
        return STATUS_FILE_CORRUPT_ERROR;
    while (Blocks) {
        uint32 byteOffset = FirstLbn >> 3;
        uint32 sectorOffset = (DataOffset + byteOffset) % SectorBytes;
        uint32 sectorIndex = (DataOffset + byteOffset) / SectorBytes;
        uint32 bits = min(Blocks, (SectorBytes - sectorOffset) * 8 -
                                  (FirstLbn & 7));
        uint32 j;
        BOOLEAN changed = FALSE;

        if (byteOffset >= BitmapBytes ||
            ((FirstLbn + bits - 1) >> 3) >= BitmapBytes)
            return STATUS_FILE_CORRUPT_ERROR;
        status = UDFReadSectors(IrpContext, Vcb, FALSE,
                                BitmapLba + sectorIndex, 1, FALSE, Sector);
        if (!NT_SUCCESS(status)) return status;
        for (j = 0; j < bits; ++j) {
            uint32 bit = FirstLbn + j;
            uint8* byte = (uint8*)Sector +
                ((DataOffset + (bit >> 3)) % SectorBytes);
            uint8 mask = (uint8)(1U << (bit & 7));
            if (!(*byte & mask)) {
                *byte |= mask;
                changed = TRUE;
            }
        }
        if (changed) {
            written = 0;
            *PossiblyModified = TRUE;
            status = UDFWriteSectors(IrpContext, Vcb, FALSE,
                                     BitmapLba + sectorIndex, 1, FALSE,
                                     Sector, &written);
            if (!NT_SUCCESS(status)) return status;
            if (written != SectorBytes) return STATUS_IO_DEVICE_ERROR;
        }
        FirstLbn += bits;
        Blocks -= bits;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
UDFTruncatePrefreeMap(PIRP_CONTEXT IrpContext, PVCB Vcb,
                      uint32 BitmapLba, PEXTENT_MAP Map,
                      uint64 KeepBytes, uint32 FeLba, int8* Sector,
                      PBOOLEAN PossiblyModified)
{
    uint64 offset = 0;
    uint32 i;
    for (i = 0; Map && (Map[i].extLength & UDF_EXTENT_LENGTH_MASK); ++i) {
        uint32 bytes = Map[i].extLength & UDF_EXTENT_LENGTH_MASK;
        uint32 blocks = (bytes + Vcb->SectorSize - 1) >> Vcb->SectorShift;
        uint32 keep = 0;
        uint32 first;
        NTSTATUS status;

        if (KeepBytes > offset) {
            uint64 within = min((uint64)bytes, KeepBytes - offset);
            keep = (uint32)((within + Vcb->SectorSize - 1) >>
                            Vcb->SectorShift);
        }
        offset += bytes;
        if ((Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED)
            continue;
        if (keep >= blocks) continue;
        if (!Map[i].extLocation ||
            Map[i].extLocation < Vcb->Partitions[0].PartitionRoot)
            return STATUS_FILE_CORRUPT_ERROR;
        /* The FE can be the first entry of an in-ICB transition.  It is
         * never a released data or allocation-descriptor block. */
        first = Map[i].extLocation + keep;
        if (first == FeLba) {
            first++;
            keep++;
        }
        if (keep >= blocks) continue;
        if (first <= FeLba && FeLba < Map[i].extLocation + blocks)
            return STATUS_NOT_SUPPORTED;
        if (UDFPhysLbaToMetadata(Vcb, first) != LBA_OUT_OF_EXTENT ||
            UDFPhysLbaToMetadata(Vcb, Map[i].extLocation + blocks - 1) !=
                LBA_OUT_OF_EXTENT)
            return STATUS_NOT_SUPPORTED;
        status = UDFTruncatePrefreeRange(IrpContext, Vcb, BitmapLba,
                    first - Vcb->Partitions[0].PartitionRoot,
                    blocks - keep, Sector, PossiblyModified);
        if (!NT_SUCCESS(status)) return status;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
UDFTruncatePrefree(PIRP_CONTEXT IrpContext, PVCB Vcb,
                   PUDF_FILE_INFO FileInfo, int64 NewLength,
                   PBOOLEAN PossiblyModified)
{
    uint32 bitmapLba, feLba, partNum;
    uint32 i;
    uint64 keep;
    uint16 ident;
    NTSTATUS status;
    int8* sector;
    PSPACE_BITMAP_DESC desc;
    BOOLEAN switchToIcb;

    if (Vcb->CDR_Mode || Vcb->BitmapFcb || !Vcb->FSBM_Bitmap ||
        !Vcb->FSBM_OldBitmap || Vcb->SectorSize != 512 ||
        !Vcb->PartitionMaps ||
        Vcb->Partitions[0].PartitionType == UDF_METADATA_MAP25 ||
        !FileInfo->Dloc->DataLoc.Mapping ||
        (FileInfo->Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED))
        return STATUS_NOT_SUPPORTED;
    partNum = 0;
    feLba = FileInfo->Dloc->FELoc.Mapping[0].extLocation;
    bitmapLba = Vcb->Partitions[partNum].PartitionRoot +
                Vcb->Partitions[partNum].UspaceBitmap;
    sector = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    if (!sector) return STATUS_INSUFFICIENT_RESOURCES;
    status = UDFReadTagged(IrpContext, Vcb, sector, bitmapLba,
                           Vcb->Partitions[partNum].UspaceBitmap, &ident);
    if (!NT_SUCCESS(status)) goto done;
    desc = (PSPACE_BITMAP_DESC)sector;
    /* The descriptor's CRC may cover the two size fields, but it must not
     * cover bitmap payload bytes changed by these sector writes. */
    if (ident != TID_SPACE_BITMAP_DESC ||
        desc->descTag.descCRCLength >
            sizeof(SPACE_BITMAP_DESC) - sizeof(tag) ||
        desc->numOfBits != Vcb->FSBM_BitCount ||
        desc->numOfBytes < Vcb->FSBM_ByteCount) {
        status = STATUS_NOT_SUPPORTED;
        goto done;
    }

    switchToIcb =
        NewLength <= (int64)(Vcb->SectorSize - FileInfo->Dloc->FileEntryLen) &&
        ((((PFILE_ENTRY)FileInfo->Dloc->FileEntry)->icbTag.flags &
          ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB);
    /* An external AD chain may be shortened by UDFBuildAllocDescs during
     * the FE flush, after this prefree scan.  Handle the all-released ICB
     * conversion, but refuse a partial shrink until its exact AD delta can
     * be included in the same transaction. */
    if (!switchToIcb && FileInfo->Dloc->AllocLoc.Mapping) {
        PEXTENT_MAP map = FileInfo->Dloc->AllocLoc.Mapping;
        for (i = 0; map[i].extLength & UDF_EXTENT_LENGTH_MASK; ++i) {
            if ((map[i].extLength >> 30) !=
                    EXTENT_NOT_RECORDED_NOT_ALLOCATED &&
                (map[i].extLocation != feLba ||
                 (map[i].extLength & UDF_EXTENT_LENGTH_MASK) >
                    Vcb->SectorSize)) {
                status = STATUS_NOT_SUPPORTED;
                goto done;
            }
        }
    }
    keep = switchToIcb ? 0 :
           (uint64)FileInfo->Dloc->DataLoc.Offset + (uint64)NewLength;
    status = UDFTruncatePrefreeMap(IrpContext, Vcb, bitmapLba,
                    FileInfo->Dloc->DataLoc.Mapping, keep, feLba,
                    sector, PossiblyModified);
    if (!NT_SUCCESS(status)) goto done;
    if (switchToIcb && FileInfo->Dloc->AllocLoc.Mapping)
        status = UDFTruncatePrefreeMap(IrpContext, Vcb, bitmapLba,
                    FileInfo->Dloc->AllocLoc.Mapping, 0, feLba,
                    sector, PossiblyModified);
done:
    MyFreePool__(sector);
    return status;
}

/* This is the old in-memory resize body.  The public wrapper below orders
 * physical frees with the on-disk bitmap and the shortened FE. */
static NTSTATUS
UDFResizeFileUnordered__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PUDF_FILE_INFO FileInfo,
    IN int64 NewLength
    )
{
    SIZE_T WrittenBytes;
    NTSTATUS status;
    uint32 PartNum;
    int8* OldInIcb = NULL;
    PEXTENT_MAP NewMap;

    UDFPrint(("UDFResizeFile__: FI %x, -> %I64x\n", FileInfo, NewLength));
    ValidateFileInfo(FileInfo);
//    ASSERT(FileInfo->RefCount >= 1);

    status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status))
        return status;

    if ((NewLength >> Vcb->SectorShift) > Vcb->TotalAllocUnits) {
        UDFPrint(("STATUS_DISK_FULL\n"));
        return STATUS_DISK_FULL;
    }
    if (NewLength == FileInfo->Dloc->DataLoc.Length) return STATUS_SUCCESS;
    if (FileInfo->ParentFile && (FileInfo->Index >= 2)) {
        UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex,FileInfo->Index)->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
    }
    if (NewLength > FileInfo->Dloc->DataLoc.Length) {
        // grow file
        return UDFWriteFile__(IrpContext, Vcb, FileInfo, NewLength, 0, FALSE, NULL, &WrittenBytes);
    }
    // truncate file
    if (NewLength <= (Vcb->SectorSize - FileInfo->Dloc->FileEntryLen)) {
        // check if we are already in IN_ICB mode
        if ((((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB) {
            // read data from old location
            if (NewLength) {
                OldInIcb = (int8*)MyAllocatePool__(NonPagedPool, (uint32)NewLength);
                if (!OldInIcb) return STATUS_INSUFFICIENT_RESOURCES;
                status = UDFReadExtent(IrpContext, Vcb, &FileInfo->Dloc->DataLoc, 0, (uint32)NewLength, FALSE, OldInIcb);
                if (!NT_SUCCESS(status)) {
                    MyFreePool__(OldInIcb);
                    return status;
                }
            } else {
                OldInIcb = NULL;
            }
            // allocate storage for new mapping
            NewMap = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , 2*sizeof(EXTENT_MAP),
                                                               MEM_EXTMAP_TAG);
            if (!(NewMap)) {
                MyFreePool__(OldInIcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            // free old location...
            if (FileInfo->Dloc->DataLoc.Mapping[0].extLocation !=
               FileInfo->Dloc->FELoc.Mapping[0].extLocation) {
               ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
mark_data_map_0:
                UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, FileInfo->Dloc->DataLoc.Mapping, AS_DISCARDED); // free
            } else {
                if ((FileInfo->Dloc->DataLoc.Mapping[0].extLength & UDF_EXTENT_LENGTH_MASK)
                       > Vcb->SectorSize) {
                    BrutePoint();
                    FileInfo->Dloc->DataLoc.Mapping[0].extLength -= Vcb->SectorSize;
                    FileInfo->Dloc->DataLoc.Mapping[0].extLocation += 1;
                    goto mark_data_map_0;
                }
                UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->DataLoc.Mapping[1]), AS_DISCARDED); // free
            }
            if (FileInfo->Dloc->AllocLoc.Mapping) {
                if ((FileInfo->Dloc->AllocLoc.Mapping[0].extLength & UDF_EXTENT_LENGTH_MASK)
                       > Vcb->SectorSize) {
                    FileInfo->Dloc->AllocLoc.Mapping[0].extLength -= Vcb->SectorSize;
                    FileInfo->Dloc->AllocLoc.Mapping[0].extLocation += 1;
                    UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, FileInfo->Dloc->AllocLoc.Mapping, AS_DISCARDED); // free
                } else {
                    UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->AllocLoc.Mapping[1]), AS_DISCARDED); // free
                }
                MyFreePool__(FileInfo->Dloc->AllocLoc.Mapping);
            }
            MyFreePool__(FileInfo->Dloc->DataLoc.Mapping);
            FileInfo->Dloc->AllocLoc.Mapping = NULL;
            FileInfo->Dloc->AllocLoc.Length = 0;
            FileInfo->Dloc->AllocLoc.Offset = 0;
            FileInfo->Dloc->AllocLoc.Modified = TRUE;
            // switch to IN_ICB mode
            ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
            ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags |= ICB_FLAG_AD_IN_ICB;
            // Set embedded data flag since file is now in ICB mode
            // Fcb may be NULL during directory operations (FCB not yet created)
            if (FileInfo->Fcb) {
                FileInfo->Fcb->FcbState |= UDF_FCB_EMBEDDED_DATA;
            }
            // init new data location descriptors
            FileInfo->Dloc->DataLoc.Mapping = NewMap;
            RtlZeroMemory((int8*)(FileInfo->Dloc->DataLoc.Mapping), 2*sizeof(EXTENT_MAP));
            FileInfo->Dloc->DataLoc.Mapping[0] = FileInfo->Dloc->FELoc.Mapping[0];
            FileInfo->Dloc->DataLoc.Length = NewLength;
            FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
            // write data to new location
            if (OldInIcb) {
                status = UDFWriteFileData(IrpContext, Vcb, FileInfo, 0, (uint32)NewLength, FALSE, OldInIcb, &WrittenBytes);
            } else {
                status = STATUS_SUCCESS;
            }
            FileInfo->Dloc->DataLoc.Modified = TRUE;
            if (OldInIcb) MyFreePool__(OldInIcb);
        } else {
            // just modify Length field
            FileInfo->Dloc->DataLoc.Length = NewLength;
            status = STATUS_SUCCESS;
        }
    } else {
        // resize extent
        ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        PartNum = UDFGetRefPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        if (Vcb->MetadataFileInfo && PartNum == Vcb->MetadataRef &&
            !UDFIsADirectory(FileInfo))
            PartNum = Vcb->MetadataPhysRef;
        status = UDFResizeExtent(IrpContext, Vcb, PartNum, NewLength, FALSE, &FileInfo->Dloc->DataLoc);
        FileInfo->Dloc->DataLoc.Modified = TRUE;
        FileInfo->Dloc->AllocLoc.Modified = TRUE;
    }
    if (NT_SUCCESS(status)) {
        UDFSetFileSize(FileInfo, NewLength);
    }

    ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));

    return status;
} // end UDFResizeFile__()

NTSTATUS
UDFResizeFile__(PIRP_CONTEXT IrpContext, PVCB Vcb,
                PUDF_FILE_INFO FileInfo, int64 NewLength)
{
    NTSTATUS status;
    uint32 partNum;
    SIZE_T entries;
    PEXTENT_MAP released;
    BOOLEAN possiblyModified = FALSE;
    BOOLEAN txnInitialized = FALSE;

    if (!FileInfo || !FileInfo->Dloc || !FileInfo->Dloc->DataLoc.Mapping ||
        NewLength < 0 ||
        NewLength >= FileInfo->Dloc->DataLoc.Length ||
        Vcb->Cow ||         // frees already wait for a committed checkpoint
        UDFIsADirectory(FileInfo) ||
        (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED))
        return UDFResizeFileUnordered__(IrpContext, Vcb, FileInfo, NewLength);

    /* No other allocator may see the freed RAM bits before the old FE is
     * durable.  This resource is recursively exclusive for calls below. */
    status = UDFDrainNewFileData(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status)) return status;
    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status)) return status;
    entries = UDFGetMappingLength(FileInfo->Dloc->DataLoc.Mapping) /
              sizeof(EXTENT_MAP) +
              UDFGetMappingLength(FileInfo->Dloc->AllocLoc.Mapping) /
              sizeof(EXTENT_MAP) + 4;
    if (entries > 65536 || entries > ((SIZE_T)-1) / sizeof(EXTENT_MAP))
        return UDFResizeFileUnordered__(IrpContext, Vcb, FileInfo, NewLength);
    released = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool,
                    entries * sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
    if (!released) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(released, entries * sizeof(EXTENT_MAP));
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    _SEH2_TRY {
        if (Vcb->TruncateTxnOwner) {
            status = STATUS_NOT_SUPPORTED;
        } else {
            Vcb->TruncateTxnReleased = released;
            Vcb->TruncateTxnCapacity = (ULONG)(entries - 1);
            Vcb->TruncateTxnCount = 0;
            Vcb->TruncateTxnError = STATUS_SUCCESS;
            txnInitialized = TRUE;
            status = UDFTruncatePrefree(IrpContext, Vcb, FileInfo, NewLength,
                                        &possiblyModified);
            if (status == STATUS_NOT_SUPPORTED && !possiblyModified) {
                /* The ordered free only covers some layouts (no metadata
                 * partition, ADs in the FE block, ...). For the others,
                 * shrink the way every other release frees space instead of
                 * failing the truncate: an overwrite-open of such a file
                 * returned STATUS_NOT_SUPPORTED to the caller. */
                status = UDFResizeFileUnordered__(IrpContext, Vcb,
                                                  FileInfo, NewLength);
                possiblyModified = TRUE;
            } else {
                if (NT_SUCCESS(status)) {
                    possiblyModified = TRUE;
                    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
                }
                if (NT_SUCCESS(status)) {
                    Vcb->TruncateTxnOwner = PsGetCurrentThread();
                    status = UDFResizeFileUnordered__(IrpContext, Vcb,
                                                      FileInfo, NewLength);
                }
            }
        }
        if (NT_SUCCESS(status) && !NT_SUCCESS(Vcb->TruncateTxnError))
            status = Vcb->TruncateTxnError;
        if (NT_SUCCESS(status)) {
            partNum = UDFGetRefPartNumByPhysLba(Vcb,
                        FileInfo->Dloc->FELoc.Mapping[0].extLocation);
            if (partNum == (uint32)-1)
                status = STATUS_FILE_CORRUPT_ERROR;
            else
                status = UDFFlushFE(IrpContext, Vcb, FileInfo, partNum);
        }
        if (NT_SUCCESS(status) && !NT_SUCCESS(Vcb->TruncateTxnError))
            status = Vcb->TruncateTxnError;
        if (NT_SUCCESS(status))
            status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (txnInitialized) {
            Vcb->TruncateTxnOwner = NULL;
            released[Vcb->TruncateTxnCount].extLength = 0;
            released[Vcb->TruncateTxnCount].extLocation = 0;
            if (NT_SUCCESS(status) && Vcb->TruncateTxnCount)
                UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, released,
                                  AS_DISCARDED);
        }
    } _SEH2_FINALLY {
        if (_SEH2_AbnormalTermination() ||
            (!NT_SUCCESS(status) && possiblyModified))
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        if (txnInitialized) {
            Vcb->TruncateTxnOwner = NULL;
            Vcb->TruncateTxnReleased = NULL;
            Vcb->TruncateTxnCapacity = 0;
            Vcb->TruncateTxnCount = 0;
        }
        UDFReleaseResource(&(Vcb->BitMapResource1));
        MyFreePool__(released);
    } _SEH2_END;
    return status;
}

/*
    This routine loads VAT.
 */
NTSTATUS
UDFLoadVAT(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 PartNdx
    )
{
    lb_addr VatFELoc;
    NTSTATUS status;
    PUDF_FILE_INFO VatFileInfo;
    uint32 len, i=0, j, to_read;
    uint32 Offset, hdrOffset;
    uint32 root;
    uint16 PartNum;
//    uint32 VatFirstLba = 0;
    int8* VatOldData;
    uint32 VatLba[6] = { Vcb->SessionEndLba,
                        Vcb->SessionEndLba - 2,
                        Vcb->SessionEndLba - 3,
                        Vcb->SessionEndLba - 5,
                        Vcb->SessionEndLba - 7,
                        0 };

    if (Vcb->Vat) return STATUS_SUCCESS;
    if (!Vcb->CDR_Mode) return STATUS_SUCCESS;
    // disable VAT for now. We'll reenable it if VAT is successfuly loaded
    Vcb->CDR_Mode = FALSE;
    PartNum = Vcb->Partitions[PartNdx].PartitionNum;
    root = Vcb->Partitions[PartNdx].PartitionRoot;

    if ((Vcb->LastTrackNum > 1) &&
       (Vcb->SessionEndLba == Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba)) {
        UDFPrint(("Hardware Read-only volume\n"));
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    }

    VatFileInfo = Vcb->VatFileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_VATFINF_TAG);
    if (!VatFileInfo) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(VatFileInfo, sizeof(UDF_FILE_INFO));
    VatFileInfo->NextLinkedFile = VatFileInfo->PrevLinkedFile = VatFileInfo;
    // load VAT FE (we know its location)
    VatFELoc.partitionReferenceNum = PartNum;
retry_load_vat:
    VatFELoc.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, VatLba[i]);
    if (!NT_SUCCESS(status = UDFOpenRootFile__(IrpContext, Vcb, &VatFELoc, VatFileInfo))) {
        UDFCleanUpFile__(Vcb, VatFileInfo);
        // try another location
        i++;
        if ( VatLba[i] &&
           (status != STATUS_FILE_CORRUPT_ERROR) &&
           (status != STATUS_CRC_ERROR)) goto retry_load_vat;
        MyFreePool__(VatFileInfo);
        Vcb->VatFileInfo = NULL;
        return status;
    }
    len = (uint32)UDFGetFileSize(VatFileInfo);
    if (Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP15) {
        // load Vat 1.50 header
        UDFPrint(("Load VAT 1.50\n"));
        struct VirtualAllocationTable15* Buf;
        if (((icbtag*)(VatFileInfo->Dloc->FileEntry+1))->fileType != UDF_FILE_TYPE_VAT15) {
            status = STATUS_FILE_CORRUPT_ERROR;
            goto err_vat_15;
        }
        Buf = (struct VirtualAllocationTable15*)MyAllocatePool__(NonPagedPool, sizeof(struct VirtualAllocationTable15));
        if (!Buf) {
err_vat_15_2:
            status = STATUS_INSUFFICIENT_RESOURCES;
err_vat_15:
            UDFCloseFile__(IrpContext, Vcb, VatFileInfo);
            UDFCleanUpFile__(Vcb, VatFileInfo);
            MyFreePool__(VatFileInfo);
            Vcb->VatFileInfo = NULL;
            return status;
        }
        Offset = 0;
        to_read =
        hdrOffset = len - sizeof(struct VirtualAllocationTable15);
        MyFreePool__(Buf);

        Vcb->minUDFReadRev  =
        Vcb->minUDFWriteRev =
        Vcb->maxUDFWriteRev = 0x0150;

        Vcb->numFiles =
        Vcb->numDirs  = -1;

    } else
    if (Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP20) {
        // load Vat 2.00 header
        UDFPrint(("Load VAT 2.00\n"));
        struct VirtualAllocationTable20* Buf;
        if (((icbtag*)(VatFileInfo->Dloc->FileEntry+1))->fileType != UDF_FILE_TYPE_VAT20) {
            status = STATUS_FILE_CORRUPT_ERROR;
            goto err_vat_15;
        }
        Buf = (struct VirtualAllocationTable20*)MyAllocatePool__(NonPagedPool, sizeof(struct VirtualAllocationTable20));
        if (!Buf) goto err_vat_15_2;
        Offset = Buf->lengthHeader;
        to_read = len - Offset;
        hdrOffset = 0;
        MyFreePool__(Buf);

        //TODO: Read VirtualAllocationTable20
        //TODO: fix Use-After-Free

        Vcb->minUDFReadRev  = Buf->minReadRevision;
        Vcb->minUDFWriteRev = Buf->minWriteRevision;
        Vcb->maxUDFWriteRev = Buf->maxWriteRevision;

        Vcb->numFiles = Buf->numFIDSFiles;
        Vcb->numDirs  = Buf->numFIDSDirectories;

    } else {
        // unknown (or wrong) VAT format
        UDFPrint(("unknown (or wrong) VAT format\n"));
        status = STATUS_FILE_CORRUPT_ERROR;
        goto err_vat_15;
    }
    // read VAT & remember old version
    Vcb->Vat = (uint32*)DbgAllocatePool(NonPagedPool, Vcb->FSBM_BitCount*sizeof(uint32) );
    if (!Vcb->Vat) {
        goto err_vat_15_2;
    }
    // store base version of VAT in memory
    VatOldData = (int8*)DbgAllocatePool(PagedPool, len);
    if (!VatOldData) {
        DbgFreePool(Vcb->Vat);
        Vcb->Vat = NULL;
        goto err_vat_15_2;
    }
    status = UDFReadFile__(IrpContext, Vcb, VatFileInfo, 0, len, FALSE, VatOldData);
    if (!NT_SUCCESS(status)) {
        UDFCloseFile__(IrpContext, Vcb, VatFileInfo);
        UDFCleanUpFile__(Vcb, VatFileInfo);
        MyFreePool__(VatFileInfo);
        DbgFreePool(Vcb->Vat);
        DbgFreePool(VatOldData);
        Vcb->Vat = NULL;
        Vcb->VatFileInfo = NULL;
    } else {
        // initialize VAT
        // !!! NOTE !!!
        // Both VAT copies - in-memory & on-disc
        // contain _relative_ addresses
        len = Vcb->NWA - root;
        for(i=0; i<=len; i++) {
            Vcb->Vat[i] = i;
        }
        RtlCopyMemory(Vcb->Vat, VatOldData+Offset, to_read);
        Vcb->InitVatCount =
        Vcb->VatCount = to_read/sizeof(uint32);
        Vcb->VatPartNdx = PartNdx;
        Vcb->CDR_Mode = TRUE;
        len = Vcb->VatCount;
        RtlFillMemory(&(Vcb->Vat[Vcb->NWA-root]), (Vcb->FSBM_BitCount-(Vcb->NWA-root))*sizeof(uint32), 0xff);
        // sync VAT and FSBM (bitmap is LBN-indexed)
        for(i=0; i<len; i++) {
            if (Vcb->Vat[i] == UDF_VAT_FREE_ENTRY) {
                if (Vcb->BitmapFcb) {
                    UDFPinBitmapPage(Vcb, i);
                    RtlSetBits(&Vcb->BitmapRtl, i - Vcb->BitmapPageStartLbn, 1);
                    UDFDirtyBitmapPage(Vcb);
                } else {
                    UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
                }
            }
        }
        UDFUnpinBitmapPage(Vcb);
        len = root + Vcb->FSBM_BitCount;  // end of partition in PSN (bitmap is LBN-indexed)
        // "pre-format" reserved area
        for(i=Vcb->NWA; i<len;) {
            for (j = 0; (j < PACKETSIZE_UDF) && (i < len); j++, i++)
            {
                ULONG lbn_pf = i - root;
                if (Vcb->BitmapFcb) {
                    UDFPinBitmapPage(Vcb, lbn_pf);
                    RtlSetBits(&Vcb->BitmapRtl, lbn_pf - Vcb->BitmapPageStartLbn, 1);
                    UDFDirtyBitmapPage(Vcb);
                } else {
                    UDFSetFreeBit(Vcb->FSBM_Bitmap, lbn_pf);
                }
            }
            for (j = 0; (j < 7) && (i < len); j++, i++)
            {
                ULONG lbn_pf = i - root;
                if (Vcb->BitmapFcb) {
                    UDFPinBitmapPage(Vcb, lbn_pf);
                    RtlClearBits(&Vcb->BitmapRtl, lbn_pf - Vcb->BitmapPageStartLbn, 1);
                    UDFDirtyBitmapPage(Vcb);
                } else {
                    UDFSetUsedBit(Vcb->FSBM_Bitmap, lbn_pf);
                }
            }
        }
        UDFUnpinBitmapPage(Vcb);
        DbgFreePool(VatOldData);
    }
    return status;
} // end UDFLoadVAT()

NTSTATUS
UDFOpenMetadataFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 PhysRef,
    IN uint32 FELbn,
    IN uint8 FileType,
    OUT PUDF_FILE_INFO* FileInfo
)
{
    NTSTATUS status;
    PUDF_FILE_INFO fi;
    lb_addr FELoc;

    *FileInfo = NULL;

    fi = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_VATFINF_TAG);

    if (!fi)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    RtlZeroMemory(fi, sizeof(UDF_FILE_INFO));
    fi->NextLinkedFile = fi->PrevLinkedFile = fi;
    
    FELoc.partitionReferenceNum = (uint16)PhysRef;
    FELoc.logicalBlockNum = FELbn;
    
    status = UDFOpenRootFile__(IrpContext, Vcb, &FELoc, fi);

    if (
        NT_SUCCESS(status) &&
        (((icbtag*)((fi->Dloc->FileEntry)+1))->fileType != FileType)
    ) {
        UDFPrint(("UDFOpenMetadataFile: unexpected type at %x got %x, expected %x\n",
            FELbn, ((icbtag*)((fi->Dloc->FileEntry)+1))->fileType, FileType));
    
        UDFCloseFile__(IrpContext, Vcb, fi);
        status = STATUS_FILE_CORRUPT_ERROR;
    }

    if (!NT_SUCCESS(status)) {
        UDFCleanUpFile__(Vcb, fi);
        MyFreePool__(fi);
        return status;
    }

    *FileInfo = fi;
    
    return STATUS_SUCCESS;
}

void
UDFReleaseMetadata(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
)
{
    PUDF_FILE_INFO* Files[3] = { 
        &Vcb->MetadataBitmapFileInfo,
        &Vcb->MetadataMirrorFileInfo,
        &Vcb->MetadataFileInfo 
    };

    uint32 i;
    for (i = 0;i < 3; i++) {

        if (*(Files[i])) {
            
            UDFCloseFile__(IrpContext, Vcb, *(Files[i]));
            UDFCleanUpFile__(Vcb, *(Files[i]));
            MyFreePool__(*(Files[i]));

            *(Files[i]) = NULL;
        }

    }

    if (Vcb->MetadataBitmap) {
        DbgFreePool(Vcb->MetadataBitmap);
        Vcb->MetadataBitmap = NULL;
    }
    if (Vcb->MetadataPendingAlloc) {
        DbgFreePool(Vcb->MetadataPendingAlloc);
        Vcb->MetadataPendingAlloc = NULL;
    }

    Vcb->MetadataBitCount = 0;
    Vcb->MetadataFreeCount = 0;
    Vcb->MetadataHint = 0;
    Vcb->MetadataBitmapModified = FALSE;
}

NTSTATUS
UDFLoadMetadata(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 PartNdx
    )
{
    NTSTATUS status;
    PSPACE_BITMAP_DESC Sbd;
    uint32 i, len, n, PhysRef = (uint32)-1;

    if (Vcb->MetadataFileInfo) {
        return STATUS_SUCCESS;
    }

    for (i = 0; i < Vcb->PartitionMaps; i++) {
        if (
            (Vcb->Partitions[i].PartitionNum == Vcb->Partitions[PartNdx].PartitionNum) &&
            (
                (Vcb->Partitions[i].PartitionType == UDF_TYPE1_MAP15) ||
                (Vcb->Partitions[i].PartitionType == UDF_SPARABLE_MAP15)
            )
        ) {
            PhysRef = i;
            break;
        }
    }

    if (PhysRef == (uint32)-1) {
        UDFPrint(("UDFLoadMetadata: metadata map physical partition not found\n"));
        return STATUS_DISK_CORRUPT_ERROR;
    }

    Vcb->MetadataPhysRef = PhysRef;

    status = UDFOpenMetadataFile(IrpContext, Vcb, PhysRef, Vcb->MetadataFELoc,
                                UDF_FILE_TYPE_METADATA, &Vcb->MetadataFileInfo);
    
    if(!NT_SUCCESS(status)) {
        UDFPrint(("UDFLoadMetadata: failed to load metadata: %x\n", status));
        return status;
    }
    
    Vcb->Partitions[PartNdx].PartitionLen =
        (uint32)(UDFGetFileSize(Vcb->MetadataFileInfo) >> Vcb->SectorShift);

    status = UDFOpenMetadataFile(IrpContext, Vcb, PhysRef, Vcb->MetadataMirrorFELoc,
                                UDF_FILE_TYPE_METADATA_MIRROR, &Vcb->MetadataMirrorFileInfo);
    if ( !NT_SUCCESS(status)){
        UDFPrint(("UDFLoadMetadata: failed to load metadata mirror: %x\n", status));
        goto err_metadata;
    }

    if (Vcb->MetadataBitmapFELoc != 0xFFFFFFFF) {
        status = UDFOpenMetadataFile(IrpContext, Vcb, PhysRef, Vcb->MetadataBitmapFELoc,
                                    UDF_FILE_TYPE_METADATA_BITMAP, &Vcb->MetadataBitmapFileInfo);
        if(!NT_SUCCESS(status)) {
            UDFPrint(("UDFLoadMetadata: failed to load metadata bitmap: %x\n", status));
            goto err_metadata;
        }

        len = (uint32)UDFGetFileSize(Vcb->MetadataBitmapFileInfo);
        if (len < sizeof(SPACE_BITMAP_DESC)) {
            status = STATUS_DISK_CORRUPT_ERROR;
            goto err_metadata;
        }

        Sbd = (PSPACE_BITMAP_DESC)DbgAllocatePool(NonPagedPool, len);
        if (!Sbd) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto err_metadata;
        }

        status = UDFReadFile__(IrpContext, Vcb, Vcb->MetadataBitmapFileInfo, 0, 
            len, FALSE, (int8*)Sbd);
        if (
            NT_SUCCESS(status) &&
            (
                (Sbd->descTag.tagIdent != TID_SPACE_BITMAP_DESC) ||
                (Sbd->numOfBytes > len - sizeof(SPACE_BITMAP_DESC)) ||
                (Sbd->numOfBits > Sbd->numOfBytes * 8)
            )
        ) {
            UDFPrint(("UDFLoadMetadata: corrupted metadata bitmap: tag %x, %x bits, %x bytes\n",
                Sbd->descTag.tagIdent, Sbd->numOfBits, Sbd->numOfBytes));
            status = STATUS_DISK_CORRUPT_ERROR;
        }

        if (!NT_SUCCESS(status)) {
            DbgFreePool(Sbd);
            goto err_metadata;
        }

        Vcb->MetadataBitmap = (PCHAR)DbgAllocatePool(NonPagedPool, Sbd->numOfBytes);
        if (!Vcb->MetadataBitmap) {
            DbgFreePool(Sbd);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto err_metadata;
        }
        Vcb->MetadataPendingAlloc = (PCHAR)DbgAllocatePool(NonPagedPool,
                                                            Sbd->numOfBytes);
        if (!Vcb->MetadataPendingAlloc) {
            DbgFreePool(Sbd);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto err_metadata;
        }
        RtlZeroMemory(Vcb->MetadataPendingAlloc, Sbd->numOfBytes);
        
        RtlCopyMemory(Vcb->MetadataBitmap, Sbd+1, Sbd->numOfBytes);
        Vcb->MetadataBitmapByteCount = Sbd->numOfBytes;
        Vcb->MetadataBitCount = min(Sbd->numOfBits, Vcb->Partitions[PartNdx].PartitionLen);
        
        DbgFreePool(Sbd);
        for (i = 0, n = 0; i < Vcb->MetadataBitCount; i++) {
            if (Vcb->MetadataBitmap[i >> 3] & (1 << (i & 7)))
                n++;
        }
        Vcb->MetadataFreeCount = n;
        Vcb->MetadataHint = 0;
    }

    Vcb->MetadataBitmapModified = FALSE;
    UDFPrint(("UDFLoadMetadata: metadata partition: ref %x, %x blocks, %x free\n",
        PartNdx, Vcb->Partitions[PartNdx].PartitionLen, Vcb->MetadataFreeCount));
    
    return STATUS_SUCCESS;

err_metadata:
    UDFReleaseMetadata(IrpContext, Vcb);
    return status;
}

NTSTATUS
UDFFlushMetadataBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
)
{
    NTSTATUS status;
    PSPACE_BITMAP_DESC Sbd;
    uint32 len;
    SIZE_T written = 0;

    if (!Vcb->MetadataBitmapModified)
        return STATUS_SUCCESS;

    if (!Vcb->MetadataBitmap || !Vcb->MetadataPendingAlloc ||
        !Vcb->MetadataBitmapFileInfo)
        return STATUS_FILE_CORRUPT_ERROR;

    len = (uint32)UDFGetFileSize(Vcb->MetadataBitmapFileInfo);
    if (len < sizeof(SPACE_BITMAP_DESC) ||
        Vcb->MetadataBitmapByteCount > len - sizeof(SPACE_BITMAP_DESC))
        return STATUS_FILE_CORRUPT_ERROR;

    /* An allocation or deletion may have made an FE/FID dirty before this
     * bitmap is serialized. Keep an OPEN integrity checkpoint durable while
     * its owners are drained to the lower device. */
    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status))
        return status;

    Sbd = (PSPACE_BITMAP_DESC)DbgAllocatePool(NonPagedPool, len);
    if (!Sbd)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Serialize the RAM copy and the disk-only prefree overlay under the
     * allocator lock.  Otherwise this whole-bitmap flush could silently
     * turn a prepared FREE bit back to USED before final FCB teardown. */
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    _SEH2_TRY {
        status = UDFReadFile__(IrpContext, Vcb,
                               Vcb->MetadataBitmapFileInfo,
                               0, len, FALSE, (int8*)Sbd);
        if (NT_SUCCESS(status)) {
            if (Sbd->descTag.tagIdent != TID_SPACE_BITMAP_DESC ||
                Sbd->numOfBytes > len - sizeof(SPACE_BITMAP_DESC) ||
                Sbd->numOfBits > Sbd->numOfBytes * 8) {
                status = STATUS_FILE_CORRUPT_ERROR;
            } else {
                uint32 descriptorLba = UDFExtentOffsetToLba(
                    Vcb, Vcb->MetadataBitmapFileInfo->Dloc->DataLoc.Mapping,
                    Vcb->MetadataBitmapFileInfo->Dloc->DataLoc.Offset,
                    NULL, NULL, NULL, NULL);
                if (descriptorLba == LBA_OUT_OF_EXTENT) {
                    status = STATUS_FILE_CORRUPT_ERROR;
                    goto done_metadata_bitmap;
                }
                Sbd->numOfBits = Vcb->MetadataBitCount;
                Sbd->numOfBytes = Vcb->MetadataBitmapByteCount;
                RtlCopyMemory(Sbd + 1, Vcb->MetadataBitmap,
                              Vcb->MetadataBitmapByteCount);
                status = UDFOverlayDeletePrefreeMetadata(
                    Vcb, (int8*)(Sbd + 1), Vcb->MetadataBitCount);
                if (!NT_SUCCESS(status))
                    goto done_metadata_bitmap;
                /* Publish allocation/free bits only after the preceding
                 * FE/FID owners are durable. UDFUmount__ runs later, so its
                 * pre-physical-bitmap barrier cannot order this write. */
                status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
                if (!NT_SUCCESS(status))
                    goto done_metadata_bitmap;
                // ECMA 167 4/14.12: the CRC covers only the two bitmap length
                // fields.  The bitmap payload is mutable and excluded from it.
                UDFSetUpTag(Vcb, &Sbd->descTag,
                            sizeof(SPACE_BITMAP_DESC),
                            UDFPhysLbaToPart(Vcb, Vcb->MetadataPhysRef,
                                             descriptorLba), 0);
                status = UDFWriteFile__(IrpContext, Vcb,
                                        Vcb->MetadataBitmapFileInfo, 0, len,
                                        FALSE, (int8*)Sbd, &written);
                if (NT_SUCCESS(status) && written != len)
                    status = STATUS_IO_DEVICE_ERROR;
                // The bitmap may be embedded in its File Entry.  Flush that FE
                // before UDFUmount__ closes the LVID; closing the FileInfo later
                // must not reopen a volume whose metadata is already committed.
                if (NT_SUCCESS(status))
                    status = UDFFlushFile__(IrpContext, Vcb,
                                            Vcb->MetadataBitmapFileInfo, 0);
                /* Retire per-bit allocation provenance only after the
                 * serialized bitmap generation reaches the lower device.
                 * A successful cache/FE flush alone does not guarantee it. */
                if (NT_SUCCESS(status))
                    status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
                if (NT_SUCCESS(status)) {
                    RtlZeroMemory(Vcb->MetadataPendingAlloc,
                                  Vcb->MetadataBitmapByteCount);
                    Vcb->MetadataBitmapModified = FALSE;
                }
            }
        }

done_metadata_bitmap:
        NOTHING;
    } _SEH2_FINALLY {
        UDFReleaseResource(&(Vcb->BitMapResource1));
        DbgFreePool(Sbd);
    } _SEH2_END;
    return status;
}

/*
    Reads Extended Attributes
    Caller should use UDFGetFileEALength to allocate Buffer of sufficient
    size
 *//*
NTSTATUS
UDFReadFileEA(
    IN PVCB Vcb,
    IN PDIR_INDEX FileDirNdx,
    OUT int8* Buffer
    )
{
    PFILE_ENTRY FileEntry;
    NTSTATUS status;

    if (FileDirNdx->FileInfo) {
        FileEntry = (PFILE_ENTRY)(FileDirNdx->FileInfo->Dloc->FileEntry);
    } else {
        FileEntry = (PFILE_ENTRY)MyAllocatePool__(NonPagedPool, Vcb->BlockSize);
        if (!FileEntry) return;
        if (!NT_SUCCESS(status = UDFReadFileEntry(Vcb, &(FileDirNdx->FileEntry), FileEntry, &Ident))) {
            MyFreePool__(FileEntry);
            return status;
        }
    }
}*/
/*
    This dumb routine checks if the file has been found is deleted
    It is introduced to make main modules FS-type independent
 */
/*BOOLEAN
UDFIsDeleted(
    IN PDIR_INDEX DirNdx
    )
{
    return (DirNdx->FileCharacteristics & FILE_DELETED);
} */

/*BOOLEAN
UDFIsADirectory(
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    if (!FileInfo) return FALSE;
    if (FileInfo->Dloc->DirIndex) return TRUE;
    if (!(FileInfo->FileIdent)) return FALSE;
    return (FileInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY);
} */
/*
    This routine calculates actual allocation size
 */
/*int64
UDFGetFileAllocationSize(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    return UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping);// +
//           UDFGetExtentLength(FileInfo->Dloc->FELoc.Mapping) +
//           UDFGetExtentLength(FileInfo->Dloc->AllocLoc.Mapping) - Vcb->BlockSize;
}*/

/*
    This routine checks if the directory is empty
 */
BOOLEAN
UDFIsDirEmpty(
    IN PDIR_INDEX_HDR hCurDirNdx
    )
{
    uint32 fc;
    uint_di i;
    PDIR_INDEX_ITEM CurDirNdx;
    // not empty
    for(i=2; (CurDirNdx = UDFDirIndex(hCurDirNdx,i)); i++) {
        fc = CurDirNdx->FileCharacteristics;
        if (!(fc & (FILE_PARENT | FILE_DELETED)) &&
           CurDirNdx->Length)
            return FALSE;
    }
    return TRUE;
} // end UDFIsDirEmpty()

/*
    Calc logicalBlocksRecorded from recorded extents of mapping (zero for in ICB data)
*/
static
void
UDFSetRecordedBlocks(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint64 Total = 0;
    uint16 AllocMode;
    PFILE_ENTRY FE = NULL;
    PEXTENDED_FILE_ENTRY EFE = NULL;
    PEXTENT_MAP Extent = NULL;
    
    FE = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);

    AllocMode = FE->icbTag.flags & ICB_FLAG_ALLOC_MASK;

    Extent = FileInfo->Dloc->DataLoc.Mapping;

    if (Extent && AllocMode != ICB_FLAG_AD_IN_ICB) {
        while (Extent->extLength) {
            Total += ((Extent->extLength >> 30) == EXTENT_RECORDED_ALLOCATED) *
                (((Extent->extLength & UDF_EXTENT_LENGTH_MASK) + Vcb->SectorSize - 1) >> Vcb->SectorShift);
            Extent++;
        }
    }

    if (FileInfo->Dloc->FileEntry->tagIdent == TID_FILE_ENTRY) {
        FE->logicalBlocksRecorded = Total;
    } else if (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) {
        EFE = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        EFE->logicalBlocksRecorded = Total;
    }
}

static
BOOLEAN
UDFAllocDescsStartInFEBlock(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
)
{
    uint16 AllocMode;
    
    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;

    return (
        FileInfo->Dloc->AllocLoc.Mapping &&
        AllocMode != ICB_FLAG_AD_IN_ICB) &&
           (FileInfo->Dloc->AllocLoc.Mapping[0].extLocation == FileInfo->Dloc->FELoc.Mapping[0].extLocation) &&
           (FileInfo->Dloc->AllocLoc.Offset == FileInfo->Dloc->FileEntryLen) &&
           (FileInfo->Dloc->AllocLoc.Offset < Vcb->SectorSize
    );
} 

static
BOOLEAN
UDFAllocDescsInFEBlock(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
)
{
    return UDFAllocDescsStartInFEBlock(Vcb, FileInfo) &&
           ((FileInfo->Dloc->AllocLoc.Offset + FileInfo->Dloc->AllocLoc.Length) <= Vcb->SectorSize);
}

static
NTSTATUS
UDFSyncInIcbData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
)
{
    uint32 FELen = FileInfo->Dloc->FileEntryLen;
    int8* FEBuf = (int8*)(FileInfo->Dloc->FileEntry);

    if (FELen + FileInfo->Dloc->DataLoc.Length 
        > Vcb->SectorSize)
        return STATUS_FILE_CORRUPT_ERROR;

    RtlZeroMemory(FEBuf + FELen, Vcb->SectorSize - FELen);
    if (!FileInfo->Dloc->DataLoc.Length)
        return STATUS_SUCCESS;

    return UDFReadExtent(IrpContext, Vcb, 
        &FileInfo->Dloc->DataLoc, 0, 
        (uint32)(FileInfo->Dloc->DataLoc.Length), 
        FALSE, FEBuf + FELen
    );
}

static
uint32
UDFSerializeFEBlock(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum
)
{
    PUDF_DATALOC_INFO Dloc = FileInfo->Dloc;

    uint32 FELen = Dloc->FileEntryLen;
    int8* FEBuf = (int8*)(Dloc->FileEntry);
    
    uint16 AllocMode;
    uint32 TailLen, WriteLen;
    int64 InfoLen;
    uint32 lba = Dloc->FELoc.Mapping[0].extLocation;

    UDFSetAllocDescLen(Vcb, FileInfo);
    UDFSetRecordedBlocks(Vcb, FileInfo);

    if (Dloc->FileEntry->tagIdent == TID_FILE_ENTRY)
        InfoLen = ((PFILE_ENTRY)(Dloc->FileEntry))->informationLength;
    else
        InfoLen = ((PEXTENDED_FILE_ENTRY)(Dloc->FileEntry))->informationLength;

    if (InfoLen != Dloc->DataLoc.Length)
        UDFSetFileSize(FileInfo, Dloc->DataLoc.Length);

    if (Dloc->FileEntry->tagIdent == TID_FILE_ENTRY)
        TailLen = ((PFILE_ENTRY)(Dloc->FileEntry))->lengthAllocDescs;
    else
        TailLen = ((PEXTENDED_FILE_ENTRY)(Dloc->FileEntry))->lengthAllocDescs;

    AllocMode = ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;

    if (
        (
            (AllocMode == ICB_FLAG_AD_IN_ICB) || 
            UDFAllocDescsStartInFEBlock(Vcb, FileInfo)
        ) &&
        (FELen + TailLen <= Vcb->SectorSize)
    ) {
        RtlZeroMemory(FEBuf + FELen + TailLen, Vcb->SectorSize - FELen - TailLen);
        WriteLen = Vcb->SectorSize;
    } else {
        TailLen = 0;
        WriteLen = (uint32)(Dloc->FELoc.Length);
    }

    UDFSetUpTag(Vcb, Dloc->FileEntry, (uint16)(FELen + TailLen), UDFPhysLbaToPart(Vcb, PartNum, lba), 0);
    UDFPrint(("Desc CRC Len %x\n", Dloc->FileEntry->descCRCLength));

    return WriteLen;
}

static
NTSTATUS
UDFWriteFEBlock(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, uint32 PartNum)
{
    PUDF_DATALOC_INFO Dloc = FileInfo->Dloc;
    uint32 WriteLen;
    SIZE_T WrittenBytes = 0;
    NTSTATUS status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status)) {
        UDFRememberMetadataWritebackError(IrpContext, Vcb, FileInfo, status);
        return status;
    }
    WriteLen = UDFSerializeFEBlock(Vcb, FileInfo, PartNum);
    /* A failed or short write may have replaced part of the old sector.
     * It must never leave an eligible old attribute-only checkpoint. */
    UDFDirectoryFEWriteStarting(Dloc);
    status = UDFWriteExtent(IrpContext, Vcb, &Dloc->FELoc,
        0, WriteLen, FALSE, (int8*)Dloc->FileEntry, &WrittenBytes);
    if (NT_SUCCESS(status) && WrittenBytes != WriteLen)
        status = STATUS_IO_DEVICE_ERROR;
    if (NT_SUCCESS(status)) {
        Dloc->DirectoryCommittedLength = Dloc->DataLoc.Length;
        Dloc->DirectoryCommitKnown = TRUE;
        UDFDirectoryAppendFECommitted(FileInfo);
        UDFDirectoryFEWriteCompleted(Vcb, FileInfo, WriteLen);
    }
    UDFRememberMetadataWritebackError(IrpContext, Vcb, FileInfo, status);
    return status;
}

static
NTSTATUS
UDFWriteFileData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
)
{
    NTSTATUS status;
    uint32 PartNum;
    PUDF_DATALOC_INFO Dloc = FileInfo->Dloc;

    if (
        ((((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB) ||
        (Dloc->DataLoc.Offset != Dloc->FileEntryLen) ||
        (Offset + (int64)Length > Dloc->DataLoc.Length) ||
        ((PartNum = UDFGetRefPartNumByPhysLba(Vcb, Dloc->FELoc.Mapping[0].extLocation)) == (uint32)-1)
    )
        return UDFWriteExtent(IrpContext, Vcb, &Dloc->DataLoc, Offset, Length, Direct, Buffer, WrittenBytes);

    *WrittenBytes = 0;

    /* New File Entries are zero-initialized, and an opened File Entry was
       read as a complete sector.  Every in-ICB write through this routine
       copies its bytes into that same sector-sized buffer before the full
       FE write.  Re-reading the on-disk in-ICB payload here only duplicates
       the buffered bytes on rewritable media.  Keep the read for sequential
       media, where the next writable location may change between writes. */
    if (Vcb->CDR_Mode) {
        status = UDFSyncInIcbData(IrpContext, Vcb, FileInfo);
        if (!NT_SUCCESS(status))
            return status;
    }
    
    if ((Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) &&
        (Offset < 0 || Dloc->DataLoc.Length < 0 ||
         (uint64)Dloc->FileEntryLen + (uint64)Dloc->DataLoc.Length > Vcb->SectorSize ||
         (uint64)Offset > (uint64)Dloc->DataLoc.Length ||
         Length > (uint64)Dloc->DataLoc.Length - (uint64)Offset))
        return STATUS_FILE_CORRUPT_ERROR;
    if (Length)
        RtlCopyMemory((int8*)(Dloc->FileEntry) + Dloc->FileEntryLen + (uint32)Offset, Buffer, Length);

    if (UDFDeferNewFileMetadata(IrpContext, Vcb, FileInfo)) {
        Dloc->DataLoc.Modified = TRUE;
        Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        *WrittenBytes = Length;
        return STATUS_SUCCESS;
    }
    status = UDFWriteFEBlock(IrpContext, Vcb, FileInfo, PartNum);

    if (NT_SUCCESS(status)) {
        *WrittenBytes = Length;

        /* UDFWriteFEBlock committed the entire in-ICB FE sector, including
           its data and allocation length.  On rewritable media an otherwise
           unchanged FE need not be written again at the next cleanup flush.
           Keep dirty state when allocation/relocation or directory retagging
           is still pending.  Sequential media has different write ordering. */
        if (Length && !Vcb->CDR_Mode &&
            !Dloc->FELoc.Modified &&
            !Dloc->AllocLoc.Modified &&
            !Dloc->AllocLoc.Mapping &&
            (!Dloc->DirIndex ||
             (!!(Dloc->DirIndex->DIFlags & UDF_DI_FLAG_INIT_IN_ICB) ==
              !!Dloc->DataLoc.Offset))) {
            Dloc->DataLoc.Modified = FALSE;
            Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
        }
    }
    
    return status;
}

/*
 */
static NTSTATUS
UDFFlushFEInternal(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum,
    int8* StageBuffer,
    PBOOLEAN Staged
    )
{
    int8* NewAllocDescs;
    NTSTATUS status;
    SIZE_T WrittenBytes = 0;
    uint16 AllocMode;
    uint32 lba;

    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
#ifdef UDF_DBG
/*    if (UDFIsADirectory(FileInfo) && (UDFGetFileSize(FileInfo) < 0x28) &&
       !UDFIsDeleted(UDFDirIndex(DirInfo->Dloc->DirIndex, FileInfo->Index)) ) {
        BrutePoint();
    }*/
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if (FileInfo->Dloc->FELoc.Offset) {
        BrutePoint();
    }
    if (FileInfo->Dloc->AllocLoc.Mapping) {
        ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
    }
#endif // UDF_DBG
retry_flush_FE:
    /* Append payload and its recorded mapping must be committed before AD
     * serialization and before a longer directory FE becomes visible. */
    status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status))
        return status;
    UDFPrint(("  FlushFE: %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));

    /* Retagging is meaningful only for directories.  It returns
     * STATUS_NOT_A_DIRECTORY for an ordinary file, and a real I/O failure
     * must stop this FE flush while its dirty state is still retained. */
    if (FileInfo->Dloc->DirIndex) {
        status = UDFReTagDirectory(IrpContext, Vcb, FileInfo);
        if (!NT_SUCCESS(status))
            return status;
    }
    if (FileInfo->Dloc->DataLoc.Modified ||
       FileInfo->Dloc->AllocLoc.Modified) {
        ASSERT(PartNum != (uint32)(-1));
        // prepare new AllocDescs for flushing...
        if (!NT_SUCCESS(status = UDFBuildAllocDescs(IrpContext, Vcb, PartNum, FileInfo, &NewAllocDescs))) {
            UDFPrint(("  FlushFE: UDFBuildAllocDescs() faliled (%x)\n", status));
            if (NewAllocDescs)
                MyFreePool__(NewAllocDescs);
            return status;
        }
#ifdef UDF_DBG
        ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
        AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
#endif // UDF_DBG
        // initiate update of lengthAllocDescs
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        if (NewAllocDescs && UDFAllocDescsInFEBlock(Vcb, FileInfo)) {
            int8* feBuffer;

            feBuffer = (int8*)FileInfo->Dloc->FileEntry;
            
            RtlCopyMemory(feBuffer + FileInfo->Dloc->AllocLoc.Offset, NewAllocDescs, (uint32)(FileInfo->Dloc->AllocLoc.Length));
            RtlZeroMemory(
                feBuffer + (FileInfo->Dloc->AllocLoc.Offset + (uint32)(FileInfo->Dloc->AllocLoc.Length)), 
                Vcb->SectorSize - (FileInfo->Dloc->AllocLoc.Offset + (uint32)(FileInfo->Dloc->AllocLoc.Length))
            );
            
            MyFreePool__(NewAllocDescs);
        } else if (NewAllocDescs) {
            ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
            status = UDFPadLastSector(IrpContext, Vcb, &FileInfo->Dloc->AllocLoc);
            if (!NT_SUCCESS(status)) {
                MyFreePool__(NewAllocDescs);
                return status;
            }
            // ... and flush it
            status = UDFWriteExtent(IrpContext, Vcb, &FileInfo->Dloc->AllocLoc, 0, (uint32)(FileInfo->Dloc->AllocLoc.Length), FALSE, NewAllocDescs, &WrittenBytes);
            if (UDFAllocDescsStartInFEBlock(Vcb, FileInfo)) {
                RtlCopyMemory((int8*)(FileInfo->Dloc->FileEntry) + FileInfo->Dloc->AllocLoc.Offset, 
                                NewAllocDescs,
                                min(
                                 (uint32)(FileInfo->Dloc->AllocLoc.Length),
                                 Vcb->SectorSize - FileInfo->Dloc->AllocLoc.Offset
                                )
                );
            }
            MyFreePool__(NewAllocDescs);
            if (!NT_SUCCESS(status)) {
                UDFPrint(("  FlushFE: UDFWriteExtent() faliled (%x)\n", status));
                return status;
            }
#ifdef UDF_DBG
        } else {
            ASSERT(AllocMode == ICB_FLAG_AD_IN_ICB);
#endif // UDF_DBG
        }
        FileInfo->Dloc->DataLoc.Modified = FALSE;
        FileInfo->Dloc->AllocLoc.Modified = FALSE;
    } else {
#if defined(UDF_DBG) && !defined(UDF_CHECK_UTIL)
        if (FileInfo->Dloc->DataLoc.Mapping[0].extLength == (uint32)(EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30) &&
            FileInfo->Dloc->DataLoc.Mapping[0].extLocation == 0) {
                // This case occurs when DeletePending is set for directories. UDFUnlinkFile__ been called.
                ASSERT(UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping) == 0);
        }
        else {
            ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
        }
#endif // UDF_DBG
    }
/*    if (FileInfo->Fcb &&
       ((FileInfo->Dloc->FELoc.Mapping[0].extLocation > Vcb->SessionEndLba) ||
        (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot))) ) {
        BrutePoint();
    }*/
/*    if (FileInfo->Dloc->FELoc.Mapping[0].extLocation) {
        ASSERT( FileInfo->Dloc->FileEntry->tagLocation ==
               (FileInfo->Dloc->FELoc.Mapping[0].extLocation - 0x580));
    }*/
    if ((FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
        FileInfo->Dloc->FELoc.Modified) {
        ASSERT(PartNum != (uint32)(-1));
        // UDF 2.50 File Entries may live in the metadata partition, whose
        // partition map is distinct from the physical partition map.
        ASSERT(PartNum < Vcb->PartitionMaps);
        if (PartNum == (uint32)(-1) || PartNum == (uint32)(-2)) {
            UDFPrint(("  bad PartNum: %d\n", PartNum));
        }
        // flush FileEntry

        // if FE is located in remapped block, place it to reliable space
        lba = FileInfo->Dloc->FELoc.Mapping[0].extLocation;
        if (Vcb->BSBM_Bitmap) {
            if (UDFGetBadBit((uint32*)(Vcb->BSBM_Bitmap), lba - Vcb->Partitions[0].PartitionRoot)) {
                AdPrint(("  bad block under FE @%x\n", lba));
                goto relocate_FE;
            }
        }

        AdPrint(("  setup tag: @%x\n", lba));
        ASSERT( lba );
        UDFPrint(("FELoc.Length = %x\n", FileInfo->Dloc->FELoc.Length));

        AllocMode = 
            ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & 
            ICB_FLAG_ALLOC_MASK;
        /* Rewritable-media in-ICB writes update the same sector-sized FE
           buffer before committing the full FE block.  A separate disk read
           during flush only reloads those bytes; keep it for sequential
           media, where the writable location can move. */
        if (AllocMode == ICB_FLAG_AD_IN_ICB && Vcb->CDR_Mode) {

            status = UDFSyncInIcbData(IrpContext, Vcb, FileInfo);

            if (!NT_SUCCESS(status)) {
                UDFPrint(("  FlushFE: UDFSyncInIcbData failed with : %x\n", status));
                return status;
            }

        }
        if (StageBuffer && UDFCanStageFileEntry(Vcb, FileInfo) &&
            UDFSerializeFEBlock(Vcb, FileInfo, PartNum) == 512) {
            RtlCopyMemory(StageBuffer, FileInfo->Dloc->FileEntry, 512);
            *Staged = TRUE;
            return STATUS_SUCCESS;
        }
        status = UDFWriteFEBlock(IrpContext, Vcb, FileInfo, PartNum);

        if (!NT_SUCCESS(status)) {
            UDFPrint(("  FlushFE: UDFWriteExtent(2) failed (%x)\n", status));
            if (status == STATUS_DEVICE_DATA_ERROR) {
relocate_FE:
                UDFPrint(("  try to relocate\n"));

                EXTENT_INFO _FEExtInfo;
                // calculate the length required

                // allocate block for FE
                if (NT_SUCCESS(UDFAllocateFESpace(IrpContext, Vcb, FileInfo->ParentFile, PartNum, &_FEExtInfo, (uint32)(FileInfo->Dloc->FELoc.Length)) )) {
                    UDFPrint(("  relocate %x -> %x\n",
                        lba,
                        _FEExtInfo.Mapping[0].extLocation));

                    UDFMarkSpaceAsXXX(Vcb, 0, FileInfo->Dloc->FELoc.Mapping, AS_BAD);

                    UDFRelocateDloc(Vcb, FileInfo->Dloc, _FEExtInfo.Mapping[0].extLocation);
                    MyFreePool__(FileInfo->Dloc->FELoc.Mapping);
                    FileInfo->Dloc->FELoc.Mapping = _FEExtInfo.Mapping;

                    FileInfo->Dloc->FELoc.Modified = TRUE;
                    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

                    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
                    if (AllocMode == ICB_FLAG_AD_IN_ICB) {
                        UDFPrint(("  IN-ICB data lost\n"));
                        FileInfo->Dloc->DataLoc.Mapping[0].extLocation = _FEExtInfo.Mapping[0].extLocation;
                        FileInfo->Dloc->DataLoc.Modified = TRUE;
                    } else {
                        FileInfo->Dloc->AllocLoc.Mapping[0].extLocation = _FEExtInfo.Mapping[0].extLocation;
                        FileInfo->Dloc->AllocLoc.Modified = TRUE;
                    }

                    if (FileInfo->Index >= 2) {
                        PDIR_INDEX_ITEM DirNdx;
                        DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
                        if (DirNdx) {
                            UDFPrint(("  update reference in FI\n"));
                            DirNdx->FileEntryLoc.logicalBlockNum =
                                FileInfo->FileIdent->icb.extLocation.logicalBlockNum =
                                UDFPhysLbaToPart(Vcb, PartNum, _FEExtInfo.Mapping[0].extLocation);
                            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
                        }
                    }
                    // this will update
                    UDFPrint(("  retry flush...\n"));
                    goto retry_flush_FE;
                }
            }
            BrutePoint();
            return status;
        }
        FileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
        FileInfo->Dloc->FELoc.Modified = FALSE;
    } else {
        ASSERT((FileInfo->Dloc->FileEntry->descVersion == 2) ||
               (FileInfo->Dloc->FileEntry->descVersion == 3));
    }
#ifdef UDF_DBG
    if (FileInfo->Dloc->AllocLoc.Mapping) {
        ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
    } else {
        ASSERT(AllocMode == ICB_FLAG_AD_IN_ICB);
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFFlushFEInternal()

NTSTATUS
UDFFlushFE(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, uint32 PartNum)
{
    NTSTATUS status = UDFDrainNewFileData(IrpContext, Vcb, FileInfo);
    if (NT_SUCCESS(status))
        status = UDFFlushFEInternal(IrpContext, Vcb, FileInfo, PartNum, NULL, NULL);
    UDFRememberMetadataWritebackError(IrpContext, Vcb, FileInfo, status);
    return status;
}

NTSTATUS
UDFStageNewFileEntry(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO FileInfo, uint32 PartNum, int8* Buffer, PBOOLEAN Staged)
{
    *Staged = FALSE;
    return UDFFlushFEInternal(IrpContext, Vcb, FileInfo, PartNum, Buffer, Staged);
}

NTSTATUS
UDFFlushFI(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum
    )
{
    PUDF_FILE_INFO DirInfo = FileInfo->ParentFile;
    PDIR_INDEX_ITEM DirNdx;
    NTSTATUS status;
    SIZE_T WrittenBytes;
    // use WrittenBytes variable to store LBA of FI to be recorded
    #define lba   WrittenBytes

//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    // some files has no FI
    if (!DirInfo || UDFIsAStreamDir(FileInfo))
        return STATUS_SUCCESS;
    DirNdx = UDFDirIndex(DirInfo->Dloc->DirIndex, FileInfo->Index);
//    ASSERT(FileInfo->FileIdent->lengthFileIdent < 0x80);
#ifdef UDF_DBG
    if (DirNdx->FileCharacteristics & FILE_DELETED) {
        ASSERT(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED);
    }
#endif // UDF_DBG
    UDFPrint(("  FlushFI: offs %x\n", (ULONG)(DirNdx->Offset)));

    if ((DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) {
        /* Test the FID we are about to publish, not stale characteristics
         * in its prior buffer. Deleted entries do not publish an FE. */
        if (!FileInfo->FileIdent)
            return STATUS_FILE_CORRUPT_ERROR;
        if ((FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) &&
            !(DirNdx->FileCharacteristics & FILE_DELETED)) {
            status = UDFFlushFE(IrpContext, Vcb, FileInfo, PartNum);
            if (!NT_SUCCESS(status))
                return status;
        }
        // flush FileIdent
        ASSERT(PartNum != (uint32)(-1));
        FileInfo->FileIdent->fileCharacteristics = DirNdx->FileCharacteristics;
        lba = UDFExtentOffsetToLba(Vcb, DirInfo->Dloc->DataLoc.Mapping,
                                        DirNdx->Offset, NULL, NULL, NULL, NULL);
        AdPrint(("  FI lba %x\n", lba));
        // check if requested Offset is allocated
        if (lba == (uint32)LBA_OUT_OF_EXTENT) {
            // write 1 byte
            if (!NT_SUCCESS(status = UDFWriteFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset, 1, FALSE, (int8*)(FileInfo->FileIdent), &WrittenBytes) )) {
                BrutePoint();
                return status;
            }
            lba = UDFExtentOffsetToLba(Vcb, DirInfo->Dloc->DataLoc.Mapping,
                                            DirNdx->Offset, NULL, NULL, NULL, NULL);
            AdPrint(("  allocated FI lba %x\n", lba));
            // check if requested Offset is allocated
            if (lba == (uint32)LBA_OUT_OF_EXTENT) {
                BrutePoint();
                return STATUS_UNSUCCESSFUL;
            }
        }
        // init structure
        UDFSetUpTag(
            Vcb, &(FileInfo->FileIdent->descTag), (uint16)(FileInfo->FileIdentLen), UDFPhysLbaToPart(Vcb, PartNum, lba),
            0);
        // record data
        {
            BOOLEAN publish =
                (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) &&
                !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED) &&
                (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_WRITER) ||
                 IrpContext->MajorFunction == IRP_MJ_FLUSH_BUFFERS);
            BOOLEAN previous = FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_PUBLISH) != 0;
            if (publish)
                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_PUBLISH);
            _SEH2_TRY {
                status = UDFWriteFile__(IrpContext, Vcb, DirInfo, DirNdx->Offset,
                    FileInfo->FileIdentLen, FALSE, (int8*)FileInfo->FileIdent, &WrittenBytes);
            } _SEH2_FINALLY {
                if (!previous)
                    ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_METADATA_PUBLISH);
            } _SEH2_END;
            if (NT_SUCCESS(status) && WrittenBytes != FileInfo->FileIdentLen)
                status = STATUS_IO_DEVICE_ERROR;
            if (!NT_SUCCESS(status))
                return status;
            if (!(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
                if (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) {
                    UDF_METADATA_COUNT(Vcb,31);
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
                    if (IrpContext->MajorFunction == IRP_MJ_CREATE) UDF_METADATA_COUNT(Vcb,32);
                    else if (IrpContext->MajorFunction == IRP_MJ_CLEANUP) UDF_METADATA_COUNT(Vcb,33);
                    else if (IrpContext->MajorFunction == IRP_MJ_CLOSE) UDF_METADATA_COUNT(Vcb,34);
                    else UDF_METADATA_COUNT(Vcb,35);
#endif
                }
                FileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_NEW_UNPUBLISHED;
            }
        }
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_FI_MODIFIED;
    }

    return STATUS_SUCCESS;
} // end UDFFlushFI()

/*
    This routine updates AllocDesc sequence, FileIdent & FileEntry
    for given file
 */
static NTSTATUS
UDFFlushFile__Internal(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN ULONG FlushFlags
    )
{
    ValidateFileInfo(FileInfo);

    if (!FileInfo) return STATUS_SUCCESS;
    NTSTATUS status;
    uint32 PartNum;

    status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status))
        return status;
    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = UDFGetRefPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if (PartNum == (uint32)-1) {
        UDFPrint(("  Is DELETED ?\n"));
        if (FileInfo->ParentFile) {
            PartNum = UDFGetRefPartNumByPhysLba(Vcb, FileInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation);
        } else {
            BrutePoint();
        }
    }
#ifdef UDF_CHECK_DISK_ALLOCATION
    if ( FileInfo->Fcb &&
        (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot) : UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation - Vcb->Partitions[0].PartitionRoot))) {

        if (UDFIsAStreamDir(FileInfo)) {
            if (!UDFIsSDirDeleted(FileInfo)) {
                UDFPrint(("  Not DELETED SDir\n"));
                BrutePoint();
            }
            ASSERT(!FileInfo->Dloc->FELoc.Modified);
        } else
        if (!FileInfo->FileIdent ||
           !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
            if (!FileInfo->FileIdent)
                AdPrint(("  No FileIdent\n"));
            if (FileInfo->FileIdent &&
               !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED))
                AdPrint(("  Not DELETED\n"));
            AdPrint(("Flushing to Discarded block %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
            BrutePoint();
        }
    }
#endif // UDF_CHECK_DISK_ALLOCATION

    // flush FE and pre-allocation charge for directories
    if (FileInfo->Dloc &&
       FileInfo->Dloc->DirIndex) {
        // if Lite Flush is used, keep preallocations
        if (!(FlushFlags & UDF_FLUSH_FLAGS_LITE)) {
            if (FileInfo->Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) {
                FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
                status = UDFResizeExtent(IrpContext, Vcb, PartNum, UDFGetFileSize(FileInfo), FALSE, &(FileInfo->Dloc->DataLoc));
                FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_CUT_PREALLOCATED;
                if (!NT_SUCCESS(status))
                    return status;
                FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_PREALLOCATED;
                AdPrint(("Dir pre-alloc truncated (Flush)\n"));
                FileInfo->Dloc->DataLoc.Modified = TRUE;
            }
        }
    }
    // flush FE
    if (!NT_SUCCESS(status = UDFFlushFE(IrpContext, Vcb, FileInfo, PartNum))) {
        UDFPrint(("Error flushing FE\n"));
        /* Keep the FE and its FID dirty for a later flush.  The write
         * failure does not authorize deleting the file's directory entry. */
        return status;
    }
    if (!NT_SUCCESS(status = UDFFlushFI(IrpContext, Vcb, FileInfo, PartNum)))
        return status;

    ASSERT((FileInfo->Dloc->FileEntry->descVersion == 2) ||
           (FileInfo->Dloc->FileEntry->descVersion == 3));

    return STATUS_SUCCESS;
} // end UDFFlushFile__Internal()

NTSTATUS
UDFFlushFile__(PIRP_CONTEXT IrpContext, PVCB Vcb, PUDF_FILE_INFO FileInfo, ULONG FlushFlags)
{
    NTSTATUS status = UDFFlushFile__Internal(IrpContext, Vcb, FileInfo, FlushFlags);
    UDFRememberMetadataWritebackError(IrpContext, Vcb, FileInfo, status);
    return status;
}

/*
    This routine compares 2 FileInfo structures
 */
BOOLEAN
UDFCompareFileInfo(
    IN PUDF_FILE_INFO f1,
    IN PUDF_FILE_INFO f2
    )
{
    uint_di i;
    PDIR_INDEX_HDR hDirIndex1;
    PDIR_INDEX_HDR hDirIndex2;
    PDIR_INDEX_ITEM DirIndex1;
    PDIR_INDEX_ITEM DirIndex2;

    if (!f1 || !f2) return FALSE;
    if (f1->Dloc->FileEntryLen != f2->Dloc->FileEntryLen) return FALSE;
//    if (f1->FileIdentLen != f2->FileIdentLen) return FALSE;
/*    if (f1->Dloc->DirIndex && !f2->Dloc->DirIndex) return FALSE;
    if (f2->Dloc->DirIndex && !f1->Dloc->DirIndex) return FALSE;
    if ((f1->Dloc->DirIndex) &&
       (f1->Dloc->DirIndex->LastFrameCount != f2->Dloc->DirIndex->LastFrameCount)) return FALSE;*/
    if (f1->Index != f2->Index) return FALSE;
    if (!(f1->Dloc->DataLoc.Mapping)) return FALSE;
    if (!(f2->Dloc->DataLoc.Mapping)) return FALSE;
    if (f1->Dloc->DataLoc.Mapping[0].extLocation != f2->Dloc->DataLoc.Mapping[0].extLocation) return FALSE;
    if (f1->Dloc->DataLoc.Mapping[0].extLength != f2->Dloc->DataLoc.Mapping[0].extLength) return FALSE;
//    if (f1-> != f2->) return FALSE;
//    if (f1-> != f2->) return FALSE;
//    if (f1-> != f2->) return FALSE;
    if (!(f1->Dloc->FileEntry)) return FALSE;
    if (!(f2->Dloc->FileEntry)) return FALSE;
    if (RtlCompareMemory(f1->Dloc->FileEntry, f2->Dloc->FileEntry, f2->Dloc->FileEntryLen) != f2->Dloc->FileEntryLen)
        return FALSE;
    if (!(hDirIndex1 = f1->Dloc->DirIndex)) return FALSE;
    if (!(hDirIndex2 = f2->Dloc->DirIndex)) return FALSE;

    for(i=2; (DirIndex1 = UDFDirIndex(hDirIndex1,i)) &&
             (DirIndex2 = UDFDirIndex(hDirIndex2,i)); i++) {
        if ( DirIndex1->FName.Buffer &&
           !DirIndex2->FName.Buffer)
            return FALSE;
        if ( DirIndex2->FName.Buffer &&
           !DirIndex1->FName.Buffer)
            return FALSE;
        if (!DirIndex2->FName.Buffer &&
           !DirIndex1->FName.Buffer)
            continue;
        if (RtlCompareUnicodeString(&(DirIndex1->FName),
                                   &(DirIndex2->FName),FALSE)) {
            return FALSE;
        }
//        if (DirIndex1[i].FileEntry != DirIndex2[i].FileEntry)
//            return FALSE;
        if (RtlCompareMemory(&(DirIndex1->FileEntryLoc),
                            &(DirIndex2->FileEntryLoc), sizeof(lb_addr)) != sizeof(lb_addr))
            return FALSE;
    }

    return TRUE;
} // end UDFCompareFileInfo()

/*
    This routine computes 32-bit hash based on CRC-32 from SSH
 */
uint32
__fastcall
crc32(
    IN uint8* s,
    IN uint32 len
    )
{
    uint32 i;
    uint32 crc32val = 0;

    for(i=0; i<len; i++, s++) {
        crc32val =
            crc32_tab[(crc32val ^ (*s)) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
} // end crc32()

/*
    Calculate a 16-bit CRC checksum using ITU-T V.41 polynomial.

    The OSTA-UDF(tm) 1.50 standard states that using CRCs is mandatory.
    The polynomial used is: x^16 + x^12 + x^15 + 1
*/
uint16
__fastcall
UDFCrc(
    IN uint8 *Data,
    IN SIZE_T Size,
    IN uint16 Crc
    )
{
    while (Size--)
        Crc = CrcTable[(Crc >> 8 ^ *Data++) & 0xff] ^ (Crc << 8);
    return Crc;
} // end UDFCrc()

/*
    Read the first block of a tagged descriptor & check it.
*/
static BOOLEAN
UDFTaggedSectorValid(
    IN PVCB Vcb,
    IN const int8* Buf,
    IN uint32 Location
    )
{
    const tag* Tag = (const tag*)Buf;
    const uint8* Bytes = (const uint8*)Buf;
    uint8 Checksum = 0;
    uint32 i;

    if (Vcb->SectorSize < sizeof(tag) ||
        Tag->tagLocation != Location ||
        (Tag->descVersion != 2 && Tag->descVersion != 3) ||
        Tag->descCRCLength > Vcb->SectorSize - sizeof(tag))
        return FALSE;

    for (i = 0; i < sizeof(tag); i++) {
        if (i != 4)
            Checksum += Bytes[i];
    }
    if (Checksum != Tag->tagChecksum)
        return FALSE;

    return !Tag->descCRC ||
           Tag->descCRC == UDFCrc((uint8*)Bytes + sizeof(tag),
                                   Tag->descCRCLength, 0);
}

NTSTATUS
UDFReadTagged(
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    int8* Buf,
    uint32 Block,
    uint32 Location,
    uint16 *Ident
    )
{
    NTSTATUS RC;
    tag* PTag = (tag*)Buf;
//    icbtag* Icb = (icbtag*)(Buf+1);
    uint8 checksum;
    unsigned int i;
    int8* tb;
    uint32 MetadataLbn, MirrorLba, MirrorFlags;
    int8* MirrorBuf;
    NTSTATUS MirrorStatus;

    // Read the block
    if (Block == 0xFFFFFFFF)
        return 0;

    _SEH2_TRY {
        RC = UDFReadSectors(IrpContext, Vcb, FALSE, Block, 1, FALSE, Buf);
        if (!NT_SUCCESS(RC)) {
            UDFPrint(("UDF: Block=%x, Location=%x: read failed\n", Block, Location));
            try_return(RC);
        }

        // A duplicated UDF 2.50 metadata partition is a recovery copy, not
        // merely reserved space.  On a bad primary descriptor, validate the
        // corresponding mirror sector before exposing it to the caller.
        if ((Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) &&
            Vcb->MetadataFileInfo && Vcb->MetadataMirrorFileInfo &&
            Vcb->MetadataMirrorFileInfo->Dloc &&
            Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping &&
            !UDFTaggedSectorValid(Vcb, Buf, Location)) {
            MetadataLbn = UDFPhysLbaToMetadata(Vcb, Block);
            if (MetadataLbn != LBA_OUT_OF_EXTENT) {
                MirrorLba = UDFExtentOffsetToLba(
                    Vcb, Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping,
                    ((int64)MetadataLbn) << Vcb->SectorShift,
                    NULL, NULL, &MirrorFlags, NULL);
                if (MirrorLba != LBA_OUT_OF_EXTENT &&
                    MirrorLba != Block &&
                    MirrorFlags == EXTENT_RECORDED_ALLOCATED) {
                    MirrorBuf = (int8*)DbgAllocatePool(NonPagedPool,
                                                       Vcb->SectorSize);
                    if (MirrorBuf) {
                        MirrorStatus = UDFReadSectors(IrpContext, Vcb, FALSE,
                                                      MirrorLba, 1, FALSE,
                                                      MirrorBuf);
                        if (NT_SUCCESS(MirrorStatus) &&
                            UDFTaggedSectorValid(Vcb, MirrorBuf, Location)) {
                            RtlCopyMemory(Buf, MirrorBuf, Vcb->SectorSize);
                            UDFPrint(("UDF: recovered metadata descriptor %x "
                                      "from mirror %x\n", Block, MirrorLba));
                        }
                        DbgFreePool(MirrorBuf);
                    }
                }
            }
        }

        *Ident = PTag->tagIdent;

        if (Location != PTag->tagLocation) {
            UDFPrint(("UDF: location mismatch block %x, tag %x != %x\n",
                Block, PTag->tagLocation, Location));
            try_return(RC = STATUS_FILE_CORRUPT_ERROR);
        }

        /* Verify the tag checksum */
        checksum = 0;
        tb = Buf;
        for (i=0; i<sizeof(tag); i++, tb++)
            checksum += (uint8)((i!=4) ? (*tb) : 0);

        if (checksum != PTag->tagChecksum) {
            UDFPrint(("UDF: tag checksum failed block %x\n", Block));
            try_return(RC = STATUS_CRC_ERROR);
        }

        // Verify the tag version
        if ((PTag->descVersion != 2) &&
           (PTag->descVersion != 3)) {
            UDFPrint(("UDF: Tag version 0x%04x != 0x0002 || 0x0003 block %x\n",
                (PTag->descVersion), Block));
            try_return(RC = STATUS_FILE_CORRUPT_ERROR);
        }

        // Verify the descriptor CRC
        if (((PTag->descCRCLength) + sizeof(tag) <= Vcb->SectorSize) &&
            (((PTag->descCRC) == UDFCrc((uint8 *)Buf + sizeof(tag),
                                       PTag->descCRCLength, 0)) ||
             !(PTag->descCRC)))
        {
            /*        UDFPrint(("Tag ID: %x, ver %x\t", PTag->tagIdent, PTag->descVersion ));
                    if ((i == TID_FILE_ENTRY) ||
                       (i == TID_EXTENDED_FILE_ENTRY)) {
                        UDFPrint(("StrategType: %x, ", Icb->strategyType ));
                        UDFPrint(("FileType: %x\t", Icb->fileType ));
                    }
                    UDFPrint(("\n"));*/
            try_return(RC = STATUS_SUCCESS);
        }
        UDFPrint(("UDF: Crc failure block %x: crc = %x, crclen = %x\n",
            Block, PTag->descCRC, PTag->descCRCLength));
        RC = STATUS_CRC_ERROR;

try_exit:    NOTHING;

    } _SEH2_FINALLY {
        ;
    } _SEH2_END

    return RC;
} // end UDFReadTagged()

/*
    This routine creates hard link for the file from DirInfo1
    to DirInfo2 & names it as fn
 */
NTSTATUS
UDFHardLinkFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN IgnoreCase,
    IN OUT BOOLEAN* Replace,        // replace if destination file exists
    IN PUNICODE_STRING fn,       // destination
    IN OUT PUDF_FILE_INFO DirInfo1,
    IN OUT PUDF_FILE_INFO DirInfo2,
    IN OUT PUDF_FILE_INFO FileInfo  // source (opened)
    )
{
    PUDF_FILE_INFO FileInfo2;
    NTSTATUS status;
    PDIR_INDEX_ITEM DirNdx1;
    PDIR_INDEX_ITEM DirNdx2;
    uint_di i;
    BOOLEAN Recovery = FALSE;
    BOOLEAN SameFE = FALSE;
    uint32 NTAttr = 0;
    ULONGLONG NewFidUniqueId;

    // validate FileInfo
    ValidateFileInfo(DirInfo1);
    ValidateFileInfo(DirInfo2);
    ValidateFileInfo(FileInfo);
    if (FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_NEW_UNPUBLISHED) {
        status = UDFFlushFile__(IrpContext, Vcb, FileInfo, 0);
        if (!NT_SUCCESS(status))
            return status;
    }


    if (UDFGetFileLinkCount(FileInfo) >= UDF_MAX_LINK_COUNT) {
        // too many links to file...
        return STATUS_TOO_MANY_LINKS;
    }

    i = 0;
    if (DirInfo1 == DirInfo2) {
        if (NT_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &i)) &&
           (i==FileInfo->Index) ) {
            // case-only difference
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

    /* Acquire the replacement FID's ID before phase 0 so a failed durable
     * high-water reservation cannot strand a half-linked object. */
    status = UDFAllocateFileUID(IrpContext, Vcb, &NewFidUniqueId);
    if (!NT_SUCCESS(status))
        return status;

    // PHASE 0
    // try to create new FileIdent & FileEntry in Dir2

HLinkRetry:
    if (!NT_SUCCESS(status = UDFCreateFile__(IrpContext, Vcb, IgnoreCase, fn, UDFGetFileEALength(FileInfo),
                    0, (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY),
                    TRUE, DirInfo2, &FileInfo2))) {
        if (UDFCleanUpFile__(Vcb, FileInfo2) && FileInfo2)
            MyFreePool__(FileInfo2);
        if (status == STATUS_ACCESS_DENIED) {
            // try to recover >;->
            if ((*Replace) && !Recovery) {
                Recovery = TRUE;
                status = UDFOpenFile__(IrpContext, Vcb, IgnoreCase, TRUE, fn, DirInfo2, &FileInfo2, NULL);
                if (NT_SUCCESS(status)) {
                    status = UDFDoesOSAllowFileToBeTargetForHLink__(FileInfo2);
                    if (!NT_SUCCESS(status)) {
                        UDFCloseFile__(IrpContext, Vcb, FileInfo2);
                        goto cleanup_and_abort_hlink;
                    }
                    if ((FileInfo->Dloc == FileInfo2->Dloc)  &&
                       (FileInfo != FileInfo2)) {
                        SameFE = TRUE;
                        // 'status' is already STATUS_SUCCESS here
                    } else {
                        status = UDFUnlinkFile__(IrpContext, Vcb, FileInfo2, TRUE);
                    }
                    UDFCloseFile__(IrpContext, Vcb, FileInfo2);
                    if (UDFCleanUpFile__(Vcb, FileInfo2)) {
                        MyFreePool__(FileInfo2);
                        FileInfo2 = NULL;
                        if (SameFE)
                            return STATUS_SUCCESS;
                    } else {
                        // we get here if the FileInfo has associated
                        // system-specific Fcb
                        // Such fact means that not all system references
                        // has already gone (except Linked file case)
                        if (SameFE)
                            return STATUS_SUCCESS;
                        if (!NT_SUCCESS(status) ||
                           (UDFGetFileLinkCount(FileInfo) < 1))
                            status = STATUS_ACCESS_DENIED;
                    }
                    if (NT_SUCCESS(status)) goto HLinkRetry;
                }
cleanup_and_abort_hlink:
                if (FileInfo2 && UDFCleanUpFile__(Vcb, FileInfo2)) {
                    MyFreePool__(FileInfo2);
                    FileInfo2 = NULL;
                }
            } else {
                status = STATUS_OBJECT_NAME_COLLISION;
            }
        }
        return status;
    }
    // update pointers
    DirNdx1 = UDFDirIndex(DirInfo1->Dloc->DirIndex, FileInfo->Index);
    DirNdx2 = UDFDirIndex(DirInfo2->Dloc->DirIndex, FileInfo2->Index);

    // copy file attributes to newly created FileIdent
    NTAttr = UDFAttributesToNT(DirNdx1, FileInfo->Dloc->FileEntry);
    FileInfo2->FileIdent->fileVersionNum = FileInfo->FileIdent->fileVersionNum;

    // PHASE 1
    // copy all necessary info from FileInfo to FileInfo2

    FileInfo2->FileIdent->icb = FileInfo->FileIdent->icb;
    FileInfo2->FileIdent->fileCharacteristics = FileInfo->FileIdent->fileCharacteristics;
    FileInfo2->FileIdent->fileVersionNum = FileInfo->FileIdent->fileVersionNum;

    DirNdx2->FileCharacteristics = DirNdx1->FileCharacteristics & ~FILE_DELETED;
    DirNdx2->FileEntryLoc = DirNdx1->FileEntryLoc;
    DirNdx2->FI_Flags = (DirNdx1->FI_Flags & ~UDF_FI_FLAG_SYS_ATTR) | UDF_FI_FLAG_FI_MODIFIED | UDF_FI_FLAG_LINKED;

    UDFAttributesToUDF(DirNdx2, FileInfo2->Dloc->FileEntry, NTAttr);

    // PHASE 2
    // update FileInfo

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    DirNdx1->FI_Flags = DirNdx2->FI_Flags;
    UDFIncFileLinkCount(FileInfo); // increase to 1
//    UDFUpdateModifyTime(Vcb, FileInfo);
    FileInfo->Dloc->LinkRefCount += FileInfo2->Dloc->LinkRefCount;
    if (FileInfo2->FileIdent)
        ((struct FidADImpUse*)&(FileInfo2->FileIdent->icb.impUse))->uniqueID = (uint32)NewFidUniqueId;

    // PHASE 3
    // drop all unnecessary info from FileInfo2

    UDFFreeFESpace(Vcb, DirInfo2, &(FileInfo2->Dloc->FELoc));
    UDFRemoveDloc(Vcb, FileInfo2->Dloc);

    // PHASE 4
    // perform in-memory linkage (update driver's tree structures) and flush

    FileInfo2->Dloc = FileInfo->Dloc;
    UDFInsertLinkedFile(FileInfo2, FileInfo);

    UDFCloseFile__(IrpContext, Vcb, FileInfo2);
    if (UDFCleanUpFile__(Vcb, FileInfo2)) {
        MyFreePool__(FileInfo2);
    }
    // return 'delete target' status
    (*Replace) = Recovery;

    return STATUS_SUCCESS;
} // end UDFHardLinkFile__()

/*
    This routine allocates FileEntry with in-ICB zero-sized data
    If it returns status != STATUS_SUCCESS caller should call UDFCleanUpFile__
    for returned pointer *WITHOUT* using UDFCloseFile__
 */
NTSTATUS
UDFCreateRootFile__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
//    IN uint16 AllocMode, // short/long/ext/in-icb  // always in-ICB
    IN uint32 PartNum,
    IN uint32 ExtAttrSz,
    IN uint32 ImpUseLen,
    IN BOOLEAN Extended,
    OUT PUDF_FILE_INFO* _FileInfo
    )
{
    NTSTATUS status;
    LONG_AD FEicb;
    PUDF_FILE_INFO FileInfo;
    *_FileInfo = NULL;
    SIZE_T ReadBytes;
    ULONGLONG NewUniqueId;

    status = UDFAllocateFileUID(IrpContext, Vcb, &NewUniqueId);
    if (!NT_SUCCESS(status))
        return status;

    FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
    *_FileInfo = FileInfo;
    if (!FileInfo)
        return STATUS_INSUFFICIENT_RESOURCES;
    ImpUseLen = (ImpUseLen + 3) & ~((uint16)3);

    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // allocate space for FileEntry
    if (!NT_SUCCESS(status =
        UDFBuildFileEntry(IrpContext, Vcb, NULL, FileInfo, PartNum, ICB_FLAG_AD_IN_ICB, ExtAttrSz, Extended) ))
        return status;
    FEicb.extLength = Vcb->SectorSize;
    FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
    RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));

    FileInfo->Dloc->DataLoc.Mapping = UDFExtentToMapping(&(FileInfo->Dloc->FELoc.Mapping[0]));
    if (!(FileInfo->Dloc->DataLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    FileInfo->Dloc->DataLoc.Length = 0;
    FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
    // init FileEntry
    UDFSetFileUID(FileInfo, NewUniqueId);
    UDFSetFileSize(FileInfo, 0);
    UDFIncFileLinkCount(FileInfo); // increase to 1
    UDFUpdateCreateTime(Vcb, FileInfo);
    // zero sector for FileEntry
    FileInfo->Dloc->DataLoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
    FileInfo->Dloc->FELoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
    status = UDFWriteData(IrpContext, Vcb, TRUE, ((int64)(FileInfo->Dloc->FELoc.Mapping[0].extLocation)) << Vcb->SectorShift, Vcb->SectorSize, FALSE, Vcb->ZBuffer, &ReadBytes);
    if (!NT_SUCCESS(status))
        return status;

    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);
    return STATUS_SUCCESS;
} // end UDFCreateRootFile__()

/*
    This routine tries to create StreamDirectory associated with given file
    Caller should use UDFCleanUpFile__ if returned status != STATUS_SUCCESS
 */
NTSTATUS
UDFCreateStreamDir__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
    OUT PUDF_FILE_INFO* _SDirInfo  // this is to be filled & doesn't contain
                                   // any pointers
    )
{
    NTSTATUS status;
    PUDF_FILE_INFO SDirInfo;
    uint16 Ident;

    *_SDirInfo = NULL;
    ValidateFileInfo(FileInfo);
    // check currently recorded UDF revision
    if (!UDFIsStreamsSupported(Vcb))
        return STATUS_INVALID_PARAMETER;
    // check if we are allowed to associate Stream Dir with this file
    if ((FileInfo->ParentFile && UDFIsAStreamDir(FileInfo->ParentFile)) ||
        UDFHasAStreamDir(FileInfo))
        return STATUS_FILE_DELETED;
    // check if we have Deleted SDir
    if (FileInfo->Dloc->SDirInfo &&
       UDFIsSDirDeleted(FileInfo->Dloc->SDirInfo))
        return STATUS_ACCESS_DENIED;
    // check if this file has ExtendedFileEntry
    if ((Ident = FileInfo->Dloc->FileEntry->tagIdent) != TID_EXTENDED_FILE_ENTRY) {
        if (!NT_SUCCESS(status = UDFConvertFEToExtended(IrpContext, Vcb, FileInfo)))
            return status;
    }

    uint32 PartNum = UDFGetRefPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    // create stream directory file
    if (!NT_SUCCESS(status = UDFCreateRootFile__(IrpContext, Vcb, PartNum, 0,0, TRUE, &SDirInfo)))
        return status;
    // link objects
    SDirInfo->ParentFile = FileInfo;
    // record directory structure
    SDirInfo->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED | UDF_FE_FLAG_IS_SDIR);

    status = UDFRecordDirectory__(IrpContext, Vcb, SDirInfo);
    UDFIncFileCounter(Vcb);
    UDFDecDirCounter(Vcb);

    InterlockedIncrement((PLONG)&FileInfo->OpenCount);
    if (!NT_SUCCESS(status)) {
        UDFUnlinkFile__(IrpContext, Vcb, SDirInfo, TRUE);
        UDFCloseFile__(IrpContext, Vcb, SDirInfo);
        UDFCleanUpFile__(Vcb, SDirInfo);
        MyFreePool__(SDirInfo);
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength = 0;
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.partitionReferenceNum = 0;
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.logicalBlockNum = 0;
        return status;
    }
    *_SDirInfo = SDirInfo;
    // do some init
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength = Vcb->SectorSize;
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.partitionReferenceNum = (uint16)PartNum;
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.logicalBlockNum =
        UDFPhysLbaToPart(Vcb, PartNum, SDirInfo->Dloc->FELoc.Mapping[0].extLocation);

    if (SDirInfo->Dloc->FileEntry->tagIdent == TID_FILE_ENTRY) {
        ((PFILE_ENTRY)(SDirInfo->Dloc->FileEntry))->icbTag.fileType = UDF_FILE_TYPE_STREAMDIR;
        ((PFILE_ENTRY)(SDirInfo->Dloc->FileEntry))->uniqueID =
            ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->uniqueID;
    }
    else {
        ((PEXTENDED_FILE_ENTRY)(SDirInfo->Dloc->FileEntry))->icbTag.fileType = UDF_FILE_TYPE_STREAMDIR;
        ((PEXTENDED_FILE_ENTRY)(SDirInfo->Dloc->FileEntry))->uniqueID =
            ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->uniqueID;
    }

    UDFDecFileLinkCount(SDirInfo);
    FileInfo->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED | UDF_FE_FLAG_HAS_SDIR);
    // open & finalize linkage
    FileInfo->Dloc->SDirInfo = SDirInfo;
    return STATUS_SUCCESS;
} // end UDFCreateStreamDir__()

/*
    This routine opens Stream Directory associated with file specified
 */
NTSTATUS
UDFOpenStreamDir__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
    OUT PUDF_FILE_INFO* _SDirInfo  // this is to be filled & doesn't contain
                                   // any pointers
    )
{
    NTSTATUS status;
    PUDF_FILE_INFO SDirInfo;
    PUDF_FILE_INFO ParSDirInfo;
    uint16 Ident;

    *_SDirInfo = NULL;
    ValidateFileInfo(FileInfo);
    // check if this file has ExtendedFileEntry
    if ((Ident = FileInfo->Dloc->FileEntry->tagIdent) != TID_EXTENDED_FILE_ENTRY) {
        return STATUS_NOT_FOUND;
    }
    if ((SDirInfo = FileInfo->Dloc->SDirInfo)) {
        // it is already opened. Good...

        // check if we have Deleted SDir
        if (FileInfo->Dloc->SDirInfo &&
           UDFIsSDirDeleted(FileInfo->Dloc->SDirInfo))
            return STATUS_FILE_DELETED;
        // All right. Look for parallel SDir (if any)
        if (SDirInfo->ParentFile != FileInfo) {
            ParSDirInfo = UDFLocateParallelFI(FileInfo, 0, SDirInfo);
            BrutePoint();
            if (ParSDirInfo->ParentFile != FileInfo) {
                SDirInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_SDFINF_TAG);
                *_SDirInfo = SDirInfo;
                if (!SDirInfo) return STATUS_INSUFFICIENT_RESOURCES;
                RtlCopyMemory(SDirInfo, FileInfo->Dloc->SDirInfo, sizeof(UDF_FILE_INFO));
    //          SDirInfo->NextLinkedFile = FileInfo->Dloc->SDirInfo->NextLinkedFile; // is already done
                UDFInsertLinkedFile(SDirInfo, FileInfo->Dloc->SDirInfo);
                SDirInfo->RefCount = 0;
                SDirInfo->ParentFile = FileInfo;
                SDirInfo->Fcb = NULL;
            } else {
                SDirInfo = ParSDirInfo;
            }
        }
        UDFReferenceFile__(SDirInfo);
        *_SDirInfo = SDirInfo;
        return STATUS_SUCCESS;
    }
    // normal open
    if (!((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength)
        return STATUS_NOT_FOUND;
    SDirInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_SDFINF_TAG);
    if (!SDirInfo) return STATUS_INSUFFICIENT_RESOURCES;
    *_SDirInfo = SDirInfo;
    status = UDFOpenRootFile__(IrpContext, Vcb, &(((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation) ,SDirInfo);
    if (!NT_SUCCESS(status)) return status;
    // open & finalize linkage
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    SDirInfo->Dloc->FE_Flags |= UDF_FE_FLAG_IS_SDIR;
    FileInfo->Dloc->SDirInfo = SDirInfo;
    SDirInfo->ParentFile = FileInfo;

    InterlockedIncrement((PLONG)&FileInfo->OpenCount);

    return STATUS_SUCCESS;
} // end UDFOpenStreamDir__()

/*
    This routine records VAT & VAT Icb at the end of session
 */
NTSTATUS
UDFRecordVAT(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    uint32 Offset;
    uint32 to_read;
    uint32 hdrOffset, hdrOffsetNew;
    uint32 hdrLen;
    NTSTATUS status;
    SIZE_T WrittenBytes;
    uint32 len;
    uint16 PartNdx = (uint16)Vcb->VatPartNdx;
    uint16 PartNum = UDFGetPartNumByPartRef(Vcb, PartNdx);
    uint32 root = UDFPartStart(Vcb, PartNum);
    PUDF_FILE_INFO VatFileInfo = Vcb->VatFileInfo;
    uint32 i;
    PEXTENT_MAP Mapping;
    uint32 off, BS, NWA;
    int8* Old;
    int8* New;
    uint32* Vat;
    uint8 AllocMode;
    uint32 VatLen;
    uint32 PacketOffset;
    uint32 MaxPacket = PACKETSIZE_UDF;
    uint32 OldLen;
    EntityID* eID;

    if (!(Vat = Vcb->Vat) || !VatFileInfo) return STATUS_INVALID_PARAMETER;
    // Disable VAT-based translation
    Vcb->Vat = NULL;
    // sync VAT and FSBM
    // Bitmap is LBN-indexed, VAT index i is already partition-relative
    len = min(UDFPartLen(Vcb, PartNum), Vcb->FSBM_BitCount);
    len = min(Vcb->VatCount, len);
    for(i=0; i<len; i++) {
        BOOLEAN isFree;
        if (Vcb->BitmapFcb) {
            isFree = UDFIsBitmapBitFree(Vcb, i);
        } else {
            isFree = UDFGetFreeBit(Vcb->FSBM_Bitmap, i);
        }
        if (isFree)
            Vat[i] = UDF_VAT_FREE_ENTRY;
    }
    UDFUnpinBitmapPage(Vcb);
    // Ok, now we shall construct new VAT image...
    // !!! NOTE !!!
    // Both VAT copies - in-memory & on-disc
    // contain _relative_ addresses
    OldLen = len = (uint32)UDFGetFileSize(Vcb->VatFileInfo);
    VatLen = (Vcb->SessionEndLba - root + 1) * sizeof(uint32);
    Old = (int8*)DbgAllocatePool(PagedPool, OldLen);
    if (!Old) {
        DbgFreePool(Vat);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // read old one
    status = UDFReadFile__(IrpContext, Vcb, VatFileInfo, 0, OldLen, FALSE, Old);
    if (!NT_SUCCESS(status)) {
        DbgFreePool(Vat);
        DbgFreePool(Old);
        return status;
    }
    // prepare some pointers
    // and fill headers
    if (Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP15) {
        Offset = 0;
        to_read =
        hdrOffset = len - sizeof(struct VirtualAllocationTable15);
        hdrLen = sizeof(struct VirtualAllocationTable15);
        hdrOffsetNew = VatLen;
        New = (int8*)DbgAllocatePool(PagedPool, VatLen + hdrLen);
        if (!New) {
            DbgFreePool(Vat);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(New+hdrOffsetNew, Old+hdrOffset, hdrLen);
        ((struct VirtualAllocationTable15*)(New + hdrOffset))->previousVATICB =
            VatFileInfo->Dloc->FELoc.Mapping[0].extLocation - root;
        eID = &(((struct VirtualAllocationTable15*)(New + hdrOffset))->ident);

        UDFSetEntityID_imp(eID, UDF_ID_ALLOC);

/*        RtlCopyMemory((int8*)&(eID->ident), UDF_ID_ALLOC, sizeof(UDF_ID_ALLOC) );
        iis = (impIdentSuffix*)&(eID->identSuffix);
        iis->OSClass = UDF_OS_CLASS_WINNT;
        iis->OSIdent = UDF_OS_ID_WINNT;*/
    } else {
        struct VirtualAllocationTable20* Buf;

        Offset = ((struct VirtualAllocationTable20*)Old)->lengthHeader;
        to_read = len - Offset;
        hdrOffset = 0;
        hdrLen = sizeof(struct VirtualAllocationTable20);
        hdrOffsetNew = 0;
        New = (int8*)DbgAllocatePool(PagedPool, VatLen + hdrLen);
        if (!New) {
            DbgFreePool(Vat);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(New+hdrOffsetNew, Old+hdrOffset, hdrLen);
        ((struct VirtualAllocationTable20*)New)->previousVatICBLoc =
            VatFileInfo->Dloc->FELoc.Mapping[0].extLocation - root;

        Buf = (struct VirtualAllocationTable20*)New;

        Buf->minReadRevision  = Vcb->minUDFReadRev;
        Buf->minWriteRevision = Vcb->minUDFWriteRev;
        Buf->maxWriteRevision = Vcb->maxUDFWriteRev;

        Buf->numFIDSFiles       = Vcb->numFiles;
        Buf->numFIDSDirectories = Vcb->numDirs;
    }

    RtlCopyMemory(New+Offset, Vat, VatLen);
    //
    if (VatFileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) {
        eID = &(((PEXTENDED_FILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->impIdent);
    } else {
        eID = &(((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->impIdent);
    }

#if 0
    UDFSetEntityID_imp(eID, UDF_ID_DEVELOPER);
#endif

/*    RtlCopyMemory((int8*)&(eID->ident), UDF_ID_DEVELOPER, sizeof(UDF_ID_DEVELOPER) );
    iis = (impIdentSuffix*)&(eID->identSuffix);
    iis->OSClass = UDF_OS_CLASS_WINNT;
    iis->OSIdent = UDF_OS_ID_WINNT;*/

    VatFileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    // drop VAT
    DbgFreePool(Vat);
    len = VatLen;
    // the operation of resize can modifiy WriteCount in WCache due to movement
    // of the data from FE. That's why we should remember PacketOffset now
    if (to_read < VatLen) {
        status = UDFResizeFile__(IrpContext, Vcb, VatFileInfo, len = hdrLen + VatLen);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        UDFMarkSpaceAsXXX(Vcb, VatFileInfo->Dloc, VatFileInfo->Dloc->DataLoc.Mapping, AS_DISCARDED); //free
    }
    //PacketOffset = WCacheGetWriteBlockCount__(&(Vcb->FastCache));
    PacketOffset = 0;
    if ( ((((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) ) {
        // now we'll place FE & built-in data to the last sector of
        // the last packet will be recorded
        if (!PacketOffset) {
            // add padding
            UDFWriteData(IrpContext, Vcb, TRUE, ((uint64)Vcb->NWA) << Vcb->SectorShift, 1, FALSE, Old, &WrittenBytes);
            PacketOffset++;
        } else {
            Vcb->Vat = (uint32*)(New+Offset);
            Vcb->Vat = NULL;
        }
        VatFileInfo->Dloc->FELoc.Mapping[0].extLocation =
        VatFileInfo->Dloc->DataLoc.Mapping[0].extLocation =
            Vcb->NWA+PacketOffset;
        VatFileInfo->Dloc->FELoc.Modified = TRUE;
        // setup descTag
        ((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->descTag.tagLocation =
            UDFPhysLbaToPart(Vcb, PartNum, VatFileInfo->Dloc->DataLoc.Mapping[0].extLocation);
        // record data
        if (NT_SUCCESS(status = UDFWriteFile__(IrpContext, Vcb, VatFileInfo, 0, VatLen + hdrLen, FALSE, New, &WrittenBytes))) {
            status = UDFFlushFile__(IrpContext, Vcb, VatFileInfo, 0);
        }
        return status;
    }
    // We can't fit the whole VAT in FE tail
    // Now lets 'unpack' VAT's mapping to make updating easier
    status = UDFUnPackMapping(Vcb, &(VatFileInfo->Dloc->DataLoc));
    if (!NT_SUCCESS(status)) return status;
    // update VAT with locations of not flushed blocks
    if (PacketOffset) {
        Vcb->Vat = (uint32*)(New+Offset);
        Vcb->Vat = NULL;
    }

    Mapping = VatFileInfo->Dloc->DataLoc.Mapping;
    off=0;
    BS = Vcb->SectorSize;
    NWA = Vcb->NWA;
    VatLen += hdrLen;
    // record modified parts of VAT & update mapping
    for(i=0; Mapping[i].extLength; i++) {
        to_read = (VatLen>=BS) ? BS : VatLen;
        if ((OldLen < off) || (RtlCompareMemory(Old+off, New+off, to_read) != to_read)) {
            // relocate frag
            Mapping[i].extLocation = NWA+PacketOffset;
            Mapping[i].extLength &= UDF_EXTENT_LENGTH_MASK;
            PacketOffset++;
            if (PacketOffset >= MaxPacket) {
                NWA += (MaxPacket + 7);
                PacketOffset = 0;
            }
            status = UDFWriteFile__(IrpContext, Vcb, VatFileInfo, off, to_read, FALSE, New+off, &WrittenBytes);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        VatLen-=BS;
        off+=BS;
    }
    // pack mapping
    UDFPackMapping(Vcb, &(VatFileInfo->Dloc->DataLoc));
    len = UDFGetMappingLength(VatFileInfo->Dloc->DataLoc.Mapping)/sizeof(EXTENT_MAP) - 1;
    // obtain AllocMode
    AllocMode = ((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
    switch(AllocMode) {
    case ICB_FLAG_AD_SHORT: {
        AllocMode = sizeof(SHORT_AD);
        break;
    }
    case ICB_FLAG_AD_LONG: {
        AllocMode = sizeof(LONG_AD);
        break;
    }
    case ICB_FLAG_AD_EXTENDED: {
//            break;
    }
    default: {
        return STATUS_INVALID_PARAMETER;
    }
    }
    // calculate actual AllocSequence length (in blocks)
    len = (len*AllocMode+BS-1+VatFileInfo->Dloc->AllocLoc.Offset) /
//          (((BS - sizeof(ALLOC_EXT_DESC))/sizeof(SHORT_AD))*sizeof(SHORT_AD));
        ((BS - sizeof(ALLOC_EXT_DESC) + AllocMode - 1) & ~(AllocMode-1));
    // Re-init AllocLoc
    if (VatFileInfo->Dloc->AllocLoc.Mapping) MyFreePool__(VatFileInfo->Dloc->AllocLoc.Mapping);
    VatFileInfo->Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , (len+1)*sizeof(EXTENT_MAP),
                                                       MEM_EXTMAP_TAG);
    if (!(VatFileInfo->Dloc->AllocLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;

    VatFileInfo->Dloc->AllocLoc.Offset = (uint32)(VatFileInfo->Dloc->FELoc.Length);
    VatFileInfo->Dloc->AllocLoc.Length = 0;
    Mapping = VatFileInfo->Dloc->AllocLoc.Mapping;
    Mapping[0].extLength = BS-VatFileInfo->Dloc->AllocLoc.Offset;
//  Mapping[0].extLocation = ???;
    for(i=1; i<len; i++) {
        // relocate frag
        Mapping[i].extLocation = NWA+PacketOffset;
        Mapping[i].extLength = BS;
        PacketOffset++;
        if (PacketOffset >= MaxPacket) {
            NWA += (MaxPacket + 7);
            PacketOffset = 0;
        }
    }
    // Terminator
    Mapping[i].extLocation =
    Mapping[i].extLength = 0;

    if ( !PacketOffset &&
        (VatFileInfo->Dloc->AllocLoc.Length <= (Vcb->SectorSize - (uint32)(VatFileInfo->Dloc->AllocLoc.Offset)) ) ) {
        // add padding
        UDFWriteData(IrpContext, Vcb, TRUE, ((uint64)NWA) << Vcb->SectorShift, 1, FALSE, Old, &WrittenBytes);
        PacketOffset++;
    }
    // now we'll place FE & built-in data to the last sector of
    // the last packet will be recorded
    VatFileInfo->Dloc->FELoc.Mapping[0].extLocation =
    VatFileInfo->Dloc->AllocLoc.Mapping[0].extLocation =
        NWA+PacketOffset;
    VatFileInfo->Dloc->FELoc.Modified = TRUE;
    // setup descTag
    ((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->descTag.tagLocation =
        UDFPhysLbaToPart(Vcb, PartNum, VatFileInfo->Dloc->FELoc.Mapping[0].extLocation);
    VatFileInfo->Dloc->DataLoc.Modified = TRUE;

    status = UDFFlushFile__(IrpContext, Vcb, VatFileInfo, 0);
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
} // end UDFRecordVAT()

/*
    This routine updates VAT according to RequestedLbaTable (RelocTab) &
    actual physical address where this data will be stored
 */
NTSTATUS
UDFUpdateVAT(
    IN void* _Vcb,
    IN uint32 Lba,
    IN uint32* RelocTab,  // can be NULL
    IN uint32 BCount
    )
{
    PVCB Vcb = (PVCB)_Vcb;
    uint16 PartNdx = (uint16)(Vcb->VatPartNdx);
    uint16 PartNum = (uint16)(Lba ? UDFGetRefPartNumByPhysLba(Vcb, Lba) : UDFGetPartNumByPartRef(Vcb, PartNdx));
    if (PartNum != UDFGetPartNumByPartRef(Vcb, PartNdx))
    {
        UDFPrint(("UDFUpdateVAT: Write to Write-Protected partition\n"));
        return STATUS_MEDIA_WRITE_PROTECTED;
    }
    // !!! NOTE !!!
    // Both VAT copies - in-memory & on-disc
    // contain _relative_ addresses
    uint32 root = Vcb->Partitions[PartNdx].PartitionRoot;
    uint32 NWA = Vcb->NWA-root;
    uint32 i;
    uint32 CurLba;

    if (!Vcb->Vat) return STATUS_SUCCESS;

    for(i=0; i<BCount; i++, NWA++) {
        if ((CurLba = (RelocTab ? RelocTab[i] : (Lba+i)) - root) >= Vcb->VatCount)
            Vcb->VatCount = CurLba+1;
        Vcb->Vat[CurLba] = NWA;
    }
    return STATUS_SUCCESS;
} // end UDFUpdateVAT()

/*
    This routine converts file's FE to extended form.
    It is needed for stream creation.
 */
NTSTATUS
UDFConvertFEToExtended(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    PEXTENDED_FILE_ENTRY ExFileEntry;
    PFILE_ENTRY FileEntry;
    uint32 Length, NewLength, l;
    NTSTATUS status;
    SIZE_T WrittenBytes;

    if (!FileInfo) return STATUS_INVALID_PARAMETER;
    ValidateFileInfo(FileInfo);
    status = UDFFlushDirectoryAppend(IrpContext, Vcb, FileInfo);
    if (!NT_SUCCESS(status))
        return status;
    if (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) return STATUS_SUCCESS;
    if (FileInfo->Dloc->FileEntry->tagIdent != TID_FILE_ENTRY) return STATUS_INVALID_PARAMETER;

/*    if (!NT_SUCCESS(status = UDFFlushFile__(Vcb, FileInfo)))
        return status;*/

    Length = FileInfo->Dloc->FileEntryLen;
    NewLength = Length - sizeof(FILE_ENTRY) + sizeof(EXTENDED_FILE_ENTRY);
    ExFileEntry = (PEXTENDED_FILE_ENTRY)MyAllocatePoolTag__(NonPagedPool, max(NewLength, Vcb->SectorSize), MEM_XFE_TAG);
    if (!ExFileEntry) return STATUS_INSUFFICIENT_RESOURCES;
    FileEntry = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
    RtlZeroMemory(ExFileEntry, max(NewLength, Vcb->SectorSize));

    ExFileEntry->descTag.tagIdent = TID_EXTENDED_FILE_ENTRY;
    ExFileEntry->icbTag = FileEntry->icbTag;
    ExFileEntry->uid = FileEntry->uid;
    ExFileEntry->gid = FileEntry->gid;
    ExFileEntry->permissions = FileEntry->permissions;
    ExFileEntry->fileLinkCount = FileEntry->fileLinkCount;
    ExFileEntry->recordFormat = FileEntry->recordFormat;
    ExFileEntry->recordDisplayAttr = FileEntry->recordDisplayAttr;
    ExFileEntry->recordLength = FileEntry->recordLength;
    ExFileEntry->informationLength = FileEntry->informationLength;
    ExFileEntry->logicalBlocksRecorded = FileEntry->logicalBlocksRecorded;
    ExFileEntry->accessTime = FileEntry->accessTime;
    ExFileEntry->modificationTime = FileEntry->modificationTime;
    ExFileEntry->attrTime = FileEntry->attrTime;
    ExFileEntry->checkpoint = FileEntry->checkpoint;
    ExFileEntry->extendedAttrICB = FileEntry->extendedAttrICB;
    ExFileEntry->impIdent = FileEntry->impIdent;
    ExFileEntry->uniqueID = FileEntry->uniqueID;
    ExFileEntry->lengthExtendedAttr = FileEntry->lengthExtendedAttr;
    ExFileEntry->lengthAllocDescs = FileEntry->lengthAllocDescs;
    RtlCopyMemory(ExFileEntry+1, FileEntry+1, FileEntry->lengthExtendedAttr);
    RtlCopyMemory((int8*)(ExFileEntry+1)+FileEntry->lengthExtendedAttr, (int8*)(FileEntry+1)+FileEntry->lengthExtendedAttr, FileEntry->lengthAllocDescs);

    if ((((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {

        if ((l = (uint32)(FileInfo->Dloc->DataLoc.Length))) {

            int8* tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, l);
            if (!tmp_buff) {
                MyFreePool__(ExFileEntry);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            if (!NT_SUCCESS(status = UDFReadFile__(IrpContext, Vcb, FileInfo, 0, l, FALSE, tmp_buff)) ||
               !NT_SUCCESS(status = UDFResizeFile__(IrpContext, Vcb, FileInfo, 0)) ) {
                MyFreePool__(ExFileEntry);
                MyFreePool__(tmp_buff);
                return status;
            }
            FileInfo->Dloc->FELoc.Length =
            FileInfo->Dloc->DataLoc.Offset = NewLength;
            FileInfo->Dloc->FELoc.Modified =
            FileInfo->Dloc->DataLoc.Modified = TRUE;
            MyFreePool__(FileInfo->Dloc->FileEntry);
            FileInfo->Dloc->FileEntry = (tag*)ExFileEntry;
            /* The data goes back after the longer Extended FE header. Resize
             * decides whether it still fits in the ICB, and places it, from
             * FileEntryLen, so that must already be the new length; data
             * that no longer fits then moves to an extent. */
            FileInfo->Dloc->FileEntryLen = NewLength;
            if (!NT_SUCCESS(status = UDFResizeFile__(IrpContext, Vcb, FileInfo, l)) ||
               !NT_SUCCESS(status = UDFWriteFile__(IrpContext, Vcb, FileInfo, 0, l, FALSE, tmp_buff, &WrittenBytes)) ) {
                MyFreePool__(ExFileEntry);
                MyFreePool__(tmp_buff);
                return status;
            }
            MyFreePool__(tmp_buff);
        } else {
            FileInfo->Dloc->FELoc.Length =
            FileInfo->Dloc->DataLoc.Offset = NewLength;
            FileInfo->Dloc->FELoc.Modified =
            FileInfo->Dloc->DataLoc.Modified = TRUE;
            MyFreePool__(FileInfo->Dloc->FileEntry);
            FileInfo->Dloc->FileEntry = (tag*)ExFileEntry;
        }
    } else {
        FileInfo->Dloc->FELoc.Length =
        FileInfo->Dloc->AllocLoc.Offset = NewLength;
        FileInfo->Dloc->FELoc.Modified =
        FileInfo->Dloc->AllocLoc.Modified = TRUE;
        MyFreePool__(FileInfo->Dloc->FileEntry);
        FileInfo->Dloc->FileEntry = (tag*)ExFileEntry;
    }
    FileInfo->Dloc->FileEntryLen = NewLength;
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    if (Vcb->minUDFReadRev < 0x0200)
        Vcb->minUDFReadRev = 0x0200;
    return STATUS_SUCCESS;
} // end UDFConvertFEToExtended()

/*
    This routine makes file almost unavailable for external callers.
    The only way to access Hidden with this routine file is OpenByIndex.
    It is usefull calling this routine to pretend file to be deleted,
    for ex. when we have UDFCleanUp__() or smth. like this in progress,
    but we want to create file with the same name.
 */
NTSTATUS
UDFPretendFileDeleted__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    AdPrint(("UDFPretendFileDeleted__:\n"));

    NTSTATUS RC;
    PDIR_INDEX_HDR hDirNdx = UDFGetDirIndexByFileInfo(FileInfo);
    if (!hDirNdx) return STATUS_CANNOT_DELETE;
    PDIR_INDEX_ITEM DirNdx = UDFDirIndex(hDirNdx, FileInfo->Index);
    if (!DirNdx) return STATUS_CANNOT_DELETE;


    // we can't hide file that is not marked as deleted
    RC = UDFDoesOSAllowFilePretendDeleted__(FileInfo);
    if (!NT_SUCCESS(RC))
        return RC;

    AdPrint(("UDFPretendFileDeleted__: set UDF_FI_FLAG_FI_INTERNAL\n"));

    DirNdx->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
    if (DirNdx->FName.Buffer) {
        MyFreePool__(DirNdx->FName.Buffer);
        DirNdx->FName.Buffer = NULL;
        DirNdx->FName.Length =
        DirNdx->FName.MaximumLength = 0;
    }
    return STATUS_SUCCESS;
} // end UDFPretendFileDeleted__()
