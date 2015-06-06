////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
        Module name:

   udf_info.cpp

        Abstract:

   This file contains filesystem-specific routines

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO

#ifdef _X86_
static const int8 valid_char_arr[] =
              {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
               1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
               1,0,1,0, 0,0,0,0, 0,0,1,1, 1,0,0,1,
               0,0,0,0, 0,0,0,0, 0,0,1,1, 1,1,1,1,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // @ABCDE....
               0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,0,0,  // ....Z[/]^_
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  // `abcde....
               0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,0,1,  // ....z{|}~

               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
               0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
#else   // NO X86 optimization , use generic C/C++
static const char valid_char_arr[] = {"*/:?\"<>|\\"};
#endif // _X86_

#define DOS_CRC_MODULUS 41
#define hexChar crcChar
static const char crcChar[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#_~-@";

/* Used to convert hex digits to ASCII for readability. */
//static const char hexChar[] = "0123456789ABCDEF";

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
   This routine allocates new memory block, copies data there & free old one
*/
/*uint32
UDFMemRealloc(
    int8* OldBuff,
    uint32 OldLength,
    int8** NewBuff,
    uint32 NewLength
    )
{
    int8* new_buff;

    (*NewBuff) = OldBuff;
    if(OldLength == NewLength) return OldLength;
    new_buff = (int8*)MyAllocatePool__(NonPagedPool, NewLength);
    if(!new_buff) return 0;
    if(OldLength > NewLength) OldLength = NewLength;
    RtlCopyMemory(new_buff, OldBuff, OldLength);
    MyFreePool__(OldBuff);
    (*NewBuff) = new_buff;
    return OldLength;
} // end UDFMemRealloc()*/

/*
    This routine converts compressed Unicode to standard
 */
void
__fastcall
UDFDecompressUnicode(
    IN OUT PUNICODE_STRING UName,
    IN uint8* CS0,
    IN uint32 Length,
    OUT uint16* valueCRC
    )
{
    uint16 compID = CS0[0];
    uint32 unicodeIndex = 0;
    uint32 byteIndex = 1;
    PWCHAR buff;
    uint8* _CS0 = CS0+1;

    if(!Length) goto return_empty_str;
    // First check for valid compID.
    switch(compID) {
    case UDF_COMP_ID_8: {

        buff = (PWCHAR)MyAllocatePoolTag__(UDF_FILENAME_MT, (Length)*sizeof(WCHAR), MEM_FNAME_TAG);
        if(!buff) goto return_empty_str;
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
        if(!buff) goto return_empty_str;
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
    if(valueCRC) {
        *valueCRC = UDFCrc(CS0+1, Length-1);
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
    IN OUT uint32* Length
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
        if((*Buff) & 0xff00) {
            compID = UDF_COMP_ID_16;
            break;
        }
    }

    CS0 = (uint8*)MyAllocatePool__(NonPagedPool, *Length = (((compID==UDF_COMP_ID_8) ? 1 : 2)*len + 1) );
    if(!CS0) return;

    CS0[0] = compID;
    *_CS0 = CS0;
    // init loop
    CS0++;
    unicodeIndex = 0;
    Buff = UName->Buffer;
    if(compID == UDF_COMP_ID_16) {
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
OSSTATUS UDFFindFile__(IN PVCB Vcb,
                                IN BOOLEAN IgnoreCase,
                                IN PUNICODE_STRING Name,
                                IN PUDF_FILE_INFO DirInfo)
          see 'Udf_info.h'
*/

/*
    This routine reads (Extended)FileEntry according to FileDesc
 */
OSSTATUS
UDFReadFileEntry(
    IN PVCB Vcb,
    IN long_ad* Icb,
 IN OUT PFILE_ENTRY FileEntry, // here we can also get ExtendedFileEntry
 IN OUT uint16* Ident
    )
{
    OSSTATUS status;

    if(!OS_SUCCESS(status = UDFReadTagged(Vcb, (int8*)FileEntry,
                         UDFPartLbaToPhys(Vcb,&(Icb->extLocation)),
                         Icb->extLocation.logicalBlockNum,
                         Ident))) return status;
    if((FileEntry->descTag.tagIdent != TID_FILE_ENTRY) &&
       (FileEntry->descTag.tagIdent != TID_EXTENDED_FILE_ENTRY)) {
        KdPrint(("  Not a FileEntry (lbn=%x, tag=%x)\n", Icb->extLocation.logicalBlockNum, FileEntry->descTag.tagIdent));
        return STATUS_FILE_CORRUPT_ERROR;
    }
    return STATUS_SUCCESS;
} // UDFReadFileEntry()

//#ifndef _X86_
#ifndef _MSC_VER
/*
    Decides if a Unicode character matches one of a list
    of ASCII characters.
    Used by DOS version of UDFIsIllegalChar for readability, since all of the
    illegal characters above 0x0020 are in the ASCII subset of Unicode.
    Works very similarly to the standard C function strchr().
 */
BOOLEAN
UDFUnicodeInString(
    IN uint8* string, // String to search through.
    IN WCHAR ch       // Unicode char to search for.
    ) 
{
    BOOLEAN found = FALSE;

    while(*string != '\0' && !found) {
        // These types should compare, since both are unsigned numbers.
        if(*string == ch) {
            found = TRUE;
        }
        string++;
    }
    return(found);
} // end UDFUnicodeInString()
#endif // _X86_

/*
    Decides whether character passed is an illegal character for a
    DOS file name.
*/
#pragma warning(push)               
#pragma warning(disable:4035)               // re-enable below

#ifdef _X86_
__declspec (naked)
#endif // _X86_
BOOLEAN
__fastcall
UDFIsIllegalChar(
    IN WCHAR chr  // ECX
    )
{
    // Genuine illegal char's for DOS.
//#ifdef _X86_
#ifdef _MSC_VER
  _asm {
    push ebx

    xor  eax,eax
//  mov  ax,chr
    mov  ax,cx
    or   ah,ah
    jnz  ERR_IIC

    lea  ebx,[valid_char_arr]
    xlatb
    jmp  short ERR_IIC2
ERR_IIC:
    mov  al,1
ERR_IIC2:

    pop  ebx
    ret
  }

#else   // NO X86 optimization , use generic C/C++
    /* FIXME */
    //return ((ch < 0x20) || UDFUnicodeInString((uint8*)&valid_char_arr, ch));
    return ((chr < 0x20) || UDFUnicodeInString((uint8*)&valid_char_arr, chr));
#endif // _X86_
} // end UDFIsIllegalChar()

#pragma warning(pop)  // re-enable warning #4035

/*
    Translate udfName to dosName using OSTA compliant.
    dosName must be a unicode string with min length of 12.
 */

/*void UDFDOSName__(
    IN PVCB Vcb,
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN PUDF_FILE_INFO FileInfo
    )
{
    BOOLEAN         KeepIntact;

    KeepIntact = (FileInfo && (FileInfo->Index < 2));
    UDFDOSName(Vcb, DosName, UdfName, KeepIntact);
}*/

void 
__fastcall
UDFDOSName(
    IN PVCB Vcb,
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact
    )
{
#ifndef _CONSOLE
    if(Vcb->CompatFlags & UDF_VCB_IC_OS_NATIVE_DOS_NAME) {
        UDFDOSNameOsNative(DosName, UdfName, KeepIntact);
        return;
    }
#endif //_CONSOLE

    switch(Vcb->CurrentUDFRev) {
    case 0x0100:
    case 0x0101:
    case 0x0102:
        UDFDOSName100(DosName, UdfName, KeepIntact);
        break;

    case 0x0150:
        // in general, we need bytes-from-media to
        // create valid UDF 1.50 name.
        // Curently it is impossible, thus,  we'll use
        // UDF 2.00 translation algorithm
    case 0x0200:
        UDFDOSName200(DosName, UdfName, KeepIntact, Vcb->CurrentUDFRev == 0x0150);
        break;

    case 0x0201:
    default:
        UDFDOSName201(DosName, UdfName, KeepIntact);
    }
}

void 
__fastcall
UDFDOSName100(
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact
    )
{
    PWCHAR dosName = DosName->Buffer;
    PWCHAR udfName = UdfName->Buffer;
    uint32 udfLen = UdfName->Length / sizeof(WCHAR);

    uint32 index, dosIndex = 0, extIndex = 0, lastPeriodIndex;
    BOOLEAN needsCRC = FALSE, hasExt = FALSE, writingExt = FALSE, isParent = FALSE;
    uint32 valueCRC;
    WCHAR ext[DOS_EXT_LEN], current;

    if(KeepIntact &&
       (udfLen <= 2) && (udfName[0] == UNICODE_PERIOD)) {
        isParent = TRUE;
        if((udfLen == 2) && (udfName[1] != UNICODE_PERIOD))
            isParent = FALSE;
    }

    for (index = 0 ; index < udfLen ; index++) {
        current = udfName[index];
        if (current == UNICODE_PERIOD && !isParent) {
            if (dosIndex==0 || hasExt) {
                // Ignore leading periods or any other than used for extension.
                needsCRC = TRUE;
            } else {
                // First, find last character which is NOT a period or space.
                lastPeriodIndex = udfLen - 1;
                while(lastPeriodIndex >=0 &&
                     (udfName[lastPeriodIndex] == UNICODE_PERIOD ||
                      udfName[lastPeriodIndex] == UNICODE_SPACE))
                    lastPeriodIndex--;
                // Now search for last remaining period. 
                while(lastPeriodIndex >= 0 &&
                      udfName[lastPeriodIndex] != UNICODE_PERIOD)
                    lastPeriodIndex--;
                // See if the period we found was the last or not.
                if (lastPeriodIndex != index)
                    needsCRC = TRUE; // If not, name needs translation.
                // As long as the period was not trailing,
                // the file name has an extension.
                if (lastPeriodIndex >= 0) hasExt = TRUE;
            }
        } else {
            if ((!hasExt && dosIndex == DOS_NAME_LEN) ||
                 extIndex == DOS_EXT_LEN) {
                // File name or extension is too long for DOS.
                needsCRC = TRUE;
            } else {
                if (current == UNICODE_SPACE) { // Ignore spaces.
                    needsCRC = TRUE;
                } else {
                    // Look for illegal or unprintable characters.
                    if (UDFIsIllegalChar(current) /*|| !UnicodeIsPrint(current)*/) {
                        needsCRC = TRUE;
                        current = ILLEGAL_CHAR_MARK;
                        /* Skip Illegal characters(even spaces),
                        * but not periods.
                        */
                        while(index+1 < udfLen &&
                              (UDFIsIllegalChar(udfName[index+1]) /*||
                              !UnicodeIsPrint(udfName[index+1])*/) &&
                              udfName[index+1] != UNICODE_PERIOD)
                            index++;
                    }
                    // Add current char to either file name or ext.
                    if (writingExt) {
                        ext[extIndex] = current;
                        extIndex++;
                    } else {
                        dosName[dosIndex] = current;
                        dosIndex++;
                    }
                }
            }
        }
        // See if we are done with file name, either because we reached
        // the end of the file name length, or the final period.
        if (!writingExt && hasExt && (dosIndex == DOS_NAME_LEN ||
            index == lastPeriodIndex)) {
            // If so, and the name has an extension, start reading it.
            writingExt = TRUE;
            // Extension starts after last period.
            index = lastPeriodIndex;
        }
    }
    //
    if (needsCRC) {
        // Add CRC to end of file name or at position 4.
        if (dosIndex >4) dosIndex = 4;
        valueCRC = UDFUnicodeCksum(udfName, udfLen);
        // set CRC prefix
        dosName[dosIndex] = UNICODE_CRC_MARK;
        // Convert 12-bit CRC to hex characters.
        dosName[dosIndex+1] = hexChar[(valueCRC & 0x0f00) >> 8];
        dosName[dosIndex+2] = hexChar[(valueCRC & 0x00f0) >> 4];
        dosName[dosIndex+3] = hexChar[(valueCRC & 0x000f)];
        dosIndex+=4;
    }
    // Add extension, if any.
    if (extIndex != 0) {
        dosName[dosIndex] = UNICODE_PERIOD;
        dosIndex++;
        for (index = 0; index < extIndex; index++) {
            dosName[dosIndex] = ext[index];
            dosIndex++;
        }
    }
    DosName->Length = (uint16)dosIndex*sizeof(WCHAR);
    RtlUpcaseUnicodeString(DosName, DosName, FALSE);
} // end UDFDOSName100()

void 
__fastcall
UDFDOSName200(
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact,
    IN BOOLEAN Mode150
    )
{
    PWCHAR dosName = DosName->Buffer;
    PWCHAR udfName = UdfName->Buffer;
    uint32 udfLen = UdfName->Length / sizeof(WCHAR);

    uint32 index, dosIndex = 0, extIndex = 0, lastPeriodIndex;
    BOOLEAN needsCRC = FALSE, hasExt = FALSE, writingExt = FALSE, isParent = FALSE;
    uint32 valueCRC;
    WCHAR ext[DOS_EXT_LEN], current;

    if(KeepIntact &&
       (udfLen <= 2) && (udfName[0] == UNICODE_PERIOD)) {
        isParent = TRUE;
        if((udfLen == 2) && (udfName[1] != UNICODE_PERIOD))
            isParent = FALSE;
    }

    for (index = 0 ; index < udfLen ; index++) {
        current = udfName[index];
        if (current == UNICODE_PERIOD && !isParent) {
            if (dosIndex==0 || hasExt) {
                // Ignore leading periods or any other than used for extension.
                needsCRC = TRUE;
            } else {
                // First, find last character which is NOT a period or space.
                lastPeriodIndex = udfLen - 1;
                while(lastPeriodIndex >=0 &&
                     (udfName[lastPeriodIndex] == UNICODE_PERIOD ||
                      udfName[lastPeriodIndex] == UNICODE_SPACE))
                    lastPeriodIndex--;
                // Now search for last remaining period. 
                while(lastPeriodIndex >= 0 &&
                      udfName[lastPeriodIndex] != UNICODE_PERIOD)
                    lastPeriodIndex--;
                // See if the period we found was the last or not.
                if (lastPeriodIndex != index)
                    needsCRC = TRUE; // If not, name needs translation.
                // As long as the period was not trailing,
                // the file name has an extension.
                if (lastPeriodIndex >= 0) hasExt = TRUE;
            }
        } else {
            if ((!hasExt && dosIndex == DOS_NAME_LEN) ||
                 extIndex == DOS_EXT_LEN) {
                // File name or extension is too long for DOS.
                needsCRC = TRUE;
            } else {
                if (current == UNICODE_SPACE) { // Ignore spaces.
                    needsCRC = TRUE;
                } else {
                    // Look for illegal or unprintable characters.
                    if (UDFIsIllegalChar(current) /*|| !UnicodeIsPrint(current)*/) {
                        needsCRC = TRUE;
                        current = ILLEGAL_CHAR_MARK;
                        /* Skip Illegal characters(even spaces),
                        * but not periods.
                        */
                        while(index+1 < udfLen &&
                              (UDFIsIllegalChar(udfName[index+1]) /*||
                              !UnicodeIsPrint(udfName[index+1])*/) &&
                              udfName[index+1] != UNICODE_PERIOD)
                            index++;
                    }
                    // Add current char to either file name or ext.
                    if (writingExt) {
                        ext[extIndex] = current;
                        extIndex++;
                    } else {
                        dosName[dosIndex] = current;
                        dosIndex++;
                    }
                }
            }
        }
        // See if we are done with file name, either because we reached
        // the end of the file name length, or the final period.
        if (!writingExt && hasExt && (dosIndex == DOS_NAME_LEN ||
            index == lastPeriodIndex)) {
            // If so, and the name has an extension, start reading it.
            writingExt = TRUE;
            // Extension starts after last period.
            index = lastPeriodIndex;
        }
    }
    // Now handle CRC if needed.
    if (needsCRC) {
        // Add CRC to end of file name or at position 4.
        if (dosIndex >4) dosIndex = 4;
        valueCRC = Mode150 ? UDFUnicodeCksum150(udfName, udfLen) : UDFUnicodeCksum(udfName, udfLen);
        // Convert 16-bit CRC to hex characters.
        dosName[dosIndex] = hexChar[(valueCRC & 0xf000) >> 12];
        dosName[dosIndex+1] = hexChar[(valueCRC & 0x0f00) >> 8];
        dosName[dosIndex+2] = hexChar[(valueCRC & 0x00f0) >> 4];
        dosName[dosIndex+3] = hexChar[(valueCRC & 0x000f)];
        dosIndex+=4;
    }
    // Add extension, if any.
    if (extIndex != 0) {
        dosName[dosIndex] = UNICODE_PERIOD;
        dosIndex++;
        for (index = 0; index < extIndex; index++) {
            dosName[dosIndex] = ext[index];
            dosIndex++;
        }
    }
    DosName->Length = (uint16)dosIndex*sizeof(WCHAR);
    RtlUpcaseUnicodeString(DosName, DosName, FALSE);
} // end UDFDOSName200()


void 
__fastcall
UDFDOSName201(
    IN OUT PUNICODE_STRING DosName,
    IN PUNICODE_STRING UdfName,
    IN BOOLEAN KeepIntact
    )
{
    PWCHAR dosName = DosName->Buffer;
    PWCHAR udfName = UdfName->Buffer;
    uint16 udfLen = UdfName->Length / sizeof(WCHAR);

    uint16 index, dosIndex = 0;
    //uint16 extIndex = 0;
    BOOLEAN needsCRC = FALSE, isParent = FALSE;
    //BOOLEAN hasExt = FALSE, writingExt = FALSE;
    uint16 valueCRC;
    WCHAR ext[DOS_EXT_LEN];
    WCHAR current;

    if(KeepIntact &&
       (udfLen <= 2) && (udfName[0] == UNICODE_PERIOD)) {
        isParent = TRUE;
        if((udfLen == 2) && (udfName[1] != UNICODE_PERIOD))
            isParent = FALSE;
    }

    #define DOS_CRC_LEN 4
    #define DOS_CRC_MODULUS 41

    int16 crcIndex;
    uint16 extLen;
    uint16 nameLen;
    uint16 charLen;
    int16 overlayBytes;
    int16 bytesLeft;

    /* Start at the end of the UDF file name and scan for a period */
    /* ('.'). This will be where the DOS extension starts (if */
    /* any). */
    index = udfLen;
    while (index-- > 0) {
        if (udfName[index] == '.')
            break;
    }
    if ((index < 0) || isParent) {
        /* There name was scanned to the beginning of the buffer */
        /* and no extension was found. */
        extLen = 0;
        nameLen = udfLen;
    } else {
        /* A DOS extension was found, process it first. */
        extLen = udfLen - index - 1;
        nameLen = index;
        dosIndex = 0;
        bytesLeft = DOS_EXT_LEN;
        while (++index < udfLen && bytesLeft > 0) {
            /* Get the current character and convert it to upper */
            /* case. */
            current = udfName[index];
            if (current == ' ') {
                /* If a space is found, a CRC must be appended to */
                /* the mangled file name. */
                needsCRC = TRUE;
            } else {
                /* Determine if this is a valid file name char and */
                /* calculate its corresponding BCS character byte */
                /* length (zero if the char is not legal or */
                /* undisplayable on this system). */

                charLen = (UDFIsIllegalChar(current)
                           /*|| !UnicodeIsPrint(current)*/) ? 0 : 1;

                /* If the char is larger than the available space */
                /* in the buffer, pretend it is undisplayable. */
                if (charLen > bytesLeft)
                    charLen = 0;
                if (charLen == 0) {
                /* Undisplayable or illegal characters are */
                /* substituted with an underscore ("_"), and */
                /* required a CRC code appended to the mangled */
                /* file name. */
                needsCRC = TRUE;
                charLen = 1;
                current = '_';
                /* Skip over any following undiplayable or */
                /* illegal chars. */
                while (index +1 <udfLen &&
                        (UDFIsIllegalChar(udfName[index+1])
                       /*|| !UnicodeIsPrint(udfName[index+1])*/))
                    index++;
                }
                /* Assign the resulting char to the next index in */
                /* the extension buffer and determine how many BCS */
                /* bytes are left. */
                ext[dosIndex++] = current;
                bytesLeft -= charLen;
            }
        }
        /* Save the number of Unicode characters in the extension */
        extLen = dosIndex;
        /* If the extension was too large, or it was zero length */
        /* (i.e. the name ended in a period), a CRC code must be */
        /* appended to the mangled name. */
        if (index < udfLen || extLen == 0)
            needsCRC = TRUE;
    }
    /* Now process the actual file name. */
    index = 0;
    dosIndex = 0;
    crcIndex = 0;
    overlayBytes = -1;
    bytesLeft = DOS_NAME_LEN;
    while (index < nameLen && bytesLeft > 0) {
        /* Get the current character and convert it to upper case. */
        current = udfName[index];
        if (current ==' ' || (current == '.' && !isParent) ) {
            /* Spaces and periods are just skipped, a CRC code */
            /* must be added to the mangled file name. */
            needsCRC = TRUE;
        } else {
            /* Determine if this is a valid file name char and */
            /* calculate its corresponding BCS character byte */
            /* length (zero if the char is not legal or */
            /* undisplayable on this system). */
            
            charLen = (UDFIsIllegalChar(current)
                       /*|| !UnicodeIsPrint(current)*/) ? 0 : 1;

            /* If the char is larger than the available space in */
            /* the buffer, pretend it is undisplayable. */
            if (charLen > bytesLeft)
                charLen = 0;

            if (charLen == 0) {
                /* Undisplayable or illegal characters are */
                /* substituted with an underscore ("_"), and */
                /* required a CRC code appended to the mangled */
                /* file name. */
                needsCRC = TRUE;
                charLen = 1;
                current = '_';
                /* Skip over any following undisplayable or illegal */
                /* chars. */
                while (index +1 <nameLen &&
                       (UDFIsIllegalChar(udfName[index+1])
                        /*|| !UnicodeIsPrint(udfName[index+1])*/))
                    index++;
                /* Terminate loop if at the end of the file name. */
                if (index >= nameLen)
                    break;
            }
            /* Assign the resulting char to the next index in the */
            /* file name buffer and determine how many BCS bytes */
            /* are left. */
            dosName[dosIndex++] = current;
            bytesLeft -= charLen;
            /* This figures out where the CRC code needs to start */
            /* in the file name buffer. */
            if (bytesLeft >= DOS_CRC_LEN) {
                /* If there is enough space left, just tack it */
                /* onto the end. */
                crcIndex = dosIndex;
            } else {
                /* If there is not enough space left, the CRC */
                /* must overlay a character already in the file */
                /* name buffer. Once this condition has been */
                /* met, the value will not change. */
                if (overlayBytes < 0) {
                    /* Determine the index and save the length of */
                    /* the BCS character that is overlayed. It */
                    /* is possible that the CRC might overlay */
                    /* half of a two-byte BCS character depending */
                    /* upon how the character boundaries line up. */
                    overlayBytes = (bytesLeft + charLen > DOS_CRC_LEN)?1 :0;
                    crcIndex = dosIndex - 1;
                }
            }
        }
        /* Advance to the next character. */
        index++;
    }
    /* If the scan did not reach the end of the file name, or the */
    /* length of the file name is zero, a CRC code is needed. */
    if (index < nameLen || index == 0)
        needsCRC = TRUE;

    /* If the name has illegal characters or and extension, it */
    /* is not a DOS device name. */

/*    if (needsCRC == FALSE && extLen == 0) {
        /* If this is the name of a DOS device, a CRC code should */
        /* be appended to the file name. 
        if (IsDeviceName(udfName, udfLen))
            needsCRC = TRUE;
    }*/

    /* Append the CRC code to the file name, if needed. */
    if (needsCRC) {
        /* Get the CRC value for the original Unicode string */
        valueCRC = UDFUnicodeCksum(udfName, udfLen);

        /* begin. */
        dosIndex = crcIndex;
        /* If the character being overlayed is a two-byte BCS */
        /* character, replace the first byte with an underscore. */
        if (overlayBytes > 0)
            dosName[dosIndex++] = '_';
        /* Append the encoded CRC value with delimiter. */
        dosName[dosIndex++] = '#';
        dosName[dosIndex++] =
            crcChar[valueCRC / (DOS_CRC_MODULUS * DOS_CRC_MODULUS)];
        valueCRC %= DOS_CRC_MODULUS * DOS_CRC_MODULUS;
            dosName[dosIndex++] =
        crcChar[valueCRC / DOS_CRC_MODULUS];
            valueCRC %= DOS_CRC_MODULUS;
        dosName[dosIndex++] = crcChar[valueCRC];
    }
    /* Append the extension, if any. */
    if (extLen > 0) {
        /* Tack on a period and each successive byte in the */
        /* extension buffer. */
        dosName[dosIndex++] = '.';
        for (index = 0; index < extLen; index++)
            dosName[dosIndex++] = ext[index];
    }
    /* Return the length of the resulting Unicode string. */
    DosName->Length = (uint16)dosIndex*sizeof(WCHAR);
    RtlUpcaseUnicodeString(DosName, DosName, FALSE);

} // end UDFDOSName201()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine initializes Tag structure. It must be called after all
    manual settings to generate valid CRC & Checksum
 */
void
UDFSetUpTag(
    IN PVCB Vcb,
    IN tag* Tag,
    IN uint16 DataLen,  // total length of descriptor _including_ icbTag
    IN uint32 TagLoc
    )
{
    uint32 i;
    int8* tb;

    AdPrint(("UDF: SetTag Loc=%x(%x), tagIdent=%x\n", TagLoc, Tag->tagLocation, Tag->tagIdent));

    if(DataLen) DataLen -= sizeof(tag);
//    int8* Data = ((int8*)Tag) + sizeof(tag);
    // Ecma-167 states, that all implementations
    // shall set this field to '3' even if
    // disc contains descriptors recorded with
    // value '2'
    // But we should ignore this to make happy othe UDF implementations :(
    Tag->descVersion = (Vcb->NSRDesc & VRS_NSR03_FOUND) ? 3 : 2;
    Tag->tagLocation = TagLoc;
    Tag->tagSerialNum = (uint16)(Vcb->SerialNumber + 1);
    Tag->descCRCLength = DataLen;
    Tag->descCRC = UDFCrc((uint8*)(Tag+1), DataLen);
    Tag->tagChecksum = 0;
    tb = ((int8*)Tag);
    for (i=0; i<sizeof(tag); i++,tb++)
        Tag->tagChecksum += (i!=4) ? (*tb) : 0;
} // end UDFSetUpTag()

/*
    This routine builds FileEntry & associated AllocDescs for specified
    extent.
 */
OSSTATUS
UDFBuildFileEntry(
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
    OSSTATUS status;
    EntityID* eID;
    uint32 l;
    EXTENT_INFO _FEExtInfo;
    uint16* lcp;

    ASSERT(!PartNum);
    ASSERT(!ExtAttrSz);
    // calculate the length required
    l = (Extended ? sizeof(EXTENDED_FILE_ENTRY) : sizeof(FILE_ENTRY)) + ExtAttrSz;
    if(l > Vcb->LBlockSize) return STATUS_INVALID_PARAMETER;
    // allocate block for FE
    if(!OS_SUCCESS(status = UDFAllocateFESpace(Vcb, DirInfo, PartNum, &_FEExtInfo, l) ))
        return status;
    // remember FE location for future hard link creation
    ASSERT(UDFFindDloc(Vcb, _FEExtInfo.Mapping[0].extLocation) == (-1));
    if(!OS_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, _FEExtInfo.Mapping[0].extLocation))) {
        ASSERT(status != STATUS_SHARING_PAUSED);
        UDFFreeFESpace(Vcb, DirInfo, &_FEExtInfo); // free
        MyFreePool__(_FEExtInfo.Mapping);
        return status;
    }
    FileEntry = (PFILE_ENTRY)MyAllocatePoolTag__(NonPagedPool, l, MEM_FE_TAG);
    if(!FileEntry) {
        UDFRemoveDloc(Vcb, FileInfo->Dloc);
        FileInfo->Dloc = NULL;
        UDFFreeFESpace(Vcb, DirInfo, &_FEExtInfo); // free
        MyFreePool__(_FEExtInfo.Mapping);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    FileInfo->Dloc->FELoc = _FEExtInfo;

    RtlZeroMemory((int8*)FileEntry, l);
    // set up in-memory FE structure
    FileEntry->icbTag.flags = AllocMode;
    FileEntry->icbTag.fileType = UDF_FILE_TYPE_REGULAR;
    FileEntry->icbTag.numEntries = 1;
//    if(DirInfo && DirInfo->Dloc && DirInfo->Dloc
    FileEntry->icbTag.strategyType = 4;
//    FileEntry->icbTag.strategyParameter = 0;
    FileEntry->descTag.tagIdent = Extended ? TID_EXTENDED_FILE_ENTRY : TID_FILE_ENTRY;
    FileEntry->descTag.tagLocation = UDFPhysLbaToPart(Vcb, PartNum, _FEExtInfo.Mapping[0].extLocation);
    FileEntry->uid = Vcb->DefaultUID;
    FileEntry->gid = Vcb->DefaultGID;

    if(Extended) {
        eID = &(((PEXTENDED_FILE_ENTRY)FileEntry)->impIdent);
        lcp = &(((PEXTENDED_FILE_ENTRY)FileEntry)->fileLinkCount);
        ((PEXTENDED_FILE_ENTRY)FileEntry)->checkpoint = 1;
    } else {
        eID = &(FileEntry->impIdent);
        lcp = &(FileEntry->fileLinkCount);
        ((PFILE_ENTRY)FileEntry)->checkpoint = 1;
    }

#if 0
    UDFSetEntityID_imp(eID, UDF_ID_DEVELOPER);
#endif

    /*RtlCopyMemory((int8*)&(eID->ident), UDF_ID_DEVELOPER, sizeof(UDF_ID_DEVELOPER) );
    iis = (impIdentSuffix*)&(eID->identSuffix);
    iis->OSClass = UDF_OS_CLASS_WINNT;
    iis->OSIdent = UDF_OS_ID_WINNT;*/

    *lcp = 0xffff;

    FileInfo->Dloc->FileEntry = (tag*)FileEntry;
    FileInfo->Dloc->FileEntryLen = l;

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

    return STATUS_SUCCESS;
} // end UDFBuildFileEntry()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine builds ExtentInfo for specified (Ex)FileEntry & associated
    AllocDescs
 */
OSSTATUS
UDFLoadExtInfo(
    IN PVCB Vcb,
    IN PFILE_ENTRY fe,
    IN PLONG_AD fe_loc,
 IN OUT PEXTENT_INFO FExtInfo,  // user data
 IN OUT PEXTENT_INFO AExtInfo   // alloc descs
    )
{
    EXTENT_AD TmpExt;

    KdPrint(("  UDFLoadExtInfo:\n"));
    FExtInfo->Mapping = UDFReadMappingFromXEntry(Vcb, fe_loc->extLocation.partitionReferenceNum,
                                       (tag*)fe, &(FExtInfo->Offset), AExtInfo);
    if(!(FExtInfo->Mapping)) {
        if(!(FExtInfo->Offset))
            return STATUS_UNSUCCESSFUL;
        TmpExt.extLength = fe_loc->extLength;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &(fe_loc->extLocation));
        if(TmpExt.extLocation == LBA_OUT_OF_EXTENT)
            return STATUS_FILE_CORRUPT_ERROR;
        FExtInfo->Mapping = UDFExtentToMapping(&TmpExt);
    }
    if(fe->descTag.tagIdent == TID_FILE_ENTRY) {
//        KdPrint(("Standard FileEntry\n"));
        FExtInfo->Length = fe->informationLength;
    } else /*if(fe->descTag.tagIdent == TID_EXTENDED_FILE_ENTRY) */ {
        FExtInfo->Length = ((PEXTENDED_FILE_ENTRY)fe)->informationLength;
    }
    KdPrint(("  FExtInfo->Length %x\n", FExtInfo->Length));
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
OSSTATUS
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
    uint32 Nlen, l;
    // prepare filename
    UDFCompressUnicode(fn, &CS0, &Nlen);
    if(!CS0) return STATUS_INSUFFICIENT_RESOURCES;
    if(Nlen < 2) {
        Nlen = 0;
    } else
    if(Nlen > UDF_NAME_LEN) {
        if(CS0) MyFreePool__(CS0);
        return STATUS_OBJECT_NAME_INVALID;
    }
    // allocate memory for FI
    l = (sizeof(FILE_IDENT_DESC) + Nlen + ImpUseLen + 3) & ~((uint32)3);
    FileId = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, l, MEM_FID_TAG);
    if(!FileId) {
        if(CS0) MyFreePool__(CS0);
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

    if(CS0) MyFreePool__(CS0);
    return STATUS_SUCCESS;
} // end UDFBuildFileIdent()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine sets informationLength field in (Ext)FileEntry
 */
void
UDFSetFileSize(
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Size
    )
{
    uint16 Ident;
//    PDIR_INDEX_ITEM DirIndex;

    ValidateFileInfo(FileInfo);
    AdPrint(("UDFSetFileSize: %I64x, FI %x\n", Size, FileInfo));

    //AdPrint(("  Dloc %x\n", FileInfo->Dloc));
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    //AdPrint(("  FileEntry %x\n", FileInfo->Dloc->FileEntry));
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    //AdPrint(("  Ident %x\n", Ident));
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        //AdPrint(("  fe %x\n", fe));
        fe->informationLength = Size;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        //AdPrint(("  ext-fe %x\n", fe));
        fe->informationLength = Size;
    }
/*    if(DirIndex = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index) ) {
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
    if(ASize) {
        AdPrint(("UDFSetFileSizeInDirNdx: %I64x\n", *ASize));
    } else {
        AdPrint(("UDFSetFileSizeInDirNdx: sync\n"));
    }

    DirIndex = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index);
    if(!DirIndex)
       return;

    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        DirIndex->FileSize = fe->informationLength;
        if(ASize) {
            DirIndex->AllocationSize = *ASize;
//        } else {
//            DirIndex->AllocationSize = (fe->informationLength + Vcb->LBlockSize - 1) & ~(Vcb->LBlockSize - 1);
        }
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        DirIndex->FileSize = fe->informationLength;
        if(ASize) {
            DirIndex->AllocationSize = *ASize;
//        } else {
//            DirIndex->AllocationSize = (fe->informationLength + Vcb->LBlockSize - 1) & ~(Vcb->LBlockSize - 1);
        }
    }
    return;
} // end UDFSetFileSizeInDirNdx()
#endif //UDF_READ_ONLY_BUILD

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
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        return fe->informationLength;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
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
    if(!DirIndex)
       return -1;

    return DirIndex->FileSize;
} // end UDFGetFileSizeFromDirNdx()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine sets lengthAllocDesc field in (Ext)FileEntry
 */
void
UDFSetAllocDescLen(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;

    ValidateFileInfo(FileInfo);

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if(FileInfo->Dloc->AllocLoc.Length) {
            fe->lengthAllocDescs = min(FileInfo->Dloc->AllocLoc.Mapping[0].extLength -
                                       FileInfo->Dloc->AllocLoc.Offset,
                                       (uint32)(FileInfo->Dloc->AllocLoc.Length));
        } else
        if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
            fe->lengthAllocDescs = (uint32)(FileInfo->Dloc->DataLoc.Length);
        }
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if(FileInfo->Dloc->AllocLoc.Length) {
            fe->lengthAllocDescs = min(FileInfo->Dloc->AllocLoc.Mapping[0].extLength -
                                       FileInfo->Dloc->AllocLoc.Offset,
                                       (uint32)(FileInfo->Dloc->AllocLoc.Length));
        } else
        if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
            fe->lengthAllocDescs = (uint32)(FileInfo->Dloc->DataLoc.Length);
        }
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
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if(Increase) {
            fe->fileLinkCount++;
        } else {
            fe->fileLinkCount--;
        }
        if(fe->fileLinkCount & 0x8000)
            fe->fileLinkCount = 0xffff;
        return;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        if(Increase) {
            fe->fileLinkCount++;
        } else {
            fe->fileLinkCount--;
        }
        if(fe->fileLinkCount & 0x8000)
            fe->fileLinkCount = 0xffff;
        return;
    }
    return;
} // end UDFChangeFileLinkCount()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine gets fileLinkCount field from (Ext)FileEntry
 */
uint16
UDFGetFileLinkCount(
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;
    uint16 d;

    ValidateFileInfo(FileInfo);

    if(!FileInfo->Dloc->FileEntry)
        return 1;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    // UDF engine assumes that LinkCount is a counter
    // of FileIdents, referencing this FE.
    // UDF 2.0 states, that it should be counter of ALL
    // references (including SDir) - 1.
    // Thus we'll write to media UDF-required value, but return
    // cooked value to callers
    d = UDFHasAStreamDir(FileInfo) ? 0 : 1;
    if(Ident == TID_FILE_ENTRY) {
        return ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount + d;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        return ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount + d;
    }
    return UDF_INVALID_LINK_COUNT;
} // end UDFGetFileLinkCount()

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
    uint16 d;

    ValidateFileInfo(FileInfo);

    if(!FileInfo->Dloc->FileEntry)
        return;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    // UDF engine assumes that LinkCount is a counter
    // of FileIdents, referencing this FE.
    // UDF 2.0 states, that it should be counter of ALL
    // references (including SDir) - 1.
    // Thus we'll write to media UDF-required value, but return
    // cooked value to callers
    d = UDFHasAStreamDir(FileInfo) ? 0 : 1;
    if(Ident == TID_FILE_ENTRY) {
        ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount = LinkCount - d;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->fileLinkCount = LinkCount - d;
    }
    return;
} // end UDFGetFileLinkCount()
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

    if(!FileInfo->Dloc->FileEntry)
        return 1;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;

    if(Ident == TID_FILE_ENTRY) {
        return ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->lengthExtendedAttr;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        return ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->lengthExtendedAttr;
    }
    return 0;
} // end UDFGetFileEALength()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine sets UniqueID field in (Ext)FileEntry
 */
int64
UDFAssingNewFUID(
    IN PVCB Vcb
    )
{
    Vcb->NextUniqueId++;
    if(!((uint32)(Vcb->NextUniqueId)))
        Vcb->NextUniqueId += 16;
    return Vcb->NextUniqueId;
}

void
UDFSetFileUID(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    uint16 Ident;
    int64 UID;

    ValidateFileInfo(FileInfo);

    UID = UDFAssingNewFUID(Vcb);

/*    UID = FileInfo->Dloc->FELoc.Mapping[0].extLocation |
          ( FileInfo->ParentFile ? (((int64)(FileInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation)) << 32) : 0);*/

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    Ident = FileInfo->Dloc->FileEntry->tagIdent;
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->uniqueID = UID;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
        PEXTENDED_FILE_ENTRY fe = (PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry);
        fe->uniqueID = UID;
    }
    if(FileInfo->FileIdent)
        ((FidADImpUse*)&(FileInfo->FileIdent->icb.impUse))->uniqueID = (uint32)UID;
    return;
} // end UDFSetFileUID()
#endif //UDF_READ_ONLY_BUILD

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
    if(Ident == TID_FILE_ENTRY) {
        PFILE_ENTRY fe = (PFILE_ENTRY)(FileEntry);
        return fe->uniqueID;
    } else if(Ident == TID_EXTENDED_FILE_ENTRY) {
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

#ifndef UDF_READ_ONLY_BUILD
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
    if(*counter == -1)
        return;
    if(Increase) {
        UDFInterlockedIncrement((int32*)counter);
    } else {
        UDFInterlockedDecrement((int32*)counter);
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

    RtlCopyMemory( (int8*)&(eID->ident), Str, Len );
    iis = (impIdentSuffix*)&(eID->identSuffix);
    iis->OSClass = UDF_OS_CLASS_WINNT;
    iis->OSIdent = UDF_OS_ID_WINNT;

} // end UDFSetEntityID_imp_()
#endif //UDF_READ_ONLY_BUILD

void
UDFReadEntityID_Domain(
    PVCB Vcb,
    EntityID* eID
    )
{
    domainIdentSuffix* dis;
    uint8 flags;

    dis = (domainIdentSuffix*)&(eID->identSuffix);

    KdPrint(("UDF: Entity Id:\n"));
    KdPrint(("flags: %x\n", eID->flags));
    KdPrint(("ident[0]: %x\n", eID->ident[0]));
    KdPrint(("ident[1]: %x\n", eID->ident[1]));
    KdPrint(("ident[2]: %x\n", eID->ident[2]));
    KdPrint(("ident[3]: %x\n", eID->ident[3]));
    KdPrint(("UDF: Entity Id Domain:\n"));
    // Get current UDF revision
    Vcb->CurrentUDFRev = max(dis->currentRev, Vcb->CurrentUDFRev);
    KdPrint(("Effective Revision: %x\n", Vcb->CurrentUDFRev));
    // Get Read-Only flags
    flags = dis->flags;
    KdPrint(("Flags: %x\n", flags));
    if((flags & ENTITYID_FLAGS_SOFT_RO) &&
        (Vcb->CompatFlags & UDF_VCB_IC_SOFT_RO)) {
        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
        Vcb->UserFSFlags |= UDF_USER_FS_FLAGS_SOFT_RO;
        KdPrint(("       Soft-RO\n"));
    }
    if((flags & ENTITYID_FLAGS_HARD_RO) &&
       (Vcb->CompatFlags & UDF_VCB_IC_HW_RO)) {
        Vcb->VCBFlags |= UDF_VCB_FLAGS_MEDIA_READ_ONLY;
        Vcb->UserFSFlags |= UDF_USER_FS_FLAGS_HW_RO;
        KdPrint(("       Hard-RO\n"));
    }

} // end UDFReadEntityID_Domain()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine writes data to file & increases it if necessary.
    In case of increasing AllocDescs will be rebuilt & flushed to disc
    (via driver's cache, of cource). Free space map will be updated only
    durring global media flush.
 */
OSSTATUS
UDFWriteFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,
    IN uint32 Length,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT uint32* WrittenBytes
    )
{
    int64 t, elen;
    OSSTATUS status;
    int8* OldInIcb = NULL;
    ValidateFileInfo(FileInfo);
    uint32 ReadBytes;
    uint32 _WrittenBytes;
    PUDF_DATALOC_INFO Dloc;
    // unwind staff
    BOOLEAN WasInIcb = FALSE;
    uint64 OldLen;

//    ASSERT(FileInfo->RefCount >= 1);

    Dloc = FileInfo->Dloc;
    ASSERT(Dloc->FELoc.Mapping[0].extLocation);
    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, Dloc->FELoc.Mapping[0].extLocation);
    (*WrittenBytes) = 0;

    AdPrint(("UDFWriteFile__ FE %x, FileInfo %x, ExtInfo %x, Mapping %x\n",
        Dloc->FELoc.Mapping[0].extLocation, FileInfo, &(Dloc->DataLoc), Dloc->DataLoc.Mapping));

    t = Offset + Length;
//    UDFUpdateModifyTime(Vcb, FileInfo);
    if(t <= Dloc->DataLoc.Length) {
        // write Alloc-Rec area
        ExtPrint(("  WAlloc-Rec: %I64x <= %I64x\n", t, Dloc->DataLoc.Length));
        status = UDFWriteExtent(Vcb, &(Dloc->DataLoc), Offset, Length, Direct, Buffer, WrittenBytes);
        return status;
    }
    elen = UDFGetExtentLength(Dloc->DataLoc.Mapping);
    ExtPrint(("  DataLoc Offs %x, Len %I64x\n",
        Dloc->DataLoc.Offset, Dloc->DataLoc.Length));
    if(t <= (elen - Dloc->DataLoc.Offset)) {
        // write Alloc-Not-Rec area
        ExtPrint(("  WAlloc-Not-Rec: %I64x <= %I64x (%I64x - %I64x)\n",
            t, elen - Dloc->DataLoc.Offset - Dloc->DataLoc.Length,
            elen - Dloc->DataLoc.Offset,
            Dloc->DataLoc.Length));
        UDFSetFileSize(FileInfo, t);
        if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
            ExtPrint(("  w2k-compat -> rebuild allocs\n"));
            Dloc->DataLoc.Modified = TRUE;
        } else
        if((ULONG)((elen+Vcb->LBlockSize-1) >> Vcb->LBlockSizeBits) != (ULONG)((t+Vcb->LBlockSize-1) >> Vcb->LBlockSizeBits)) {
            ExtPrint(("  LBS boundary crossed -> rebuild allocs\n"));
            Dloc->DataLoc.Modified = TRUE;
        }
        Dloc->DataLoc.Length = t;
        return UDFWriteExtent(Vcb, &(Dloc->DataLoc), Offset, Length, Direct, Buffer, WrittenBytes);
    }
    // We should not get here if Direct=TRUE
    if(Direct) return STATUS_INVALID_PARAMETER;
    OldLen = Dloc->DataLoc.Length;
    if(Dloc->DataLoc.Offset && Dloc->DataLoc.Length) {
        // read in-icb data. it'll be replaced after resize
        ExtPrint(("  read in-icb data\n"));
        OldInIcb = (int8*)MyAllocatePool__(NonPagedPool, (uint32)(Dloc->DataLoc.Length));
        if(!OldInIcb) return STATUS_INSUFFICIENT_RESOURCES;
        status = UDFReadExtent(Vcb, &(Dloc->DataLoc), 0, (uint32)OldLen, FALSE, OldInIcb, &ReadBytes);
        if(!OS_SUCCESS(status)) {
            MyFreePool__(OldInIcb);
            return status;
        }
    }
    // init Alloc mode
    ExtPrint(("  init Alloc mode\n"));
    if((((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |= Vcb->DefaultAllocMode;
        WasInIcb = TRUE;
    }
    // increase extent
    ExtPrint(("  %s %s %s\n",
        UDFIsADirectory(FileInfo) ? "DIR" : "FILE",
        WasInIcb ? "In-Icb" : "",
        Vcb->LowFreeSpace ? "LowSpace" : ""));
    if(UDFIsADirectory(FileInfo) && !WasInIcb && !Vcb->LowFreeSpace) {
        FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_ALLOC_SEQUENTIAL;
        status = UDFResizeExtent(Vcb, PartNum, (t*2+Vcb->WriteBlockSize-1) & ~(Vcb->WriteBlockSize-1), FALSE, &(Dloc->DataLoc));
        if(OS_SUCCESS(status)) {
            AdPrint(("  preallocated space for Dir\n"));
            FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_PREALLOCATED;
            //UDFSetFileSize(FileInfo, t);
            Dloc->DataLoc.Length = t;
        } else
        if(status == STATUS_DISK_FULL) {
            status = UDFResizeExtent(Vcb, PartNum, t, FALSE, &(Dloc->DataLoc));
        }
    } else {
        status = UDFResizeExtent(Vcb, PartNum, t, FALSE, &(Dloc->DataLoc));
    }
    ExtPrint(("  DataLoc Offs %x, Len %I64x\n",
        Dloc->DataLoc.Offset, Dloc->DataLoc.Length));
    AdPrint(("UDFWriteFile__ (2) FileInfo %x, ExtInfo %x, Mapping %x\n", FileInfo, &(Dloc->DataLoc), Dloc->DataLoc.Mapping));
    if(!OS_SUCCESS(status)) {
        // rollback
        ExtPrint(("  err -> rollback\n"));
        if(WasInIcb) {
            // restore Alloc mode
            ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
            ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |= ICB_FLAG_AD_IN_ICB;
            if(Dloc->AllocLoc.Mapping) {
                MyFreePool__(Dloc->AllocLoc.Mapping);
                Dloc->AllocLoc.Mapping = NULL;
            }
        }
        if(OldInIcb) {
            MyFreePool__(OldInIcb);
            UDFWriteExtent(Vcb, &(Dloc->DataLoc), 0, (uint32)OldLen, FALSE, OldInIcb, &_WrittenBytes);
        }
        if((int64)OldLen != Dloc->DataLoc.Length) {
            // restore file size
            AdPrint(("  restore alloc\n"));
            FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
            UDFResizeExtent(Vcb, PartNum, OldLen, FALSE, &(Dloc->DataLoc));
            FileInfo->Dloc->DataLoc.Flags &= ~EXTENT_FLAG_CUT_PREALLOCATED;
        }
        return status;
    }
    if(OldInIcb) {
        // replace data from ICB (if any) & free buffer
        ExtPrint(("  write old in-icd data\n"));
        status = UDFWriteExtent(Vcb, &(Dloc->DataLoc), 0, (uint32)OldLen, FALSE, OldInIcb, &_WrittenBytes);
        MyFreePool__(OldInIcb);
        if(!OS_SUCCESS(status))
            return status;
    }
    // ufff... 
    // & now we'll write out data to well prepared extent... 
    // ... like all normal people do...
    ExtPrint(("  write user data\n"));
    if(!OS_SUCCESS(status = UDFWriteExtent(Vcb, &(Dloc->DataLoc), Offset, Length, FALSE, Buffer, WrittenBytes)))
        return status;
    UDFSetFileSize(FileInfo, t);
    Dloc->DataLoc.Modified = TRUE;
#ifdef UDF_DBG
    if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
        ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
    } else {
        ASSERT(((UDFGetFileSize(FileInfo)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)) ==
               ((UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFWriteFile__()

/*
    This routine marks file as deleted & decrements file link counter.
    It can optionaly free allocation space
 */
OSSTATUS
UDFUnlinkFile__(
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
    OSSTATUS status;
    BOOLEAN IsSDir;

    AdPrint(("UDFUnlinkFile__:\n"));
    if(!FileInfo) return STATUS_SUCCESS;

    ValidateFileInfo(FileInfo);

#ifndef _CONSOLE
    // now we can't call this if there is no OS-specific File Desc. present
    if(FileInfo->Fcb)
        UDFRemoveFileId__(Vcb, FileInfo);
#endif //_CONSOLE
    // check references
    Dloc = FileInfo->Dloc;
    if((FileInfo->OpenCount /*> (uint32)(UDFHasAStreamDir(FileInfo) ? 1 : 0)*/) ||
       (FileInfo->RefCount>1)) return STATUS_CANNOT_DELETE;
    if(Dloc->SDirInfo)
        return STATUS_CANNOT_DELETE;
    ASSERT(FileInfo->RefCount == 1);
    DirInfo = FileInfo->ParentFile;
    // root dir or self
    if(!DirInfo || ((FileInfo->Index < 2) && !UDFIsAStreamDir(FileInfo))) return STATUS_CANNOT_DELETE;
    hDirNdx = DirInfo->Dloc->DirIndex;
    Index = FileInfo->Index;
    // we can't delete modified file
    // it should be closed & reopened (or flushed) before deletion
    DirNdx = UDFDirIndex(hDirNdx,Index);
#if defined UDF_DBG || defined PRINT_ALWAYS
    if(DirNdx && DirNdx->FName.Buffer) {
        AdPrint(("Unlink: %ws\n",DirNdx->FName.Buffer));
    }
#endif // UDF_DBG
    if(FreeSpace &&
       ((Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
         Dloc->DataLoc.Modified ||
         Dloc->AllocLoc.Modified ||
         Dloc->FELoc.Modified ||
        (DirNdx && (DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) )) {
//        BrutePoint();
        return STATUS_CANNOT_DELETE;
    }
    SDirInfo = Dloc->SDirInfo;
/*    if(FreeSpace && SDirInfo) {
        KdPrint(("Unlink: SDirInfo should be NULL !!!\n"));
        BrutePoint();
        return STATUS_CANNOT_DELETE;
    }*/
    // stream directory can be deleted even being not empty
    // otherwise we should perform some checks
    if(!(IsSDir = UDFIsAStreamDir(FileInfo))) {
        // check if not empty direcory
        if((DirNdx->FileCharacteristics & FILE_DIRECTORY) &&
           (hCurDirNdx = Dloc->DirIndex) &&
            FreeSpace) {
            if(!UDFIsDirEmpty(hCurDirNdx))
                return STATUS_DIRECTORY_NOT_EMPTY;
        }
        DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
        DirNdx->FileCharacteristics |= FILE_DELETED;
        FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
        hDirNdx->DelCount++;
        UDFChangeFileCounter(Vcb, !UDFIsADirectory(FileInfo), FALSE);
    }
    UDFDecFileLinkCount(FileInfo); // decrease
    lc = UDFGetFileLinkCount(FileInfo);
    if(DirNdx && FreeSpace) {
        // FileIdent marked as 'deleted' should have an empty ICB
        // We shall do it only if object has parent Dir
        // (for ex. SDir has parent object, but has no parent Dir)
        DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
        // Root Files (Root/SDir/Vat/etc.) has no FileIdent...
        if(FileInfo->FileIdent)
            RtlZeroMemory(&(FileInfo->FileIdent->icb), sizeof(long_ad));
    }
    // caller wishes to free allocation, but we can't do it due to
    // alive links. In this case we should just remove reference
    if(FreeSpace && lc) {
        ((icbtag*)(Dloc->FileEntry+1))->parentICBLocation.logicalBlockNum = 0;
        ((icbtag*)(Dloc->FileEntry+1))->parentICBLocation.partitionReferenceNum = 0;
        Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    } else
    // if caller wishes to free file allocation &
    // there are no more references(links) to this file, lets do it >;->
    if(FreeSpace && !lc) {
        if(UDFHasAStreamDir(FileInfo) &&
           !UDFIsSDirDeleted(Dloc->SDirInfo) ) {
            // we have a Stream Dir associated...
            PUDF_FILE_INFO SFileInfo;
            // ... try to open it
            if(Dloc->SDirInfo) {
                UDFFlushFile__(Vcb, FileInfo);
                return STATUS_CANNOT_DELETE;
            }
            // open SDir
            status = UDFOpenStreamDir__(Vcb, FileInfo, &(Dloc->SDirInfo));
            if(!OS_SUCCESS(status)) {
                // abort Unlink on error
                SFileInfo = Dloc->SDirInfo;
cleanup_SDir:
                UDFCleanUpFile__(Vcb, SFileInfo);
                if(SFileInfo) MyFreePool__(SFileInfo);
                UDFFlushFile__(Vcb, FileInfo);
                return status;
            }
            SDirInfo = Dloc->SDirInfo;
            // try to perform deltree for Streams
            status = UDFUnlinkAllFilesInDir(Vcb, SDirInfo);
            if(!OS_SUCCESS(status)) {
                // abort Unlink on error
                UDFCloseFile__(Vcb, SDirInfo);
                SFileInfo = SDirInfo;
                BrutePoint();
                goto cleanup_SDir;
            }
            // delete SDir
            UDFFlushFile__(Vcb, SDirInfo);
            AdPrint(("  "));
            UDFUnlinkFile__(Vcb, SDirInfo, TRUE);
            // close SDir
            UDFCloseFile__(Vcb, SDirInfo);
            if(UDFCleanUpFile__(Vcb, SDirInfo)) {
                MyFreePool__(SDirInfo);
#ifdef UDF_DBG
            } else {
                BrutePoint();
#endif // UDF_DBG
            }
            // update FileInfo
            ASSERT(Dloc->FileEntry);
            RtlZeroMemory( &(((PEXTENDED_FILE_ENTRY)(Dloc->FileEntry))->streamDirectoryICB), sizeof(long_ad));
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        } else
        if(IsSDir) {
            // do deltree for Streams
            status = UDFUnlinkAllFilesInDir(Vcb, FileInfo);
            if(!OS_SUCCESS(status)) {
                UDFFlushFile__(Vcb, FileInfo);
                return status;
            }
            // update parent FileInfo
            ASSERT(FileInfo->ParentFile->Dloc->FileEntry);
            RtlZeroMemory( &(((PEXTENDED_FILE_ENTRY)(FileInfo->ParentFile->Dloc->FileEntry))->streamDirectoryICB), sizeof(long_ad));
            FileInfo->ParentFile->Dloc->FE_Flags &= ~UDF_FE_FLAG_HAS_SDIR;
            FileInfo->ParentFile->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED |
                                                     UDF_FE_FLAG_HAS_DEL_SDIR);
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_IS_DEL_SDIR;
            UDFDecFileLinkCount(FileInfo->ParentFile);
        }
        if(Dloc->DirIndex) {
            UDFFlushFESpace(Vcb, Dloc, FLUSH_FE_FOR_DEL);
        }
        // flush file
        UDFFlushFile__(Vcb, FileInfo);
        UDFUnlinkDloc(Vcb, Dloc);
        // free allocation
        UDFFreeFileAllocation(Vcb, DirInfo, FileInfo);
        ASSERT(!(FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED));
        FileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
    }
    return STATUS_SUCCESS;
} // end UDFUnlinkFile__()

OSSTATUS
UDFUnlinkAllFilesInDir(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo
    )
{
    PDIR_INDEX_HDR hCurDirNdx;
    PDIR_INDEX_ITEM CurDirNdx;
    PUDF_FILE_INFO FileInfo;
    OSSTATUS status;
    uint_di i;

    hCurDirNdx = DirInfo->Dloc->DirIndex;
    // check if we can delete all files
    for(i=2; CurDirNdx = UDFDirIndex(hCurDirNdx,i); i++) {
        // try to open Stream
        if(CurDirNdx->FileInfo)
            return STATUS_CANNOT_DELETE;
    }
    // start deletion
    for(i=2; CurDirNdx = UDFDirIndex(hCurDirNdx,i); i++) {
        // try to open Stream
        status = UDFOpenFile__(Vcb, FALSE, TRUE, NULL, DirInfo, &FileInfo, &i);
        if(status == STATUS_FILE_DELETED) {
            // we should not release on-disk allocation for
            // deleted streams twice
            if(CurDirNdx->FileInfo) {
                BrutePoint();
                goto err_del_stream;
            }
            goto skip_del_stream;
        } else
        if(!OS_SUCCESS(status)) {
            // Error :(((
err_del_stream:
            UDFCleanUpFile__(Vcb, FileInfo);
            if(FileInfo)
                MyFreePool__(FileInfo);
            return status;
        }

        UDFFlushFile__(Vcb, FileInfo);
        AdPrint(("    "));
        UDFUnlinkFile__(Vcb, FileInfo, TRUE);
        UDFCloseFile__(Vcb, FileInfo);
skip_del_stream:
        if(FileInfo && UDFCleanUpFile__(Vcb, FileInfo)) {
            MyFreePool__(FileInfo);
        }
    }
    return STATUS_SUCCESS;
} // end UDFUnlinkAllFilesInDir()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine inits UDF_FILE_INFO structure for specified file
    If it returns status != STATUS_SUCCESS caller should call UDFCleanUpFile__
    for returned pointer *WITHOUT* using UDFCloseFile__
 */
OSSTATUS
UDFOpenFile__(
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
    OSSTATUS status;
    uint_di i=0;
    EXTENT_AD FEExt;
    uint16 Ident;
    PDIR_INDEX_HDR hDirNdx = DirInfo->Dloc->DirIndex;
    PDIR_INDEX_ITEM DirNdx;
    PUDF_FILE_INFO FileInfo;
    PUDF_FILE_INFO ParFileInfo;
    uint32 ReadBytes;
    *_FileInfo = NULL;
    if(!hDirNdx) return STATUS_NOT_A_DIRECTORY;

    // find specified file in directory index
    // if it is already known, skip this foolish code
    if(IndexToOpen) {
        i=*IndexToOpen;
    } else
        if(!OS_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, NotDeleted, fn, DirInfo, &i)))
            return status;
    // do this check for OpenByIndex
    // some routines may send invalid Index
    if(!(DirNdx = UDFDirIndex(hDirNdx,i)))
        return STATUS_OBJECT_NAME_NOT_FOUND;
    if(FileInfo = DirNdx->FileInfo) {
        // file is already opened.
        if((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
            AdPrint(("  FILE_DELETED on open\n"));
            return STATUS_FILE_DELETED;
        }
        if((FileInfo->ParentFile != DirInfo) &&
           (FileInfo->Index >= 2)) {
            ParFileInfo = UDFLocateParallelFI(DirInfo, i, FileInfo);
            BrutePoint();
            if(ParFileInfo->ParentFile != DirInfo) {
                FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
                *_FileInfo = FileInfo;
                if(!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
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
    if(IndexToOpen) {
        if((DirNdx->FileCharacteristics & FILE_DELETED) && NotDeleted) {
            AdPrint(("  FILE_DELETED on open (2)\n"));
            return STATUS_FILE_DELETED;
        }
    }
    FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
    *_FileInfo = FileInfo;
    if(!FileInfo) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // read FileIdent
    FileInfo->FileIdent = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, DirNdx->Length, MEM_FID_TAG);
    if(!(FileInfo->FileIdent)) return STATUS_INSUFFICIENT_RESOURCES;
    FileInfo->FileIdentLen = DirNdx->Length;
    if(!OS_SUCCESS(status = UDFReadExtent(Vcb, &(DirInfo->Dloc->DataLoc), DirNdx->Offset,
                             DirNdx->Length, FALSE, (int8*)(FileInfo->FileIdent), &ReadBytes) ))
        return status;
    if(FileInfo->FileIdent->descTag.tagIdent != TID_FILE_IDENT_DESC) {
        BrutePoint();
        return STATUS_FILE_CORRUPT_ERROR;
    }
    // check for opened links
    if(!OS_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation)))))
        return status;
    // init pointer to parent object
    FileInfo->Index = i;
    FileInfo->ParentFile = DirInfo;
    // init pointers to linked files (if any)
    if(FileInfo->Dloc->LinkedFileInfo != FileInfo)
        UDFInsertLinkedFile(FileInfo, FileInfo->Dloc->LinkedFileInfo);
    if(FileInfo->Dloc->FileEntry)
        goto init_tree_entry;
    // read (Ex)FileEntry
    FileInfo->Dloc->FileEntry = (tag*)MyAllocatePoolTag__(NonPagedPool, Vcb->LBlockSize, MEM_FE_TAG);
    if(!(FileInfo->Dloc->FileEntry)) return STATUS_INSUFFICIENT_RESOURCES;
    if(!OS_SUCCESS(status = UDFReadFileEntry(Vcb, &(FileInfo->FileIdent->icb), (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &Ident)))
        return status;
    // build mappings for Data & AllocDescs
    if(!FileInfo->Dloc->AllocLoc.Mapping) {
        FEExt.extLength = FileInfo->FileIdent->icb.extLength;
        FEExt.extLocation = UDFPartLbaToPhys(Vcb, &(FileInfo->FileIdent->icb.extLocation) );
        if(FEExt.extLocation == LBA_OUT_OF_EXTENT)
            return STATUS_FILE_CORRUPT_ERROR;
        FileInfo->Dloc->AllocLoc.Mapping = UDFExtentToMapping(&FEExt);
        if(!(FileInfo->Dloc->AllocLoc.Mapping))
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    // read location info
    status = UDFLoadExtInfo(Vcb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &(FileInfo->FileIdent->icb),
                           &(FileInfo->Dloc->DataLoc), &(FileInfo->Dloc->AllocLoc) );
    if(!OS_SUCCESS(status))
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
    if(UDFGetFileLinkCount(FileInfo) > 1) {
        DirNdx->FI_Flags |= UDF_FI_FLAG_LINKED;
    } else {
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_LINKED;
    }
    // resize FE cache (0x800 instead of 0x40 is not a good idea)
    if(!MyReallocPool__((int8*)((FileInfo->Dloc->FileEntry)), Vcb->LBlockSize,
                     (int8**)&((FileInfo->Dloc->FileEntry)), FileInfo->Dloc->FileEntryLen))
        return STATUS_INSUFFICIENT_RESOURCES;
    // check if this file has a SDir
    if((FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) &&
       ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength )
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    if(!(FileInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY)) {
        UDFReferenceFile__(FileInfo);
        ASSERT(FileInfo->ParentFile == DirInfo);
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        return STATUS_SUCCESS;
    }

    UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used

    // build index for directories
    if(!FileInfo->Dloc->DirIndex) {
        status = UDFIndexDirectory(Vcb, FileInfo);
        if(!OS_SUCCESS(status))
            return status;
#ifndef UDF_READ_ONLY_BUILD
        if((FileInfo->Dloc->DirIndex->DelCount > Vcb->PackDirThreshold) &&
           !(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)) {
            status = UDFPackDirectory__(Vcb, FileInfo);
            if(!OS_SUCCESS(status))
                return status;
        }
#endif //UDF_READ_ONLY_BUILD
    }
    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);
    ASSERT(FileInfo->ParentFile == DirInfo);

    return status;
} // end UDFOpenFile__()


/*
    This routine inits UDF_FILE_INFO structure for root directory
 */
OSSTATUS
UDFOpenRootFile__(
    IN PVCB Vcb,
    IN lb_addr* RootLoc,
    OUT PUDF_FILE_INFO FileInfo
    )
{
    uint32 RootLBA;
    OSSTATUS status;
//    uint32 PartNum = RootLoc->partitionReferenceNum;
    uint32 LBS = Vcb->LBlockSize;
    uint16 Ident;
    LONG_AD FELoc;
    EXTENT_AD FEExt;
    uint8 FileType;

    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    RootLBA = UDFPartLbaToPhys(Vcb,RootLoc);
    if(RootLBA == LBA_OUT_OF_EXTENT)
        return STATUS_FILE_CORRUPT_ERROR;
    FELoc.extLocation = *RootLoc;
    FELoc.extLength = LBS;
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // check for opened links
    if(!OS_SUCCESS(status = UDFStoreDloc(Vcb, FileInfo, RootLBA)))
        return status;
    if(FileInfo->Dloc->FileEntry)
        goto init_tree_entry;
    // read (Ex)FileEntry
    FileInfo->Dloc->FileEntry = (tag*)MyAllocatePoolTag__(NonPagedPool, LBS, MEM_FE_TAG);
    if(!(FileInfo->Dloc->FileEntry)) return STATUS_INSUFFICIENT_RESOURCES;

    if(!OS_SUCCESS(status = UDFReadFileEntry(Vcb, &FELoc, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &Ident)))
        return status;
    // build mappings for Data & AllocDescs
    FEExt.extLength = LBS;
    FEExt.extLocation = UDFPartLbaToPhys(Vcb, &(FELoc.extLocation) );
    if(FEExt.extLocation == LBA_OUT_OF_EXTENT)
        return STATUS_FILE_CORRUPT_ERROR;
    FileInfo->Dloc->FELoc.Mapping = UDFExtentToMapping(&FEExt);
    if(!(FileInfo->Dloc->FELoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    // build mappings for AllocDescs
    if(!FileInfo->Dloc->AllocLoc.Mapping) {
        FileInfo->Dloc->AllocLoc.Mapping = UDFExtentToMapping(&FEExt);
        if(!(FileInfo->Dloc->AllocLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    }
    if(!OS_SUCCESS(status = UDFLoadExtInfo(Vcb, (PFILE_ENTRY)(FileInfo->Dloc->FileEntry), &FELoc, 
                       &(FileInfo->Dloc->DataLoc), &(FileInfo->Dloc->AllocLoc) ) ))
        return status;
    FileInfo->Dloc->FileEntryLen = (uint32)
    (FileInfo->Dloc->FELoc.Length = (FileInfo->Dloc->DataLoc.Offset) ? FileInfo->Dloc->DataLoc.Offset :
                                                          FileInfo->Dloc->AllocLoc.Offset);
init_tree_entry:
    // resize FE cache (0x800 instead of 0x40 is not a good idea)
    if(!MyReallocPool__((int8*)((FileInfo->Dloc->FileEntry)), LBS,
                     (int8**)&((FileInfo->Dloc->FileEntry)), FileInfo->Dloc->FileEntryLen))
        return STATUS_INSUFFICIENT_RESOURCES;
    // init DirIndex
    if( (FileType = ((icbtag*)((FileInfo->Dloc->FileEntry)+1))->fileType) != UDF_FILE_TYPE_DIRECTORY &&
        (FileType != UDF_FILE_TYPE_STREAMDIR) ) {
        UDFReferenceFile__(FileInfo);
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        return STATUS_SUCCESS;
    }
    // build index for directories
    if(!FileInfo->Dloc->DirIndex) {
        status = UDFIndexDirectory(Vcb, FileInfo);
        if(!OS_SUCCESS(status))
            return status;
#ifndef UDF_READ_ONLY_BUILD
        if((FileInfo->Dloc->DirIndex->DelCount > Vcb->PackDirThreshold) &&
           !(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)) {
            status = UDFPackDirectory__(Vcb, FileInfo);
            if(!OS_SUCCESS(status))
                return status;
        }
#endif //UDF_READ_ONLY_BUILD
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
    PDIR_INDEX_ITEM DirNdx, DirNdx2;
    BOOLEAN Parallel = FALSE;
    BOOLEAN Linked = FALSE;
#ifdef UDF_DBG
    BOOLEAN Modified = FALSE;
    PDIR_INDEX_HDR hDirNdx;
    uint_di Index;
    PUDF_FILE_INFO DirInfo;
#endif // UDF_DBG

    if(!FileInfo) return UDF_FREE_FILEINFO;

    ValidateFileInfo(FileInfo);

    if(FileInfo->OpenCount || FileInfo->RefCount) {
        KdPrint(("UDF: not all references are closed\n"));
        KdPrint(("     Skipping cleanup\n"));
        KdPrint(("UDF: OpenCount = %x, RefCount = %x, LinkRefCount = %x\n",
                              FileInfo->OpenCount,FileInfo->RefCount,FileInfo->Dloc->LinkRefCount));
        return UDF_FREE_NOTHING;
    }
    if(FileInfo->Fcb) {
        KdPrint(("Operating System still has references to this file\n"));
        KdPrint(("     Skipping cleanup\n"));
//        BrutePoint();
        return UDF_FREE_NOTHING;
    }

    IsASDir = UDFIsAStreamDir(FileInfo);

    if(Dloc = FileInfo->Dloc) {

#ifdef UDF_DBG
        DirInfo = FileInfo->ParentFile;
        if(DirInfo) {
            hDirNdx = DirInfo->Dloc->DirIndex;
            Index = FileInfo->Index;
            // we can't delete modified file
            // it should be closed & reopened (or flushed) before deletion
            DirNdx = UDFDirIndex(hDirNdx,Index);
            KdPrint(("Cleanup Mod: %s%s%s%s%s%s\n",
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

        if(Dloc->DirIndex) {
            uint_di i;
            for(i=2; DirNdx = UDFDirIndex(Dloc->DirIndex,i); i++) {
                if(DirNdx->FileInfo) {
                    if(!KeepDloc) {
                        BrutePoint();
                        KdPrint(("UDF: Found not cleaned up reference.\n"));
                        KdPrint(("     Skipping cleanup (1)\n"));
//                        BrutePoint();
                        return UDF_FREE_NOTHING;
                    }
                    // The file being cleaned up may have not closed Dirs
                    // (linked Dir). In this case each of them may have
                    // reference to FileInfo in DirIndex[1]
                    // Here we'll check it and change for valid value if
                    // necessary (Update Child Objects - I)
                    if(DirNdx->FileInfo->Dloc) {
                        // we can get here only when (Parallel == TRUE)
                        DirNdx2 = UDFDirIndex(DirNdx->FileInfo->Dloc->DirIndex, 1);
                        // It is enough to check DirNdx2->FileInfo only.
                        // If one of Parallel FI's has reference (and equal)
                        // to the FI being removed, it'll be removed from
                        // the chain & nothing wrong will happen.
                        if(DirNdx2 && (DirNdx2->FileInfo == FileInfo)) {
                            if(FileInfo->PrevLinkedFile == FileInfo) {
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
        if(Dloc->SDirInfo) {
            KdPrint(("UDF: Found not cleaned up reference (SDir).\n"));

            // (Update Child Objects - II)
            if(Dloc->SDirInfo->ParentFile == FileInfo) {
                BrutePoint();
                ASSERT(ParFileInfo);
                Dloc->SDirInfo->ParentFile = ParFileInfo;
            }
            // We should break Cleanup process if alive reference detected
            // and there is no possibility to store pointer in some other
            // place (in parallel object)
            if(!KeepDloc) {
                BrutePoint();
                KdPrint(("     Skipping cleanup\n"));
                return UDF_FREE_NOTHING;
            }

            if(!UDFIsSDirDeleted(Dloc->SDirInfo) &&
               Dloc->SDirInfo->Dloc) {
                DirNdx2 = UDFDirIndex(Dloc->SDirInfo->Dloc->DirIndex, 1);
                if(DirNdx2 && (DirNdx2->FileInfo == FileInfo)) {
                    DirNdx2->FileInfo = 
                        Parallel ? ParFileInfo : NULL;
                    ASSERT(!DirNdx2->FileInfo->RefCount);
                }
            }
        }

        if(!KeepDloc) {

            ASSERT(!Modified);

#ifndef UDF_TRACK_ONDISK_ALLOCATION
            if(Dloc->DataLoc.Mapping)  MyFreePool__(Dloc->DataLoc.Mapping);
            if(Dloc->AllocLoc.Mapping) MyFreePool__(Dloc->AllocLoc.Mapping);
            if(Dloc->FELoc.Mapping)    MyFreePool__(Dloc->FELoc.Mapping);
            if(Dloc->FileEntry) {
                // plain file
                lc = UDFGetFileLinkCount(FileInfo);
                MyFreePool__(Dloc->FileEntry);
                Dloc->FileEntry = NULL;
            } else if(FileInfo->Index >= 2) {
                // error durring open operation
                lc = UDF_INVALID_LINK_COUNT;
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION
            if(FileInfo->Dloc->DirIndex) {
                uint_di i;
                for(i=2; DirNdx = UDFDirIndex(Dloc->DirIndex,i); i++) {
                    ASSERT(!DirNdx->FileInfo);
                    if(DirNdx->FName.Buffer)
                        MyFreePool__(DirNdx->FName.Buffer);
                }
                // The only place where we can free FE_Charge extent is here
                UDFFlushFESpace(Vcb, Dloc);
                UDFDirIndexFree(Dloc->DirIndex);
                Dloc->DirIndex = NULL;
#ifdef UDF_TRACK_ONDISK_ALLOCATION
                UDFIndexDirectory(Vcb, FileInfo);
                if(FileInfo->Dloc->DirIndex) {
                    for(i=2; DirNdx = UDFDirIndex(Dloc->DirIndex,i); i++) {
                        ASSERT(!DirNdx->FileInfo);
                        if(DirNdx->FName.Buffer)
                            MyFreePool__(DirNdx->FName.Buffer);
                    }
                    UDFDirIndexFree(Dloc->DirIndex);
                    Dloc->DirIndex = NULL;
                }
#endif //UDF_TRACK_ONDISK_ALLOCATION
            }

#ifdef UDF_TRACK_ONDISK_ALLOCATION
            if(Dloc->AllocLoc.Mapping) MyFreePool__(Dloc->AllocLoc.Mapping);
            if(Dloc->FELoc.Mapping)    MyFreePool__(Dloc->FELoc.Mapping);
            if(Dloc->FileEntry) {
                // plain file
                lc = UDFGetFileLinkCount(FileInfo);
                MyFreePool__(Dloc->FileEntry);
                Dloc->FileEntry = NULL;
            } else if(FileInfo->Index >= 2) {
                // error durring open operation
                lc = UDF_INVALID_LINK_COUNT;
            }
            if(Dloc->DataLoc.Mapping) {
                if(lc && (lc != UDF_INVALID_LINK_COUNT)) {
                    UDFCheckSpaceAllocation(Vcb, 0, Dloc->DataLoc.Mapping, AS_USED); // check if used
                } else {
                    UDFCheckSpaceAllocation(Vcb, 0, Dloc->DataLoc.Mapping, AS_FREE); // check if free
                }
                MyFreePool__(Dloc->DataLoc.Mapping);
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION

            if(lc && (lc != UDF_INVALID_LINK_COUNT)) {
                UDFRemoveDloc(Vcb, Dloc);
            } else {
                UDFFreeDloc(Vcb, Dloc);
            }
        } else // KeepDloc cannot be FALSE if (Linked == TRUE)
        if(Linked) {
//            BrutePoint();
            // Update pointers in ParentObject (if any)
            if(FileInfo->ParentFile->Dloc->SDirInfo == FileInfo)
                FileInfo->ParentFile->Dloc->SDirInfo = FileInfo->PrevLinkedFile;
            DirNdx = UDFDirIndex(FileInfo->Dloc->DirIndex, 0);
            if(DirNdx && (DirNdx->FileInfo == FileInfo))
                DirNdx->FileInfo = FileInfo->PrevLinkedFile;
            DirNdx = UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex, FileInfo->Index);
            if(DirNdx && (DirNdx->FileInfo == FileInfo))
                DirNdx->FileInfo = ParFileInfo;
            // remove from linked chain
            FileInfo->NextLinkedFile->PrevLinkedFile = FileInfo->PrevLinkedFile;
            FileInfo->PrevLinkedFile->NextLinkedFile = FileInfo->NextLinkedFile;
            // update pointer in Dloc
            if(FileInfo->Dloc->LinkedFileInfo == FileInfo)
                FileInfo->Dloc->LinkedFileInfo = FileInfo->PrevLinkedFile;
        }
        FileInfo->Dloc = NULL;
    } else {
        KeepDloc = FALSE;
    }

    // Cleanup pointers in ParentObject (if any)
    if(IsASDir) {
        if(FileInfo->ParentFile->Dloc->SDirInfo == FileInfo) {
            ASSERT(!Linked);
            FileInfo->ParentFile->Dloc->SDirInfo = NULL;
            FileInfo->ParentFile->Dloc->FE_Flags &= ~UDF_FE_FLAG_HAS_DEL_SDIR;
        }
    } else
    if(FileInfo->ParentFile) {
        ASSERT(FileInfo->ParentFile->Dloc);
        DirNdx = UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex, FileInfo->Index);
        ASSERT(DirNdx);
#ifdef UDF_DBG
        PUDF_FILE_INFO OldFI;
        if(Parallel) {
            ASSERT(!DirNdx || !(OldFI = DirNdx->FileInfo) ||
                    !(OldFI == FileInfo));
        } else {
            ASSERT(!DirNdx || !(OldFI = DirNdx->FileInfo) ||
                     (OldFI == FileInfo));
        }
#endif
        if( DirNdx && (DirNdx->FileInfo == FileInfo) ) {
            if(!Parallel)
                DirNdx->FileInfo = NULL;
#ifdef UDF_DBG
        } else {
            // We can get here after incomplete Open
            if(!Parallel && DirNdx->FileInfo)
                BrutePoint();
#endif
        }
#ifdef UDF_DBG
    } else {
//        BrutePoint();
#endif
    }

    if(!Parallel && FileInfo->FileIdent)
        MyFreePool__(FileInfo->FileIdent);
    FileInfo->FileIdent = NULL;
    // Kill reference to parent object
    FileInfo->ParentFile = NULL;
    // Kill references to parallel object(s) since it has no reference to
    // this one now
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    if(FileInfo->ListPtr)
        FileInfo->ListPtr->FileInfo = NULL;;
    return KeepDloc ? UDF_FREE_FILEINFO : (UDF_FREE_FILEINFO | UDF_FREE_DLOC);
} // end UDFCleanUpFile__()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine creates FileIdent record in destination directory &
    allocates FileEntry with in-ICB zero-sized data
    If it returns status != STATUS_SUCCESS caller should call UDFCleanUpFile__
    for returned pointer *WITHOUT* using UDFCloseFile__
 */
OSSTATUS
UDFCreateFile__(
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
    OSSTATUS status;
    LONG_AD FEicb;
    UDF_DIR_SCAN_CONTEXT ScanContext;
    PDIR_INDEX_HDR hDirNdx = DirInfo->Dloc->DirIndex;
    PDIR_INDEX_ITEM DirNdx;
    uint32 LBS = Vcb->LBlockSize;
    PUDF_FILE_INFO FileInfo;
    *_FileInfo = NULL;
    BOOLEAN undel = FALSE;
    uint32 ReadBytes;
//    BOOLEAN PackDir = FALSE;
    BOOLEAN FEAllocated = FALSE;

    ValidateFileInfo(DirInfo);
    *_FileInfo = NULL;

    ASSERT(DirInfo->Dloc->FELoc.Mapping[0].extLocation);
    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
    if(!hDirNdx) return STATUS_NOT_A_DIRECTORY;
    i = 0;

    _SEH2_TRY {

        // check if exists
        status = UDFFindFile(Vcb, IgnoreCase, FALSE, _fn, DirInfo, &i);
        DirNdx = UDFDirIndex(hDirNdx,i);
        if(OS_SUCCESS(status)) {
            // file is a Cur(Parent)Dir
            if(i<2) try_return (status = STATUS_ACCESS_DENIED);
            // file deleted
            if(UDFIsDeleted(DirNdx)) {
                j=0;
                if(OS_SUCCESS(UDFFindFile(Vcb, IgnoreCase, TRUE, _fn, DirInfo, &j))) {
                   i=j;
                   DirNdx = UDFDirIndex(hDirNdx,i);
                   goto CreateBothFound;
                }
                // we needn't allocating new FileIdent inside Dir stream
                // perform 'undel'
                if(DirNdx->FileInfo) {
    //                BrutePoint();
                    status = UDFPretendFileDeleted__(Vcb, DirNdx->FileInfo);
                    if(!OS_SUCCESS(status))
                        try_return (status = STATUS_FILE_DELETED);
                } else {
                    undel = TRUE;
                }
    //            BrutePoint();
                goto CreateUndel;
            }
CreateBothFound:
            // file already exists
            if(CreateNew) try_return (status = STATUS_ACCESS_DENIED);
            // try to open it
            BrutePoint();
            status = UDFOpenFile__(Vcb, IgnoreCase, TRUE, _fn, DirInfo, _FileInfo,&i);
    //        *_FileInfo = FileInfo; // OpenFile__ has already done it, so update it...
            DirNdx = UDFDirIndex(hDirNdx,i);
            DirNdx->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
            FileInfo = *_FileInfo;
            if(!OS_SUCCESS(status)) {
                // :(( can't open....
                if(FileInfo && UDFCleanUpFile__(Vcb, FileInfo)) {
                    MyFreePool__(FileInfo);
                    *_FileInfo = NULL;
                }
                BrutePoint();
                try_return (status);
            }
            // check if we can delete this file
            if(FileInfo->OpenCount || (FileInfo->RefCount > 1)) {
                BrutePoint();
                UDFCloseFile__(Vcb, FileInfo);
                try_return (status = STATUS_CANNOT_DELETE);
            }
            BrutePoint();
            // remove DIRECTORY flag
            DirNdx->FileCharacteristics &= ~FILE_DIRECTORY;
            FileInfo->FileIdent->fileCharacteristics &= ~FILE_DIRECTORY;
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            // truncate file size to ZERO
            status = UDFResizeFile__(Vcb, FileInfo, 0);
            if(!OS_SUCCESS(status)) {
                BrutePoint();
                UDFCloseFile__(Vcb, FileInfo);
            }
            // set NORMAL flag
            FileInfo->FileIdent->fileCharacteristics = 0;
            DirNdx->FileCharacteristics = 0;
            // update DeletedFiles counter in Directory... (for PackDir)
            if(undel && OS_SUCCESS(status))
                hDirNdx->DelCount--;
            try_return (status);
        }

CreateUndel:

        // allocate FileInfo
        FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FE_TAG);
        *_FileInfo = FileInfo;
        if(!FileInfo)
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        ImpUseLen = (ImpUseLen + 3) & ~((uint16)3);

        RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
        // init horizontal links
        FileInfo->NextLinkedFile =
        FileInfo->PrevLinkedFile = FileInfo;
        // allocate space for FileEntry
        if(!OS_SUCCESS(status =
            UDFBuildFileEntry(Vcb, DirInfo, FileInfo, PartNum, ICB_FLAG_AD_IN_ICB, ExtAttrSz, Extended) )) {
            BrutePoint();
            try_return (status);
        }
        FEAllocated = TRUE;
        FEicb.extLength = LBS;
        ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
        RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));

        if(!undel) {
            // build FileIdent
            if(!OS_SUCCESS(status =
                UDFBuildFileIdent(Vcb, _fn, &FEicb, ImpUseLen,
                        &(FileInfo->FileIdent), &(FileInfo->FileIdentLen)) ))
                try_return (status);
        } else {
            // read FileIdent
            FileInfo->FileIdent = (PFILE_IDENT_DESC)MyAllocatePoolTag__(NonPagedPool, DirNdx->Length, MEM_FID_TAG);
            if(!(FileInfo->FileIdent)) try_return (status = STATUS_INSUFFICIENT_RESOURCES);
            FileInfo->FileIdentLen = DirNdx->Length;
            if(!OS_SUCCESS(status = UDFReadExtent(Vcb, &(DirInfo->Dloc->DataLoc), DirNdx->Offset,
                                     DirNdx->Length, FALSE, (int8*)(FileInfo->FileIdent), &ReadBytes) ))
                try_return (status);
            FileInfo->FileIdent->fileCharacteristics = 0;
            FileInfo->FileIdent->icb = FEicb;
            ImpUseLen = FileInfo->FileIdent->lengthOfImpUse;
            DirNdx->FileCharacteristics = 0;
        }
        // init 'parentICBLocation' & so on in FE
        ((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.logicalBlockNum =
             UDFPhysLbaToPart(Vcb, PartNum, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
        ((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.partitionReferenceNum = (uint16)PartNum;
    //    ((icbtag*)(FileInfo->Dloc->FileEntry+1))->strategyType = 4;
    //    ((icbtag*)(FileInfo->Dloc->FileEntry+1))->numEntries = 1;
        // try to find suitable unused FileIdent in DirIndex
        l = FileInfo->FileIdentLen;
        if(undel) goto CrF__2;
#ifndef UDF_LIMIT_DIR_SIZE
        if(Vcb->CDR_Mode) {
#endif // UDF_LIMIT_DIR_SIZE
            // search for suitable unused entry
            if(UDFDirIndexInitScan(DirInfo, &ScanContext, 2)) {
                while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {
                    if((DirNdx->Length == l) && UDFIsDeleted(DirNdx) &&
                       !DirNdx->FileInfo ) {
                        // free unicode-buffer with old name
                        if(DirNdx->FName.Buffer) {
                            MyFreePool__(DirNdx->FName.Buffer);
                            DirNdx->FName.Buffer = NULL;
                        }
                        i = ScanContext.i;
                        goto CrF__1;
                    }
                }
            }
#ifndef UDF_LIMIT_DIR_SIZE
        } else {
#endif // UDF_LIMIT_DIR_SIZE
            i = UDFDirIndexGetLastIndex(hDirNdx); // 'i' points beyond EO DirIndex
#ifndef UDF_LIMIT_DIR_SIZE
        }
#endif // UDF_LIMIT_DIR_SIZE

        // append entry
        if(!OS_SUCCESS(status = UDFDirIndexGrow(&(DirInfo->Dloc->DirIndex), 1))) {
            try_return (status);
        }

        // init offset of new FileIdent in directory Data extent
        hDirNdx = DirInfo->Dloc->DirIndex;
        if(i-1) {
            DirNdx = UDFDirIndex(hDirNdx,i-1);
            UDFDirIndex(hDirNdx,i)->Offset = DirNdx->Offset + DirNdx->Length;
            DirNdx = UDFDirIndex(hDirNdx,i);
        } else {
            DirNdx = UDFDirIndex(hDirNdx,i);
            DirNdx->Offset = 0;
        }
        // new terminator is recorded by UDFDirIndexGrow()
        if( ((d = (LBS - (DirNdx->Offset + l + DirInfo->Dloc->DataLoc.Offset) & (LBS-1) )) < sizeof(FILE_IDENT_DESC)) &&
              d ) {
            // insufficient space at the end of last sector for
            // next FileIdent's tag. fill it with ImpUse data

            // generally, all data should be DWORD-aligned, but if it is not so
            // this opearation will help us to avoid glitches
            d = (d+3) & ~((uint32)3);

            uint32 IUl, FIl;
            if(!MyReallocPool__((int8*)(FileInfo->FileIdent), l,
                         (int8**)&(FileInfo->FileIdent), (l+d+3) & ~((uint32)(3)) ))
                try_return (status = STATUS_INSUFFICIENT_RESOURCES);
            l += d;
            IUl = FileInfo->FileIdent->lengthOfImpUse;
            FIl = FileInfo->FileIdent->lengthFileIdent;
            // move filename to higher addr
            RtlMoveMemory(((int8*)(FileInfo->FileIdent+1))+IUl+d,
                          ((int8*)(FileInfo->FileIdent+1))+IUl, FIl);
            RtlZeroMemory(((int8*)(FileInfo->FileIdent+1))+IUl, d);
            FileInfo->FileIdent->lengthOfImpUse += (uint16)d;
            FileInfo->FileIdentLen = l;
        }
        DirNdx->Length = l;
CrF__1:
        // clone unicode string
        // it  **<<MUST>>**  be allocated with internal memory manager
        DirNdx->FName.Buffer = (PWCHAR)MyAllocatePoolTag__(UDF_FILENAME_MT, (DirNdx->FName.MaximumLength = _fn->Length + sizeof(WCHAR)), MEM_FNAMECPY_TAG);
        DirNdx->FName.Length = _fn->Length;
        if(!DirNdx->FName.Buffer)
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        RtlCopyMemory(DirNdx->FName.Buffer, _fn->Buffer, _fn->Length);
        DirNdx->FName.Buffer[_fn->Length/sizeof(WCHAR)] = 0;
CrF__2:
        DirNdx->FI_Flags |= UDFBuildHashEntry(Vcb, &(DirNdx->FName), &(DirNdx->hashes), HASH_ALL);
        // we get here immediately when 'undel' occured
        FileInfo->Index = i;
        DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
        ASSERT(!DirNdx->FileInfo);
        DirNdx->FileInfo = FileInfo;
        DirNdx->FileEntryLoc = FEicb.extLocation;
        // mark file as 'deleted' for now
        DirNdx->FileCharacteristics = FILE_DELETED;
        FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
        FileInfo->Dloc->DataLoc.Mapping = UDFExtentToMapping(&(FileInfo->Dloc->FELoc.Mapping[0]));
        if(!(FileInfo->Dloc->DataLoc.Mapping)) {
            UDFFlushFI(Vcb, FileInfo, PartNum);
            try_return (status = STATUS_INSUFFICIENT_RESOURCES);
        }
        FileInfo->Dloc->DataLoc.Length = 0;
        FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
        FileInfo->ParentFile = DirInfo;
        // init FileEntry
        UDFSetFileUID(Vcb, FileInfo);
        UDFSetFileSize(FileInfo, 0);
        UDFIncFileLinkCount(FileInfo); // increase to 1
        UDFUpdateCreateTime(Vcb, FileInfo);
        UDFAttributesToUDF(UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo),FileInfo->Index),
                             FileInfo->Dloc->FileEntry, Vcb->DefaultAttr);
        FileInfo->Dloc->DataLoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
        FileInfo->Dloc->DataLoc.Modified = TRUE;
        FileInfo->Dloc->FELoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
        // zero sector for FileEntry
        if(!Vcb->CDR_Mode) {
            status = UDFWriteData(Vcb, TRUE, ((int64)(FileInfo->Dloc->FELoc.Mapping[0].extLocation)) << Vcb->BlockSizeBits, LBS, FALSE, Vcb->ZBuffer, &ReadBytes);
            if(!OS_SUCCESS(status)) {
                UDFFlushFI(Vcb, FileInfo, PartNum);
                try_return (status);
            }
        }
#if 0
        if((i >= 2) && (DirNdx->FName.Buffer[0] == L'.')) {
            BrutePoint();
        }
#endif

#ifdef UDF_CHECK_DISK_ALLOCATION
        if(  /*FileInfo->Fcb &&*/
             UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation)) {

            if(!FileInfo->FileIdent ||
               !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
                AdPrint(("Flushing to Discarded block %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
                BrutePoint();
            }
        }
#endif // UDF_CHECK_DISK_ALLOCATION

        // make FileIdent valid
        FileInfo->FileIdent->fileCharacteristics = 0;
        DirNdx->FileCharacteristics = 0;
        UDFReferenceFile__(FileInfo);
        UDFFlushFE(Vcb, FileInfo, PartNum);
        if(undel)
            hDirNdx->DelCount--;
        UDFReleaseDloc(Vcb, FileInfo->Dloc);
        UDFIncFileCounter(Vcb);

        UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used

        try_return (status = STATUS_SUCCESS);

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        if(!OS_SUCCESS(status)) {
            if(FEAllocated)
                UDFFreeFESpace(Vcb, DirInfo, &(FileInfo->Dloc->FELoc));
        }
    } _SEH2_END
    return status;

} // end UDFCreateFile__()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine reads data from file described by FileInfo
 */
/*__inline
OSSTATUS
UDFReadFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN uint32 Length,
    IN BOOLEAN Direct,
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFReadExtent(Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, Buffer, ReadBytes);
} // end UDFReadFile__()*/

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine zeros data in file described by FileInfo
 */
__inline
OSSTATUS
UDFZeroFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN uint32 Length,
    IN BOOLEAN Direct,
    OUT uint32* ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFZeroExtent__(Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, ReadBytes);
} // end UDFZeroFile__()*/

/*
    This routine makes sparse area in file described by FileInfo
 */
__inline
OSSTATUS
UDFSparseFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN int64 Offset,   // offset in extent
    IN uint32 Length,
    IN BOOLEAN Direct,
    OUT uint32* ReadBytes
    )
{
    ValidateFileInfo(FileInfo);

    return UDFSparseExtent__(Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, ReadBytes);
} // end UDFSparseFile__()*/

/*
    This routine fills tails of the last sector in extent with ZEROs
 */
OSSTATUS
UDFPadLastSector(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo
    )
{
    if(!ExtInfo || !(ExtInfo->Mapping) || !(ExtInfo->Length)) return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_write, Lba, sect_offs, flags, WrittenBytes;
    OSSTATUS status;
    // Length should not be zero
    int64 Offset = ExtInfo->Length + ExtInfo->Offset;
    // data is sector-size-aligned, we needn't any padding
    if(Offset && !((uint32)Offset & (Vcb->LBlockSize-1) )) return STATUS_SUCCESS;
    // get Lba of the last sector
    Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
    // EOF check. If we have valid ExtInfo this will not happen, but who knows..
    if((Lba == (uint32)-1) ||
       (flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED))
        return STATUS_END_OF_FILE;
    // write tail
    status = UDFWriteData(Vcb, TRUE, (((int64)Lba) << Vcb->BlockSizeBits) + sect_offs, to_write, FALSE, Vcb->ZBuffer, &WrittenBytes);
    return status;
} // UDFPadLastSector()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine updates AllocDesc sequence, FileIdent & FileEntry
    for given file
 */
OSSTATUS
UDFCloseFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    if(!FileInfo) return STATUS_SUCCESS;
    if(FileInfo->Index<2 && (FileInfo->ParentFile) && !UDFIsAStreamDir(FileInfo)) {
        KdPrint(("Closing Current or Parent Directory... :-\\\n"));
        if(FileInfo->RefCount) {
            UDFInterlockedDecrement((PLONG)&(FileInfo->RefCount));
            ASSERT(FileInfo->Dloc);
            if(FileInfo->Dloc)
                UDFInterlockedDecrement((PLONG)&(FileInfo->Dloc->LinkRefCount));
#ifdef UDF_DBG
        } else {
            BrutePoint();
            KdPrint(("ERROR: Closing unreferenced file!\n"));
#endif // UDF_DBG
        }
        if(FileInfo->ParentFile->OpenCount) {
            UDFInterlockedDecrement((PLONG)&(FileInfo->ParentFile->OpenCount));
#ifdef UDF_DBG
        } else {
            BrutePoint();
            KdPrint(("ERROR: Closing unopened file!\n"));
#endif // UDF_DBG
        }
        return STATUS_SUCCESS;
    }
    PUDF_FILE_INFO DirInfo = FileInfo->ParentFile;
    OSSTATUS status;
    uint32 PartNum;
    if(FileInfo->RefCount) {
        UDFInterlockedDecrement((PLONG)&(FileInfo->RefCount));
        ASSERT(FileInfo->Dloc);
        if(FileInfo->Dloc)
            UDFInterlockedDecrement((PLONG)&(FileInfo->Dloc->LinkRefCount));
#ifdef UDF_DBG
    } else {
        BrutePoint();
        KdPrint(("ERROR: Closing unreferenced file!\n"));
#endif // UDF_DBG
    }
    if(DirInfo) {
        // validate DirInfo
        ValidateFileInfo(DirInfo);

        if(DirInfo->OpenCount) {
            UDFInterlockedDecrement((PLONG)&(DirInfo->OpenCount));
#ifdef UDF_DBG
        } else {
            BrutePoint();
            KdPrint(("ERROR: Closing unopened file!\n"));
#endif // UDF_DBG
        }
    }
    // If the file has gone (unlinked) we should return STATUS_SUCCESS here.
    if(!FileInfo->Dloc) return STATUS_SUCCESS;

    if(FileInfo->RefCount ||
       FileInfo->OpenCount ||
       !(FileInfo->Dloc->FELoc.Mapping)) return STATUS_SUCCESS;
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if(PartNum == (uint32)-1) {
        KdPrint(("  Is DELETED ?\n"));
        if(DirInfo) {
            PartNum = UDFGetPartNumByPhysLba(Vcb, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
        } else {
            BrutePoint();
        }
    }
#ifdef UDF_CHECK_DISK_ALLOCATION
    if(  FileInfo->Fcb &&
         UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation)) {

        //ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        if(UDFIsAStreamDir(FileInfo)) {
            if(!UDFIsSDirDeleted(FileInfo)) {
                KdPrint(("  Not DELETED SDir\n"));
                BrutePoint();
            }
            ASSERT(!FileInfo->Dloc->FELoc.Modified);
        } else
        if(!FileInfo->FileIdent ||
           !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
            if(!FileInfo->FileIdent) {
                AdPrint(("  No FileIdent\n"));
            }
            if(FileInfo->FileIdent &&
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
        if(!FileInfo->Dloc->FELoc.Mapping[0].extLocation ||
            UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation)) {
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_FREE); // check if free
        } else {
            UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used
        }
    }
#endif // UDF_CHECK_DISK_ALLOCATION
    // check if we should update parentICBLocation
    if( !((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.logicalBlockNum &&
        !((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.partitionReferenceNum &&
        DirInfo &&
        !Vcb->CDR_Mode &&
        Vcb->Modified &&
        UDFGetFileLinkCount(FileInfo) ) {
        ASSERT(DirInfo->Dloc->FELoc.Mapping[0].extLocation);
        ((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.logicalBlockNum =
             UDFPhysLbaToPart(Vcb, PartNum, DirInfo->Dloc->FELoc.Mapping[0].extLocation);
        ((icbtag*)(FileInfo->Dloc->FileEntry+1))->parentICBLocation.partitionReferenceNum = (uint16)PartNum;
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
    }

    // we needn't flushing FE & Allocs untill all links are closed...
    if(!FileInfo->Dloc->LinkRefCount) {

        // flush FE and pre-allocation charge for directories
        if(FileInfo->Dloc &&
           FileInfo->Dloc->DirIndex) {

            UDFFlushFESpace(Vcb, FileInfo->Dloc);
            if(FileInfo->Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) {
                FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
                status = UDFResizeExtent(Vcb, PartNum, UDFGetFileSize(FileInfo), FALSE, &(FileInfo->Dloc->DataLoc));
                FileInfo->Dloc->DataLoc.Flags &= ~(EXTENT_FLAG_PREALLOCATED | EXTENT_FLAG_CUT_PREALLOCATED);
                if(OS_SUCCESS(status)) {
                    AdPrint(("Dir pre-alloc truncated (Close)\n"));
                    FileInfo->Dloc->DataLoc.Modified = TRUE;
                }
            }
        }

        if(!OS_SUCCESS(status = UDFFlushFE(Vcb, FileInfo, PartNum))) {
            KdPrint(("Error flushing FE\n"));
//flush_recovery:
            BrutePoint();
            if(FileInfo->Index >= 2) {
                PDIR_INDEX_ITEM DirNdx;
                DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
                if(DirNdx) {
                    KdPrint(("Recovery: mark as deleted & flush FI\n"));
                    DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
                    DirNdx->FileCharacteristics |= FILE_DELETED;
                    FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
                    UDFFlushFI(Vcb, FileInfo, PartNum);
                }
            }
            return status;
        }
    }
    // ... but FI must be updated (if any)
    if(!OS_SUCCESS(status = UDFFlushFI(Vcb, FileInfo, PartNum))) {
        KdPrint(("Error flushing FI\n"));
        return status;
    }
#ifdef UDF_DBG
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if((FileInfo->Dloc->FileEntry->descVersion != 2) &&
       (FileInfo->Dloc->FileEntry->descVersion != 3)) {
        ASSERT(UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFCloseFile__()


#ifndef UDF_READ_ONLY_BUILD
/*
    This routine moves file from DirInfo1 to DirInfo2 & renames it to fn
 */
OSSTATUS
UDFRenameMoveFile__(
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
    OSSTATUS status;
    PDIR_INDEX_ITEM DirNdx1;
    PDIR_INDEX_ITEM DirNdx2;
    uint_di i,j;
    BOOLEAN Recovery = FALSE;
    BOOLEAN SameFE = FALSE;
    uint32 NTAttr = 0;

    // validate FileInfo
    ValidateFileInfo(DirInfo1);
    ValidateFileInfo(DirInfo2);
    ValidateFileInfo(FileInfo);

    i = j = 0;
    if(DirInfo1 == DirInfo2) {
        if(OS_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &j)) &&
           (j==FileInfo->Index) ) {
            // case-only rename
            uint8* CS0;
            uint32 Nlen, /* l, FIXME ReactOS */ IUl;

            // prepare filename
            UDFCompressUnicode(fn, &CS0, &Nlen);
            if(!CS0) return STATUS_INSUFFICIENT_RESOURCES;
/*            if(Nlen > UDF_NAME_LEN) {
                if(CS0) MyFreePool__(CS0);
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
            
            if(CS0) MyFreePool__(CS0);
            
            DirNdx2->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            UDFBuildHashEntry(Vcb, &(DirNdx2->FName), &(DirNdx2->hashes), HASH_ALL);
            return STATUS_SUCCESS;
/*        } else
        if(!OS_SUCCESS(status) && (fn->Length == UDFDirIndex(DirInfo2->Dloc->DirIndex, j=FileInfo->Index)->FName.Length)) {
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
    if(!OS_SUCCESS(status = UDFCreateFile__(Vcb, IgnoreCase, fn, UDFGetFileEALength(FileInfo),
                    0, (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY),
                    TRUE, DirInfo2, &FileInfo2))) {
        UDFCleanUpFile__(Vcb, FileInfo2);
        if(FileInfo2) MyFreePool__(FileInfo2);
        if(status == STATUS_ACCESS_DENIED) {
            // try to recover >;->
            if((*Replace) && !Recovery) {
                Recovery = TRUE;
                status = UDFOpenFile__(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &FileInfo2, NULL);
                if(OS_SUCCESS(status)) {
                    status = UDFDoesOSAllowFileToBeTargetForRename__(FileInfo2);
                    if(!OS_SUCCESS(status)) {
                        UDFCloseFile__(Vcb, FileInfo2);
                        goto cleanup_and_abort_rename;
                    }
                    status = UDFUnlinkFile__(Vcb, FileInfo2, TRUE);
//                    UDFPretendFileDeleted__(Vcb, FileInfo2);
                    UDFCloseFile__(Vcb, FileInfo2);
                    if(UDFCleanUpFile__(Vcb, FileInfo2)) {
                        MyFreePool__(FileInfo2);
                        FileInfo2 = NULL;
                        if(SameFE)
                            return status;
                    } else {
                        // we get here if the FileInfo has associated
                        // system-specific Fcb
                        // Such fact means that not all system references
                        // has already gone (except Linked file case)
/*                        if(SameFE)
                            return status;*/
//                        UDFRemoveOSReferences__(FileInfo2);
                        if(!OS_SUCCESS(status) ||
                           (UDFGetFileLinkCount(FileInfo2) < 1))
                            status = STATUS_ACCESS_DENIED;
                    }
                    if(OS_SUCCESS(status)) goto RenameRetry;
                }
cleanup_and_abort_rename:
                if(FileInfo2 && UDFCleanUpFile__(Vcb, FileInfo2)) {
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
    // unlink source FileIdent
    if(!OS_SUCCESS(status = UDFUnlinkFile__(Vcb, FileInfo, FALSE))) {
        // kill newly created entry
        UDFFlushFile__(Vcb, FileInfo2);
        UDFUnlinkFile__(Vcb, FileInfo2, TRUE);
        UDFCloseFile__(Vcb, FileInfo2);
        UDFCleanUpFile__(Vcb, FileInfo2);
        MyFreePool__(FileInfo2);
        return status;
    }

    // PHASE 1
    // drop all unnecessary info from FileInfo & flush FI

    DirNdx1->FileInfo = NULL;
    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    UDFFlushFI(Vcb, FileInfo, UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation));
    UDFInterlockedExchangeAdd((PLONG)&(DirInfo1->OpenCount),
                            -((LONG)(FileInfo->RefCount)));
    // PHASE 2
    // copy all necessary info from FileInfo to FileInfo2

    FileInfo2->FileIdent->icb = FileInfo->FileIdent->icb;
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
    UDFInterlockedExchangeAdd((PLONG)&(DirInfo2->OpenCount),
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

//    UDFUpdateModifyTime(Vcb, FileInfo);

    // PHASE 4
    // drop all unnecessary info from FileInfo2

    UDFFreeFESpace(Vcb, DirInfo2, &(FileInfo2->Dloc->FELoc));
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
OSSTATUS
UDFRecordDirectory__(
    IN PVCB Vcb,
 IN OUT PUDF_FILE_INFO DirInfo   // source (opened)
    )
{
    OSSTATUS status;
    LONG_AD FEicb;
    UDF_FILE_INFO FileInfo;
    UDF_DATALOC_INFO Dloc;
    UNICODE_STRING PName;
    uint32 PartNum;
    uint32 WrittenBytes;
    PDIR_INDEX_ITEM CurDirNdx;
    uint32 lba;

    // validate DirInfo
    ValidateFileInfo(DirInfo);
    if(DirInfo->ParentFile && UDFIsAStreamDir(DirInfo->ParentFile))
        return STATUS_ACCESS_DENIED;
    // file should be empty
    if(UDFGetFileSize(DirInfo)) {
        if( DirInfo->FileIdent &&
           (DirInfo->FileIdent->fileCharacteristics & FILE_DIRECTORY)) return STATUS_FILE_IS_A_DIRECTORY;
        return STATUS_NOT_A_DIRECTORY;
    }
    if(DirInfo->Dloc->DirIndex) return STATUS_FILE_IS_A_DIRECTORY;
    // create empty DirIndex
    if(DirInfo->FileIdent) DirInfo->FileIdent->fileCharacteristics |= FILE_DIRECTORY;
    if((CurDirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(DirInfo),DirInfo->Index)))
        CurDirNdx->FileCharacteristics |= FILE_DIRECTORY;
    ((icbtag*)(DirInfo->Dloc->FileEntry+1))->fileType = UDF_FILE_TYPE_DIRECTORY;
    // init temporary FileInfo
    RtlZeroMemory(&FileInfo, sizeof(UDF_FILE_INFO));
    FileInfo.Dloc = &Dloc;
    FileInfo.Dloc->FileEntry = DirInfo->ParentFile->Dloc->FileEntry;
    FileInfo.Dloc->FileEntryLen = DirInfo->ParentFile->Dloc->FileEntryLen;
    FileInfo.Dloc->DataLoc = DirInfo->Dloc->DataLoc;
    FileInfo.Dloc->FELoc = DirInfo->Dloc->FELoc;
    FileInfo.ParentFile = DirInfo;
    // prepare FileIdent for 'parent Dir'
    lba = DirInfo->Dloc->FELoc.Mapping[0].extLocation;
    ASSERT(lba);
    PartNum = UDFGetPartNumByPhysLba(Vcb, lba);
    FEicb.extLength = Vcb->LBlockSize;
    FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, lba);
    FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
    RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));
    PName.Buffer = (PWCH)L"";
    PName.Length = (PName.MaximumLength = sizeof(L"")) - sizeof(WCHAR);
    if(!OS_SUCCESS(status =
        UDFBuildFileIdent(Vcb, &PName, &FEicb, 0,
                &(FileInfo.FileIdent), &(FileInfo.FileIdentLen)) ))
        return status;
    FileInfo.FileIdent->fileCharacteristics |= (FILE_PARENT | FILE_DIRECTORY);
    UDFDecFileCounter(Vcb);
    UDFIncDirCounter(Vcb);
    // init structure
    UDFSetUpTag(Vcb, &(FileInfo.FileIdent->descTag), (uint16)(FileInfo.FileIdentLen),
              FEicb.extLocation.logicalBlockNum);
    FileInfo.Dloc->DataLoc.Flags |= EXTENT_FLAG_VERIFY; // for metadata
    // flush
    status = UDFWriteFile__(Vcb, DirInfo, 0, FileInfo.FileIdentLen, FALSE, (int8*)(FileInfo.FileIdent), &WrittenBytes);
//    status = UDFFlushFI(Vcb, &FileInfo, PartNum);

#ifdef UDF_DBG
    if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
        ASSERT(UDFGetFileSize(DirInfo) <= UDFGetExtentLength(DirInfo->Dloc->DataLoc.Mapping));
    } else {
        ASSERT(((UDFGetFileSize(DirInfo)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)) ==
               ((UDFGetExtentLength(DirInfo->Dloc->DataLoc.Mapping)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)));
    }
#endif // UDF_DBG

    MyFreePool__(FileInfo.FileIdent);
    if(!OS_SUCCESS(status)) return status;
    if(CurDirNdx) CurDirNdx->FileCharacteristics =
        DirInfo->FileIdent->fileCharacteristics;
    return UDFIndexDirectory(Vcb, DirInfo);
} // end UDFRecordDirectory__()

/*
    This routine changes file size (on disc)
 */
OSSTATUS
UDFResizeFile__(
    IN PVCB Vcb,
 IN OUT PUDF_FILE_INFO FileInfo,
    IN int64 NewLength
    )
{
    uint32 WrittenBytes;
    OSSTATUS status;
    uint32 PartNum;
    int8* OldInIcb = NULL;
    PEXTENT_MAP NewMap;

    KdPrint(("UDFResizeFile__: FI %x, -> %I64x\n", FileInfo, NewLength));
    ValidateFileInfo(FileInfo);
//    ASSERT(FileInfo->RefCount >= 1);

    if((NewLength >> Vcb->LBlockSizeBits) > Vcb->TotalAllocUnits) {
        KdPrint(("STATUS_DISK_FULL\n"));
        return STATUS_DISK_FULL;
    }
    if (NewLength == FileInfo->Dloc->DataLoc.Length) return STATUS_SUCCESS;
    if(FileInfo->ParentFile && (FileInfo->Index >= 2)) {
        UDFDirIndex(FileInfo->ParentFile->Dloc->DirIndex,FileInfo->Index)->FI_Flags &= ~UDF_FI_FLAG_SYS_ATTR;
    }
    if(NewLength > FileInfo->Dloc->DataLoc.Length) {
        // grow file
        return UDFWriteFile__(Vcb, FileInfo, NewLength, 0, FALSE, NULL, &WrittenBytes);
    }
    // truncate file
    if(NewLength <= (Vcb->LBlockSize - FileInfo->Dloc->FileEntryLen)) {
        // check if we are already in IN_ICB mode
        if((((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) != ICB_FLAG_AD_IN_ICB) {
            // read data from old location
            if(NewLength) {
                OldInIcb = (int8*)MyAllocatePool__(NonPagedPool, (uint32)NewLength);
                if(!OldInIcb) return STATUS_INSUFFICIENT_RESOURCES;
                status = UDFReadExtent(Vcb, &(FileInfo->Dloc->DataLoc), 0, (uint32)NewLength, FALSE, OldInIcb, &WrittenBytes);
                if(!OS_SUCCESS(status)) {
                    MyFreePool__(OldInIcb);
                    return status;
                }
            } else {
                OldInIcb = NULL;
            }
            // allocate storage for new mapping
            NewMap = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , 2*sizeof(EXTENT_MAP),
                                                               MEM_EXTMAP_TAG);
            if(!(NewMap)) {
                MyFreePool__(OldInIcb);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            // free old location...
            if(FileInfo->Dloc->DataLoc.Mapping[0].extLocation !=
               FileInfo->Dloc->FELoc.Mapping[0].extLocation) {
               ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
mark_data_map_0:
                UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, FileInfo->Dloc->DataLoc.Mapping, AS_DISCARDED); // free
            } else {
                if((FileInfo->Dloc->DataLoc.Mapping[0].extLength & UDF_EXTENT_LENGTH_MASK)
                       > Vcb->LBlockSize) {
                    BrutePoint();
                    FileInfo->Dloc->DataLoc.Mapping[0].extLength -= Vcb->LBlockSize;
                    FileInfo->Dloc->DataLoc.Mapping[0].extLocation += (1 << Vcb->LB2B_Bits);
                    goto mark_data_map_0;
                }
                UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->DataLoc.Mapping[1]), AS_DISCARDED); // free
            }
            if(FileInfo->Dloc->AllocLoc.Mapping) {
                if((FileInfo->Dloc->AllocLoc.Mapping[0].extLength & UDF_EXTENT_LENGTH_MASK)
                       > Vcb->LBlockSize) {
                    FileInfo->Dloc->AllocLoc.Mapping[0].extLength -= Vcb->LBlockSize;
                    FileInfo->Dloc->AllocLoc.Mapping[0].extLocation += (1 << Vcb->LB2B_Bits);
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
            // init new data location descriptors
            FileInfo->Dloc->DataLoc.Mapping = NewMap;
            RtlZeroMemory((int8*)(FileInfo->Dloc->DataLoc.Mapping), 2*sizeof(EXTENT_MAP));
            FileInfo->Dloc->DataLoc.Mapping[0] = FileInfo->Dloc->FELoc.Mapping[0];
            FileInfo->Dloc->DataLoc.Length = NewLength;
            FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
            // write data to new location
            if(OldInIcb) {
                status = UDFWriteExtent(Vcb, &(FileInfo->Dloc->DataLoc), 0, (uint32)NewLength, FALSE, OldInIcb, &WrittenBytes);
            } else {
                status = STATUS_SUCCESS;
            }
            FileInfo->Dloc->DataLoc.Modified = TRUE;
            if(OldInIcb) MyFreePool__(OldInIcb);
        } else {
            // just modify Length field
            FileInfo->Dloc->DataLoc.Length = NewLength;
            status = STATUS_SUCCESS;
        }
    } else {
        // resize extent
        ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        PartNum = UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
        status = UDFResizeExtent(Vcb, PartNum, NewLength, FALSE, &(FileInfo->Dloc->DataLoc));
        FileInfo->Dloc->DataLoc.Modified = TRUE;
        FileInfo->Dloc->AllocLoc.Modified = TRUE;
    }
    if(OS_SUCCESS(status)) {
        UDFSetFileSize(FileInfo, NewLength);
    }

#ifdef UDF_DBG
    if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
        ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
    } else {
        ASSERT(((UDFGetFileSize(FileInfo)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)) ==
               ((UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)));
    }
#endif // UDF_DBG

    return status;
} // end UDFResizeFile__()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine loads VAT.
 */
OSSTATUS
UDFLoadVAT(
    IN PVCB Vcb,
    IN uint32 PartNdx
    )
{
    lb_addr VatFELoc;
    OSSTATUS status;
    PUDF_FILE_INFO VatFileInfo;
    uint32 len, i=0, j, to_read;
    uint32 Offset, hdrOffset;
    uint32 ReadBytes;
    uint32 root;
    uint16 PartNum;
//    uint32 VatFirstLba = 0;
    int8* VatOldData;
    uint32 VatLba[6] = { Vcb->LastLBA,
                        Vcb->LastLBA - 2,
                        Vcb->LastLBA - 3,
                        Vcb->LastLBA - 5,
                        Vcb->LastLBA - 7,
                        0 };

    if(Vcb->Vat) return STATUS_SUCCESS;
    if(!Vcb->CDR_Mode) return STATUS_SUCCESS;
    // disable VAT for now. We'll reenable it if VAT is successfuly loaded
    Vcb->CDR_Mode = FALSE;
    PartNum = Vcb->Partitions[PartNdx].PartitionNum;
    root = Vcb->Partitions[PartNdx].PartitionRoot;
    if(Vcb->LBlockSize != Vcb->BlockSize) {
        // don't know how to operate... :(((
        return STATUS_UNRECOGNIZED_VOLUME;
    }
    if((Vcb->LastTrackNum > 1) &&
       (Vcb->LastLBA == Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba)) {
        KdPrint(("Hardware Read-only volume\n"));
        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
    }

    VatFileInfo = Vcb->VatFileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT, sizeof(UDF_FILE_INFO), MEM_VATFINF_TAG);
    if(!VatFileInfo) return STATUS_INSUFFICIENT_RESOURCES;
    // load VAT FE (we know its location)
    VatFELoc.partitionReferenceNum = PartNum;
retry_load_vat:
    VatFELoc.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, VatLba[i]);
    if(!OS_SUCCESS(status = UDFOpenRootFile__(Vcb, &VatFELoc, VatFileInfo))) {
        UDFCleanUpFile__(Vcb, VatFileInfo);
        // try another location
        i++;
        if( VatLba[i] &&
           (status != STATUS_FILE_CORRUPT_ERROR) &&
           (status != STATUS_CRC_ERROR)) goto retry_load_vat;
        MyFreePool__(VatFileInfo);
        Vcb->VatFileInfo = NULL;
        return status;
    }
    len = (uint32)UDFGetFileSize(VatFileInfo);
    if(Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP15) {
        // load Vat 1.50 header
        KdPrint(("Load VAT 1.50\n"));
        VirtualAllocationTable15* Buf;
        if(((icbtag*)(VatFileInfo->Dloc->FileEntry+1))->fileType != UDF_FILE_TYPE_VAT15) {
            status = STATUS_FILE_CORRUPT_ERROR;
            goto err_vat_15;
        }
        Buf = (VirtualAllocationTable15*)MyAllocatePool__(NonPagedPool, sizeof(VirtualAllocationTable15));
        if(!Buf) {
err_vat_15_2:
            status = STATUS_INSUFFICIENT_RESOURCES;
err_vat_15:
            UDFCloseFile__(Vcb, VatFileInfo);
            UDFCleanUpFile__(Vcb, VatFileInfo);
            MyFreePool__(VatFileInfo);
            Vcb->VatFileInfo = NULL;
            return status;
        }
        Offset = 0;
        to_read =
        hdrOffset = len - sizeof(VirtualAllocationTable15);
        MyFreePool__(Buf);

        Vcb->minUDFReadRev  =
        Vcb->minUDFWriteRev =
        Vcb->maxUDFWriteRev = 0x0150;

        Vcb->numFiles =
        Vcb->numDirs  = -1;

    } else
    if(Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP20) {
        // load Vat 2.00 header
        KdPrint(("Load VAT 2.00\n"));
        VirtualAllocationTable20* Buf;
        if(((icbtag*)(VatFileInfo->Dloc->FileEntry+1))->fileType != UDF_FILE_TYPE_VAT20) {
            status = STATUS_FILE_CORRUPT_ERROR;
            goto err_vat_15;
        }
        Buf = (VirtualAllocationTable20*)MyAllocatePool__(NonPagedPool, sizeof(VirtualAllocationTable20));
        if(!Buf) goto err_vat_15_2;
        Offset = Buf->lengthHeader;
        to_read = len - Offset;
        hdrOffset = 0;
        MyFreePool__(Buf);

        Vcb->minUDFReadRev  = Buf->minReadRevision;
        Vcb->minUDFWriteRev = Buf->minWriteRevision;
        Vcb->maxUDFWriteRev = Buf->maxWriteRevision;

        Vcb->numFiles = Buf->numFIDSFiles;
        Vcb->numDirs  = Buf->numFIDSDirectories;

    } else {
        // unknown (or wrong) VAT format
        KdPrint(("unknown (or wrong) VAT format\n"));
        status = STATUS_FILE_CORRUPT_ERROR;
        goto err_vat_15;
    }
    // read VAT & remember old version
    Vcb->Vat = (uint32*)DbgAllocatePool(NonPagedPool, (Vcb->LastPossibleLBA+1)*sizeof(uint32) );
    if(!Vcb->Vat) {
        goto err_vat_15_2;
    }
    // store base version of VAT in memory
    VatOldData = (int8*)DbgAllocatePool(PagedPool, len);
    if(!VatOldData) {
        DbgFreePool(Vcb->Vat);
        Vcb->Vat = NULL;
        goto err_vat_15_2;
    }
    status = UDFReadFile__(Vcb, VatFileInfo, 0, len, FALSE, VatOldData, &ReadBytes);
    if(!OS_SUCCESS(status)) {
        UDFCloseFile__(Vcb, VatFileInfo);
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
        RtlFillMemory(&(Vcb->Vat[Vcb->NWA-root]), (Vcb->LastPossibleLBA-Vcb->NWA+1)*sizeof(uint32), 0xff);
        // sync VAT and FSBM
        for(i=0; i<len; i++) {
            if(Vcb->Vat[i] == UDF_VAT_FREE_ENTRY) {
                UDFSetFreeBit(Vcb->FSBM_Bitmap, root+i);
            }
        }
        len = Vcb->LastPossibleLBA;
        // "pre-format" reserved area
        for(i=Vcb->NWA; i<len;) {
            for(j=0; (j<PACKETSIZE_UDF) && (i<len); j++, i++)
                UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
            for(j=0; (j<7) && (i<len); j++, i++)
                UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
        }
        DbgFreePool(VatOldData);
    }
    return status;
} // end UDFLoadVAT()

/*
    Reads Extended Attributes
    Caller should use UDFGetFileEALength to allocate Buffer of sufficient
    size
 *//*
OSSTATUS
UDFReadFileEA(
    IN PVCB Vcb,
    IN PDIR_INDEX FileDirNdx,
    OUT int8* Buffer
    )
{
    PFILE_ENTRY FileEntry;
    OSSTATUS status;

    if(FileDirNdx->FileInfo) {
        FileEntry = (PFILE_ENTRY)(FileDirNdx->FileInfo->Dloc->FileEntry);
    } else {
        FileEntry = (PFILE_ENTRY)MyAllocatePool__(NonPagedPool, Vcb->BlockSize);
        if(!FileEntry) return;
        if(!OS_SUCCESS(status = UDFReadFileEntry(Vcb, &(FileDirNdx->FileEntry), FileEntry, &Ident))) {
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

    if(!FileInfo) return FALSE;
    if(FileInfo->Dloc->DirIndex) return TRUE;
    if(!(FileInfo->FileIdent)) return FALSE;
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
        if(!(fc & (FILE_PARENT | FILE_DELETED)) &&
           CurDirNdx->Length)
            return FALSE;
    }
    return TRUE;
} // end UDFIsDirEmpty()

/*
 */
OSSTATUS
UDFFlushFE(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum
    )
{
    int8* NewAllocDescs;
    OSSTATUS status;
    uint32 WrittenBytes;
    uint16 AllocMode;
    uint32 lba;

    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
#ifdef UDF_DBG
/*    if(UDFIsADirectory(FileInfo) && (UDFGetFileSize(FileInfo) < 0x28) &&
       !UDFIsDeleted(UDFDirIndex(DirInfo->Dloc->DirIndex, FileInfo->Index)) ) {
        BrutePoint();
    }*/
//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if(FileInfo->Dloc->FELoc.Offset) {
        BrutePoint();
    }
    if(FileInfo->Dloc->AllocLoc.Mapping) {
        ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
    }
#endif // UDF_DBG
retry_flush_FE:
    KdPrint(("  FlushFE: %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
#ifndef UDF_READ_ONLY_BUILD
    UDFReTagDirectory(Vcb, FileInfo);
    if(FileInfo->Dloc->DataLoc.Modified ||
       FileInfo->Dloc->AllocLoc.Modified) {
        ASSERT(PartNum != (uint32)(-1));
        // prepare new AllocDescs for flushing...
        if(!OS_SUCCESS(status = UDFBuildAllocDescs(Vcb, PartNum, FileInfo, &NewAllocDescs))) {
            KdPrint(("  FlushFE: UDFBuildAllocDescs() faliled (%x)\n", status));
            if(NewAllocDescs)
                MyFreePool__(NewAllocDescs);
            return status;
        }
#ifdef UDF_DBG
        if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
            ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
        } else {
            ASSERT(((UDFGetFileSize(FileInfo)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)) ==
                   ((UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)));
        }
        AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
#endif // UDF_DBG
        // initiate update of lengthAllocDescs
        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        if(NewAllocDescs) {
            ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
            status = UDFPadLastSector(Vcb, &(FileInfo->Dloc->AllocLoc));
            // ... and flush it
            status = UDFWriteExtent(Vcb, &(FileInfo->Dloc->AllocLoc), 0, (uint32)(FileInfo->Dloc->AllocLoc.Length), FALSE, NewAllocDescs, &WrittenBytes);
            MyFreePool__(NewAllocDescs);
            if(!OS_SUCCESS(status)) {
                KdPrint(("  FlushFE: UDFWriteExtent() faliled (%x)\n", status));
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
        if(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) {
            ASSERT(UDFGetFileSize(FileInfo) <= UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping));
        } else {
            ASSERT(((UDFGetFileSize(FileInfo)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)) ==
                   ((UDFGetExtentLength(FileInfo->Dloc->DataLoc.Mapping)+Vcb->LBlockSize-1) & (Vcb->LBlockSize-1)));
        }
#endif // UDF_DBG
    }
/*    if(FileInfo->Fcb &&
       ((FileInfo->Dloc->FELoc.Mapping[0].extLocation > Vcb->LastLBA) ||
        UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation)) ) {
        BrutePoint();
    }*/
/*    if(FileInfo->Dloc->FELoc.Mapping[0].extLocation) {
        ASSERT( FileInfo->Dloc->FileEntry->tagLocation ==
               (FileInfo->Dloc->FELoc.Mapping[0].extLocation - 0x580));
    }*/
    if((FileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
        FileInfo->Dloc->FELoc.Modified) {
        ASSERT(PartNum != (uint32)(-1));
        ASSERT(!PartNum);
        if(PartNum == (uint32)(-1) || PartNum == (uint32)(-2)) {
            KdPrint(("  bad PartNum: %d\n", PartNum));
        }
        // update lengthAllocDescs in FE
        UDFSetAllocDescLen(Vcb, FileInfo);
/*        ASSERT( FileInfo->Dloc->FileEntry->tagLocation ==
               (FileInfo->Dloc->FELoc.Mapping[0].extLocation - 0x580));*/
        // flush FileEntry

        // if FE is located in remapped block, place it to reliable space
        lba = FileInfo->Dloc->FELoc.Mapping[0].extLocation;
        if(Vcb->BSBM_Bitmap) {
            if(UDFGetBadBit((uint32*)(Vcb->BSBM_Bitmap), lba)) {
                AdPrint(("  bad block under FE @%x\n", lba));
                goto relocate_FE;
            }
        }

        AdPrint(("  setup tag: @%x\n", lba));
        ASSERT( lba );
        UDFSetUpTag(Vcb, FileInfo->Dloc->FileEntry, (uint16)(FileInfo->Dloc->FileEntryLen),
                  UDFPhysLbaToPart(Vcb, PartNum, lba));
        status = UDFWriteExtent(Vcb, &(FileInfo->Dloc->FELoc), 0,
                  (uint32)(FileInfo->Dloc->FELoc.Length), FALSE,
                  (int8*)(FileInfo->Dloc->FileEntry), &WrittenBytes);
        if(!OS_SUCCESS(status)) {
            KdPrint(("  FlushFE: UDFWriteExtent(2) faliled (%x)\n", status));
            if(status == STATUS_DEVICE_DATA_ERROR) {
relocate_FE:
                KdPrint(("  try to relocate\n"));

                EXTENT_INFO _FEExtInfo;
                // calculate the length required
                
                // allocate block for FE
                if(OS_SUCCESS(UDFAllocateFESpace(Vcb, FileInfo->ParentFile, PartNum, &_FEExtInfo, (uint32)(FileInfo->Dloc->FELoc.Length)) )) {
                    KdPrint(("  relocate %x -> %x\n",
                        lba,
                        _FEExtInfo.Mapping[0].extLocation));

                    UDFMarkSpaceAsXXX(Vcb, 0, FileInfo->Dloc->FELoc.Mapping, AS_BAD);

                    UDFRelocateDloc(Vcb, FileInfo->Dloc, _FEExtInfo.Mapping[0].extLocation);
                    MyFreePool__(FileInfo->Dloc->FELoc.Mapping);
                    FileInfo->Dloc->FELoc.Mapping = _FEExtInfo.Mapping;

                    FileInfo->Dloc->FELoc.Modified = TRUE;
                    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;

                    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
                    if(AllocMode == ICB_FLAG_AD_IN_ICB) {
                        KdPrint(("  IN-ICB data lost\n"));
                        FileInfo->Dloc->DataLoc.Mapping[0].extLocation = _FEExtInfo.Mapping[0].extLocation;
                        FileInfo->Dloc->DataLoc.Modified = TRUE;
                    } else {
                        FileInfo->Dloc->AllocLoc.Mapping[0].extLocation = _FEExtInfo.Mapping[0].extLocation;
                        FileInfo->Dloc->AllocLoc.Modified = TRUE;
                    }

                    if(FileInfo->Index >= 2) {
                        PDIR_INDEX_ITEM DirNdx;
                        DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
                        if(DirNdx) {
                            KdPrint(("  update reference in FI\n"));
                            DirNdx->FileEntryLoc.logicalBlockNum =
                                FileInfo->FileIdent->icb.extLocation.logicalBlockNum =
                                UDFPhysLbaToPart(Vcb, PartNum, _FEExtInfo.Mapping[0].extLocation);
                            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
                        }
                    }
                    // this will update 
                    KdPrint(("  retry flush...\n"));
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
#endif //UDF_READ_ONLY_BUILD
#ifdef UDF_DBG
    if(FileInfo->Dloc->AllocLoc.Mapping) {
        ASSERT(AllocMode != ICB_FLAG_AD_IN_ICB);
    } else {
        ASSERT(AllocMode == ICB_FLAG_AD_IN_ICB);
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFFlushFE()

OSSTATUS
UDFFlushFI(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint32 PartNum
    )
{
    PUDF_FILE_INFO DirInfo = FileInfo->ParentFile;
    PDIR_INDEX_ITEM DirNdx;
    OSSTATUS status;
    uint32 WrittenBytes;
    // use WrittenBytes variable to store LBA of FI to be recorded
    #define lba   WrittenBytes

//    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    // some files has no FI
    if(!DirInfo || UDFIsAStreamDir(FileInfo))
        return STATUS_SUCCESS;
    DirNdx = UDFDirIndex(DirInfo->Dloc->DirIndex, FileInfo->Index);
//    ASSERT(FileInfo->FileIdent->lengthFileIdent < 0x80);
#ifdef UDF_DBG
    if(DirNdx->FileCharacteristics & FILE_DELETED) {
        ASSERT(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED);
    }
#endif // UDF_DBG
    KdPrint(("  FlushFI: offs %x\n", (ULONG)(DirNdx->Offset)));
#ifndef UDF_READ_ONLY_BUILD
    if((DirNdx->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) {
        // flush FileIdent
        ASSERT(PartNum != (uint32)(-1));
        FileInfo->FileIdent->fileCharacteristics = DirNdx->FileCharacteristics;
        lba = UDFExtentOffsetToLba(Vcb, DirInfo->Dloc->DataLoc.Mapping,
                                        DirNdx->Offset, NULL, NULL, NULL, NULL);
        AdPrint(("  FI lba %x\n", lba));
        // check if requested Offset is allocated
        if(lba == (uint32)LBA_OUT_OF_EXTENT) {
            // write 1 byte
            if(!OS_SUCCESS(status = UDFWriteFile__(Vcb, DirInfo, DirNdx->Offset, 1, FALSE, (int8*)(FileInfo->FileIdent), &WrittenBytes) )) {
                BrutePoint();
                return status;
            }
            lba = UDFExtentOffsetToLba(Vcb, DirInfo->Dloc->DataLoc.Mapping,
                                            DirNdx->Offset, NULL, NULL, NULL, NULL);
            AdPrint(("  allocated FI lba %x\n", lba));
            // check if requested Offset is allocated
            if(lba == (uint32)LBA_OUT_OF_EXTENT) {
                BrutePoint();
                return STATUS_UNSUCCESSFUL;
            }
        }
        // init structure
        UDFSetUpTag(Vcb, &(FileInfo->FileIdent->descTag), (uint16)(FileInfo->FileIdentLen),
                  UDFPhysLbaToPart(Vcb, PartNum, lba));
        // record data
        if(!OS_SUCCESS(status = UDFWriteFile__(Vcb, DirInfo, DirNdx->Offset, FileInfo->FileIdentLen, FALSE, (int8*)(FileInfo->FileIdent), &WrittenBytes) )) {
            BrutePoint();
            return status;
        }
        DirNdx->FI_Flags &= ~UDF_FI_FLAG_FI_MODIFIED;
    }
#endif //UDF_READ_ONLY_BUILD
    return STATUS_SUCCESS;
} // end UDFFlushFI()

/*
    This routine updates AllocDesc sequence, FileIdent & FileEntry
    for given file
 */
OSSTATUS
UDFFlushFile__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN ULONG FlushFlags
    )
{
    ValidateFileInfo(FileInfo);

    if(!FileInfo) return STATUS_SUCCESS;
    OSSTATUS status;
    uint32 PartNum;

    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if(PartNum == (uint32)-1) {
        KdPrint(("  Is DELETED ?\n"));
        if(FileInfo->ParentFile) {
            PartNum = UDFGetPartNumByPhysLba(Vcb, FileInfo->ParentFile->Dloc->FELoc.Mapping[0].extLocation);
        } else {
            BrutePoint();
        }
    }
#ifdef UDF_CHECK_DISK_ALLOCATION
    if( FileInfo->Fcb &&
        UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), FileInfo->Dloc->FELoc.Mapping[0].extLocation)) {

        if(UDFIsAStreamDir(FileInfo)) {
            if(!UDFIsSDirDeleted(FileInfo)) {
                KdPrint(("  Not DELETED SDir\n"));
                BrutePoint();
            }
            ASSERT(!FileInfo->Dloc->FELoc.Modified);
        } else
        if(!FileInfo->FileIdent ||
           !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED)) {
            if(!FileInfo->FileIdent)
                AdPrint(("  No FileIdent\n"));
            if(FileInfo->FileIdent &&
               !(FileInfo->FileIdent->fileCharacteristics & FILE_DELETED))
                AdPrint(("  Not DELETED\n"));
            AdPrint(("Flushing to Discarded block %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
            BrutePoint();
        }
    }
#endif // UDF_CHECK_DISK_ALLOCATION

    // flush FE and pre-allocation charge for directories
    if(FileInfo->Dloc &&
       FileInfo->Dloc->DirIndex) {
        // if Lite Flush is used, keep preallocations
        if(!(FlushFlags & UDF_FLUSH_FLAGS_LITE)) {
full_flush:
            UDFFlushFESpace(Vcb, FileInfo->Dloc);
            if(FileInfo->Dloc->DataLoc.Flags & EXTENT_FLAG_PREALLOCATED) {
                FileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_CUT_PREALLOCATED;
                status = UDFResizeExtent(Vcb, PartNum, UDFGetFileSize(FileInfo), FALSE, &(FileInfo->Dloc->DataLoc));
                FileInfo->Dloc->DataLoc.Flags &= ~(EXTENT_FLAG_PREALLOCATED | EXTENT_FLAG_CUT_PREALLOCATED);
                if(OS_SUCCESS(status)) {
                    AdPrint(("Dir pre-alloc truncated (Flush)\n"));
                    FileInfo->Dloc->DataLoc.Modified = TRUE;
                }
            }
        }
    }
    // flush FE
    if(!OS_SUCCESS(status = UDFFlushFE(Vcb, FileInfo, PartNum))) {
        KdPrint(("Error flushing FE\n"));
        BrutePoint();
        if(FlushFlags & UDF_FLUSH_FLAGS_LITE) {
            KdPrint(("  flush pre-alloc\n"));
            FlushFlags &= ~UDF_FLUSH_FLAGS_LITE;
            goto full_flush;
        }
        if(FileInfo->Index >= 2) {
            PDIR_INDEX_ITEM DirNdx;
            DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(FileInfo), FileInfo->Index);
            if(DirNdx) {
                KdPrint(("Recovery: mark as deleted & flush FI\n"));
                DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
                DirNdx->FileCharacteristics |= FILE_DELETED;
                FileInfo->FileIdent->fileCharacteristics |= FILE_DELETED;
                UDFFlushFI(Vcb, FileInfo, PartNum);
            }
        }
        return status;
    }
    if(!OS_SUCCESS(status = UDFFlushFI(Vcb, FileInfo, PartNum)))
        return status;

    ASSERT((FileInfo->Dloc->FileEntry->descVersion == 2) ||
           (FileInfo->Dloc->FileEntry->descVersion == 3));

    return STATUS_SUCCESS;
} // end UDFFlushFile__()

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

    if(!f1 || !f2) return FALSE;
    if(f1->Dloc->FileEntryLen != f2->Dloc->FileEntryLen) return FALSE;
//    if(f1->FileIdentLen != f2->FileIdentLen) return FALSE;
/*    if(f1->Dloc->DirIndex && !f2->Dloc->DirIndex) return FALSE;
    if(f2->Dloc->DirIndex && !f1->Dloc->DirIndex) return FALSE;
    if((f1->Dloc->DirIndex) &&
       (f1->Dloc->DirIndex->LastFrameCount != f2->Dloc->DirIndex->LastFrameCount)) return FALSE;*/
    if(f1->Index != f2->Index) return FALSE;
    if(!(f1->Dloc->DataLoc.Mapping)) return FALSE;
    if(!(f2->Dloc->DataLoc.Mapping)) return FALSE;
    if(f1->Dloc->DataLoc.Mapping[0].extLocation != f2->Dloc->DataLoc.Mapping[0].extLocation) return FALSE;
    if(f1->Dloc->DataLoc.Mapping[0].extLength != f2->Dloc->DataLoc.Mapping[0].extLength) return FALSE;
//    if(f1-> != f2->) return FALSE;
//    if(f1-> != f2->) return FALSE;
//    if(f1-> != f2->) return FALSE;
    if(!(f1->Dloc->FileEntry)) return FALSE;
    if(!(f2->Dloc->FileEntry)) return FALSE;
    if(RtlCompareMemory(f1->Dloc->FileEntry, f2->Dloc->FileEntry, f2->Dloc->FileEntryLen) != f2->Dloc->FileEntryLen)
        return FALSE;
    if(!(hDirIndex1 = f1->Dloc->DirIndex)) return FALSE;
    if(!(hDirIndex2 = f2->Dloc->DirIndex)) return FALSE;

    for(i=2; (DirIndex1 = UDFDirIndex(hDirIndex1,i)) &&
             (DirIndex2 = UDFDirIndex(hDirIndex2,i)); i++) {
        if( DirIndex1->FName.Buffer &&
           !DirIndex2->FName.Buffer)
            return FALSE;
        if( DirIndex2->FName.Buffer &&
           !DirIndex1->FName.Buffer)
            return FALSE;
        if(!DirIndex2->FName.Buffer &&
           !DirIndex1->FName.Buffer)
            continue;
        if(RtlCompareUnicodeString(&(DirIndex1->FName),
                                   &(DirIndex2->FName),FALSE)) {
            return FALSE;
        }
//        if(DirIndex1[i].FileEntry != DirIndex2[i].FileEntry)
//            return FALSE;
        if(RtlCompareMemory(&(DirIndex1->FileEntryLoc),
                            &(DirIndex2->FileEntryLoc), sizeof(lb_addr)) != sizeof(lb_addr))
            return FALSE;
    }

    return TRUE;
} // end UDFCompareFileInfo()

/*
    This routine computes 32-bit hash based on CRC-32 from SSH
 */

#pragma warning(push)               
#pragma warning(disable:4035)               // re-enable below

//#ifdef _X86_
#ifdef _MSC_VER
__declspec (naked)
#endif // _X86_
uint32 
__fastcall
crc32(
    IN uint8* s,  // ECX
    IN uint32 len // EDX
    )
{
//#ifdef _X86_
#ifdef _MSC_VER
//    uint32 _Size = len;

    __asm {
        push  ebx
        push  ecx
        push  edx
        push  esi

        xor   eax,eax
        mov   esi,ecx  // ESI <- s
        mov   ecx,edx  // ECX <- len

        jecxz EO_CRC

        lea   ebx,[crc32_tab]
        xor   edx,edx

CRC_loop:

        mov   dl,al
        xor   dl,[esi]
        shr   eax,8
        xor   eax,[dword ptr ebx+edx*4]
        inc   esi
        loop  CRC_loop

EO_CRC:

        pop   esi
        pop   edx
        pop   ecx
        pop   ebx

        ret
    }
#else  // NO X86 optimization , use generic C/C++
    uint32 i;
    uint32 crc32val = 0;
  
    for(i=0; i<len; i++, s++) {
        crc32val =
            crc32_tab[(crc32val ^ (*s)) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
#endif // _X86_
} // end crc32()

/*
    Calculate a 16-bit CRC checksum for unicode strings
    using ITU-T V.41 polynomial.

    The OSTA-UDF(tm) 1.50 standard states that using CRCs is mandatory.
    The polynomial used is: x^16 + x^12 + x^15 + 1
*/

//#ifdef _X86_
#ifdef _MSC_VER
__declspec (naked)
#endif // _X86_
uint16 
__fastcall
UDFUnicodeCksum(
    PWCHAR s, // ECX
    uint32 n  // EDX
    )
{
//#ifdef _X86_
#ifdef _MSC_VER
//    uint32 _Size = n;

    __asm {
        push  ebx
        push  ecx
        push  edx
        push  esi

        xor   eax,eax
        mov   esi,ecx
        mov   ecx,edx

        jecxz EO_uCRC

        lea   ebx,[CrcTable]
        xor   edx,edx

uCRC_loop:

        mov   dl,ah            // dl = (Crc >> 8)
        xor   dl,[esi+1]       // dl = ((Crc >> 8) ^ (*s >> 8)) & 0xff
        mov   ah,al            
        mov   al,dh            // ax = (Crc << 8)
        xor   ax,[word ptr ebx+edx*2]  // ax = ...........

        mov   dl,ah
        xor   dl,[esi]
        mov   ah,al
        mov   al,dh
        xor   ax,[word ptr ebx+edx*2]

        inc   esi
        inc   esi
        loop  uCRC_loop

EO_uCRC:

        pop   esi
        pop   edx
        pop   ecx
        pop   ebx
        
        ret
    }
#else  // NO X86 optimization , use generic C/C++
    uint16 Crc = 0;
    while (n--) {
        Crc = CrcTable[(Crc >> 8 ^ (*s >> 8)) & 0xff] ^ (Crc << 8);
        Crc = CrcTable[(Crc >> 8 ^ (*s++ & 0xff)) & 0xff] ^ (Crc << 8);
    }
    return Crc;

#endif // _X86_
} // end UDFUnicodeCksum()

//#ifdef _X86_
#ifdef _MSC_VER
__declspec (naked)
#endif // _X86_
uint16 
__fastcall
UDFUnicodeCksum150(
    PWCHAR s, // ECX
    uint32 n  // EDX
    )
{
//#ifdef _X86_
#ifdef _MSC_VER
//    uint32 _Size = n;

    __asm {
        push  ebx
        push  ecx
        push  edx
        push  esi
        push  edi

        xor   eax,eax
        mov   esi,ecx
        mov   ecx,edx
        xor   edi,edi

        jecxz EO_uCRC

        //lea   ebx,[CrcTable]
        xor   edx,edx
        xor   ebx,ebx

uCRC_loop:

        mov   dl,ah            // dl = (Crc >> 8)
        or    edi,edx          // if(*s & 0xff00) Use16 = TRUE;
        xor   dl,[esi+1]       // dl = ((Crc >> 8) ^ (*s >> 8)) & 0xff
        mov   ah,al            
        mov   al,0             // ax = (Crc << 8)
        xor   ax,[word ptr CrcTable+edx*2]  // ax = ...........

        mov   dl,ah
        xor   dl,[esi]
        mov   ah,al
        mov   al,0
        xor   ax,[word ptr CrcTable+edx*2]

        or    edi,edi          // if(!Use16) {
        jnz   use16_1

        rol   eax,16

        mov   bl,ah            // dl = (Crc >> 8)
        xor   bl,[esi]         // dl = ((Crc >> 8) ^ (*s >> 8)) & 0xff
        mov   ah,al            
        mov   al,0             // ax = (Crc << 8)
        xor   ax,[word ptr CrcTable+ebx*2]  // ax = ...........

        rol   eax,16
use16_1:
        inc   esi
        inc   esi
        loop  uCRC_loop

EO_uCRC:

        or    edi,edi          // if(!Use16) {
        jnz   use16_2

        rol   eax,16           // }
use16_2:
        and   eax,0xffff

        pop   edi
        pop   esi
        pop   edx
        pop   ecx
        pop   ebx
        
        ret
    }
#else  // NO X86 optimization , use generic C/C++
    uint16 Crc = 0;
    uint16 Crc2 = 0;
    BOOLEAN Use16 = FALSE;
    while (n--) {
        if(!Use16) {
            if((*s) & 0xff00) {
                Use16 = TRUE;
            } else {
                Crc2 = CrcTable[(Crc2 >> 8 ^ (*s >> 8)) & 0xff] ^ (Crc2 << 8);
            }
        }
        Crc = CrcTable[(Crc >> 8 ^ (*s >> 8)) & 0xff] ^ (Crc << 8);
        Crc = CrcTable[(Crc >> 8 ^ (*s++ & 0xff)) & 0xff] ^ (Crc << 8);
    }
    return Use16 ? Crc : Crc2;
#endif // _X86_
} // end UDFUnicodeCksum150()

/*
    Calculate a 16-bit CRC checksum using ITU-T V.41 polynomial.

    The OSTA-UDF(tm) 1.50 standard states that using CRCs is mandatory.
    The polynomial used is: x^16 + x^12 + x^15 + 1
*/
//#ifdef _X86_
#ifdef _MSC_VER
__declspec (naked)
#endif // _X86_
uint16 
__fastcall
UDFCrc(
    IN uint8* Data, // ECX
    IN uint32 Size  // EDX
    )
{
//#ifdef _X86_
#ifdef _MSC_VER
//    uint32 _Size = Size;

    __asm {
        push  ebx
        push  ecx
        push  edx
        push  esi

        mov   esi,ecx
        mov   ecx,edx
        xor   eax,eax

        jecxz EO_CRC

        lea   ebx,[CrcTable]
        xor   edx,edx

CRC_loop:

        mov   dl,ah
        xor   dl,[esi]
        mov   ah,al
        mov   al,dh
        xor   ax,[word ptr ebx+edx*2]
        inc   esi
        loop  CRC_loop

EO_CRC:

        pop   esi
        pop   edx
        pop   ecx
        pop   ebx
        
        ret
    }
#else  // NO X86 optimization , use generic C/C++
    uint16 Crc = 0;
    while (Size--)
        Crc = CrcTable[(Crc >> 8 ^ *Data++) & 0xff] ^ (Crc << 8);
    return Crc;
#endif // _X86_

} // end UDFCrc()

#pragma warning(pop)    // re-enable warning #4035

/*
    Read the first block of a tagged descriptor & check it.
*/
OSSTATUS
UDFReadTagged(
    PVCB Vcb,
    int8* Buf,
    uint32 Block, 
    uint32 Location, 
    uint16 *Ident
    )
{
    OSSTATUS RC;
    tag* PTag = (tag*)Buf;
//    icbtag* Icb = (icbtag*)(Buf+1);
    uint8 checksum;
    unsigned int i;
    uint32 ReadBytes;
    int8* tb;

    // Read the block
    if(Block == 0xFFFFFFFF)
        return NULL;

    _SEH2_TRY {
        RC = UDFReadSectors(Vcb, FALSE, Block, 1, FALSE, Buf, &ReadBytes);
        if(!OS_SUCCESS(RC)) {
            KdPrint(("UDF: Block=%x, Location=%x: read failed\n", Block, Location));
            try_return(RC);
        }

        *Ident = PTag->tagIdent;

        if(Location != PTag->tagLocation) {
            KdPrint(("UDF: location mismatch block %x, tag %x != %x\n",
                Block, PTag->tagLocation, Location));
            try_return(RC = STATUS_FILE_CORRUPT_ERROR);
        }
        
        /* Verify the tag checksum */
        checksum = 0;
        tb = Buf;
        for (i=0; i<sizeof(tag); i++, tb++)
            checksum += (uint8)((i!=4) ? (*tb) : 0);

        if(checksum != PTag->tagChecksum) {
            KdPrint(("UDF: tag checksum failed block %x\n", Block));
            try_return(RC = STATUS_CRC_ERROR);
        }

        // Verify the tag version
        if((PTag->descVersion != 2) &&
           (PTag->descVersion != 3)) {
            KdPrint(("UDF: Tag version 0x%04x != 0x0002 || 0x0003 block %x\n",
                (PTag->descVersion), Block));
            try_return(RC = STATUS_FILE_CORRUPT_ERROR);
        }

        // Verify the descriptor CRC
        if(((PTag->descCRCLength) + sizeof(tag) > Vcb->BlockSize) ||
           ((PTag->descCRC) == UDFCrc((uint8*)Buf + sizeof(tag), PTag->descCRCLength)) ||
           !(PTag->descCRC) ) {
    /*        KdPrint(("Tag ID: %x, ver %x\t", PTag->tagIdent, PTag->descVersion ));
            if((i == TID_FILE_ENTRY) ||
               (i == TID_EXTENDED_FILE_ENTRY)) {
                KdPrint(("StrategType: %x, ", Icb->strategyType ));
                KdPrint(("FileType: %x\t", Icb->fileType ));
            }
            KdPrint(("\n"));*/
            try_return(RC = STATUS_SUCCESS);
        }
        KdPrint(("UDF: Crc failure block %x: crc = %x, crclen = %x\n",
            Block, PTag->descCRC, PTag->descCRCLength));
        RC = STATUS_CRC_ERROR;

try_exit:    NOTHING;

    } _SEH2_FINALLY {
        ;
    } _SEH2_END

    return RC;
} // end UDFReadTagged()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine creates hard link for the file from DirInfo1
    to DirInfo2 & names it as fn
 */
OSSTATUS
UDFHardLinkFile__(
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
    OSSTATUS status;
    PDIR_INDEX_ITEM DirNdx1;
    PDIR_INDEX_ITEM DirNdx2;
    uint_di i;
    BOOLEAN Recovery = FALSE;
    BOOLEAN SameFE = FALSE;
    uint32 NTAttr = 0;

    // validate FileInfo
    ValidateFileInfo(DirInfo1);
    ValidateFileInfo(DirInfo2);
    ValidateFileInfo(FileInfo);

    if(UDFGetFileLinkCount(FileInfo) >= UDF_MAX_LINK_COUNT) {
        // too many links to file...
        return STATUS_TOO_MANY_LINKS;
    }

    i = 0;
    if(DirInfo1 == DirInfo2) {
        if(OS_SUCCESS(status = UDFFindFile(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &i)) &&
           (i==FileInfo->Index) ) {
            // case-only difference
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

    // PHASE 0
    // try to create new FileIdent & FileEntry in Dir2

HLinkRetry:
    if(!OS_SUCCESS(status = UDFCreateFile__(Vcb, IgnoreCase, fn, UDFGetFileEALength(FileInfo),
                    0, (FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY),
                    TRUE, DirInfo2, &FileInfo2))) {
        if(UDFCleanUpFile__(Vcb, FileInfo2) && FileInfo2)
            MyFreePool__(FileInfo2);
        if(status == STATUS_ACCESS_DENIED) {
            // try to recover >;->
            if((*Replace) && !Recovery) {
                Recovery = TRUE;
                status = UDFOpenFile__(Vcb, IgnoreCase, TRUE, fn, DirInfo2, &FileInfo2, NULL);
                if(OS_SUCCESS(status)) {
                    status = UDFDoesOSAllowFileToBeTargetForHLink__(FileInfo2);
                    if(!OS_SUCCESS(status)) {
                        UDFCloseFile__(Vcb, FileInfo2);
                        goto cleanup_and_abort_hlink;
                    }
                    if((FileInfo->Dloc == FileInfo2->Dloc)  &&
                       (FileInfo != FileInfo2)) {
                        SameFE = TRUE;
                        // 'status' is already STATUS_SUCCESS here
                    } else {
                        status = UDFUnlinkFile__(Vcb, FileInfo2, TRUE);
                    }
                    UDFCloseFile__(Vcb, FileInfo2);
                    if(UDFCleanUpFile__(Vcb, FileInfo2)) {
                        MyFreePool__(FileInfo2);
                        FileInfo2 = NULL;
                        if(SameFE)
                            return STATUS_SUCCESS;
                    } else {
                        // we get here if the FileInfo has associated
                        // system-specific Fcb
                        // Such fact means that not all system references
                        // has already gone (except Linked file case)
                        if(SameFE)
                            return STATUS_SUCCESS;
                        if(!OS_SUCCESS(status) ||
                           (UDFGetFileLinkCount(FileInfo) < 1))
                            status = STATUS_ACCESS_DENIED;
                    }
                    if(OS_SUCCESS(status)) goto HLinkRetry;
                }
cleanup_and_abort_hlink:
                if(FileInfo2 && UDFCleanUpFile__(Vcb, FileInfo2)) {
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
    if(FileInfo2->FileIdent)
        ((FidADImpUse*)&(FileInfo2->FileIdent->icb.impUse))->uniqueID = (uint32)UDFAssingNewFUID(Vcb);

    // PHASE 3
    // drop all unnecessary info from FileInfo2

    UDFFreeFESpace(Vcb, DirInfo2, &(FileInfo2->Dloc->FELoc));
    UDFRemoveDloc(Vcb, FileInfo2->Dloc);

    // PHASE 4
    // perform in-memory linkage (update driver's tree structures) and flush

    FileInfo2->Dloc = FileInfo->Dloc;
    UDFInsertLinkedFile(FileInfo2, FileInfo);

    UDFCloseFile__(Vcb, FileInfo2);
    if(UDFCleanUpFile__(Vcb, FileInfo2)) {
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
OSSTATUS
UDFCreateRootFile__(
    IN PVCB Vcb,
//    IN uint16 AllocMode, // short/long/ext/in-icb  // always in-ICB
    IN uint32 PartNum,
    IN uint32 ExtAttrSz,
    IN uint32 ImpUseLen,
    IN BOOLEAN Extended,
    OUT PUDF_FILE_INFO* _FileInfo
    )
{
    OSSTATUS status;
    LONG_AD FEicb;
    PUDF_FILE_INFO FileInfo;
    *_FileInfo = NULL;
    uint32 ReadBytes;

    FileInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_FINF_TAG);
    *_FileInfo = FileInfo;
    if(!FileInfo)
        return STATUS_INSUFFICIENT_RESOURCES;
    ImpUseLen = (ImpUseLen + 3) & ~((uint16)3);

    RtlZeroMemory(FileInfo, sizeof(UDF_FILE_INFO));
    // init horizontal links
    FileInfo->NextLinkedFile =
    FileInfo->PrevLinkedFile = FileInfo;
    // allocate space for FileEntry
    if(!OS_SUCCESS(status =
        UDFBuildFileEntry(Vcb, NULL, FileInfo, PartNum, ICB_FLAG_AD_IN_ICB, ExtAttrSz, Extended) ))
        return status;
    FEicb.extLength = Vcb->LBlockSize;
    FEicb.extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    FEicb.extLocation.partitionReferenceNum = (uint16)PartNum;
    RtlZeroMemory(&(FEicb.impUse), sizeof(FEicb.impUse));

    FileInfo->Dloc->DataLoc.Mapping = UDFExtentToMapping(&(FileInfo->Dloc->FELoc.Mapping[0]));
    if(!(FileInfo->Dloc->DataLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;
    FileInfo->Dloc->DataLoc.Length = 0;
    FileInfo->Dloc->DataLoc.Offset = FileInfo->Dloc->FileEntryLen;
    // init FileEntry
    UDFSetFileUID(Vcb, FileInfo);
    UDFSetFileSize(FileInfo, 0);
    UDFIncFileLinkCount(FileInfo); // increase to 1
    UDFUpdateCreateTime(Vcb, FileInfo);
    // zero sector for FileEntry
    FileInfo->Dloc->DataLoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
    FileInfo->Dloc->FELoc.Mapping[0].extLength &= UDF_EXTENT_LENGTH_MASK;
    status = UDFWriteData(Vcb, TRUE, ((int64)(FileInfo->Dloc->FELoc.Mapping[0].extLocation)) << Vcb->BlockSizeBits, Vcb->LBlockSize, FALSE, Vcb->ZBuffer, &ReadBytes);
    if(!OS_SUCCESS(status))
        return status;

    UDFReferenceFile__(FileInfo);
    UDFReleaseDloc(Vcb, FileInfo->Dloc);
    return STATUS_SUCCESS;
} // end UDFCreateRootFile__()

/*
    This routine tries to create StreamDirectory associated with given file
    Caller should use UDFCleanUpFile__ if returned status != STATUS_SUCCESS
 */
OSSTATUS
UDFCreateStreamDir__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
    OUT PUDF_FILE_INFO* _SDirInfo  // this is to be filled & doesn't contain
                                   // any pointers
    )
{
    OSSTATUS status;
    PUDF_FILE_INFO SDirInfo;
    uint16 Ident;

    *_SDirInfo = NULL;
    ValidateFileInfo(FileInfo);
    // check currently recorded UDF revision
    if(!UDFStreamsSupported(Vcb))
        return STATUS_INVALID_PARAMETER;
    // check if we are allowed to associate Stream Dir with this file
    if((FileInfo->ParentFile && UDFIsAStreamDir(FileInfo->ParentFile)) ||
        UDFHasAStreamDir(FileInfo))
        return STATUS_FILE_DELETED;
    // check if we have Deleted SDir
    if(FileInfo->Dloc->SDirInfo &&
       UDFIsSDirDeleted(FileInfo->Dloc->SDirInfo))
        return STATUS_ACCESS_DENIED;
    // check if this file has ExtendedFileEntry
    if((Ident = FileInfo->Dloc->FileEntry->tagIdent) != TID_EXTENDED_FILE_ENTRY) {
        if(!OS_SUCCESS(status = UDFConvertFEToExtended(Vcb, FileInfo)))
            return status;
    }

    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    // create stream directory file
    if(!OS_SUCCESS(status = UDFCreateRootFile__(Vcb, PartNum, 0,0,FALSE, &SDirInfo)))
        return status;
    // link objects
    SDirInfo->ParentFile = FileInfo;
    // record directory structure
    SDirInfo->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED | UDF_FE_FLAG_IS_SDIR);

    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    UDFIncFileLinkCount(FileInfo);
    FileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_HAS_SDIR;

    status = UDFRecordDirectory__(Vcb, SDirInfo);
    UDFDecDirCounter(Vcb);

    UDFInterlockedIncrement((PLONG)&(FileInfo->OpenCount));
    if(!OS_SUCCESS(status)) {
        UDFUnlinkFile__(Vcb, SDirInfo, TRUE);
        UDFCloseFile__(Vcb, SDirInfo);
        UDFCleanUpFile__(Vcb, SDirInfo);
        MyFreePool__(SDirInfo);
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength = 0;
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.partitionReferenceNum = 0;
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.logicalBlockNum = 0;
        return status;
    }
    *_SDirInfo = SDirInfo;
    // do some init
    ((PEXTENDED_FILE_ENTRY)(SDirInfo->Dloc->FileEntry))->icbTag.fileType = UDF_FILE_TYPE_STREAMDIR;
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength = Vcb->LBlockSize;
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.partitionReferenceNum = (uint16)PartNum;
    ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation.logicalBlockNum =
        UDFPhysLbaToPart(Vcb, PartNum, SDirInfo->Dloc->FELoc.Mapping[0].extLocation);
    ((PEXTENDED_FILE_ENTRY)(SDirInfo->Dloc->FileEntry))->uniqueID = 
        ((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->uniqueID;
    FileInfo->Dloc->FE_Flags |= (UDF_FE_FLAG_FE_MODIFIED | UDF_FE_FLAG_HAS_SDIR);
    // open & finalize linkage
    FileInfo->Dloc->SDirInfo = SDirInfo;
    return STATUS_SUCCESS;
} // end UDFCreateStreamDir__()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine opens Stream Directory associated with file specified
 */
OSSTATUS
UDFOpenStreamDir__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
    OUT PUDF_FILE_INFO* _SDirInfo  // this is to be filled & doesn't contain
                                   // any pointers
    )
{
    OSSTATUS status;
    PUDF_FILE_INFO SDirInfo;
    PUDF_FILE_INFO ParSDirInfo;
    uint16 Ident;

    *_SDirInfo = NULL;
    ValidateFileInfo(FileInfo);
    // check if this file has ExtendedFileEntry
    if((Ident = FileInfo->Dloc->FileEntry->tagIdent) != TID_EXTENDED_FILE_ENTRY) {
        return STATUS_NOT_FOUND;
    }
    if((SDirInfo = FileInfo->Dloc->SDirInfo)) {
        // it is already opened. Good...
    
        // check if we have Deleted SDir
        if(FileInfo->Dloc->SDirInfo &&
           UDFIsSDirDeleted(FileInfo->Dloc->SDirInfo))
            return STATUS_FILE_DELETED;
        // All right. Look for parallel SDir (if any)
        if(SDirInfo->ParentFile != FileInfo) {
            ParSDirInfo = UDFLocateParallelFI(FileInfo, 0, SDirInfo);
            BrutePoint();
            if(ParSDirInfo->ParentFile != FileInfo) {
                SDirInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_SDFINF_TAG);
                *_SDirInfo = SDirInfo;
                if(!SDirInfo) return STATUS_INSUFFICIENT_RESOURCES;
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
    if(!((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLength)
        return STATUS_NOT_FOUND;
    SDirInfo = (PUDF_FILE_INFO)MyAllocatePoolTag__(UDF_FILE_INFO_MT,sizeof(UDF_FILE_INFO), MEM_SDFINF_TAG);
    if(!SDirInfo) return STATUS_INSUFFICIENT_RESOURCES;
    *_SDirInfo = SDirInfo;
    status = UDFOpenRootFile__(Vcb, &(((PEXTENDED_FILE_ENTRY)(FileInfo->Dloc->FileEntry))->streamDirectoryICB.extLocation) ,SDirInfo);
    if(!OS_SUCCESS(status)) return status;
    // open & finalize linkage
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_HAS_SDIR;
    SDirInfo->Dloc->FE_Flags |= UDF_FE_FLAG_IS_SDIR;
    FileInfo->Dloc->SDirInfo = SDirInfo;
    SDirInfo->ParentFile = FileInfo;

    UDFInterlockedIncrement((PLONG)&(FileInfo->OpenCount));

    return STATUS_SUCCESS;
} // end UDFOpenStreamDir__()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine records VAT & VAT Icb at the end of session
 */
OSSTATUS
UDFRecordVAT(
    IN PVCB Vcb
    )
{
    uint32 Offset;
    uint32 to_read;
    uint32 hdrOffset, hdrOffsetNew;
    uint32 hdrLen;
    OSSTATUS status;
    uint32 ReadBytes;
    uint32 len;
    uint16 PartNdx = (uint16)Vcb->VatPartNdx;
    uint16 PartNum = UDFGetPartNumByPartNdx(Vcb, PartNdx);
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
    uint32 BSh = Vcb->BlockSizeBits;
    uint32 MaxPacket = Vcb->WriteBlockSize >> BSh;
    uint32 OldLen;
    EntityID* eID;

    if(!(Vat = Vcb->Vat) || !VatFileInfo) return STATUS_INVALID_PARAMETER;
    // Disable VAT-based translation
    Vcb->Vat = NULL;
    // sync VAT and FSBM
    len = min(UDFPartLen(Vcb, PartNum), Vcb->FSBM_BitCount - root);
    len = min(Vcb->VatCount, len);
    for(i=0; i<len; i++) {
        if(UDFGetFreeBit(Vcb->FSBM_Bitmap, root+i))
            Vat[i] = UDF_VAT_FREE_ENTRY;
    }
    // Ok, now we shall construct new VAT image...
    // !!! NOTE !!!
    // Both VAT copies - in-memory & on-disc
    // contain _relative_ addresses
    OldLen = len = (uint32)UDFGetFileSize(Vcb->VatFileInfo);
    VatLen = (Vcb->LastLBA - root + 1) * sizeof(uint32);
    Old = (int8*)DbgAllocatePool(PagedPool, OldLen);
    if(!Old) {
        DbgFreePool(Vat);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // read old one
    status = UDFReadFile__(Vcb, VatFileInfo, 0, OldLen, FALSE, Old, &ReadBytes);
    if(!OS_SUCCESS(status)) {
        DbgFreePool(Vat);
        DbgFreePool(Old);
        return status;
    }
    // prepare some pointers
    // and fill headers
    if(Vcb->Partitions[PartNdx].PartitionType == UDF_VIRTUAL_MAP15) {
        Offset = 0;
        to_read =
        hdrOffset = len - sizeof(VirtualAllocationTable15);
        hdrLen = sizeof(VirtualAllocationTable15);
        hdrOffsetNew = VatLen;
        New = (int8*)DbgAllocatePool(PagedPool, VatLen + hdrLen);
        if(!New) {
            DbgFreePool(Vat);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(New+hdrOffsetNew, Old+hdrOffset, hdrLen);
        ((VirtualAllocationTable15*)(New + hdrOffset))->previousVATICB =
            VatFileInfo->Dloc->FELoc.Mapping[0].extLocation - root;
        eID = &(((VirtualAllocationTable15*)(New + hdrOffset))->ident);

        UDFSetEntityID_imp(eID, UDF_ID_ALLOC);

/*        RtlCopyMemory((int8*)&(eID->ident), UDF_ID_ALLOC, sizeof(UDF_ID_ALLOC) );
        iis = (impIdentSuffix*)&(eID->identSuffix);
        iis->OSClass = UDF_OS_CLASS_WINNT;
        iis->OSIdent = UDF_OS_ID_WINNT;*/
    } else {
        VirtualAllocationTable20* Buf;

        Offset = ((VirtualAllocationTable20*)Old)->lengthHeader;
        to_read = len - Offset;
        hdrOffset = 0;
        hdrLen = sizeof(VirtualAllocationTable20);
        hdrOffsetNew = 0;
        New = (int8*)DbgAllocatePool(PagedPool, VatLen + hdrLen);
        if(!New) {
            DbgFreePool(Vat);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(New+hdrOffsetNew, Old+hdrOffset, hdrLen);
        ((VirtualAllocationTable20*)New)->previousVatICBLoc =
            VatFileInfo->Dloc->FELoc.Mapping[0].extLocation - root;

        Buf = (VirtualAllocationTable20*)New;

        Buf->minReadRevision  = Vcb->minUDFReadRev;
        Buf->minWriteRevision = Vcb->minUDFWriteRev;
        Buf->maxWriteRevision = Vcb->maxUDFWriteRev;

        Buf->numFIDSFiles       = Vcb->numFiles;
        Buf->numFIDSDirectories = Vcb->numDirs;
    }

    RtlCopyMemory(New+Offset, Vat, VatLen);
    //
    if(VatFileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) {
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
    if(to_read < VatLen) {
        status = UDFResizeFile__(Vcb, VatFileInfo, len = hdrLen + VatLen);
        if(!OS_SUCCESS(status)) {
            return status;
        }
        UDFMarkSpaceAsXXX(Vcb, VatFileInfo->Dloc, VatFileInfo->Dloc->DataLoc.Mapping, AS_DISCARDED); //free
    }
    PacketOffset = WCacheGetWriteBlockCount__(&(Vcb->FastCache));
    if( ((((PFILE_ENTRY)(VatFileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) ) {
        // now we'll place FE & built-in data to the last sector of
        // the last packet will be recorded
        if(!PacketOffset) {
            // add padding
            UDFWriteData(Vcb, TRUE, ((uint64)Vcb->NWA) << Vcb->BlockSizeBits, 1, FALSE, Old, &ReadBytes);
            PacketOffset++;
        } else {
            Vcb->Vat = (uint32*)(New+Offset);
            WCacheSyncReloc__(&(Vcb->FastCache), Vcb);
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
        if(OS_SUCCESS(status = UDFWriteFile__(Vcb, VatFileInfo, 0, VatLen + hdrLen, FALSE, New, &ReadBytes))) {
            status = UDFFlushFile__(Vcb, VatFileInfo);
        }
        return status;
    }
    // We can't fit the whole VAT in FE tail
    // Now lets 'unpack' VAT's mapping to make updating easier
    status = UDFUnPackMapping(Vcb, &(VatFileInfo->Dloc->DataLoc));
    if(!OS_SUCCESS(status)) return status;
    // update VAT with locations of not flushed blocks
    if(PacketOffset) {
        Vcb->Vat = (uint32*)(New+Offset);
        WCacheSyncReloc__(&(Vcb->FastCache), Vcb);
        Vcb->Vat = NULL;
    }

    Mapping = VatFileInfo->Dloc->DataLoc.Mapping;
    off=0;
    BS = Vcb->BlockSize;
    NWA = Vcb->NWA;
    VatLen += hdrLen;
    // record modified parts of VAT & update mapping
    for(i=0; Mapping[i].extLength; i++) {
        to_read = (VatLen>=BS) ? BS : VatLen;
        if((OldLen < off) || (RtlCompareMemory(Old+off, New+off, to_read) != to_read)) {
            // relocate frag
            Mapping[i].extLocation = NWA+PacketOffset;
            Mapping[i].extLength &= UDF_EXTENT_LENGTH_MASK;
            PacketOffset++;
            if(PacketOffset >= MaxPacket) {
                NWA += (MaxPacket + 7);
                PacketOffset = 0;
            }
            status = UDFWriteFile__(Vcb, VatFileInfo, off, to_read, FALSE, New+off, &ReadBytes);
            if(!OS_SUCCESS(status)) {
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
    if(VatFileInfo->Dloc->AllocLoc.Mapping) MyFreePool__(VatFileInfo->Dloc->AllocLoc.Mapping);
    VatFileInfo->Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , (len+1)*sizeof(EXTENT_MAP),
                                                       MEM_EXTMAP_TAG);
    if(!(VatFileInfo->Dloc->AllocLoc.Mapping)) return STATUS_INSUFFICIENT_RESOURCES;

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
        if(PacketOffset >= MaxPacket) {
            NWA += (MaxPacket + 7);
            PacketOffset = 0;
        }
    }
    // Terminator
    Mapping[i].extLocation =
    Mapping[i].extLength = 0;

    if( !PacketOffset &&
        (VatFileInfo->Dloc->AllocLoc.Length <= (Vcb->BlockSize - (uint32)(VatFileInfo->Dloc->AllocLoc.Offset)) ) ) {
        // add padding
        UDFWriteData(Vcb, TRUE, ((uint64)NWA) << Vcb->BlockSizeBits, 1, FALSE, Old, &ReadBytes);
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

    status = UDFFlushFile__(Vcb, VatFileInfo);
    if(!OS_SUCCESS(status))
        return status;
    WCacheFlushAll__(&(Vcb->FastCache), Vcb);
    return STATUS_SUCCESS;
} // end UDFRecordVAT()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine updates VAT according to RequestedLbaTable (RelocTab) &
    actual physical address where this data will be stored
 */
OSSTATUS
UDFUpdateVAT(
    IN void* _Vcb,
    IN uint32 Lba,
    IN uint32* RelocTab,  // can be NULL
    IN uint32 BCount
    )
{
#ifndef UDF_READ_ONLY_BUILD
    PVCB Vcb = (PVCB)_Vcb;
    uint16 PartNdx = (uint16)(Vcb->VatPartNdx);
    uint16 PartNum = (uint16)(Lba ? UDFGetPartNumByPhysLba(Vcb, Lba) : UDFGetPartNumByPartNdx(Vcb, PartNdx));
    if(PartNum != UDFGetPartNumByPartNdx(Vcb, PartNdx)) {
        KdPrint(("UDFUpdateVAT: Write to Write-Protected partition\n"));
        return STATUS_MEDIA_WRITE_PROTECTED;
    }
    // !!! NOTE !!!
    // Both VAT copies - in-memory & on-disc
    // contain _relative_ addresses
    uint32 root = Vcb->Partitions[PartNdx].PartitionRoot;
    uint32 NWA = Vcb->NWA-root;
    uint32 i;
    uint32 CurLba;

    if(!Vcb->Vat) return STATUS_SUCCESS;

    for(i=0; i<BCount; i++, NWA++) {
        if((CurLba = (RelocTab ? RelocTab[i] : (Lba+i)) - root) >= Vcb->VatCount)
            Vcb->VatCount = CurLba+1;
        Vcb->Vat[CurLba] = NWA;
    }
    return STATUS_SUCCESS;
#else //UDF_READ_ONLY_BUILD
    return STATUS_MEDIA_WRITE_PROTECTED;
#endif //UDF_READ_ONLY_BUILD
} // end UDFUpdateVAT()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine rebuilds file's FE in order to move data from
    ICB to separate Block.
 */
OSSTATUS
UDFConvertFEToNonInICB(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo,
    IN uint8 NewAllocMode
    )
{
    OSSTATUS status;
    int8* OldInIcb = NULL;
    uint32 OldLen;
    ValidateFileInfo(FileInfo);
    uint32 ReadBytes;
    uint32 _WrittenBytes;
    PUDF_DATALOC_INFO Dloc;

//    ASSERT(FileInfo->RefCount >= 1);

    Dloc = FileInfo->Dloc;
    ASSERT(Dloc->FELoc.Mapping[0].extLocation);
    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, Dloc->FELoc.Mapping[0].extLocation);

    if(NewAllocMode == ICB_FLAG_AD_DEFAULT_ALLOC_MODE) {
        NewAllocMode = (uint8)(Vcb->DefaultAllocMode);
    }
    // we do not support recording of extended AD now
    if(NewAllocMode != ICB_FLAG_AD_SHORT &&
       NewAllocMode != ICB_FLAG_AD_LONG)
        return STATUS_INVALID_PARAMETER;
    if(!Dloc->DataLoc.Offset || !Dloc->DataLoc.Length)
        return STATUS_SUCCESS;
    ASSERT(!Dloc->AllocLoc.Mapping);
    // read in-icb data. it'll be replaced after resize
    OldInIcb = (int8*)MyAllocatePool__(NonPagedPool, (uint32)(Dloc->DataLoc.Length));
    if(!OldInIcb)
        return STATUS_INSUFFICIENT_RESOURCES;
    OldLen = (uint32)(Dloc->DataLoc.Length);
    status = UDFReadExtent(Vcb, &(Dloc->DataLoc), 0, OldLen, FALSE, OldInIcb, &ReadBytes);
    if(!OS_SUCCESS(status)) {
        MyFreePool__(OldInIcb);
        return status;
    }
/*    if(!Dloc->AllocLoc.Mapping) {
        Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, sizeof(EXTENT_MAP)*2);
        if(!Dloc->AllocLoc.Mapping) {
            MyFreePool__(OldInIcb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    // init Alloc mode
    if((((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |= Vcb->DefaultAllocMode;
    } else {
        BrutePoint();
    }
    RtlZeroMemory(Dloc->AllocLoc.Mapping, sizeof(EXTENT_MAP)*2);
//    Dloc->AllocLoc.Mapping[0].extLocation = 0;
    Dloc->AllocLoc.Mapping[0].extLength   = Vcb->LBlockSize | EXTENT_NOT_RECORDED_NOT_ALLOCATED;
//    Dloc->AllocLoc.Mapping[1].extLocation = 0;
//    Dloc->AllocLoc.Mapping[1].extLength = 0;
*/

    // grow extent in order to force space allocation
    status = UDFResizeExtent(Vcb, PartNum, Vcb->LBlockSize, FALSE, &Dloc->DataLoc);
    if(!OS_SUCCESS(status)) {
        MyFreePool__(OldInIcb);
        return status;
    }

    // set Alloc mode
    if((((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags &= ~ICB_FLAG_ALLOC_MASK;
        ((PFILE_ENTRY)(Dloc->FileEntry))->icbTag.flags |= NewAllocMode;
    } else {
        BrutePoint();
    }

    // revert to initial extent size. This will not cause NonInICB->InICB transform
    status = UDFResizeExtent(Vcb, PartNum, OldLen, FALSE, &Dloc->DataLoc);
    if(!OS_SUCCESS(status)) {
        MyFreePool__(OldInIcb);
        return status;
    }

    // replace data from ICB (if any) & free buffer
    status = UDFWriteExtent(Vcb, &(Dloc->DataLoc), 0, OldLen, FALSE, OldInIcb, &_WrittenBytes);
    MyFreePool__(OldInIcb);
    if(!OS_SUCCESS(status)) {
        return status;
    }
    // inform UdfInfo, that AllocDesc's must be rebuilt on flush/close
    Dloc->AllocLoc.Modified = TRUE;
    Dloc->DataLoc.Modified = TRUE;
    return STATUS_SUCCESS;
} // end UDFConvertFEToNonInICB()

/*
    This routine converts file's FE to extended form.
    It is needed for stream creation.
 */
OSSTATUS
UDFConvertFEToExtended(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    PEXTENDED_FILE_ENTRY ExFileEntry;
    PFILE_ENTRY FileEntry;
    uint32 Length, NewLength, l;
    OSSTATUS status;
    uint32 ReadBytes;

    if(!FileInfo) return STATUS_INVALID_PARAMETER;
    ValidateFileInfo(FileInfo);
    if(FileInfo->Dloc->FileEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) return STATUS_SUCCESS;
    if(FileInfo->Dloc->FileEntry->tagIdent != TID_FILE_ENTRY) return STATUS_INVALID_PARAMETER;

/*    if(!OS_SUCCESS(status = UDFFlushFile__(Vcb, FileInfo)))
        return status;*/

    Length = FileInfo->Dloc->FileEntryLen;
    NewLength = Length - sizeof(FILE_ENTRY) + sizeof(EXTENDED_FILE_ENTRY);
    ExFileEntry = (PEXTENDED_FILE_ENTRY)MyAllocatePoolTag__(NonPagedPool, NewLength, MEM_XFE_TAG);
    if(!ExFileEntry) return STATUS_INSUFFICIENT_RESOURCES;
    FileEntry = (PFILE_ENTRY)(FileInfo->Dloc->FileEntry);
    RtlZeroMemory(ExFileEntry, NewLength);

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
    RtlCopyMemory((int8*)(ExFileEntry+1)+FileEntry->lengthExtendedAttr, (int8*)(ExFileEntry+1)+FileEntry->lengthExtendedAttr, FileEntry->lengthAllocDescs);

    if((((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB) {

        if((l = (uint32)(FileInfo->Dloc->DataLoc.Length))) {

            int8* tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, l);
            if(!tmp_buff) {
                MyFreePool__(ExFileEntry);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            if(!OS_SUCCESS(status = UDFReadFile__(Vcb, FileInfo, 0, l, FALSE, tmp_buff, &ReadBytes)) ||
               !OS_SUCCESS(status = UDFResizeFile__(Vcb, FileInfo, 0)) ) {
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
            if(!OS_SUCCESS(status = UDFResizeFile__(Vcb, FileInfo, l)) ||
               !OS_SUCCESS(status = UDFWriteFile__(Vcb, FileInfo, 0, l, FALSE, tmp_buff, &ReadBytes)) ) {
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
    if(Vcb->minUDFReadRev < 0x0200)
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
OSSTATUS
UDFPretendFileDeleted__(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO FileInfo
    )
{
    AdPrint(("UDFPretendFileDeleted__:\n"));

    NTSTATUS RC;
    PDIR_INDEX_HDR hDirNdx = UDFGetDirIndexByFileInfo(FileInfo);
    if(!hDirNdx) return STATUS_CANNOT_DELETE;
    PDIR_INDEX_ITEM DirNdx = UDFDirIndex(hDirNdx, FileInfo->Index);
    if(!DirNdx) return STATUS_CANNOT_DELETE;


    // we can't hide file that is not marked as deleted
    RC = UDFDoesOSAllowFilePretendDeleted__(FileInfo);
    if(!NT_SUCCESS(RC))
        return RC;

    AdPrint(("UDFPretendFileDeleted__: set UDF_FI_FLAG_FI_INTERNAL\n"));

    DirNdx->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
    if(DirNdx->FName.Buffer) {
        MyFreePool__(DirNdx->FName.Buffer);
        DirNdx->FName.Buffer = NULL;
        DirNdx->FName.Length =
        DirNdx->FName.MaximumLength = 0;
    }
    return STATUS_SUCCESS;
} // end UDFPretendFileDeleted__()
#endif //UDF_READ_ONLY_BUILD
