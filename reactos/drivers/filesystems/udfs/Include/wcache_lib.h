////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __CDRW_WCACHE_LIB_H__
#define __CDRW_WCACHE_LIB_H__

extern "C" {

#include "platform.h"

#ifdef _CONSOLE
#include "env_spec_w32.h"
#else
//#include "env_spec.h"
#endif 

#define WCACHE_BOUND_CHECKS

typedef OSSTATUS     (*PWRITE_BLOCK) (IN PVOID Context,
                                      IN PVOID Buffer,     // Target buffer
                                      IN ULONG Length,
                                      IN lba_t Lba,
                                      OUT PULONG WrittenBytes,
                                      IN uint32 Flags);

typedef OSSTATUS     (*PREAD_BLOCK) (IN PVOID Context,
                                     IN PVOID Buffer,     // Target buffer
                                     IN ULONG Length,
                                     IN lba_t Lba,
                                     OUT PULONG ReadBytes,
                                     IN uint32 Flags);

typedef OSSTATUS     (*PWRITE_BLOCK_ASYNC) (IN PVOID Context,
                                            IN PVOID WContext,
                                            IN PVOID Buffer,     // Target buffer
                                            IN ULONG Length,
                                            IN lba_t Lba,
                                            OUT PULONG WrittenBytes,
                                            IN BOOLEAN FreeBuffer);

typedef OSSTATUS     (*PREAD_BLOCK_ASYNC) (IN PVOID Context,
                                           IN PVOID WContext,
                                           IN PVOID Buffer,     // Source buffer
                                           IN ULONG Length,
                                           IN lba_t Lba,
                                           OUT PULONG ReadBytes);

/*typedef BOOLEAN      (*PCHECK_BLOCK) (IN PVOID Context,
                                      IN lba_t Lba);*/

#define WCACHE_BLOCK_USED    0x01
#define WCACHE_BLOCK_ZERO    0x02
#define WCACHE_BLOCK_BAD     0x04

typedef ULONG        (*PCHECK_BLOCK) (IN PVOID Context,
                                      IN lba_t Lba);

typedef OSSTATUS     (*PUPDATE_RELOC) (IN PVOID Context,
                                       IN lba_t Lba,
                                       IN PULONG RelocTab,
                                       IN ULONG BCount);

#define WCACHE_ERROR_READ     0x0001
#define WCACHE_ERROR_WRITE    0x0002
#define WCACHE_ERROR_INTERNAL 0x0003

#define WCACHE_W_OP     FALSE
#define WCACHE_R_OP     TRUE

typedef struct _WCACHE_ERROR_CONTEXT {
    ULONG WCErrorCode;
    OSSTATUS Status;
    BOOLEAN  Retry;
    UCHAR    Padding[3];
    union {
        struct {
            ULONG    Lba;
            ULONG    BCount;
            PVOID    Buffer;
        } ReadWrite;
        struct {
            ULONG    p1;
            ULONG    p2;
            ULONG    p3;
            ULONG    p4;
        } Internal;
    };
} WCACHE_ERROR_CONTEXT, *PWCACHE_ERROR_CONTEXT;

typedef OSSTATUS     (*PWC_ERROR_HANDLER) (IN PVOID Context,
                                           IN PWCACHE_ERROR_CONTEXT ErrorInfo);
// array of pointers to cached data
// each entry corresponds to logical block on disk
typedef struct _W_CACHE_ENTRY {
    union {
        PCHAR Sector;
        UCHAR Flags:3;
    };
} W_CACHE_ENTRY, *PW_CACHE_ENTRY;

// array of pointers to cache frames
// each frame corresponds to extent of logical blocks
typedef struct _W_CACHE_FRAME {
    PW_CACHE_ENTRY Frame;
    ULONG BlockCount;
    //ULONG WriteCount;      // number of modified packets in cache frame, is always 0, shall be removed
    ULONG UpdateCount;     // number of updates in cache frame
    ULONG AccessCount;     // number of accesses to cache frame
} W_CACHE_FRAME, *PW_CACHE_FRAME;

// memory type for cached blocks
#define CACHED_BLOCK_MEMORY_TYPE PagedPool
#define MAX_TRIES_FOR_NA         3

#define WCACHE_ADDR_MASK     0xfffffff8
#define WCACHE_FLAG_MASK     0x00000007
#define WCACHE_FLAG_MODIFIED 0x00000001
#define WCACHE_FLAG_ZERO     0x00000002
#define WCACHE_FLAG_BAD      0x00000004

#define WCACHE_MODE_ROM      0x00000000  // read only (CD-ROM)
#define WCACHE_MODE_RW       0x00000001  // rewritable (CD-RW)
#define WCACHE_MODE_R        0x00000002  // WORM (CD-R)
#define WCACHE_MODE_RAM      0x00000003  // random writable device (HDD)
#define WCACHE_MODE_EWR      0x00000004  // ERASE-cycle required (MO)
#define WCACHE_MODE_MAX      WCACHE_MODE_RAM

#define PH_TMP_BUFFER          1

struct _W_CACHE_ASYNC;

typedef struct _W_CACHE {
    // cache tables
    ULONG Tag;
    PW_CACHE_FRAME FrameList;   // pointer to list of Frames
    lba_t* CachedBlocksList;    // sorted list of cached blocks
    lba_t* CachedFramesList;    // sorted list of cached frames
    lba_t* CachedModifiedBlocksList;    // sorted list of cached modified blocks
    // settings & current state
    ULONG BlocksPerFrame;
    ULONG BlocksPerFrameSh;
    ULONG BlockCount;
    ULONG MaxBlocks;
    ULONG MaxBytesToRead;
    ULONG FrameCount;
    ULONG MaxFrames;
    ULONG PacketSize;      // number of blocks in packet
    ULONG PacketSizeSh;
    ULONG BlockSize;
    ULONG BlockSizeSh;
    ULONG WriteCount;      // number of modified packets in cache
    lba_t FirstLba;
    lba_t LastLba;
    ULONG Mode;            // RO/WOR/RW/EWR

    ULONG Flags;
    BOOLEAN CacheWholePacket;
    BOOLEAN DoNotCompare;
    BOOLEAN Chained;
    BOOLEAN RememberBB;
    BOOLEAN NoWriteBB;
    BOOLEAN NoWriteThrough;
    UCHAR  Padding[2];

    ULONG RBalance;
    ULONG WBalance;
    ULONG FramesToKeepFree;
    // callbacks
    PWRITE_BLOCK WriteProc;
    PREAD_BLOCK ReadProc;
    PWRITE_BLOCK_ASYNC WriteProcAsync;
    PREAD_BLOCK_ASYNC ReadProcAsync;
    PCHECK_BLOCK CheckUsedProc;
    PUPDATE_RELOC UpdateRelocProc;
    PWC_ERROR_HANDLER ErrorHandlerProc;
    // sync resource
    ERESOURCE WCacheLock;
//    BOOLEAN WCResInit;
    // preallocated tmp buffers
    PCHAR tmp_buff;
    PCHAR tmp_buff_r;
    PULONG reloc_tab;

} W_CACHE, *PW_CACHE;

#define WCACHE_INVALID_LBA  ((lba_t)(-1))

#define WCACHE_CACHE_WHOLE_PACKET   0x01
#define WCACHE_DO_NOT_COMPARE       0x02
#define WCACHE_CHAINED_IO           0x04
#define WCACHE_MARK_BAD_BLOCKS      0x08
#define WCACHE_RO_BAD_BLOCKS        0x10
#define WCACHE_NO_WRITE_THROUGH     0x20

#define WCACHE_VALID_FLAGS          (WCACHE_CACHE_WHOLE_PACKET | \
                                     WCACHE_DO_NOT_COMPARE | \
                                     WCACHE_CHAINED_IO | \
                                     WCACHE_MARK_BAD_BLOCKS | \
                                     WCACHE_RO_BAD_BLOCKS | \
                                     WCACHE_NO_WRITE_THROUGH)

#define WCACHE_INVALID_FLAGS        (0xffffffff)

// init cache
OSSTATUS WCacheInit__(IN PW_CACHE Cache,
                      IN ULONG MaxFrames,
                      IN ULONG MaxBlocks,
                      IN ULONG MaxBytesToRead,
                      IN ULONG PacketSizeSh,    // number of blocks in packet (bit shift)
                      IN ULONG BlockSizeSh,     // bit shift
                      IN ULONG BlocksPerFrameSh,// bit shift
                      IN lba_t FirstLba,
                      IN lba_t LastLba,
                      IN ULONG Mode,
                      IN ULONG Flags,
                      IN ULONG FramesToKeepFree,
                      IN PWRITE_BLOCK WriteProc,
                      IN PREAD_BLOCK ReadProc,
                      IN PWRITE_BLOCK_ASYNC WriteProcAsync,
                      IN PREAD_BLOCK_ASYNC ReadProcAsync,
                      IN PCHECK_BLOCK CheckUsedProc,
                      IN PUPDATE_RELOC UpdateRelocProc,
                      IN PWC_ERROR_HANDLER ErrorHandlerProc);
// write cached
OSSTATUS WCacheWriteBlocks__(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN PCHAR Buffer,
                             IN lba_t Lba,
                             IN ULONG BCount,
                             OUT PULONG WrittenBytes,
                             IN BOOLEAN CachedOnly);
// read cached
OSSTATUS WCacheReadBlocks__(IN PW_CACHE Cache,
                            IN PVOID Context,
                            IN PCHAR Buffer,
                            IN lba_t Lba,
                            IN ULONG BCount,
                            OUT PULONG ReadBytes,
                            IN BOOLEAN CachedOnly);
// flush blocks
OSSTATUS WCacheFlushBlocks__(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t Lba,
                             IN ULONG BCount);
// discard blocks
VOID     WCacheDiscardBlocks__(IN PW_CACHE Cache,
                               IN PVOID Context,
                               IN lba_t Lba,
                               IN ULONG BCount);
// flush whole cache
VOID     WCacheFlushAll__(IN PW_CACHE Cache,
                          IN PVOID Context);
// purge whole cache
VOID     WCachePurgeAll__(IN PW_CACHE Cache,
                          IN PVOID Context);
// free structures
VOID     WCacheRelease__(IN PW_CACHE Cache);

// check if initialized
BOOLEAN  WCacheIsInitialized__(IN PW_CACHE Cache);

// direct access to cached data
OSSTATUS WCacheDirect__(IN PW_CACHE Cache,
                        IN PVOID Context,
                        IN lba_t Lba,
                        IN BOOLEAN Modified,
                        OUT PCHAR* CachedBlock,
                        IN BOOLEAN CachedOnly);
// release resources after direct access
OSSTATUS WCacheEODirect__(IN PW_CACHE Cache,
                          IN PVOID Context);
// release resources before direct access
OSSTATUS WCacheStartDirect__(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN BOOLEAN Exclusive);
// check if requested extent completly cached
BOOLEAN  WCacheIsCached__(IN PW_CACHE Cache,
                          IN lba_t Lba,
                          IN ULONG BCount);

// change cache media mode
OSSTATUS WCacheSetMode__(IN PW_CACHE Cache,
                         IN ULONG Mode);
//
ULONG    WCacheGetMode__(IN PW_CACHE Cache);
//
ULONG    WCacheGetWriteBlockCount__(IN PW_CACHE Cache);
//
VOID     WCacheSyncReloc__(IN PW_CACHE Cache,
                           IN PVOID Context);

VOID     WCacheDiscardBlocks__(IN PW_CACHE Cache,
                             IN PVOID Context,
                             IN lba_t ReqLba,
                             IN ULONG BCount);

ULONG    WCacheChFlags__(IN PW_CACHE Cache,
                         IN ULONG SetFlags,
                         IN ULONG ClrFlags);

};

// complete async request (callback)
OSSTATUS WCacheCompleteAsync__(IN PVOID WContext,
                               IN OSSTATUS Status);

#endif // __CDRW_WCACHE_LIB_H__
