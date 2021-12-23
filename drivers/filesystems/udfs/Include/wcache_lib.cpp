////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

/*********************************************************************/

OSSTATUS __fastcall WCacheCheckLimits(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t ReqLba,
                             IN ULONG BCount);

OSSTATUS __fastcall WCacheCheckLimitsRAM(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t ReqLba,
                             IN ULONG BCount);

OSSTATUS __fastcall WCacheCheckLimitsRW(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t ReqLba,
                             IN ULONG BCount);

OSSTATUS __fastcall WCacheCheckLimitsR(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t ReqLba,
                             IN ULONG BCount);

VOID     __fastcall WCachePurgeAllRW(IN PW_CACHE Cache,
                             IN PVOID Context);

VOID     __fastcall WCacheFlushAllRW(IN PW_CACHE Cache,
                             IN PVOID Context);

VOID     __fastcall WCachePurgeAllR(IN PW_CACHE Cache,
                             IN PVOID Context);

OSSTATUS __fastcall WCacheDecodeFlags(IN PW_CACHE Cache,
                             IN ULONG Flags);

#define ASYNC_STATE_NONE      0
#define ASYNC_STATE_READ_PRE  1
#define ASYNC_STATE_READ      2
#define ASYNC_STATE_WRITE_PRE 3
#define ASYNC_STATE_WRITE     4
#define ASYNC_STATE_DONE      5

#define ASYNC_CMD_NONE        0
#define ASYNC_CMD_READ        1
#define ASYNC_CMD_UPDATE      2

#define WCACHE_MAX_CHAIN      (0x10)

#define MEM_WCCTX_TAG         'xtCW'
#define MEM_WCFRM_TAG         'rfCW'
#define MEM_WCBUF_TAG         'fbCW'

#define USE_WC_PRINT

#ifdef USE_WC_PRINT
 #define WcPrint UDFPrint
#else
 #define WcPrint(x) {;}
#endif

typedef struct _W_CACHE_ASYNC {
    UDF_PH_CALL_CONTEXT PhContext;
    ULONG State;
    ULONG Cmd;
    PW_CACHE Cache;
    PVOID Buffer;
    PVOID Buffer2;
    SIZE_T TransferredBytes;
    ULONG BCount;
    lba_t Lba;
    struct _W_CACHE_ASYNC* NextWContext;
    struct _W_CACHE_ASYNC* PrevWContext;
} W_CACHE_ASYNC, *PW_CACHE_ASYNC;

VOID
WCacheUpdatePacketComplete(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN OUT PW_CACHE_ASYNC* FirstWContext, // pointer to head async IO context
    IN OUT PW_CACHE_ASYNC* PrevWContext,  // pointer to tail async IO context
    IN BOOLEAN FreePacket = TRUE
    );

/*********************************************************************/
ULONG WCache_random;

/*
  WCacheInit__() fills all necesary fileds in passed in PW_CACHE Cache
  structure, allocates memory and synchronization resources.
  Cacheable area is subdiveded on Frames - contiguous sets of blocks.
  Internally each Frame is an array of pointers and attributes of cached
  Blocks. To optimize memory usage WCache keeps in memory limited number
  of frames (MaxFrames).
  Frame length (number of Blocks) must be be a power of 2 and aligned on
  minimum writeable block size - Packet.
  Packet size must be a power of 2 (2, 4, 8, 16, etc.).
  Each cached Block belongs to one of the Frames. To optimize memory usage
  WCache keeps in memory limited number of Blocks (MaxBlocks). Block size
  must be a power of 2.
  WCache splits low-level request(s) into some parts if requested data length
  exceeds MaxBytesToRead.
  If requested data length exceeds maximum cache size WCache makes
  recursive calls to read/write routines with shorter requests

  WCacheInit__() returns initialization status. If initialization failed,
  all allocated memory and resources are automaticelly freed.

  Public routine
 */
OSSTATUS
WCacheInit__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure to be initialized
    IN ULONG MaxFrames,       // maximum number of Frames to be kept in memory
                              //   simultaneously
    IN ULONG MaxBlocks,       // maximum number of Blocks to be kept in memory
                              //   simultaneously
    IN SIZE_T MaxBytesToRead,  // maximum IO length (split boundary)
    IN ULONG PacketSizeSh,    // number of blocks in packet (bit shift)
                              //   Packes size = 2^PacketSizeSh
    IN ULONG BlockSizeSh,     // Block size (bit shift)
                              //   Block size = 2^BlockSizeSh
    IN ULONG BlocksPerFrameSh,// number of blocks in Frame (bit shift)
                              //   Frame size = 2^BlocksPerFrameSh
    IN lba_t FirstLba,        // Logical Block Address (LBA) of the 1st block
                              //   in cacheable area
    IN lba_t LastLba,         // Logical Block Address (LBA) of the last block
                              //   in cacheable area
    IN ULONG Mode,            // media mode:
                              //   WCACHE_MODE_ROM
                              //   WCACHE_MODE_RW
                              //   WCACHE_MODE_R
                              //   WCACHE_MODE_RAM
                              //   the following modes are planned to be implemented:
                              //   WCACHE_MODE_EWR
    IN ULONG Flags,           // cache mode flags:
                              //   WCACHE_CACHE_WHOLE_PACKET
                              //     read long (Packet-sized) blocks of
                              //     data from media
    IN ULONG FramesToKeepFree,
                              // number of Frames to be flushed & purged from cache
                              //   when Frame counter reaches top-limit and allocation
                              //   of a new Frame required
    IN PWRITE_BLOCK WriteProc,
                              // pointer to synchronous physical write call-back routine
    IN PREAD_BLOCK ReadProc,
                              // pointer to synchronous physical read call-back routine
    IN PWRITE_BLOCK_ASYNC WriteProcAsync,
                              // pointer to _asynchronous_ physical write call-back routine
                              //   currently must be set to NULL because async support
                              //   is not completly implemented
    IN PREAD_BLOCK_ASYNC ReadProcAsync,
                              // pointer to _asynchronous_ physical read call-back routine
                              //   must be set to NULL (see above)
    IN PCHECK_BLOCK CheckUsedProc,
                              // pointer to call-back routine that checks whether the Block
                              //   specified (by LBA) is allocated for some data or should
                              //   be treated as unused (and thus, zero-filled).
                              //   Is used to avoid physical reads and writes from/to such Blocks
    IN PUPDATE_RELOC UpdateRelocProc,
                              // pointer to call-back routine that updates caller's
                              //   relocation table _after_ physical write (append) in WORM
                              //   (WCACHE_MODE_R) mode. WCache sends original and new
                              //   (derived from last LBA) logical addresses to this routine
    IN PWC_ERROR_HANDLER ErrorHandlerProc
    )
{
    ULONG l1, l2, l3;
    ULONG PacketSize = (1) << PacketSizeSh;
    ULONG BlockSize = (1) << BlockSizeSh;
    ULONG BlocksPerFrame = (1) << BlocksPerFrameSh;
    OSSTATUS RC = STATUS_SUCCESS;
    LARGE_INTEGER rseed;
    ULONG res_init_flags = 0;

#define WCLOCK_RES   1

    _SEH2_TRY {
        // check input parameters
        if(Mode == WCACHE_MODE_R) {
            UDFPrint(("Disable Async-Write for WORM media\n"));
            WriteProcAsync = NULL;
        }
        if((MaxBlocks % PacketSize) || !MaxBlocks) {
            UDFPrint(("Total number of sectors must be packet-size-aligned\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(BlocksPerFrame % PacketSize) {
            UDFPrint(("Number of sectors per Frame must be packet-size-aligned\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(!ReadProc) {
            UDFPrint(("Read routine pointer must be valid\n"));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(FirstLba >= LastLba) {
            UDFPrint(("Invalid cached area parameters: (%x - %x)\n",FirstLba, LastLba));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(!MaxFrames) {
            UDFPrint(("Total frame number must be non-zero\n",FirstLba, LastLba));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(Mode > WCACHE_MODE_MAX) {
            UDFPrint(("Invalid media mode. Should be 0-%x\n",WCACHE_MODE_MAX));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        if(FramesToKeepFree >= MaxFrames/2) {
            UDFPrint(("Invalid FramesToKeepFree (%x). Should be Less or equal to MaxFrames/2 (%x)\n", FramesToKeepFree, MaxFrames/2));
            try_return(RC = STATUS_INVALID_PARAMETER);
        }
        // check 'features'
        if(!WriteProc) {
            UDFPrint(("Write routine not specified\n"));
            UDFPrint(("Read-only mode enabled\n"));
        }
        MaxBlocks = max(MaxBlocks, BlocksPerFrame*3);
        // initialize required structures
        // we'll align structure size on system page size to
        // avoid system crashes caused by pool fragmentation
        if(!(Cache->FrameList =
            (PW_CACHE_FRAME)MyAllocatePoolTag__(NonPagedPool, l1 = (((LastLba >> BlocksPerFrameSh)+1)*sizeof(W_CACHE_FRAME)), MEM_WCFRM_TAG) )) {
            UDFPrint(("Cache init err 1\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!(Cache->CachedBlocksList =
            (PULONG)MyAllocatePoolTag__(NonPagedPool, l2 = ((MaxBlocks+2)*sizeof(lba_t)), MEM_WCFRM_TAG) )) {
            UDFPrint(("Cache init err 2\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!(Cache->CachedModifiedBlocksList =
            (PULONG)MyAllocatePoolTag__(NonPagedPool, l2, MEM_WCFRM_TAG) )) {
            UDFPrint(("Cache init err 3\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!(Cache->CachedFramesList =
            (PULONG)MyAllocatePoolTag__(NonPagedPool, l3 = ((MaxFrames+2)*sizeof(lba_t)), MEM_WCFRM_TAG) )) {
            UDFPrint(("Cache init err 4\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory(Cache->FrameList, l1);
        RtlZeroMemory(Cache->CachedBlocksList, l2);
        RtlZeroMemory(Cache->CachedModifiedBlocksList, l2);
        RtlZeroMemory(Cache->CachedFramesList, l3);
        // remember all useful parameters
        Cache->BlocksPerFrame = BlocksPerFrame;
        Cache->BlocksPerFrameSh = BlocksPerFrameSh;
        Cache->BlockCount = 0;
        Cache->MaxBlocks = MaxBlocks;
        Cache->MaxBytesToRead = MaxBytesToRead;
        Cache->FrameCount = 0;
        Cache->MaxFrames = MaxFrames;
        Cache->PacketSize = PacketSize;
        Cache->PacketSizeSh = PacketSizeSh;
        Cache->BlockSize = BlockSize;
        Cache->BlockSizeSh = BlockSizeSh;
        Cache->WriteCount = 0;
        Cache->FirstLba = FirstLba;
        Cache->LastLba = LastLba;
        Cache->Mode = Mode;

        if(!OS_SUCCESS(RC = WCacheDecodeFlags(Cache, Flags))) {
            return RC;
        }

        Cache->FramesToKeepFree = FramesToKeepFree;
        Cache->WriteProc = WriteProc;
        Cache->ReadProc = ReadProc;
        Cache->WriteProcAsync = WriteProcAsync;
        Cache->ReadProcAsync = ReadProcAsync;
        Cache->CheckUsedProc = CheckUsedProc;
        Cache->UpdateRelocProc = UpdateRelocProc;
        Cache->ErrorHandlerProc = ErrorHandlerProc;
        // init permanent tmp buffers
        if(!(Cache->tmp_buff =
            (PCHAR)MyAllocatePoolTag__(NonPagedPool, PacketSize*BlockSize, MEM_WCFRM_TAG))) {
            UDFPrint(("Cache init err 5.W\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!(Cache->tmp_buff_r =
            (PCHAR)MyAllocatePoolTag__(NonPagedPool, PacketSize*BlockSize, MEM_WCFRM_TAG))) {
            UDFPrint(("Cache init err 5.R\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!(Cache->reloc_tab =
            (PULONG)MyAllocatePoolTag__(NonPagedPool, Cache->PacketSize*sizeof(ULONG), MEM_WCFRM_TAG))) {
            UDFPrint(("Cache init err 6\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        if(!OS_SUCCESS(RC = ExInitializeResourceLite(&(Cache->WCacheLock)))) {
            UDFPrint(("Cache init err (res)\n"));
            try_return(RC);
        }
        res_init_flags |= WCLOCK_RES;
        KeQuerySystemTime((PLARGE_INTEGER)(&rseed));
        WCache_random = rseed.LowPart;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(!OS_SUCCESS(RC)) {
            if(res_init_flags & WCLOCK_RES)
                ExDeleteResourceLite(&(Cache->WCacheLock));
            if(Cache->FrameList)
                MyFreePool__(Cache->FrameList);
            if(Cache->CachedBlocksList)
                MyFreePool__(Cache->CachedBlocksList);
            if(Cache->CachedModifiedBlocksList)
                MyFreePool__(Cache->CachedModifiedBlocksList);
            if(Cache->CachedFramesList)
                MyFreePool__(Cache->CachedFramesList);
            if(Cache->tmp_buff_r)
                MyFreePool__(Cache->tmp_buff_r);
            if(Cache->tmp_buff)
                MyFreePool__(Cache->tmp_buff);
            if(Cache->reloc_tab)
                MyFreePool__(Cache->reloc_tab);
            RtlZeroMemory(Cache, sizeof(W_CACHE));
        } else {
            Cache->Tag = 0xCAC11E00;
        }

    } _SEH2_END;

    return RC;
} // end WCacheInit__()

/*
  WCacheRandom() - just a random generator
  Returns random LONGLONG number
  Internal routine
 */
LONGLONG
WCacheRandom(VOID)
{
    WCache_random = (WCache_random * 0x8088405 + 1);
    return WCache_random;
} // end WCacheRandom()

/*
  WCacheFindLbaToRelease() finds Block to be flushed and purged from cache
  Returns random LBA
  Internal routine
 */
lba_t
__fastcall
WCacheFindLbaToRelease(
    IN PW_CACHE Cache
    )
{
    if(!(Cache->BlockCount))
        return WCACHE_INVALID_LBA;
    return(Cache->CachedBlocksList[((ULONG)WCacheRandom() % Cache->BlockCount)]);
} // end WCacheFindLbaToRelease()

/*
  WCacheFindModifiedLbaToRelease() finds Block to be flushed and purged from cache.
  This routine looks for Blocks among modified ones
  Returns random LBA (nodified)
  Internal routine
 */
lba_t
__fastcall
WCacheFindModifiedLbaToRelease(
    IN PW_CACHE Cache
    )
{
    if(!(Cache->WriteCount))
        return WCACHE_INVALID_LBA;
    return(Cache->CachedModifiedBlocksList[((ULONG)WCacheRandom() % Cache->WriteCount)]);
} // end WCacheFindModifiedLbaToRelease()

/*
  WCacheFindFrameToRelease() finds Frame to be flushed and purged with all
  Blocks (from this Frame) from cache
  Returns random Frame number
  Internal routine
 */
lba_t
__fastcall
WCacheFindFrameToRelease(
    IN PW_CACHE Cache
    )
{
    ULONG i, j;
    ULONG frame = 0;
    ULONG prev_uc = -1;
    ULONG uc = -1;
    lba_t lba;
    BOOLEAN mod = FALSE;

    if(!(Cache->FrameCount))
        return 0;
    /*
    return(Cache->CachedFramesList[((ULONG)WCacheRandom() % Cache->FrameCount)]);
    */

    for(i=0; i<Cache->FrameCount; i++) {

        j = Cache->CachedFramesList[i];

        mod |= (Cache->FrameList[j].UpdateCount != 0);
        uc = Cache->FrameList[j].UpdateCount*32 + Cache->FrameList[j].AccessCount;

        if(prev_uc > uc) {
            prev_uc = uc;
            frame = j;
        }
    }
    if(!mod) {
        frame = Cache->CachedFramesList[((ULONG)WCacheRandom() % Cache->FrameCount)];
        lba = frame << Cache->BlocksPerFrameSh;
        WcPrint(("WC:-frm %x\n", lba));
    } else {
        lba = frame << Cache->BlocksPerFrameSh;
        WcPrint(("WC:-frm(mod) %x\n", lba));
        for(i=0; i<Cache->FrameCount; i++) {

            j = Cache->CachedFramesList[i];
            Cache->FrameList[j].UpdateCount = (Cache->FrameList[j].UpdateCount*2)/3;
            Cache->FrameList[j].AccessCount = (Cache->FrameList[j].AccessCount*3)/4;
        }
    }
    return frame;
} // end WCacheFindFrameToRelease()

/*
  WCacheGetSortedListIndex() returns index of searched Lba
  (Lba is ULONG in sorted array) or index of minimal cached Lba
  greater than searched.
  If requested Lba is less than minimum cached, 0 is returned.
  If requested Lba is greater than maximum cached, BlockCount value
  is returned.
  Internal routine
 */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4035)               // re-enable below
#endif

ULONG
//__fastcall
WCacheGetSortedListIndex(
    IN ULONG BlockCount,      // number of items in array (pointed by List)
    IN lba_t* List,           // pointer to sorted (ASC) array of ULONGs
    IN lba_t Lba              // ULONG value to be searched for
    )
{
    if(!BlockCount)
        return 0;

#if defined(_X86_) && defined(_MSC_VER) && !defined(__clang__)

    __asm push  ecx
    __asm push  ebx
    __asm push  edx
    __asm push  esi
    __asm push  edi
//    left = 0;
//    right = BlockCount - 1;
//    pos = 0;
    __asm xor   edx,edx                 // left
    __asm mov   ebx,BlockCount
    __asm dec   ebx                     // right
    __asm xor   esi,esi                 // pos
    __asm mov   edi,List                // List
    __asm mov   ecx,Lba                 // Lba

While_1:
//    while(left != right) {
    __asm cmp   edx,ebx
    __asm jz    EO_while_1

//        pos = (left + right) >> 1;
    __asm lea   esi,[ebx+edx]
    __asm shr   esi,1
//        if(List[pos] == Lba)
//            return pos;
    __asm mov   eax,[edi+esi*4]
    __asm cmp   eax,ecx
    __asm jz    EO_while_2

//        if(right - left == 1) {
    __asm sub   ebx,edx
    __asm cmp   ebx,1
    __asm jne    NO_r_sub_l_eq_1
//            if(List[pos+1] < Lba)      <=>        if(List[pos+1] >= Lba)
//                return (pos+2);        <=>            break;
//            break;                     <=>        return (pos+2);
    __asm cmp   [edi+esi*4+4],ecx
    __asm jae   EO_while_1
    __asm add   esi,2
    __asm jmp   EO_while_2
//        }
NO_r_sub_l_eq_1:
//        if(List[pos] < Lba) {
    __asm cmp   eax,ecx
    __asm jae   Update_r
//            left = pos;
    __asm add   ebx,edx
    __asm mov   edx,esi
    __asm jmp   While_1
//        } else {
Update_r:
//            right = pos;
    __asm mov   ebx,esi
    __asm jmp   While_1
//        }
//    }
EO_while_1:
//    if((List[pos] < Lba) && ((pos+1) <= BlockCount)) pos++;
    __asm mov   eax,[edi+esi*4]
    __asm cmp   eax,ecx
    __asm jae   EO_while_2
    __asm inc   esi
    __asm cmp   esi,BlockCount
    __asm jbe   EO_while_2
    __asm dec   esi
EO_while_2:
//  return pos;
    __asm mov   eax,esi

    __asm pop   edi
    __asm pop   esi
    __asm pop   edx
    __asm pop   ebx
    __asm pop   ecx

#else   // NO X86 optimization , use generic C/C++

    ULONG pos;
    ULONG left;
    ULONG right;

    if(!BlockCount)
        return 0;

    left = 0;
    right = BlockCount - 1;
    pos = 0;
    while(left != right) {
        pos = (left + right) >> 1;
        if(List[pos] == Lba)
            return pos;
        if(right - left == 1) {
            if(List[pos+1] < Lba)
                return (pos+2);
            break;
        }
        if(List[pos] < Lba) {
            left = pos;
        } else {
            right = pos;
        }
    }
    if((List[pos] < Lba) && ((pos+1) <= BlockCount)) pos++;

    return pos;

#endif // _X86_

}

#ifdef _MSC_VER
#pragma warning(pop) // re-enable warning #4035
#endif

/*
  WCacheInsertRangeToList() inserts values laying in range described
  by Lba (1st value) and BCount (number of sequentially incremented
  values) in sorted array of ULONGs pointed by List.
  Ex.: (Lba, BCount)=(7,3) will insert values {7,8,9}.
  If target array already contains one or more values falling in
  requested range, they will be removed before insertion.
  WCacheInsertRangeToList() updates value of (*BlockCount) to reflect
  performed changes.
  WCacheInsertRangeToList() assumes that target array is of enough size.
  Internal routine
 */
VOID
__fastcall
WCacheInsertRangeToList(
    IN lba_t* List,           // pointer to sorted (ASC) array of ULONGs
    IN PULONG BlockCount,     // pointer to number of items in array (pointed by List)
    IN lba_t Lba,             // initial value for insertion
    IN ULONG BCount           // number of sequentially incremented values to be inserted
    )
{
    if(!BCount)
        return;

    ASSERT(!(BCount & 0x80000000));

    ULONG firstPos = WCacheGetSortedListIndex(*BlockCount, List, Lba);
    ULONG lastPos = WCacheGetSortedListIndex(*BlockCount, List, Lba+BCount);
    ULONG offs = firstPos + BCount - lastPos;

    if(offs) {
        // move list tail
//        ASSERT(lastPos+offs + ((*BlockCount) - lastPos) <= qq);
        if(*BlockCount) {
#ifdef WCACHE_BOUND_CHECKS
            MyCheckArray(List, lastPos+offs+(*BlockCount)-lastPos-1);
#endif //WCACHE_BOUND_CHECKS
            DbgMoveMemory(&(List[lastPos+offs]), &(List[lastPos]), ((*BlockCount) - lastPos) * sizeof(ULONG));
        }
        lastPos += offs;
        for(; firstPos<lastPos; firstPos++) {
#ifdef WCACHE_BOUND_CHECKS
            MyCheckArray(List, firstPos);
#endif //WCACHE_BOUND_CHECKS
            List[firstPos] = Lba;
            Lba++;
        }
        (*BlockCount) += offs;
    }
} // end WCacheInsertRangeToList()

/*
  WCacheInsertItemToList() inserts value Lba in sorted array of
  ULONGs pointed by List.
  If target array already contains requested value, no
  operations are performed.
  WCacheInsertItemToList() updates value of (*BlockCount) to reflect
  performed changes.
  WCacheInsertItemToList() assumes that target array is of enough size.
  Internal routine
 */
VOID
__fastcall
WCacheInsertItemToList(
    IN lba_t* List,           // pointer to sorted (ASC) array of lba_t's
    IN PULONG BlockCount,     // pointer to number of items in array (pointed by List)
    IN lba_t Lba              // value to be inserted
    )
{
    ULONG firstPos = WCacheGetSortedListIndex(*BlockCount, List, Lba+1);
    if(firstPos && (List[firstPos-1] == Lba))
        return;

    // move list tail
    if(*BlockCount) {
#ifdef WCACHE_BOUND_CHECKS
        MyCheckArray(List, firstPos+1+(*BlockCount)-firstPos-1);
#endif //WCACHE_BOUND_CHECKS
//        DbgMoveMemory(&(List[firstPos+1]), &(List[firstPos]), ((*BlockCount) - firstPos)*sizeof(ULONG));
        DbgMoveMemory(&(List[firstPos+1]), &(List[firstPos]), ((*BlockCount) - firstPos) * sizeof(ULONG));
    }
#ifdef WCACHE_BOUND_CHECKS
    MyCheckArray(List, firstPos);
#endif //WCACHE_BOUND_CHECKS
    List[firstPos] = Lba;
    (*BlockCount) ++;
} // end WCacheInsertItemToList()

/*
  WCacheRemoveRangeFromList() removes values falling in range described
  by Lba (1st value) and BCount (number of sequentially incremented
  values) from sorted array of ULONGs pointed by List.
  Ex.: (Lba, BCount)=(7,3) will remove values {7,8,9}.
  If target array doesn't contain values falling in
  requested range, no operation is performed.
  WCacheRemoveRangeFromList() updates value of (*BlockCount) to reflect
  performed changes.
  Internal routine
 */
VOID
__fastcall
WCacheRemoveRangeFromList(
    IN lba_t* List,           // pointer to sorted (ASC) array of ULONGs
    IN PULONG BlockCount,     // pointer to number of items in array (pointed by List)
    IN lba_t Lba,             // initial value for removal
    IN ULONG BCount           // number of sequentially incremented values to be removed
    )
{
    ULONG firstPos = WCacheGetSortedListIndex(*BlockCount, List, Lba);
    ULONG lastPos = WCacheGetSortedListIndex(*BlockCount, List, Lba+BCount);
    ULONG offs = lastPos - firstPos;

    if(offs) {
        // move list tail
        DbgMoveMemory(&(List[lastPos-offs]), &(List[lastPos]), ((*BlockCount) - lastPos) * sizeof(ULONG));
        (*BlockCount) -= offs;
    }
} // end WCacheRemoveRangeFromList()

/*
  WCacheRemoveItemFromList() removes value Lba from sorted array
  of ULONGs pointed by List.
  If target array doesn't contain requested value, no
  operations are performed.
  WCacheRemoveItemFromList() updates value of (*BlockCount) to reflect
  performed changes.
  Internal routine
 */
VOID
__fastcall
WCacheRemoveItemFromList(
    IN lba_t* List,           // pointer to sorted (ASC) array of ULONGs
    IN PULONG BlockCount,     // pointer to number of items in array (pointed by List)
    IN lba_t Lba              // value to be removed
    )
{
    if(!(*BlockCount)) return;
    ULONG lastPos = WCacheGetSortedListIndex(*BlockCount, List, Lba+1);
    if(!lastPos || (lastPos && (List[lastPos-1] != Lba)))
        return;

    // move list tail
    DbgMoveMemory(&(List[lastPos-1]), &(List[lastPos]), ((*BlockCount) - lastPos) * sizeof(ULONG));
    (*BlockCount) --;
} // end WCacheRemoveItemFromList()

/*
  WCacheInitFrame() allocates storage for Frame (block_array)
  with index 'frame', fills it with 0 (none of Blocks from
  this Frame is cached) and inserts it's index to sorted array
  of frame indexes.
  WCacheInitFrame() also checks if number of frames reaches limit
  and invokes WCacheCheckLimits() to free some Frames/Blocks
  Internal routine
 */
PW_CACHE_ENTRY
__fastcall
WCacheInitFrame(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // caller's context (currently unused)
    IN ULONG frame            // frame index
    )
{
    PW_CACHE_ENTRY block_array;
    ULONG l;
#ifdef DBG
    ULONG old_count = Cache->FrameCount;
#endif //DBG

    // We are about to add new cache frame.
    // Thus check if we have enough free entries and
    // flush unused ones if it is neccessary.
    if(Cache->FrameCount >= Cache->MaxFrames) {
        BrutePoint();
        WCacheCheckLimits(Cache, Context, frame << Cache->BlocksPerFrameSh, Cache->PacketSize*2);
    }
    ASSERT(Cache->FrameCount < Cache->MaxFrames);
    block_array = (PW_CACHE_ENTRY)MyAllocatePoolTag__(NonPagedPool, l = sizeof(W_CACHE_ENTRY) << Cache->BlocksPerFrameSh, MEM_WCFRM_TAG);
    Cache->FrameList[frame].Frame = block_array;

    // Keep history !!!
    //Cache->FrameList[frame].UpdateCount = 0;
    //Cache->FrameList[frame].AccessCount = 0;

    if(block_array) {
        ASSERT((ULONG_PTR)block_array > 0x1000);
        WCacheInsertItemToList(Cache->CachedFramesList, &(Cache->FrameCount), frame);
        RtlZeroMemory(block_array, l);
    } else {
        BrutePoint();
    }
    ASSERT(Cache->FrameCount <= Cache->MaxFrames);
#ifdef DBG
    ASSERT(old_count < Cache->FrameCount);
#endif //DBG
    return block_array;
} // end WCacheInitFrame()

/*
  WCacheRemoveFrame() frees storage for Frame (block_array) with
  index 'frame' and removes it's index from sorted array of
  frame indexes.
  Internal routine
 */
VOID
__fastcall
WCacheRemoveFrame(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user's context (currently unused)
    IN ULONG frame            // frame index
    )
{
    PW_CACHE_ENTRY block_array;
#ifdef DBG
    ULONG old_count = Cache->FrameCount;
#endif //DBG

    ASSERT(Cache->FrameCount <= Cache->MaxFrames);
    block_array = Cache->FrameList[frame].Frame;

    WCacheRemoveItemFromList(Cache->CachedFramesList, &(Cache->FrameCount), frame);
    MyFreePool__(block_array);
//    ASSERT(!(Cache->FrameList[frame].WriteCount));
//    ASSERT(!(Cache->FrameList[frame].WriteCount));
    Cache->FrameList[frame].Frame = NULL;
    ASSERT(Cache->FrameCount < Cache->MaxFrames);
#ifdef DBG
    ASSERT(old_count > Cache->FrameCount);
#endif //DBG

} // end WCacheRemoveFrame()

/*
  WCacheSetModFlag() sets Modified flag for Block with offset 'i'
  in Frame 'block_array'
  Internal routine
 */
#define WCacheSetModFlag(block_array, i) \
    *((PULONG)&(block_array[i].Sector)) |= WCACHE_FLAG_MODIFIED

/*
  WCacheClrModFlag() clears Modified flag for Block with offset 'i'
  in Frame 'block_array'
  Internal routine
 */
#define WCacheClrModFlag(block_array, i) \
    *((PULONG)&(block_array[i].Sector)) &= ~WCACHE_FLAG_MODIFIED

/*
  WCacheGetModFlag() returns non-zero value if Modified flag for
  Block with offset 'i' in Frame 'block_array' is set. Otherwise
  0 is returned.
  Internal routine
 */
#define WCacheGetModFlag(block_array, i) \
    (*((PULONG)&(block_array[i].Sector)) & WCACHE_FLAG_MODIFIED)

#if 0
/*
  WCacheSetBadFlag() sets Modified flag for Block with offset 'i'
  in Frame 'block_array'
  Internal routine
 */
#define WCacheSetBadFlag(block_array, i) \
    *((PULONG)&(block_array[i].Sector)) |= WCACHE_FLAG_BAD

/*
  WCacheClrBadFlag() clears Modified flag for Block with offset 'i'
  in Frame 'block_array'
  Internal routine
 */
#define WCacheClrBadFlag(block_array, i) \
    *((PULONG)&(block_array[i].Sector)) &= ~WCACHE_FLAG_BAD

/*
  WCacheGetBadFlag() returns non-zero value if Modified flag for
  Block with offset 'i' in Frame 'block_array' is set. Otherwise
  0 is returned.
  Internal routine
 */
#define WCacheGetBadFlag(block_array, i) \
    (((UCHAR)(block_array[i].Sector)) & WCACHE_FLAG_BAD)
#endif //0

/*
  WCacheSectorAddr() returns pointer to memory block containing cached
  data for Block described by Frame (block_array) and offset in this
  Frame (i). If requested Block is not cached yet NULL is returned.
  Internal routine
 */
#define WCacheSectorAddr(block_array, i) \
    ((ULONG_PTR)(block_array[i].Sector) & WCACHE_ADDR_MASK)

/*
  WCacheFreeSector() releases memory block containing cached
  data for Block described by Frame (block_array) and offset in this
  Frame (i). Should never be called for non-cached Blocks.
  Internal routine
 */
#define WCacheFreeSector(frame, offs) \
{                          \
    DbgFreePool((PVOID)WCacheSectorAddr(block_array, offs)); \
    block_array[offs].Sector = NULL; \
    Cache->FrameList[frame].BlockCount--; \
}

/*
  WCacheAllocAsyncEntry() allocates storage for async IO context,
  links it to previously allocated async IO context (if any),
  initializes synchronization (completion) event
  and allocates temporary IO buffers.
  Async IO contexts are used to create chained set of IO requests
  durring top-level request precessing and wait for their completion.
  Internal routine
 */
PW_CACHE_ASYNC
WCacheAllocAsyncEntry(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN OUT PW_CACHE_ASYNC* FirstWContext, // pointer to the pointer to
                              //   the head of async IO context chain
    IN OUT PW_CACHE_ASYNC* PrevWContext,  // pointer to the storage for pointer
                              //   to newly allocated async IO context chain
    IN ULONG BufferSize       // requested IO buffer size
    )
{
    PW_CACHE_ASYNC WContext;
    PCHAR Buffer;

    WContext = (PW_CACHE_ASYNC)MyAllocatePoolTag__(NonPagedPool,sizeof(W_CACHE_ASYNC), MEM_WCCTX_TAG);
    if(!WContext)
        return NULL;
    Buffer = (PCHAR)DbgAllocatePoolWithTag(NonPagedPool, BufferSize*(2-Cache->Chained), MEM_WCBUF_TAG);
    if(!Buffer) {
        MyFreePool__(WContext);
        return NULL;
    }

    if(!Cache->Chained)
        KeInitializeEvent(&(WContext->PhContext.event), SynchronizationEvent, FALSE);
    WContext->Cache = Cache;
    if(*PrevWContext)
        (*PrevWContext)->NextWContext = WContext;
//    WContext->NextWContext = (*PrevWContext);
    WContext->NextWContext = NULL;
    WContext->Buffer = Buffer;
    WContext->Buffer2 = Buffer+(Cache->Chained ? 0 : BufferSize);

    if(!(*FirstWContext))
        (*FirstWContext) = WContext;
    (*PrevWContext) = WContext;

    return WContext;
} // end WCacheAllocAsyncEntry()

/*
  WCacheFreeAsyncEntry() releases storage previously allocated for
  async IO context.
  Internal routine
 */
VOID
WCacheFreeAsyncEntry(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    PW_CACHE_ASYNC WContext   // pointer to async IO context to release
    )
{
    DbgFreePool(WContext->Buffer);
    MyFreePool__(WContext);
} // end WCacheFreeAsyncEntry()

//#define WCacheRaiseIoError(c, ct, s, l, bc, b, o, r)

OSSTATUS
WCacheRaiseIoError(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,
    IN OSSTATUS Status,
    IN ULONG Lba,
    IN ULONG BCount,
    IN PVOID Buffer,
    IN BOOLEAN ReadOp,
    IN PBOOLEAN Retry
    )
{
    if(!Cache->ErrorHandlerProc)
        return Status;

    WCACHE_ERROR_CONTEXT ec;

    ec.WCErrorCode = ReadOp ? WCACHE_ERROR_READ : WCACHE_ERROR_WRITE;
    ec.Status = Status;
    ec.ReadWrite.Lba    = Lba;
    ec.ReadWrite.BCount = BCount;
    ec.ReadWrite.Buffer = Buffer;
    Status = Cache->ErrorHandlerProc(Context, &ec);
    if(Retry)
        (*Retry) = ec.Retry;

    return Status;

} // end WCacheRaiseIoError()

/*
  WCacheUpdatePacket() attempts to updates packet containing target Block.
  If async IO is enabled new IO context is added to the chain.
  If packet containing target Block is modified and PrefereWrite flag
  is NOT set, function returns with status STATUS_RETRY. This setting is
  user in WCACHE_MODE_R mode to reduce physical writes on flush.
  'State' parameter is used in async mode to determine the next processing
  stege for given request
  Internal routine
 */
OSSTATUS
WCacheUpdatePacket(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user's context to be passed to user-supplied
                              //   low-level IO routines (IO callbacks)
    IN OUT PW_CACHE_ASYNC* FirstWContext, // pointer to head async IO context
    IN OUT PW_CACHE_ASYNC* PrevWContext,  // pointer to tail async IO context
    IN PW_CACHE_ENTRY block_array, // pointer to target Frame
    IN lba_t firstLba,        // LBA of the 1st block in target Frame
    IN lba_t Lba,             // LBA of target Block
    IN ULONG BSh,             // bit shift for Block size
    IN ULONG BS,              // Block size (bytes)
    IN ULONG PS,              // Packet size (bytes)
    IN ULONG PSs,             // Packet size (sectors)
    IN PSIZE_T ReadBytes,      // pointer to number of successfully read/written bytes
    IN BOOLEAN PrefereWrite,  // allow physical write (flush) of modified packet
    IN ULONG State            // callers state
    )
{
    OSSTATUS status;
    PCHAR tmp_buff = Cache->tmp_buff;
    PCHAR tmp_buff2 = Cache->tmp_buff;
    BOOLEAN mod;
    BOOLEAN read;
    BOOLEAN zero;
    ULONG i;
    lba_t Lba0;
    PW_CACHE_ASYNC WContext;
    BOOLEAN Async = (Cache->ReadProcAsync && Cache->WriteProcAsync);
    ULONG block_type;
    BOOLEAN Chained = Cache->Chained;

    // Check if we are going to write down to disk
    // all prewiously prepared (chained) data
    if(State == ASYNC_STATE_WRITE) {
        WContext = (*PrevWContext);
        tmp_buff  = (PCHAR)(WContext->Buffer);
        tmp_buff2 = (PCHAR)(WContext->Buffer2);
        if(!Chained)
            mod = (DbgCompareMemory(tmp_buff2, tmp_buff, PS) != PS);
        goto try_write;
    }

    // Check if packet contains modified blocks
    // If packet contains non-cached and unchanged, but used
    // blocks, it must be read from media before modification
    mod = read = zero = FALSE;
    Lba0 = Lba - firstLba;
    for(i=0; i<PSs; i++, Lba0++) {
        if(WCacheGetModFlag(block_array, Lba0)) {
            mod = TRUE;
        } else if(!WCacheSectorAddr(block_array,Lba0) &&
                  ((block_type = Cache->CheckUsedProc(Context, Lba+i)) & WCACHE_BLOCK_USED) ) {
            //
            if(block_type & WCACHE_BLOCK_ZERO) {
                zero = TRUE;
            } else {
                read = TRUE;
            }
        }
    }
    // check if we are allowed to write to media
    if(mod && !PrefereWrite) {
        return STATUS_RETRY;
    }
    // return STATUS_SUCCESS if requested packet contains no modified blocks
    if(!mod) {
        (*ReadBytes) = PS;
        return STATUS_SUCCESS;
    }

    // pefrorm full update cycle: prepare(optional)/read/modify/write

    // do some preparations
    if(Chained || Async) {
        // For chained and async I/O we allocates context entry
        // and add it to list (chain)
        // We shall only read data to temporary buffer and
        // modify it. Write operations will be invoked later.
        // This is introduced in order to avoid frequent
        // read.write mode switching, because it significantly degrades
        // performance
        WContext = WCacheAllocAsyncEntry(Cache, FirstWContext, PrevWContext, PS);
        if(!WContext) {
            //return STATUS_INSUFFICIENT_RESOURCES;
            // try to recover
            Chained = FALSE;
            Async = FALSE;
        } else {
            tmp_buff = tmp_buff2 = (PCHAR)(WContext->Buffer);
            WContext->Lba = Lba;
            WContext->Cmd = ASYNC_CMD_UPDATE;
            WContext->State = ASYNC_STATE_NONE;
        }
    }

    // read packet (if it necessary)
    if(read) {
        if(Async) {
            WContext->State = ASYNC_STATE_READ;
            status = Cache->ReadProcAsync(Context, WContext, tmp_buff, PS, Lba,
                                           &(WContext->TransferredBytes));
//                tmp_buff2 = (PCHAR)(WContext->Buffer2);
            (*ReadBytes) = PS;
            return status;
        } else {
            status = Cache->ReadProc(Context, tmp_buff, PS, Lba, ReadBytes, PH_TMP_BUFFER);
        }
        if(!OS_SUCCESS(status)) {
            status = WCacheRaiseIoError(Cache, Context, status, Lba, PSs, tmp_buff, WCACHE_R_OP, NULL);
            if(!OS_SUCCESS(status)) {
                return status;
            }
        }
    } else
    if(zero) {
        RtlZeroMemory(tmp_buff, PS);
    }

    if(Chained) {
        // indicate that we prepared for writing block to disk
        WContext->State = ASYNC_STATE_WRITE_PRE;
        tmp_buff2 = tmp_buff;
        status = STATUS_SUCCESS;
    }

    // modify packet

    // If we didn't read packet from media, we can't
    // perform comparison to assure that packet was really modified.
    // Thus, assume that it is modified in this case.
    mod = !read || Cache->DoNotCompare;
    Lba0 = Lba - firstLba;
    for(i=0; i<PSs; i++, Lba0++) {
        if( WCacheGetModFlag(block_array, Lba0) ||
            (!read && WCacheSectorAddr(block_array,Lba0)) ) {

#ifdef _NTDEF_
            ASSERT((ULONG)WCacheSectorAddr(block_array,Lba0) & 0x80000000);
#endif //_NTDEF_
            if(!mod) {
                ASSERT(read);
                mod = (DbgCompareMemory(tmp_buff2 + (i << BSh),
                            (PVOID)WCacheSectorAddr(block_array, Lba0),
                            BS) != BS);
            }
            if(mod) {
                DbgCopyMemory(tmp_buff2 + (i << BSh),
                            (PVOID)WCacheSectorAddr(block_array, Lba0),
                            BS);
            }
        }
    }

    if(Chained &&
       WContext->State == ASYNC_STATE_WRITE_PRE) {
        // Return if block is prepared for write and we are in chained mode.
        if(!mod) {
            // Mark block as written if we have found that data in it
            // is not actually modified.
            WContext->State = ASYNC_STATE_DONE;
            (*ReadBytes) = PS;
        }
        return STATUS_SUCCESS;
    }

    // write packet

    // If the check above reported some changes in packet
    // we should write packet out to media.
    // Otherwise, just complete request.
    if(mod) {
try_write:
        if(Async) {
            WContext->State = ASYNC_STATE_WRITE;
            status = Cache->WriteProcAsync(Context, WContext, tmp_buff2, PS, Lba,
                                           &(WContext->TransferredBytes), FALSE);
            (*ReadBytes) = PS;
        } else {
            status = Cache->WriteProc(Context, tmp_buff2, PS, Lba, ReadBytes, 0);
            if(!OS_SUCCESS(status)) {
                status = WCacheRaiseIoError(Cache, Context, status, Lba, PSs, tmp_buff2, WCACHE_W_OP, NULL);
            }
        }
    } else {
        if(Async)
            WCacheCompleteAsync__(WContext, STATUS_SUCCESS);
        (*ReadBytes) = PS;
        return STATUS_SUCCESS;
    }

    return status;
} // end WCacheUpdatePacket()

/*
  WCacheFreePacket() releases storage for all Blocks in packet.
  'frame' describes Frame, offset - Block in Frame. offset should be
  aligned on Packet size.
  Internal routine
 */
VOID
WCacheFreePacket(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
//    IN PVOID Context,
    IN ULONG frame,           // Frame index
    IN PW_CACHE_ENTRY block_array, // Frame
    IN ULONG offs,            // offset in Frame
    IN ULONG PSs              // Packet size (in Blocks)
    )
{
    ULONG i;
    // mark as non-cached & free pool
    for(i=0; i<PSs; i++, offs++) {
        if(WCacheSectorAddr(block_array,offs)) {
            WCacheFreeSector(frame, offs);
        }
    }
} // end WCacheFreePacket()

/*
  WCacheUpdatePacketComplete() is called to continue processing of packet
  being updated.
  In async mode it waits for completion of pre-read requests,
  initiates writes, waits for their completion and returns control to
  caller.
  Internal routine
 */
VOID
WCacheUpdatePacketComplete(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN OUT PW_CACHE_ASYNC* FirstWContext, // pointer to head async IO context
    IN OUT PW_CACHE_ASYNC* PrevWContext,  // pointer to tail async IO context
    IN BOOLEAN FreePacket
    )
{
    PW_CACHE_ASYNC WContext = (*FirstWContext);
    if(!WContext)
        return;
    PW_CACHE_ASYNC NextWContext;
    ULONG PS = Cache->BlockSize << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    ULONG frame;
    lba_t firstLba;

    // Walk through all chained blocks and wait
    // for completion of read operations.
    // Also invoke writes of already prepared packets.
    while(WContext) {
        if(WContext->Cmd == ASYNC_CMD_UPDATE &&
           WContext->State == ASYNC_STATE_READ) {
            // wait for async read for update
            DbgWaitForSingleObject(&(WContext->PhContext.event), NULL);

            WContext->State = ASYNC_STATE_WRITE;
            WCacheUpdatePacket(Cache, Context, NULL, &WContext, NULL, -1, WContext->Lba, -1, -1,
                               PS, -1, &(WContext->TransferredBytes), TRUE, ASYNC_STATE_WRITE);
        } else
        if(WContext->Cmd == ASYNC_CMD_UPDATE &&
           WContext->State == ASYNC_STATE_WRITE_PRE) {
            // invoke physical write it the packet is prepared for writing
            // by previuous call to WCacheUpdatePacket()
            WContext->State = ASYNC_STATE_WRITE;
            WCacheUpdatePacket(Cache, Context, NULL, &WContext, NULL, -1, WContext->Lba, -1, -1,
                               PS, -1, &(WContext->TransferredBytes), TRUE, ASYNC_STATE_WRITE);
            WContext->State = ASYNC_STATE_DONE;
        } else
        if(WContext->Cmd == ASYNC_CMD_READ &&
           WContext->State == ASYNC_STATE_READ) {
            // wait for async read
            DbgWaitForSingleObject(&(WContext->PhContext.event), NULL);
        }
        WContext = WContext->NextWContext;
    }
    // Walk through all chained blocks and wait
    // and wait for completion of async writes (if any).
    // Also free temporary buffers containing already written blocks.
    WContext = (*FirstWContext);
    while(WContext) {
        NextWContext = WContext->NextWContext;
        if(WContext->Cmd == ASYNC_CMD_UPDATE &&
           WContext->State == ASYNC_STATE_WRITE) {

            if(!Cache->Chained)
                DbgWaitForSingleObject(&(WContext->PhContext.event), NULL);

            frame = WContext->Lba >> Cache->BlocksPerFrameSh;
            firstLba = frame << Cache->BlocksPerFrameSh;

            if(FreePacket) {
                WCacheFreePacket(Cache, frame,
                                Cache->FrameList[frame].Frame,
                                WContext->Lba - firstLba, PSs);
            }
        }
        WCacheFreeAsyncEntry(Cache, WContext);
        WContext = NextWContext;
    }
    (*FirstWContext) = NULL;
    (*PrevWContext) = NULL;
} // end WCacheUpdatePacketComplete()

/*
  WCacheCheckLimits() checks if we've enough free Frame- &
  Block-entries under Frame- and Block-limit to feet
  requested Blocks.
  If there is not enough entries, WCache initiates flush & purge
  process to satisfy request.
  This is dispatch routine, which calls
  WCacheCheckLimitsR() or WCacheCheckLimitsRW() depending on
  media type.
  Internal routine
 */
OSSTATUS
__fastcall
WCacheCheckLimits(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t ReqLba,          // first LBA to access/cache
    IN ULONG BCount           // number of Blocks to access/cache
    )
{
/*    if(!Cache->FrameCount || !Cache->BlockCount) {
        ASSERT(!Cache->FrameCount);
        ASSERT(!Cache->BlockCount);
        if(!Cache->FrameCount)
            return STATUS_SUCCESS;
    }*/

    // check if we have reached Frame or Block limit
    if(!Cache->FrameCount && !Cache->BlockCount) {
        return STATUS_SUCCESS;
    }

    // check for empty frames
    if(Cache->FrameCount > (Cache->MaxFrames*3)/4) {
        ULONG frame;
        ULONG i;
        for(i=Cache->FrameCount; i>0; i--) {
            frame = Cache->CachedFramesList[i-1];
            // check if frame is empty
            if(!(Cache->FrameList[frame].BlockCount)) {
                WCacheRemoveFrame(Cache, Context, frame);
            } else {
                ASSERT(Cache->FrameList[frame].Frame);
            }
        }
    }

    if(!Cache->BlockCount) {
        return STATUS_SUCCESS;
    }

    // invoke media-specific limit-checker
    switch(Cache->Mode) {
    case WCACHE_MODE_RAM:
        return WCacheCheckLimitsRAM(Cache, Context, ReqLba, BCount);
    case WCACHE_MODE_ROM:
    case WCACHE_MODE_RW:
        return WCacheCheckLimitsRW(Cache, Context, ReqLba, BCount);
    case WCACHE_MODE_R:
        return WCacheCheckLimitsR(Cache, Context, ReqLba, BCount);
    }
    return STATUS_DRIVER_INTERNAL_ERROR;
} // end WCacheCheckLimits()

/*
  WCacheCheckLimitsRW() implements automatic flush and purge of
  unused blocks to keep enough free cache entries for newly
  read/written blocks for Random Access and ReWritable media
  using Read/Modify/Write technology.
  See also WCacheCheckLimits()
  Internal routine
 */
OSSTATUS
__fastcall
WCacheCheckLimitsRW(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t ReqLba,          // first LBA to access/cache
    IN ULONG BCount           // number of Blocks to access/cache
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t lastLba;
    lba_t Lba;
//    PCHAR tmp_buff = Cache->tmp_buff;
    ULONG firstPos;
    ULONG lastPos;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    ULONG try_count = 0;
    PW_CACHE_ENTRY block_array;
    OSSTATUS status;
    SIZE_T ReadBytes;
    ULONG FreeFrameCount = 0;
//    PVOID addr;
    PW_CACHE_ASYNC FirstWContext = NULL;
    PW_CACHE_ASYNC PrevWContext = NULL;
    ULONG chain_count = 0;

    if(Cache->FrameCount >= Cache->MaxFrames) {
        FreeFrameCount = Cache->FramesToKeepFree;
    } else
    if((Cache->BlockCount + WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba) +
           BCount - WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba+BCount)) > Cache->MaxBlocks) {
        // we need free space to grow WCache without flushing data
        // for some period of time
        FreeFrameCount = Cache->FramesToKeepFree;
        goto Try_Another_Frame;
    }
    // remove(flush) some frames
    while((Cache->FrameCount >= Cache->MaxFrames) || FreeFrameCount) {
Try_Another_Frame:
        if(!Cache->FrameCount || !Cache->BlockCount) {
            //ASSERT(!Cache->FrameCount);
            if(Cache->FrameCount) {
                UDFPrint(("ASSERT: Cache->FrameCount = %d, when 0 is expected\n", Cache->FrameCount));
            }
            ASSERT(!Cache->BlockCount);
            if(!Cache->FrameCount)
                break;
        }

        frame = WCacheFindFrameToRelease(Cache);
#if 0
        if(Cache->FrameList[frame].WriteCount) {
            try_count++;
            if(try_count < MAX_TRIES_FOR_NA) goto Try_Another_Frame;
        } else {
            try_count = 0;
        }
#else
        if(Cache->FrameList[frame].UpdateCount) {
            try_count = MAX_TRIES_FOR_NA;
        } else {
            try_count = 0;
        }
#endif

        if(FreeFrameCount)
            FreeFrameCount--;

        firstLba = frame << Cache->BlocksPerFrameSh;
        lastLba = firstLba + Cache->BlocksPerFrame;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, firstLba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, lastLba);
        block_array = Cache->FrameList[frame].Frame;

        if(!block_array) {
            UDFPrint(("Hmm...\n"));
            BrutePoint();
            return STATUS_DRIVER_INTERNAL_ERROR;
        }

        while(firstPos < lastPos) {
            // flush packet
            Lba = List[firstPos] & ~(PSs-1);

            // write packet out or prepare and add to chain (if chained mode enabled)
            status = WCacheUpdatePacket(Cache, Context, &FirstWContext, &PrevWContext, block_array, firstLba,
                Lba, BSh, BS, PS, PSs, &ReadBytes, TRUE, ASYNC_STATE_NONE);

            if(status != STATUS_PENDING) {
                // free memory
                WCacheFreePacket(Cache, frame, block_array, Lba-firstLba, PSs);
            }

            Lba += PSs;
            while((firstPos < lastPos) && (Lba > List[firstPos])) {
                firstPos++;
            }
            chain_count++;
            // write chained packets
            if(chain_count >= WCACHE_MAX_CHAIN) {
                WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
                chain_count = 0;
            }
        }
        // remove flushed blocks from all lists
        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), firstLba, Cache->BlocksPerFrame);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), firstLba, Cache->BlocksPerFrame);

        WCacheRemoveFrame(Cache, Context, frame);
    }

    // check if we try to read too much data
    if(BCount > Cache->MaxBlocks) {
        WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext);
        return STATUS_INVALID_PARAMETER;
    }

    // remove(flush) packet
    while((Cache->BlockCount + WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba) +
           BCount - WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba+BCount)) > Cache->MaxBlocks) {
        try_count = 0;
Try_Another_Block:

        Lba = WCacheFindLbaToRelease(Cache) & ~(PSs-1);
        if(Lba == WCACHE_INVALID_LBA) {
            ASSERT(!Cache->FrameCount);
            ASSERT(!Cache->BlockCount);
            break;
        }
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba+PSs);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            // write already prepared blocks to disk and return error
            WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext);
            ASSERT(FALSE);
            return STATUS_DRIVER_INTERNAL_ERROR;
        }

        // write packet out or prepare and add to chain (if chained mode enabled)
        status = WCacheUpdatePacket(Cache, Context, &FirstWContext, &PrevWContext, block_array, firstLba,
            Lba, BSh, BS, PS, PSs, &ReadBytes, (try_count >= MAX_TRIES_FOR_NA), ASYNC_STATE_NONE);

        if(status == STATUS_RETRY) {
            try_count++;
            goto Try_Another_Block;
        }

        // free memory
        WCacheFreePacket(Cache, frame, block_array, Lba-firstLba, PSs);

        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), Lba, PSs);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, PSs);
        // check if frame is empty
        if(!(Cache->FrameList[frame].BlockCount)) {
            WCacheRemoveFrame(Cache, Context, frame);
        } else {
            ASSERT(Cache->FrameList[frame].Frame);
        }
        chain_count++;
        if(chain_count >= WCACHE_MAX_CHAIN) {
            WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
            chain_count = 0;
        }
    }
    WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext);
    return STATUS_SUCCESS;
} // end WCacheCheckLimitsRW()

OSSTATUS
__fastcall
WCacheFlushBlocksRAM(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    PW_CACHE_ENTRY block_array,
    lba_t* List,
    ULONG firstPos,
    ULONG lastPos,
    BOOLEAN Purge
    )
{
    ULONG frame;
    lba_t Lba;
    lba_t PrevLba;
    lba_t firstLba;
    PCHAR tmp_buff = NULL;
    ULONG n;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
//    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    SIZE_T _WrittenBytes;
    OSSTATUS status = STATUS_SUCCESS;

    frame = List[firstPos] >> Cache->BlocksPerFrameSh;
    firstLba = frame << Cache->BlocksPerFrameSh;

    while(firstPos < lastPos) {
        // flush blocks
        ASSERT(Cache->FrameCount <= Cache->MaxFrames);
        Lba = List[firstPos];
        if(!WCacheGetModFlag(block_array, Lba - firstLba)) {
            // free memory
            if(Purge) {
                WCacheFreePacket(Cache, frame, block_array, Lba-firstLba, 1);
            }
            firstPos++;
            continue;
        }
        tmp_buff = Cache->tmp_buff;
        PrevLba = Lba;
        n=1;
        while((firstPos+n < lastPos) &&
              (List[firstPos+n] == PrevLba+1)) {
            PrevLba++;
            if(!WCacheGetModFlag(block_array, PrevLba - firstLba))
                break;
            DbgCopyMemory(tmp_buff + (n << BSh),
                        (PVOID)WCacheSectorAddr(block_array, PrevLba - firstLba),
                        BS);
            n++;
            if(n >= PSs)
                break;
        }
        if(n > 1) {
            DbgCopyMemory(tmp_buff,
                        (PVOID)WCacheSectorAddr(block_array, Lba - firstLba),
                        BS);
        } else {
            tmp_buff = (PCHAR)WCacheSectorAddr(block_array, Lba - firstLba);
        }
        // write sectors out
        status = Cache->WriteProc(Context, tmp_buff, n<<BSh, Lba, &_WrittenBytes, 0);
        if(!OS_SUCCESS(status)) {
            status = WCacheRaiseIoError(Cache, Context, status, Lba, n, tmp_buff, WCACHE_W_OP, NULL);
            if(!OS_SUCCESS(status)) {
                BrutePoint();
            }
        }
        firstPos += n;
        if(Purge) {
            // free memory
            WCacheFreePacket(Cache, frame, block_array, Lba-firstLba, n);
        } else {
            // clear Modified flag
            ULONG i;
            Lba -= firstLba;
            for(i=0; i<n; i++) {
                WCacheClrModFlag(block_array, Lba+i);
            }
        }
    }

    return status;
} // end WCacheFlushBlocksRAM()

/*
  WCacheCheckLimitsRAM() implements automatic flush and purge of
  unused blocks to keep enough free cache entries for newly
  read/written blocks for Random Access media.
  See also WCacheCheckLimits()
  Internal routine
 */
OSSTATUS
__fastcall
WCacheCheckLimitsRAM(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t ReqLba,          // first LBA to access/cache
    IN ULONG BCount           // number of Blocks to access/cache
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t lastLba;
    lba_t Lba;
//    PCHAR tmp_buff = Cache->tmp_buff;
    ULONG firstPos;
    ULONG lastPos;
//    ULONG BSh = Cache->BlockSizeSh;
//    ULONG BS = Cache->BlockSize;
//    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
//    ULONG try_count = 0;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;
    ULONG FreeFrameCount = 0;
//    PVOID addr;

    if(Cache->FrameCount >= Cache->MaxFrames) {
        FreeFrameCount = Cache->FramesToKeepFree;
    } else
    if((Cache->BlockCount + WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba) +
           BCount - WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba+BCount)) > Cache->MaxBlocks) {
        // we need free space to grow WCache without flushing data
        // for some period of time
        FreeFrameCount = Cache->FramesToKeepFree;
        goto Try_Another_Frame;
    }
    // remove(flush) some frames
    while((Cache->FrameCount >= Cache->MaxFrames) || FreeFrameCount) {
        ASSERT(Cache->FrameCount <= Cache->MaxFrames);
Try_Another_Frame:
        if(!Cache->FrameCount || !Cache->BlockCount) {
            ASSERT(!Cache->FrameCount);
            ASSERT(!Cache->BlockCount);
            if(!Cache->FrameCount)
                break;
        }

        frame = WCacheFindFrameToRelease(Cache);
#if 0
        if(Cache->FrameList[frame].WriteCount) {
            try_count++;
            if(try_count < MAX_TRIES_FOR_NA) goto Try_Another_Frame;
        } else {
            try_count = 0;
        }
#else
/*
        if(Cache->FrameList[frame].UpdateCount) {
            try_count = MAX_TRIES_FOR_NA;
        } else {
            try_count = 0;
        }
*/
#endif

        if(FreeFrameCount)
            FreeFrameCount--;

        firstLba = frame << Cache->BlocksPerFrameSh;
        lastLba = firstLba + Cache->BlocksPerFrame;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, firstLba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, lastLba);
        block_array = Cache->FrameList[frame].Frame;

        if(!block_array) {
            UDFPrint(("Hmm...\n"));
            BrutePoint();
            return STATUS_DRIVER_INTERNAL_ERROR;
        }
        WCacheFlushBlocksRAM(Cache, Context, block_array, List, firstPos, lastPos, TRUE);

        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), firstLba, Cache->BlocksPerFrame);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), firstLba, Cache->BlocksPerFrame);
        WCacheRemoveFrame(Cache, Context, frame);
    }

    // check if we try to read too much data
    if(BCount > Cache->MaxBlocks) {
        return STATUS_INVALID_PARAMETER;
    }

    // remove(flush) packet
    while((Cache->BlockCount + WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba) +
           BCount - WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba+BCount)) > Cache->MaxBlocks) {
//        try_count = 0;
//Try_Another_Block:

        ASSERT(Cache->FrameCount <= Cache->MaxFrames);
        Lba = WCacheFindLbaToRelease(Cache) & ~(PSs-1);
        if(Lba == WCACHE_INVALID_LBA) {
            ASSERT(!Cache->FrameCount);
            ASSERT(!Cache->BlockCount);
            break;
        }
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba+PSs);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            ASSERT(FALSE);
            return STATUS_DRIVER_INTERNAL_ERROR;
        }
        WCacheFlushBlocksRAM(Cache, Context, block_array, List, firstPos, lastPos, TRUE);
        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), Lba, PSs);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, PSs);
        // check if frame is empty
        if(!(Cache->FrameList[frame].BlockCount)) {
            WCacheRemoveFrame(Cache, Context, frame);
        } else {
            ASSERT(Cache->FrameList[frame].Frame);
        }
    }
    return STATUS_SUCCESS;
} // end WCacheCheckLimitsRAM()

/*
  WCachePurgeAllRAM()
  Internal routine
 */
OSSTATUS
__fastcall
WCachePurgeAllRAM(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context          // user-supplied context for IO callbacks
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t lastLba;
    ULONG firstPos;
    ULONG lastPos;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;

    // remove(flush) some frames
    while(Cache->FrameCount) {

        frame = Cache->CachedFramesList[0];

        firstLba = frame << Cache->BlocksPerFrameSh;
        lastLba = firstLba + Cache->BlocksPerFrame;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, firstLba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, lastLba);
        block_array = Cache->FrameList[frame].Frame;

        if(!block_array) {
            UDFPrint(("Hmm...\n"));
            BrutePoint();
            return STATUS_DRIVER_INTERNAL_ERROR;
        }
        WCacheFlushBlocksRAM(Cache, Context, block_array, List, firstPos, lastPos, TRUE);

        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), firstLba, Cache->BlocksPerFrame);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), firstLba, Cache->BlocksPerFrame);
        WCacheRemoveFrame(Cache, Context, frame);
    }

    ASSERT(!Cache->FrameCount);
    ASSERT(!Cache->BlockCount);
    return STATUS_SUCCESS;
} // end WCachePurgeAllRAM()

/*
  WCacheFlushAllRAM()
  Internal routine
 */
OSSTATUS
__fastcall
WCacheFlushAllRAM(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context          // user-supplied context for IO callbacks
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t lastLba;
    ULONG firstPos;
    ULONG lastPos;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;

    // flush frames
    while(Cache->WriteCount) {

        frame = Cache->CachedModifiedBlocksList[0] >> Cache->BlocksPerFrameSh;

        firstLba = frame << Cache->BlocksPerFrameSh;
        lastLba = firstLba + Cache->BlocksPerFrame;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, firstLba);
        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, lastLba);
        block_array = Cache->FrameList[frame].Frame;

        if(!block_array) {
            UDFPrint(("Hmm...\n"));
            BrutePoint();
            return STATUS_DRIVER_INTERNAL_ERROR;
        }
        WCacheFlushBlocksRAM(Cache, Context, block_array, List, firstPos, lastPos, FALSE);

        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), firstLba, Cache->BlocksPerFrame);
    }

    return STATUS_SUCCESS;
} // end WCacheFlushAllRAM()

/*
  WCachePreReadPacket__() reads & caches the whole packet containing
  requested LBA. This routine just caches data, it doesn't copy anything
  to user buffer.
  In general we have no user buffer here... ;)
  Public routine
*/
OSSTATUS
WCachePreReadPacket__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t Lba              // LBA to cache together with whole packet
    )
{
    ULONG frame;
    OSSTATUS status = STATUS_SUCCESS;
    PW_CACHE_ENTRY block_array;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    PCHAR addr;
    SIZE_T _ReadBytes;
    ULONG PS = Cache->PacketSize; // (in blocks)
    ULONG BCount = PS;
    ULONG i, n, err_count;
    BOOLEAN sector_added = FALSE;
    ULONG block_type;
    BOOLEAN zero = FALSE;//TRUE;
/*
    ULONG first_zero=0, last_zero=0;
    BOOLEAN count_first_zero = TRUE;
*/

    Lba &= ~(PS-1);
    frame = Lba >> Cache->BlocksPerFrameSh;
    i = Lba - (frame << Cache->BlocksPerFrameSh);

    // assume successful operation
    block_array = Cache->FrameList[frame].Frame;
    if(!block_array) {
        ASSERT(Cache->FrameCount < Cache->MaxFrames);
        block_array = WCacheInitFrame(Cache, Context, frame);
        if(!block_array)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    // skip cached extent (if any)
    n=0;
    while((n < BCount) &&
          (n < Cache->BlocksPerFrame)) {

        addr = (PCHAR)WCacheSectorAddr(block_array, i+n);
        block_type = Cache->CheckUsedProc(Context, Lba+n);
        if(/*WCacheGetBadFlag(block_array,i+n)*/
           block_type & WCACHE_BLOCK_BAD) {
            // bad packet. no pre-read
            return STATUS_DEVICE_DATA_ERROR;
        }
        if(!(block_type & WCACHE_BLOCK_ZERO)) {
            zero = FALSE;
            //count_first_zero = FALSE;
            //last_zero = 0;
            if(!addr) {
                // sector is not cached, stop search
                break;
            }
        } else {
/*
            if(count_first_zero) {
                first_zero++;
            }
            last_zero++;
*/
        }
        n++;
    }
    // do nothing if all sectors are already cached
    if(n < BCount) {

        // read whole packet
        if(!zero) {
            status = Cache->ReadProc(Context, Cache->tmp_buff_r, PS<<BSh, Lba, &_ReadBytes, PH_TMP_BUFFER);
            if(!OS_SUCCESS(status)) {
                status = WCacheRaiseIoError(Cache, Context, status, Lba, PS, Cache->tmp_buff_r, WCACHE_R_OP, NULL);
            }
        } else {
            status = STATUS_SUCCESS;
            //RtlZeroMemory(Cache->tmp_buff_r, PS<<BSh);
            _ReadBytes = PS<<BSh;
        }
        if(OS_SUCCESS(status)) {
            // and now we'll copy them to cache
            for(n=0; n<BCount; n++, i++) {
                if(WCacheSectorAddr(block_array,i)) {
                    continue;
                }
                addr = block_array[i].Sector = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
                if(!addr) {
                    BrutePoint();
                    break;
                }
                sector_added = TRUE;
                if(!zero) {
                    DbgCopyMemory(addr, Cache->tmp_buff_r+(n<<BSh), BS);
                } else {
                    RtlZeroMemory(addr, BS);
                }
                Cache->FrameList[frame].BlockCount++;
            }
        } else {
            // read sectors one by one and copy them to cache
            // unreadable sectors will be treated as zero-filled
            err_count = 0;
            for(n=0; n<BCount; n++, i++) {
                if(WCacheSectorAddr(block_array,i)) {
                    continue;
                }
                addr = block_array[i].Sector = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
                if(!addr) {
                    BrutePoint();
                    break;
                }
                sector_added = TRUE;
                status = Cache->ReadProc(Context, Cache->tmp_buff_r, BS, Lba+n, &_ReadBytes, PH_TMP_BUFFER);
                if(!OS_SUCCESS(status)) {
                    status = WCacheRaiseIoError(Cache, Context, status, Lba+n, 1, Cache->tmp_buff_r, WCACHE_R_OP, NULL);
                    if(!OS_SUCCESS(status)) {
                        err_count++;
                    }
                }
                if(!zero && OS_SUCCESS(status)) {
                    DbgCopyMemory(addr, Cache->tmp_buff_r, BS);
                } else
                if(Cache->RememberBB) {
                    RtlZeroMemory(addr, BS);
                    /*
                    if(!OS_SUCCESS(status)) {
                        WCacheSetBadFlag(block_array,i);
                    }
                    */
                }
                Cache->FrameList[frame].BlockCount++;
                if(err_count >= 2) {
                    break;
                }
            }
//            _ReadBytes = n<<BSh;
        }
    }

    // we know the number of unread sectors if an error occured
    // so we can need to update BlockCount
    // return number of read bytes
    if(sector_added)
        WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, n);

    return status;
} // end WCachePreReadPacket__()

/*
  WCacheReadBlocks__() reads data from cache or
  read it form media and store in cache.
  Public routine
 */
OSSTATUS
WCacheReadBlocks__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN PCHAR Buffer,          // user-supplied buffer for read blocks
    IN lba_t Lba,             // LBA to start read from
    IN ULONG BCount,          // number of blocks to be read
    OUT PSIZE_T ReadBytes,     // user-supplied pointer to ULONG that will
                              //   recieve number of actually read bytes
    IN BOOLEAN CachedOnly     // specifies that cache is already locked
    )
{
    ULONG frame;
    ULONG i, saved_i, saved_BC = BCount, n;
    OSSTATUS status = STATUS_SUCCESS;
    PW_CACHE_ENTRY block_array;
    ULONG BSh = Cache->BlockSizeSh;
    SIZE_T BS = Cache->BlockSize;
    PCHAR addr;
    ULONG to_read, saved_to_read;
//    PCHAR saved_buff = Buffer;
    SIZE_T _ReadBytes;
    ULONG PS = Cache->PacketSize;
    ULONG MaxR = Cache->MaxBytesToRead;
    ULONG PacketMask = PS-1; // here we assume that Packet Size value is 2^n
    ULONG d;
    ULONG block_type;

    WcPrint(("WC:R %x (%x)\n", Lba, BCount));

    (*ReadBytes) = 0;
    // check if we try to read too much data
    if(BCount >= Cache->MaxBlocks) {
        i = 0;
        if(CachedOnly) {
            status = STATUS_INVALID_PARAMETER;
            goto EO_WCache_R2;
        }
        while(TRUE) {
            status = WCacheReadBlocks__(Cache, Context, Buffer + (i<<BSh), Lba, PS, &_ReadBytes, FALSE);
            (*ReadBytes) += _ReadBytes;
            if(!OS_SUCCESS(status) || (BCount <= PS)) break;
            BCount -= PS;
            Lba += PS;
            i += PS;
        }
        return status;
    }
    // check if we try to access beyond cached area
    if((Lba < Cache->FirstLba) ||
       (Lba + BCount - 1 > Cache->LastLba)) {
        status = Cache->ReadProc(Context, Buffer, BCount, Lba, ReadBytes, 0);
        if(!OS_SUCCESS(status)) {
            status = WCacheRaiseIoError(Cache, Context, status, Lba, BCount, Buffer, WCACHE_R_OP, NULL);
        }
        return status;
    }
    if(!CachedOnly) {
        ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);
    }

    frame = Lba >> Cache->BlocksPerFrameSh;
    i = Lba - (frame << Cache->BlocksPerFrameSh);

    if(Cache->CacheWholePacket && (BCount < PS)) {
        if(!CachedOnly &&
           !OS_SUCCESS(status = WCacheCheckLimits(Cache, Context, Lba & ~(PS-1), PS*2)) ) {
            ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
            return status;
        }
    } else {
        if(!CachedOnly &&
           !OS_SUCCESS(status = WCacheCheckLimits(Cache, Context, Lba, BCount))) {
            ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
            return status;
        }
    }
    if(!CachedOnly) {
        // convert to shared
//        ExConvertExclusiveToSharedLite(&(Cache->WCacheLock));
    }

    // pre-read packet. It is very useful for
    // highly fragmented files
    if(Cache->CacheWholePacket && (BCount < PS)) {
//        status = WCacheReadBlocks__(Cache, Context, Cache->tmp_buff_r, Lba & (~PacketMask), PS, &_ReadBytes, TRUE);
        // we should not perform IO if user requested CachedOnly data
        if(!CachedOnly) {
            status = WCachePreReadPacket__(Cache, Context, Lba);
        }
        status = STATUS_SUCCESS;
    }

    // assume successful operation
    block_array = Cache->FrameList[frame].Frame;
    if(!block_array) {
        ASSERT(!CachedOnly);
        ASSERT(Cache->FrameCount < Cache->MaxFrames);
        block_array = WCacheInitFrame(Cache, Context, frame);
        if(!block_array) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto EO_WCache_R;
        }
    }

    Cache->FrameList[frame].AccessCount++;
    while(BCount) {
        if(i >= Cache->BlocksPerFrame) {
            frame++;
            block_array = Cache->FrameList[frame].Frame;
            i -= Cache->BlocksPerFrame;
        }
        if(!block_array) {
            ASSERT(Cache->FrameCount < Cache->MaxFrames);
            block_array = WCacheInitFrame(Cache, Context, frame);
            if(!block_array) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto EO_WCache_R;
            }
        }
        // 'read' cached extent (if any)
        // it is just copying
        while(BCount &&
              (i < Cache->BlocksPerFrame) &&
              (addr = (PCHAR)WCacheSectorAddr(block_array, i)) ) {
            block_type = Cache->CheckUsedProc(Context, Lba+saved_BC-BCount);
            if(block_type & WCACHE_BLOCK_BAD) {
            //if(WCacheGetBadFlag(block_array,i)) {
                status = STATUS_DEVICE_DATA_ERROR;
                goto EO_WCache_R;
            }
            DbgCopyMemory(Buffer, addr, BS);
            Buffer += BS;
            *ReadBytes += BS;
            i++;
            BCount--;
        }
        // read non-cached packet-size-aligned extent (if any)
        // now we'll calculate total length & decide if it has enough size
        if(!((d = Lba+saved_BC-BCount) & PacketMask) && d ) {
            n = 0;
            while(BCount &&
                  (i < Cache->BlocksPerFrame) &&
                  (!WCacheSectorAddr(block_array, i)) ) {
                 n++;
                 BCount--;
            }
            BCount += n;
            n &= ~PacketMask;
            if(n>PS) {
                if(!OS_SUCCESS(status = Cache->ReadProc(Context, Buffer, BS*n, Lba+saved_BC-BCount, &_ReadBytes, 0))) {
                    status = WCacheRaiseIoError(Cache, Context, status, Lba+saved_BC-BCount, n, Buffer, WCACHE_R_OP, NULL);
                    if(!OS_SUCCESS(status)) {
                        goto EO_WCache_R;
                    }
                }
//                WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, saved_BC - BCount);
                BCount -= n;
                Lba += saved_BC - BCount;
                saved_BC = BCount;
                i += n;
                Buffer += BS*n;
                *ReadBytes += BS*n;
            }
//        } else {
//            UDFPrint(("Unaligned\n"));
        }
        // read non-cached extent (if any)
        // firstable, we'll get total number of sectors to read
        to_read = 0;
        saved_i = i;
        d = BCount;
        while(d &&
              (i < Cache->BlocksPerFrame) &&
              (!WCacheSectorAddr(block_array, i)) ) {
            i++;
            to_read += BS;
            d--;
        }
        // read some not cached sectors
        if(to_read) {
            i = saved_i;
            saved_to_read = to_read;
            d = BCount - d;
            // split request if necessary
            if(saved_to_read > MaxR) {
                WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, saved_BC - BCount);
                n = MaxR >> BSh;
                do {
                    status = Cache->ReadProc(Context, Buffer, MaxR, i + (frame << Cache->BlocksPerFrameSh), &_ReadBytes, 0);
                    *ReadBytes += _ReadBytes;
                    if(!OS_SUCCESS(status)) {
                        _ReadBytes &= ~(BS-1);
                        BCount -= _ReadBytes >> BSh;
                        saved_to_read -= _ReadBytes;
                        Buffer += _ReadBytes;
                        saved_BC = BCount;
                        goto store_read_data_1;
                    }
                    Buffer += MaxR;
                    saved_to_read -= MaxR;
                    i += n;
                    BCount -= n;
                    d -= n;
                } while(saved_to_read > MaxR);
                saved_BC = BCount;
            }
            if(saved_to_read) {
                status = Cache->ReadProc(Context, Buffer, saved_to_read, i + (frame << Cache->BlocksPerFrameSh), &_ReadBytes, 0);
                *ReadBytes += _ReadBytes;
                if(!OS_SUCCESS(status)) {
                    _ReadBytes &= ~(BS-1);
                    BCount -= _ReadBytes >> BSh;
                    saved_to_read -= _ReadBytes;
                    Buffer += _ReadBytes;
                    goto store_read_data_1;
                }
                Buffer += saved_to_read;
                saved_to_read = 0;
                BCount -= d;
            }

store_read_data_1:
            // and now we'll copy them to cache

            //
            Buffer -= (to_read - saved_to_read);
            i = saved_i;
            while(to_read - saved_to_read) {
                block_array[i].Sector = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
                if(!block_array[i].Sector) {
                    BCount += to_read >> BSh;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto EO_WCache_R;
                }
                DbgCopyMemory(block_array[i].Sector, Buffer, BS);
                Cache->FrameList[frame].BlockCount++;
                i++;
                Buffer += BS;
                to_read -= BS;
            }
            if(!OS_SUCCESS(status))
                goto EO_WCache_R;
            to_read = 0;
        }
    }

EO_WCache_R:

    // we know the number of unread sectors if an error occured
    // so we can need to update BlockCount
    // return number of read bytes
    WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, saved_BC - BCount);
//    Cache->FrameList[frame].BlockCount -= BCount;
EO_WCache_R2:
    if(!CachedOnly) {
        ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    }

    return status;
} // end WCacheReadBlocks__()

/*
  WCacheWriteBlocks__() writes data to cache.
  Data is written directly to media if:
  1) requested block is Packet-aligned
  2) requested Lba(s) lays beyond cached area
  Public routine
 */
OSSTATUS
WCacheWriteBlocks__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN PCHAR Buffer,          // user-supplied buffer containing data to be written
    IN lba_t Lba,             // LBA to start write from
    IN ULONG BCount,          // number of blocks to be written
    OUT PSIZE_T WrittenBytes,  // user-supplied pointer to ULONG that will
                              //   recieve number of actually written bytes
    IN BOOLEAN CachedOnly     // specifies that cache is already locked
    )
{
    ULONG frame;
    ULONG i, saved_BC = BCount, n, d;
    OSSTATUS status = STATUS_SUCCESS;
    PW_CACHE_ENTRY block_array;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    PCHAR addr;
//    PCHAR saved_buff = Buffer;
    SIZE_T _WrittenBytes;
    ULONG PS = Cache->PacketSize;
    ULONG PacketMask = PS-1; // here we assume that Packet Size value is 2^n
    ULONG block_type;
//    BOOLEAN Aligned = FALSE;

    BOOLEAN WriteThrough = FALSE;
    lba_t   WTh_Lba;
    ULONG   WTh_BCount;

    WcPrint(("WC:W %x (%x)\n", Lba, BCount));

    *WrittenBytes = 0;
//    UDFPrint(("BCount:%x\n",BCount));
    // check if we try to read too much data
    if(BCount >= Cache->MaxBlocks) {
        i = 0;
        if(CachedOnly) {
            status = STATUS_INVALID_PARAMETER;
            goto EO_WCache_W2;
        }
        while(TRUE) {
//            UDFPrint(("  BCount:%x\n",BCount));
            status = WCacheWriteBlocks__(Cache, Context, Buffer + (i<<BSh), Lba, min(PS,BCount), &_WrittenBytes, FALSE);
            (*WrittenBytes) += _WrittenBytes;
            BCount -= PS;
            Lba += PS;
            i += PS;
            if(!OS_SUCCESS(status) || (BCount < PS))
                return status;
        }
    }
    // check if we try to access beyond cached area
    if((Lba < Cache->FirstLba) ||
       (Lba + BCount - 1 > Cache->LastLba)) {
        return STATUS_INVALID_PARAMETER;
    }
    if(!CachedOnly) {
        ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);
    }

    frame = Lba >> Cache->BlocksPerFrameSh;
    i = Lba - (frame << Cache->BlocksPerFrameSh);

    if(!CachedOnly &&
       !OS_SUCCESS(status = WCacheCheckLimits(Cache, Context, Lba, BCount))) {
        ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
        return status;
    }

    // assume successful operation
    block_array = Cache->FrameList[frame].Frame;
    if(!block_array) {

        if(BCount && !(BCount & (PS-1)) && !(Lba & (PS-1)) &&
           (Cache->Mode != WCACHE_MODE_R) &&
           (i+BCount <= Cache->BlocksPerFrame) &&
            !Cache->NoWriteThrough) {
            status = Cache->WriteProc(Context, Buffer, BCount<<BSh, Lba, WrittenBytes, 0);
            if(!OS_SUCCESS(status)) {
                status = WCacheRaiseIoError(Cache, Context, status, Lba, BCount, Buffer, WCACHE_W_OP, NULL);
            }
            goto EO_WCache_W2;
        }

        ASSERT(!CachedOnly);
        ASSERT(Cache->FrameCount < Cache->MaxFrames);
        block_array = WCacheInitFrame(Cache, Context, frame);
        if(!block_array) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto EO_WCache_W;
        }
    }

    if(Cache->Mode == WCACHE_MODE_RAM &&
       BCount &&
//       !(Lba & (PS-1)) &&
       (!(BCount & (PS-1)) || (BCount > PS)) ) {
        WriteThrough = TRUE;
        WTh_Lba = Lba;
        WTh_BCount = BCount;
    } else
    if(Cache->Mode == WCACHE_MODE_RAM &&
       ((Lba & ~PacketMask) != ((Lba+BCount-1) & ~PacketMask))
      ) {
        WriteThrough = TRUE;
        WTh_Lba = Lba & ~PacketMask;
        WTh_BCount = PS;
    }

    Cache->FrameList[frame].UpdateCount++;
//    UDFPrint(("    BCount:%x\n",BCount));
    while(BCount) {
        if(i >= Cache->BlocksPerFrame) {
            frame++;
            block_array = Cache->FrameList[frame].Frame;
            i -= Cache->BlocksPerFrame;
        }
        if(!block_array) {
            ASSERT(Cache->FrameCount < Cache->MaxFrames);
            block_array = WCacheInitFrame(Cache, Context, frame);
            if(!block_array) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto EO_WCache_W;
            }
        }
        // 'write' cached extent (if any)
        // it is just copying
        while(BCount &&
              (i < Cache->BlocksPerFrame) &&
              (addr = (PCHAR)WCacheSectorAddr(block_array, i)) ) {
//            UDFPrint(("addr:%x:Buffer:%x:BS:%x:BCount:%x\n",addr, Buffer, BS, BCount));
            block_type = Cache->CheckUsedProc(Context, Lba+saved_BC-BCount);
            if(Cache->NoWriteBB &&
               /*WCacheGetBadFlag(block_array,i)*/
               (block_type & WCACHE_BLOCK_BAD)) {
                // bad packet. no cached write
                status = STATUS_DEVICE_DATA_ERROR;
                goto EO_WCache_W;
            }
            DbgCopyMemory(addr, Buffer, BS);
            WCacheSetModFlag(block_array, i);
            Buffer += BS;
            *WrittenBytes += BS;
            i++;
            BCount--;
        }
        // write non-cached not-aligned extent (if any) till aligned one
        while(BCount &&
              (i & PacketMask) &&
              (Cache->Mode != WCACHE_MODE_R) &&
              (i < Cache->BlocksPerFrame) &&
              (!WCacheSectorAddr(block_array, i)) ) {
            block_array[i].Sector = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
            if(!block_array[i].Sector) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto EO_WCache_W;
            }
//            UDFPrint(("addr:%x:Buffer:%x:BS:%x:BCount:%x\n",block_array[i].Sector, Buffer, BS, BCount));
            DbgCopyMemory(block_array[i].Sector, Buffer, BS);
            WCacheSetModFlag(block_array, i);
            i++;
            Buffer += BS;
            *WrittenBytes += BS;
            BCount--;
            Cache->FrameList[frame].BlockCount ++;
        }
        // write non-cached packet-size-aligned extent (if any)
        // now we'll calculate total length & decide if has enough size
        if(!Cache->NoWriteThrough
                     &&
           ( !(i & PacketMask) ||
             ((Cache->Mode == WCACHE_MODE_R) && (BCount >= PS)) )) {
            n = 0;
            while(BCount &&
                  (i < Cache->BlocksPerFrame) &&
                  (!WCacheSectorAddr(block_array, i)) ) {
                 n++;
                 BCount--;
            }
            BCount += n;
            n &= ~PacketMask;
//                if(!OS_SUCCESS(status = Cache->WriteProcAsync(Context, Buffer, BS*n, Lba+saved_BC-BCount, &_WrittenBytes, FALSE)))
            if(n) {
                // add previously written data to list
                d = saved_BC - BCount;
                WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, d);
                WCacheInsertRangeToList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, d);
                Lba += d;
                saved_BC = BCount;

                while(n) {
                    if(Cache->Mode == WCACHE_MODE_R)
                        Cache->UpdateRelocProc(Context, Lba, NULL, PS);
                    if(!OS_SUCCESS(status = Cache->WriteProc(Context, Buffer, PS<<BSh, Lba, &_WrittenBytes, 0))) {
                        status = WCacheRaiseIoError(Cache, Context, status, Lba, PS, Buffer, WCACHE_W_OP, NULL);
                        if(!OS_SUCCESS(status)) {
                            goto EO_WCache_W;
                        }
                    }
                    BCount -= PS;
                    Lba += PS;
                    saved_BC = BCount;
                    i += PS;
                    Buffer += PS<<BSh;
                    *WrittenBytes += PS<<BSh;
                    n-=PS;
                }
            }
        }
        // write non-cached not-aligned extent (if any)
        while(BCount &&
              (i < Cache->BlocksPerFrame) &&
              (!WCacheSectorAddr(block_array, i)) ) {
            block_array[i].Sector = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
            if(!block_array[i].Sector) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto EO_WCache_W;
            }
//            UDFPrint(("addr:%x:Buffer:%x:BS:%x:BCount:%x\n",block_array[i].Sector, Buffer, BS, BCount));
            DbgCopyMemory(block_array[i].Sector, Buffer, BS);
            WCacheSetModFlag(block_array, i);
            i++;
            Buffer += BS;
            *WrittenBytes += BS;
            BCount--;
            Cache->FrameList[frame].BlockCount ++;
        }
    }

EO_WCache_W:

    // we know the number of unread sectors if an error occured
    // so we can need to update BlockCount
    // return number of read bytes
    WCacheInsertRangeToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba, saved_BC - BCount);
    WCacheInsertRangeToList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, saved_BC - BCount);

    if(WriteThrough && !BCount) {
        ULONG d;
//        lba_t lastLba;
        ULONG firstPos;
        ULONG lastPos;

        BCount = WTh_BCount;
        Lba = WTh_Lba;
        while(BCount) {
            frame = Lba >> Cache->BlocksPerFrameSh;
//            firstLba = frame << Cache->BlocksPerFrameSh;
            firstPos = WCacheGetSortedListIndex(Cache->BlockCount, Cache->CachedBlocksList, Lba);
            d = min(Lba+BCount, (frame+1) << Cache->BlocksPerFrameSh) - Lba;
            lastPos = WCacheGetSortedListIndex(Cache->BlockCount, Cache->CachedBlocksList, Lba+d);
            block_array = Cache->FrameList[frame].Frame;
            if(!block_array) {
                ASSERT(FALSE);
                BCount -= d;
                Lba += d;
                continue;
            }
            status = WCacheFlushBlocksRAM(Cache, Context, block_array, Cache->CachedBlocksList, firstPos, lastPos, FALSE);
            WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, d);
            BCount -= d;
            Lba += d;
        }
    }

EO_WCache_W2:

    if(!CachedOnly) {
        ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    }
    return status;
} // end WCacheWriteBlocks__()

/*
  WCacheFlushAll__() copies all data stored in cache to media.
  Flushed blocks are kept in cache.
  Public routine
 */
VOID
WCacheFlushAll__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context)         // user-supplied context for IO callbacks
{
    if(!(Cache->ReadProc)) return;
    ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);

    switch(Cache->Mode) {
    case WCACHE_MODE_RAM:
        WCacheFlushAllRAM(Cache, Context);
        break;
    case WCACHE_MODE_ROM:
    case WCACHE_MODE_RW:
        WCacheFlushAllRW(Cache, Context);
        break;
    case WCACHE_MODE_R:
        WCachePurgeAllR(Cache, Context);
        break;
    }

    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    return;
} // end WCacheFlushAll__()

/*
  WCachePurgeAll__() copies all data stored in cache to media.
  Flushed blocks are removed cache.
  Public routine
 */
VOID
WCachePurgeAll__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context)         // user-supplied context for IO callbacks
{
    if(!(Cache->ReadProc)) return;
    ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);

    switch(Cache->Mode) {
    case WCACHE_MODE_RAM:
        WCachePurgeAllRAM(Cache, Context);
        break;
    case WCACHE_MODE_ROM:
    case WCACHE_MODE_RW:
        WCachePurgeAllRW(Cache, Context);
        break;
    case WCACHE_MODE_R:
        WCachePurgeAllR(Cache, Context);
        break;
    }

    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    return;
} // end WCachePurgeAll__()
/*
  WCachePurgeAllRW() copies modified blocks from cache to media
  and removes them from cache
  This routine can be used for RAM, RW and ROM media.
  For ROM media blocks are just removed.
  Internal routine
 */
VOID
__fastcall
WCachePurgeAllRW(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context)         // user-supplied context for IO callbacks
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t Lba;
//    ULONG firstPos;
//    ULONG lastPos;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;
    SIZE_T ReadBytes;
    PW_CACHE_ASYNC FirstWContext = NULL;
    PW_CACHE_ASYNC PrevWContext = NULL;
    ULONG chain_count = 0;

    if(!(Cache->ReadProc)) return;

    while(Cache->BlockCount) {
        Lba = List[0] & ~(PSs-1);
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
//        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba);
//        lastPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba+PSs);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            BrutePoint();
            return;
        }

        WCacheUpdatePacket(Cache, Context, &FirstWContext, &PrevWContext, block_array, firstLba,
            Lba, BSh, BS, PS, PSs, &ReadBytes, TRUE, ASYNC_STATE_NONE);

        // free memory
        WCacheFreePacket(Cache, frame, block_array, Lba-firstLba, PSs);

        WCacheRemoveRangeFromList(List, &(Cache->BlockCount), Lba, PSs);
        WCacheRemoveRangeFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba, PSs);
        // check if frame is empty
        if(!(Cache->FrameList[frame].BlockCount)) {
            WCacheRemoveFrame(Cache, Context, frame);
        } else {
            ASSERT(Cache->FrameList[frame].Frame);
        }
        chain_count++;
        if(chain_count >= WCACHE_MAX_CHAIN) {
            WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
            chain_count = 0;
        }
    }
    WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext);
    return;
} // end WCachePurgeAllRW()

/*
  WCacheFlushAllRW() copies modified blocks from cache to media.
  All blocks are not removed from cache.
  This routine can be used for RAM, RW and ROM media.
  Internal routine
 */
VOID
__fastcall
WCacheFlushAllRW(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context)         // user-supplied context for IO callbacks
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedModifiedBlocksList;
    lba_t Lba;
//    ULONG firstPos;
//    ULONG lastPos;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    ULONG BFs = Cache->BlocksPerFrameSh;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;
    SIZE_T ReadBytes;
    PW_CACHE_ASYNC FirstWContext = NULL;
    PW_CACHE_ASYNC PrevWContext = NULL;
    ULONG i;
    ULONG chain_count = 0;

    if(!(Cache->ReadProc)) return;

    // walk through modified blocks
    while(Cache->WriteCount) {
        Lba = List[0] & ~(PSs-1);
        frame = Lba >> BFs;
        firstLba = frame << BFs;
//        firstPos = WCacheGetSortedListIndex(Cache->WriteCount, List, Lba);
//        lastPos = WCacheGetSortedListIndex(Cache->WriteCount, List, Lba+PSs);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            BrutePoint();
            continue;;
        }
        // queue modify request
        WCacheUpdatePacket(Cache, Context, &FirstWContext, &PrevWContext, block_array, firstLba,
            Lba, BSh, BS, PS, PSs, &ReadBytes, TRUE, ASYNC_STATE_NONE);
        // clear MODIFIED flag for queued blocks
        WCacheRemoveRangeFromList(List, &(Cache->WriteCount), Lba, PSs);
        Lba -= firstLba;
        for(i=0; i<PSs; i++) {
            WCacheClrModFlag(block_array, Lba+i);
        }
        chain_count++;
        // check queue size
        if(chain_count >= WCACHE_MAX_CHAIN) {
            WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
            chain_count = 0;
        }
    }
    WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
#ifdef DBG
#if 1
    // check consistency
    List = Cache->CachedBlocksList;
    for(i=0; i<Cache->BlockCount; i++) {
        Lba = List[i] /*& ~(PSs-1)*/;
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            BrutePoint();
        }
        ASSERT(!WCacheGetModFlag(block_array, Lba-firstLba));
    }
#endif // 1
#endif // DBG
    return;
} // end WCacheFlushAllRW()

/*
  WCacheRelease__() frees all allocated memory blocks and
  deletes synchronization resources
  Public routine
 */
VOID
WCacheRelease__(
    IN PW_CACHE Cache         // pointer to the Cache Control structure
    )
{
    ULONG i, j, k;
    PW_CACHE_ENTRY block_array;

    Cache->Tag = 0xDEADCACE;
    if(!(Cache->ReadProc)) return;
//    ASSERT(Cache->Tag == 0xCAC11E00);
    ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);
    for(i=0; i<Cache->FrameCount; i++) {
        j = Cache->CachedFramesList[i];
        block_array = Cache->FrameList[j].Frame;
        if(block_array) {
            for(k=0; k<Cache->BlocksPerFrame; k++) {
                if(WCacheSectorAddr(block_array, k)) {
                    WCacheFreeSector(j, k);
                }
            }
            MyFreePool__(block_array);
        }
    }
    if(Cache->FrameList)
        MyFreePool__(Cache->FrameList);
    if(Cache->CachedBlocksList)
        MyFreePool__(Cache->CachedBlocksList);
    if(Cache->CachedModifiedBlocksList)
        MyFreePool__(Cache->CachedModifiedBlocksList);
    if(Cache->CachedFramesList)
        MyFreePool__(Cache->CachedFramesList);
    if(Cache->tmp_buff_r)
        MyFreePool__(Cache->tmp_buff_r);
    if(Cache->CachedFramesList)
        MyFreePool__(Cache->tmp_buff);
    if(Cache->CachedFramesList)
        MyFreePool__(Cache->reloc_tab);
    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    ExDeleteResourceLite(&(Cache->WCacheLock));
    RtlZeroMemory(Cache, sizeof(W_CACHE));
    return;
} // end WCacheRelease__()

/*
  WCacheIsInitialized__() checks if the pointer supplied points
  to initialized cache structure.
  Public routine
 */
BOOLEAN
WCacheIsInitialized__(
    IN PW_CACHE Cache
    )
{
    return (Cache->ReadProc != NULL);
} // end WCacheIsInitialized__()

OSSTATUS
WCacheFlushBlocksRW(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t _Lba,             // LBA to start flush from
    IN ULONG BCount           // number of blocks to be flushed
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedModifiedBlocksList;
    lba_t Lba;
//    ULONG firstPos;
//    ULONG lastPos;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    ULONG BFs = Cache->BlocksPerFrameSh;
    PW_CACHE_ENTRY block_array;
//    OSSTATUS status;
    SIZE_T ReadBytes;
    PW_CACHE_ASYNC FirstWContext = NULL;
    PW_CACHE_ASYNC PrevWContext = NULL;
    ULONG i;
    ULONG chain_count = 0;
    lba_t lim;

    if(!(Cache->ReadProc)) return STATUS_INVALID_PARAMETER;

    // walk through modified blocks
    lim = (_Lba+BCount+PSs-1) & ~(PSs-1);
    for(Lba = _Lba & ~(PSs-1);Lba < lim ; Lba += PSs) {
        frame = Lba >> BFs;
        firstLba = frame << BFs;
//        firstPos = WCacheGetSortedListIndex(Cache->WriteCount, List, Lba);
//        lastPos = WCacheGetSortedListIndex(Cache->WriteCount, List, Lba+PSs);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            // not cached block may be requested for flush
            Lba += (1 << BFs) - PSs;
            continue;
        }
        // queue modify request
        WCacheUpdatePacket(Cache, Context, &FirstWContext, &PrevWContext, block_array, firstLba,
            Lba, BSh, BS, PS, PSs, &ReadBytes, TRUE, ASYNC_STATE_NONE);
        // clear MODIFIED flag for queued blocks
        WCacheRemoveRangeFromList(List, &(Cache->WriteCount), Lba, PSs);
        Lba -= firstLba;
        for(i=0; i<PSs; i++) {
            WCacheClrModFlag(block_array, Lba+i);
        }
        Lba += firstLba;
        chain_count++;
        // check queue size
        if(chain_count >= WCACHE_MAX_CHAIN) {
            WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
            chain_count = 0;
        }
    }
    WCacheUpdatePacketComplete(Cache, Context, &FirstWContext, &PrevWContext, FALSE);
/*
    if(Cache->Mode != WCACHE_MODE_RAM)
        return STATUS_SUCCESS;
*/

    return STATUS_SUCCESS;
} // end WCacheFlushBlocksRW()

/*
  WCacheFlushBlocks__() copies specified blocks stored in cache to media.
  Flushed blocks are kept in cache.
  Public routine
 */
OSSTATUS
WCacheFlushBlocks__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t Lba,             // LBA to start flush from
    IN ULONG BCount           // number of blocks to be flushed
    )
{
    OSSTATUS status;

    if(!(Cache->ReadProc)) return STATUS_INVALID_PARAMETER;
    ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);

    // check if we try to access beyond cached area
    if((Lba < Cache->FirstLba) ||
       (Lba+BCount-1 > Cache->LastLba)) {
        UDFPrint(("LBA %#x (%x) is beyond cacheable area\n", Lba, BCount));
        BrutePoint();
        status = STATUS_INVALID_PARAMETER;
        goto EO_WCache_F;
    }

    switch(Cache->Mode) {
    case WCACHE_MODE_RAM:
//        WCacheFlushBlocksRW(Cache, Context);
//        break;
    case WCACHE_MODE_ROM:
    case WCACHE_MODE_RW:
        status = WCacheFlushBlocksRW(Cache, Context, Lba, BCount);
        break;
    case WCACHE_MODE_R:
        status = STATUS_SUCCESS;
        break;
    }
EO_WCache_F:
    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    return status;
} // end WCacheFlushBlocks__()

/*
  WCacheDirect__() returns pointer to memory block where
  requested block is stored in.
  If no #CachedOnly flag specified this routine locks cache,
  otherwise it assumes that cache is already locked by previous call
  to WCacheStartDirect__().
  Cache can be unlocked by WCacheEODirect__().
  Using this routine caller can access cached block directly in memory
  without Read_to_Tmp and Modify/Write steps.
  Public routine
 */
OSSTATUS
WCacheDirect__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t Lba,             // LBA of block to get pointer to
    IN BOOLEAN Modified,      // indicates that block will be modified
    OUT PCHAR* CachedBlock,   // address for pointer to cached block to be stored in
    IN BOOLEAN CachedOnly     // specifies that cache is already locked
    )
{
    ULONG frame;
    ULONG i;
    OSSTATUS status = STATUS_SUCCESS;
    PW_CACHE_ENTRY block_array;
    ULONG BS = Cache->BlockSize;
    PCHAR addr;
    SIZE_T _ReadBytes;
    ULONG block_type;

    WcPrint(("WC:%sD %x (1)\n", Modified ? "W" : "R", Lba));

    // lock cache if nececcary
    if(!CachedOnly) {
        ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);
    }
    // check if we try to access beyond cached area
    if((Lba < Cache->FirstLba) ||
       (Lba > Cache->LastLba)) {
        UDFPrint(("LBA %#x is beyond cacheable area\n", Lba));
        BrutePoint();
        status = STATUS_INVALID_PARAMETER;
        goto EO_WCache_D;
    }

    frame = Lba >> Cache->BlocksPerFrameSh;
    i = Lba - (frame << Cache->BlocksPerFrameSh);
    // check if we have enough space to store requested block
    if(!CachedOnly &&
       !OS_SUCCESS(status = WCacheCheckLimits(Cache, Context, Lba, 1))) {
        BrutePoint();
        goto EO_WCache_D;
    }

    // small updates are more important
    block_array = Cache->FrameList[frame].Frame;
    if(Modified) {
        Cache->FrameList[frame].UpdateCount+=8;
    } else {
        Cache->FrameList[frame].AccessCount+=8;
    }
    if(!block_array) {
        ASSERT(Cache->FrameCount < Cache->MaxFrames);
        block_array = WCacheInitFrame(Cache, Context, frame);
        if(!block_array) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto EO_WCache_D;
        }
    }
    // check if requested block is already cached
    if( !(addr = (PCHAR)WCacheSectorAddr(block_array, i)) ) {
        // block is not cached
        // allocate memory and read block from media
        // do not set block_array[i].Sector here, because if media access fails and recursive access to cache
        // comes, this block should not be marked as 'cached'
        addr = (PCHAR)DbgAllocatePoolWithTag(CACHED_BLOCK_MEMORY_TYPE, BS, MEM_WCBUF_TAG);
        if(!addr) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto EO_WCache_D;
        }
        block_type = Cache->CheckUsedProc(Context, Lba);
        if(block_type == WCACHE_BLOCK_USED) {
            status = Cache->ReadProc(Context, addr, BS, Lba, &_ReadBytes, PH_TMP_BUFFER);
            if(Cache->RememberBB) {
                if(!OS_SUCCESS(status)) {
                    RtlZeroMemory(addr, BS);
                    //WCacheSetBadFlag(block_array,i);
                }
            }
        } else {
            if(block_type & WCACHE_BLOCK_BAD) {
                DbgFreePool(addr);
                addr = NULL;
                status = STATUS_DEVICE_DATA_ERROR;
                goto EO_WCache_D;
            }
            if(!(block_type & WCACHE_BLOCK_ZERO)) {
                BrutePoint();
            }
            status = STATUS_SUCCESS;
            RtlZeroMemory(addr, BS);
        }
        // now add pointer to buffer to common storage
        block_array[i].Sector = addr;
        WCacheInsertItemToList(Cache->CachedBlocksList, &(Cache->BlockCount), Lba);
        if(Modified) {
            WCacheInsertItemToList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
            WCacheSetModFlag(block_array, i);
        }
        Cache->FrameList[frame].BlockCount ++;
    } else {
        // block is not cached
        // just return pointer
        block_type = Cache->CheckUsedProc(Context, Lba);
        if(block_type & WCACHE_BLOCK_BAD) {
        //if(WCacheGetBadFlag(block_array,i)) {
            // bad packet. no pre-read
            status = STATUS_DEVICE_DATA_ERROR;
            goto EO_WCache_D;
        }
#ifndef UDF_CHECK_UTIL
        ASSERT(block_type & WCACHE_BLOCK_USED);
#else
        if(!(block_type & WCACHE_BLOCK_USED)) {
            UDFPrint(("LBA %#x is not marked as used\n", Lba));
        }
#endif
        if(Modified &&
           !WCacheGetModFlag(block_array, i)) {
            WCacheInsertItemToList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
            WCacheSetModFlag(block_array, i);
        }
    }
    (*CachedBlock) = addr;

EO_WCache_D:

    return status;
} // end WCacheDirect__()

/*
  WCacheEODirect__() must be used to unlock cache after calls to
  to WCacheStartDirect__().
  Public routine
 */
OSSTATUS
WCacheEODirect__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context          // user-supplied context for IO callbacks
    )
{
    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
    return STATUS_SUCCESS;
} // end WCacheEODirect__()

/*
  WCacheStartDirect__() locks cache for exclusive use.
  Using this routine caller can access cached block directly in memory
  without Read_to_Tmp and Modify/Write steps.
  See also WCacheDirect__()
  Cache can be unlocked by WCacheEODirect__().
  Public routine
 */
OSSTATUS
WCacheStartDirect__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN BOOLEAN Exclusive      // lock cache for exclusive use,
                              //   currently must be TRUE.
    )
{
    if(Exclusive) {
        ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);
    } else {
        BrutePoint();
        ExAcquireResourceSharedLite(&(Cache->WCacheLock), TRUE);
    }
    return STATUS_SUCCESS;
} // end WCacheStartDirect__()

/*
  WCacheIsCached__() checks if requested blocks are immediately available.
  Cache must be previously locked for exclusive use with WCacheStartDirect__().
  Using this routine caller can access cached block directly in memory
  without Read_to_Tmp and Modify/Write steps.
  See also WCacheDirect__().
  Cache can be unlocked by WCacheEODirect__().
  Public routine
 */
BOOLEAN
WCacheIsCached__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN lba_t Lba,             // LBA to start check from
    IN ULONG BCount           // number of blocks to be checked
    )
{
    ULONG frame;
    ULONG i;
    PW_CACHE_ENTRY block_array;

    // check if we try to access beyond cached area
    if((Lba < Cache->FirstLba) ||
       (Lba + BCount - 1 > Cache->LastLba)) {
        return FALSE;
    }

    frame = Lba >> Cache->BlocksPerFrameSh;
    i = Lba - (frame << Cache->BlocksPerFrameSh);

    block_array = Cache->FrameList[frame].Frame;
    if(!block_array) {
        return FALSE;
    }

    while(BCount) {
        if(i >= Cache->BlocksPerFrame) {
            frame++;
            block_array = Cache->FrameList[frame].Frame;
            i -= Cache->BlocksPerFrame;
        }
        if(!block_array) {
            return FALSE;
        }
        // 'read' cached extent (if any)
        while(BCount &&
              (i < Cache->BlocksPerFrame) &&
              WCacheSectorAddr(block_array, i) &&
              /*!WCacheGetBadFlag(block_array, i)*/
              /*!(Cache->CheckUsedProc(Context, Lba) & WCACHE_BLOCK_BAD)*/
              TRUE ) {
            i++;
            BCount--;
            Lba++;
        }
        if(BCount &&
              (i < Cache->BlocksPerFrame) /*&&
              (!WCacheSectorAddr(block_array, i))*/ ) {
            return FALSE;
        }
    }
    return TRUE;
} // end WCacheIsCached__()

/*
  WCacheCheckLimitsR() implements automatic flush and purge of
  unused blocks to keep enough free cache entries for newly
  read/written blocks for WORM media.
  See also WCacheCheckLimits()
  Internal routine
 */
OSSTATUS
__fastcall
WCacheCheckLimitsR(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context,         // user-supplied context for IO callbacks
    IN lba_t ReqLba,          // first LBA to access/cache
    IN ULONG BCount           // number of Blocks to access/cache
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t Lba;
    PCHAR tmp_buff = Cache->tmp_buff;
    ULONG firstPos;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
    ULONG PSs = Cache->PacketSize;
    ULONG i;
    PW_CACHE_ENTRY block_array;
    BOOLEAN mod;
    OSSTATUS status;
    SIZE_T ReadBytes;
    ULONG MaxReloc = Cache->PacketSize;
    PULONG reloc_tab = Cache->reloc_tab;

    // check if we try to read too much data
    if(BCount > Cache->MaxBlocks) {
        return STATUS_INVALID_PARAMETER;
    }

    // remove(flush) packets from entire frame(s)
    while( ((Cache->BlockCount + WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba) +
             BCount - WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba+BCount)) > Cache->MaxBlocks) ||
           (Cache->FrameCount >= Cache->MaxFrames) ) {

WCCL_retry_1:

        Lba = WCacheFindLbaToRelease(Cache);
        if(Lba == WCACHE_INVALID_LBA) {
            ASSERT(!Cache->FrameCount);
            ASSERT(!Cache->BlockCount);
            break;
        }
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        firstPos = WCacheGetSortedListIndex(Cache->BlockCount, List, Lba);
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            return STATUS_DRIVER_INTERNAL_ERROR;
        }
        // check if modified
        mod = WCacheGetModFlag(block_array, Lba - firstLba);
        // read/modify/write
        if(mod && (Cache->CheckUsedProc(Context, Lba) & WCACHE_BLOCK_USED)) {
            if(Cache->WriteCount < MaxReloc) goto WCCL_retry_1;
            firstPos = WCacheGetSortedListIndex(Cache->WriteCount, Cache->CachedModifiedBlocksList, Lba);
            if(!block_array) {
                return STATUS_DRIVER_INTERNAL_ERROR;
            }
            // prepare packet & reloc table
            for(i=0; i<MaxReloc; i++) {
                Lba = Cache->CachedModifiedBlocksList[firstPos];
                frame = Lba >> Cache->BlocksPerFrameSh;
                firstLba = frame << Cache->BlocksPerFrameSh;
                block_array = Cache->FrameList[frame].Frame;
                DbgCopyMemory(tmp_buff + (i << BSh),
                              (PVOID)WCacheSectorAddr(block_array, Lba-firstLba),
                              BS);
                reloc_tab[i] = Lba;
                WCacheRemoveItemFromList(List, &(Cache->BlockCount), Lba);
                WCacheRemoveItemFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
                // mark as non-cached & free pool
                WCacheFreeSector(frame, Lba-firstLba);
                // check if frame is empty
                if(!Cache->FrameList[frame].BlockCount) {
                    WCacheRemoveFrame(Cache, Context, frame);
                }
                if(firstPos >= Cache->WriteCount) firstPos=0;
            }
            // write packet
//            status = Cache->WriteProcAsync(Context, tmp_buff, PS, Lba, &ReadBytes, FALSE);
            Cache->UpdateRelocProc(Context, NULL, reloc_tab, MaxReloc);
            status = Cache->WriteProc(Context, tmp_buff, PS, NULL, &ReadBytes, 0);
            if(!OS_SUCCESS(status)) {
                status = WCacheRaiseIoError(Cache, Context, status, NULL, PSs, tmp_buff, WCACHE_W_OP, NULL);
            }
        } else {

            if((i = Cache->BlockCount - Cache->WriteCount) > MaxReloc) i = MaxReloc;
            // discard blocks
            for(; i; i--) {
                Lba = List[firstPos];
                frame = Lba >> Cache->BlocksPerFrameSh;
                firstLba = frame << Cache->BlocksPerFrameSh;
                block_array = Cache->FrameList[frame].Frame;

                if( (mod = WCacheGetModFlag(block_array, Lba - firstLba)) &&
                    (Cache->CheckUsedProc(Context, Lba) & WCACHE_BLOCK_USED) )
                    continue;
                WCacheRemoveItemFromList(List, &(Cache->BlockCount), Lba);
                if(mod)
                    WCacheRemoveItemFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
                // mark as non-cached & free pool
                WCacheFreeSector(frame, Lba-firstLba);
                // check if frame is empty
                if(!Cache->FrameList[frame].BlockCount) {
                    WCacheRemoveFrame(Cache, Context, frame);
                }
                if(firstPos >= Cache->WriteCount) firstPos=0;
            }
        }
    }
    return STATUS_SUCCESS;
} // end WCacheCheckLimitsR()

/*
  WCachePurgeAllR() copies modified blocks from cache to media
  and removes them from cache
  This routine can be used for R media only.
  Internal routine
 */
VOID
__fastcall
WCachePurgeAllR(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN PVOID Context)         // user-supplied context for IO callbacks
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t Lba;
    PCHAR tmp_buff = Cache->tmp_buff;
    ULONG BSh = Cache->BlockSizeSh;
    ULONG BS = Cache->BlockSize;
//    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
//    ULONG PSs = Cache->PacketSize;
    PW_CACHE_ENTRY block_array;
    BOOLEAN mod;
    OSSTATUS status;
    SIZE_T ReadBytes;
    ULONG MaxReloc = Cache->PacketSize;
    PULONG reloc_tab = Cache->reloc_tab;
    ULONG RelocCount = 0;
    BOOLEAN IncompletePacket;
    ULONG i=0;
    ULONG PacketTail;

    while(Cache->WriteCount < Cache->BlockCount) {

        Lba = List[i];
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            BrutePoint();
            return;
        }
        // check if modified
        mod = WCacheGetModFlag(block_array, Lba - firstLba);
        // just discard
        if(!mod || !(Cache->CheckUsedProc(Context, Lba) & WCACHE_BLOCK_USED)) {
            // mark as non-cached & free pool
            if(WCacheSectorAddr(block_array,Lba-firstLba)) {
                WCacheRemoveItemFromList(List, &(Cache->BlockCount), Lba);
                if(mod)
                    WCacheRemoveItemFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
                // mark as non-cached & free pool
                WCacheFreeSector(frame, Lba-firstLba);
                // check if frame is empty
                if(!Cache->FrameList[frame].BlockCount) {
                    WCacheRemoveFrame(Cache, Context, frame);
                }
            } else {
                BrutePoint();
            }
        } else {
            i++;
        }
    }

    PacketTail = Cache->WriteCount & (MaxReloc-1);
    IncompletePacket = (Cache->WriteCount >= MaxReloc) ? FALSE : TRUE;

    // remove(flush) packet
    while((Cache->WriteCount > PacketTail) || (Cache->WriteCount && IncompletePacket)) {

        Lba = List[0];
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            BrutePoint();
            return;
        }
        // check if modified
        mod = WCacheGetModFlag(block_array, Lba - firstLba);
        // pack/reloc/write
        if(mod) {
            DbgCopyMemory(tmp_buff + (RelocCount << BSh),
                          (PVOID)WCacheSectorAddr(block_array, Lba-firstLba),
                          BS);
            reloc_tab[RelocCount] = Lba;
            RelocCount++;
            // write packet
            if((RelocCount >= MaxReloc) || (Cache->BlockCount == 1)) {
//                status = Cache->WriteProcAsync(Context, tmp_buff, PS, Lba, &ReadBytes, FALSE);
                Cache->UpdateRelocProc(Context, NULL, reloc_tab, RelocCount);
                status = Cache->WriteProc(Context, tmp_buff, RelocCount<<BSh, NULL, &ReadBytes, 0);
                if(!OS_SUCCESS(status)) {
                    status = WCacheRaiseIoError(Cache, Context, status, NULL, RelocCount, tmp_buff, WCACHE_W_OP, NULL);
                }
                RelocCount = 0;
            }
            WCacheRemoveItemFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
        } else {
            BrutePoint();
        }
        // mark as non-cached & free pool
        if(WCacheSectorAddr(block_array,Lba-firstLba)) {
            WCacheRemoveItemFromList(List, &(Cache->BlockCount), Lba);
            // mark as non-cached & free pool
            WCacheFreeSector(frame, Lba-firstLba);
            // check if frame is empty
            if(!Cache->FrameList[frame].BlockCount) {
                WCacheRemoveFrame(Cache, Context, frame);
            }
        } else {
            BrutePoint();
        }
    }
} // end WCachePurgeAllR()

/*
  WCacheSetMode__() changes cache operating mode (ROM/R/RW/RAM).
  Public routine
 */
OSSTATUS
WCacheSetMode__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN ULONG Mode             // cache mode/media type to be used
    )
{
    if(Mode > WCACHE_MODE_MAX) return STATUS_INVALID_PARAMETER;
    Cache->Mode = Mode;
    return STATUS_SUCCESS;
} // end WCacheSetMode__()

/*
  WCacheGetMode__() returns cache operating mode (ROM/R/RW/RAM).
  Public routine
 */
ULONG
WCacheGetMode__(
    IN PW_CACHE Cache
    )
{
    return Cache->Mode;
} // end WCacheGetMode__()

/*
  WCacheGetWriteBlockCount__() returns number of modified blocks, those are
  not flushed to media. Is usually used to preallocate blocks for
  relocation table on WORM (R) media.
  Public routine
 */
ULONG
WCacheGetWriteBlockCount__(
    IN PW_CACHE Cache
    )
{
    return Cache->WriteCount;
} // end WCacheGetWriteBlockCount__()

/*
  WCacheSyncReloc__() builds list of all modified blocks, currently
  stored in cache. For each modified block WCacheSyncReloc__() calls
  user-supplied callback routine in order to update relocation table
  on WORM (R) media.
  Public routine
 */
VOID
WCacheSyncReloc__(
    IN PW_CACHE Cache,
    IN PVOID Context)
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List = Cache->CachedBlocksList;
    lba_t Lba;
//    ULONG BSh = Cache->BlockSizeSh;
//    ULONG BS = Cache->BlockSize;
//    ULONG PS = BS << Cache->PacketSizeSh; // packet size (bytes)
//    ULONG PSs = Cache->PacketSize;
    PW_CACHE_ENTRY block_array;
    BOOLEAN mod;
    ULONG MaxReloc = Cache->PacketSize;
    PULONG reloc_tab = Cache->reloc_tab;
    ULONG RelocCount = 0;
    BOOLEAN IncompletePacket;

    IncompletePacket = (Cache->WriteCount >= MaxReloc) ? FALSE : TRUE;
    // enumerate modified blocks
    for(ULONG i=0; IncompletePacket && (i<Cache->BlockCount); i++) {

        Lba = List[i];
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            return;
        }
        // check if modified
        mod = WCacheGetModFlag(block_array, Lba - firstLba);
        // update relocation table for modified sectors
        if(mod && (Cache->CheckUsedProc(Context, Lba) & WCACHE_BLOCK_USED)) {
            reloc_tab[RelocCount] = Lba;
            RelocCount++;
            if(RelocCount >= Cache->WriteCount) {
                Cache->UpdateRelocProc(Context, NULL, reloc_tab, RelocCount);
                break;
            }
        }
    }
} // end WCacheSyncReloc__()

/*
  WCacheDiscardBlocks__() removes specified blocks from cache.
  Blocks are not flushed to media.
  Public routine
 */
VOID
WCacheDiscardBlocks__(
    IN PW_CACHE Cache,
    IN PVOID Context,
    IN lba_t ReqLba,
    IN ULONG BCount
    )
{
    ULONG frame;
    lba_t firstLba;
    lba_t* List;
    lba_t Lba;
    PW_CACHE_ENTRY block_array;
    BOOLEAN mod;
    ULONG i;

    ExAcquireResourceExclusiveLite(&(Cache->WCacheLock), TRUE);

    UDFPrint(("  Discard req: %x@%x\n",BCount, ReqLba));

    List = Cache->CachedBlocksList;
    if(!List) {
        ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
        return;
    }
    i = WCacheGetSortedListIndex(Cache->BlockCount, List, ReqLba);

    // enumerate requested blocks
    while((List[i] < (ReqLba+BCount)) && (i < Cache->BlockCount)) {

        Lba = List[i];
        frame = Lba >> Cache->BlocksPerFrameSh;
        firstLba = frame << Cache->BlocksPerFrameSh;
        block_array = Cache->FrameList[frame].Frame;
        if(!block_array) {
            ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
            BrutePoint();
            return;
        }
        // check if modified
        mod = WCacheGetModFlag(block_array, Lba - firstLba);
        // just discard

        // mark as non-cached & free pool
        if(WCacheSectorAddr(block_array,Lba-firstLba)) {
            WCacheRemoveItemFromList(List, &(Cache->BlockCount), Lba);
            if(mod)
                WCacheRemoveItemFromList(Cache->CachedModifiedBlocksList, &(Cache->WriteCount), Lba);
            // mark as non-cached & free pool
            WCacheFreeSector(frame, Lba-firstLba);
            // check if frame is empty
            if(!Cache->FrameList[frame].BlockCount) {
                WCacheRemoveFrame(Cache, Context, frame);
            } else {
                ASSERT(Cache->FrameList[frame].Frame);
            }
        } else {
            // we should never get here !!!
            // getting this part of code means that we have
            // placed non-cached block in CachedBlocksList
            BrutePoint();
        }
    }
    ExReleaseResourceForThreadLite(&(Cache->WCacheLock), ExGetCurrentResourceThread());
} // end WCacheDiscardBlocks__()

OSSTATUS
WCacheCompleteAsync__(
    IN PVOID WContext,
    IN OSSTATUS Status
    )
{
    PW_CACHE_ASYNC AsyncCtx = (PW_CACHE_ASYNC)WContext;
//    PW_CACHE Cache = AsyncCtx->Cache;

    AsyncCtx->PhContext.IosbToUse.Status = Status;
    KeSetEvent(&(AsyncCtx->PhContext.event), 0, FALSE);

    return STATUS_SUCCESS;
} // end WCacheSetMode__()

/*
  WCacheDecodeFlags() updates internal BOOLEANs according to Flags
  Internal routine
 */
OSSTATUS
__fastcall
WCacheDecodeFlags(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN ULONG Flags            // cache mode flags
    )
{
    //ULONG OldFlags;
    if(Flags & ~WCACHE_VALID_FLAGS) {
        UDFPrint(("Invalid flags: %x\n", Flags & ~WCACHE_VALID_FLAGS));
        return STATUS_INVALID_PARAMETER;
    }
    Cache->CacheWholePacket = (Flags & WCACHE_CACHE_WHOLE_PACKET) ? TRUE : FALSE;
    Cache->DoNotCompare = (Flags & WCACHE_DO_NOT_COMPARE) ? TRUE : FALSE;
    Cache->Chained = (Flags & WCACHE_CHAINED_IO) ? TRUE : FALSE;
    Cache->RememberBB = (Flags & WCACHE_MARK_BAD_BLOCKS) ? TRUE : FALSE;
    if(Cache->RememberBB) {
        Cache->NoWriteBB = (Flags & WCACHE_RO_BAD_BLOCKS) ? TRUE : FALSE;
    }
    Cache->NoWriteThrough = (Flags & WCACHE_NO_WRITE_THROUGH) ? TRUE : FALSE;

    Cache->Flags = Flags;

    return STATUS_SUCCESS;
}

/*
  WCacheChFlags__() changes cache flags.
  Public routine
 */
ULONG
WCacheChFlags__(
    IN PW_CACHE Cache,        // pointer to the Cache Control structure
    IN ULONG SetFlags,        // cache mode/media type to be set
    IN ULONG ClrFlags         // cache mode/media type to be cleared
    )
{
    ULONG Flags;

    if(SetFlags || ClrFlags) {
        Flags = (Cache->Flags & ~ClrFlags) | SetFlags;

        if(!OS_SUCCESS(WCacheDecodeFlags(Cache, Flags))) {
            return -1;
        }
    } else {
        return Cache->Flags;
    }
    return Flags;
} // end WCacheSetMode__()
