////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
        Module name:

   extent.cpp

        Abstract:

   This file contains filesystem-specific routines
   responsible for extent & mapping management

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_EXTENT

/*
    This routine converts offset in extent to Lba & returns offset in the 1st
    sector & bytes before end of block.
    Here we assume no references to AllocDescs
 */
uint32
UDFExtentOffsetToLba(
    IN PVCB Vcb,
    IN PEXTENT_MAP Extent,   // Extent array
    IN int64 Offset,      // offset in extent
    OUT uint32* SectorOffset,
    OUT uint32* AvailLength,  // available data in this block
    OUT uint32* Flags,
    OUT uint32* Index
    )
{
    uint32 j=0, l, d, BSh = Vcb->BlockSizeBits;
    uint32 Offs;
    uint32 i=0, BOffset; // block nums

    BOffset = (uint32)(Offset >> BSh);
    // scan extent table for suitable range (frag)
    ExtPrint(("ExtLen %x\n", Extent->extLength));
    while(i+(d = (l = (Extent->extLength & UDF_EXTENT_LENGTH_MASK)) >> BSh) <= BOffset) {

        if(!l) {
            if(Index) (*Index) = j-1;
            if(Flags) {
                Extent--;
                (*Flags) = (Extent->extLength >> 30);
            }
            return LBA_OUT_OF_EXTENT;
        }
        if(!d)
            break;
        i += d; //frag offset
        j++; // frag index
        Extent++;
    }
    BOffset -= i;
    Offs = (*((uint32*)&Offset)) - (i << BSh); // offset in frag
    
    if(SectorOffset)
        (*SectorOffset) = Offs & (Vcb->BlockSize-1);// offset in 1st Lba
    if(AvailLength)
        (*AvailLength) = l - Offs;// bytes to EO frag
    if(Flags)
        (*Flags) = (Extent->extLength >> 30);
    if(Index)
        (*Index) = j;

    ASSERT(((Extent->extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) || Extent->extLocation);

    return Extent->extLocation + BOffset;// 1st Lba
} // end UDFExtentOffsetToLba()

uint32
UDFNextExtentToLba(
    IN PVCB Vcb,
    IN PEXTENT_MAP Extent,   // Extent array
    OUT uint32* AvailLength,  // available data in this block
    OUT uint32* Flags,
    OUT uint32* Index
    )
{
//    uint32 Lba;

    uint32 l, d;

    // scan extent table for suitable range (frag)
    d = (l = (Extent->extLength & UDF_EXTENT_LENGTH_MASK));

    if(!l) {
        (*Index) = -1;
        Extent--;
        (*Flags) = (Extent->extLength >> 30);
        return LBA_OUT_OF_EXTENT;
    }

    (*Index) = 0;
    (*AvailLength) = l;// bytes to EO frag
    (*Flags) = (Extent->extLength >> 30);

    ASSERT(((*Flags) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) || Extent->extLocation);

    return Extent->extLocation;// 1st Lba
} // end UDFNextExtentToLba()

/*
    This routine locates frag containing specified Lba in extent
 */
ULONG
UDFLocateLbaInExtent(
    IN PVCB Vcb,
    IN PEXTENT_MAP Extent,   // Extent array
    IN lba_t lba
    )
{
    uint32 l, BSh = Vcb->BlockSizeBits;
    uint32 i=0;

    while(l = ((Extent->extLength & UDF_EXTENT_LENGTH_MASK) >> BSh)) {

        if(Extent->extLocation   >= lba &&
           Extent->extLocation+l <  lba) {
            return i;
        }
        i++; //frag offset
        Extent++;
    }
    return LBA_OUT_OF_EXTENT;// index of item in extent, containing out Lba
} // end UDFLocateLbaInExtent()

/*
    This routine calculates total length of specified extent.
    Here we assume no references to AllocDescs
 */
int64
UDFGetExtentLength(
    IN PEXTENT_MAP Extent   // Extent array
    )
{
    if(!Extent) return 0;
    int64 i=0;

//#ifdef _X86_
#ifdef _MSC_VER

    __asm push  ebx
    __asm push  ecx
    __asm push  esi

    __asm lea   ebx,i
    __asm mov   esi,Extent
    __asm xor   ecx,ecx
While_1:
    __asm mov   eax,[esi+ecx*8]  // Extent[j].extLength
    __asm and   eax,UDF_EXTENT_LENGTH_MASK
    __asm jz    EO_While
    __asm add   [ebx],eax
    __asm adc   [ebx+4],0
    __asm inc   ecx
    __asm jmp   While_1
EO_While:;
    __asm pop   esi
    __asm pop   ecx
    __asm pop   ebx

#else   // NO X86 optimization , use generic C/C++

    while(Extent->extLength) {
        i += (Extent->extLength & UDF_EXTENT_LENGTH_MASK);
        Extent++;
    }

#endif // _X86_

    return i;
} // UDFGetExtentLength()

/*
    This routine appends Zero-terminator to single Extent-entry.
    Such operation makes it compatible with other internal routines
 */
PEXTENT_MAP
__fastcall
UDFExtentToMapping_(
    IN PEXTENT_AD Extent
#ifdef UDF_TRACK_EXTENT_TO_MAPPING
   ,IN ULONG src,
    IN ULONG line
#endif //UDF_TRACK_EXTENT_TO_MAPPING
    )
{
    PEXTENT_MAP Map;

#ifdef UDF_TRACK_EXTENT_TO_MAPPING
#define UDF_EXT_MAP_MULT 4
#else //UDF_TRACK_EXTENT_TO_MAPPING
#define UDF_EXT_MAP_MULT 2
#endif //UDF_TRACK_EXTENT_TO_MAPPING

    Map = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , UDF_EXT_MAP_MULT *
                                                       sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
    if(!Map) return NULL;
    RtlZeroMemory((int8*)(Map+1), sizeof(EXTENT_MAP));
    Map[0].extLength = Extent->extLength;
    Map[0].extLocation = Extent->extLocation;
#ifdef UDF_TRACK_EXTENT_TO_MAPPING
    Map[2].extLength = src;
    Map[2].extLocation = line;
#endif //UDF_TRACK_EXTENT_TO_MAPPING
    return Map;
} // end UDFExtentToMapping()

/*
    This routine calculates file mapping length (in bytes) including
    ZERO-terminator
 */
uint32
UDFGetMappingLength(
    IN PEXTENT_MAP Extent
    )
{
    if(!Extent) return 0;
    uint32 i=0;

//#ifdef _X86_
#ifdef _MSC_VER
    __asm push  ebx

    __asm mov   ebx,Extent
    __asm xor   eax,eax
While_1:
    __asm mov   ecx,[ebx+eax*8]
    __asm jecxz EO_While
    __asm inc   eax
    __asm jmp   While_1
EO_While:
    __asm inc   eax
    __asm shl   eax,3
    __asm mov   i,eax

    __asm pop   ebx

#else   // NO X86 optimization , use generic C/C++

    while(Extent->extLength) {
        i++;
        Extent++;
    }
    i++;
    i*=sizeof(EXTENT_MAP);

#endif // _X86_

    return i; //  i*sizeof(EXTENT_MAP)
} // end UDFGetMappingLength()

/*
    This routine merges 2 sequencial file mappings
 */
PEXTENT_MAP
__fastcall
UDFMergeMappings(
    IN PEXTENT_MAP Extent,
    IN PEXTENT_MAP Extent2
    )
{
    PEXTENT_MAP NewExt;
    uint32 len, len2;

    len = UDFGetMappingLength(Extent);
    len2 = UDFGetMappingLength(Extent2);
    ASSERT(len2 && len);
    if(!len2) {
        return Extent;
    }
    if(MyReallocPool__((int8*)Extent, len, (int8**)(&NewExt), len+len2-sizeof(EXTENT_MAP))) {
        RtlCopyMemory(((int8*)NewExt)+len-sizeof(EXTENT_MAP), (int8*)Extent2, len2);
    } else {
        ExtPrint(("UDFMergeMappings failed\n"));
        BrutePoint();
    }
    return NewExt;
} // end UDFMergeMappings()

/*
    This routine builds file mapping according to ShortAllocDesc (SHORT_AD)
    array
 */
PEXTENT_MAP
UDFShortAllocDescToMapping(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN PSHORT_AD AllocDesc,
    IN uint32 AllocDescLength,
    IN uint32 SubCallCount,
    OUT PEXTENT_INFO AllocLoc
    )
{
    uint32 i, lim, l, len, BSh, type;
    PEXTENT_MAP Extent, Extent2, AllocMap;
    EXTENT_AD AllocExt;
    PALLOC_EXT_DESC NextAllocDesc;
    lb_addr locAddr;
    uint32 ReadBytes;
    EXTENT_INFO NextAllocLoc;
    BOOLEAN w2k_compat = FALSE;

    ExtPrint(("UDFShortAllocDescToMapping: len=%x\n", AllocDescLength));

    if(SubCallCount > ALLOC_DESC_MAX_RECURSE) return NULL;

    locAddr.partitionReferenceNum = (uint16)PartNum;
    BSh = Vcb->BlockSizeBits;
    l = ((lim = (AllocDescLength/sizeof(SHORT_AD))) + 1 ) * sizeof(EXTENT_AD);
    Extent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool, l, MEM_EXTMAP_TAG);
    if(!Extent) return NULL;

    NextAllocLoc.Offset = 0;

    for(i=0;i<lim;i++) {
        type = AllocDesc[i].extLength >> 30;
        len  = AllocDesc[i].extLength & UDF_EXTENT_LENGTH_MASK;
        ExtPrint(("ShExt: type %x, loc %x, len %x\n", type, AllocDesc[i].extPosition, len));
        if(type == EXTENT_NEXT_EXTENT_ALLOCDESC) {
            // read next frag of allocation descriptors if encountered
            if(len < sizeof(ALLOC_EXT_DESC)) {
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocDesc = (PALLOC_EXT_DESC)MyAllocatePoolTag__(NonPagedPool, len, MEM_ALLOCDESC_TAG);
            if(!NextAllocDesc) {
                MyFreePool__(Extent);
                return NULL;
            }
            // record information about this frag
            locAddr.logicalBlockNum = AllocDesc[i].extPosition;
            AllocExt.extLength = len;
            AllocExt.extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if(AllocExt.extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address\n"));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocLoc.Mapping =
            AllocMap = UDFExtentToMapping(&AllocExt);
            NextAllocLoc.Length = len;
            if(!AllocMap) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            AllocLoc->Mapping = UDFMergeMappings(AllocLoc->Mapping, AllocMap);
            if(!AllocLoc->Mapping ||
            // read this frag
               !OS_SUCCESS(UDFReadExtent(Vcb, &NextAllocLoc,
                                0, len, FALSE, (int8*)NextAllocDesc, &ReadBytes)))
            {
                MyFreePool__(AllocMap);
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            MyFreePool__(AllocMap);
            // check integrity
            if((NextAllocDesc->descTag.tagIdent != TID_ALLOC_EXTENT_DESC) ||
               (NextAllocDesc->lengthAllocDescs > (len - sizeof(ALLOC_EXT_DESC))) ) {
                KdPrint(("Integrity check failed\n"));
                KdPrint(("NextAllocDesc->descTag.tagIdent = %x\n", NextAllocDesc->descTag.tagIdent));
                KdPrint(("NextAllocDesc->lengthAllocDescs = %x\n", NextAllocDesc->lengthAllocDescs));
                KdPrint(("len = %x\n", len));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            // perform recursive call to obtain mapping
            NextAllocLoc.Flags = 0;
            Extent2 = UDFShortAllocDescToMapping(Vcb, PartNum, (PSHORT_AD)(NextAllocDesc+1),
                                      NextAllocDesc->lengthAllocDescs, SubCallCount+1, AllocLoc);
            if(!Extent2) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            UDFCheckSpaceAllocation(Vcb, 0, Extent2, AS_USED); // check if used
            // and merge this 2 mappings into 1
            Extent[i].extLength = 0;
            Extent[i].extLocation = 0;
            Extent = UDFMergeMappings(Extent, Extent2);

            if(NextAllocLoc.Flags & EXTENT_FLAG_2K_COMPAT) {
                ExtPrint(("w2k-compat\n"));
                AllocLoc->Flags |= EXTENT_FLAG_2K_COMPAT;
            }

            MyFreePool__(Extent2);
            return Extent;
        }
        //
#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(len & (Vcb->LBlockSize-1) ));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        if(len & (Vcb->LBlockSize-1)) {
            w2k_compat = TRUE;
        }
        Extent[i].extLength = (len+Vcb->LBlockSize-1) & ~(Vcb->LBlockSize-1);
        locAddr.logicalBlockNum = AllocDesc[i].extPosition;
        // Note: for compatibility Adaptec DirectCD we check 'len' here
        //       That strange implementation records bogus extLocation in terminal entries
        if(type != EXTENT_NOT_RECORDED_NOT_ALLOCATED && len) {
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if(Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address (2)\n"));
                MyFreePool__(Extent);
                return NULL;
            }
        } else {
            Extent[i].extLocation = 0;
        }
        if(!len) {
            // some UDF implementations set strange AllocDesc sequence length,
            // but terminates it with zeros in proper place, so handle
            // this case
            ASSERT(i>=(lim-1));
            ASSERT(!Extent[i].extLength);
            Extent[i].extLocation = 0;
            if(/*!SubCallCount &&*/ w2k_compat) {
                ExtPrint(("w2k-compat\n"));
                AllocLoc->Flags |= EXTENT_FLAG_2K_COMPAT;
            }
            return Extent;
        }
        Extent[i].extLength |= (type << 30);
    }
    // set terminator
    Extent[i].extLength = 0;
    Extent[i].extLocation = 0;

    if(/*!SubCallCount &&*/ w2k_compat) {
        ExtPrint(("w2k-compat\n"));
        AllocLoc->Flags |= EXTENT_FLAG_2K_COMPAT;
    }

    return Extent;
} // end UDFShortAllocDescToMapping()

/*
    This routine builds file mapping according to LongAllocDesc (LONG_AD)
    array
 */
PEXTENT_MAP
UDFLongAllocDescToMapping(
    IN PVCB Vcb,
    IN PLONG_AD AllocDesc,
    IN uint32 AllocDescLength,
    IN uint32 SubCallCount,
    OUT PEXTENT_INFO AllocLoc // .Mapping must be intialized (non-Zero)
    )
{
    uint32 i, lim, l, len, BSh, type;
    PEXTENT_MAP Extent, Extent2, AllocMap;
    EXTENT_AD AllocExt;
    PALLOC_EXT_DESC NextAllocDesc;
    uint32 ReadBytes;
    EXTENT_INFO NextAllocLoc;

    ExtPrint(("UDFLongAllocDescToMapping: len=%x\n", AllocDescLength));

    if(SubCallCount > ALLOC_DESC_MAX_RECURSE) return NULL;

    BSh = Vcb->BlockSizeBits;
    l = ((lim = (AllocDescLength/sizeof(LONG_AD))) + 1 ) * sizeof(EXTENT_AD);
    Extent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool, l, MEM_EXTMAP_TAG);
    if(!Extent) return NULL;

    NextAllocLoc.Offset = 0;

    for(i=0;i<lim;i++) {
        type = AllocDesc[i].extLength >> 30;
        len  = AllocDesc[i].extLength & UDF_EXTENT_LENGTH_MASK;
        ExtPrint(("LnExt: type %x, loc %x (%x:%x), len %x\n", type, UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation)),
            AllocDesc[i].extLocation.partitionReferenceNum, AllocDesc[i].extLocation.logicalBlockNum,
            len));
        if(type == EXTENT_NEXT_EXTENT_ALLOCDESC) {
            // read next frag of allocation descriptors if encountered
            if(len < sizeof(ALLOC_EXT_DESC)) {
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocDesc = (PALLOC_EXT_DESC)MyAllocatePoolTag__(NonPagedPool, len, MEM_ALLOCDESC_TAG);
            if(!NextAllocDesc) {
                MyFreePool__(Extent);
                return NULL;
            }
            // record information about this frag
            AllocExt.extLength = len;
            AllocExt.extLocation = UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation));
            if(AllocExt.extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address\n"));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocLoc.Mapping =
            AllocMap = UDFExtentToMapping(&AllocExt);
            NextAllocLoc.Length = len;
            if(!AllocMap) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            AllocLoc->Mapping = UDFMergeMappings(AllocLoc->Mapping, AllocMap);
            if(!AllocLoc->Mapping ||
            // read this frag
               !OS_SUCCESS(UDFReadExtent(Vcb, &NextAllocLoc,
                                0, len, FALSE, (int8*)NextAllocDesc, &ReadBytes)))
            {
                MyFreePool__(AllocMap);
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            MyFreePool__(AllocMap);
            // check integrity
            if((NextAllocDesc->descTag.tagIdent != TID_ALLOC_EXTENT_DESC) ||
               (NextAllocDesc->lengthAllocDescs > (len - sizeof(ALLOC_EXT_DESC))) ) {
                KdPrint(("Integrity check failed\n"));
                KdPrint(("NextAllocDesc->descTag.tagIdent = %x\n", NextAllocDesc->descTag.tagIdent));
                KdPrint(("NextAllocDesc->lengthAllocDescs = %x\n", NextAllocDesc->lengthAllocDescs));
                KdPrint(("len = %x\n", len));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            // perform recursive call to obtain mapping
            Extent2 = UDFLongAllocDescToMapping(Vcb, (PLONG_AD)(NextAllocDesc+1),
                                      NextAllocDesc->lengthAllocDescs, SubCallCount+1, AllocLoc);
            if(!Extent2) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            // and merge this 2 mappings into 1
            Extent[i].extLength = 0;
            Extent[i].extLocation = 0;
            Extent = UDFMergeMappings(Extent, Extent2);
            MyFreePool__(Extent2);
            return Extent;
        }
        //
        Extent[i].extLength = len;
#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(len & (Vcb->LBlockSize-1) ));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        Extent[i].extLength = (len+Vcb->LBlockSize-1) & ~(Vcb->LBlockSize-1);
        // Note: for compatibility Adaptec DirectCD we check 'len' here
        //       That strange implementation records bogus extLocation in terminal entries
        if(type != EXTENT_NOT_RECORDED_NOT_ALLOCATED && len) {
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation));
            if(Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address (2)\n"));
                MyFreePool__(Extent);
                return NULL;
            }
        } else {
            Extent[i].extLocation = 0;
        }
        if(!len) {
            // some UDF implementations set strange AllocDesc sequence length,
            // but terminates it with zeros in proper place, so handle
            // this case
            Extent[i].extLocation = 0;
            return Extent;
        }
        Extent[i].extLength |= (type << 30);
    }
    // set terminator
    Extent[i].extLength = 0;
    Extent[i].extLocation = 0;

    return Extent;
} // end UDFLongAllocDescToMapping()

/*
    This routine builds file mapping according to ExtendedAllocDesc (EXT_AD)
    array
 */
PEXTENT_MAP
UDFExtAllocDescToMapping(
    IN PVCB Vcb,
    IN PEXT_AD AllocDesc,
    IN uint32 AllocDescLength,
    IN uint32 SubCallCount,
    OUT PEXTENT_INFO AllocLoc // .Mapping must be intialized (non-Zero)
    )
{
    uint32 i, lim, l, len, BSh, type;
    PEXTENT_MAP Extent, Extent2, AllocMap;
    EXTENT_AD AllocExt;
    PALLOC_EXT_DESC NextAllocDesc;
    uint32 ReadBytes;
    EXTENT_INFO NextAllocLoc;

    ExtPrint(("UDFExtAllocDescToMapping: len=%x\n", AllocDescLength));

    if(SubCallCount > ALLOC_DESC_MAX_RECURSE) return NULL;

    BSh = Vcb->BlockSizeBits;
    l = ((lim = (AllocDescLength/sizeof(EXT_AD))) + 1 ) * sizeof(EXTENT_AD);
    Extent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool, l, MEM_EXTMAP_TAG);
    if(!Extent) return NULL;

    NextAllocLoc.Offset = 0;

    for(i=0;i<lim;i++) {
        type = AllocDesc[i].extLength >> 30;
        len  = AllocDesc[i].extLength & UDF_EXTENT_LENGTH_MASK;
        ExtPrint(("ExExt: type %x, loc %x, len %x\n", type, UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation)), len));
        if(type == EXTENT_NEXT_EXTENT_ALLOCDESC) {
            // read next frag of allocation descriptors if encountered
            if(len < sizeof(ALLOC_EXT_DESC)) {
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocDesc = (PALLOC_EXT_DESC)MyAllocatePoolTag__(NonPagedPool, len, MEM_ALLOCDESC_TAG);
            if(!NextAllocDesc) {
                MyFreePool__(Extent);
                return NULL;
            }
            // record information about this frag
            AllocExt.extLength = len;
            AllocExt.extLocation = UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation));
            if(AllocExt.extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address\n"));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            NextAllocLoc.Mapping =
            AllocMap = UDFExtentToMapping(&AllocExt);
            NextAllocLoc.Length = len;
            if(!AllocMap) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            AllocLoc->Mapping = UDFMergeMappings(AllocLoc->Mapping, AllocMap);
            if(!AllocLoc->Mapping ||
            // read this frag
               !OS_SUCCESS(UDFReadExtent(Vcb, &NextAllocLoc,
                                0, len, FALSE, (int8*)NextAllocDesc, &ReadBytes)))
            {
                MyFreePool__(AllocMap);
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            MyFreePool__(AllocMap);
            // check integrity
            if((NextAllocDesc->descTag.tagIdent != TID_ALLOC_EXTENT_DESC) ||
               (NextAllocDesc->lengthAllocDescs > (len - sizeof(ALLOC_EXT_DESC))) ) {
                KdPrint(("Integrity check failed\n"));
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            // perform recursive call to obtain mapping
            Extent2 = UDFExtAllocDescToMapping(Vcb, (PEXT_AD)(NextAllocDesc+1),
                                      NextAllocDesc->lengthAllocDescs, SubCallCount+1, AllocLoc);
            if(!Extent2) {
                MyFreePool__(NextAllocDesc);
                MyFreePool__(Extent);
                return NULL;
            }
            // and merge this 2 mappings into 1
            Extent[i].extLength = 0;
            Extent[i].extLocation = 0;
            Extent = UDFMergeMappings(Extent, Extent2);
            MyFreePool__(Extent2);
            return Extent;
        }
/*        if((AllocDesc[i].extLength & UDF_EXTENT_LENGTH_MASK) > // Uncomment!!!
           (AllocDesc[i].recordedLength & UDF_EXTENT_LENGTH_MASK)) {
            Extent[i].extLength = AllocDesc[i].recordedLength;
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation));
        }*/
        Extent[i].extLength = len;
#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(len & (Vcb->LBlockSize-1) ));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        // Note: for compatibility Adaptec DirectCD we check 'len' here
        //       That strange implementation records bogus extLocation in terminal entries
        if(type != EXTENT_NOT_RECORDED_NOT_ALLOCATED && len) {
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb,&(AllocDesc[i].extLocation));
            if(Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                KdPrint(("bad address (2)\n"));
                MyFreePool__(Extent);
                return NULL;
            }
        } else {
            Extent[i].extLocation = 0;
        }
        if(!len) {
            // some UDF implementations set strange AllocDesc sequence length,
            // but terminates it with zeros in proper place, so handle
            // this case
            Extent[i].extLocation = 0;
            return Extent;
        }
        Extent[i].extLength |= (type << 30);
    }
    // set terminator
    Extent[i].extLength = 0;
    Extent[i].extLocation = 0;

    return Extent;
} // end UDFExtAllocDescToMapping()


/*
    This routine builds FileMapping according to given FileEntry
    Return:    pointer to EXTENT_MAP array
            or offset inside FileEntry (negative)
               when ICB_FLAG_AD_IN_ICB encountered
            of NULL if an error occured
 */
PEXTENT_MAP
UDFReadMappingFromXEntry(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN tag* XEntry,
    IN OUT uint32* Offset,
    OUT PEXTENT_INFO AllocLoc // .Mapping must be intialized (non-Zero)
    )
{
    PEXTENT_AD Extent;
    uint16 AllocMode;
    int8* AllocDescs;
    uint32 len;
//    EntityID* eID;  // for compatibility with Adaptec DirectCD

    Extent = NULL;
    (*Offset) = 0;


    if(XEntry->tagIdent == TID_FILE_ENTRY) {
//        KdPrint(("Standard FileEntry\n"));
        PFILE_ENTRY FileEntry = (PFILE_ENTRY)XEntry;
        ExtPrint(("Standard FileEntry\n"));

        AllocDescs = (int8*)(((int8*)(FileEntry+1))+(FileEntry->lengthExtendedAttr));
        len = FileEntry->lengthAllocDescs;
        AllocLoc->Offset = sizeof(FILE_ENTRY) + FileEntry->lengthExtendedAttr;
//        eID = &(FileEntry->impIdent);

        AllocMode = FileEntry->icbTag.flags & ICB_FLAG_ALLOC_MASK;

    } else if(XEntry->tagIdent == TID_EXTENDED_FILE_ENTRY) {
//        KdPrint(("Extended FileEntry\n"));
        ExtPrint(("Extended FileEntry\n"));
        PEXTENDED_FILE_ENTRY ExFileEntry = (PEXTENDED_FILE_ENTRY)XEntry;

        AllocDescs = (((int8*)(ExFileEntry+1))+(ExFileEntry->lengthExtendedAttr));
        len = ExFileEntry->lengthAllocDescs;
        AllocLoc->Offset = sizeof(EXTENDED_FILE_ENTRY) + ExFileEntry->lengthExtendedAttr;
//        eID = &(FileEntry->impIdent);

        AllocMode = ExFileEntry->icbTag.flags & ICB_FLAG_ALLOC_MASK;

    } else {
        return NULL;
    }

    // for compatibility with Adaptec DirectCD
//    if(!(Vcb->UDF_VCB_IC_ADAPTEC_NONALLOC_COMPAT))

    AllocLoc->Length=len;
    AllocLoc->Flags |= EXTENT_FLAG_VERIFY; // for metadata

    switch (AllocMode) {
    case ICB_FLAG_AD_SHORT: {
        Extent = UDFShortAllocDescToMapping(Vcb, PartNum, (PSHORT_AD)AllocDescs, len, 0, AllocLoc);
        break;
    }
    case ICB_FLAG_AD_LONG: {
        Extent = UDFLongAllocDescToMapping(Vcb, (PLONG_AD)AllocDescs, len, 0, AllocLoc);
        break;
    }
    case ICB_FLAG_AD_EXTENDED: {
        Extent = UDFExtAllocDescToMapping(Vcb, (PEXT_AD)AllocDescs, len, 0, AllocLoc);
        break;
    }
    default : {  // case ICB_FLAG_AD_IN_ICB
        Extent = NULL;
        *Offset = (uint32)AllocDescs - (uint32)XEntry;
        AllocLoc->Offset=0;
        AllocLoc->Length=0;
        if(AllocLoc->Mapping) MyFreePool__(AllocLoc->Mapping);
        AllocLoc->Mapping=NULL;
        break;
    }
    }

    ExtPrint(("UDFReadMappingFromXEntry: mode %x, loc %x, len %x\n", AllocMode,
        AllocLoc->Mapping ? AllocLoc->Mapping[0].extLocation : -1, len));

    UDFCheckSpaceAllocation(Vcb, 0, Extent, AS_USED); // check if used

    return Extent;
}// end UDFReadMappingFromXEntry()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine builds data for AllocDesc sequence for specified
    extent
 */
OSSTATUS
UDFBuildShortAllocDescs(
    IN PVCB Vcb,
    IN uint32 PartNum,
    OUT int8** Buff,  // data for AllocLoc
    IN uint32 InitSz,
 IN OUT PUDF_FILE_INFO FileInfo
    )
{
    uint32 i, j;
    uint32 len=0;
    PEXTENT_MAP Extent = FileInfo->Dloc->DataLoc.Mapping;
    PEXTENT_INFO AllocExtent = &(FileInfo->Dloc->AllocLoc);
    PSHORT_AD Alloc;
    uint32 NewLen;
    OSSTATUS status;
    uint32 ph_len=0; // in general, this should be uint64,
                     // but we need its lower part only
#ifdef UDF_ALLOW_FRAG_AD
    uint32 ts, ac, len2;
    uint32 LBS = Vcb->LBlockSize;
    uint32 LBSh = Vcb->BlockSizeBits;
    uint32 TagLen = 0;
    tag* Tag = NULL;
    PSHORT_AD saved_Alloc;
    uint32 TagLoc, prevTagLoc;
    uint32 BufOffs;
    uint32 ExtOffs;
    uint32 saved_NewLen;
#endif //UDF_ALLOW_FRAG_AD

    ValidateFileInfo(FileInfo);
    ExtPrint(("UDFBuildShortAllocDescs: FE %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
    // calculate length
    for(len=0; i=(Extent[len].extLength & UDF_EXTENT_LENGTH_MASK); len++, ph_len+=i) {
        ExtPrint(("bShExt: type %x, loc %x, len %x\n",
            Extent[len].extLength >> 30, Extent[len].extLocation, Extent[len].extLength & UDF_EXTENT_LENGTH_MASK));
    }
    Alloc = (PSHORT_AD)MyAllocatePoolTag__(NonPagedPool, (len+1)*sizeof(SHORT_AD), MEM_SHAD_TAG);
    if(!Alloc) {
        BrutePoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // fill contiguous AllocDesc buffer (decribing UserData)
    for(i=0;i<len;i++) {
        Alloc[i].extLength = Extent[i].extLength;
        Alloc[i].extPosition = UDFPhysLbaToPart(Vcb, PartNum, Extent[i].extLocation);
    }
    if((Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) && i) {
        Alloc[i-1].extLength -= (ph_len - (ULONG)(FileInfo->Dloc->DataLoc.Length)) &
                                (Vcb->LBlockSize-1);
        ExtPrint(("bShExt: cut tail -> %x\n",
            Alloc[i-1].extLength & UDF_EXTENT_LENGTH_MASK));
    }
    Alloc[i].extLength =
    Alloc[i].extPosition = 0;
    j = len*sizeof(SHORT_AD); // required space
    len = (InitSz & ~(sizeof(SHORT_AD)-1)); // space available in 1st block
    ASSERT(len == InitSz);

    // Ok. Let's init AllocLoc
    if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
        FileInfo->Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool, 2 * sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
        if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
            BrutePoint();
            MyFreePool__(Alloc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // allocation descriptors are located in the same sector as FileEntry
        // (at least their 1st part), just after it
        FileInfo->Dloc->AllocLoc.Mapping[0] = FileInfo->Dloc->FELoc.Mapping[0];
        FileInfo->Dloc->AllocLoc.Offset = FileInfo->Dloc->FileEntryLen;
        FileInfo->Dloc->AllocLoc.Length = 0;
        // set terminator
        FileInfo->Dloc->AllocLoc.Mapping[1].extLength =
        FileInfo->Dloc->AllocLoc.Mapping[1].extLocation = 0;
    }

    if(j <= len) {
        // we needn't allocating additional blocks to store AllocDescs
        AdPrint(("in-ICB AllocDescs, j=%x\n",j));
        RtlCopyMemory(*Buff, (int8*)Alloc, j);
        NewLen = j;
        MyFreePool__(Alloc);
    } else {
#ifndef UDF_ALLOW_FRAG_AD
        AdPrint(("  DISK_FULL\n"));
        return STATUS_DISK_FULL;
#else //UDF_ALLOW_FRAG_AD
        AdPrint(("multi-block AllocDescs, j=%x\n",j));
        BufOffs = 0;
        TagLoc = prevTagLoc = 0;
        // calculate the space available for SHORT_ADs in each block
        ac = (LBS - (sizeof(ALLOC_EXT_DESC) + sizeof(SHORT_AD))) & ~(sizeof(SHORT_AD)-1);
        len2 = len;
        // tail size
        ts = InitSz - len2;
        len -= sizeof(SHORT_AD);
        // calculate actual AllocSequence length (in bytes)
        NewLen = ( ((j - len + ac - 1) / ac) << LBSh) + InitSz + sizeof(SHORT_AD);
        MyFreePool__(*Buff);
        (*Buff) = (int8*)MyAllocatePoolTag__(NonPagedPool, NewLen, MEM_SHAD_TAG);
        if(!(*Buff)) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KdPrint(("UDFResizeExtent() failed (%x)\n",status));
            BrutePoint();
            goto sh_alloc_err;
        }
        if(UDFGetExtentLength(AllocExtent->Mapping) < NewLen) {
            status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
            if(!OS_SUCCESS(status)) {
                KdPrint(("UDFResizeExtent(2) failed (%x)\n",status));
                BrutePoint();
sh_alloc_err:
                MyFreePool__(Alloc);
                return status;
            }
        }
        ExtOffs = AllocExtent->Offset;
        RtlZeroMemory(*Buff, NewLen);
        saved_NewLen = NewLen;
        NewLen = 0; // recorded length
        saved_Alloc = Alloc;
        // fill buffer sector by sector (adding links at the end of each one)
        while(TRUE) {

            // j - remained AllocDescs length (in bytes)
            // len - bytes available for AllocDescs in current block
            // ac - bytes available for AllocDescs in each block

            // leave space for terminator or pointer to next part of sequence
            if(j == len2) {
                // if we have only 1 SHORT_AD that we can fit in last sector
                // we shall do it instead of recording link & allocating new block
                len =
                TagLen = len2;
            }
            ASSERT(saved_NewLen >= (BufOffs + len));
            RtlCopyMemory( (*Buff)+BufOffs, (int8*)Alloc, len);
            Alloc = (PSHORT_AD)((int8*)Alloc + len);
            j -= len;
            BufOffs += len;
            if(Tag) {
                // Set up Tag for AllocDesc
                Tag->tagIdent = TID_ALLOC_EXTENT_DESC;
                UDFSetUpTag(Vcb, Tag, (uint16)TagLen, TagLoc);
                prevTagLoc = TagLoc;
            }
            if(!j) {
                // terminate loop
                NewLen = BufOffs;
                break;
            }
            len = ac;
            if(j <= (len + sizeof(SHORT_AD)))
                len = j - sizeof(SHORT_AD);
            len2 = len + sizeof(SHORT_AD);
            // we have more than 1 SHORT_AD that we can't fit in current block
            // so we shall set up pointer to the next block
            ((PSHORT_AD)((*Buff)+BufOffs))->extLength = /*LBS*/ len2 |
                (((uint32)EXTENT_NEXT_EXTENT_ALLOCDESC) << 30) ;
            ((PSHORT_AD)((*Buff)+BufOffs))->extPosition = TagLoc =
                UDFPhysLbaToPart(Vcb, PartNum,
                    UDFExtentOffsetToLba(Vcb, AllocExtent->Mapping,
                        ExtOffs+BufOffs+sizeof(SHORT_AD)+ts,
                        NULL, NULL, NULL, NULL) );
            // reflect additional (link) block & LBlock tail (if any)
            BufOffs += ts+sizeof(SHORT_AD);
            // init AllocDesc
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->lengthAllocDescs = len2;
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->previousAllocExtLocation = prevTagLoc;
            Tag = (tag*)((*Buff)+BufOffs);
            TagLen = len2;
            ts = LBS-len2-sizeof(ALLOC_EXT_DESC);
            BufOffs += sizeof(ALLOC_EXT_DESC);
        }
        MyFreePool__(saved_Alloc);
#endif //UDF_ALLOW_FRAG_AD
    }
    status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
    return status;
} // end UDFBuildShortAllocDescs()

/*
    This routine builds data for AllocDesc sequence for specified
    extent
 */
OSSTATUS
UDFBuildLongAllocDescs(
    IN PVCB Vcb,
    IN uint32 PartNum,
    OUT int8** Buff,  // data for AllocLoc
    IN uint32 InitSz,
 IN OUT PUDF_FILE_INFO FileInfo
    )
{
    uint32 i, j;
    uint32 len=0;
    PEXTENT_MAP Extent = FileInfo->Dloc->DataLoc.Mapping;
    PEXTENT_INFO AllocExtent = &(FileInfo->Dloc->AllocLoc);
    PLONG_AD Alloc;
    uint32 NewLen;
    OSSTATUS status;
    uint32 ph_len=0; // in general, this should be uint64,
                     // but we need its lower part only
#ifdef UDF_ALLOW_FRAG_AD
    uint32 ac, len2, ts;
    uint32 TagLoc, prevTagLoc;
    uint32 LBS = Vcb->LBlockSize;
    uint32 LBSh = Vcb->BlockSizeBits;
    uint32 BufOffs;
    uint32 ExtOffs = AllocExtent->Offset;
    PLONG_AD saved_Alloc;
    uint32 TagLen = 0;
    tag* Tag = NULL;
#endif //UDF_ALLOW_FRAG_AD

    ValidateFileInfo(FileInfo);
    ExtPrint(("UDFBuildLongAllocDescs: FE %x\n", FileInfo->Dloc->FELoc.Mapping[0].extLocation));
    // calculate length
    //for(len=0; i=(Extent[len].extLength & UDF_EXTENT_LENGTH_MASK); len++, ph_len+=i);
    for(len=0; i=(Extent[len].extLength & UDF_EXTENT_LENGTH_MASK); len++, ph_len+=i) {
        ExtPrint(("bLnExt: type %x, loc %x, len %x\n",
            Extent[len].extLength >> 30, Extent[len].extLocation, Extent[len].extLength & UDF_EXTENT_LENGTH_MASK));
    }
    Alloc = (PLONG_AD)MyAllocatePoolTag__(NonPagedPool, (len+1)*sizeof(LONG_AD), MEM_LNGAD_TAG);
    if(!Alloc) return STATUS_INSUFFICIENT_RESOURCES;
    // fill contiguous AllocDesc buffer (decribing UserData)
    for(i=0;i<len;i++) {
        Alloc[i].extLength = Extent[i].extLength;
        Alloc[i].extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, Extent[i].extLocation);
        Alloc[i].extLocation.partitionReferenceNum = (uint16)PartNum;
        RtlZeroMemory(&(Alloc[i].impUse), sizeof(Alloc[i].impUse));
    }
    if((Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS) && i) {
        Alloc[i-1].extLength -= (ph_len - (ULONG)(FileInfo->Dloc->DataLoc.Length)) &
                                (Vcb->LBlockSize-1);
        ExtPrint(("bLnExt: cut tail -> %x\n",
            Alloc[i-1].extLength & UDF_EXTENT_LENGTH_MASK));
    }
    RtlZeroMemory(&(Alloc[i]), sizeof(LONG_AD));
    j = len*sizeof(LONG_AD); // required space              
    len = (InitSz & ~(sizeof(LONG_AD)-1)); // space available in 1st block
    ASSERT(len == InitSz);

    // Ok. Let's init AllocLoc
    if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
        FileInfo->Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool, 2 * sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
        if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
            MyFreePool__(Alloc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // allocation descriptors are located in the same sector as FileEntry
        // (at least their 1st part), just after it
        FileInfo->Dloc->AllocLoc.Mapping[0] = FileInfo->Dloc->FELoc.Mapping[0];
        FileInfo->Dloc->AllocLoc.Offset = FileInfo->Dloc->FileEntryLen;
        FileInfo->Dloc->AllocLoc.Length = 0;
        // set terminator
        FileInfo->Dloc->AllocLoc.Mapping[1].extLength =
        FileInfo->Dloc->AllocLoc.Mapping[1].extLocation = 0;
    }

    if(j <= len) {
        // we needn't allocating additional blocks to store AllocDescs
        RtlCopyMemory(*Buff, (int8*)Alloc, j);
        NewLen = j;
        MyFreePool__(Alloc);
    } else {
#ifndef UDF_ALLOW_FRAG_AD
        AdPrint(("  DISK_FULL\n"));
        return STATUS_DISK_FULL;
#else //UDF_ALLOW_FRAG_AD
        BufOffs = 0;
        TagLoc = prevTagLoc = 0;
        // calculate the space available for LONG_ADs in each block
        ac = (LBS - (sizeof(ALLOC_EXT_DESC) + sizeof(LONG_AD))) & ~(sizeof(LONG_AD)-1);
        len2 = len;
        // tail size
        ts = InitSz - len2;
        len -= sizeof(LONG_AD);
        // calculate actual AllocSequence length (in LBlocks)
        NewLen = ( ((j - len + ac - 1) / ac) << LBSh) + InitSz + sizeof(LONG_AD);
        MyFreePool__(*Buff);
        (*Buff) = (int8*)MyAllocatePoolTag__(NonPagedPool, NewLen, MEM_LNGAD_TAG);
        if(!(*Buff)) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto lad_alloc_err;
        }
        if(UDFGetExtentLength(AllocExtent->Mapping) < NewLen) {
            status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
            if(!OS_SUCCESS(status)) {
lad_alloc_err:
                MyFreePool__(Alloc);
                return status;
            }
        }
        ExtOffs = AllocExtent->Offset;
        RtlZeroMemory(*Buff, NewLen);
        NewLen = 0; // recorded length
        saved_Alloc = Alloc;
        len2 = len+sizeof(LONG_AD);
        // fill buffer sector by sector (adding links at the end of each one)
        while(TRUE) {

            // j - remained AllocDescs length (in bytes)
            // len - bytes available for in AllocDescs each block

            // leave space for terminator or pointer to next part of sequence
            if(j == len2) {
                // if we have only 1 LONG_AD that we can fit in last sector
                // we shall do it instead of recording link & allocating new block
                len =
                TagLen = len2;
            }
            RtlCopyMemory( (*Buff)+BufOffs, (int8*)Alloc, len);
            Alloc = (PLONG_AD)((int8*)Alloc + len);
            j -= len;
            BufOffs += len;
            if(Tag) {
                // Set up Tag for AllocDesc
                Tag->tagIdent = TID_ALLOC_EXTENT_DESC;
                UDFSetUpTag(Vcb, Tag, (uint16)TagLen, TagLoc);
                prevTagLoc = TagLoc;
            }
            if(!j) {
                // terminate loop
                NewLen = BufOffs;
                break;
            }
            len = ac;
            if(j <= (len + sizeof(LONG_AD)))
                len = j - sizeof(LONG_AD);
            len2 = len+sizeof(LONG_AD);
            // we have more than 1 LONG_AD that we can't fit in current block
            // so we shall set up pointer to the next block
            ((PLONG_AD)((*Buff)+BufOffs))->extLength = /*LBS*/ len2 |
                (((uint32)EXTENT_NEXT_EXTENT_ALLOCDESC) << 30) ;
            ((PLONG_AD)((*Buff)+BufOffs))->extLocation.logicalBlockNum = TagLoc =
                UDFPhysLbaToPart(Vcb, PartNum,
                    UDFExtentOffsetToLba(Vcb, AllocExtent->Mapping,
                        ExtOffs+BufOffs+sizeof(LONG_AD)+ts,
                        NULL, NULL, NULL, NULL) );
            ((PLONG_AD)((*Buff)+BufOffs))->extLocation.partitionReferenceNum = (uint16)PartNum;
            // reflect additional (link) block & LBlock tail (if any)
            BufOffs += ts+sizeof(LONG_AD);
            // init AllocDesc
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->lengthAllocDescs = len2;
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->previousAllocExtLocation = prevTagLoc;
            Tag = (tag*)((*Buff)+BufOffs);
            TagLen = len2;
            ts = LBS-len2-sizeof(ALLOC_EXT_DESC);
            BufOffs += sizeof(ALLOC_EXT_DESC);
        }
        MyFreePool__(saved_Alloc);
#endif //UDF_ALLOW_FRAG_AD
    }
    status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
    return status;
} // end UDFBuildLongAllocDescs()

/*
    This routine builds data for AllocDesc sequence for specified
    extent
 */
/*OSSTATUS
UDFBuildExtAllocDescs(
    IN PVCB Vcb,
    IN uint32 PartNum,
    OUT int8** Buff,  // data for AllocLoc
    IN uint32 InitSz,
 IN OUT PUDF_FILE_INFO FileInfo
    )
{
    uint32 i, j;
    uint32 len=0, ac, len2;
    uint32 TagLoc, prevTagLoc;
    uint32 LBS = Vcb->LBlockSize;
    uint32 LBSh = Vcb->BlockSizeBits;
    PEXTENT_MAP Extent = FileInfo->Dloc->DataLoc.Mapping;
    PEXTENT_INFO AllocExtent = &(FileInfo->Dloc->AllocLoc);
    PEXT_AD Alloc, saved_Alloc;
    uint32 BufOffs;
    uint32 ExtOffs = AllocExtent->Offset;
    uint32 NewLen;
    OSSTATUS status;
    uint32 TagLen = 0;
    tag* Tag = NULL;

    ValidateFileInfo(FileInfo);
    // calculate length
    for(len=0; Extent[len].extLength; len++);
    Alloc = (PEXT_AD)MyAllocatePool__(NonPagedPool, (len+1)*sizeof(EXT_AD));
    if(!Alloc) return STATUS_INSUFFICIENT_RESOURCES;
    // fill contiguous AllocDesc buffer (decribing UserData)
    for(i=0;i<len;i++) {
        Alloc[i].extLength =
        Alloc[i].recordedLength =
        Alloc[i].informationLength = Extent[i].extLength;
        Alloc[i].extLocation.logicalBlockNum = UDFPhysLbaToPart(Vcb, PartNum, Extent[i].extLocation);
        Alloc[i].extLocation.partitionReferenceNum = (uint16)PartNum;
    }
    RtlZeroMemory(&(Alloc[i]), sizeof(EXT_AD));
    j = len*sizeof(EXT_AD); // required space
    len = InitSz;            // space available in 1st block

    // Ok. Let's init AllocLoc
    if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
        FileInfo->Dloc->AllocLoc.Mapping = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, 2 * sizeof(EXTENT_MAP));
        if(!(FileInfo->Dloc->AllocLoc.Mapping)) {
            MyFreePool__(Alloc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // allocation descriptors are located in the same sector as FileEntry
        // (at least their 1st part), just after it
        FileInfo->Dloc->AllocLoc.Mapping[0] = FileInfo->Dloc->FELoc.Mapping[0];
        FileInfo->Dloc->AllocLoc.Offset = FileInfo->Dloc->FileEntryLen;
        FileInfo->Dloc->AllocLoc.Length = 0;
        // set terminator
        FileInfo->Dloc->AllocLoc.Mapping[1].extLength =
        FileInfo->Dloc->AllocLoc.Mapping[1].extLocation = 0;
    }

    if(j <= len) {
        // we needn't allocating additional blocks to store AllocDescs
        RtlCopyMemory(*Buff, (int8*)Alloc, j);
        NewLen = j;
        MyFreePool__(Alloc);
    } else {
        BufOffs = 0;
        TagLoc = prevTagLoc = 0;
        // calculate the space available for EXT_ADs in each block
        ac = (LBS - (sizeof(ALLOC_EXT_DESC) + sizeof(EXT_AD))) & ~(sizeof(EXT_AD)-1);
        // calculate actual AllocSequence length (in LBlocks)
        len -= sizeof(EXT_AD);
        NewLen = ( ((j - len + ac - 1) / ac) << LBSh) + len + sizeof(EXT_AD);
        MyFreePool__(*Buff);
        (*Buff) = (int8*)MyAllocatePool__(NonPagedPool, NewLen);
        if(UDFGetExtentLength(AllocExtent->Mapping) < NewLen) {
            status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
            if(!OS_SUCCESS(status)) {
                MyFreePool__(Alloc);
                return status;
            }
        }
        RtlZeroMemory(*Buff, NewLen);
        NewLen = 0; // recorded length
        saved_Alloc = Alloc;
        len2 = len + sizeof(EXT_AD);
        // fill buffer sector by sector (adding links at the end of each one)
        while(TRUE) {

            // j - remained AllocDescs length (in bytes)
            // len - bytes available for in AllocDescs each block

            // leave space for terminator or pointer to next part of sequence
            if(j == len2) {
                // if we have only 1 EXT_AD that we can fit in last sector
                // we shall do it instead of recording link & allocating new block
                len =
                TagLen = len2;
            }
            RtlCopyMemory( (*Buff)+BufOffs, (int8*)Alloc, len);
            Alloc = (PEXT_AD)((int8*)Alloc + len);
            j -= len;
            BufOffs += len;
            if(Tag) {
                // Set up Tag for AllocDesc
                Tag->tagIdent = TID_ALLOC_EXTENT_DESC;
                UDFSetUpTag(Vcb, Tag, (uint16)TagLen, TagLoc);
                prevTagLoc = TagLoc;
            }
            if(!j) {
                // terminate loop
                NewLen = BufOffs;
                break;
            }
            len = ac;
            if(j <= (len + sizeof(EXT_AD)))
                len = j - sizeof(EXT_AD);
            len2 = len + sizeof(EXT_AD);
            // we have more than 1 EXT_AD that we can't fit in current block
            // so we shall set up pointer to the next block
            ((PEXT_AD)((*Buff)+BufOffs))->extLength =
            ((PEXT_AD)((*Buff)+BufOffs))->recordedLength = LBS;
            ((PEXT_AD)((*Buff)+BufOffs))->informationLength = len2 |
                (((uint32)EXTENT_NEXT_EXTENT_ALLOCDESC) << 30) ;
            ((PEXT_AD)((*Buff)+BufOffs))->extLocation.logicalBlockNum = TagLoc =
                UDFPhysLbaToPart(Vcb, PartNum,
                    UDFExtentOffsetToLba(Vcb, AllocExtent->Mapping, ExtOffs + BufOffs + 2*sizeof(EXT_AD)-1, NULL, NULL, NULL, NULL) );
            ((PEXT_AD)((*Buff)+BufOffs))->extLocation.partitionReferenceNum = (uint16)PartNum;
            BufOffs = (BufOffs + 2*sizeof(EXT_AD) - 1) & ~(sizeof(EXT_AD)-1) ;
            // init AllocDesc
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->lengthAllocDescs = len2;
            ( (PALLOC_EXT_DESC) ((*Buff)+BufOffs))->previousAllocExtLocation = prevTagLoc;
            Tag = (tag*)((*Buff)+BufOffs);
            TagLen = len2;
            BufOffs += sizeof(ALLOC_EXT_DESC);
        }
        MyFreePool__(saved_Alloc);
    }
    status = UDFResizeExtent(Vcb, PartNum, NewLen, TRUE, AllocExtent);
    return status;
} // end UDFBuildExtAllocDescs()*/

void
UDFDiscardFESpace(
    IN PVCB Vcb,
    IN PEXTENT_MAP Mapping,
    IN uint32 lim
    )
{
#ifdef UDF_FE_ALLOCATION_CHARGE // UDF_FE_ALLOCATION_CHARGE
    PEXTENT_MAP Mapping2;
    uint32 i;

    KdPrint(("  DiscardFESpace\n"));
    Mapping2 = Mapping;
    for(i=0;i<lim;i++, Mapping++) {
        // we should not discard allocated FEs
        if( (Mapping->extLength >> 30) == EXTENT_RECORDED_ALLOCATED) {
            KdPrint(("  used @ %x\n", Mapping->extLocation));
            Mapping->extLength = Vcb->LBlockSize | (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            Mapping->extLocation = 0;
        } else {
            KdPrint(("  free @ %x\n", Mapping->extLocation));
        }
    }
    UDFMarkSpaceAsXXX(Vcb, 0, Mapping2, AS_DISCARDED);

    MyFreePool__(Mapping2);
#else // UDF_FE_ALLOCATION_CHARGE
    ASSERT(!Dloc->DirIndex->FECharge.Mapping);
    return;
#endif // UDF_FE_ALLOCATION_CHARGE
} // end UDFDiscardFESpace()

OSSTATUS
UDFInitAllocationCache(
    IN PVCB Vcb,
    IN uint32 AllocClass,
   OUT PUDF_ALLOCATION_CACHE_ITEM* _AllocCache,
   OUT uint32* _lim,
    IN BOOLEAN Init
    )
{
    PUDF_ALLOCATION_CACHE_ITEM AllocCache;
    PUDF_ALLOCATION_CACHE_ITEM* pAllocCache;
    uint32 i, lim;
    uint32* plim;

    switch(AllocClass) {
    case UDF_PREALLOC_CLASS_FE:
        KdPrint(("AllocationCache FE:\n"));
        pAllocCache = &(Vcb->FEChargeCache);
        plim = &(Vcb->FEChargeCacheMaxSize);
        lim = 32;
        break;
    case UDF_PREALLOC_CLASS_DIR:
        KdPrint(("AllocationCache DIR:\n"));
        pAllocCache = &(Vcb->PreallocCache);
        plim = &(Vcb->PreallocCacheMaxSize);
        lim = 32;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }
    if(!(*plim)) {
        if(!Init) {
            return STATUS_UNSUCCESSFUL;
        }
        (*pAllocCache) = AllocCache =
                  (PUDF_ALLOCATION_CACHE_ITEM)
                  MyAllocatePoolTag__(NonPagedPool , sizeof(UDF_ALLOCATION_CACHE_ITEM)*lim,
                                      MEM_ALLOC_CACHE_TAG);
        if(!AllocCache) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(AllocCache, sizeof(UDF_ALLOCATION_CACHE_ITEM)*lim);
        for(i=0; i<lim; i++) {
            AllocCache[i].ParentLocation = LBA_NOT_ALLOCATED;
        }
        (*plim) = lim;
    } else {
        lim = (*plim);
        AllocCache = (*pAllocCache);
    }
    (*_lim) = lim;
    (*_AllocCache) = AllocCache;

    return STATUS_SUCCESS;
} // end UDFInitAllocationCache()

OSSTATUS
UDFGetCachedAllocation(
    IN PVCB Vcb,
    IN uint32 ParentLocation,
   OUT PEXTENT_INFO Ext,
   OUT uint32* Items, // optional
    IN uint32 AllocClass
    )
{
    PUDF_ALLOCATION_CACHE_ITEM AllocCache;
    uint32 i, lim;
    OSSTATUS status;

    UDFAcquireResourceExclusive(&(Vcb->PreallocResource),TRUE);

    status = UDFInitAllocationCache(Vcb, AllocClass, &AllocCache, &lim, FALSE);
    if(!OS_SUCCESS(status)) {
        UDFReleaseResource(&(Vcb->PreallocResource));
        return status;
    }
    KdPrint(("Get AllocationCache for %x\n", ParentLocation));

    for(i=0; i<lim; i++) {
        if(AllocCache[i].ParentLocation == ParentLocation) {
            (*Ext) = AllocCache[i].Ext;
            AdPrint(("    map %x (%x)\n", Ext->Mapping, i));
            if(Items) {
                (*Items) = AllocCache[i].Items;
            }
            RtlZeroMemory(&(AllocCache[i]), sizeof(AllocCache[i]));
            AllocCache[i].ParentLocation = LBA_NOT_ALLOCATED;
            UDFReleaseResource(&(Vcb->PreallocResource));
            return STATUS_SUCCESS;
        }
    }
    AdPrint(("    no map\n"));
    UDFReleaseResource(&(Vcb->PreallocResource));
    return STATUS_UNSUCCESSFUL;
} // end UDFGetCachedAllocation()

OSSTATUS
UDFStoreCachedAllocation(
    IN PVCB Vcb,
    IN uint32 ParentLocation,
    IN PEXTENT_INFO Ext,
    IN uint32 Items,
    IN uint32 AllocClass
    )
{
    PUDF_ALLOCATION_CACHE_ITEM AllocCache;
    uint32 i, lim;
    OSSTATUS status;

    UDFAcquireResourceExclusive(&(Vcb->PreallocResource),TRUE);

    status = UDFInitAllocationCache(Vcb, AllocClass, &AllocCache, &lim, TRUE);
    if(!OS_SUCCESS(status)) {
        UDFReleaseResource(&(Vcb->PreallocResource));
        return status;
    }
    KdPrint(("Store AllocationCache for %x, map %x\n", ParentLocation, Ext->Mapping));

    for(i=0; i<lim; i++) {
        if(AllocCache[i].ParentLocation == LBA_NOT_ALLOCATED) {
            AdPrint(("    stored in %x\n", i));
            AllocCache[i].Ext = (*Ext);
            AllocCache[i].Items = Items;
            AllocCache[i].ParentLocation = ParentLocation;
            UDFReleaseResource(&(Vcb->PreallocResource));
            return STATUS_SUCCESS;
        }
    }
    //
    AdPrint(("    drop map %x (%x)\n", AllocCache[lim-1].Ext.Mapping, lim-1));
    switch(AllocClass) {
    case UDF_PREALLOC_CLASS_FE:
        UDFDiscardFESpace(Vcb, AllocCache[lim-1].Ext.Mapping, AllocCache[lim-1].Items);
        break;
    case UDF_PREALLOC_CLASS_DIR:
        UDFMarkSpaceAsXXX(Vcb, 0, AllocCache[lim-1].Ext.Mapping, AS_DISCARDED);
        break;
    }
    RtlMoveMemory(&(AllocCache[1]), &(AllocCache[0]), sizeof(UDF_ALLOCATION_CACHE_ITEM)*(lim-1));
    AllocCache[0].Ext = (*Ext);
    AllocCache[0].Items = Items;
    AllocCache[0].ParentLocation = ParentLocation;
    AdPrint(("    stored in 0\n"));
    UDFReleaseResource(&(Vcb->PreallocResource));
    return STATUS_SUCCESS;
} // end UDFStoreCachedAllocation()

OSSTATUS
UDFFlushAllCachedAllocations(
    IN PVCB Vcb,
    IN uint32 AllocClass
    )
{
    PUDF_ALLOCATION_CACHE_ITEM AllocCache;
    uint32 i, lim;
    OSSTATUS status;

    KdPrint(("Flush AllocationCache\n"));
    UDFAcquireResourceExclusive(&(Vcb->PreallocResource),TRUE);

    status = UDFInitAllocationCache(Vcb, AllocClass, &AllocCache, &lim, FALSE);
    if(!OS_SUCCESS(status)) {
        UDFReleaseResource(&(Vcb->PreallocResource));
        return status;
    }

    for(i=0; i<lim; i++) {
        if(AllocCache[i].ParentLocation != LBA_NOT_ALLOCATED) {
            switch(AllocClass) {
            case UDF_PREALLOC_CLASS_FE:
                UDFDiscardFESpace(Vcb, AllocCache[i].Ext.Mapping, AllocCache[i].Items);
                break;
            case UDF_PREALLOC_CLASS_DIR:
                UDFMarkSpaceAsXXX(Vcb, 0, AllocCache[i].Ext.Mapping, AS_DISCARDED);
                break;
            }
        }
    }
    MyFreePool__(AllocCache);
    switch(AllocClass) {
    case UDF_PREALLOC_CLASS_FE:
        Vcb->FEChargeCache = NULL;
        Vcb->FEChargeCacheMaxSize = 0;
        break;
    case UDF_PREALLOC_CLASS_DIR:
        Vcb->PreallocCache = NULL;
        Vcb->PreallocCacheMaxSize = 0;
        break;
    }
    UDFReleaseResource(&(Vcb->PreallocResource));
    //
    return STATUS_SUCCESS;
} // end UDFFlushAllCachedAllocations()

/*
    This routine allocates space for FE of the file being created
    If FE-Charge is enabled it reserves an extent & allocates
    space in it. It works much faster then usual way both while
    allocating & accessing on disk
    If FE-Charge is disabled FE may be allocated at any suitable
    location
 */
OSSTATUS
UDFAllocateFESpace(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN uint32 PartNum,
    IN PEXTENT_INFO FEExtInfo,
    IN uint32 Len
    )
{
#ifdef UDF_FE_ALLOCATION_CHARGE // UDF_FE_ALLOCATION_CHARGE
    OSSTATUS status;
    PEXTENT_INFO Ext;
    EXTENT_AD Extent;
    BOOLEAN retry = FALSE;
    uint32 i, lim;

/*
    1. #Dir1#->*File*                ->  Dir1's FECharge
    2. #Dir1#->*Dir*                 ->  Dir1's FECharge
    3. #Dir1#->*SDir*                ->  Dir1's FECharge
    4. Dir1->#SDir#->*Stream*        ->  Dir1's FEChargeSDir
    5. Dir1->#File#->*SDir*          ->  Dir1's FEChargeSDir
    6. Dir1->#Dir#->*SDir*           ->  (see p.2)
    7. Dir1->File->#SDir#->*Stream*  ->  Dir1's FEChargeSDir
    8. Dir1->Dir->#SDir#->*Stream*   ->  (see p.4)

## ~ DirInfo
** ~ Object to be created

*/

//    ASSERT(!FEExtInfo->Mapping);
    // check if DirInfo we are called with is a Directory
    // (it can be a file with SDir)
    if(!DirInfo || !DirInfo->Dloc->DirIndex ||
       ((lim = ((DirInfo->Dloc->FE_Flags & UDF_FE_FLAG_IS_SDIR) ? Vcb->FEChargeSDir : Vcb->FECharge)) <= 1))
#endif // UDF_FE_ALLOCATION_CHARGE
        return UDFAllocFreeExtent(Vcb, Len,
               UDFPartStart(Vcb, PartNum), UDFPartEnd(Vcb, PartNum), FEExtInfo, EXTENT_FLAG_VERIFY);
#ifdef UDF_FE_ALLOCATION_CHARGE // UDF_FE_ALLOCATION_CHARGE

    Ext = &(DirInfo->Dloc->DirIndex->FECharge);

    while(TRUE) {

        if(!Ext->Mapping) {
            ULONG p_start;
            ULONG p_end;
            ULONG fe_loc;
            ULONG l1, l2;
            
            p_start = UDFPartStart(Vcb, PartNum);
            p_end   = UDFPartEnd(Vcb, PartNum);
            fe_loc  = DirInfo->Dloc->FELoc.Mapping[0].extLocation;

            status = UDFGetCachedAllocation(Vcb, fe_loc, Ext, NULL, UDF_PREALLOC_CLASS_FE);
            if(OS_SUCCESS(status)) {
                // do nothing, even do not unpack
            } else
            if(Vcb->LowFreeSpace) {
                status = UDFAllocFreeExtent(Vcb, Len << Vcb->LBlockSizeBits,p_start, p_end, FEExtInfo, EXTENT_FLAG_VERIFY);
                if(OS_SUCCESS(status)) {
                    KdPrint(("FE @ %x (1)\n", FEExtInfo->Mapping[0].extLocation ));
                }
                return status;
            } else {
                if(fe_loc > p_start + 512*16) {
                    l1 = fe_loc - 512*16;
                } else {
                    l1 = p_start;
                }
                if(fe_loc + 512*16 < p_end) {
                    l2 = fe_loc + 512*16;
                } else {
                    l2 = p_end;
                }
                status = UDFAllocFreeExtent(Vcb, lim << Vcb->LBlockSizeBits, l1, l2, Ext, EXTENT_FLAG_VERIFY);
                if(!OS_SUCCESS(status)) {
                    status = UDFAllocFreeExtent(Vcb, lim << Vcb->LBlockSizeBits, (p_start+fe_loc)/2, (fe_loc+p_end)/2, Ext, EXTENT_FLAG_VERIFY);
                }
                if(!OS_SUCCESS(status)) {
                    status = UDFAllocFreeExtent(Vcb, lim << Vcb->LBlockSizeBits, p_start, p_end, Ext, EXTENT_FLAG_VERIFY);
                }
                if(!OS_SUCCESS(status)) {
                    status = UDFAllocFreeExtent(Vcb, lim << Vcb->LBlockSizeBits, p_start+1024, p_end-1024, Ext, EXTENT_FLAG_VERIFY);
                }
                if(!OS_SUCCESS(status = UDFAllocFreeExtent(Vcb, lim << Vcb->LBlockSizeBits, p_start, p_end, Ext, EXTENT_FLAG_VERIFY) )) {
                    // can't pre-allocate space for multiple FEs. Try single FE
                    KdPrint(("allocate single FE entry\n"));
                    status = UDFAllocFreeExtent(Vcb, Len,
                           p_start, p_end, FEExtInfo, EXTENT_FLAG_VERIFY);
                    if(OS_SUCCESS(status)) {
                        KdPrint(("FE @ %x (2)\n", FEExtInfo->Mapping[0].extLocation ));
                    }
                    return status;
                }
                status = UDFUnPackMapping(Vcb, Ext);
                if(!OS_SUCCESS(status)) {
                    MyFreePool__(Ext->Mapping);
                    Ext->Mapping = NULL;
                    return status;
                }
            }
        }

        for(i=0;i<lim;i++) {
            if( (Ext->Mapping[i].extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED ) {
                Ext->Mapping[i].extLength &= UDF_EXTENT_LENGTH_MASK; // EXTENT_RECORDED_ALLOCATED

                Extent.extLength = Vcb->LBlockSize | (EXTENT_NOT_RECORDED_ALLOCATED << 30);
                Extent.extLocation = Ext->Mapping[i].extLocation;

                if(Vcb->BSBM_Bitmap) {
                    uint32 lba = Ext->Mapping[i].extLocation;
                    if(UDFGetBadBit((uint32*)(Vcb->BSBM_Bitmap), lba)) {
                        KdPrint(("Remove BB @ %x from FE charge\n", lba));
                        Ext->Mapping[i].extLength |= (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
                        Ext->Mapping[i].extLocation = 0;
                        continue;
                    }
                }

                FEExtInfo->Mapping = UDFExtentToMapping(&Extent);
                if(!FEExtInfo->Mapping) {
                    ASSERT(!(Ext->Mapping[i].extLength >> 30));
                    Ext->Mapping[i].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                KdPrint(("FE @ %x (3)\n", FEExtInfo->Mapping[0].extLocation ));
                FEExtInfo->Length = Len;
                FEExtInfo->Offset = 0;
                FEExtInfo->Modified = TRUE;
                return STATUS_SUCCESS;
            }
        }

        if(Vcb->LowFreeSpace) {
            status = UDFAllocFreeExtent(Vcb, Len,
                   UDFPartStart(Vcb, PartNum), UDFPartEnd(Vcb, PartNum), FEExtInfo, EXTENT_FLAG_VERIFY);
            if(OS_SUCCESS(status)) {
                KdPrint(("FE @ %x (4)\n", FEExtInfo->Mapping[0].extLocation ));
            }
            return status;
        }
        if(retry)
            return STATUS_INSUFFICIENT_RESOURCES;

        // we can get here if there are no free slots in
        // preallocated FE charge. So, we should release
        // memory and try to allocate space for new FE charge.
        MyFreePool__(Ext->Mapping);
        Ext->Mapping = NULL;
        retry = TRUE;
    }
    return STATUS_INSUFFICIENT_RESOURCES;
#endif // UDF_FE_ALLOCATION_CHARGE

} // end UDFAllocateFESpace()

/*
    This routine frees space allocated for FE.
 */
void
UDFFreeFESpace(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PEXTENT_INFO FEExtInfo
    )
{
#ifdef UDF_FE_ALLOCATION_CHARGE // UDF_FE_ALLOCATION_CHARGE
    PEXTENT_INFO Ext;
    uint32 i, lim, j=-1;
    uint32 Lba;

    // check if the DirInfo we are called with is a Directory
    // (it can be a file with SDir)
    if(DirInfo && DirInfo->Dloc->DirIndex &&
       (Ext = &(DirInfo->Dloc->DirIndex->FECharge))->Mapping) {
        if(!FEExtInfo->Mapping)
            return;
        Lba = FEExtInfo->Mapping[0].extLocation;

        lim = (DirInfo->Dloc->FE_Flags & UDF_FE_FLAG_IS_SDIR) ? Vcb->FEChargeSDir : Vcb->FECharge;
        for(i=0;i<lim;i++) {
            if(Ext->Mapping[i].extLocation == Lba) {
                ASSERT(!(Ext->Mapping[i].extLength >> 30));
                Ext->Mapping[i].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
                goto clean_caller;
            }
            if(!Ext->Mapping[i].extLocation) {
                j = i;
            }
        }
        if(j != -1) {
            i = j;
            Ext->Mapping[i].extLocation = Lba;
            Ext->Mapping[i].extLength   = Vcb->LBlockSize | (EXTENT_NOT_RECORDED_ALLOCATED << 30);
            goto clean_caller;
        }
    }
#endif // UDF_FE_ALLOCATION_CHARGE
    UDFMarkSpaceAsXXX(Vcb, 0, FEExtInfo->Mapping, AS_DISCARDED); // free
clean_caller:
    FEExtInfo->Mapping[0].extLocation = 0;
    FEExtInfo->Mapping[0].extLength = (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
    return;
} // end UDFFreeFESpace()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine flushes FE-Charge buffer, marks unused blocks as free
    in bitmap & releases memory allocated for FE-Charge management
 */
void
UDFFlushFESpace(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc,
    IN BOOLEAN Discard
    )
{
#ifdef UDF_FE_ALLOCATION_CHARGE // UDF_FE_ALLOCATION_CHARGE
    PEXTENT_MAP Mapping;
    uint32 lim;

    if(!(Mapping = Dloc->DirIndex->FECharge.Mapping))
        return;

    lim = (Dloc->FE_Flags & UDF_FE_FLAG_IS_SDIR) ? Vcb->FEChargeSDir : Vcb->FECharge;

    if(!Discard) {
        // cache it!
        if(OS_SUCCESS(UDFStoreCachedAllocation(Vcb, 
                                 Dloc->FELoc.Mapping[0].extLocation,
                                 &Dloc->DirIndex->FECharge, lim, UDF_PREALLOC_CLASS_FE))) {
            Dloc->DirIndex->FECharge.Mapping = NULL;
            return;
        }
    }
    Dloc->DirIndex->FECharge.Mapping = NULL;
    UDFDiscardFESpace(Vcb, Mapping, lim);
#else // UDF_FE_ALLOCATION_CHARGE
    ASSERT(!Dloc->DirIndex->FECharge.Mapping);
    return;
#endif // UDF_FE_ALLOCATION_CHARGE
} // end UDFFlushFESpace()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine rebuilds mapping on write attempts to Alloc-Not-Rec area.
    Here we assume that required area lays in a single frag.
 */
OSSTATUS
UDFMarkAllocatedAsRecorded(
    IN PVCB Vcb,
    IN int64 Offset,
    IN uint32 Length,
    IN PEXTENT_INFO ExtInfo   // Extent array
    )
{
    uint32 i, len, lba, sLen;
    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    PEXTENT_MAP NewExtent;
    uint32 BS = Vcb->BlockSize;
    uint32 LBS = Vcb->LBlockSize;
    uint32 BSh = Vcb->BlockSizeBits;
    BOOLEAN TryPack = TRUE;
#ifdef UDF_DBG
    int64 check_size;
#endif //UDF_DBG
    // I don't know what else comment can be added here.
    // Just belive that it works
    lba = UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, (Offset & ~((int64)LBS-1)), NULL, NULL, NULL, &i);
    if(i == -1) return STATUS_INVALID_PARAMETER;
#ifdef UDF_DBG
    check_size = UDFGetExtentLength(ExtInfo->Mapping);
    ASSERT(!(check_size & (LBS-1)));
#endif //UDF_DBG
    AdPrint(("Alloc->Rec  ExtInfo %x, Extent %x\n", ExtInfo, Extent));
    if((Extent[i].extLength >> 30) == EXTENT_RECORDED_ALLOCATED) return STATUS_SUCCESS;
    if((Extent[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) return STATUS_INVALID_PARAMETER;
    ASSERT((((uint32)Offset) & (LBS-1)) + Length <= (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK));
    sLen = (( (((uint32)Offset) & (LBS-1)) + Length+LBS-1) & ~(LBS-1)) >> BSh;
    if((Extent[i].extLocation == lba) && (((Extent[i].extLength & UDF_EXTENT_LENGTH_MASK ) >> BSh) == sLen)) {
        // xxxxxx ->  RRRRRR
        Extent[i].extLength &= UDF_EXTENT_LENGTH_MASK;
//      Extent[i].extLength |= (EXTENT_RECORDED_ALLOCATED << 30); // = 0;
        ExtInfo->Modified = TRUE;
        if(i && 
           ((Extent[i-1].extLength >> 30) == EXTENT_RECORDED_ALLOCATED) &&
           (lba == (Extent[i-1].extLocation + ((len = Extent[i-1].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh))) &&
           ((len + (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK)) <= UDF_MAX_EXTENT_LENGTH) &&
           (i == ((UDFGetMappingLength(Extent) / sizeof(EXTENT_MAP)) - 2)) &&
           TRUE) {
            // make optimization for sequentially written files
            Extent[i-1].extLength += Extent[i].extLength;
            Extent[i].extLocation = 0;
            Extent[i].extLength = 0;
        } else {
            UDFPackMapping(Vcb, ExtInfo);
        }
        AdPrint(("Alloc->Rec (1) new %x\n", ExtInfo->Mapping));
        ASSERT(check_size == UDFGetExtentLength(ExtInfo->Mapping));
        AdPrint(("Alloc->Rec: ExtInfo %x, Extent %x\n", ExtInfo, ExtInfo->Mapping));
        return STATUS_SUCCESS;
    }
    if(Extent[i].extLocation < lba) {
        if(  (((Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) - (lba - Extent[i].extLocation))
             > sLen ) {
            // xxxxxx ->  xxRRxx
            NewExtent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , UDFGetMappingLength(Extent) + sizeof(EXTENT_MAP)*2,
                                                               MEM_EXTMAP_TAG);
            if(!NewExtent) return STATUS_INSUFFICIENT_RESOURCES;
            Extent[i].extLength &= UDF_EXTENT_LENGTH_MASK;
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+3]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLocation = Extent[i].extLocation;
            NewExtent[i].extLength = (lba - Extent[i].extLocation) << BSh;
            NewExtent[i+1].extLength = (Length+BS-1) & ~(BS-1);
            NewExtent[i+1].extLocation = lba;
            NewExtent[i+2].extLength = Extent[i].extLength - NewExtent[i].extLength - NewExtent[i+1].extLength;
            NewExtent[i+2].extLocation = lba + ((Length+BS-1) >> BSh);
            ASSERT(!(NewExtent[i].extLength >> 30));
            ASSERT(!(NewExtent[i+2].extLength >> 30));
            NewExtent[i].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
            NewExtent[i+2].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
            TryPack = FALSE;
            AdPrint(("Alloc->Rec (2) new %x\n", NewExtent));
        } else {
            // xxxxxx ->  xxRRRR
            NewExtent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , UDFGetMappingLength(Extent) + sizeof(EXTENT_MAP),
                                                               MEM_EXTMAP_TAG);
            if(!NewExtent) return STATUS_INSUFFICIENT_RESOURCES;
            Extent[i].extLength &= UDF_EXTENT_LENGTH_MASK;
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+2]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLocation = Extent[i].extLocation;
            NewExtent[i].extLength = (lba - Extent[i].extLocation) << BSh;
            NewExtent[i+1].extLength = Extent[i].extLength - NewExtent[i].extLength;
            NewExtent[i+1].extLocation = lba;
            ASSERT(!(NewExtent[i].extLength >> 30));
            NewExtent[i].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
            AdPrint(("Alloc->Rec (3) new %x\n", NewExtent));
        }
    } else {
        // xxxxxx ->  RRRRxx
        NewExtent = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , UDFGetMappingLength(Extent) + sizeof(EXTENT_MAP),
                                                           MEM_EXTMAP_TAG);
        if(!NewExtent) return STATUS_INSUFFICIENT_RESOURCES;
        Extent[i].extLength &= UDF_EXTENT_LENGTH_MASK;
        RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
        RtlCopyMemory((int8*)&(NewExtent[i+2]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
        NewExtent[i].extLocation = Extent[i].extLocation;
        NewExtent[i].extLength = (Length+BS-1) & ~(BS-1);
        NewExtent[i+1].extLength = Extent[i].extLength - NewExtent[i].extLength;
        NewExtent[i+1].extLocation = Extent[i].extLocation + (NewExtent[i].extLength >> BSh);
        ASSERT(!(NewExtent[i+1].extLength >> 30));
        NewExtent[i+1].extLength |= (EXTENT_NOT_RECORDED_ALLOCATED << 30);
        AdPrint(("Alloc->Rec (4) new %x\n", NewExtent));
    }

    //ASSERT(check_size == UDFGetExtentLength(Extent));
    //ASSERT(!(check_size & (LBS-1)));

    AdPrint(("Free Extent %x (new %x)\n", Extent, NewExtent));
    MyFreePool__(Extent);
    ExtInfo->Modified = TRUE;
    ExtInfo->Mapping = NewExtent;
    if(TryPack)
        UDFPackMapping(Vcb, ExtInfo);
    ASSERT(check_size == UDFGetExtentLength(ExtInfo->Mapping));
    ASSERT(!(check_size & (LBS-1)));

    AdPrint(("Alloc->Rec: ExtInfo %x, Extent %x\n", ExtInfo, ExtInfo->Mapping));

    return STATUS_SUCCESS;
} // end UDFMarkAllocatedAsRecorded()

/*
    This routine rebuilds mapping on write attempts to Not-Alloc-Not-Rec area.
    Here we assume that required area lays in a single frag.
 */
OSSTATUS
UDFMarkNotAllocatedAsAllocated(
    IN PVCB Vcb,
    IN int64 Offset,
    IN uint32 Length,
    IN PEXTENT_INFO ExtInfo   // Extent array
    )
{
    uint32 i, len, /*lba,*/ d, l, BOffs, j;
    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    PEXTENT_MAP NewExtent;
//    uint32 BS = Vcb->BlockSize;
    uint32 BSh = Vcb->BlockSizeBits;
    OSSTATUS status;
    EXTENT_INFO TmpExtInf;
    uint32 aLen, sLen;
    uint32 LBS = Vcb->LBlockSize;
    // I don't know what else comment can be added here.
    // Just belive that it works
    /*lba = */
#ifndef ALLOW_SPARSE
    BrutePoint();
#endif
    AdPrint(("Not->Alloc  ExtInfo %x, Extent %x\n", ExtInfo, Extent));
    UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, Offset, NULL, NULL, NULL, &i);
    if(i == -1) return STATUS_INVALID_PARAMETER;
    if((Extent[i].extLength >> 30) != EXTENT_NOT_RECORDED_NOT_ALLOCATED) return STATUS_SUCCESS;

    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, Extent[0].extLocation);
    BOffs = (uint32)(Offset >> BSh);
    // length of existing Not-Alloc-Not-Rec frag
    sLen = (( (((uint32)Offset) & (LBS-1)) + Length+LBS-1) & ~(LBS-1)) >> BSh;
    // required allocation length increment (in bytes)
    aLen = (uint32)( ((Offset+Length+LBS-1) & ~(LBS-1)) - (Offset & ~(LBS-1)));

    // try to extend previous frag or allocate space _after_ it to
    // avoid backward seeks, if previous frag is not Not-Rec-Not-Alloc
    if(i && ((Extent[i-1].extLength >> 30) != EXTENT_NOT_RECORDED_NOT_ALLOCATED) ) {
        status = UDFAllocFreeExtent(Vcb, aLen,
                                      Extent[i-1].extLocation + ((Extent[i-1].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh),
                                      min(UDFPartEnd(Vcb, PartNum), Extent[i-1].extLocation + ((Extent[i-1].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) + sLen ),
                                      &TmpExtInf, ExtInfo->Flags /*& EXTENT_FLAG_ALLOC_MASK*/);
        if(status == STATUS_DISK_FULL)
            // if there are not enough free blocks after that frag...
            goto try_alloc_anywhere;
    } else {
try_alloc_anywhere:
        // ... try to alloc required disk space anywhere
        status = UDFAllocFreeExtent(Vcb, aLen,
                                      UDFPartStart(Vcb, PartNum),
                                      UDFPartEnd(Vcb, PartNum),
                                      &TmpExtInf, ExtInfo->Flags /*& EXTENT_FLAG_ALLOC_MASK*/);
    }
    // check for successfull allocation
    if(!OS_SUCCESS(status)) {
        AdPrint(("Not->Alloc  no free\n"));
        return status;
    }
    // get number of frags in allocated block
    d = (UDFGetMappingLength(TmpExtInf.Mapping) / sizeof(EXTENT_MAP)) - 1;
    // calculate number of existing blocks before the frag to be changed
    l=0;
    for(j=0; j<i; j++) {
        l += (uint32)((Extent[j].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
    }
    // and now just update mapping...
    if( (l == BOffs) && (((Extent[j].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) == sLen) ) {
        // xxxxxx ->  RRRRRR
        // (d-1) - since we have to raplace last frag of Extent with 1 or more frags of TmpExtInf.Mapping
        NewExtent = (PEXTENT_AD)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + (d-1)*sizeof(EXTENT_MAP) );
        if(!NewExtent) {
            MyFreePool__(TmpExtInf.Mapping);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
        RtlCopyMemory((int8*)&(NewExtent[i]), (int8*)(TmpExtInf.Mapping), d*sizeof(EXTENT_MAP) );
        RtlCopyMemory((int8*)&(NewExtent[i+d]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
        AdPrint(("Not->Alloc (1) new %x\n", NewExtent));
    } else
    if(l < BOffs) {
        // .ExtLength, BOffs & l are already aligned...
        if( (((Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) - (BOffs-l)) > sLen ) {
            // xxxxxx ->  xxRRxx
            NewExtent = (PEXTENT_AD)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + (d+1)*sizeof(EXTENT_MAP) );
            if(!NewExtent) {
                MyFreePool__(TmpExtInf.Mapping);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+1]), (int8*)(TmpExtInf.Mapping), d*sizeof(EXTENT_MAP) );
            RtlCopyMemory((int8*)&(NewExtent[i+d+2]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLocation = 0;
            NewExtent[i].extLength = (BOffs - l) << BSh;
            NewExtent[i+d+1].extLength = Extent[i].extLength - NewExtent[i].extLength - aLen;
            NewExtent[i+d+1].extLocation = 0;
            NewExtent[i].extLength |= (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            NewExtent[i+d+1].extLength |= (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            AdPrint(("Not->Alloc (2) new %x\n", NewExtent));
        } else {
            // xxxxxx ->  xxRRRR
            NewExtent = (PEXTENT_AD)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + d*sizeof(EXTENT_MAP) );
            if(!NewExtent) {
                MyFreePool__(TmpExtInf.Mapping);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+1]), (int8*)(TmpExtInf.Mapping), d*sizeof(EXTENT_MAP) );
            RtlCopyMemory((int8*)&(NewExtent[i+d+1]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLocation = 0;
            NewExtent[i].extLength = (BOffs - l) << BSh;
            NewExtent[i].extLength |= (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            AdPrint(("Not->Alloc (3) new %x\n", NewExtent));
        }
    } else {
        // xxxxxx ->  RRRRxx
        NewExtent = (PEXTENT_AD)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + d*sizeof(EXTENT_MAP) );
        if(!NewExtent) {
            MyFreePool__(TmpExtInf.Mapping);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
        RtlCopyMemory((int8*)&(NewExtent[i]), (int8*)(TmpExtInf.Mapping), d*sizeof(EXTENT_MAP) );
        RtlCopyMemory((int8*)&(NewExtent[i+d+1]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
        NewExtent[i+d].extLength = (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) - aLen;
        NewExtent[i+d].extLocation = 0;
        NewExtent[i+d].extLength |= (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
        AdPrint(("Not->Alloc (4) new %x\n", NewExtent));
    }

    AdPrint(("Free Extent %x, TmpExtInf.Mapping, (new %x)\n", Extent, TmpExtInf.Mapping, NewExtent));
    MyFreePool__(Extent);
    MyFreePool__(TmpExtInf.Mapping);
    ExtInfo->Modified = TRUE;
    ExtInfo->Mapping = NewExtent;

    AdPrint(("Not->Alloc: ExtInfo %x, Extent %x\n", ExtInfo, ExtInfo->Mapping));

    return STATUS_SUCCESS;
} // end UDFMarkNotAllocatedAsAllocated()

//#if 0
/*
    This routine rebuilds mapping on write zero attempts to
    Alloc-Not-Rec area.
    Here we assume that required area lays in a single frag.
 */
OSSTATUS
UDFMarkAllocatedAsNotXXX(
    IN PVCB Vcb,
    IN int64 Offset,
    IN uint32 Length,
    IN PEXTENT_INFO ExtInfo,   // Extent array
    IN BOOLEAN Deallocate
    )
{
    uint32 i, len, /*lba, d,*/ l, BOffs, j;
    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    PEXTENT_MAP NewExtent;
//    EXTENT_MAP TmpExtent;
//    uint32 BS = Vcb->BlockSize;
    uint32 BSh = Vcb->BlockSizeBits;
//    OSSTATUS status;
    EXTENT_INFO TmpExtInf;
    uint32 aLen, sLen;
    uint32 flags;
    uint32 target_flags = Deallocate ?
                             EXTENT_NOT_RECORDED_NOT_ALLOCATED :
                             EXTENT_NOT_RECORDED_ALLOCATED;
    uint32 LBS = Vcb->LBlockSize;
    EXTENT_MAP DeadMapping[2];
    // I don't know what else comment can be added here.
    // Just belive that it works
    /*lba = */
#ifndef ALLOW_SPARSE
    if(Deallocate) {
        BrutePoint();
    }
#endif

    AdPrint(("Alloc->Not ExtInfo %x, Extent %x\n", ExtInfo, Extent));

    DeadMapping[0].extLocation =
        UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, Offset, NULL, NULL, NULL, &i);
    if(i == -1) {
        BrutePoint();
        return STATUS_INVALID_PARAMETER;
    }
    DeadMapping[0].extLength = Extent[i].extLength;
    DeadMapping[1].extLocation =
    DeadMapping[1].extLength = 0;
    TmpExtInf.Mapping = (PEXTENT_MAP)&DeadMapping;
    TmpExtInf.Offset = 0;
    TmpExtInf.Length = Extent[i].extLength & UDF_EXTENT_LENGTH_MASK;

    flags = Extent[i].extLength >> 30;
    if(flags == target_flags) return STATUS_SUCCESS;

//    uint32 PartNum = UDFGetPartNumByPhysLba(Vcb, Extent[0].extLocation);
    BOffs = (uint32)(Offset >> BSh);
    // length of existing Alloc-(Not-)Rec frag (in sectors)
    sLen = (( (((uint32)Offset) & (LBS-1)) + Length+LBS-1) & ~(LBS-1)) >> BSh;
    // required deallocation length increment (in bytes)
    aLen = (uint32)( ((Offset+Length+LBS-1) & ~(LBS-1)) - (Offset & ~(LBS-1)) );

    l=0;
    for(j=0; j<i; j++) {
        l += (uint32)((Extent[j].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
    }
    flags <<= 30;
    if( (l == BOffs) && (((Extent[j].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) == sLen) ) {
        // xxxxxx ->  RRRRRR
        Extent[i].extLocation = 0;
        Extent[i].extLength = (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) | flags;
        NewExtent = Extent;
        AdPrint(("Alloc->Not (1) NewExtent = Extent = %x\n", NewExtent));
    } else
    if(l < BOffs) {
        // .ExtLength, BOffs & l are already aligned...
        if( (((Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh) - (BOffs-l)) > sLen ) {
            // xxxxxx ->  xxRRxx
            NewExtent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + 2*sizeof(EXTENT_MAP) );
            if(!NewExtent) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+3]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLength = (BOffs - l) << BSh;
            NewExtent[i].extLength |= flags;
            NewExtent[i+1].extLocation = 0;
            NewExtent[i+1].extLength = aLen | (target_flags << 30);
            NewExtent[i+2].extLength = (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) -
                                       (NewExtent[i].extLength & UDF_EXTENT_LENGTH_MASK) - aLen ;
            NewExtent[i+2].extLocation = Extent[i].extLocation +
                                       (NewExtent[i+2].extLength >> BSh);
            NewExtent[i+2].extLength |= flags;
            AdPrint(("Alloc->Not (2) new %x\n", NewExtent));
        } else {
            // xxxxxx ->  xxRRRR
            NewExtent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + sizeof(EXTENT_MAP) );
            if(!NewExtent) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
            RtlCopyMemory((int8*)&(NewExtent[i+2]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
            NewExtent[i].extLength = ((BOffs - l) << BSh) | flags;
            NewExtent[i+1].extLocation = 0;
            NewExtent[i+1].extLength = aLen | (target_flags << 30);
            AdPrint(("Alloc->Not (3) new %x\n", NewExtent));
        }
    } else {
        // xxxxxx ->  RRRRxx
        NewExtent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, UDFGetMappingLength(Extent) + sizeof(EXTENT_MAP) );
        if(!NewExtent) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory((int8*)NewExtent, (int8*)Extent, i*sizeof(EXTENT_MAP));
        RtlCopyMemory((int8*)&(NewExtent[i+2]), (int8*)&(Extent[i+1]), len = UDFGetMappingLength(&(Extent[i+1])) );
        NewExtent[i+1].extLength = (Extent[i].extLength & UDF_EXTENT_LENGTH_MASK) - aLen;
        NewExtent[i+1].extLength |= flags;
        NewExtent[i].extLocation = 0;
        NewExtent[i].extLength = aLen | (target_flags << 30);
        AdPrint(("Alloc->Not (4) new %x\n", NewExtent));
    }

    if(Deallocate)
        UDFMarkSpaceAsXXX(Vcb, (-1), TmpExtInf.Mapping, AS_DISCARDED); // mark as free

    if(Extent) {
        AdPrint(("Alloc->Not kill %x\n", Extent));
        MyFreePool__(Extent);
    } else {
        AdPrint(("Alloc->Not keep %x\n", Extent));
    }
    ExtInfo->Modified = TRUE;
    ExtInfo->Mapping = NewExtent;
    AdPrint(("Alloc->Not: ExtInfo %x, Extent %x\n", ExtInfo, ExtInfo->Mapping));

    return STATUS_SUCCESS;
} // end UDFMarkAllocatedAsNotXXX()
//#endif //0

/*
    This routine resizes extent & updates associated mapping
 */
OSSTATUS
UDFResizeExtent(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN int64 Length,          // Required Length
    IN BOOLEAN AlwaysInIcb,   // must be TRUE for AllocDescs
    OUT PEXTENT_INFO ExtInfo
    )
{
    uint32 i, flags, lba, lim;
    int64 l;
    OSSTATUS status;
    EXTENT_INFO TmpExtInf;
    EXTENT_MAP  TmpMapping[2];
    uint32 s, req_s, pe, BSh, LBS, PS;
    LBS = Vcb->LBlockSize;
    BSh = Vcb->BlockSizeBits;
    PS = Vcb->WriteBlockSize >> Vcb->BlockSizeBits;
    uint32 MaxGrow = (UDF_MAX_EXTENT_LENGTH & ~(LBS-1));
    BOOLEAN Sequential = FALSE;

    ASSERT(PartNum < 3);

    ExtPrint(("Resize ExtInfo %x, %I64x -> %I64x\n", ExtInfo, ExtInfo->Length, Length));

    if(ExtInfo->Flags & EXTENT_FLAG_CUT_PREALLOCATED) {
        AdPrint(("  cut preallocated\n"));
    } else
    if(ExtInfo->Length == Length) {
        return STATUS_SUCCESS;
    }
    if((ExtInfo->Flags & EXTENT_FLAG_ALLOC_MASK) == EXTENT_FLAG_ALLOC_SEQUENTIAL) {
        MaxGrow &= ~(Vcb->WriteBlockSize-1);
        Sequential = TRUE;
    }

    UDFCheckSpaceAllocation(Vcb, 0, ExtInfo->Mapping, AS_USED); // check if used
    if(ExtInfo->Offset) {
        if(ExtInfo->Offset + Length <= LBS) {
            ExtPrint(("Resize IN-ICB\n"));
            ExtInfo->Length = Length;
            return STATUS_SUCCESS;
        }
        if(!AlwaysInIcb)           // simulate unused 1st sector in extent
            ExtInfo->Offset = LBS; // it'll be truncated later
        Length += ExtInfo->Offset; // convert to real offset in extent
    }
    lba = UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, Length, NULL, NULL, &flags, &i);
    if(ExtInfo->Length < Length) {
        // increase extent
        if(OS_SUCCESS(UDFGetCachedAllocation(Vcb, ExtInfo->Mapping[0].extLocation,
                              &TmpExtInf, NULL, UDF_PREALLOC_CLASS_DIR))) {
            AdPrint(("Resize found cached(1)\n"));
            ExtInfo->Mapping = UDFMergeMappings(ExtInfo->Mapping, TmpExtInf.Mapping);
            MyFreePool__(TmpExtInf.Mapping);
        }
        if((l = UDFGetExtentLength(ExtInfo->Mapping)) >= Length) {
            // we have enough space inside extent
            ExtInfo->Length = Length;
            AdPrint(("Resize do nothing (1)\n"));
        } else /*if(lba == LBA_OUT_OF_EXTENT)*/ {

            Length -= ExtInfo->Offset;
            if(/*Length && l &&*/  (l % MaxGrow) &&
               (Length-1)/MaxGrow != (l-1)/MaxGrow) {
                AdPrint(("Crossing MAX_FRAG boundary...\n"));
                int64 l2 = ((l-1)/MaxGrow + 1)*MaxGrow;
                status = UDFResizeExtent(Vcb, PartNum, l2, AlwaysInIcb, ExtInfo);
                if(!OS_SUCCESS(status)) {
                    KdPrint(("Sub-call to UDFResizeExtent() failed (%x)\n", status));
                    return status;
                }
                l = ExtInfo->Length;
                ASSERT(l == l2);
            }
            while((Length - l) > MaxGrow) {
                status = UDFResizeExtent(Vcb, PartNum, l+MaxGrow, AlwaysInIcb, ExtInfo);
                if(!OS_SUCCESS(status)) {
                    KdPrint(("Sub-call (2) to UDFResizeExtent() failed (%x)\n", status));
                    return status;
                }
                l = ExtInfo->Length;
            }
            Length += ExtInfo->Offset;
            // at first, try to resize existing frag
#ifndef UDF_ALLOW_FRAG_AD
            i = UDFGetMappingLength(ExtInfo->Mapping);
            if(i > (LBS-sizeof(EXTENDED_FILE_ENTRY))) {
                // this is very important check since we will not
                // be able to _record_ too long AllocDesc because of
                // some DEMO limitations in UDFBuildXXXAllocDescs()
                AdPrint(("  DISK_FULL\n"));
                return STATUS_DISK_FULL;
            }
            i /= sizeof(EXTENT_MAP);
#else //UDF_ALLOW_FRAG_AD
            i = UDFGetMappingLength(ExtInfo->Mapping) / sizeof(EXTENT_MAP);
#endif //UDF_ALLOW_FRAG_AD
#ifdef ALLOW_SPARSE
            if(!AlwaysInIcb && !(ExtInfo->Offset) &&
               (Length - l >= (Vcb->SparseThreshold << BSh))) {
                // last frag will be Not-Alloc-Not-Rec...
                AdPrint(("Resize sparse (2)\n"));
                RtlZeroMemory(&TmpExtInf, sizeof(EXTENT_INFO));
                TmpExtInf.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , sizeof(EXTENT_MAP)*2,
                                                                   MEM_EXTMAP_TAG);
                if(!TmpExtInf.Mapping) return STATUS_INSUFFICIENT_RESOURCES;
                TmpExtInf.Mapping[0].extLength = (((uint32)(Length - l) + LBS-1) & ~(LBS-1)) | (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
                TmpExtInf.Mapping[0].extLocation =// 0;
                TmpExtInf.Mapping[1].extLength =
                TmpExtInf.Mapping[1].extLocation = 0;
                l = Length;
                ExtInfo->Mapping = UDFMergeMappings(ExtInfo->Mapping, TmpExtInf.Mapping);
                MyFreePool__(TmpExtInf.Mapping);
            } else
#endif //ALLOW_SPARSE
            // allocate some sectors
            if(i>1 && !(ExtInfo->Offset)) {
                i-=2;
                // check if Not-Alloc-Not-Rec at the end of mapping
                if((uint32)Length - (uint32)l + (ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK) > MaxGrow) {
                    // do nothing, but jump directly to allocator
                } else
                if((ExtInfo->Mapping[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
                    AdPrint(("Resize grow sparse (3)\n"));
                    ExtInfo->Mapping[i].extLength +=
                        (((uint32)Length-(uint32)l+LBS-1) & ~(LBS-1)) ;
                    l = Length;
                // check if Alloc-Not-Rec at the end of mapping
                } else if((ExtInfo->Mapping[i].extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
                    AdPrint(("Resize grow Not-Rec (3)\n"));
                    // current length of last frag
                    s = ((ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
                    // prefered location of the next frag
                    lba = ExtInfo->Mapping[i].extLocation + s;
                    pe=UDFPartEnd(Vcb,PartNum);
                    // maximum frag length
                    if(Sequential) {
                        lim = (((uint32)UDF_MAX_EXTENT_LENGTH) >> BSh) & ~(PS-1);
                    } else {
                        lim = (((uint32)UDF_MAX_EXTENT_LENGTH) >> BSh) & ~(LBS-1);
                    }
                    // required last extent length
                    req_s = s + (uint32)( (((Length + LBS - 1) & ~(LBS-1)) -
                                           ((l      + LBS - 1) & ~(LBS-1))   ) >> BSh);
                    if(lim > req_s) {
                        lim = req_s;
                    }
                    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
/*                    if((ExtInfo->Flags & EXTENT_FLAG_SEQUENTIAL) && 
                       ((Length & ~(PS-1)) > (l & ~(PS-1))) &&
                       TRUE) {
                        status = UDFResizeExtent(Vcb, PartNum, l+MaxGrow, AlwaysInIcb, ExtInfo);
                    }*/
                    // how many sectors we should add
                    req_s = lim - s;
                    ASSERT(req_s);
                    if((lba < pe) && UDFGetFreeBit(Vcb->FSBM_Bitmap, lba)) {
                        s += UDFGetBitmapLen((uint32*)(Vcb->FSBM_Bitmap), lba, min(pe, lba+req_s-1));
                    }
/*                    for(s1=lba; (s<lim) && (s1<pe) && UDFGetFreeBit(Vcb->FSBM_Bitmap, s1); s1++) {
                        s++;
                    }*/
                    if(s==lim) {
                        // we can just increase the last frag
                        AdPrint(("Resize grow last Not-Rec (4)\n"));
                        ExtInfo->Mapping[i].extLength = (lim << BSh) | (EXTENT_NOT_RECORDED_ALLOCATED << 30);
                        l = Length;
                        UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &(ExtInfo->Mapping[i]), AS_USED); // mark as used
                    } else {
                        // we get here if simple increasing of last frag failed
                        // it worth truncating last frag and try to allocate
                        // all required data as a single frag

/*                        if(Sequential && s>=PS) {
                            s &= ~(PS-1);
                            AdPrint(("Resize grow last Not-Rec (4/2)\n"));
                            ExtInfo->Mapping[i].extLength = (s << BSh) | (EXTENT_NOT_RECORDED_ALLOCATED << 30);
                            l += (s << BSh);
                            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &(ExtInfo->Mapping[i]), AS_USED); // mark as used
                        }*/
                        AdPrint(("Resize reloc last Not-Rec (5)\n"));
                        TmpExtInf.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , (i+1)*sizeof(EXTENT_MAP),
                                                                           MEM_EXTMAP_TAG);
                        if(!TmpExtInf.Mapping) {
                            KdPrint(("UDFResizeExtent: !TmpExtInf.Mapping\n"));
                            UDFReleaseResource(&(Vcb->BitMapResource1));
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        RtlCopyMemory(TmpExtInf.Mapping, ExtInfo->Mapping, i*sizeof(EXTENT_MAP));
                        TmpExtInf.Mapping[i].extLength =
                        TmpExtInf.Mapping[i].extLocation = 0;
                        TmpExtInf.Offset = ExtInfo->Offset;
                        l -= (ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK);
                        TmpExtInf.Length = l;
                        ASSERT(i || !ExtInfo->Offset);
                        UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &(ExtInfo->Mapping[i]), AS_DISCARDED); // mark as free
                        MyFreePool__(ExtInfo->Mapping);
                        (*ExtInfo) = TmpExtInf;
                    }
                    UDFCheckSpaceAllocation(Vcb, 0, ExtInfo->Mapping, AS_USED); // check if used
                    UDFReleaseResource(&(Vcb->BitMapResource1));
                // check if Alloc-Rec
                } else {
                    // current length of last frag
                    s = ((ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
                    // prefered location of the next frag
                    lba = ExtInfo->Mapping[i].extLocation + s;
                    pe=UDFPartEnd(Vcb,PartNum);
                    // maximum frag length
                    if(Sequential) {
                        lim = (((uint32)UDF_MAX_EXTENT_LENGTH) >> BSh) & ~(PS-1);
                    } else {
                        lim = (((uint32)UDF_MAX_EXTENT_LENGTH) >> BSh) & ~(LBS-1);
                    }
                    // required last extent length
                    req_s = s + (uint32)( (((Length + LBS - 1) & ~(LBS-1)) -
                                           ((l      + LBS - 1) & ~(LBS-1))   ) >> BSh);
                    if(lim > req_s) {
                        lim = req_s;
                    }
//                    s=0;
                    // how many sectors we should add
                    req_s = lim - s;
                    if(req_s) {
                        uint32 d=0;

                        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
                        //ASSERT(req_s);
                        if((lba < pe) && UDFGetFreeBit(Vcb->FSBM_Bitmap, lba)) {
                            s += (d = UDFGetBitmapLen((uint32*)(Vcb->FSBM_Bitmap), lba, min(pe, lba+req_s-1)));
                        }
    /*                    for(s1=lba; (s<lim) && (s1<pe) && UDFGetFreeBit(Vcb->FSBM_Bitmap, s1); s1++) {
                            s++;
                        }*/

                        if(s==lim) {
                            AdPrint(("Resize grow last Rec (6)\n"));
                            // we can just increase last frag
                            TmpMapping[0].extLength = req_s << BSh;
                            TmpMapping[0].extLocation = lba;
                            TmpMapping[1].extLength =
                            TmpMapping[1].extLocation = 0;
                            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &TmpMapping[0], AS_USED); // mark as used
                            l += (s << BSh) - (ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK);
                            ExtInfo->Mapping[i].extLength = (ExtInfo->Mapping[i].extLength & UDF_EXTENT_FLAG_MASK) | (s << BSh);
                        } else if(d) {
                            AdPrint(("Resize part-grow last Rec (6)\n"));
                            // increase last frag, then alloc rest
                            TmpMapping[0].extLength = d << BSh;
                            TmpMapping[0].extLocation = lba;
                            TmpMapping[1].extLength =
                            TmpMapping[1].extLocation = 0;
                            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &TmpMapping[0], AS_USED); // mark as used
                            l += (s << BSh) - (ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK);
                            ExtInfo->Mapping[i].extLength = (ExtInfo->Mapping[i].extLength & UDF_EXTENT_FLAG_MASK) | (s << BSh);
                        } else {
                            AdPrint(("Can't grow last Rec (6)\n"));
                        }
                        UDFReleaseResource(&(Vcb->BitMapResource1));
                    } else {
                        AdPrint(("Max frag length reached (6)\n"));
                    }
                }
            }
            if(l < Length) {
                // we get here if simple increasing of the last frag failed
                AdPrint(("Resize add new frag (7)\n"));
                if(l < LBS && Length >= LBS &&
                   (ExtInfo->Flags & EXTENT_FLAG_ALLOC_MASK) == EXTENT_FLAG_ALLOC_SEQUENTIAL) {
                    AdPrint(("Resize tune for SEQUENTIAL i/o\n"));
                }
                status = UDFAllocFreeExtent(Vcb, Length - l,
                                                   UDFPartStart(Vcb, PartNum),
                                                   UDFPartEnd(Vcb, PartNum),
                                                   &TmpExtInf,
                                                   ExtInfo->Flags /*& EXTENT_FLAG_ALLOC_MASK*/);
                if(!OS_SUCCESS(status)) {
                    KdPrint(("UDFResizeExtent: UDFAllocFreeExtent() failed (%x)\n", status));
                    return status;
                }
                ExtInfo->Mapping = UDFMergeMappings(ExtInfo->Mapping, TmpExtInf.Mapping);
                MyFreePool__(TmpExtInf.Mapping);
            }
            UDFPackMapping(Vcb, ExtInfo);
        }
    } else 
    if(Length) {
        // decrease extent
        AdPrint(("Resize cut (8)\n"));
        lba = UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, Length-1, NULL, &lim, &flags, &i);
        i++;
        ASSERT(lba != LBA_OUT_OF_EXTENT);
        ASSERT(lba != LBA_NOT_ALLOCATED);
        ASSERT(i);
        if(ExtInfo->Mapping[i].extLength) {
            UDFCheckSpaceAllocation(Vcb, 0, &(ExtInfo->Mapping[i]), AS_USED); // check if used
            if(!ExtInfo->Offset && (ExtInfo->Flags & EXTENT_FLAG_PREALLOCATED)) {

                AdPrint(("Resize try save cutted (8)\n"));
                RtlZeroMemory(&TmpExtInf, sizeof(EXTENT_INFO));
                s = UDFGetMappingLength(&(ExtInfo->Mapping[i]));

                TmpExtInf.Mapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , s, MEM_EXTMAP_TAG);
                if(TmpExtInf.Mapping) {
                    RtlCopyMemory(TmpExtInf.Mapping, &(ExtInfo->Mapping[i]), s);
                    AdPrint(("Resize save cutted (8)\n"));
                    if(OS_SUCCESS(UDFStoreCachedAllocation(Vcb, ExtInfo->Mapping[0].extLocation, 
                                               &TmpExtInf, 0, UDF_PREALLOC_CLASS_DIR))) {
                        ExtInfo->Mapping[i].extLength = 0;
                        ExtInfo->Mapping[i].extLocation = 0;
                        goto tail_cached;
                    }
                }
            }
            UDFMarkSpaceAsXXX(Vcb, 0, &(ExtInfo->Mapping[i]), AS_DISCARDED); // mark as free
tail_cached:;
        }
        if((lim-1 >= LBS) &&
           (flags != EXTENT_NOT_RECORDED_NOT_ALLOCATED)) {
            AdPrint(("i=%x, lba=%x, len=%x\n",i,lba,lim));
            ASSERT(lim);
//            BrutePoint();
            EXTENT_MAP ClrMap[2];
            ClrMap[0].extLength = lim & ~(LBS-1);
            s = (ExtInfo->Mapping[i-1].extLength - ClrMap[0].extLength) & UDF_EXTENT_LENGTH_MASK;
            ClrMap[0].extLocation = ExtInfo->Mapping[i-1].extLocation +
                                   (s >> BSh);
            ClrMap[1].extLength =
            ClrMap[1].extLocation = 0;
            ASSERT((ExtInfo->Mapping[i].extLocation <   ClrMap[0].extLocation) ||
                   (ExtInfo->Mapping[i].extLocation >= (ClrMap[0].extLocation + (ClrMap[0].extLength >> BSh))));
            UDFCheckSpaceAllocation(Vcb, 0, (PEXTENT_MAP)(&ClrMap), AS_USED); // check if used
            UDFMarkSpaceAsXXX(Vcb, 0, (PEXTENT_MAP)(&ClrMap), AS_DISCARDED); // mark as free
            ExtInfo->Mapping[i-1].extLength = s | (flags << 30);
        }

        s = UDFGetMappingLength(ExtInfo->Mapping);
        if(!MyReallocPool__((int8*)(ExtInfo->Mapping), s, (int8**)&(ExtInfo->Mapping), (i+1)*sizeof(EXTENT_MAP))) {
            // This must never happen on truncate !!!
            AdPrint(("ResizeExtent: MyReallocPool__(8) failed\n"));
        }
        ExtInfo->Mapping[i].extLength =
        ExtInfo->Mapping[i].extLocation = 0;
    } else {
        AdPrint(("Resize zero (9)\n"));
        ASSERT(!ExtInfo->Offset);
        UDFMarkSpaceAsXXX(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED); // mark as free
        s = UDFGetMappingLength(ExtInfo->Mapping);
        if(!MyReallocPool__((int8*)(ExtInfo->Mapping), s, (int8**)&(ExtInfo->Mapping), 2*sizeof(EXTENT_MAP))) {
            // This must never happen on truncate !!!
            AdPrint(("ResizeExtent: MyReallocPool__(9) failed\n"));
        }
        ExtInfo->Mapping[0].extLength = LBS | (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
        ExtInfo->Mapping[0].extLocation =
        ExtInfo->Mapping[1].extLength =
        ExtInfo->Mapping[1].extLocation = 0;
    }
    if(ExtInfo->Offset) {
        if(!AlwaysInIcb) {
            // remove 1st entry pointing to FileEntry
            s = UDFGetMappingLength(ExtInfo->Mapping);
            RtlMoveMemory(&(ExtInfo->Mapping[0]), &(ExtInfo->Mapping[1]), s - sizeof(EXTENT_MAP));
            if(!MyReallocPool__((int8*)(ExtInfo->Mapping), s,
                          (int8**)&(ExtInfo->Mapping), s - sizeof(EXTENT_MAP) )) {
                // This must never happen on truncate !!!
                AdPrint(("ResizeExtent: MyReallocPool__(10) failed\n"));
            }
            Length -= ExtInfo->Offset;
            ExtInfo->Offset = 0;
        } else {
            Length -= ExtInfo->Offset; // back to in-icb
        }
    }
    ExtInfo->Length = Length;
    UDFCheckSpaceAllocation(Vcb, 0, ExtInfo->Mapping, AS_USED); // check if used

    for(i=0; (ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK); i++) {
        ExtPrint(("Resized Ext: type %x, loc %x, len %x\n",
            ExtInfo->Mapping[i].extLength >> 30, ExtInfo->Mapping[i].extLocation, ExtInfo->Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK));
    }
    
    return STATUS_SUCCESS;
} // end UDFResizeExtent()

/*
    This routine (re)builds AllocDescs data for all allocation modes except
    in-ICB & resizes associated extent (FileInfo->Dloc->AllocLoc) for
    already allocated user data extent (FileInfo->Dloc->DataLoc).
    AllocMode in FileEntry pointed by FileInfo must be already initialized.
 */
OSSTATUS
UDFBuildAllocDescs(
    IN PVCB Vcb,
    IN uint32 PartNum,
 IN OUT PUDF_FILE_INFO FileInfo,
    OUT int8** AllocData
    )
{
    PEXTENT_MAP InMap;
//    uint32 i=0;
    int8* Allocs;
    uint16 AllocMode;
    uint32 InitSz;
    OSSTATUS status;

    ValidateFileInfo(FileInfo);
    AdPrint(("BuildAllocDesc\n"));
    // get space available in the 1st LBlock after FE
    InitSz = Vcb->LBlockSize - FileInfo->Dloc->FileEntryLen;
    Allocs = (int8*)MyAllocatePool__(NonPagedPool, InitSz);
    if(!Allocs) {
        AdPrint(("BuildAllocDesc: cant alloc %x bytes for Allocs\n", InitSz));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Allocs, InitSz);
    InMap = FileInfo->Dloc->DataLoc.Mapping;
    UDFCheckSpaceAllocation(Vcb, 0, InMap, AS_USED); // check if used

    // TODO: move data from mapped locations here

    AllocMode = ((PFILE_ENTRY)(FileInfo->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK;
    switch(AllocMode) {
    case ICB_FLAG_AD_IN_ICB: {
        MyFreePool__(Allocs);
        ASSERT(!FileInfo->Dloc->AllocLoc.Mapping);
        Allocs = NULL;
        status = STATUS_SUCCESS;
        break;
    }
    case ICB_FLAG_AD_SHORT: {
        status = UDFBuildShortAllocDescs(Vcb, PartNum, &Allocs, InitSz, FileInfo);
        break;
    }
    case ICB_FLAG_AD_LONG: {
        status = UDFBuildLongAllocDescs(Vcb, PartNum, &Allocs, InitSz, FileInfo);
        break;
    }
/*    case ICB_FLAG_AD_EXTENDED: {
        status = UDFBuildExtAllocDescs(Vcb, PartNum, &Allocs, InitSz, FileInfo);
        break;
    }*/
    default: {
        MyFreePool__(Allocs);
        Allocs = NULL;
        status = STATUS_INVALID_PARAMETER;
    }
    }

    *AllocData = Allocs;
    UDFCheckSpaceAllocation(Vcb, 0, FileInfo->Dloc->DataLoc.Mapping, AS_USED); // check if used

    return status;
} // end UDFBuildAllocDescs()

/*
    This routine discards file's allocation
 */
void
UDFFreeFileAllocation(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO DirInfo,
    IN PUDF_FILE_INFO FileInfo
    )
{
    if(FileInfo->Dloc->DataLoc.Offset) {
        // in-ICB data
        if(FileInfo->Dloc->DataLoc.Mapping) {
            ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation ==
                   FileInfo->Dloc->DataLoc.Mapping[0].extLocation);
            UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->DataLoc.Mapping[1]), AS_DISCARDED); // free
            FileInfo->Dloc->DataLoc.Mapping[1].extLocation =
            FileInfo->Dloc->DataLoc.Mapping[1].extLength = 0;
            FileInfo->Dloc->DataLoc.Mapping[0].extLocation = 0;
            FileInfo->Dloc->DataLoc.Mapping[0].extLength = EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30;
        }
        if(FileInfo->Dloc->AllocLoc.Mapping) {
            ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation ==
                   FileInfo->Dloc->AllocLoc.Mapping[0].extLocation);
            UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->AllocLoc.Mapping[1]), AS_DISCARDED); // free
            FileInfo->Dloc->AllocLoc.Mapping[1].extLocation =
            FileInfo->Dloc->AllocLoc.Mapping[1].extLength = 0;
            FileInfo->Dloc->AllocLoc.Mapping[0].extLocation = 0;
            FileInfo->Dloc->AllocLoc.Mapping[0].extLength = EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30;
        }
        UDFFreeFESpace(Vcb, DirInfo, &(FileInfo->Dloc->FELoc));
    } else {
        if(FileInfo->Dloc->AllocLoc.Mapping) {
            ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation ==
                   FileInfo->Dloc->AllocLoc.Mapping[0].extLocation);
            UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, &(FileInfo->Dloc->AllocLoc.Mapping[1]), AS_DISCARDED); // free
            FileInfo->Dloc->AllocLoc.Mapping[1].extLocation =
            FileInfo->Dloc->AllocLoc.Mapping[1].extLength = 0;
            FileInfo->Dloc->AllocLoc.Mapping[0].extLocation = 0;
            FileInfo->Dloc->AllocLoc.Mapping[0].extLength = EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30;
        }
        UDFFreeFESpace(Vcb, DirInfo, &(FileInfo->Dloc->FELoc));
        UDFMarkSpaceAsXXX(Vcb, FileInfo->Dloc, FileInfo->Dloc->DataLoc.Mapping, AS_DISCARDED); // free
    }
    FileInfo->Dloc->DataLoc.Modified =
    FileInfo->Dloc->AllocLoc.Modified =
    FileInfo->Dloc->FELoc.Modified = FALSE;
} // end UDFFreeFileAllocation()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine packs physically sequential extents into single one
 */
void
__fastcall
UDFPackMapping(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo   // Extent array
    )
{
    PEXTENT_MAP NewMap, OldMap;
    uint32 i, j, l;
    uint32 LastLba, LastType, OldLen;
    uint32 OldSize, NewSize;
#ifdef UDF_DBG
    int64 check_size;
#endif //UDF_DBG

    AdPrint(("Pack ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));
    AdPrint(("  Length %x\n", ExtInfo->Length));

    OldMap = ExtInfo->Mapping;
    LastLba = OldMap[0].extLocation;
    OldLen = (OldMap[0].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits;
    LastType = OldMap[0].extLength >> 30;
    OldSize =
    NewSize = UDFGetMappingLength(OldMap);
#ifdef UDF_DBG
    check_size = UDFGetExtentLength(ExtInfo->Mapping);
    ASSERT(!(check_size & (2048-1)));
#endif //UDF_DBG

    l=OldMap[0].extLength & UDF_EXTENT_LENGTH_MASK;
    // calculate required length
    for(i=1; OldMap[i].extLength; i++) {
        if((LastType == (OldMap[i].extLength >> 30))
            &&
           ((OldMap[i].extLocation == LastLba + OldLen) ||
            (!OldMap[i].extLocation && !LastLba && (LastType == EXTENT_NOT_RECORDED_NOT_ALLOCATED)))
            &&
           (l + (OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK) <= UDF_MAX_EXTENT_LENGTH)) {
            // we can pack two blocks in one
            l += OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK;
            NewSize -= sizeof(EXTENT_MAP);
        } else {
            l = OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK;
        }
        LastLba = OldMap[i].extLocation;
        LastType = OldMap[i].extLength >> 30;
        OldLen = (OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits;
    }
    // no changes ?
    if(OldSize <= (NewSize + PACK_MAPPING_THRESHOLD)) {
        if(OldSize == NewSize)
            return;
        if(NewSize >= PACK_MAPPING_THRESHOLD)
            return;
    }
    AdPrint(("Pack ExtInfo %x, Mapping %x, realloc\n", ExtInfo, ExtInfo->Mapping));
    NewMap = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , NewSize,
                                                       MEM_EXTMAP_TAG);
    // can't alloc ?
    if(!NewMap) return;
    // Ok, lets pack it...
    j=0;
    NewMap[0] = OldMap[0];
    LastLba = OldMap[0].extLocation;
    OldLen = (OldMap[0].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits;
    LastType = OldMap[0].extLength >> 30;
    for(i=1; OldMap[i].extLength; i++) {

        ExtPrint(("oShExt: type %x, loc %x, len %x\n",
            OldMap[i].extLength >> 30, OldMap[i].extLocation, OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK));

        if((LastType == (OldMap[i].extLength >> 30))
            &&
           ((OldMap[i].extLocation == LastLba + OldLen) ||
            (!OldMap[i].extLocation && !LastLba && (LastType == EXTENT_NOT_RECORDED_NOT_ALLOCATED)))
            &&
           ((NewMap[j].extLength & UDF_EXTENT_LENGTH_MASK) + (OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK) <= UDF_MAX_EXTENT_LENGTH)) {
            NewMap[j].extLength += OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK;
        } else {
            j++;
            NewMap[j] = OldMap[i];
        }

        ExtPrint(("nShExt: type %x, loc %x, len %x\n",
            NewMap[j].extLength >> 30, NewMap[j].extLocation, NewMap[j].extLength & UDF_EXTENT_LENGTH_MASK));

        LastLba = OldMap[i].extLocation;
        LastType = OldMap[i].extLength >> 30;
        OldLen = (OldMap[i].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits;
    }
    // write terminator
    j++;
    ASSERT(NewSize == (j+1)*sizeof(EXTENT_MAP));
    NewMap[j].extLength =
    NewMap[j].extLocation = 0;

    ASSERT(check_size == UDFGetExtentLength(ExtInfo->Mapping));
    ASSERT(check_size == UDFGetExtentLength(NewMap));

    AdPrint(("Pack ExtInfo %x, NewMap %x, OldMap %x\n", ExtInfo, NewMap, OldMap));
    
    ExtInfo->Mapping = NewMap;
    MyFreePool__(OldMap);

    AdPrint(("Pack ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));
    AdPrint(("  Length %x\n", ExtInfo->Length));
} // end UDFPackMapping()

/*
    This routine expands mapping to 'frag-per-LBlock' state
 */
OSSTATUS
__fastcall
UDFUnPackMapping(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo   // Extent array
    )
{
    PEXTENT_MAP NewMapping;
    PEXTENT_MAP Mapping = ExtInfo->Mapping;
    uint32 LBS = Vcb->LBlockSize;
    uint32 len = (uint32)(UDFGetExtentLength(Mapping) >> Vcb->LBlockSizeBits);
    uint32 i,j, type, base, d;
    LONG l;

    NewMapping = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , (len+1)*sizeof(EXTENT_MAP),
                                                       MEM_EXTMAP_TAG);
    if(!NewMapping) return STATUS_INSUFFICIENT_RESOURCES;

    j=0;
    d = LBS >> Vcb->BlockSizeBits;
    for(i=0; l = (Mapping[i].extLength & UDF_EXTENT_LENGTH_MASK); i++) {
        base = Mapping[i].extLocation;
        type = Mapping[i].extLength & UDF_EXTENT_FLAG_MASK;
        for(; l>=(LONG)LBS; j++) {
            NewMapping[j].extLength = LBS | type;
            NewMapping[j].extLocation = base;
            base+=d;
            l-=LBS;
        }
    }
    // record terminator
    ASSERT(NewMapping);
    RtlZeroMemory(&(NewMapping[j]), sizeof(EXTENT_MAP));
    MyFreePool__(Mapping);
    ExtInfo->Mapping = NewMapping;

    return STATUS_SUCCESS;
} // end UDFUnPackMapping()

/*
    Relocate a part of extent that starts from relative (inside extent)
    block number 'ExtBlock' and has length of 'BC' blocks to continuous
    run which starts at block 'Lba'
 */
OSSTATUS
UDFRelocateExtent(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,
    IN uint32 ExtBlock,
    IN uint32 Lba,
    IN uint32 BC
    )
{
    return STATUS_ACCESS_DENIED;
}

/*
    This routine checks if all the data required is in cache.
 */
BOOLEAN
UDFIsExtentCached(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,   // Extent array
    IN int64 Offset,      // offset in extent
    IN uint32 Length,
    IN BOOLEAN ForWrite
    )
{
    BOOLEAN retstat = FALSE;
    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_read, Lba, sect_offs, flags, i;

    WCacheStartDirect__(&(Vcb->FastCache), Vcb, TRUE/*FALSE/*ForWrite*/);
    if(!ExtInfo || !ExtInfo->Mapping) goto EO_IsCached;
    if(!Length) {
        retstat = TRUE;
        goto EO_IsCached;
    }

    // prevent reading out of data space
    if(Offset > ExtInfo->Length) goto EO_IsCached;
    if(Offset+Length > ExtInfo->Length) goto EO_IsCached;
    Offset += ExtInfo->Offset;               // used for in-ICB data
    // read maximal possible part of each frag of extent
    Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_read, &flags, &i);
    while(((LONG)Length) > 0) {
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) goto EO_IsCached;
        Extent += (i + 1);
        // check for reading tail
        to_read = min(to_read, Length);
        if(flags == EXTENT_RECORDED_ALLOCATED) {
            retstat = UDFIsDataCached(Vcb, Lba, (to_read+sect_offs+Vcb->BlockSize-1)>>Vcb->BlockSizeBits);
            if(!retstat) goto EO_IsCached;
        } else if(ForWrite) {
            goto EO_IsCached;
        }
        Offset += to_read;
        Length -= to_read;
        Lba = UDFNextExtentToLba(Vcb, Extent, &to_read, &flags, &i);
    }
    retstat = TRUE;
EO_IsCached:
    if(!retstat) {
        WCacheEODirect__(&(Vcb->FastCache), Vcb);
    }
    return retstat;
} // end UDFIsExtentCached()

/*
    This routine reads cached data only.
 */
/*OSSTATUS
UDFReadExtentCached(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,   // Extent array
    IN int64 Offset,      // offset in extent
    IN uint32 Length,
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{
    (*ReadBytes) = 0;
    if(!ExtInfo || !ExtInfo->Mapping) return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_read, Lba, sect_offs, flags, _ReadBytes;
    OSSTATUS status;
    // prevent reading out of data space
    if(Offset > ExtInfo->Length) return STATUS_END_OF_FILE;
    if(Offset+Length > ExtInfo->Length) Length = (uint32)(ExtInfo->Length - Offset);
    Offset += ExtInfo->Offset;               // used for in-ICB data
    // read maximal possible part of each frag of extent
    while(((LONG)Length) > 0) {
        Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_read, &flags, NULL);
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) return STATUS_END_OF_FILE;
        // check for reading tail
        to_read = (to_read < Length) ?
                   to_read : Length;
        if(flags == EXTENT_RECORDED_ALLOCATED) {
            status = UDFReadDataCached(Vcb, TRUE, ( ((uint64)Lba) << Vcb->BlockSizeBits) + sect_offs, to_read, Buffer, &_ReadBytes);
            (*ReadBytes) += _ReadBytes;
        } else {
            RtlZeroMemory(Buffer, to_read);
            (*ReadBytes) += to_read;
            status = STATUS_SUCCESS;
        }
        if(!OS_SUCCESS(status)) return status;
        // prepare for reading next frag...
        Buffer += to_read;
        Offset += to_read;
        Length -= to_read;
    }
    return STATUS_SUCCESS;
} // end UDFReadExtentCached()*/

/*
    This routine reads data at any offset from specified extent.
 */
OSSTATUS
UDFReadExtent(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo, // Extent array
    IN int64 Offset,      // offset in extent
    IN uint32 Length,
    IN BOOLEAN Direct,
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{
    (*ReadBytes) = 0;
    if(!ExtInfo || !ExtInfo->Mapping) return STATUS_INVALID_PARAMETER;
    ASSERT((uint32)Buffer > 0x1000);

    AdPrint(("Read ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_read, Lba, sect_offs, flags, _ReadBytes;
    OSSTATUS status;
    // prevent reading out of data space
    if(Offset > ExtInfo->Length) return STATUS_END_OF_FILE;
    if(Offset+Length > ExtInfo->Length) Length = (uint32)(ExtInfo->Length - Offset);
    Offset += ExtInfo->Offset;               // used for in-ICB data
    // read maximal possible part of each frag of extent
    Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_read, &flags, &_ReadBytes);
    while(Length) {
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) return STATUS_END_OF_FILE;
        Extent += (_ReadBytes + 1);
        // check for reading tail
        to_read = min(to_read, Length);
        if(flags == EXTENT_RECORDED_ALLOCATED) {
            status = UDFReadData(Vcb, TRUE, ( ((uint64)Lba) << Vcb->BlockSizeBits) + sect_offs, to_read, Direct, Buffer, &_ReadBytes);
            (*ReadBytes) += _ReadBytes;
            if(!OS_SUCCESS(status)) return status;
        } else {
            RtlZeroMemory(Buffer, to_read);
            (*ReadBytes) += to_read;
        }
        // prepare for reading next frag...
        Length -= to_read;
        if(!Length)
            break;
        ASSERT(to_read);
        Buffer += to_read;
//        Offset += to_read;
        Lba = UDFNextExtentToLba(Vcb, Extent, &to_read, &flags, &_ReadBytes);
        sect_offs = 0;
    }
    return STATUS_SUCCESS;
} // end UDFReadExtent()

/*
    This routine reads and builds mapping for
    specified amount of data at any offset from specified extent.
    Size of output buffer is limited by *_SubExtInfoSz
 */
OSSTATUS
UDFReadExtentLocation(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,      // Extent array
    IN int64 Offset,              // offset in extent to start SubExtent from
    OUT PEXTENT_MAP* _SubExtInfo, // SubExtent mapping array
 IN OUT uint32* _SubExtInfoSz,    // IN:  maximum number of fragments to get
                                  // OUT: actually obtained fragments
    OUT int64* _NextOffset        // offset, caller can start from to continue
    )
{
    if(!ExtInfo || !ExtInfo->Mapping)
        return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    PEXTENT_MAP SubExtInfo;
    uint32 to_read, Lba, sect_offs, flags, Skip_MapEntries;
    int32 SubExtInfoSz = *_SubExtInfoSz;
    int64 Length;
    int64 NextOffset;
    OSSTATUS status = STATUS_BUFFER_OVERFLOW;

    (*_SubExtInfo) = NULL;
    (*_SubExtInfoSz) = 0;
    NextOffset = Offset;
    // prevent reading out of data space
    if(Offset >= ExtInfo->Length)
        return STATUS_END_OF_FILE;
    Length = ExtInfo->Length - Offset;
    Offset += ExtInfo->Offset;               // used for in-ICB data
    // read maximal possible part of each frag of extent
    SubExtInfo = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool , SubExtInfoSz*sizeof(EXTENT_MAP),
                                                       MEM_EXTMAP_TAG);
    (*_SubExtInfo) = SubExtInfo;
    if(!SubExtInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_read, &flags, &Skip_MapEntries);
    while(Length && SubExtInfoSz) {
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) {
            BrutePoint();
            return STATUS_END_OF_FILE;
        }
        Extent += (Skip_MapEntries + 1);
        // check for reading tail
        to_read = (int32)min((int64)to_read, Length);
        SubExtInfo->extLength   = to_read;
        if(flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            SubExtInfo->extLocation = LBA_NOT_ALLOCATED;
        } else
        if(flags == EXTENT_NOT_RECORDED_ALLOCATED) {
            ASSERT(!(Lba & 0x80000000));
            SubExtInfo->extLocation = Lba | 0x80000000;
        } else {
            SubExtInfo->extLocation = Lba;
        }
        (*_SubExtInfoSz)++;
        SubExtInfoSz--;
        NextOffset += to_read;
        // prepare for reading next frag...
        Length -= to_read;
        if(!Length) {
            status = STATUS_SUCCESS;
            break;
        }
        ASSERT(to_read);
        Lba = UDFNextExtentToLba(Vcb, Extent, &to_read, &flags, &Skip_MapEntries);
        sect_offs = 0;
    }
    (*_NextOffset) = NextOffset;
    return STATUS_SUCCESS;
} // end UDFReadExtentLocation()

#pragma warning(push)               
#pragma warning(disable:4035)               // re-enable below

uint32
UDFGetZeroLength(
    IN int8* Buffer,
    IN uint32 Length
    )
{
    uint32 i;
    Length /= sizeof(uint32);
    for(i=0; i<Length; i++) {
        if( ((uint32*)Buffer)[i] )
            break;
    }
    return Length*sizeof(uint32);
}

#pragma warning(pop) // re-enable warning #4035

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine writes data at any offset to specified extent.
 */
OSSTATUS
UDFWriteExtent(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,   // Extent array
    IN int64 Offset,        // offset in extent
    IN uint32 Length,
    IN BOOLEAN Direct,         // setting this flag delays flushing of given
                               // data to indefinite term
    IN int8* Buffer,
    OUT uint32* WrittenBytes
    )
{
    if(!ExtInfo || !ExtInfo->Mapping)
        return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_write, Lba, sect_offs, flags;
    OSSTATUS status;
    uint32 _WrittenBytes;
    BOOLEAN reread_lba;
//    BOOLEAN already_prepared = FALSE;
//    BOOLEAN prepare = !Buffer;

    AdPrint(("Write ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));

    Offset += ExtInfo->Offset;               // used for in-ICB data
    // write maximal possible part of each frag of extent
    while(((LONG)Length) > 0) {
        UDFCheckSpaceAllocation(Vcb, 0, Extent, AS_USED); // check if used
        Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) {
            return STATUS_END_OF_FILE;
        }
/*        if((to_write < Length) &&
           !Direct && !prepare && !already_prepared) {
            // rebuild mapping, allocate space, etc.
            // to indicate this, set Buffer to NULL
            AdPrint(("UDFWriteExtent: Prepare\n"));
            BrutePoint();
            _WrittenBytes = 0;
            status = UDFWriteExtent(Vcb, ExtInfo, Offset, Length, *//*Direct*//*FALSE, NULL, &_WrittenBytes);
            if(!OS_SUCCESS(status)) {
                return status;
            }
            Extent = ExtInfo->Mapping;
            Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
            already_prepared = TRUE;
        }*/
        if(flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // here we should allocate space for this extent
            if(!OS_SUCCESS(status = UDFMarkNotAllocatedAsAllocated(Vcb, Offset, to_write, ExtInfo)))
                return status;
            Extent = ExtInfo->Mapping;
            UDFCheckSpaceAllocation(Vcb, 0, Extent, AS_USED); // check if used
            Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
            if(Lba == LBA_OUT_OF_EXTENT) {
                return STATUS_END_OF_FILE;
            }
            // we have already re-read Lba
            reread_lba = FALSE;
        } else {
            // we may need to re-read Lba if some changes are
            // made while converting from Alloc-Not-Rec
            reread_lba = TRUE;
        }
        // check if writing to not recorded allocated
        // in this case we must pad blocks with zeros around
        // modified area
        //
        // ...|xxxxxxxx|xxxxxxxx|xxxxxxxx|...
        //        .                .
        //        .     ||         .
        //        .     \/         .
        //        .                .
        // ...|000ddddd|dddddddd|dd000000|...
        //        .                .
        //        ^                ^
        //        sect_offs        sect_offs+to_write
        //        .                .
        //        .<-- to_write -->.
        //
        to_write = min(to_write, Length);
        if(flags == EXTENT_NOT_RECORDED_ALLOCATED) {
            if(!OS_SUCCESS(status = UDFMarkAllocatedAsRecorded(Vcb, Offset, to_write, ExtInfo)))
                return status;
            Extent = ExtInfo->Mapping;
            UDFCheckSpaceAllocation(Vcb, 0, Extent, AS_USED); // check if used
            if(reread_lba) {
                Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
                to_write = min(to_write, Length);
            }
            /*
              we must fill 1st block with zeros in 1 of 2 cases:
                1) start offset is not aligned on LBlock boundary
                      OR
                2) end offset is not aligned on LBlock boundary and lays in
                   the same LBlock

              we must fill last block with zeros if both
                1) end offset is not aligned on LBlock boundary
                      AND
                2) end offset DOESN'T lay in the 1st LBlock
            */

//            if(!prepare) {
                // pad 1st logical block
            if((sect_offs || (sect_offs + to_write < Vcb->LBlockSize) )
                                &&
                           !Vcb->CDR_Mode) {
                status = UDFWriteData(Vcb, TRUE,
                                      ( ((uint64)Lba) << Vcb->BlockSizeBits), 
                                      Vcb->LBlockSize, Direct, Vcb->ZBuffer, &_WrittenBytes);
                if(!OS_SUCCESS(status))
                    return status;
            }
            // pad last logical block
            if((sect_offs + to_write > Vcb->LBlockSize) &&
               (sect_offs + to_write) & (Vcb->LBlockSize - 1)) {
                status = UDFWriteData(Vcb, TRUE,
                                      (( ((uint64)Lba) << Vcb->BlockSizeBits) + sect_offs + to_write) & ~((int64)(Vcb->LBlockSize)-1),
                                      Vcb->LBlockSize, Direct, Vcb->ZBuffer, &_WrittenBytes);
            }
            if(!OS_SUCCESS(status))
                return status;
/*            } else {
                status = STATUS_SUCCESS;
            }*/
        }
        ASSERT(to_write);
//        if(!prepare) {
        status = UDFWriteData(Vcb, TRUE, ( ((uint64)Lba) << Vcb->BlockSizeBits) + sect_offs, to_write, Direct, Buffer, &_WrittenBytes);
        *WrittenBytes += _WrittenBytes;
        if(!OS_SUCCESS(status)) return status;
/*        } else {
            status = STATUS_SUCCESS;
            *WrittenBytes += to_write;
        }*/
        // prepare for writing next frag...
        Buffer += to_write;
        Offset += to_write;
        Length -= to_write;
    }
    AdPrint(("Write: ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));
    return STATUS_SUCCESS;
} // end UDFWriteExtent()

//#if 0
/*
    This routine zeroes/deallocates data at any offset to specified extent.
 */
OSSTATUS
UDFZeroExtent(
    IN PVCB Vcb,
    IN PEXTENT_INFO ExtInfo,   // Extent array
    IN int64 Offset,           // offset in extent
    IN uint32 Length,
    IN BOOLEAN Deallocate,     // deallocate frag or just mark as unrecorded
    IN BOOLEAN Direct,         // setting this flag delays flushing of given
                               // data to indefinite term
    OUT uint32* WrittenBytes
    )
{
    if(!ExtInfo || !ExtInfo->Mapping)
        return STATUS_INVALID_PARAMETER;

    PEXTENT_MAP Extent = ExtInfo->Mapping;   // Extent array
    uint32 to_write, Lba, sect_offs, flags;
    OSSTATUS status;
    uint32 _WrittenBytes;
    uint32 LBS = Vcb->LBlockSize;

    AdPrint(("Zero ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));

    Offset += ExtInfo->Offset;               // used for in-ICB data
    // fill/deallocate maximal possible part of each frag of extent
    while(((LONG)Length) > 0) {
        Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
        // EOF check
        if(Lba == LBA_OUT_OF_EXTENT) {
            return STATUS_END_OF_FILE;
        }
        // check for writing tail
        to_write = min(to_write, Length);

        if(flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // here we should do nothing
            *WrittenBytes += to_write;
        } else
        if(flags == EXTENT_NOT_RECORDED_ALLOCATED) {
            // we should just deallocate this frag
            if(Deallocate) {
                if(!OS_SUCCESS(status = UDFMarkAllocatedAsNotAllocated(Vcb, Offset, to_write, ExtInfo)))
                    return status;
            }
            Extent = ExtInfo->Mapping;
            *WrittenBytes += to_write;
        } else {
            // fill tail of the 1st Block with ZEROs
            if(sect_offs) {
                status = UDFWriteData(Vcb, TRUE, ( ((uint64)Lba) << Vcb->BlockSizeBits) + sect_offs,
                                      min(to_write, LBS-sect_offs),
                                      Direct, Vcb->ZBuffer, &_WrittenBytes);
                *WrittenBytes += _WrittenBytes;
                if(!OS_SUCCESS(status))
                    return status;
                Offset += _WrittenBytes;
                Length -= _WrittenBytes;
                to_write -= _WrittenBytes;
                Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
                ASSERT(flags != EXTENT_NOT_RECORDED_NOT_ALLOCATED);
                ASSERT(flags != EXTENT_NOT_RECORDED_ALLOCATED);
                ASSERT(!sect_offs);
            }
            // deallocate Blocks
            if(to_write >= LBS) {
                // use 'sect_offs' as length of extent to be deallocated
                sect_offs = to_write & ~(LBS - 1);
                if(Deallocate) {
                    status = UDFMarkAllocatedAsNotAllocated(Vcb, Offset, sect_offs, ExtInfo);
                } else {
                    status = UDFMarkRecordedAsAllocated(Vcb, Offset, sect_offs, ExtInfo);
                }
                if(!OS_SUCCESS(status))
                    return status;
                // reload extent mapping
                Extent = ExtInfo->Mapping;
                Offset += sect_offs;
                Length -= sect_offs;
                *WrittenBytes += sect_offs;
                to_write -= sect_offs;
                Lba = UDFExtentOffsetToLba(Vcb, Extent, Offset, &sect_offs, &to_write, &flags, NULL);
                ASSERT(flags != EXTENT_NOT_RECORDED_NOT_ALLOCATED);
                ASSERT(flags != EXTENT_NOT_RECORDED_ALLOCATED);
                ASSERT(!sect_offs);
            }
            // fill beginning of the last Block with ZEROs
            if(to_write) {
                status = UDFWriteData(Vcb, TRUE, ( ((uint64)Lba) << Vcb->BlockSizeBits), to_write, Direct, Vcb->ZBuffer, &_WrittenBytes);
                *WrittenBytes += _WrittenBytes;
                if(!OS_SUCCESS(status))
                    return status;
                ASSERT(to_write == _WrittenBytes);
            }
        }
        AdPrint(("Zero... ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));
        // prepare for filling next frag...
        Offset += to_write;
        Length -= to_write;
    }
    AdPrint(("Zero: ExtInfo %x, Mapping %x\n", ExtInfo, ExtInfo->Mapping));
    return STATUS_SUCCESS;
} // end UDFZeroExtent()
//#endif //0
#endif //UDF_READ_ONLY_BUILD
