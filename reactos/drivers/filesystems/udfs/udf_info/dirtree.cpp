////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
        Module name:

   udf_info.cpp

        Abstract:

   This file contains filesystem-specific routines
   for Directory tree & related structures support

*/

#include "udf.h"

#ifdef UDF_CHECK_UTIL
  #include "..\namesup.h"
#else
  #ifdef UDF_BUG_CHECK_ID
    #undef UDF_BUG_CHECK_ID
  #endif //UDF_BUG_CHECK_ID
#endif //UDF_CHECK_UTIL

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_DIR

#define         MEM_USDIRHASH_TAG               "USDirHash"

#define UDF_DUMP_DIRTREE
#ifdef UDF_DUMP_DIRTREE
#define DirPrint(x)  KdPrint(x)
#else
#define DirPrint(x)  {;}
#endif

/*
    This routine initializes DirIndex array
 */
PDIR_INDEX_HDR
UDFDirIndexAlloc(
    IN uint_di i
    )
{
    uint_di j,k;
    PDIR_INDEX_HDR hDirNdx;
    PDIR_INDEX_ITEM* FrameList;

    if(!i)
        return NULL;
#ifdef UDF_LIMIT_DIR_SIZE
    if(i>UDF_DIR_INDEX_FRAME)
        return NULL;
#endif //UDF_LIMIT_DIR_SIZE

    j = i >> UDF_DIR_INDEX_FRAME_SH;
    i &= (UDF_DIR_INDEX_FRAME-1);

    hDirNdx = (PDIR_INDEX_HDR)MyAllocatePoolTag__(UDF_DIR_INDEX_MT, sizeof(DIR_INDEX_HDR)+(j+(i!=0))*sizeof(PDIR_INDEX_ITEM), MEM_DIR_HDR_TAG);
    if(!hDirNdx) return NULL;
    RtlZeroMemory(hDirNdx, sizeof(DIR_INDEX_HDR));

    FrameList = (PDIR_INDEX_ITEM*)(hDirNdx+1);
    for(k=0; k<j; k++, FrameList++) {
        (*FrameList) = (PDIR_INDEX_ITEM)MyAllocatePoolTag__(UDF_DIR_INDEX_MT, UDF_DIR_INDEX_FRAME*sizeof(DIR_INDEX_ITEM), MEM_DIR_NDX_TAG);
        if(!(*FrameList)) {
free_hdi:
            // item pointet by FrameList is NULL, it could not be allocated
            while(k) {
                k--;
                FrameList--;
                MyFreePool__(*FrameList);
            }
            MyFreePool__(hDirNdx);
            return NULL;
        }
        RtlZeroMemory((*FrameList), UDF_DIR_INDEX_FRAME*sizeof(DIR_INDEX_ITEM));
    }
    if(i) {
        (*FrameList) = (PDIR_INDEX_ITEM)MyAllocatePoolTag__(UDF_DIR_INDEX_MT, AlignDirIndex(i)*sizeof(DIR_INDEX_ITEM), MEM_DIR_NDX_TAG);
        if(!(*FrameList))
            goto free_hdi;
        RtlZeroMemory((*FrameList), i*sizeof(DIR_INDEX_ITEM));
    }

    hDirNdx->FrameCount = j+(i!=0);
    hDirNdx->LastFrameCount = i ? i : UDF_DIR_INDEX_FRAME;

    return hDirNdx;
} // UDFDirIndexAlloc()

/*
    This routine releases DirIndex array
 */
void
UDFDirIndexFree(
    PDIR_INDEX_HDR hDirNdx
    )
{
    uint32 k;
    PDIR_INDEX_ITEM* FrameList;

    FrameList = (PDIR_INDEX_ITEM*)(hDirNdx+1);
    if(!hDirNdx) return;
    for(k=0; k<hDirNdx->FrameCount; k++, FrameList++) {
        if(*FrameList) MyFreePool__(*FrameList);
    }
    MyFreePool__(hDirNdx);
} // UDFDirIndexFree();

/*
    This routine grows DirIndex array
 */
OSSTATUS
UDFDirIndexGrow(
    IN PDIR_INDEX_HDR* _hDirNdx,
    IN uint_di d // increment
    )
{
    uint_di j,k;
    PDIR_INDEX_HDR hDirNdx = *_hDirNdx;
    PDIR_INDEX_ITEM* FrameList;

    if(d > UDF_DIR_INDEX_FRAME)
        return STATUS_INVALID_PARAMETER;

    j = hDirNdx->LastFrameCount+d;

    if(j > UDF_DIR_INDEX_FRAME) {
#ifndef UDF_LIMIT_DIR_SIZE // release
        // Grow header
        k = hDirNdx->FrameCount;
        if(!MyReallocPool__((int8*)hDirNdx, sizeof(DIR_INDEX_HDR) + k*sizeof(PDIR_INDEX_ITEM),
                       (int8**)(&hDirNdx), sizeof(DIR_INDEX_HDR) + (k+1)*sizeof(PDIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;
        FrameList = (PDIR_INDEX_ITEM*)(hDirNdx+1);
        // Grow last frame
        if(!MyReallocPool__((int8*)(FrameList[k-1]), AlignDirIndex(hDirNdx->LastFrameCount)*sizeof(DIR_INDEX_ITEM),
                       (int8**)(&(FrameList[k-1])), UDF_DIR_INDEX_FRAME*sizeof(DIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(&(FrameList[k-1][hDirNdx->LastFrameCount]),
                       (UDF_DIR_INDEX_FRAME-hDirNdx->LastFrameCount)*sizeof(DIR_INDEX_ITEM));
        hDirNdx->LastFrameCount = UDF_DIR_INDEX_FRAME;
        // Allocate new frame
        FrameList[k] = (PDIR_INDEX_ITEM)MyAllocatePoolTag__(UDF_DIR_INDEX_MT, AlignDirIndex(j-UDF_DIR_INDEX_FRAME)*sizeof(DIR_INDEX_ITEM), MEM_DIR_NDX_TAG );
        if(!FrameList[k])
            return STATUS_INSUFFICIENT_RESOURCES;
        hDirNdx->FrameCount++;
        RtlZeroMemory(FrameList[k], (j-UDF_DIR_INDEX_FRAME)*sizeof(DIR_INDEX_ITEM));
        hDirNdx->LastFrameCount = j-UDF_DIR_INDEX_FRAME;
        (*_hDirNdx) = hDirNdx;
#else   // UDF_LIMIT_DIR_SIZE
        return STATUS_INSUFFICIENT_RESOURCES;
#endif  // UDF_LIMIT_DIR_SIZE
    } else {
        k = hDirNdx->FrameCount;
        FrameList = (PDIR_INDEX_ITEM*)(hDirNdx+1);
        if(!MyReallocPool__((int8*)(FrameList[k-1]), AlignDirIndex(hDirNdx->LastFrameCount)*sizeof(DIR_INDEX_ITEM),
                       (int8**)(&(FrameList[k-1])), AlignDirIndex(j)*sizeof(DIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(&(FrameList[k-1][hDirNdx->LastFrameCount]),
                       (j-hDirNdx->LastFrameCount)*sizeof(DIR_INDEX_ITEM));
        hDirNdx->LastFrameCount = j;
    }
    return STATUS_SUCCESS;
} // end UDFDirIndexGrow()

/*
    Thisd routine truncates DirIndex array
 */
OSSTATUS
UDFDirIndexTrunc(
    IN PDIR_INDEX_HDR* _hDirNdx,
    IN uint_di d // decrement
    )
{
    uint_di j,k;

    if(d > UDF_DIR_INDEX_FRAME) {
        OSSTATUS status;
        while(d) {
            k = (d > UDF_DIR_INDEX_FRAME) ? UDF_DIR_INDEX_FRAME : d;
            if(!OS_SUCCESS(status = UDFDirIndexTrunc(_hDirNdx, k))) {
                return status;
            }
            d -= k;
        }
        return STATUS_SUCCESS;
    }

    PDIR_INDEX_HDR hDirNdx = *_hDirNdx;
    PDIR_INDEX_ITEM* FrameList;

    j = UDF_DIR_INDEX_FRAME+hDirNdx->LastFrameCount-d;
    FrameList = (PDIR_INDEX_ITEM*)(hDirNdx+1);
    k = hDirNdx->FrameCount-1;

    if(j <= UDF_DIR_INDEX_FRAME) {
        // free last frame
        if(!k && (j < 2)) {
            // someone tries to trunc. residual entries...
            return STATUS_INVALID_PARAMETER;
        }
        MyFreePool__(FrameList[k]);
        FrameList[k] = NULL;
        hDirNdx->LastFrameCount = UDF_DIR_INDEX_FRAME;
        hDirNdx->FrameCount--;
        // Truncate new last frame
        if(!MyReallocPool__((int8*)(FrameList[k-1]), UDF_DIR_INDEX_FRAME*sizeof(DIR_INDEX_ITEM),
                       (int8**)(&(FrameList[k-1])), AlignDirIndex(j)*sizeof(DIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;
        hDirNdx->LastFrameCount = j;
        // Truncate header
        if(!MyReallocPool__((int8*)hDirNdx, sizeof(DIR_INDEX_HDR) + (k+1)*sizeof(PDIR_INDEX_ITEM),
                       (int8**)(&hDirNdx), sizeof(DIR_INDEX_HDR) + k*sizeof(PDIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;

        (*_hDirNdx) = hDirNdx;

    } else {

        j -= UDF_DIR_INDEX_FRAME;
        if(!k && (j < 2)) {
            // someone tries to trunc. residual entries...
            return STATUS_INVALID_PARAMETER;
        }
        
        if(!MyReallocPool__((int8*)(FrameList[k]), AlignDirIndex(hDirNdx->LastFrameCount)*sizeof(DIR_INDEX_ITEM),
                       (int8**)(&(FrameList[k])), AlignDirIndex(j)*sizeof(DIR_INDEX_ITEM) ) )
            return STATUS_INSUFFICIENT_RESOURCES;
        hDirNdx->LastFrameCount = j;
    }
    return STATUS_SUCCESS;
} // end UDFDirIndexTrunc()

#if defined _X86_ && !defined UDF_LIMIT_DIR_SIZE
#pragma warning(disable:4035)               // re-enable below
/*
    This routine returns pointer to DirIndex item with index i.
 */
__declspec (naked)
PDIR_INDEX_ITEM
__fastcall
UDFDirIndex(
    IN PDIR_INDEX_HDR hDirNdx, // ECX
    IN uint32 i                // EDX
    )
{
#ifdef _MSC_VER
    __asm {
        push ebx
        push ecx
        push edx

//        mov  ebx,hDirNdx
        mov  ebx,ecx
        mov  ecx,edx
        or   ebx,ebx
        jz   EO_udi_err

        mov  eax,ecx
        shr  ecx,UDF_DIR_INDEX_FRAME_SH           ; ecx = j
        mov  edx,[ebx]hDirNdx.FrameCount          ; edx = k
        cmp  ecx,edx
        jae  EO_udi_err

        and  eax,(1 shl UDF_DIR_INDEX_FRAME_SH)-1 ; eax = i
        dec  edx
        cmp  ecx,edx
        jb   No_check

        cmp  eax,[ebx].LastFrameCount
        jae  EO_udi_err
No_check:
        add  ebx,size DIR_INDEX_HDR      ; ((PDIR_INDEX_ITEM*)(hDirNdx+1))...
        mov  ebx,[ebx+ecx*4]             ; ...[j]...
        mov  edx,size DIR_INDEX_ITEM
        mul  edx                         ; ...[i]...
        add  eax,ebx                     ; &(...)
        jmp  udi_OK
EO_udi_err:
        xor  eax,eax
udi_OK:
        pop  edx
        pop  ecx
        pop  ebx

        ret
    }
#else
    /* FIXME ReactOS */
    uint_di j, k;
    if( hDirNdx &&
        ((j = (i >> UDF_DIR_INDEX_FRAME_SH)) < (k = hDirNdx->FrameCount) ) &&
        ((i = (i & (UDF_DIR_INDEX_FRAME-1))) < ((j < (k-1)) ? UDF_DIR_INDEX_FRAME : hDirNdx->LastFrameCount)) )
        return &( (((PDIR_INDEX_ITEM*)(hDirNdx+1))[j])[i] );
    return NULL;
#endif
}
#pragma warning(default:4035)
#endif // _X86_

/*
    This routine returns pointer to DirIndex'es frame & index inside it
    according to start Index parameter. It also initializes scan parameters
 */
PDIR_INDEX_ITEM
UDFDirIndexGetFrame(
    IN PDIR_INDEX_HDR hDirNdx,
    IN uint32 Frame,
   OUT uint32* FrameLen,
   OUT uint_di* Index,
    IN uint_di Rel
    )
{
    if(Frame >= hDirNdx->FrameCount)
        return NULL;
    if(Index) {
#ifdef UDF_LIMIT_DIR_SIZE
        (*Index) = Rel;
//    if(FrameLen)
        (*FrameLen) = hDirNdx->LastFrameCount;
#else //UDF_LIMIT_DIR_SIZE
        (*Index) = Frame*UDF_DIR_INDEX_FRAME+Rel;
//    if(FrameLen)
        (*FrameLen) = (Frame < (hDirNdx->FrameCount-1)) ? UDF_DIR_INDEX_FRAME :
                                                          hDirNdx->LastFrameCount;
#endif //UDF_LIMIT_DIR_SIZE
    }
    return ((PDIR_INDEX_ITEM*)(hDirNdx+1))[Frame]+Rel;
} // end UDFDirIndexGetFrame()

/*
    This routine initializes indexes for optimized DirIndex scan
    according to start Index parameter
 */

BOOLEAN
UDFDirIndexInitScan(
    IN PUDF_FILE_INFO DirInfo,   //
   OUT PUDF_DIR_SCAN_CONTEXT Context,
    IN uint_di Index
    )
{
    Context->DirInfo = DirInfo;
    Context->hDirNdx = DirInfo->Dloc->DirIndex;
    if( (Context->frame = (Index >> UDF_DIR_INDEX_FRAME_SH)) >=
                                                 Context->hDirNdx->FrameCount) {
        return FALSE;
    }
    if( (Context->j = Index & (UDF_DIR_INDEX_FRAME-1)) >=
               ((Context->frame < (Context->hDirNdx->FrameCount-1))
                                    ? 
                                 UDF_DIR_INDEX_FRAME : Context->hDirNdx->LastFrameCount) ) {
        return FALSE;
    }
    Context->DirNdx = UDFDirIndexGetFrame(Context->hDirNdx,
                                          Context->frame,
                                          &(Context->d),
                                          &(Context->i),
                                          Context->j);
    Context->i--;
    Context->j--;
    Context->DirNdx--;

    return TRUE;
} // end UDFDirIndexInitScan()

PDIR_INDEX_ITEM
UDFDirIndexScan(
    PUDF_DIR_SCAN_CONTEXT Context,
    PUDF_FILE_INFO* _FileInfo
    )
{
    PUDF_FILE_INFO FileInfo;
    PUDF_FILE_INFO ParFileInfo;

    Context->i++;
    Context->j++;
    Context->DirNdx++;

    if(Context->j >= Context->d) {
        Context->j=0;
        Context->frame++;
        Context->DirNdx = UDFDirIndexGetFrame(Context->hDirNdx, 
                                              Context->frame,
                                              &(Context->d),
                                              &(Context->i),
                                              Context->j);
    }
    if(!Context->DirNdx) {
        if(_FileInfo)
            (*_FileInfo) = NULL;
        return NULL;
    }

    if(_FileInfo) {
        if(FileInfo = Context->DirNdx->FileInfo) {
            if(FileInfo->ParentFile != Context->DirInfo) {
                ParFileInfo = UDFLocateParallelFI(Context->DirInfo, 
                                                  Context->i,
                                                  FileInfo);
#ifdef UDF_DBG
                if(ParFileInfo->ParentFile != Context->DirInfo) {
                    BrutePoint();
                }
#endif // UDF_DBG
                FileInfo = ParFileInfo;
            }
        }
        (*_FileInfo) = FileInfo;
    }

    return (Context->DirNdx);
} // end UDFDirIndexScan()

/*
    This routine calculates hashes for directory search
 */
uint8
UDFBuildHashEntry(
    IN PVCB Vcb,
    IN PUNICODE_STRING Name,
   OUT PHASH_ENTRY hashes,
    IN uint8 Mask
    )
{
    UNICODE_STRING UName;
    WCHAR ShortNameBuffer[13];
    uint8 RetFlags = 0;

    if(!Name->Buffer) return 0;

    if(Mask & HASH_POSIX)
        hashes->hPosix = crc32((uint8*)(Name->Buffer), Name->Length);

    if(Mask & HASH_ULFN) {
/*        if(OS_SUCCESS(MyInitUnicodeString(&UName, L"")) &&
           OS_SUCCESS(MyAppendUnicodeStringToStringTag(&UName, Name, MEM_USDIRHASH_TAG))) {*/
        if(OS_SUCCESS(MyCloneUnicodeString(&UName, Name))) {
            RtlUpcaseUnicodeString(&UName, &UName, FALSE);
    /*        if(!RtlCompareUnicodeString(Name, &UName, FALSE)) {
                RetFlags |= UDF_FI_FLAG_LFN;
            }*/
            hashes->hLfn = crc32((uint8*)(UName.Buffer), UName.Length);
        } else {
            BrutePoint();
        }
        MyFreePool__(UName.Buffer);
    }

    if(Mask & HASH_DOS) {
        UName.Buffer = (PWCHAR)(&ShortNameBuffer);
        UName.MaximumLength = 13*sizeof(WCHAR);
        UDFDOSName(Vcb, &UName, Name, (Mask & HASH_KEEP_NAME) ? TRUE : FALSE);
        if(!RtlCompareUnicodeString(Name, &UName, TRUE)) {
            RetFlags |= UDF_FI_FLAG_DOS;
        }
        hashes->hDos = crc32((uint8*)(UName.Buffer), UName.Length);
    }
    return RetFlags;
} // UDFBuildHashEntry()

#ifdef UDF_CHECK_UTIL
uint32
UDFFindNextFI(
    IN int8* buff,
    IN uint32 prevOffset,
    IN uint32 Length
    )
{
    PFILE_IDENT_DESC FileId;
    while(prevOffset+sizeof(FILE_IDENT_DESC) < Length) {
        prevOffset++;
        FileId = (PFILE_IDENT_DESC)(buff+prevOffset);
        if(FileId->descTag.tagIdent != TID_FILE_IDENT_DESC) 
            continue;
        if(FileId->descTag.descVersion != 2 && FileId->descTag.descVersion != 3)
            continue;
        if(FileId->fileVersionNum != 1)
            continue;
        if(FileId->fileCharacteristics & (~0x1f))
            continue;
        if(prevOffset + ((FileId->lengthFileIdent + FileId->lengthOfImpUse + sizeof(FILE_IDENT_DESC) + 3) & (~((uint32)3))) <= Length) {
            KdPrint(("UDFFindNextFI OK: %x\n", prevOffset));
            return prevOffset;
        }
    }
    return 0;
} // end UDFFindNextFI()
#else //UDF_CHECK_UTIL
#define UDFFindNextFI(a,b,c)  0
#endif //UDF_CHECK_UTIL

/*
    This routine scans directory extent & builds index table for FileIdents
 */
OSSTATUS
UDFIndexDirectory(
    IN PVCB Vcb,
 IN OUT PUDF_FILE_INFO FileInfo
    )
{
    PDIR_INDEX_HDR hDirNdx;
    PDIR_INDEX_ITEM DirNdx;
    PFILE_IDENT_DESC FileId;
    uint32 Offset = 0;
    uint32 prevOffset = 0;
    uint_di Count = 0;
    OSSTATUS status;
    int8* buff;
    PEXTENT_INFO ExtInfo;  // Extent array for directory
    uint16 PartNum;
    uint32 ReadBytes;
    uint16 valueCRC;

    if(!FileInfo) return STATUS_INVALID_PARAMETER;
    ValidateFileInfo(FileInfo);

    ExtInfo = &(FileInfo->Dloc->DataLoc);
    FileInfo->Dloc->DirIndex = NULL;
    KdPrint(("UDF: scaning directory\n"));
    // allocate buffer for the whole directory
    ASSERT((uint32)(ExtInfo->Length));
    if(!ExtInfo->Length)
        return STATUS_FILE_CORRUPT_ERROR;
    buff = (int8*)DbgAllocatePool(PagedPool, (uint32)(ExtInfo->Length));
    if(!buff)
        return STATUS_INSUFFICIENT_RESOURCES;

    ExtInfo->Flags |= EXTENT_FLAG_ALLOC_SEQUENTIAL;

    // read FileIdents
    status = UDFReadExtent(Vcb, ExtInfo, 0, (uint32)(ExtInfo->Length), FALSE, buff, &ReadBytes);
    if(!OS_SUCCESS(status)) {
        DbgFreePool(buff);
        return status;
    }
    // scan Dir to get entry counter
    FileId = (PFILE_IDENT_DESC)buff;
    DirPrint(("  ExtInfo->Length %x\n", ExtInfo->Length));
    prevOffset = 0;
    while(Offset<ExtInfo->Length) {
        DirPrint(("  Offset %x\n", Offset));
        if(!FileId->descTag.tagIdent) {
            DirPrint(("  term item\n"));
            break;
        }
        if(FileId->descTag.tagIdent != TID_FILE_IDENT_DESC) {
            DirPrint(("  Inv. tag %x\n", FileId->descTag.tagIdent));
            Offset = UDFFindNextFI(buff, prevOffset, (ULONG)(ExtInfo->Length));
            if(!Offset) {
                DirPrint(("  can't find next\n"));
                break;
            } else {
                DirPrint(("  found next offs %x\n", Offset));
                FileId = (PFILE_IDENT_DESC)((buff)+Offset);
            }
        }
        if(((ULONG)Offset & (Vcb->LBlockSize-1)) > (Vcb->LBlockSize-sizeof(FILE_IDENT_DESC))) {
            DirPrint(("  badly aligned\n", Offset));
            if(Vcb->Modified) {
                DirPrint(("  queue repack request\n"));
                FileInfo->Dloc->DirIndex->DelCount = Vcb->PackDirThreshold+1;
            }
        }
        prevOffset = Offset;
        Offset += (FileId->lengthFileIdent + FileId->lengthOfImpUse + sizeof(FILE_IDENT_DESC) + 3) & (~((uint32)3));
        FileId = (PFILE_IDENT_DESC)((buff)+Offset);
        Count++;
        if(Offset+sizeof(FILE_IDENT_DESC) > ExtInfo->Length) {
            if(Offset != ExtInfo->Length) {
                KdPrint(("  Trash at the end of Dir\n"));
            }
//            BrutePoint();
            break;
        }
    }
    DirPrint(("  final Offset %x\n", Offset));
    if(Offset > ExtInfo->Length) {
        BrutePoint();
        KdPrint(("  Unexpected end of Dir\n"));
        DbgFreePool(buff);
        return STATUS_FILE_CORRUPT_ERROR;
    }
    // allocate buffer for directory index & zero it
    DirPrint(("  Count %x\n", Count));
    hDirNdx = UDFDirIndexAlloc(Count+1);
    if(!hDirNdx) {
        DbgFreePool(buff);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Offset = Count = 0;
    hDirNdx->DIFlags |= (ExtInfo->Offset ? UDF_DI_FLAG_INIT_IN_ICB : 0);
    // add entry pointing to the directory itself
    DirNdx = UDFDirIndex(hDirNdx,0);
    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    DirNdx->FileEntryLoc.partitionReferenceNum = PartNum =
        (uint16)UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    ASSERT(PartNum != -1);
    DirNdx->FileEntryLoc.logicalBlockNum =
        UDFPhysLbaToPart(Vcb, PartNum, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    if(DirNdx->FileEntryLoc.logicalBlockNum == -1) {
        DirPrint(("  err: FileEntryLoc=-1\n"));
        DbgFreePool(buff);
        return STATUS_FILE_CORRUPT_ERROR;
    }
    DirNdx->FileCharacteristics = (FileInfo->FileIdent) ?
                         FileInfo->FileIdent->fileCharacteristics :
                         FILE_DIRECTORY;
//    DirNdx->Offset = 0;
//    DirNdx->Length = 0;
    DirNdx->FName.Buffer = L".";
    DirNdx->FName.Length =
    (DirNdx->FName.MaximumLength = sizeof(L".")) - sizeof(WCHAR);
    DirNdx->FileInfo = FileInfo;
    DirNdx->FI_Flags |= UDF_FI_FLAG_KEEP_NAME;
    DirNdx->FI_Flags |= UDFBuildHashEntry(Vcb, &(DirNdx->FName), &(DirNdx->hashes),
        HASH_ALL | HASH_KEEP_NAME);
    Count++;
    FileId = (PFILE_IDENT_DESC)buff;
    status = STATUS_SUCCESS;
    prevOffset = 0;
    while((Offset<ExtInfo->Length) && FileId->descTag.tagIdent) {
        // add new entry to index list
        if(FileId->descTag.tagIdent != TID_FILE_IDENT_DESC) {
            KdPrint(("  Invalid tagIdent %x (expected %x) offst %x\n", FileId->descTag.tagIdent, TID_FILE_IDENT_DESC, Offset));
            DirPrint(("    FileId: filen %x, iulen %x, charact %x\n",
                FileId->lengthFileIdent, FileId->lengthOfImpUse, FileId->fileCharacteristics));
            DirPrint(("    loc: @%x\n", UDFExtentOffsetToLba(Vcb, ExtInfo->Mapping, Offset, NULL, NULL, NULL, NULL)));
            KdDump(FileId, sizeof(FileId->descTag));
            Offset = UDFFindNextFI(buff, prevOffset, (ULONG)(ExtInfo->Length));
            if(!Offset) {
                DbgFreePool(buff);
                UDFDirIndexFree(hDirNdx);
                return STATUS_FILE_CORRUPT_ERROR;
            } else {
                DirPrint(("  found next offs %x\n", Offset));
                FileId = (PFILE_IDENT_DESC)((buff)+Offset);
            }
        }
        DirNdx = UDFDirIndex(hDirNdx,Count);
        // allocate buffer & fill it with decompressed unicode filename
        if(FileId->fileCharacteristics & FILE_DELETED) {
            DirPrint(("  FILE_DELETED\n"));
            hDirNdx->DelCount++;
        }
        DirPrint(("  FileId: offs %x, filen %x, iulen %x\n", Offset, FileId->lengthFileIdent, FileId->lengthOfImpUse));
        DirNdx->Length = (FileId->lengthFileIdent + FileId->lengthOfImpUse + sizeof(FILE_IDENT_DESC) + 3) & (~((uint32)3));
        DirPrint(("  DirNdx: Length %x, Charact %x\n", DirNdx->Length, FileId->fileCharacteristics));
        if(FileId->fileCharacteristics & FILE_PARENT) {
            DirPrint(("  parent\n"));
            // init 'parent' entry
            // '..' points to Parent Object (if any),
            // otherwise it points to the Dir itself
            DirNdx->FName.Buffer = L"..";
            DirNdx->FName.Length =
            (DirNdx->FName.MaximumLength = sizeof(L"..")) - sizeof(WCHAR);
            DirNdx->FileInfo = (FileInfo->ParentFile) ?
                                      FileInfo->ParentFile : FileInfo;
            DirNdx->FI_Flags |= UDF_FI_FLAG_KEEP_NAME;
            DirNdx->FI_Flags |= UDFBuildHashEntry(Vcb, &(DirNdx->FName), &(DirNdx->hashes), HASH_ALL | HASH_KEEP_NAME);
        } else {
            // init plain file/dir entry
            ASSERT( (Offset+sizeof(FILE_IDENT_DESC)+FileId->lengthOfImpUse+FileId->lengthFileIdent) <=
                    ExtInfo->Length );
            UDFDecompressUnicode(&(DirNdx->FName),
                             ((uint8*)(FileId+1)) + (FileId->lengthOfImpUse),
                             FileId->lengthFileIdent,
                             &valueCRC);
            UDFNormalizeFileName(&(DirNdx->FName), valueCRC);
            DirNdx->FI_Flags |= UDFBuildHashEntry(Vcb, &(DirNdx->FName), &(DirNdx->hashes), HASH_ALL);
        }
        if((FileId->fileCharacteristics & FILE_METADATA)
                       ||
              !DirNdx->FName.Buffer
                       ||
           ((DirNdx->FName.Length >= sizeof(UDF_RESERVED_NAME_HDR)-sizeof(WCHAR)) &&
            (RtlCompareMemory(DirNdx->FName.Buffer, UDF_RESERVED_NAME_HDR, sizeof(UDF_RESERVED_NAME_HDR)-sizeof(WCHAR)) == sizeof(UDF_RESERVED_NAME_HDR)-sizeof(WCHAR)) )) {
            DirPrint(("  metadata\n"));
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
        }
#if 0
        KdPrint(("%ws\n", DirNdx->FName.Buffer));
#endif
        DirPrint(("%ws\n", DirNdx->FName.Buffer));
        // remember FileEntry location...
        DirNdx->FileEntryLoc = FileId->icb.extLocation;
        // ... and some file characteristics
        DirNdx->FileCharacteristics = FileId->fileCharacteristics;
        DirNdx->Offset = Offset;
#ifdef UDF_CHECK_DISK_ALLOCATION
        if(!(FileId->fileCharacteristics & FILE_DELETED) &&
            (UDFPartLbaToPhys(Vcb, &(DirNdx->FileEntryLoc)) != LBA_OUT_OF_EXTENT) &&
             UDFGetFreeBit(((uint32*)(Vcb->FSBM_Bitmap)), UDFPartLbaToPhys(Vcb, &(DirNdx->FileEntryLoc)) )) {

            AdPrint(("Ref to Discarded block %x\n",UDFPartLbaToPhys(Vcb, &(DirNdx->FileEntryLoc)) ));
            BrutePoint();
            FileId->fileCharacteristics |= FILE_DELETED;
        } else
        if(UDFPartLbaToPhys(Vcb, &(DirNdx->FileEntryLoc)) == LBA_OUT_OF_EXTENT) {
            AdPrint(("Ref to Invalid block %x\n", UDFPartLbaToPhys(Vcb, &(DirNdx->FileEntryLoc)) ));
            BrutePoint();
            FileId->fileCharacteristics |= FILE_DELETED;
        }
#endif // UDF_CHECK_DISK_ALLOCATION
        prevOffset = Offset;
        Offset += DirNdx->Length;
        FileId = (PFILE_IDENT_DESC)(((int8*)FileId)+DirNdx->Length);
        Count++;
        if(Offset+sizeof(FILE_IDENT_DESC) > ExtInfo->Length) {
            if(Offset != ExtInfo->Length) {
                KdPrint(("  Trash at the end of Dir (2)\n"));
            }
//            BrutePoint();
            break;
        }
    } // while()
    // we needn't writing terminator 'cause the buffer is already zero-filled
    DbgFreePool(buff);
    if(Count < 2) {
        UDFDirIndexFree(hDirNdx);
        KdPrint(("  Directory too short\n"));
        return STATUS_FILE_CORRUPT_ERROR;
    }
    // store index
    FileInfo->Dloc->DirIndex = hDirNdx;
    return status;
} // end UDFIndexDirectory()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine removes all DELETED entries from Dir & resizes it.
    It must be called before closing, no files sould be opened.
 */
OSSTATUS
UDFPackDirectory__(
    IN PVCB Vcb,
 IN OUT PUDF_FILE_INFO FileInfo   // source (opened)
    )
{
#ifdef UDF_PACK_DIRS
    uint32 d, LBS;
    uint_di i, j;
    uint32 IUl, FIl, l;
    uint32 DataLocOffset;
    uint32 Offset, curOffset;
    int8* Buf;
    OSSTATUS status;
    uint32 ReadBytes;
    int8* storedFI;
    PUDF_FILE_INFO curFileInfo;
    PDIR_INDEX_ITEM DirNdx, DirNdx2;
    UDF_DIR_SCAN_CONTEXT ScanContext;
    uint_di dc=0;
    uint16 PartNum;
#endif //UDF_PACK_DIRS

    ValidateFileInfo(FileInfo);
    PDIR_INDEX_HDR hDirNdx = FileInfo->Dloc->DirIndex;
    if(!hDirNdx) return STATUS_NOT_A_DIRECTORY;
#ifndef UDF_PACK_DIRS
    return STATUS_SUCCESS;
#else // UDF_PACK_DIRS

    // do not pack dirs on unchanged disks
    if(!Vcb->Modified)
        return STATUS_SUCCESS;
    // start packing
    LBS = Vcb->LBlockSize;
    Buf = (int8*)DbgAllocatePool(PagedPool, LBS*2);
    if(!Buf) return STATUS_INSUFFICIENT_RESOURCES;
    // we shall never touch 1st entry 'cause it can't be deleted
    Offset = UDFDirIndex(hDirNdx,2)->Offset;
    DataLocOffset = FileInfo->Dloc->DataLoc.Offset;

    i=j=2;

    if(!UDFDirIndexInitScan(FileInfo, &ScanContext, i)) {
        DbgFreePool(Buf);
        return STATUS_SUCCESS;
    }

    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = (uint16)UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    ASSERT(PartNum != -1);

    while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {

        if(UDFIsDeleted(DirNdx))
            dc++;

        if(!UDFIsDeleted(DirNdx) ||
             DirNdx->FileInfo) {
            // move down valid entry
            status = UDFReadFile__(Vcb, FileInfo, curOffset = DirNdx->Offset,
                                                          l = DirNdx->Length, FALSE, Buf, &ReadBytes);
            if(!OS_SUCCESS(status)) {
                DbgFreePool(Buf);
                return status;
            }
            // remove ImpUse field
            IUl = ((PFILE_IDENT_DESC)Buf)->lengthOfImpUse;
            curFileInfo = DirNdx->FileInfo;
            // align next entry
            if((d = LBS - ((curOffset + (l - IUl) + DataLocOffset) & (LBS-1)) ) < sizeof(FILE_IDENT_DESC)) {

                // insufficient space at the end of last sector for
                // next FileIdent's tag. fill it with ImpUse data

                // generally, all data should be DWORD-aligned, but if it is not so
                // this opearation will help us to avoid glitches
                d = (d+3) & ~(3);
                if(d != IUl) {
                    l = l + d - IUl;
                    FIl = ((PFILE_IDENT_DESC)Buf)->lengthFileIdent;
                    // copy filename to upper addr
                    RtlMoveMemory(Buf+sizeof(FILE_IDENT_DESC)+d,
                                  Buf+sizeof(FILE_IDENT_DESC)+IUl, FIl);
                    RtlZeroMemory(Buf+sizeof(FILE_IDENT_DESC), d);
                    ((PFILE_IDENT_DESC)Buf)->lengthOfImpUse = (uint16)d;

                    if(curFileInfo && curFileInfo->FileIdent) {
                        // update stored FI if any
                        if(!MyReallocPool__((int8*)(curFileInfo->FileIdent), l,
                                     (int8**)&(curFileInfo->FileIdent), (l+IUl-d) )) {
                            DbgFreePool(Buf);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        storedFI = (int8*)(curFileInfo->FileIdent);
                        RtlMoveMemory(storedFI+sizeof(FILE_IDENT_DESC)+d,
                                      storedFI+sizeof(FILE_IDENT_DESC)+IUl, FIl);
                        RtlZeroMemory(storedFI+sizeof(FILE_IDENT_DESC), d);
                        ((PFILE_IDENT_DESC)storedFI)->lengthOfImpUse = (uint16)d;
                        FileInfo->Dloc->FELoc.Modified = TRUE;
                        FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
                    }
                }
            } else {
                d = 0;
            }
            // write modified to new addr
            if((d != IUl) ||
               (curOffset != Offset)) {

                UDFSetUpTag(Vcb, (tag*)Buf, (uint16)l,
                          UDFPhysLbaToPart(Vcb, PartNum,
                                     UDFExtentOffsetToLba(Vcb, FileInfo->Dloc->DataLoc.Mapping,
                                                Offset, NULL, NULL, NULL, NULL)));

                status = UDFWriteFile__(Vcb, FileInfo, Offset, l, FALSE, Buf, &ReadBytes);
                if(!OS_SUCCESS(status)) {
                    DbgFreePool(Buf);
                    return status;
                }
            }
            DirNdx2 = UDFDirIndex(hDirNdx, j);
            *DirNdx2 = *DirNdx;
            DirNdx2->Offset = Offset;
            DirNdx2->Length = l;
            if(curFileInfo) {
                curFileInfo->Index = j;
                DirNdx2->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
            }
            Offset += l;
            j++;
        }
    }
    // resize DirIndex
    DbgFreePool(Buf);
    if(dc) {
        if(!OS_SUCCESS(status = UDFDirIndexTrunc(&(FileInfo->Dloc->DirIndex), dc))) {
            return status;
        }
    }
    // terminator is set by UDFDirIndexTrunc()
    FileInfo->Dloc->DirIndex->DelCount = 0;
    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);

    // now Offset points to EOF. Let's truncate directory
    return UDFResizeFile__(Vcb, FileInfo, Offset);
#endif // UDF_PACK_DIRS
} // end UDFPackDirectory__()

/*
    This routine rebuilds tags for all entries from Dir.
 */
OSSTATUS
UDFReTagDirectory(
    IN PVCB Vcb,
 IN OUT PUDF_FILE_INFO FileInfo   // source (opened)
    )
{
    uint32 l;
    uint32 Offset;
    int8* Buf;
    OSSTATUS status;
    uint32 ReadBytes;
    PUDF_FILE_INFO curFileInfo;
    PDIR_INDEX_ITEM DirNdx;
    UDF_DIR_SCAN_CONTEXT ScanContext;
    uint16 PartNum;

    ValidateFileInfo(FileInfo);
    PDIR_INDEX_HDR hDirNdx = FileInfo->Dloc->DirIndex;
    if(!hDirNdx) return STATUS_NOT_A_DIRECTORY;

    // do not pack dirs on unchanged disks
    if(!Vcb->Modified)
        return STATUS_SUCCESS;

    if( ((hDirNdx->DIFlags & UDF_DI_FLAG_INIT_IN_ICB) ? TRUE : FALSE) ==
        ((FileInfo->Dloc->DataLoc.Offset) ? TRUE : FALSE) ) {
        return STATUS_SUCCESS;
    }

    // start packing
    Buf = (int8*)DbgAllocatePool(PagedPool, Vcb->LBlockSize*2);
    if(!Buf) return STATUS_INSUFFICIENT_RESOURCES;

    Offset = UDFDirIndex(hDirNdx,1)->Offset;

    if(!UDFDirIndexInitScan(FileInfo, &ScanContext, 1)) {
        DbgFreePool(Buf);
        return STATUS_SUCCESS;
    }

    ASSERT(FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    PartNum = (uint16)UDFGetPartNumByPhysLba(Vcb, FileInfo->Dloc->FELoc.Mapping[0].extLocation);
    ASSERT(PartNum != -1);

    while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {

        status = UDFReadFile__(Vcb, FileInfo, Offset = DirNdx->Offset,
                                                   l = DirNdx->Length, FALSE, Buf, &ReadBytes);
        if(!OS_SUCCESS(status)) {
            DbgFreePool(Buf);
            return status;
        }
        curFileInfo = DirNdx->FileInfo;
        // write modified
        UDFSetUpTag(Vcb, (tag*)Buf, (uint16)l,
                  UDFPhysLbaToPart(Vcb, PartNum,
                             UDFExtentOffsetToLba(Vcb, FileInfo->Dloc->DataLoc.Mapping,
                                        Offset, NULL, NULL, NULL, NULL)));

        if(curFileInfo && curFileInfo->FileIdent) {
            FileInfo->Dloc->FELoc.Modified = TRUE;
            FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FE_MODIFIED;
        }

        status = UDFWriteFile__(Vcb, FileInfo, Offset, l, FALSE, Buf, &ReadBytes);
        if(!OS_SUCCESS(status)) {
            DbgFreePool(Buf);
            return status;
        }
        if(curFileInfo) {
            DirNdx->FI_Flags |= UDF_FI_FLAG_FI_MODIFIED;
        }
    }
    // resize DirIndex
    DbgFreePool(Buf);

    hDirNdx->DIFlags &= ~UDF_DI_FLAG_INIT_IN_ICB;
    hDirNdx->DIFlags |= (FileInfo->Dloc->DataLoc.Offset ? UDF_DI_FLAG_INIT_IN_ICB : 0);
    return status;

} // end UDFReTagDirectory()
#endif //UDF_READ_ONLY_BUILD

/*
    This routine performs search for specified file in specified directory &
    returns corresponding offset in extent if found.
 */
OSSTATUS
UDFFindFile(
    IN PVCB Vcb,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN NotDeleted,
    IN PUNICODE_STRING Name,
    IN PUDF_FILE_INFO DirInfo,
 IN OUT uint_di* Index      // IN:start index OUT:found file index
    )
{
//    PDIR_INDEX_HDR hDirIndex = DirInfo->Dloc->DirIndex;
    UNICODE_STRING ShortName;
    WCHAR ShortNameBuffer[13];
    PDIR_INDEX_ITEM DirNdx;
    UDF_DIR_SCAN_CONTEXT ScanContext;
    uint_di j=(-1), k=(-1);
    HASH_ENTRY hashes;
    BOOLEAN CanBe8d3;

    UDFBuildHashEntry(Vcb, Name, &hashes, HASH_POSIX | HASH_ULFN);

    if(CanBe8d3 = UDFCanNameBeA8dot3(Name)) {
        ShortName.MaximumLength = 13 * sizeof(WCHAR);
        ShortName.Buffer = (PWCHAR)&ShortNameBuffer;
    }

    if(!UDFDirIndexInitScan(DirInfo, &ScanContext, (*Index)))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    if(!IgnoreCase && !CanBe8d3) {
        // perform case sensetive sequential directory scan

        while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {
            if( (DirNdx->hashes.hPosix == hashes.hPosix) &&
                 DirNdx->FName.Buffer &&
                (!RtlCompareUnicodeString(&(DirNdx->FName), Name, FALSE)) &&
               ( (!UDFIsDeleted(DirNdx)) || (!NotDeleted) ) ) {
                (*Index) = ScanContext.i;
                return STATUS_SUCCESS;
            }
        }
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if(hashes.hPosix == hashes.hLfn) {

        while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {
            if(!DirNdx->FName.Buffer ||
               (NotDeleted && UDFIsDeleted(DirNdx)) )
                continue;
            if( (DirNdx->hashes.hLfn == hashes.hLfn) &&
                (!RtlCompareUnicodeString(&(DirNdx->FName), Name, IgnoreCase)) ) {
                (*Index) = ScanContext.i;
                return STATUS_SUCCESS;
            } else
            if( CanBe8d3 &&
                !(DirNdx->FI_Flags & UDF_FI_FLAG_DOS) &&
                (DirNdx->hashes.hDos == hashes.hLfn) &&
                (k == (uint_di)(-1))) {
                UDFDOSName(Vcb, &ShortName, &(DirNdx->FName), ScanContext.i < 2) ;
                if(!RtlCompareUnicodeString(&ShortName, Name, IgnoreCase))
                    k = ScanContext.i;
            }
        }

    } else {

        while(DirNdx = UDFDirIndexScan(&ScanContext, NULL)) {
            // perform sequential directory scan
            if(!DirNdx->FName.Buffer ||
               (NotDeleted && UDFIsDeleted(DirNdx)) )
                continue;
            if( (DirNdx->hashes.hPosix == hashes.hPosix) &&
                (!RtlCompareUnicodeString(&(DirNdx->FName), Name, FALSE)) ) {
                (*Index) = ScanContext.i;
                return STATUS_SUCCESS;
            } else 
            if( (DirNdx->hashes.hLfn == hashes.hLfn) &&
                (j == (uint_di)(-1)) &&
                (!RtlCompareUnicodeString(&(DirNdx->FName), Name, IgnoreCase)) ) {
                j = ScanContext.i;
            } else
            if( CanBe8d3 &&
                !(DirNdx->FI_Flags & UDF_FI_FLAG_DOS) &&
                (DirNdx->hashes.hDos == hashes.hLfn) &&
                (k == (uint_di)(-1))) {
                UDFDOSName(Vcb, &ShortName, &(DirNdx->FName), ScanContext.i < 2 );
                if(!RtlCompareUnicodeString(&ShortName, Name, IgnoreCase)) {
                    k = ScanContext.i;
                }
            }
        }
    }

    if(j != (uint_di)(-1)) {
        (*Index) = j;
        return STATUS_SUCCESS;
    } else
    if(k != (uint_di)(-1)) {
        (*Index) = k;
        return STATUS_SUCCESS;
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;

} // end UDFFindFile()

/*
    This routine returns pointer to parent DirIndex
*/
PDIR_INDEX_HDR
UDFGetDirIndexByFileInfo(
    IN PUDF_FILE_INFO FileInfo
    )
{
    ValidateFileInfo(FileInfo);

    if(!FileInfo) {
        BrutePoint();
        return NULL;
    }
    if (FileInfo->ParentFile) {
        ValidateFileInfo(FileInfo->ParentFile);

        if(UDFIsAStreamDir(FileInfo))
            return NULL;
        if(FileInfo->ParentFile->Dloc)
            return FileInfo->ParentFile->Dloc->DirIndex;
        return NULL;
    }
    if(FileInfo->Dloc)
        return FileInfo->Dloc->DirIndex;
    return NULL;
}

/*
    File Data Location support routines (UDFXxxDloc)
    This group is responsible for caching FE locations
    If requested FE referenced by another FI the file is assumed to be linked
    All linked files reference to common Data Location (& attr) structure
 */

/*
    Check if given FE is already in use
 */
LONG
UDFFindDloc(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    PUDF_DATALOC_INDEX DlocList;
    uint32 l;

    if(!(DlocList = Vcb->DlocList) || !Lba) return (-1);
    // scan FE location cache
    l = Vcb->DlocCount;
    for(uint32 i=0; i<l; i++, DlocList++) {
        if(DlocList->Lba == Lba)
            return i;
    }
    return (-1);
} // end UDFFindDloc()

/*
    Check if given FE is already stored in memory
 */
LONG
UDFFindDlocInMem(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    PUDF_DATALOC_INDEX DlocList;
    uint32 l;

    if(!(DlocList = Vcb->DlocList) || !Dloc) return (-1);
    // scan FE location cache
    l = Vcb->DlocCount;
    for(uint32 i=0; i<l; i++, DlocList++) {
        if(DlocList->Dloc == Dloc)
            return i;
    }
    return (-1);
} // end UDFFindDlocInMem()

/*
    Find free cache entry
 */
LONG
UDFFindFreeDloc(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    PUDF_DATALOC_INDEX DlocList;
    uint32 l;

    if(!Vcb->DlocList) {
        // init FE location cache
        if(!(Vcb->DlocList = (PUDF_DATALOC_INDEX)MyAllocatePoolTag__(NonPagedPool, sizeof(UDF_DATALOC_INDEX)*DLOC_LIST_GRANULARITY, MEM_DLOC_NDX_TAG)))
            return (-1);
        RtlZeroMemory(Vcb->DlocList, DLOC_LIST_GRANULARITY*sizeof(UDF_DATALOC_INDEX));
        Vcb->DlocCount = DLOC_LIST_GRANULARITY;
    }
    // scan for free entry
    DlocList = Vcb->DlocList;
    l = Vcb->DlocCount;
    for(uint32 i=0; i<l; i++, DlocList++) {
        if(!DlocList->Dloc)
            return i;
    }
    // alloc some free entries
    if(!MyReallocPool__((int8*)(Vcb->DlocList), Vcb->DlocCount*sizeof(UDF_DATALOC_INDEX),
                     (int8**)&(Vcb->DlocList), (Vcb->DlocCount+DLOC_LIST_GRANULARITY)*sizeof(UDF_DATALOC_INDEX))) {
        return (-1);
    }
    RtlZeroMemory(&(Vcb->DlocList[Vcb->DlocCount]), DLOC_LIST_GRANULARITY*sizeof(UDF_DATALOC_INDEX));
    Vcb->DlocCount += DLOC_LIST_GRANULARITY;
    return (Vcb->DlocCount - DLOC_LIST_GRANULARITY);
} // end UDFFindFreeDloc()

/*
 */
OSSTATUS
UDFAcquireDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    UDFAcquireResourceExclusive(&(Vcb->DlocResource2),TRUE);
    if(Dloc->FE_Flags & UDF_FE_FLAG_UNDER_INIT) {
        UDFReleaseResource(&(Vcb->DlocResource2));
        return STATUS_SHARING_PAUSED;
    }
    Dloc->FE_Flags |= UDF_FE_FLAG_UNDER_INIT;
    UDFReleaseResource(&(Vcb->DlocResource2));
    return STATUS_SUCCESS;
} // end UDFAcquireDloc()

/*
 */
OSSTATUS
UDFReleaseDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    UDFAcquireResourceExclusive(&(Vcb->DlocResource2),TRUE);
    Dloc->FE_Flags &= ~UDF_FE_FLAG_UNDER_INIT;
    UDFReleaseResource(&(Vcb->DlocResource2));
    return STATUS_SUCCESS;
} // end UDFReleaseDloc()

/*
    Try to store FE location in cache
    If it is already in use, caller will be informed about it
 */
OSSTATUS
UDFStoreDloc(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO fi,
    IN uint32 Lba
    )
{
    LONG i;
    PUDF_DATALOC_INFO Dloc;

    if(!Lba) return STATUS_INVALID_PARAMETER;
    if(Lba == (-1)) return STATUS_INVALID_PARAMETER;

    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);

    // check if FE specified is already in use
    if((i = UDFFindDloc(Vcb, Lba)) == (-1)) {
        // not used
        if((i = UDFFindFreeDloc(Vcb, Lba)) == (-1)) {
            UDFReleaseResource(&(Vcb->DlocResource));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        if(!OS_SUCCESS(UDFAcquireDloc(Vcb, Dloc = Vcb->DlocList[i].Dloc))) {
            UDFReleaseResource(&(Vcb->DlocResource));
            return STATUS_SHARING_PAUSED;
        }
        // update caller's structures & exit
        fi->Dloc = Dloc;
        UDFReleaseDloc(Vcb, Dloc);
#if defined UDF_DBG && !defined _CONSOLE
        if(fi->Dloc->CommonFcb) {
            ASSERT((uint32)(fi->Dloc->CommonFcb) != 0xDEADDA7A);
            ASSERT(fi->Dloc->CommonFcb->CommonFCBHeader.NodeTypeCode == UDF_NODE_TYPE_NT_REQ_FCB);
        }
#endif // UDF_DBG
        UDFReleaseResource(&(Vcb->DlocResource));
        return STATUS_SUCCESS;
    }
    // allocate common DataLocation (Dloc) descriptor
    Dloc = fi->Dloc = (PUDF_DATALOC_INFO)MyAllocatePoolTag__(UDF_DATALOC_INFO_MT, sizeof(UDF_DATALOC_INFO), MEM_DLOC_INF_TAG);
    if(!Dloc) {
        UDFReleaseResource(&(Vcb->DlocResource));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Vcb->DlocList[i].Lba = Lba;
    Vcb->DlocList[i].Dloc = Dloc;
    RtlZeroMemory(Dloc, sizeof(UDF_DATALOC_INFO));
    Dloc->LinkedFileInfo = fi;
    UDFAcquireDloc(Vcb, Dloc);
    UDFReleaseResource(&(Vcb->DlocResource));
    return STATUS_SUCCESS;
} // end UDFStoreDloc()

/*
    Remove unreferenced FE location from cache & free allocated memory
    This routine must be invoked when there are no more opened files
    associated with given FE
 */
OSSTATUS
UDFRemoveDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    LONG i;

    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);

    if((i = UDFFindDlocInMem(Vcb, Dloc)) == (-1)) {
        // FE specified is not in cache. exit
        UDFReleaseResource(&(Vcb->DlocResource));
        return STATUS_INVALID_PARAMETER;
    }
    // remove from cache
    ASSERT(Vcb->DlocList);
    RtlZeroMemory(&(Vcb->DlocList[i]), sizeof(UDF_DATALOC_INDEX));
    UDFReleaseResource(&(Vcb->DlocResource));
    MyFreePool__(Dloc);
    return STATUS_SUCCESS;
} // end UDFRemoveDloc()

/*
    Remove unlinked FE location from cache & keep allocated memory
    This routine must be invoked when there are no more opened files
    associated with given FE
 */
OSSTATUS
UDFUnlinkDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    LONG i;

    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);

    if((i = UDFFindDlocInMem(Vcb, Dloc)) == (-1)) {
        // FE specified is not in cache. exit
        UDFReleaseResource(&(Vcb->DlocResource));
        return STATUS_INVALID_PARAMETER;
    }
    // remove from cache
    ASSERT(Vcb->DlocList);
    RtlZeroMemory(&(Vcb->DlocList[i]), sizeof(UDF_DATALOC_INDEX));
    UDFReleaseResource(&(Vcb->DlocResource));
    return STATUS_SUCCESS;
} // end UDFUnlinkDloc()

/*
    This routine releases memory allocated for Dloc & removes it from
    cache (if it is still there)
 */
void
UDFFreeDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    )
{
    LONG i;

    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);

    if((i = UDFFindDlocInMem(Vcb, Dloc)) != (-1)) {
        ASSERT(Vcb->DlocList);
        RtlZeroMemory(&(Vcb->DlocList[i]), sizeof(UDF_DATALOC_INDEX));
    }
    UDFReleaseResource(&(Vcb->DlocResource));
    MyFreePool__(Dloc);
} // end UDFFreeDloc()

/*
    This routine updates Dloc LBA after relocation
 */
void
UDFRelocateDloc(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc,
    IN uint32 NewLba
    )
{
    LONG i;

    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);

    if((i = UDFFindDlocInMem(Vcb, Dloc)) != (-1)) {
        ASSERT(Vcb->DlocList);
        Vcb->DlocList[i].Lba = NewLba;
    }
    UDFReleaseResource(&(Vcb->DlocResource));

} // end UDFRelocateDloc()

/*
    Release FE cache
 */
void
UDFReleaseDlocList(
    IN PVCB Vcb
    )
{
    if(!Vcb->DlocList) return;
    UDFAcquireResourceExclusive(&(Vcb->DlocResource),TRUE);
    for(uint32 i=0; i<Vcb->DlocCount; i++) {
        if(Vcb->DlocList[i].Dloc)
            MyFreePool__(Vcb->DlocList[i].Dloc);
    }
    MyFreePool__(Vcb->DlocList);
    Vcb->DlocList = NULL;
    Vcb->DlocCount = 0;
    UDFReleaseResource(&(Vcb->DlocResource));
} // end UDFReleaseDlocList()

/*
    This routine walks through Linked/Parallel FI chain and looks for
    FE with same Index & Parent File
 */
PUDF_FILE_INFO
UDFLocateParallelFI(
    PUDF_FILE_INFO di,  // parent FileInfo
    uint_di i,            // Index
    PUDF_FILE_INFO fi   // FileInfo to start search from
    )
{
    PUDF_FILE_INFO    ParFileInfo = fi->NextLinkedFile;
//    PUDF_DATALOC_INFO Dloc = di->Dloc;
    while((ParFileInfo != fi) &&
          ((ParFileInfo->ParentFile != di) ||
           (ParFileInfo->Index != i)) ) {
        ParFileInfo = ParFileInfo->NextLinkedFile;
    }
    return ParFileInfo;
//   BrutePoint();
} // end UDFLocateParallelFI()

/*
    This routine walks through Linked/Parallel FI chain and looks for
    FE with same Index & Parent Dloc
 */
PUDF_FILE_INFO
UDFLocateAnyParallelFI(
    PUDF_FILE_INFO fi   // FileInfo to start search from
    )
{
    if(!fi->ParentFile) {
        if(fi->NextLinkedFile == fi)
            return NULL;
        return fi->NextLinkedFile;
    }
    PUDF_FILE_INFO    ParFileInfo = fi->NextLinkedFile;
    PUDF_DATALOC_INFO Dloc = fi->ParentFile->Dloc;
    uint_di i = fi->Index;
    BOOLEAN NotFound = TRUE;
    while((ParFileInfo != fi) &&
          (NotFound =
           ((ParFileInfo->Index != i) ||
            (ParFileInfo->ParentFile->Dloc != Dloc))) ) {
        ParFileInfo = ParFileInfo->NextLinkedFile;
    }
/*    if(NotFound) {
        if((ParFileInfo->Index == i) &&
           (ParFileInfo->ParentFile->Dloc == Dloc))
            return ParFileInfo;
        return NULL;
    }
    return ParFileInfo;*/
    return NotFound ? NULL : ParFileInfo;
//   BrutePoint();
} // end UDFLocateAnyParallelFI()

void
UDFInsertLinkedFile(
    PUDF_FILE_INFO fi,   // FileInfo to be added to chain
    PUDF_FILE_INFO fi2   // any FileInfo fro the chain
    )
{
    fi->NextLinkedFile = fi2->NextLinkedFile;
    fi->PrevLinkedFile = fi2;
    fi->NextLinkedFile->PrevLinkedFile =
    fi->PrevLinkedFile->NextLinkedFile = fi;
    return;
} // end UDFInsertLinkedFile()

