////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __UDF_STRUCT_SUPPORT_H__
#define __UDF_STRUCT_SUPPORT_H__

#include "ecma_167.h"
#include "osta_misc.h"
#include "udf_rel.h"
#include "wcache.h"

// memory re-allocation (returns new buffer size)
uint32    UDFMemRealloc(IN int8* OldBuff,     // old buffer
                       IN uint32 OldLength,   // old buffer size
                       OUT int8** NewBuff,   // address to store new pointer
                       IN uint32 NewLength);  // required size
// convert offset in extent to Lba & calculate block parameters
// it also returns pointer to last valid entry & flags
uint32
UDFExtentOffsetToLba(IN PVCB Vcb,
                     IN PEXTENT_AD Extent,   // Extent array
                     IN int64 Offset,     // offset in extent
                     OUT uint32* SectorOffset,
                     OUT uint32* AvailLength, // available data in this block
                     OUT uint32* Flags,
                     OUT uint32* Index);

// locate frag containing specified Lba in extent
ULONG
UDFLocateLbaInExtent(
    IN PVCB Vcb,
    IN PEXTENT_MAP Extent,   // Extent array
    IN lba_t lba
    );

// see udf_rel.h
//#define LBA_OUT_OF_EXTENT       ((LONG)(-1))
//#define LBA_NOT_ALLOCATED       ((LONG)(-2))

// read data at any offset from extent
OSSTATUS UDFReadExtent(IN PVCB Vcb,
                       IN PEXTENT_INFO ExtInfo, // Extent array
                       IN int64 Offset,   // offset in extent
                       IN uint32 Length,
                       IN BOOLEAN Direct,
                       OUT int8* Buffer,
                       OUT uint32* ReadBytes);
// builds mapping for specified amount of data at any offset from specified extent.
OSSTATUS
UDFReadExtentLocation(IN PVCB Vcb,
                      IN PEXTENT_INFO ExtInfo,      // Extent array
                      IN int64 Offset,              // offset in extent to start SubExtent from
                      OUT PEXTENT_MAP* _SubExtInfo, // SubExtent mapping array
                   IN OUT uint32* _SubExtInfoSz,     // IN:  maximum number fragments to get
                                                    // OUT: actually obtained fragments
                      OUT int64* _NextOffset        // offset, caller can start from to continue
                      );
// calculate total length of extent
int64 UDFGetExtentLength(IN PEXTENT_MAP Extent);  // Extent array
// convert compressed Unicode to standard
void 
__fastcall UDFDecompressUnicode(IN OUT PUNICODE_STRING UName,
                              IN uint8* CS0,
                              IN uint32 Length,
                              OUT uint16* valueCRC);
// calculate hashes for directory search
uint8    UDFBuildHashEntry(IN PVCB Vcb,
                           IN PUNICODE_STRING Name,
                          OUT PHASH_ENTRY hashes,
                           IN uint8 Mask);

#define HASH_POSIX 0x01
#define HASH_ULFN  0x02
#define HASH_DOS   0x04
#define HASH_ALL   0x07
#define HASH_KEEP_NAME 0x08  // keep DOS '.' and '..' intact

// get dirindex's frame
PDIR_INDEX_ITEM UDFDirIndexGetFrame(IN PDIR_INDEX_HDR hDirNdx,
                                    IN uint32 Frame,
                                   OUT uint32* FrameLen,
                                   OUT uint_di* Index,
                                    IN uint_di Rel);
// release DirIndex
void UDFDirIndexFree(PDIR_INDEX_HDR hDirNdx);
// grow DirIndex
OSSTATUS UDFDirIndexGrow(IN PDIR_INDEX_HDR* _hDirNdx,
                         IN uint_di d);
// truncate DirIndex
OSSTATUS UDFDirIndexTrunc(IN PDIR_INDEX_HDR* _hDirNdx,
                          IN uint_di d);
// init variables for scan (using knowledge about internal structure)
BOOLEAN UDFDirIndexInitScan(IN PUDF_FILE_INFO DirInfo,   //
                           OUT PUDF_DIR_SCAN_CONTEXT Context,
                            IN uint_di Index);
//
PDIR_INDEX_ITEM UDFDirIndexScan(PUDF_DIR_SCAN_CONTEXT Context,
                                PUDF_FILE_INFO* _FileInfo);
// build directory index
OSSTATUS UDFIndexDirectory(IN PVCB Vcb,
                        IN OUT PUDF_FILE_INFO FileInfo);
// search for specified file in specified directory &
// returns corresponding offset in extent if found.
OSSTATUS UDFFindFile(IN PVCB Vcb,
                     IN BOOLEAN IgnoreCase,
                     IN BOOLEAN NotDeleted,
                     IN PUNICODE_STRING Name,
                     IN PUDF_FILE_INFO DirInfo,
                  IN OUT uint_di* Index);

__inline OSSTATUS UDFFindFile__(IN PVCB Vcb,
                                IN BOOLEAN IgnoreCase,
                                IN PUNICODE_STRING Name,
                                IN PUDF_FILE_INFO DirInfo)
{
    if(!DirInfo->Dloc->DirIndex)
        return STATUS_NOT_A_DIRECTORY;
    uint_di i=0;
    return UDFFindFile(Vcb, IgnoreCase, TRUE, Name, DirInfo, &i);
}

// calculate file mapping length (in bytes) including ZERO-terminator
uint32   UDFGetMappingLength(IN PEXTENT_MAP Extent);
// merge 2 sequencial file mappings
PEXTENT_MAP
__fastcall UDFMergeMappings(IN PEXTENT_MAP Extent,
                             IN PEXTENT_MAP Extent2);
// build file mapping according to ShortAllocDesc (SHORT_AD) array
PEXTENT_MAP UDFShortAllocDescToMapping(IN PVCB Vcb,
                                      IN uint32 PartNum,
                                      IN PLONG_AD AllocDesc,
                                      IN uint32 AllocDescLength,
                                      IN uint32 SubCallCount,
                                      OUT PEXTENT_INFO AllocLoc);
// build file mapping according to LongAllocDesc (LONG_AD) array
PEXTENT_MAP UDFLongAllocDescToMapping(IN PVCB Vcb,
                                      IN PLONG_AD AllocDesc,
                                      IN uint32 AllocDescLength,
                                      IN uint32 SubCallCount,
                                      OUT PEXTENT_INFO AllocLoc);
// build file mapping according to ExtendedAllocDesc (EXT_AD) array
PEXTENT_MAP UDFExtAllocDescToMapping(IN PVCB Vcb,
                                      IN PLONG_AD AllocDesc,
                                      IN uint32 AllocDescLength,
                                      IN uint32 SubCallCount,
                                      OUT PEXTENT_INFO AllocLoc);
// build file mapping according to (Extended)FileEntry
PEXTENT_MAP UDFReadMappingFromXEntry(IN PVCB Vcb,
                                     IN uint32 PartNum,
                                     IN tag* XEntry,
                                     IN OUT uint32* Offset,
                                     OUT PEXTENT_INFO AllocLoc);
// read FileEntry described in FileIdentDesc
OSSTATUS UDFReadFileEntry(IN PVCB Vcb,
//                          IN PFILE_IDENT_DESC FileDesc,
                          IN long_ad* Icb,
                       IN OUT PFILE_ENTRY FileEntry, // here we can also get ExtendedFileEntry
                       IN OUT uint16* Ident);
// scan FileSet sequence & return last valid FileSet
OSSTATUS UDFFindLastFileSet(IN PVCB Vcb,
                            IN lb_addr *Addr,  // Addr for the 1st FileSet
                            IN OUT PFILE_SET_DESC FileSetDesc);
// read all sparing tables & stores them in contiguos memory
OSSTATUS UDFLoadSparingTable(IN PVCB Vcb,
                              IN PSPARABLE_PARTITION_MAP PartMap);
// build mapping for extent
PEXTENT_MAP
__fastcall UDFExtentToMapping_(IN PEXTENT_AD Extent
#ifdef UDF_TRACK_EXTENT_TO_MAPPING
                              ,IN ULONG src,
                               IN ULONG line
#endif //UDF_TRACK_EXTENT_TO_MAPPING
                              );

#ifdef UDF_TRACK_EXTENT_TO_MAPPING
  #define UDFExtentToMapping(e)  UDFExtentToMapping_(e, UDF_BUG_CHECK_ID, __LINE__) 
#else //UDF_TRACK_EXTENT_TO_MAPPING
  #define UDFExtentToMapping(e)  UDFExtentToMapping_(e)
#endif //UDF_TRACK_EXTENT_TO_MAPPING

//    This routine remaps sectors from bad packet
OSSTATUS
__fastcall UDFRemapPacket(IN PVCB Vcb,
                        IN uint32 Lba,
                        IN BOOLEAN RemapSpared);

//    This routine releases sector mapping when entire packet is marked as free
OSSTATUS
__fastcall UDFUnmapRange(IN PVCB Vcb,
                        IN uint32 Lba,
                        IN uint32 BCount);

// return physical address for relocated sector
uint32
__fastcall UDFRelocateSector(IN PVCB Vcb,
                          IN uint32 Lba);
// check
BOOLEAN
__fastcall UDFAreSectorsRelocated(IN PVCB Vcb,
                                  IN uint32 Lba,
                                  IN uint32 BlockCount);
// build mapping for relocated extent
PEXTENT_MAP
__fastcall UDFRelocateSectors(IN PVCB Vcb,
                               IN uint32 Lba,
                               IN uint32 BlockCount);
// check for presence of given char among specified ones
BOOLEAN  UDFUnicodeInString(IN uint8* string,
                            IN WCHAR ch);     // Unicode char to search for.
// validate char
BOOLEAN 
__fastcall UDFIsIllegalChar(IN WCHAR ch);
// translate udfName to dosName using OSTA compliant.
#define  UDFDOSName__(Vcb, DosName, UdfName, FileInfo) \
    UDFDOSName(Vcb, DosName, UdfName, (FileInfo) && ((FileInfo)->Index < 2));

void 
__fastcall UDFDOSName(IN PVCB Vcb,
                    IN OUT PUNICODE_STRING DosName,
                    IN PUNICODE_STRING UdfName,
                    IN BOOLEAN KeepIntact);

void 
__fastcall UDFDOSName201(IN OUT PUNICODE_STRING DosName,
                       IN PUNICODE_STRING UdfName,
                       IN BOOLEAN KeepIntact);

void 
__fastcall UDFDOSName200(IN OUT PUNICODE_STRING DosName,
                       IN PUNICODE_STRING UdfName,
                       IN BOOLEAN KeepIntact,
                       IN BOOLEAN Mode150);

void 
__fastcall UDFDOSName100(IN OUT PUNICODE_STRING DosName,
                       IN PUNICODE_STRING UdfName,
                       IN BOOLEAN KeepIntact);

// return length of bit-chain starting from Offs bit
#ifdef _X86_
uint32
__stdcall
UDFGetBitmapLen(
#else   // NO X86 optimization , use generic C/C++
uint32    UDFGetBitmapLen(
#endif // _X86_
                         uint32* Bitmap,
                         uint32 Offs,
                         uint32 Lim);
// scan disc free space bitmap for minimal suitable extent
uint32    UDFFindMinSuitableExtent(IN PVCB Vcb,
                                   IN uint32 Length, // in blocks
                                   IN uint32 SearchStart,
                                   IN uint32 SearchLim,
                                   OUT uint32* MaxExtLen,
                                   IN uint8  AllocFlags);

#ifdef UDF_CHECK_DISK_ALLOCATION
// mark space described by Mapping as Used/Freed (optionaly)
void     UDFCheckSpaceAllocation_(IN PVCB Vcb,
                                  IN PEXTENT_MAP Map,
                                  IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
                                 ,IN uint32 FE_lba,
                                  IN uint32 BugCheckId,
                                  IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
                                  );

#ifdef UDF_TRACK_ONDISK_ALLOCATION
#define UDFCheckSpaceAllocation(Vcb, FileInfo, Map, asXXX) \
    UDFCheckSpaceAllocation_(Vcb, Map, asXXX, (uint32)FileInfo, UDF_BUG_CHECK_ID,__LINE__);
#else //UDF_TRACK_ONDISK_ALLOCATION
#define UDFCheckSpaceAllocation(Vcb, FileInfo, Map, asXXX) \
    UDFCheckSpaceAllocation_(Vcb, Map, asXXX);
#endif //UDF_TRACK_ONDISK_ALLOCATION
#else // UDF_CHECK_DISK_ALLOCATION
#define UDFCheckSpaceAllocation(Vcb, FileInfo, Map, asXXX) {;}
#endif //UDF_CHECK_DISK_ALLOCATION

// mark space described by Mapping as Used/Freed (optionaly)
// this routine doesn't acquire any resource
void
UDFMarkSpaceAsXXXNoProtect_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    );

#ifdef UDF_TRACK_ONDISK_ALLOCATION
#define UDFMarkSpaceAsXXXNoProtect(Vcb, FileInfo, Map, asXXX) \
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX, (uint32)FileInfo, UDF_BUG_CHECK_ID,__LINE__);
#else //UDF_TRACK_ONDISK_ALLOCATION
#define UDFMarkSpaceAsXXXNoProtect(Vcb, FileInfo, Map, asXXX) \
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX);
#endif //UDF_TRACK_ONDISK_ALLOCATION


// mark space described by Mapping as Used/Freed (optionaly)
void     UDFMarkSpaceAsXXX_(IN PVCB Vcb,
                            IN PEXTENT_MAP Map,
                            IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
                           ,IN uint32 FE_lba,
                            IN uint32 BugCheckId,
                            IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
                            );

#ifdef UDF_TRACK_ONDISK_ALLOCATION
#define UDFMarkSpaceAsXXX(Vcb, FileInfo, Map, asXXX) \
    UDFMarkSpaceAsXXX_(Vcb, Map, asXXX, (uint32)FileInfo, UDF_BUG_CHECK_ID,__LINE__);
#else //UDF_TRACK_ONDISK_ALLOCATION
#define UDFMarkSpaceAsXXX(Vcb, FileInfo, Map, asXXX) \
    UDFMarkSpaceAsXXX_(Vcb, Map, asXXX);
#endif //UDF_TRACK_ONDISK_ALLOCATION

#define AS_FREE         0x00
#define AS_USED         0x01
#define AS_DISCARDED    0x02
#define AS_BAD          0x04

// build mapping for Length bytes in FreeSpace
OSSTATUS UDFAllocFreeExtent_(IN PVCB Vcb,
                            IN int64 Length,
                            IN uint32 SearchStart,
                            IN uint32 SearchLim,
                            OUT PEXTENT_INFO Extent,
                            IN uint8 AllocFlags
#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
                           ,IN uint32 src,
                            IN uint32 line
#endif //UDF_TRACK_ALLOC_FREE_EXTENT
                            );

#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
#define UDFAllocFreeExtent(v, l, ss, sl, e, af)  UDFAllocFreeExtent_(v, l, ss, sl, e, af, UDF_BUG_CHECK_ID, __LINE__)
#else //UDF_TRACK_ALLOC_FREE_EXTENT
#define UDFAllocFreeExtent(v, l, ss, sl, e, af)  UDFAllocFreeExtent_(v, l, ss, sl, e, af)
#endif //UDF_TRACK_ALLOC_FREE_EXTENT
//

uint32 __fastcall
UDFGetPartFreeSpace(IN PVCB Vcb,
                           IN uint32 partNum);

#define UDF_PREALLOC_CLASS_FE    0x00
#define UDF_PREALLOC_CLASS_DIR   0x01

// try to find cached allocation
OSSTATUS
UDFGetCachedAllocation(
    IN PVCB Vcb,
    IN uint32 ParentLocation,
   OUT PEXTENT_INFO Ext,
   OUT uint32* Items, // optional
    IN uint32 AllocClass
    );
// put released pre-allocation to cache
OSSTATUS
UDFStoreCachedAllocation(
    IN PVCB Vcb,
    IN uint32 ParentLocation,
    IN PEXTENT_INFO Ext,
    IN uint32 Items,
    IN uint32 AllocClass
    );
// discard all cached allocations
OSSTATUS
UDFFlushAllCachedAllocations(
    IN PVCB Vcb,
    IN uint32 AllocClass
    );
// allocate space for FE
OSSTATUS UDFAllocateFESpace(IN PVCB Vcb,
                            IN PUDF_FILE_INFO DirInfo,
                            IN uint32 PartNum,
                            IN PEXTENT_INFO FEExtInfo,
                            IN uint32 Len);
#ifndef UDF_READ_ONLY_BUILD
// free space FE's allocation
void UDFFreeFESpace(IN PVCB Vcb,
                    IN PUDF_FILE_INFO DirInfo,
                    IN PEXTENT_INFO FEExtInfo);
#endif //UDF_READ_ONLY_BUILD

#define FLUSH_FE_KEEP       FALSE
#define FLUSH_FE_FOR_DEL    TRUE

// flush FE charge
void UDFFlushFESpace(IN PVCB Vcb,
                     IN PUDF_DATALOC_INFO Dloc,
                     IN BOOLEAN Discard = FLUSH_FE_KEEP);
// discard file allocation
void UDFFreeFileAllocation(IN PVCB Vcb,
                           IN PUDF_FILE_INFO DirInfo,
                           IN PUDF_FILE_INFO FileInfo);
// convert physical address to logical in specified partition
uint32    UDFPhysLbaToPart(IN PVCB Vcb,
                          IN uint32 PartNum,
                          IN uint32 Addr);
/*#define UDFPhysLbaToPart(Vcb, PartNum, Addr) \
    ((Addr - Vcb->Partitions[PartNum].PartitionRoot) >> Vcb->LB2B_Bits)*/
// initialize Tag structure.
void     UDFSetUpTag(IN PVCB Vcb,
                     IN tag* Tag,
                     IN uint16 DataLen,
                     IN uint32 TagLoc);
// build content for AllocDesc sequence for specified extent
OSSTATUS UDFBuildShortAllocDescs(IN PVCB Vcb,
                                 IN uint32 PartNum,
                                 OUT int8** Buff,  // data for AllocLoc
                                 IN uint32 InitSz,
                              IN OUT PUDF_FILE_INFO FileInfo);
// build data for AllocDesc sequence for specified
OSSTATUS UDFBuildLongAllocDescs(IN PVCB Vcb,
                                IN uint32 PartNum,
                                OUT int8** Buff,  // data for AllocLoc
                                IN uint32 InitSz,
                             IN OUT PUDF_FILE_INFO FileInfo);
// builds FileEntry & associated AllocDescs for specified extent.
OSSTATUS UDFBuildFileEntry(IN PVCB Vcb,
                           IN PUDF_FILE_INFO DirInfo,
                           IN PUDF_FILE_INFO FileInfo,
                           IN uint32 PartNum,
                           IN uint16 AllocMode, // short/long/ext/in-icb
                           IN uint32 ExtAttrSz,
                           IN BOOLEAN Extended/*,
                           OUT PFILE_ENTRY* FEBuff,
                           OUT uint32* FELen,
                           OUT PEXTENT_INFO FEExtInfo*/);
// find partition containing given physical sector
uint32
__fastcall UDFGetPartNumByPhysLba(IN PVCB Vcb,
                                IN uint32 Lba);
// add given bitmap to existing one
#define UDF_FSPACE_BM    0x00
#define UDF_ZSPACE_BM    0x01

OSSTATUS UDFAddXSpaceBitmap(IN PVCB Vcb,
                            IN uint32 PartNum,
                            IN PSHORT_AD bm,
                            IN ULONG bm_type);
// subtract given Bitmap to existing one
OSSTATUS UDFDelXSpaceBitmap(IN PVCB Vcb,
                            IN uint32 PartNum,
                            IN PSHORT_AD bm);
// build FreeSpaceBitmap (internal) according to media parameters & input data
OSSTATUS UDFBuildFreeSpaceBitmap(IN PVCB Vcb,
                                IN uint32 PartNdx,
                                IN PPARTITION_HEADER_DESC phd,
                                IN uint32 Lba);
// fill ExtentInfo for specified FileEntry
OSSTATUS UDFLoadExtInfo(IN PVCB Vcb,
                        IN PFILE_ENTRY fe,
                        IN PLONG_AD fe_loc,
                        IN OUT PEXTENT_INFO FExtInfo,
                        IN OUT PEXTENT_INFO AExtInfo);
// convert standard Unicode to compressed
void 
__fastcall UDFCompressUnicode(IN PUNICODE_STRING UName,
                            IN OUT uint8** _CS0,
                            IN OUT uint32* Length);
// build FileIdent for specified FileEntry.
OSSTATUS UDFBuildFileIdent(IN PVCB Vcb,
                           IN PUNICODE_STRING fn,
                           IN PLONG_AD FileEntryIcb,       // virtual address of FileEntry
                           IN uint32 ImpUseLen,
                           OUT PFILE_IDENT_DESC* _FileId,
                           OUT uint32* FileIdLen);
// rebuild mapping on write attempts to Alloc-Not-Rec area.
OSSTATUS UDFMarkAllocatedAsRecorded(IN PVCB Vcb,
                                    IN int64 Offset,
                                    IN uint32 Length,
                                    IN PEXTENT_INFO ExtInfo);   // Extent array
// rebuild mapping on write attempts to Not-Alloc-Not-Rec area
OSSTATUS UDFMarkNotAllocatedAsAllocated(IN PVCB Vcb,
                                        IN int64 Offset,
                                        IN uint32 Length,
                                        IN PEXTENT_INFO ExtInfo);   // Extent array
OSSTATUS UDFMarkAllocatedAsNotXXX(IN PVCB Vcb,
                                  IN int64 Offset,
                                  IN uint32 Length,
                                  IN PEXTENT_INFO ExtInfo,   // Extent array
                                  IN BOOLEAN Deallocate);
#ifdef DBG
__inline OSSTATUS UDFMarkAllocatedAsNotAllocated(IN PVCB Vcb,
                                  IN int64 Offset,
                                  IN uint32 Length,
                                  IN PEXTENT_INFO ExtInfo)
{
    return UDFMarkAllocatedAsNotXXX(Vcb, Offset, Length, ExtInfo, TRUE);
}
#else
#define UDFMarkAllocatedAsNotAllocated(Vcb, Off, Len, Ext) \
    UDFMarkAllocatedAsNotXXX(Vcb, Off, Len, Ext, TRUE)
#endif //DBG

#ifdef DBG
__inline OSSTATUS UDFMarkRecordedAsAllocated(IN PVCB Vcb,
                                  IN int64 Offset,
                                  IN uint32 Length,
                                  IN PEXTENT_INFO ExtInfo)
{
    return UDFMarkAllocatedAsNotXXX(Vcb, Offset, Length, ExtInfo, FALSE);
}
#else
#define UDFMarkRecordedAsAllocated(Vcb, Off, Len, Ext) \
    UDFMarkAllocatedAsNotXXX(Vcb, Off, Len, Ext, FALSE)
#endif //DBG
// write data at any offset from specified extent.
OSSTATUS UDFWriteExtent(IN PVCB Vcb,
                        IN PEXTENT_INFO ExtInfo,   // Extent array
                        IN int64 Offset,           // offset in extent
                        IN uint32 Length,
                        IN BOOLEAN Direct,         // setting this flag delays flushing of given
                                                   // data to indefinite term
                        IN int8* Buffer,
                        OUT uint32* WrittenBytes);

// deallocate/zero data at any offset from specified extent.
OSSTATUS UDFZeroExtent(IN PVCB Vcb,
                       IN PEXTENT_INFO ExtInfo,   // Extent array
                       IN int64 Offset,           // offset in extent
                       IN uint32 Length,
                       IN BOOLEAN Deallocate,     // deallocate frag or just mark as unrecorded
                       IN BOOLEAN Direct,         // setting this flag delays flushing of given
                                                  // data to indefinite term
                       OUT uint32* WrittenBytes);

#define UDFZeroExtent__(Vcb, Ext, Off, Len, Dir, WB) \
  UDFZeroExtent(Vcb, Ext, Off, Len, FALSE, Dir, WB)

#define UDFSparseExtent__(Vcb, Ext, Off, Len, Dir, WB) \
  UDFZeroExtent(Vcb, Ext, Off, Len, TRUE, Dir, WB)

uint32 
__fastcall UDFPartStart(PVCB Vcb,
                        uint32 PartNum);
uint32
__fastcall UDFPartEnd(PVCB Vcb,
                      uint32 PartNum);
// resize extent & associated mapping
OSSTATUS UDFResizeExtent(IN PVCB Vcb,
                         IN uint32 PartNum,
                         IN int64 Length,
                         IN BOOLEAN AlwaysInIcb,   // must be TRUE for AllocDescs
                         OUT PEXTENT_INFO ExtInfo);
// (re)build AllocDescs data  & resize associated extent
OSSTATUS UDFBuildAllocDescs(IN PVCB Vcb,
                            IN uint32 PartNum,
                         IN OUT PUDF_FILE_INFO FileInfo,
                            OUT int8** AllocData);
// set informationLength field in (Ext)FileEntry
void     UDFSetFileSize(IN PUDF_FILE_INFO FileInfo,
                        IN int64 Size);
// sync cached FileSize from DirNdx and actual FileSize from FE
void     UDFSetFileSizeInDirNdx(IN PVCB Vcb,
                                IN PUDF_FILE_INFO FileInfo,
                                IN int64* ASize);
// get informationLength field in (Ext)FileEntry
int64 UDFGetFileSize(IN PUDF_FILE_INFO FileInfo);
//
int64 UDFGetFileSizeFromDirNdx(IN PVCB Vcb,
                                  IN PUDF_FILE_INFO FileInfo);
// set lengthAllocDesc field in (Ext)FileEntry
void     UDFSetAllocDescLen(IN PVCB Vcb,
                            IN PUDF_FILE_INFO FileInfo);
// change fileLinkCount field in (Ext)FileEntry
void     UDFChangeFileLinkCount(IN PUDF_FILE_INFO FileInfo,
                                IN BOOLEAN Increase);
#define  UDFIncFileLinkCount(fi)  UDFChangeFileLinkCount(fi, TRUE)
#define  UDFDecFileLinkCount(fi)  UDFChangeFileLinkCount(fi, FALSE)
// ee
void     UDFSetEntityID_imp_(IN EntityID* eID,
                             IN uint8* Str,
                             IN uint32 Len);

// get fileLinkCount field from (Ext)FileEntry
uint16   UDFGetFileLinkCount(IN PUDF_FILE_INFO FileInfo);
#ifdef UDF_CHECK_UTIL
// set fileLinkCount field in (Ext)FileEntry
void
UDFSetFileLinkCount(
    IN PUDF_FILE_INFO FileInfo,
    uint16 LinkCount
    );
#endif //UDF_CHECK_UTIL

#define  UDFSetEntityID_imp(eID, Str) \
    UDFSetEntityID_imp_(eID, (uint8*)(Str), sizeof(Str));
//
void     UDFReadEntityID_Domain(PVCB Vcb,
                                EntityID* eID);
// get lengthExtendedAttr field in (Ext)FileEntry
uint32    UDFGetFileEALength(IN PUDF_FILE_INFO FileInfo);
// set UniqueID field in (Ext)FileEntry
void     UDFSetFileUID(IN PVCB Vcb,
                       IN PUDF_FILE_INFO FileInfo);
// get UniqueID field in (Ext)FileEntry
int64 UDFGetFileUID(IN PUDF_FILE_INFO FileInfo);
// change counters in LVID
void  UDFChangeFileCounter(IN PVCB Vcb,
                           IN BOOLEAN FileCounter,
                           IN BOOLEAN Increase);
#define UDFIncFileCounter(Vcb) UDFChangeFileCounter(Vcb, TRUE, TRUE);
#define UDFDecFileCounter(Vcb) UDFChangeFileCounter(Vcb, TRUE, FALSE);
#define UDFIncDirCounter(Vcb)  UDFChangeFileCounter(Vcb, FALSE, TRUE);
#define UDFDecDirCounter(Vcb)  UDFChangeFileCounter(Vcb, FALSE, FALSE);
// write to file
OSSTATUS UDFWriteFile__(IN PVCB Vcb,
                        IN PUDF_FILE_INFO FileInfo,
                        IN int64 Offset,
                        IN uint32 Length,
                        IN BOOLEAN Direct,
                        IN int8* Buffer,
                        OUT uint32* WrittenBytes);
// mark file as deleted & decrease file link counter.
OSSTATUS UDFUnlinkFile__(IN PVCB Vcb,
                         IN PUDF_FILE_INFO FileInfo,
                         IN BOOLEAN FreeSpace);
// delete all files in directory (FreeSpace = TRUE)
OSSTATUS UDFUnlinkAllFilesInDir(IN PVCB Vcb,
                                IN PUDF_FILE_INFO DirInfo);
// init UDF_FILE_INFO structure for specifiend file
OSSTATUS UDFOpenFile__(IN PVCB Vcb,
                       IN BOOLEAN IgnoreCase,
                       IN BOOLEAN NotDeleted,
                       IN PUNICODE_STRING fn,
                       IN PUDF_FILE_INFO DirInfo,
                       OUT PUDF_FILE_INFO* _FileInfo,
                       IN uint_di* IndexToOpen);
// init UDF_FILE_INFO structure for root directory
OSSTATUS UDFOpenRootFile__(IN PVCB Vcb,
                          IN lb_addr* RootLoc,
                          OUT PUDF_FILE_INFO FileInfo);
// free all memory blocks referenced by given FileInfo
uint32    UDFCleanUpFile__(IN PVCB Vcb,
                          IN PUDF_FILE_INFO FileInfo);
#define  UDF_FREE_NOTHING     0x00
#define  UDF_FREE_FILEINFO    0x01
#define  UDF_FREE_DLOC        0x02
// create zero-sized file
OSSTATUS UDFCreateFile__(IN PVCB Vcb,
                         IN BOOLEAN IgnoreCase,
                         IN PUNICODE_STRING fn,
                         IN uint32 ExtAttrSz,
                         IN uint32 ImpUseLen,
                         IN BOOLEAN Extended,
                         IN BOOLEAN CreateNew,
                      IN OUT PUDF_FILE_INFO DirInfo,
                         OUT PUDF_FILE_INFO* _FileInfo);
// read data from file described with FileInfo
/*
    This routine reads data from file described by FileInfo
 */
__inline
OSSTATUS UDFReadFile__(IN PVCB Vcb,
                       IN PUDF_FILE_INFO FileInfo,
                       IN int64 Offset,   // offset in extent
                       IN uint32 Length,
                       IN BOOLEAN Direct,
                       OUT int8* Buffer,
                       OUT uint32* ReadBytes)
{
    ValidateFileInfo(FileInfo);

    return UDFReadExtent(Vcb, &(FileInfo->Dloc->DataLoc), Offset, Length, Direct, Buffer, ReadBytes);
} // end UDFReadFile__()*/

/*
    This routine reads data from file described by FileInfo
 */
__inline
OSSTATUS UDFReadFileLocation__(IN PVCB Vcb,
                               IN PUDF_FILE_INFO FileInfo,
                               IN int64 Offset,              // offset in extent to start SubExtent from
                               OUT PEXTENT_MAP* SubExtInfo,  // SubExtent mapping array
                            IN OUT uint32* SubExtInfoSz,      // IN:  maximum number fragments to get
                                                             // OUT: actually obtained fragments
                               OUT int64* NextOffset         // offset, caller can start from to continue
                               )
{
    ValidateFileInfo(FileInfo);

    return UDFReadExtentLocation(Vcb, &(FileInfo->Dloc->DataLoc), Offset, SubExtInfo, SubExtInfoSz, NextOffset);
} // end UDFReadFile__()*/

/*
#define UDFReadFile__(Vcb, FileInfo, Offset, Length, Direct, Buffer, ReadBytes)  \
    (UDFReadExtent(Vcb, &((FileInfo)->Dloc->DataLoc), Offset, Length, Direct, Buffer, ReadBytes))
*/

// zero data in file described by FileInfo
__inline
OSSTATUS UDFZeroFile__(IN PVCB Vcb,
                       IN PUDF_FILE_INFO FileInfo,
                       IN int64 Offset,   // offset in extent
                       IN uint32 Length,
                       IN BOOLEAN Direct,
                       OUT uint32* ReadBytes);
// make sparse area in file described by FileInfo
__inline
OSSTATUS UDFSparseFile__(IN PVCB Vcb,
                         IN PUDF_FILE_INFO FileInfo,
                         IN int64 Offset,   // offset in extent
                         IN uint32 Length,
                         IN BOOLEAN Direct,
                         OUT uint32* ReadBytes);
// pad sector tail with zeros
OSSTATUS UDFPadLastSector(IN PVCB Vcb,
                          IN PEXTENT_INFO ExtInfo);
// update AllocDesc sequence, FileIdent & FileEntry
OSSTATUS UDFCloseFile__(IN PVCB Vcb,
                        IN PUDF_FILE_INFO FileInfo);
// load specified bitmap.
OSSTATUS UDFPrepareXSpaceBitmap(IN PVCB Vcb,
                             IN OUT PSHORT_AD XSpaceBitmap,
                             IN OUT PEXTENT_INFO XSBMExtInfo,
                             IN OUT int8** XSBM,
                             IN OUT uint32* XSl);
// update Freed & Unallocated space bitmaps
OSSTATUS UDFUpdateXSpaceBitmaps(IN PVCB Vcb,
                                IN uint32 PartNum,
                                IN PPARTITION_HEADER_DESC phd); // partition header pointing to Bitmaps
// update Partition Desc & associated data structures
OSSTATUS UDFUpdatePartDesc(PVCB Vcb,
                           int8* Buf);
// update Logical volume integrity descriptor
OSSTATUS UDFUpdateLogicalVolInt(PVCB            Vcb,
                                BOOLEAN         Close);
// blank Unalloc Space Desc
OSSTATUS UDFUpdateUSpaceDesc(IN PVCB Vcb,
                             int8* Buf);
// update Volume Descriptor Sequence
OSSTATUS UDFUpdateVDS(IN PVCB Vcb,
                      IN uint32 block,
                      IN uint32 lastblock,
                      IN uint32 flags);
// rebuild & flushes all system areas
OSSTATUS UDFUmount__(IN PVCB Vcb);
// move file from DirInfo1 to DirInfo2 & renames it to fn
OSSTATUS UDFRenameMoveFile__(IN PVCB Vcb,
                             IN BOOLEAN IgnoreCase,
                          IN OUT BOOLEAN* Replace,   // replace if destination file exists
                             IN PUNICODE_STRING fn,  // destination
                         //    IN uint32 ExtAttrSz,
                          IN OUT PUDF_FILE_INFO DirInfo1,
                          IN OUT PUDF_FILE_INFO DirInfo2,
                          IN OUT PUDF_FILE_INFO FileInfo); // source (opened)
// change file size (on disc)
OSSTATUS UDFResizeFile__(IN PVCB Vcb,
                      IN OUT PUDF_FILE_INFO FileInfo,
                         IN int64 NewLength);
// transform zero-sized file to directory
OSSTATUS UDFRecordDirectory__(IN PVCB Vcb,
                           IN OUT PUDF_FILE_INFO DirInfo); // source (opened)
// remove all DELETED entries from Dir & resize it.
#ifndef UDF_READ_ONLY_BUILD
OSSTATUS UDFPackDirectory__(IN PVCB Vcb,
                         IN OUT PUDF_FILE_INFO FileInfo);   // source (opened)
// rebuild tags for all entries from Dir.
OSSTATUS
UDFReTagDirectory(IN PVCB Vcb,
                  IN OUT PUDF_FILE_INFO FileInfo);   // source (opened)
#endif //UDF_READ_ONLY_BUILD
// load VAT.
OSSTATUS UDFLoadVAT(IN PVCB Vcb,
                     IN uint32 PartNdx);
// get volume free space
int64
__fastcall UDFGetFreeSpace(IN PVCB Vcb);
// get volume total space
int64
__fastcall UDFGetTotalSpace(IN PVCB Vcb);
// get DirIndex for specified FileInfo
PDIR_INDEX_HDR UDFGetDirIndexByFileInfo(IN PUDF_FILE_INFO FileInfo);
// check if the file has been found is deleted
/*BOOLEAN  UDFIsDeleted(IN PDIR_INDEX_ITEM DirNdx);*/
#define UDFIsDeleted(DirNdx) \
    (((DirNdx)->FileCharacteristics & FILE_DELETED) ? TRUE : FALSE)
// check Directory flag
/*BOOLEAN  UDFIsADirectory(IN PUDF_FILE_INFO FileInfo);*/
#define UDFIsADirectory(FileInfo) \
    (((FileInfo) && ((FileInfo)->Dloc) && ((FileInfo)->Dloc->DirIndex || ((FileInfo)->FileIdent && ((FileInfo)->FileIdent->fileCharacteristics & FILE_DIRECTORY)))) ? TRUE : FALSE)
// calculate actual allocation size
/*int64 UDFGetFileAllocationSize(IN PVCB Vcb,
                                  IN PUDF_FILE_INFO FileInfo);*/
#define UDFGetFileAllocationSize(Vcb, FileInfo)  \
    (((FileInfo)->Dloc->DataLoc.Mapping) ? UDFGetExtentLength((FileInfo)->Dloc->DataLoc.Mapping) : Vcb->LBlockSize)
// check if the directory is empty
BOOLEAN  UDFIsDirEmpty(IN PDIR_INDEX_HDR hCurDirNdx);
// flush FE
OSSTATUS UDFFlushFE(IN PVCB Vcb,
                    IN PUDF_FILE_INFO FileInfo,
                    IN uint32 PartNum);
// flush FI
OSSTATUS UDFFlushFI(IN PVCB Vcb,
                    IN PUDF_FILE_INFO FileInfo,
                    IN uint32 PartNum);
// flush all metadata & update counters
OSSTATUS UDFFlushFile__(IN PVCB Vcb,
                        IN PUDF_FILE_INFO FileInfo,
                        IN ULONG FlushFlags = 0);
// check if the file is flushed
#define UDFIsFlushed(FI) \
    (   FI &&                    \
      !(FI->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) &&      \
      !(FI->Dloc->DataLoc.Modified) && \
      !(FI->Dloc->AllocLoc.Modified) &&\
      !(FI->Dloc->FELoc.Modified) &&   \
      !(UDFGetDirIndexByFileInfo(FI)[FI->Index].FI_Flags & UDF_FI_FLAG_FI_MODIFIED) )
// compare opened directories
BOOLEAN  UDFCompareFileInfo(IN PUDF_FILE_INFO f1,
                           IN PUDF_FILE_INFO f2);
// pack mappings
void
__fastcall UDFPackMapping(IN PVCB Vcb,
                        IN PEXTENT_INFO ExtInfo);   // Extent array
// check if all the data is in cache.
BOOLEAN  UDFIsExtentCached(IN PVCB Vcb,
                           IN PEXTENT_INFO ExtInfo, // Extent array
                           IN int64 Offset,      // offset in extent
                           IN uint32 Length,
                           IN BOOLEAN ForWrite);
/*BOOLEAN  UDFIsFileCached__(IN PVCB Vcb,
                       IN PUDF_FILE_INFO FileInfo,
                       IN int64 Offset,   // offset in extent
                       IN uint32 Length,
                       IN BOOLEAN ForWrite);*/
#define UDFIsFileCached__(Vcb, FileInfo, Offset, Length, ForWrite)  \
    (UDFIsExtentCached(Vcb, &((FileInfo)->Dloc->DataLoc), Offset, Length, ForWrite))
// check if specified sector belongs to a file
ULONG  UDFIsBlockAllocated(IN void* _Vcb,
                           IN uint32 Lba);
// record VolIdent
OSSTATUS UDFUpdateVolIdent(IN PVCB Vcb,
                           IN UDF_VDS_RECORD Lba,
                           IN PUNICODE_STRING VolIdent);
// calculate checksum for unicode string (for DOS-names)
uint16 
__fastcall UDFUnicodeCksum(PWCHAR s,
                         uint32 n);
//#define UDFUnicodeCksum(s,n)  UDFCrc((uint8*)(s), (n)*sizeof(WCHAR))
//
uint16 
__fastcall
UDFUnicodeCksum150(PWCHAR s,
                uint32 n);

uint32 
__fastcall crc32(IN uint8* s,
            IN uint32 len);
// calculate a 16-bit CRC checksum using ITU-T V.41 polynomial
uint16 
__fastcall UDFCrc(IN uint8* Data,
                IN uint32 Size);
// read the first block of a tagged descriptor & check it
OSSTATUS UDFReadTagged(IN PVCB Vcb,
                       IN int8* Buf,
                       IN uint32 Block, 
                       IN uint32 Location, 
                       OUT uint16 *Ident);
// get physycal Lba for partition-relative addr
uint32
__fastcall UDFPartLbaToPhys(IN PVCB Vcb,
                            IN lb_addr* Addr);
// look for Anchor(s) at all possible locations
lba_t  UDFFindAnchor(PVCB           Vcb);         // Volume control block
// look for Volume recognition sequence
uint32    UDFFindVRS(PVCB           Vcb);
// process Primary volume descriptor
void     UDFLoadPVolDesc(PVCB Vcb,
                         int8* Buf); // pointer to buffer containing PVD
//
#define UDFGetLVIDiUse(Vcb) \
    ( ((Vcb) && (Vcb)->LVid) ? \
        ( (LogicalVolIntegrityDescImpUse*) \
                     ( ((int8*)(Vcb->LVid+1)) + \
                       Vcb->LVid->numOfPartitions*2*sizeof(uint32))) \
       : NULL)

// load Logical volume integrity descriptor
OSSTATUS UDFLoadLogicalVolInt(PDEVICE_OBJECT DeviceObject,
                              PVCB           Vcb,
                              extent_ad      loc);
// load Logical volume descriptor
OSSTATUS UDFLoadLogicalVol(PDEVICE_OBJECT DeviceObject,
                           PVCB           Vcb,
                           int8*          Buf,
                           lb_addr        *fileset);
// process Partition descriptor
OSSTATUS UDFLoadPartDesc(PVCB      Vcb,
                         int8*     Buf);
// scan VDS & fill special array
OSSTATUS UDFReadVDS(IN PVCB Vcb,
                    IN uint32 block,
                    IN uint32 lastblock,
                    IN PUDF_VDS_RECORD vds,
                    IN int8* Buf);
// process a main/reserve volume descriptor sequence.
OSSTATUS UDFProcessSequence(IN PDEVICE_OBJECT DeviceObject,
                            IN PVCB           Vcb,
                            IN uint32          block,
                            IN uint32          lastblock,
                           OUT lb_addr        *fileset);
// Verifies a main/reserve volume descriptor sequence.
OSSTATUS UDFVerifySequence(IN PDEVICE_OBJECT    DeviceObject,
                           IN PVCB              Vcb,
                           IN uint32             block,
                           IN uint32             lastblock,
                          OUT lb_addr           *fileset);
// remember some useful info about FileSet & RootDir location
void     UDFLoadFileset(IN PVCB            Vcb,
                        IN PFILE_SET_DESC  fset,
                       OUT lb_addr         *root,
                       OUT lb_addr         *sysstream);
// load partition info
OSSTATUS UDFLoadPartition(IN PDEVICE_OBJECT  DeviceObject,
                          IN PVCB            Vcb,
                         OUT lb_addr         *fileset);
// check if this is an UDF-formatted disk
OSSTATUS UDFGetDiskInfoAndVerify(IN PDEVICE_OBJECT DeviceObject, // the target device object
                                 IN PVCB           Vcb);         // Volume control block from this DevObj
// create hard link for the file
OSSTATUS UDFHardLinkFile__(IN PVCB Vcb,
                           IN BOOLEAN IgnoreCase,
                        IN OUT BOOLEAN* Replace,      // replace if destination file exists
                           IN PUNICODE_STRING fn,     // destination
                        IN OUT PUDF_FILE_INFO DirInfo1,
                        IN OUT PUDF_FILE_INFO DirInfo2,
                        IN OUT PUDF_FILE_INFO FileInfo);  // source (opened)
//
LONG     UDFFindDloc(IN PVCB Vcb,
                     IN uint32 Lba);
//
LONG     UDFFindFreeDloc(IN PVCB Vcb,
                         IN uint32 Lba);
//
OSSTATUS UDFAcquireDloc(IN PVCB Vcb,
                        IN PUDF_DATALOC_INFO Dloc);
//
OSSTATUS UDFReleaseDloc(IN PVCB Vcb,
                        IN PUDF_DATALOC_INFO Dloc);
//
OSSTATUS UDFStoreDloc(IN PVCB Vcb,
                      IN PUDF_FILE_INFO fi,
                      IN uint32 Lba);
//
OSSTATUS UDFRemoveDloc(IN PVCB Vcb,
                       IN PUDF_DATALOC_INFO Dloc);
//
OSSTATUS UDFUnlinkDloc(IN PVCB Vcb,
                       IN PUDF_DATALOC_INFO Dloc);
//
void     UDFFreeDloc(IN PVCB Vcb,
                     IN PUDF_DATALOC_INFO Dloc);
//
void     UDFRelocateDloc(IN PVCB Vcb,
                         IN PUDF_DATALOC_INFO Dloc,
                         IN uint32 NewLba);
//
void     UDFReleaseDlocList(IN PVCB Vcb);
//
PUDF_FILE_INFO UDFLocateParallelFI(PUDF_FILE_INFO di,  // parent FileInfo
                                   uint_di i,            // Index
                                   PUDF_FILE_INFO fi);
//
PUDF_FILE_INFO UDFLocateAnyParallelFI(PUDF_FILE_INFO fi);   // FileInfo to start search from
//
void UDFInsertLinkedFile(PUDF_FILE_INFO fi,   // FileInfo to be added to chain
                         PUDF_FILE_INFO fi2);   // any FileInfo fro the chain
//
OSSTATUS UDFCreateRootFile__(IN PVCB Vcb,
                         //    IN uint16 AllocMode, // short/long/ext/in-icb  // always in-ICB
                             IN uint32 PartNum,
                             IN uint32 ExtAttrSz,
                             IN uint32 ImpUseLen,
                             IN BOOLEAN Extended,
                             OUT PUDF_FILE_INFO* _FileInfo);
// try to create StreamDirectory associated with given file
OSSTATUS UDFCreateStreamDir__(IN PVCB Vcb,
                              IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
                              OUT PUDF_FILE_INFO* _SDirInfo);
//
OSSTATUS UDFOpenStreamDir__(IN PVCB Vcb,
                            IN PUDF_FILE_INFO FileInfo,    // file containing stream-dir
                            OUT PUDF_FILE_INFO* _SDirInfo);
//
#define UDFIsAStreamDir(FI)  ((FI) && ((FI)->Dloc) && ((FI)->Dloc->FE_Flags & UDF_FE_FLAG_IS_SDIR))
//
#define UDFHasAStreamDir(FI)  ((FI) && ((FI)->Dloc) && ((FI)->Dloc->FE_Flags & UDF_FE_FLAG_HAS_SDIR))
//
#define UDFIsAStream(FI)  ((FI) && UDFIsAStreamDir((FI)->ParentFile))
//
#define UDFIsSDirDeleted(FI)  ((FI) && (FI)->Dloc && ((FI)->Dloc->FE_Flags & UDF_FE_FLAG_IS_DEL_SDIR))
// Record updated VAT (if updated)
OSSTATUS UDFRecordVAT(IN PVCB Vcb);
//
OSSTATUS UDFModifyVAT(IN PVCB Vcb,
                      IN uint32 Lba,
                      IN uint32 Length);
//
OSSTATUS UDFUpdateVAT(IN void* _Vcb,
                      IN uint32 Lba,
                      IN uint32* RelocTab,
                      IN uint32 BCount);
//
OSSTATUS
__fastcall UDFUnPackMapping(IN PVCB Vcb,
                          IN PEXTENT_INFO ExtInfo);   // Extent array
//
OSSTATUS UDFConvertFEToNonInICB(IN PVCB Vcb,
                                IN PUDF_FILE_INFO FileInfo,
                                IN uint8 NewAllocMode);
//
OSSTATUS UDFConvertFEToExtended(IN PVCB Vcb,
                                IN PUDF_FILE_INFO FileInfo);
//
#define UDFGetPartNumByPartNdx(Vcb, pi) (Vcb->Partitions[pi].PartitionNum)
//
uint32
__fastcall UDFPartLen(PVCB Vcb,
                      uint32 PartNum);
//
OSSTATUS UDFPretendFileDeleted__(IN PVCB Vcb,
                                 IN PUDF_FILE_INFO FileInfo);

#define UDFStreamsSupported(Vcb) \
    (Vcb->maxUDFWriteRev >= 0x0200)

#define UDFNtAclSupported(Vcb) \
    (Vcb->maxUDFWriteRev >= 0x0200)

#define UDFReferenceFile__(fi)                       \
{                                                    \
    UDFInterlockedIncrement((PLONG)&((fi)->RefCount));  \
    UDFInterlockedIncrement((PLONG)&((fi)->Dloc->LinkRefCount));  \
    if((fi)->ParentFile) {                           \
        UDFInterlockedIncrement((PLONG)&((fi)->ParentFile->OpenCount));  \
    }                                                \
}

#define UDFReferenceFileEx__(fi,i)                   \
{                                                    \
    UDFInterlockedExchangeAdd((PLONG)&((fi)->RefCount),i);  \
    UDFInterlockedExchangeAdd((PLONG)&((fi)->Dloc->LinkRefCount),i);  \
    if((fi)->ParentFile) {                           \
        UDFInterlockedExchangeAdd((PLONG)&((fi)->ParentFile->OpenCount),i);  \
    }                                                \
}

#define UDFDereferenceFile__(fi)                     \
{                                                    \
    UDFInterlockedDecrement((PLONG)&((fi)->RefCount));  \
    UDFInterlockedDecrement((PLONG)&((fi)->Dloc->LinkRefCount));  \
    if((fi)->ParentFile) {                           \
        UDFInterlockedDecrement((PLONG)&((fi)->ParentFile->OpenCount));  \
    }                                                \
}

#define UDFIsDirEmpty__(fi) UDFIsDirEmpty((fi)->Dloc->DirIndex)
#define UDFIsDirOpened__(fi) (fi->OpenCount)

#define UDFSetFileAllocMode__(fi, mode)  \
{                                       \
    (fi)->Dloc->DataLoc.Flags = \
        ((fi)->Dloc->DataLoc.Flags & ~EXTENT_FLAG_ALLOC_MASK) | (mode & EXTENT_FLAG_ALLOC_MASK);     \
}

#define UDFGetFileAllocMode__(fi)  ((fi)->Dloc->DataLoc.Flags & EXTENT_FLAG_ALLOC_MASK)

#define UDFGetFileICBAllocMode__(fi)  (((PFILE_ENTRY)((fi)->Dloc->FileEntry))->icbTag.flags & ICB_FLAG_ALLOC_MASK)

#ifndef UDF_LIMIT_DIR_SIZE // release
#define UDF_DIR_INDEX_FRAME_SH   9
#else   //  demo
#define UDF_DIR_INDEX_FRAME_SH   7
#endif

#define UDF_DIR_INDEX_FRAME      ((uint_di)(1 << UDF_DIR_INDEX_FRAME_SH))

#define UDF_DIR_INDEX_FRAME_GRAN (32)
#define UDF_DIR_INDEX_FRAME_GRAN_MASK (UDF_DIR_INDEX_FRAME_GRAN-1)
#define AlignDirIndex(n)   ((n+UDF_DIR_INDEX_FRAME_GRAN_MASK) & ~(UDF_DIR_INDEX_FRAME_GRAN_MASK))

#if defined _X86_ && !defined UDF_LIMIT_DIR_SIZE

PDIR_INDEX_ITEM
__fastcall
UDFDirIndex(
    IN PDIR_INDEX_HDR hDirNdx,
    IN uint32 i
    );

#else   // NO X86 optimization , use generic C/C++
__inline PDIR_INDEX_ITEM UDFDirIndex(IN PDIR_INDEX_HDR hDirNdx,
                                     IN uint_di i)
{
#ifdef UDF_LIMIT_DIR_SIZE
    if( hDirNdx && (i < hDirNdx->LastFrameCount))
        return &( (((PDIR_INDEX_ITEM*)(hDirNdx+1))[0])[i] );
#else //UDF_LIMIT_DIR_SIZE
    uint_di j, k;
    if( hDirNdx &&
        ((j = (i >> UDF_DIR_INDEX_FRAME_SH)) < (k = hDirNdx->FrameCount) ) &&
        ((i = (i & (UDF_DIR_INDEX_FRAME-1))) < ((j < (k-1)) ? UDF_DIR_INDEX_FRAME : hDirNdx->LastFrameCount)) )
        return &( (((PDIR_INDEX_ITEM*)(hDirNdx+1))[j])[i] );
#endif // UDF_LIMIT_DIR_SIZE
    return NULL;
}
#endif // _X86_

#define UDFDirIndexGetLastIndex(di)  ((((di)->FrameCount - 1) << UDF_DIR_INDEX_FRAME_SH) + (di)->LastFrameCount)

// arr - bit array,  bit - number of bit
#ifdef _X86_

#ifdef _CONSOLE
#define CheckAddr(addr) {ASSERT((uint32)(addr) > 0x1000);}
#else
#define CheckAddr(addr) {ASSERT((uint32)(addr) & 0x80000000);}
#endif

#define UDFGetBit(arr, bit) UDFGetBit__((uint32*)(arr), bit)

BOOLEAN
__fastcall
UDFGetBit__(
    IN uint32* arr,
    IN uint32 bit
    );

#define UDFSetBit(arr, bit) UDFSetBit__((uint32*)(arr), bit)

void
__fastcall
UDFSetBit__(
    IN uint32* arr,
    IN uint32 bit
    );

#define UDFSetBits(arr, bit, bc) UDFSetBits__((uint32*)(arr), bit, bc)

void
UDFSetBits__(
    IN uint32* arr,
    IN uint32 bit,
    IN uint32 bc
    );

#define UDFClrBit(arr, bit) UDFClrBit__((uint32*)(arr), bit)

void
__fastcall
UDFClrBit__(
    IN uint32* arr,
    IN uint32 bit
    );

#define UDFClrBits(arr, bit, bc) UDFClrBits__((uint32*)(arr), bit, bc)

void
UDFClrBits__(
    IN uint32* arr,
    IN uint32 bit,
    IN uint32 bc
    );

#else   // NO X86 optimization , use generic C/C++

#define UDFGetBit(arr, bit) (    (BOOLEAN) ( ((((uint32*)(arr))[(bit)>>5]) >> ((bit)&31)) &1 )    )
#define UDFSetBit(arr, bit) ( (((uint32*)(arr))[(bit)>>5]) |= (((uint32)1) << ((bit)&31)) )
#define UDFClrBit(arr, bit) ( (((uint32*)(arr))[(bit)>>5]) &= (~(((uint32)1) << ((bit)&31))) )

#define UDFSetBits(arr, bit, bc) \
{uint32 j;                       \
    for(j=0;j<bc;j++) {          \
        UDFSetBit(arr, bit+j);   \
}}

#define UDFClrBits(arr, bit, bc) \
{uint32 j;                       \
    for(j=0;j<bc;j++) {          \
        UDFClrBit(arr, bit+j);   \
}}

#endif // _X86_

#define UDFGetUsedBit(arr,bit)      (!UDFGetBit(arr,bit))
#define UDFGetFreeBit(arr,bit)      UDFGetBit(arr,bit)
#define UDFSetUsedBit(arr,bit)      UDFClrBit(arr,bit)
#define UDFSetFreeBit(arr,bit)      UDFSetBit(arr,bit)
#define UDFSetUsedBits(arr,bit,bc)  UDFClrBits(arr,bit,bc)
#define UDFSetFreeBits(arr,bit,bc)  UDFSetBits(arr,bit,bc)

#define UDFGetBadBit(arr,bit)       UDFGetBit(arr,bit)

#define UDFGetZeroBit(arr,bit)      UDFGetBit(arr,bit)
#define UDFSetZeroBit(arr,bit)      UDFSetBit(arr,bit)
#define UDFClrZeroBit(arr,bit)      UDFClrBit(arr,bit)
#define UDFSetZeroBits(arr,bit,bc)  UDFSetBits(arr,bit,bc)
#define UDFClrZeroBits(arr,bit,bc)  UDFClrBits(arr,bit,bc)

#if defined UDF_DBG || defined _CONSOLE
  #ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
    #define UDFSetFreeBitOwner(Vcb, i) (Vcb)->FSBM_Bitmap_owners[i] = 0;
    #define UDFSetUsedBitOwner(Vcb, i, o) (Vcb)->FSBM_Bitmap_owners[i] = o;
    #define UDFGetUsedBitOwner(Vcb, i) ((Vcb)->FSBM_Bitmap_owners[i])
    #define UDFCheckUsedBitOwner(Vcb, i, o) { \
      ASSERT(i<(Vcb)->FSBM_BitCount); \
      if((Vcb)->FSBM_Bitmap_owners[i] != -1) { \
        ASSERT((Vcb)->FSBM_Bitmap_owners[i] == o); \
      } else { \
        ASSERT((Vcb)->FSBM_Bitmap_owners[i] != 0); \
        (Vcb)->FSBM_Bitmap_owners[i] = o; \
      } \
    }
    #define UDFCheckFreeBitOwner(Vcb, i) ASSERT((Vcb)->FSBM_Bitmap_owners[i] == 0);
  #else
    #define UDFSetFreeBitOwner(Vcb, i)
    #define UDFSetUsedBitOwner(Vcb, i, o)
    #define UDFCheckUsedBitOwner(Vcb, i, o)
    #define UDFCheckFreeBitOwner(Vcb, i)
  #endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS
#else
    #define UDFSetFreeBitOwner(Vcb, i)
    #define UDFSetUsedBitOwner(Vcb, i, o)
    #define UDFCheckUsedBitOwner(Vcb, i, o)
    #define UDFCheckFreeBitOwner(Vcb, i)
#endif //UDF_DBG

#ifdef UDF_TRACK_FS_STRUCTURES
extern
VOID
UDFRegisterFsStructure(
    PVCB   Vcb,
    uint32 Lba,
    uint32 Length  // sectors
    );
#else //UDF_TRACK_FS_STRUCTURES
#define UDFRegisterFsStructure(Vcb, Lba, Length)   {NOTHING;}
#endif //UDF_TRACK_FS_STRUCTURES

extern const char hexChar[];

#define UDF_MAX_VERIFY_CACHE   (8*1024*1024/2048)
#define UDF_VERIFY_CACHE_LOW   (4*1024*1024/2048)
#define UDF_VERIFY_CACHE_GRAN  (512*1024/2048)
#define UDF_SYS_CACHE_STOP_THR (10*1024*1024/2048)

OSSTATUS
UDFVInit(
    IN PVCB Vcb
    );

VOID
UDFVRelease(
    IN PVCB Vcb
    );

#define PH_FORGET_VERIFIED    0x00800000
#define PH_READ_VERIFY_CACHE  0x00400000
#define PH_KEEP_VERIFY_CACHE  0x00200000

OSSTATUS
UDFVWrite(
    IN PVCB Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 BCount,
    IN uint32 LBA,
//    OUT uint32* WrittenBytes,
    IN uint32 Flags
    );

OSSTATUS
UDFVRead(
    IN PVCB Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 BCount,
    IN uint32 LBA,
//    OUT uint32* ReadBytes,
    IN uint32 Flags
    );

OSSTATUS
UDFVForget(
    IN PVCB Vcb,
    IN uint32 BCount,
    IN uint32 LBA,
    IN uint32 Flags
    );

#define UFD_VERIFY_FLAG_FORCE   0x01
#define UFD_VERIFY_FLAG_WAIT    0x02
#define UFD_VERIFY_FLAG_BG      0x04
#define UFD_VERIFY_FLAG_LOCKED  0x10

VOID
UDFVVerify(
    IN PVCB Vcb,
    IN ULONG Flags
    );

VOID
UDFVFlush(
    IN PVCB Vcb
    );

__inline
BOOLEAN
__fastcall UDFVIsStored(
    IN PVCB Vcb,
    IN lba_t lba
    )
{
    if(!Vcb->VerifyCtx.VInited)
        return FALSE;
    return UDFGetBit(Vcb->VerifyCtx.StoredBitMap, lba);
} // end UDFVIsStored()

BOOLEAN
__fastcall
UDFCheckArea(
    IN PVCB Vcb,
    IN lba_t LBA,
    IN uint32 BCount
    );

#endif // __UDF_STRUCT_SUPPORT_H__
