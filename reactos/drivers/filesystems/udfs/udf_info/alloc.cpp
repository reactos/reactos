////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
        Module name:

   alloc.cpp

        Abstract:

   This file contains filesystem-specific routines
   responsible for disk space management

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_ALLOC

static const int8 bit_count_tab[] = {
    0, 1, 1, 2, 1, 2, 2, 3,   1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,

    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7,   5, 6, 6, 7, 6, 7, 7, 8
};

/*
    This routine converts physical address to logical in specified partition
 */
uint32
UDFPhysLbaToPart(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN uint32 Addr
    )
{
    PUDFPartMap pm = Vcb->Partitions;
//#ifdef _X86_
#ifdef _MSC_VER
    uint32 retval;
    __asm {
        push ebx
        push ecx
        push edx

        mov  ebx,Vcb
        mov  edx,[ebx]Vcb.PartitionMaps
        mov  ebx,pm
        mov  ecx,PartNum
        xor  eax,eax
loop_pl2p:
        cmp  ecx,edx
        jae  short EO_pl2p
        cmp  [ebx]pm.PartitionNum,cx
        jne  short cont_pl2p
        mov  eax,Addr
        sub  eax,[ebx]pm.PartitionRoot
        mov  ecx,Vcb
        mov  ecx,[ecx]Vcb.LB2B_Bits
        shr  eax,cl
        jmp  short EO_pl2p
cont_pl2p:
        add  ebx,size UDFPartMap
        inc  ecx
        jmp  short loop_pl2p
EO_pl2p:
        mov  retval,eax

        pop  edx
        pop  ecx
        pop  ebx
    }
#ifdef UDF_DBG
    {
        // validate return value
        lb_addr locAddr;
        locAddr.logicalBlockNum = retval;
        locAddr.partitionReferenceNum = (uint16)PartNum;
        UDFPartLbaToPhys(Vcb, &locAddr);
    }
#endif // UDF_DBG
    return retval;
#else   // NO X86 optimization , use generic C/C++
    uint32 i;
    // walk through partition maps to find suitable one...
    for(i=PartNum; i<Vcb->PartitionMaps; i++, pm++) {
        if(pm->PartitionNum == PartNum)
            // wow! return relative address
            return (Addr - pm->PartitionRoot) >> Vcb->LB2B_Bits;
    }
    return 0;
#endif // _X86_
} // end UDFPhysLbaToPart()

/*
    This routine returns physycal Lba for partition-relative addr
 */
uint32
__fastcall
UDFPartLbaToPhys(
  IN PVCB Vcb,
  IN lb_addr* Addr
  )
{
    uint32 i, a;
    if(Addr->partitionReferenceNum >= Vcb->PartitionMaps) {
        AdPrint(("UDFPartLbaToPhys: part %x, lbn %x (err)\n",
            Addr->partitionReferenceNum, Addr->logicalBlockNum));
        if(Vcb->PartitionMaps &&
           (Vcb->CompatFlags & UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS)) {
            AdPrint(("UDFPartLbaToPhys: try to recover: part %x -> %x\n",
                Addr->partitionReferenceNum, Vcb->PartitionMaps-1));
            Addr->partitionReferenceNum = (USHORT)(Vcb->PartitionMaps-1);
        } else {
            return LBA_OUT_OF_EXTENT;
        }
    }
    // walk through partition maps & transform relative address
    // to physical
    for(i=Addr->partitionReferenceNum; i<Vcb->PartitionMaps; i++) {
        if(Vcb->Partitions[i].PartitionNum == Addr->partitionReferenceNum) {
            a = Vcb->Partitions[i].PartitionRoot +
                    (Addr->logicalBlockNum << Vcb->LB2B_Bits);
            if(a > Vcb->LastPossibleLBA) {
                AdPrint(("UDFPartLbaToPhys: root %x, lbn %x, lba %x (err1)\n",
                    Vcb->Partitions[i].PartitionRoot, Addr->logicalBlockNum, a));
                BrutePoint();
                return LBA_OUT_OF_EXTENT;
            }
            return a;
        }
    }
    a = Vcb->Partitions[i-1].PartitionRoot +
            (Addr->logicalBlockNum << Vcb->LB2B_Bits);
    if(a > Vcb->LastPossibleLBA) {
        AdPrint(("UDFPartLbaToPhys: i %x, root %x, lbn %x, lba %x (err2)\n",
            i, Vcb->Partitions[i-1].PartitionRoot, Addr->logicalBlockNum, a));
        BrutePoint();
        return LBA_OUT_OF_EXTENT;
    }
    return a;
} // end UDFPartLbaToPhys()


/*
    This routine returns physycal Lba for partition-relative addr
    No partition bounds check is performed.
    This routine only checks if requested partition exists.
    It is introduced for 'Adaptec DirectCD' compatibility,
    because it uses negative values as extent terminator (against standard)
 */
/*uint32
__fastcall
UDFPartLbaToPhysCompat(
  IN PVCB Vcb,
  IN lb_addr* Addr
  )
{
    uint32 i, a;
    if(Addr->partitionReferenceNum >= Vcb->PartitionMaps) return LBA_NOT_ALLOCATED;
    // walk through partition maps & transform relative address
    // to physical
    for(i=Addr->partitionReferenceNum; i<Vcb->PartitionMaps; i++) {
        if(Vcb->Partitions[i].PartitionNum == Addr->partitionReferenceNum) {
            a = Vcb->Partitions[i].PartitionRoot +
                    (Addr->logicalBlockNum << Vcb->LB2B_Bits);
            if(a > Vcb->LastPossibleLBA) {
                BrutePoint();
            }
            return a;
        }
    }
    a = Vcb->Partitions[i-1].PartitionRoot +
            (Addr->logicalBlockNum << Vcb->LB2B_Bits);
    if(a > Vcb->LastPossibleLBA) {
        BrutePoint();
    }
    return a;
} // end UDFPartLbaToPhysCompat()*/


/*
    This routine looks for the partition containing given physical sector
 */
uint32
__fastcall
UDFGetPartNumByPhysLba(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    uint32 i=Vcb->PartitionMaps-1, root;
    PUDFPartMap pm = &(Vcb->Partitions[i]);
    // walk through the partition maps to find suitable one
    for(;i!=0xffffffff;i--,pm--) {
        if( ((root = pm->PartitionRoot) <= Lba) &&
            ((root + pm->PartitionLen) > Lba) ) return (uint16)pm->PartitionNum;
    }
    return LBA_OUT_OF_EXTENT; // Lba doesn't belong to any partition
} // end UDFGetPartNumByPhysLba()

/*
    Very simple routine. It walks through the Partition Maps & returns
    the 1st Lba of the 1st suitable one
 */
uint32
__fastcall
UDFPartStart(
    PVCB Vcb,
    uint32 PartNum
    )
{
    uint32 i;
    if(PartNum == (uint32)-1) return 0;
    if(PartNum == (uint32)-2) return Vcb->Partitions[0].PartitionRoot;
    for(i=PartNum; i<Vcb->PartitionMaps; i++) {
        if(Vcb->Partitions[i].PartitionNum == PartNum) return Vcb->Partitions[i].PartitionRoot;
    }
    return 0;
} // end UDFPartStart(

/*
   This routine does almost the same as previous.
   The only difference is changing First Lba to Last one...
 */
uint32
__fastcall
UDFPartEnd(
    PVCB Vcb,
    uint32 PartNum
    )
{
    uint32 i;
    if(PartNum == (uint32)-1) return Vcb->LastLBA;
    if(PartNum == (uint32)-2) PartNum = Vcb->PartitionMaps-1;
    for(i=PartNum; i<Vcb->PartitionMaps; i++) {
        if(Vcb->Partitions[i].PartitionNum == PartNum)
            return (Vcb->Partitions[i].PartitionRoot +
                    Vcb->Partitions[i].PartitionLen);
    }
    return (Vcb->Partitions[i-1].PartitionRoot +
            Vcb->Partitions[i-1].PartitionLen);
} // end UDFPartEnd()

/*
    Very simple routine. It walks through the Partition Maps & returns
    the 1st Lba of the 1st suitable one
 */
uint32
__fastcall
UDFPartLen(
    PVCB Vcb,
    uint32 PartNum
    )
{

    if(PartNum == (uint32)-2) return UDFPartEnd(Vcb, -2) - UDFPartStart(Vcb, -2);
/*#ifdef _X86_
    uint32 ret_val;
    __asm {
        mov  ebx,Vcb
        mov  eax,PartNum
        cmp  eax,-1
        jne  short NOT_last_gpl
        mov  eax,[ebx]Vcb.LastLBA
        jmp  short EO_gpl
NOT_last_gpl:
        mov  esi,eax
        xor  eax,eax
        mov  ecx,[ebx]Vcb.PartitionMaps
        jecxz EO_gpl

        mov  eax,esi
        mov  edx,size UDFTrackMap
        mul  edx
        add  ebx,eax
        mov  eax,esi
gpl_loop:
        cmp  [ebx]Vcb.PartitionMaps.PartitionNum,ax
        je   short EO_gpl_1
        add  ebx,size UDFTrackMap
        inc  eax
        cmp  eax,ecx
        jb   short gpl_loop
        sub  ebx,size UDFTrackMap
EO_gpl_1:
        mov  eax,[ebx]Vcb.PartitionMaps.PartitionLen
        add  eax,[ebx]Vcb.PartitionMaps.PartitionRoot
EO_gpl:
        mov  ret_val,eax
    }
    return ret_val;
#else   // NO X86 optimization , use generic C/C++*/
    uint32 i;
    if(PartNum == (uint32)-1) return Vcb->LastLBA;
    for(i=PartNum; i<Vcb->PartitionMaps; i++) {
        if(Vcb->Partitions[i].PartitionNum == PartNum)
            return Vcb->Partitions[i].PartitionLen;
    }
    return (Vcb->Partitions[i-1].PartitionRoot +
            Vcb->Partitions[i-1].PartitionLen);
/*#endif // _X86_*/
} // end UDFPartLen()

/*
    This routine returns length of bit-chain starting from Offs bit in
    array Bitmap. Bitmap scan is limited with Lim.
 */

//#if defined _X86_
#if defined _MSC_VER

__declspec (naked)
uint32
__stdcall
UDFGetBitmapLen(
    uint32* Bitmap,
    uint32 Offs,
    uint32 Lim          // NOT included
    )
{
  _asm {
    push  ebp
    mov   ebp, esp

    push  ebx
    push  ecx
    push  edx
    push  esi
    push  edi

    xor   edx,edx            // init bit-counter
    mov   ebx,[ebp+0x08]     // set base pointer in EBX (Bitmap)
    mov   esi,[ebp+0x0c]     // set Offs in ESI
    mov   edi,[ebp+0x10]     // set Lim in EDI

    // check if Lim <= Offs
    cmp   esi,edi
//    jb    start_count
//    ja    exit_count
//    inc   edx
//    jmp   exit_count
    jae   exit_count

//start_count:

    // set 1st bit number in CL
    mov   ecx,esi
    and   cl,0x1f
    // make ESI uint32-index
    shr   esi,5

    // save last bit number in CH
    mov   eax,edi
    and   al,0x1f
    mov   ch,al
    // make EDI uint32-index of the last uint32
    shr   edi,5

    mov   eax,[ebx+esi*4]
    shr   eax,cl
    test  eax,1

    jz    Loop_0

    /* COUNT 1-BITS SECTION */
Loop_1:

    cmp   esi,edi
    ja    exit_count      // must never happen
    jb    non_last_1

Loop_last_1:

    cmp   cl,ch
    jae   exit_count
    // do we met 0 ?
    test  eax,1
    jz    exit_count
    shr   eax,1
    inc   edx
    inc   cl
    jmp   Loop_last_1

non_last_1:

    or    cl,cl
    jnz   std_count_1
    cmp   eax,-1
    je    quick_count_1

std_count_1:

    cmp   cl,0x1f
    ja    next_uint32_1
    // do we met 0 ?
    test  eax,1
    jz    exit_count
    shr   eax,1
    inc   edx
    inc   cl
    jmp   std_count_1

quick_count_1:

    add   edx,0x20

next_uint32_1:

    inc   esi
    mov   eax,[ebx+esi*4]
    xor   cl,cl
    jmp   Loop_1

    /* COUNT 0-BITS SECTION */
Loop_0:

    cmp   esi,edi
    ja    exit_count      // must never happen
    jb    non_last_0

Loop_last_0:

    cmp   cl,ch
    jae   exit_count
    // do we met 1 ?
    test  eax,1
    jnz   exit_count
    shr   eax,1
    inc   edx
    inc   cl
    jmp   Loop_last_0

non_last_0:

    or    cl,cl
    jnz   std_count_0
    or    eax,eax
    jz    quick_count_0

std_count_0:

    cmp   cl,0x1f
    ja    next_uint32_0
    // do we met 1 ?
    test  eax,1
    jnz   exit_count
    shr   eax,1
    inc   edx
    inc   cl
    jmp   std_count_0

quick_count_0:

    add   edx,0x20

next_uint32_0:

    inc   esi
    mov   eax,[ebx+esi*4]
    xor   cl,cl
    jmp   Loop_0

exit_count:

    mov   eax,edx

    pop   edi
    pop   esi
    pop   edx
    pop   ecx
    pop   ebx

    pop   ebp

    ret   0x0c
  }

#else   // NO X86 optimization , use generic C/C++

uint32
__stdcall
UDFGetBitmapLen(
    uint32* Bitmap,
    uint32 Offs,
    uint32 Lim          // NOT included
    )
{
    ASSERT(Offs <= Lim);
    if(Offs >= Lim) {
        return 0;//(Offs == Lim);
    }

    BOOLEAN bit = UDFGetBit(Bitmap, Offs);
    uint32 i=Offs>>5;
    uint32 len=0;
    uint8 j=(uint8)(Offs&31);
    uint8 lLim=(uint8)(Lim&31);

    Lim = Lim>>5;

    ASSERT((bit == 0) || (bit == 1));

    uint32 a;

    a = Bitmap[i] >> j;

    while(i<=Lim) {

        while( j < ((i<Lim) ? 32 : lLim) ) {
            if( ((BOOLEAN)(a&1)) != bit) 
                return len;
            len++;
            a>>=1;
            j++;
        }
        j=0;
While_3:
        i++;
        a = Bitmap[i];

        if(i<Lim) {
            if((bit && (a==0xffffffff)) ||
               (!bit && !a)) {
                len+=32;
                goto While_3;
            }
        }
    }

    return len;

#endif // _X86_

} // end UDFGetBitmapLen()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine scans disc free space Bitmap for minimal suitable extent.
    It returns maximal available extent if no long enough extents found.
 */
uint32
UDFFindMinSuitableExtent(
    IN PVCB Vcb,
    IN uint32 Length, // in blocks
    IN uint32 SearchStart,
    IN uint32 SearchLim,    // NOT included
    OUT uint32* MaxExtLen,
    IN uint8  AllocFlags
    )
{
    uint32 i, len;
    uint32* cur;
    uint32 best_lba=0;
    uint32 best_len=0;
    uint32 max_lba=0;
    uint32 max_len=0;
    BOOLEAN align = FALSE;
    uint32 PS = Vcb->WriteBlockSize >> Vcb->BlockSizeBits;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    // we'll try to allocate packet-aligned block at first
    if(!(Length & (PS-1)) && !Vcb->CDR_Mode && (Length >= PS*2))
        align = TRUE;
    if(AllocFlags & EXTENT_FLAG_ALLOC_SEQUENTIAL)
        align = TRUE;
    if(Length > (uint32)(UDF_MAX_EXTENT_LENGTH >> Vcb->BlockSizeBits))
        Length = (UDF_MAX_EXTENT_LENGTH >> Vcb->BlockSizeBits);
    // align Length according to _Logical_ block size & convert it to BCount
    i = (1<<Vcb->LB2B_Bits)-1;
    Length = (Length+i) & ~i;
    cur = (uint32*)(Vcb->FSBM_Bitmap);

retry_no_align:

    i=SearchStart;
    // scan Bitmap
    while(i<SearchLim) {
        ASSERT(i <= SearchLim);
        if(align) {
            i = (i+PS-1) & ~(PS-1);
            ASSERT(i <= SearchLim);
            if(i >= SearchLim)
                break;
        }
        len = UDFGetBitmapLen(cur, i, SearchLim);
        if(UDFGetFreeBit(cur, i)) { // is the extent found free or used ?
            // wow! it is free!
            if(len >= Length) {
                // minimize extent length
                if(!best_len || (best_len > len)) {
                    best_lba = i;
                    best_len = len;
                }
                if(len == Length)
                    break;
            } else {
                // remember max extent
                if(max_len < len) {
                    max_lba = i;
                    max_len = len;
                }
            }
            // if this is CD-R mode, we should not think about fragmentation
            // due to CD-R nature file will be fragmented in any case
            if(Vcb->CDR_Mode) break;
        }
        i += len;
    }
    // if we can't find suitable Packet-size aligned block,
    // retry without any alignment requirements
    if(!best_len && align) {
        align = FALSE;
        goto retry_no_align;
    }
    if(best_len) {
        // minimal suitable block
        (*MaxExtLen) = best_len;
        return best_lba;
    }
    // maximal available
    (*MaxExtLen) = max_len;
    return max_lba;
} // end UDFFindMinSuitableExtent()
#endif //UDF_READ_ONLY_BUILD

#ifdef UDF_CHECK_DISK_ALLOCATION
/*
    This routine checks space described by Mapping as Used/Freed (optionaly)
 */
void
UDFCheckSpaceAllocation_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    )
{
    uint32 i=0;
    uint32 lba, j, len, BS, BSh;
    BOOLEAN asUsed = (asXXX == AS_USED);

    if(!Map) return;

    BS = Vcb->BlockSize;
    BSh = Vcb->BlockSizeBits;

    UDFAcquireResourceShared(&(Vcb->BitMapResource1),TRUE);
    // walk through all frags in data area specified
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    AdPrint(("ChkAlloc:Map:%x:File:%x:Line:%d\n",
        Map,
        BugCheckId,
        Line
        ));
#endif //UDF_TRACK_ONDISK_ALLOCATION
    while(Map[i].extLength & UDF_EXTENT_LENGTH_MASK) {
        
#ifdef UDF_TRACK_ONDISK_ALLOCATION
        AdPrint(("ChkAlloc:%x:%s:%x:@:%x:(%x):File:%x:Line:%d\n",
            FE_lba,
            asUsed ? "U" : "F",
            (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh,
            Map[i].extLocation,
            (Map[i].extLength >> 30),
            BugCheckId,
            Line
            ));
#endif //UDF_TRACK_ONDISK_ALLOCATION
        if(asUsed) {
            UDFCheckUsedBitOwner(Vcb, (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh, FE_lba);
        } else {
            UDFCheckFreeBitOwner(Vcb, (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
        }
        
        if((Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // skip unallocated frags
//            ASSERT(!(Map[i].extLength & UDF_EXTENT_LENGTH_MASK));
            ASSERT(!Map[i].extLocation);
            i++;
            continue;
        } else {
//            ASSERT(!(Map[i].extLength & UDF_EXTENT_LENGTH_MASK));
            ASSERT(Map[i].extLocation);
        }

#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(Map[i].extLength & (BS-1)));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
        lba = Map[i].extLocation;
        if((lba+len) > Vcb->LastPossibleLBA) {
            // skip blocks beyond media boundary
            if(lba > Vcb->LastPossibleLBA) {
                ASSERT(FALSE);
                i++;
                continue;
            }
            len = Vcb->LastPossibleLBA - lba;
        }

        // mark frag as XXX (see asUsed parameter)
        if(asUsed) {

            ASSERT(len);
            for(j=0;j<len;j++) {
                if(lba+j > Vcb->LastPossibleLBA) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers block(s) beyond media @%x\n",lba+j));
                    break;
                }
                if(!UDFGetUsedBit(Vcb->FSBM_Bitmap, lba+j)) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers FREE block(s) @%x\n",lba+j));
                    break;
                }
            }

        } else {

            ASSERT(len);
            for(j=0;j<len;j++) {
                if(lba+j > Vcb->LastPossibleLBA) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers block(s) beyond media @%x\n",lba+j));
                    break;
                }
                if(!UDFGetFreeBit(Vcb->FSBM_Bitmap, lba+j)) {
                    BrutePoint();
                    AdPrint(("FREE Mapping covers USED block(s) @%x\n",lba+j));
                    break;
                }
            }
        }

        i++;
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
} // end UDFCheckSpaceAllocation_()
#endif //UDF_CHECK_DISK_ALLOCATION

void
UDFMarkBadSpaceAsUsed(
    IN PVCB Vcb,
    IN lba_t lba,
    IN ULONG len
    )
{
    uint32 j;
#define BIT_C   (sizeof(Vcb->BSBM_Bitmap[0])*8)
    len = (lba+len+BIT_C-1)/BIT_C;
    if(Vcb->BSBM_Bitmap) {
        for(j=lba/BIT_C; j<len; j++) {
            Vcb->FSBM_Bitmap[j] &= ~Vcb->BSBM_Bitmap[j];
        }
    }
#undef BIT_C
} // UDFMarkBadSpaceAsUsed()

/*
    This routine marks space described by Mapping as Used/Freed (optionaly)
 */
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
    )
{
    uint32 i=0;
    uint32 lba, j, len, BS, BSh;
    uint32 root;
    BOOLEAN asUsed = (asXXX == AS_USED || (asXXX & AS_BAD));
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    BOOLEAN bit_before, bit_after;
#endif //UDF_TRACK_ONDISK_ALLOCATION

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    if(!Map) return;

    BS = Vcb->BlockSize;
    BSh = Vcb->BlockSizeBits;
    Vcb->BitmapModified = TRUE;
    UDFSetModified(Vcb);
    // walk through all frags in data area specified
    while(Map[i].extLength & UDF_EXTENT_LENGTH_MASK) {
        if((Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // skip unallocated frags
            i++;
            continue;
        }
        ASSERT(Map[i].extLocation);
        
#ifdef UDF_TRACK_ONDISK_ALLOCATION
        AdPrint(("Alloc:%x:%s:%x:@:%x:File:%x:Line:%d\n",
            FE_lba,
            asUsed ? ((asXXX & AS_BAD) ? "B" : "U") : "F",
            (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits,
            Map[i].extLocation,
            BugCheckId,
            Line
            ));
#endif //UDF_TRACK_ONDISK_ALLOCATION

#ifdef UDF_DBG
#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(Map[i].extLength & (BS-1)));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
//        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
#else // UDF_DBG
//        len = (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh;
#endif // UDF_DBG
        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
        lba = Map[i].extLocation;
        if((lba+len) > Vcb->LastPossibleLBA) {
            // skip blocks beyond media boundary
            if(lba > Vcb->LastPossibleLBA) {
                ASSERT(FALSE);
                i++;
                continue;
            }
            len = Vcb->LastPossibleLBA - lba;
        }

#ifdef UDF_TRACK_ONDISK_ALLOCATION
        if(lba)
            bit_before = UDFGetBit(Vcb->FSBM_Bitmap, lba-1);
        bit_after = UDFGetBit(Vcb->FSBM_Bitmap, lba+len);
#endif //UDF_TRACK_ONDISK_ALLOCATION

        // mark frag as XXX (see asUsed parameter)
        if(asUsed) {
/*            for(j=0;j<len;j++) {
                UDFSetUsedBit(Vcb->FSBM_Bitmap, lba+j);
            }*/
            ASSERT(len);
            UDFSetUsedBits(Vcb->FSBM_Bitmap, lba, len);
#ifdef UDF_TRACK_ONDISK_ALLOCATION
            for(j=0;j<len;j++) {
                ASSERT(UDFGetUsedBit(Vcb->FSBM_Bitmap, lba+j));
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION

            if(Vcb->Vat) {
                // mark logical blocks in VAT as used
                for(j=0;j<len;j++) {
                    root = UDFPartStart(Vcb, UDFGetPartNumByPhysLba(Vcb, lba));
                    if((Vcb->Vat[lba-root+j] == UDF_VAT_FREE_ENTRY) &&
                       (lba > Vcb->LastLBA)) {
                         Vcb->Vat[lba-root+j] = 0x7fffffff;
                    }
                }
            }
        } else {
/*            for(j=0;j<len;j++) {
                UDFSetFreeBit(Vcb->FSBM_Bitmap, lba+j);
            }*/
            ASSERT(len);
            UDFSetFreeBits(Vcb->FSBM_Bitmap, lba, len);
#ifdef UDF_TRACK_ONDISK_ALLOCATION
            for(j=0;j<len;j++) {
                ASSERT(UDFGetFreeBit(Vcb->FSBM_Bitmap, lba+j));
            }
#endif //UDF_TRACK_ONDISK_ALLOCATION
            if(asXXX & AS_BAD) {
                UDFSetBits(Vcb->BSBM_Bitmap, lba, len);
            }
            UDFMarkBadSpaceAsUsed(Vcb, lba, len);

            if(asXXX & AS_DISCARDED) {
                UDFUnmapRange(Vcb, lba, len);
                WCacheDiscardBlocks__(&(Vcb->FastCache), Vcb, lba, len);
                UDFSetZeroBits(Vcb->ZSBM_Bitmap, lba, len);
            }
            if(Vcb->Vat) {
                // mark logical blocks in VAT as free
                // this operation can decrease resulting VAT size
                for(j=0;j<len;j++) {
                    root = UDFPartStart(Vcb, UDFGetPartNumByPhysLba(Vcb, lba));
                    Vcb->Vat[lba-root+j] = UDF_VAT_FREE_ENTRY;
                }
            }
            // mark discarded extent as Not-Alloc-Not-Rec to
            // prevent writes there
            Map[i].extLength = (len << BSh) | (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            Map[i].extLocation = 0;
        }

#ifdef UDF_TRACK_ONDISK_ALLOCATION
        if(lba)
            ASSERT(bit_before == UDFGetBit(Vcb->FSBM_Bitmap, lba-1));
        ASSERT(bit_after == UDFGetBit(Vcb->FSBM_Bitmap, lba+len));
#endif //UDF_TRACK_ONDISK_ALLOCATION
        
        i++;
    }
} // end UDFMarkSpaceAsXXXNoProtect_()

/*
    This routine marks space described by Mapping as Used/Freed (optionaly)
    It protects data with sync Resource
 */
void
UDFMarkSpaceAsXXX_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    )
{
    if(!Map) return;
    if(!Map[0].extLength) {
#ifdef UDF_DBG
        ASSERT(!Map[0].extLocation);
#endif // UDF_DBG
        return;
    }

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX, FE_lba, BugCheckId, Line);
#else //UDF_TRACK_ONDISK_ALLOCATION
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX);
#endif //UDF_TRACK_ONDISK_ALLOCATION
    UDFReleaseResource(&(Vcb->BitMapResource1));

} // end UDFMarkSpaceAsXXX_()

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine builds mapping for Length bytes in FreeSpace
    It should be used when IN_ICB method is unavailable.
 */
OSSTATUS
UDFAllocFreeExtent_(
    IN PVCB   Vcb,
    IN int64  Length,
    IN uint32 SearchStart,
    IN uint32 SearchLim,     // NOT included
    OUT PEXTENT_INFO ExtInfo,
    IN uint8  AllocFlags
#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
   ,IN uint32 src,
    IN uint32 line
#endif //UDF_TRACK_ALLOC_FREE_EXTENT
    )
{
    EXTENT_AD Ext;
    PEXTENT_MAP Map = NULL;
    uint32 len, LBS, BSh, blen;

    LBS = Vcb->LBlockSize;
    BSh = Vcb->BlockSizeBits;
    blen = (uint32)(((Length+LBS-1) & ~((int64)LBS-1)) >> BSh);
    ExtInfo->Mapping = NULL;
    ExtInfo->Offset = 0;

    ASSERT(blen <= (uint32)(UDF_MAX_EXTENT_LENGTH >> BSh));

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);

    if(blen > (SearchLim - SearchStart)) {
        goto no_free_space_err;
    }
    // walk through the free space bitmap & find a single extent or a set of
    // frags giving in sum the Length specified
    while(blen) {
        Ext.extLocation = UDFFindMinSuitableExtent(Vcb, blen, SearchStart,
                                                               SearchLim, &len, AllocFlags);

//        ASSERT(len <= (uint32)(UDF_MAX_EXTENT_LENGTH >> BSh));
        if(len >= blen) {
            // complete search
            Ext.extLength = blen<<BSh;
            blen = 0;
        } else if(len) {
            // we need still some frags to complete request &
            // probably we have the opportunity to do it
            Ext.extLength = len<<BSh;
            blen -= len;
        } else {
no_free_space_err:
            // no more free space. abort
            if(ExtInfo->Mapping) {
                UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED); // free
                MyFreePool__(ExtInfo->Mapping);
                ExtInfo->Mapping = NULL;
            }
            UDFReleaseResource(&(Vcb->BitMapResource1));
            ExtInfo->Length = 0;//UDFGetExtentLength(ExtInfo->Mapping);
            AdPrint(("  DISK_FULL\n"));
            return STATUS_DISK_FULL;
        }
        // append the frag found to mapping
        ASSERT(!(Ext.extLength >> 30));
        ASSERT(Ext.extLocation);

        // mark newly allocated blocks as zero-filled
        UDFSetZeroBits(Vcb->ZSBM_Bitmap, Ext.extLocation, (Ext.extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);

        if(AllocFlags & EXTENT_FLAG_VERIFY) {
            if(!UDFCheckArea(Vcb, Ext.extLocation, Ext.extLength >> BSh)) {
                AdPrint(("newly allocated extent contains BB\n"));
                UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED); // free
                UDFMarkBadSpaceAsUsed(Vcb, Ext.extLocation, Ext.extLength >> BSh); // bad -> bad+used
                // roll back
                blen += Ext.extLength>>BSh;
                continue;
            }
        }

        Ext.extLength |= EXTENT_NOT_RECORDED_ALLOCATED << 30;
        if(!(ExtInfo->Mapping)) {
            // create new
#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
            ExtInfo->Mapping = UDFExtentToMapping_(&Ext, src, line);
#else // UDF_TRACK_ALLOC_FREE_EXTENT
            ExtInfo->Mapping = UDFExtentToMapping(&Ext);
#endif // UDF_TRACK_ALLOC_FREE_EXTENT
            if(!ExtInfo->Mapping) {
                BrutePoint();
                UDFReleaseResource(&(Vcb->BitMapResource1));
                ExtInfo->Length = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_USED); // used
        } else {
            // update existing
            Map = UDFExtentToMapping(&Ext);
            if(!Map) {
                BrutePoint();
                UDFReleaseResource(&(Vcb->BitMapResource1));
                ExtInfo->Length = UDFGetExtentLength(ExtInfo->Mapping);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, Map, AS_USED); // used
            ExtInfo->Mapping = UDFMergeMappings(ExtInfo->Mapping, Map);
            MyFreePool__(Map);
        }
        if(!ExtInfo->Mapping) {
            BrutePoint();
            UDFReleaseResource(&(Vcb->BitMapResource1));
            ExtInfo->Length = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
    ExtInfo->Length = Length;
    return STATUS_SUCCESS;
} // end UDFAllocFreeExtent_()
#endif //UDF_READ_ONLY_BUILD

/*
    Returns block-count
 */
uint32
__fastcall
UDFGetPartFreeSpace(
    IN PVCB Vcb,
    IN uint32 partNum
    )
{
    uint32 lim/*, len=1*/;
    uint32 s=0;
    uint32 j;
    PUCHAR cur = (PUCHAR)(Vcb->FSBM_Bitmap);

    lim = (UDFPartEnd(Vcb,partNum)+7)/8;
    for(j=(UDFPartStart(Vcb,partNum)+7)/8; j<lim/* && len*/; j++) {
        s+=bit_count_tab[cur[j]];
    }
    return s;
} // end UDFGetPartFreeSpace()

int64
__fastcall
UDFGetFreeSpace(
    IN PVCB Vcb
    )
{
    int64 s=0;
    uint32 i;
//    uint32* cur = (uint32*)(Vcb->FSBM_Bitmap);

    if(!Vcb->CDR_Mode &&
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)) {
        for(i=0;i<Vcb->PartitionMaps;i++) {
/*            lim = UDFPartEnd(Vcb,i);
            for(j=UDFPartStart(Vcb,i); j<lim && len; ) {
                len = UDFGetBitmapLen(cur, j, lim);
                if(UDFGetFreeBit(cur, j)) // is the extent found free or used ?
                    s+=len;
                j+=len;
            }*/
            s += UDFGetPartFreeSpace(Vcb, i);
        }
    } else {
        ASSERT(Vcb->LastPossibleLBA >= max(Vcb->NWA, Vcb->LastLBA));
        s = Vcb->LastPossibleLBA - max(Vcb->NWA, Vcb->LastLBA);
        //if(s & ((int64)1 << 64)) s=0;
    }
    return s >> Vcb->LB2B_Bits;
} // end UDFGetFreeSpace()

/*
    Returns block-count
 */
int64
__fastcall
UDFGetTotalSpace(
    IN PVCB Vcb
    )
{
    int64 s=0;
    uint32 i;

    if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) {
        s= Vcb->LastPossibleLBA;
    } else if(!Vcb->CDR_Mode) {
        for(i=0;i<Vcb->PartitionMaps;i++) {
            s+=Vcb->Partitions[i].PartitionLen;
        }
    } else {
        if(s & ((int64)1 << 64)) s=0;
        s= Vcb->LastPossibleLBA - Vcb->Partitions[0].PartitionRoot;
    }
    return s >> Vcb->LB2B_Bits;
} // end UDFGetTotalSpace()

/*
    Callback for WCache
    returns Allocated and Zero-filled flags for given block
    any data in 'unallocated' blocks may be changed during flush process
 */
uint32
UDFIsBlockAllocated(
    IN void* _Vcb,
    IN uint32 Lba
    )
{
    ULONG ret_val = 0;
    uint32* bm;
//    return TRUE;
    if(!(((PVCB)_Vcb)->VCBFlags & UDF_VCB_ASSUME_ALL_USED)) {
        // check used
        if((bm = (uint32*)(((PVCB)_Vcb)->FSBM_Bitmap)))
            ret_val = (UDFGetUsedBit(bm, Lba) ? WCACHE_BLOCK_USED : 0);
        // check zero-filled
        if((bm = (uint32*)(((PVCB)_Vcb)->ZSBM_Bitmap)))
            ret_val |= (UDFGetZeroBit(bm, Lba) ? WCACHE_BLOCK_ZERO : 0);
    } else {
        ret_val = WCACHE_BLOCK_USED;
    }
    // check bad block

    // WCache works with LOGICAL addresses, not PHYSICAL, BB check must be performed UNDER cache
/*
    if(bm = (uint32*)(((PVCB)_Vcb)->BSBM_Bitmap)) {
        ret_val |= (UDFGetBadBit(bm, Lba) ? WCACHE_BLOCK_BAD : 0);
        if(ret_val & WCACHE_BLOCK_BAD) {
            KdPrint(("Marked BB @ %#x\n", Lba));
        }
    }
*/
    return ret_val;
} // end UDFIsBlockAllocated()

#ifdef _X86_

#pragma warning(disable:4035)               // re-enable below

__declspec (naked)
BOOLEAN
__fastcall
UDFGetBit__(
    IN uint32* arr, // ECX
    IN uint32 bit   // EDX
    )
{
//    CheckAddr(arr);
//    ASSERT(bit < 300000);
#ifdef _MSC_VER
    __asm {
        push ebx
        push ecx
//        mov  eax,bit
        mov  eax,edx
        shr  eax,3
        and  al,0fch
        add  eax,ecx // eax+arr
        mov  eax,[eax]
        mov  cl,dl
        ror  eax,cl
        and  eax,1

        pop  ecx
        pop  ebx
        ret
    }
#else
/* FIXME ReactOS */
    return ((BOOLEAN)(((((uint32*)(arr))[(bit)>>5]) >> ((bit)&31)) &1));
#endif
} // end UDFGetBit__()

__declspec (naked)
void
__fastcall
UDFSetBit__(
    IN uint32* arr, // ECX
    IN uint32 bit   // EDX
    )
{
//    CheckAddr(arr);
//    ASSERT(bit < 300000);
#ifdef _MSC_VER
    __asm {
        push eax
        push ebx
        push ecx
//        mov  eax,bit
        mov  eax,edx
        shr  eax,3
        and  al,0fch
        add  eax,ecx // eax+arr
        mov  ebx,1
        mov  cl,dl
        rol  ebx,cl
        or   [eax],ebx

        pop  ecx
        pop  ebx
        pop  eax
        ret
    }
#else
/* FIXME ReactOS */
    (((uint32*)(arr))[(bit)>>5]) |= (((uint32)1) << ((bit)&31));
#endif
} // end UDFSetBit__()

void
UDFSetBits__(
    IN uint32* arr,
    IN uint32 bit,
    IN uint32 bc
    )
{
#ifdef _MSC_VER
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push esi

        mov  edx,bc            
        or   edx,edx
        jz   short EO_sb_loop

        mov  ecx,bit
        mov  esi,arr

        mov  ebx,1
        rol  ebx,cl

        mov  eax,ecx
        shr  eax,3
        and  al,0fch

        test cl, 0x1f
        jnz  short sb_loop_cont
sb_loop_2:
        cmp  edx,0x20
        jb   short sb_loop_cont

        mov  [dword ptr esi+eax],0xffffffff
        sub  edx,0x20
        jz   short EO_sb_loop
        add  eax,4
        add  ecx,0x20
        jmp  short sb_loop_2

sb_loop_cont:
        or   [esi+eax],ebx

        rol  ebx,1
        inc  ecx
        dec  edx
        jz   short EO_sb_loop

        test cl, 0x1f
        jnz  short sb_loop_cont
        add  eax,4
        jmp  short sb_loop_2
EO_sb_loop:
        pop  esi
        pop  edx
        pop  ecx
        pop  ebx
        pop  eax
    }
#else
/* FIXME ReactOS */
    uint32 j;
    for(j=0;j<bc;j++) {
        UDFSetBit(arr, bit+j);
    }
#endif
} // end UDFSetBits__()

__declspec (naked)
void
__fastcall
UDFClrBit__(
    IN uint32* arr, // ECX
    IN uint32 bit   // EDX
    )
{
//    CheckAddr(arr);
//    ASSERT(bit < 300000);
#ifdef _MSC_VER
    __asm {
        push eax
        push ebx
        push ecx
//        mov  eax,bit
        mov  eax,edx
        shr  eax,3
        and  al,0fch
        add  eax,ecx // eax+arr
        mov  ebx,0fffffffeh
        mov  cl,dl
        rol  ebx,cl
        and  [eax],ebx

        pop  ecx
        pop  ebx
        pop  eax
        ret
    }
#else
/* FIXME ReactOS */
    (((uint32*)(arr))[(bit)>>5]) &= (~(((uint32)1) << ((bit)&31)));
#endif
} // end UDFClrBit__()

void
UDFClrBits__(
    IN uint32* arr,
    IN uint32 bit,
    IN uint32 bc
    )
{
#ifdef _MSC_VER
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push esi

        mov  edx,bc            
        or   edx,edx
        jz   short EO_cp_loop

        mov  ecx,bit
        mov  esi,arr

        mov  ebx,0xfffffffe
        rol  ebx,cl

        mov  eax,ecx
        shr  eax,3
        and  al,0fch

        test cl, 0x1f
        jnz  short cp_loop_cont
cp_loop_2:
        cmp  edx,0x20
        jb   short cp_loop_cont

        mov  [dword ptr esi+eax],0x00000000
        sub  edx,0x20
        jz   short EO_cp_loop
        add  eax,4
        add  ecx,0x20
        jmp  short cp_loop_2

cp_loop_cont:
        and  [esi+eax],ebx

        rol  ebx,1
        inc  ecx
        dec  edx
        jz   short EO_cp_loop

        test cl, 0x1f
        jnz  short cp_loop_cont
        add  eax,4
        jmp  short cp_loop_2
EO_cp_loop:
        pop  esi
        pop  edx
        pop  ecx
        pop  ebx
        pop  eax
    }
#else
/* FIXME ReactOS */
    uint32 j;
    for(j=0;j<bc;j++) {
        UDFClrBit(arr, bit+j);
    }
#endif
} // end UDFClrBits__()

#pragma warning(default:4035)
#endif // _X86_
