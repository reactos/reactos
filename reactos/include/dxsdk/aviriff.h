



#if !defined AVIRIFF_H
#define AVIRIFF_H

#if !defined NUMELMS
  #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#include <pshpack2.h>

#define FCC(ch4)              ((((DWORD)(ch4) & 0xFF) << 24) | (((DWORD)(ch4) & 0xFF00) << 8) | (((DWORD)(ch4) & 0xFF0000) >> 8) | (((DWORD)(ch4) & 0xFF000000) >> 24))
#define RIFFROUND(cb)         ((cb) + ((cb)&1))
#define RIFFNEXT(pChunk)      (LPRIFFCHUNK)((LPBYTE)(pChunk) + sizeof(RIFFCHUNK) + RIFFROUND(((LPRIFFCHUNK)pChunk)->cb))
#define TIMECODE_RATE_30DROP   0
#define TIMECODE_SMPTE_BINARY_GROUP     0x07
#define TIMECODE_SMPTE_COLOR_FRAME      0x08
#define AVI_INDEX_OF_INDEXES            0x00
#define AVI_INDEX_OF_CHUNKS             0x01
#define AVI_INDEX_OF_TIMED_CHUNKS       0x02
#define AVI_INDEX_OF_SUB_2FIELD         0x03
#define AVI_INDEX_IS_DATA               0x80
#define AVI_INDEX_SUB_DEFAULT           0x00
#define AVI_INDEX_SUB_2FIELD            0x01
#define STDINDEXSIZE                    0x4000
#define NUMINDEX(wLongsPerEntry)        ((STDINDEXSIZE-32)/4/(wLongsPerEntry))
#define NUMINDEXFILL(wLongsPerEntry)    ((STDINDEXSIZE/4) - NUMINDEX(wLongsPerEntry))
#define Valid_SUPERINDEX(pi)            (*(DWORD *)(&((pi)->wLongsPerEntry)) == (4 | (AVI_INDEX_OF_INDEXES << 24)))
#define AVISTDINDEX_DELTAFRAME          ( 0x80000000)
#define AVISTDINDEX_SIZEMASK            (~0x80000000)

#ifndef ckidSTREAMHEADER
  #define ckidSTREAMHEADER      FCC('strh')
#endif

#ifndef ckidSTREAMFORMAT
  #define ckidSTREAMFORMAT      FCC('strf')
#endif

#define ckidMAINAVIHEADER     FCC('avih')
#define ckidODML              FCC('odml')
#define ckidAVIEXTHEADER      FCC('dmlh')
#define ckidSTREAMLIST        FCC('strl')
#define ckidAVIOLDINDEX       FCC('idx1')
#define ckidAVISUPERINDEX     FCC('indx')

#ifndef TIMECODE_DEFINED
#define TIMECODE_DEFINED
  typedef union _timecode
  {
    struct
    {
      WORD wFrameRate;
      WORD wFrameFract;
      LONG cFrames;
    };
    DWORDLONG  qw;
  } TIMECODE;
#endif


typedef struct _riffchunk
{
 FOURCC fcc;
 DWORD cb;
 } RIFFCHUNK, *LPRIFFCHUNK;

typedef struct _rifflist
{
  FOURCC fcc;
  DWORD cb;
  FOURCC fccListType;
} RIFFLIST, *LPRIFFLIST;

typedef struct _avimainheader
{
  FOURCC fcc;
  DWORD cb;
  DWORD dwMicroSecPerFrame;
  DWORD dwMaxBytesPerSec;
  DWORD dwPaddingGranularity;
  DWORD dwFlags;
  #define AVIF_HASINDEX        0x00000010
  #define AVIF_MUSTUSEINDEX    0x00000020
  #define AVIF_ISINTERLEAVED   0x00000100
  #define AVIF_TRUSTCKTYPE     0x00000800
  #define AVIF_WASCAPTUREFILE  0x00010000
  #define AVIF_COPYRIGHTED     0x00020000
  DWORD dwTotalFrames;
  DWORD dwInitialFrames;
  DWORD dwStreams;
  DWORD dwSuggestedBufferSize;
  DWORD dwWidth;
  DWORD dwHeight;
  DWORD dwReserved[4];
} AVIMAINHEADER;

typedef struct _aviextheader
{
  FOURCC fcc;
  DWORD cb;
  DWORD dwGrandFrames;
  DWORD dwFuture[61];
} AVIEXTHEADER;

typedef struct _avistreamheader
{
  FOURCC fcc;
  DWORD cb;
  FOURCC fccType;
  #ifndef streamtypeVIDEO
    #define streamtypeVIDEO FCC('vids')
    #define streamtypeAUDIO FCC('auds')
    #define streamtypeMIDI FCC('mids')
    #define streamtypeTEXT FCC('txts')
  #endif

  FOURCC fccHandler;
  DWORD  dwFlags;
  #define AVISF_DISABLED 0x00000001
  #define AVISF_VIDEO_PALCHANGES 0x00010000

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

#pragma warning(disable:4200)

typedef struct _avioldindex
{
  FOURCC fcc;
  DWORD cb;
  struct _avioldindex_entry
  {
    DWORD dwChunkId;
    DWORD dwFlags;

    #ifndef AVIIF_LIST
      #define AVIIF_LIST 0x00000001
      #define AVIIF_KEYFRAME 0x00000010
    #endif

    #define AVIIF_NO_TIME 0x00000100
    #define AVIIF_COMPRESSOR 0x0FFF0000
    DWORD dwOffset;
    DWORD dwSize;
  } aIndex[];
} AVIOLDINDEX;

typedef struct _timecodedata
{
  TIMECODE time;
  DWORD dwSMPTEflags;
  DWORD dwUser;
} TIMECODEDATA;

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
  DWORD adwIndex[];
} AVIMETAINDEX;


typedef struct _avisuperindex
{
  FOURCC fcc;
  UINT cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORD dwReserved[3];
  struct _avisuperindex_entry
   {
     DWORDLONG qwOffset;
     DWORD    dwSize;
     DWORD    dwDuration;
   } aIndex[NUMINDEX(4)];
} AVISUPERINDEX;

typedef struct _avistdindex_entry
{
  DWORD dwOffset;
  DWORD dwSize;
} AVISTDINDEX_ENTRY;

typedef struct _avistdindex
{
  FOURCC fcc;
  UINT cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORDLONG qwBaseOffset;
  DWORD dwReserved_3;
  AVISTDINDEX_ENTRY aIndex[NUMINDEX(2)];
} AVISTDINDEX;


typedef struct _avitimedindex_entry
{
  DWORD dwOffset;
  DWORD dwSize;
  DWORD dwDuration;
} AVITIMEDINDEX_ENTRY;

typedef struct _avitimedindex
{
  FOURCC fcc;
  UINT cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORDLONG qwBaseOffset;
  DWORD dwReserved_3;
  AVITIMEDINDEX_ENTRY aIndex[NUMINDEX(3)];
  DWORD adwTrailingFill[NUMINDEXFILL(3)];
} AVITIMEDINDEX;

typedef struct _avitimecodeindex
{
  FOURCC fcc;
  UINT cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORD dwReserved[3];
  TIMECODEDATA aIndex[NUMINDEX(sizeof(TIMECODEDATA)/sizeof(LONG))];
} AVITIMECODEINDEX;


typedef struct _avitcdlindex_entry
{
  DWORD dwTick;
  TIMECODE time;
  DWORD dwSMPTEflags;
  DWORD dwUser;
  TCHAR szReelId[12];
} AVITCDLINDEX_ENTRY;

typedef struct _avitcdlindex
{
  FOURCC fcc;
  UINT cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORD dwReserved[3];
  AVITCDLINDEX_ENTRY aIndex[NUMINDEX(7)];
  DWORD adwTrailingFill[NUMINDEXFILL(7)];
} AVITCDLINDEX;

typedef struct _avifieldindex_chunk
{
  FOURCC fcc;
  DWORD cb;
  WORD wLongsPerEntry;
  BYTE bIndexSubType;
  BYTE bIndexType;
  DWORD nEntriesInUse;
  DWORD dwChunkId;
  DWORDLONG qwBaseOffset;
  DWORD dwReserved3;
  struct _avifieldindex_entry
  {
    DWORD dwOffset;
    DWORD dwSize;
    DWORD dwOffsetField2;
  } aIndex[];
} AVIFIELDINDEX, *PAVIFIELDINDEX;

#include <poppack.h>

#endif