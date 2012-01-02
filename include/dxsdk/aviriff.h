/*
 * Copyright (C) 2003 Robert Shearman
 * Copyright (C) 2008 Maarten Lankhorst
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* All definitions here are packed structures of on-disk formats */
#include <pshpack2.h>

typedef struct _riffchunk
{
    FOURCC fcc;
    DWORD cb;
} RIFFCHUNK, * LPRIFFCHUNK;

typedef struct _rifflist
{
    FOURCC fcc;
    DWORD cb;
    FOURCC fccListType;
} RIFFLIST, * LPRIFFLIST;

#define FCC( ch0, ch1, ch2, ch3 ) \
    ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) <<  8 ) | \
    ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

#define RIFFROUND(cb) ((cb) + ((cb)&1))
#define RIFFNEXT(pChunk) (LPRIFFCHUNK)((LPBYTE)(pChunk)+sizeof(RIFFCHUNK)+RIFFROUND(((LPRIFFCHUNK)pChunk)->cb))

/* flags for dwFlags member of AVIMAINHEADER */
#define AVIF_HASINDEX       0x00000010
#define AVIF_MUSTUSEINDEX   0x00000020
#define AVIF_ISINTERLEAVED  0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED    0x00020000

#define ckidMAINAVIHEADER FCC('a','v','i','h')
typedef struct _avimainheader
{
    FOURCC fcc;
    DWORD cb;
    DWORD dwMicroSecPerFrame;
    DWORD dwMaxBytesPerSec;
    DWORD dwPaddingGranularity;
    DWORD dwFlags;
    DWORD dwTotalFrames;
    DWORD dwInitialFrames;
    DWORD dwStreams;
    DWORD dwSuggestedBufferSize;
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD dwReserved[4];
} AVIMAINHEADER;

#define ckidODML FCC('o','d','m','l')
#define ckidAVIEXTHEADER  FCC('d','m','l','h')
typedef struct _aviextheader
{
    FOURCC fcc;
    DWORD cb;
    DWORD dwGrandFrames;
    DWORD dwFuture[61];
} AVIEXTHEADER;

#define ckidSTREAMLIST FCC('s','t','r','l')

/* flags for dwFlags member of AVISTREAMHEADER */
#define AVISF_DISABLED         0x00000001
#define AVISF_VIDEO_PALCHANGES 0x00010000

#ifndef ckidSTREAMHEADER
#define ckidSTREAMHEADER FCC('s','t','r','h')
#endif

#ifndef streamtypeVIDEO
#define streamtypeVIDEO FCC('v','i','d','s')
#define streamtypeAUDIO FCC('a','u','d','s')
#define streamtypeMIDI FCC('m','i','d','s')
#define streamtypeTEXT FCC('t','x','t','s')
#endif

typedef struct _avistreamheader
{
    FOURCC fcc;
    DWORD cb;
    FOURCC fccType;
    FOURCC fccHandler;
    DWORD dwFlags;
    WORD wPriority;
    WORD wLanguage;
    DWORD dwInitialFrames;
    DWORD dwScale;
    DWORD dwRate;
    DWORD dwStart;
    DWORD dwLength;
    DWORD dwSuggestedBufferSize;
    DWORD dwQuality;
    DWORD dwSampleSize;
    struct
    {
        short int left;
        short int top;
        short int right;
        short int bottom;
    } rcFrame;
} AVISTREAMHEADER;

#ifndef ckidSTREAMFORMAT
#define ckidSTREAMFORMAT FCC('s','t','r','f')
#endif
#define ckidAVIOLDINDEX FCC('i','d','x','1')

/* flags for dwFlags member of _avioldindex_entry */
#define AVIIF_LIST       0x00000001
#define AVIIF_KEYFRAME   0x00000010
#define AVIIF_NO_TIME    0x00000100
#define AVIIF_COMPRESSOR 0x0FFF0000

typedef struct _avioldindex
{
    FOURCC fcc;
    DWORD cb;
    struct _avioldindex_entry
    {
        DWORD dwChunkId;
        DWORD dwFlags;
        DWORD dwOffset;
        DWORD dwSize;
    } aIndex[ANYSIZE_ARRAY];
} AVIOLDINDEX;

typedef union _timecode
{
    struct
    {
        WORD wFrameRate;
        WORD wFrameFract;
        LONG cFrames;
    } DUMMYSTRUCTNAME;
    DWORDLONG qw;
} TIMECODE;

#define TIMECODE_RATE_30DROP 0

/* flags for dwSMPTEflags member of TIMECODEDATA */
#define TIMECODE_SMPTE_BINARY_GROUP 0x07
#define TIMECODE_SMPTE_COLOR_FRAME  0x08

typedef struct _timecodedata
{
    TIMECODE time;
    DWORD dwSMPTEflags;
    DWORD dwUser;
} TIMECODEDATA;

#define AVI_INDEX_OF_INDEXES      0x00
#define AVI_INDEX_OF_CHUNKS       0x01
#define AVI_INDEX_OF_TIMED_CHUNKS 0x02
#define AVI_INDEX_OF_SUB_2FIELD   0x03
#define AVI_INDEX_IS_DATA         0x80

#define AVI_INDEX_SUB_DEFAULT     0x00
#define AVI_INDEX_SUB_2FIELD      0x01

typedef struct _avimetaindex
{
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORD dwReserved[3];
    DWORD adwIndex[ANYSIZE_ARRAY];
} AVIMETAINDEX;

#define ckidAVISUPERINDEX FCC('i','n','d','x')
typedef struct _avisuperindex {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORD dwReserved[3];
    struct _avisuperindex_entry {
        DWORDLONG qwOffset;
        DWORD dwSize;
        DWORD dwDuration;
    } aIndex[ANYSIZE_ARRAY];
} AVISUPERINDEX;

#define AVISTDINDEX_DELTAFRAME (0x80000000)
#define AVISTDINDEX_SIZEMASK (~0x80000000)

typedef struct _avistdindex_entry {
    DWORD dwOffset;
    DWORD dwSize;
} AVISTDINDEX_ENTRY;

typedef struct _avistdindex {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD    nEntriesInUse;
    DWORD    dwChunkId;
    DWORDLONG qwBaseOffset;
    DWORD    dwReserved_3;
    AVISTDINDEX_ENTRY aIndex[ANYSIZE_ARRAY];
} AVISTDINDEX;

typedef struct _avitimedindex_entry {
    DWORD dwOffset;
    DWORD dwSize;
    DWORD dwDuration;
} AVITIMEDINDEX_ENTRY;

typedef struct _avitimedindex {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORDLONG qwBaseOffset;
    DWORD dwReserved_3;
    AVITIMEDINDEX_ENTRY aIndex[ANYSIZE_ARRAY];
    /* DWORD adwTrailingFill[ANYSIZE_ARRAY]; */
} AVITIMEDINDEX;

typedef struct _avitimecodeindex {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORD dwReserved[3];
    TIMECODEDATA aIndex[ANYSIZE_ARRAY];
} AVITIMECODEINDEX;

typedef struct _avitcdlindex_entryA {
    DWORD dwTick;
    TIMECODE time;
    DWORD dwSMPTEflags;
    DWORD dwUser;
    CHAR szReelId[12];
} AVITCDLINDEX_ENTRYA;

typedef struct _avitcdlindex_entryW {
    DWORD dwTick;
    TIMECODE time;
    DWORD dwSMPTEflags;
    DWORD dwUser;
    WCHAR szReelId[12];
} AVITCDLINDEX_ENTRYW;

typedef struct _avitcdlindexA {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORD dwReserved[3];
    AVITCDLINDEX_ENTRYA aIndex[ANYSIZE_ARRAY];
    /* DWORD adwTrailingFill[ANYSIZE_ARRAY]; */
} AVITCDLINDEXA;

typedef struct _avitcdlindexW {
    FOURCC fcc;
    UINT cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORD dwReserved[3];
    AVITCDLINDEX_ENTRYW aIndex[ANYSIZE_ARRAY];
    /* DWORD adwTrailingFill[ANYSIZE_ARRAY]; */
} AVITCDLINDEXW;

#define AVITCDLINDEX_ENTRY WINELIB_NAME_AW(AVITCDLINDEX_ENTRY)
#define AVITCDLINDEX WINELIB_NAME_AW(AVITCDLINDEX)

typedef struct _avifieldindex_chunk {
    FOURCC fcc;
    DWORD cb;
    WORD wLongsPerEntry;
    BYTE bIndexSubType;
    BYTE bIndexType;
    DWORD nEntriesInUse;
    DWORD dwChunkId;
    DWORDLONG qwBaseOffset;
    DWORD dwReserved3;
    struct _avifieldindex_entry {
        DWORD dwOffset;
        DWORD dwSize;
        DWORD dwOffsetField2;
    } aIndex[ANYSIZE_ARRAY];
} AVIFIELDINDEX, * PAVIFIELDINDEX;

#include <poppack.h>
