/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   AVIIDX.H - AVI Index stuff

*****************************************************************************/

#ifndef _INC_AVIFMT
#include <avifmt.h>
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
	#define EXTERN_C extern "C"
#else
	#define EXTERN_C extern
#endif
#endif

#define ERR_POS     (-100)      // bad position
#define ERR_IDX     (-1)        // bad index

typedef AVIINDEXENTRY _huge *PAVIINDEXENTRY;

//
//  this is the in memory form of a AVI INDEX, we want this to be 8 bytes
//  to save memory.
//
//  the bit fields may not be portable, so a new structure will be needed.
//
//  we always access via macros, so changing the structure should be posible
//
//  this stucture sotres the following:
//
//      offset  0-1GB   (30 bits)   we assume even number so only need 29bits
//      flags           (4 bits)
//      stream  0-127   (7 bits)
//      length  0-4MB   (24 bits)
//
#pragma pack(1)
typedef union {
    struct {
        DWORD   offset;     // 0-1GB
        DWORD   length;     // 0-4MB
    };
    struct {
        BYTE    ack[3];     // 24 bits of the offset
        WORD    flags;      // access to all flags
        BYTE    smag[3];    // length (24 bits)
    };
#if 0   // I hate bit-fields
    struct {
        DWORD   offset:29;  // 0-1GB
        DWORD   key:1;
        DWORD   nonkey:1;
        DWORD   pal:1;
        DWORD   stream:7;   // 0-127
        DWORD   half:1;
        DWORD   length:24;  // 0-4MB
    };
#endif
}   AVIIDX;
#pragma pack()

//
// limits
//
#define MAX_OFFSET  (1l<<30)
#define MAX_LENGTH  (1l<<24)
#define MAX_STREAM  (1l<<7)

//
//  index flags
//
#define IDX_OFFHI   0x001F      // hi part of offset.
#define IDX_KEY     0x0020      // key frame
#define IDX_NONKEY  0x0040      // not a key frame (but not blank either)
#define IDX_PAL     0x0080      // palette change
#define IDX_STREAM  0x7F00      // stream number
#define IDX_HALF    0x8000      // RLE half frame.
#define IDX_NOTIME  IDX_PAL

//
// macros to acces index to help porting
//
#define Index(px, lx)               ((AVIIDX _huge *)px)[lx+1]
#define IndexOffset(px, lx)         (LONG)((Index(px,lx).offset & 0x1FFFFFFF) * 2)
#define IndexLength(px, lx)         (LONG)((Index(px,lx).length) >> 8)
#define IndexStream(px, lx)         (BYTE)((Index(px,lx).flags & IDX_STREAM) >> 8)
#define IndexFlags(px, lx)          (UINT)(Index(px,lx).flags)

#define IndexSetOffset(px, lx, x)   { Index(px,lx).offset &= ~0x1FFFFFFF; Index(px,lx).offset |= (DWORD)(x)>>1; }
#define IndexSetLength(px, lx, x)   { Index(px,lx).length &= ~0xFFFFFF00; Index(px,lx).length |= (DWORD)(x)<<8; }
#define IndexSetStream(px, lx, x)   { Index(px,lx).flags  &= ~IDX_STREAM; Index(px,lx).flags  |= (DWORD)(x)<<8; }
#define IndexSetFlags(px, lx, x)    { Index(px,lx).flags  &= IDX_STREAM|IDX_OFFHI; Index(px,lx).flags |= (UINT)(x); }
#define IndexSetKey(px, lx)         { Index(px,lx).flags  |= IDX_KEY; Index(px,lx).flags &= ~(IDX_NONKEY); }

//
// special streams
//
#define STREAM_REC      0x7F        // interleave records.

//
// this is the header we put on a list of AVIIDX entries.
//
typedef struct
{
    LONG            nIndex;         // number of entries in index
    LONG            nIndexSize;     // alocated size
    AVIIDX          idx[];          // the entries.
}   AVIINDEX, _huge *PAVIINDEX;

//
// AVI Stream Index
//
typedef LONG (FAR PASCAL *STREAMIOPROC)(HANDLE hFile, LONG off, LONG cb, LPVOID p);

typedef struct
{
    UINT            stream;         // stream number
    UINT            flags;          // combination of all flags in index.

    PAVIINDEX       px;             // main index

    LONG            lx;             // Index index
    LONG            lPos;           // index position

    LONG            lxStart;        // Index start

    LONG            lStart;         // start of stream
    LONG            lEnd;           // end of stream

    LONG            lMaxSampleSize; // largest sample
    LONG            lSampleSize;    // sample size for stream

    LONG            lFrames;        // total "frames"
    LONG            lKeyFrames;     // total key "frames"
    LONG            lPalFrames;     // total pal "frames"
    LONG            lNulFrames;     // total nul "frames"

    HANDLE          hFile;
    STREAMIOPROC    Read;
    STREAMIOPROC    Write;

}   STREAMINDEX, *PSTREAMINDEX;

//
// create and free a index.
//
EXTERN_C PAVIINDEX IndexCreate(void);
#define   FreeIndex(px)  GlobalFreePtr(px)

//
//  convert to and from a file index
//
EXTERN_C PAVIINDEX IndexAddFileIndex(PAVIINDEX px, AVIINDEXENTRY _huge *pidx, LONG cnt, LONG lAdjust, BOOL fRle);
EXTERN_C LONG      IndexGetFileIndex(PAVIINDEX px, LONG l, LONG cnt, PAVIINDEXENTRY pidx, LONG lAdjust);

EXTERN_C PSTREAMINDEX MakeStreamIndex(PAVIINDEX px, UINT stream, LONG lStart, LONG lSampleSize, HANDLE hFile, STREAMIOPROC ReadProc, STREAMIOPROC WriteProc);
#define FreeStreamIndex(psx)    LocalFree((HLOCAL)psx)

//
// index access functions
//
EXTERN_C LONG IndexFirst(PAVIINDEX px, UINT stream);
EXTERN_C LONG IndexNext (PAVIINDEX px, LONG lx, UINT f);
EXTERN_C LONG IndexPrev (PAVIINDEX px, LONG lx, UINT f);

//
//  search index for data
//
#ifndef FIND_DIR
#define FIND_DIR        0x0000000FL     // direction
#define FIND_NEXT       0x00000001L     // go forward
#define FIND_PREV       0x00000004L     // go backward

#define FIND_TYPE       0x000000F0L     // type mask
#define FIND_KEY        0x00000010L     // find key frame.
#define FIND_ANY        0x00000020L     // find any (non-empty) sample
#define FIND_FORMAT     0x00000040L     // find format change
#endif

#ifndef FIND_RET
#define FIND_RET        0x0000F000L     // return mask
#define FIND_POS        0x00000000L     // return logical position
#define FIND_LENGTH     0x00001000L     // return logical size
#define FIND_OFFSET     0x00002000L     // return physical position
#define FIND_SIZE       0x00003000L     // return physical size
#define FIND_INDEX      0x00004000L     // return index position
#endif

EXTERN_C LONG StreamFindSample(PSTREAMINDEX psx, LONG lPos, UINT f);
EXTERN_C LONG StreamRead(PSTREAMINDEX psx, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer);
EXTERN_C LONG StreamWrite(PSTREAMINDEX psx, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer);
