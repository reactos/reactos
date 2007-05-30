ifndef _DMDLS_
#define _DMDLS_

#include "dls1.h"

#define DMUS_DOWNLOADINFO_INSTRUMENT            1
#define DMUS_DOWNLOADINFO_WAVE                  2
#define DMUS_DOWNLOADINFO_INSTRUMENT2           3
#define DMUS_DOWNLOADINFO_WAVEARTICULATION      4
#define DMUS_DOWNLOADINFO_STREAMINGWAVE         5
#define DMUS_DOWNLOADINFO_ONESHOTWAVE           6
#define DMUS_DEFAULT_SIZE_OFFSETTABLE           1
#define DMUS_INSTRUMENT_GM_INSTRUMENT           (1 << 0)
#define DMUS_MIN_DATA_SIZE                      4

typedef long PCENT;
typedef long GCENT;
typedef long TCENT;
typedef long PERCENT;
typedef LONGLONG REFERENCE_TIME;
typedef REFERENCE_TIME *LPREFERENCE_TIME;

#ifndef MAKE_FOURCC
  #define MAKEFOURCC(ch0, ch1, ch2, ch3) ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
  typedef DWORD           FOURCC;
#endif


typedef struct _DMUS_LFOPARAMS
{
  PCENT pcFrequency;
  TCENT tcDelay;
  GCENT gcVolumeScale;
  PCENT pcPitchScale;
  GCENT gcMWToVolume;
  PCENT pcMWToPitch;
} DMUS_LFOPARAMS;

typedef struct _DMUS_VEGPARAMS
{
  TCENT tcAttack;
  TCENT tcDecay;
  PERCENT ptSustain;
  TCENT tcRelease;
  TCENT tcVel2Attack;
  TCENT tcKey2Decay;
} DMUS_VEGPARAMS;

typedef struct _DMUS_PEGPARAMS
{
  TCENT tcAttack;
  TCENT tcDecay;
  PERCENT ptSustain;
  TCENT tcRelease;
  TCENT tcVel2Attack;
  TCENT tcKey2Decay;
  PCENT pcRange;
} DMUS_PEGPARAMS;

typedef struct _DMUS_MSCPARAMS
{
  PERCENT ptDefaultPan;
} DMUS_MSCPARAMS;

typedef struct _DMUS_DOWNLOADINFO
{
  DWORD dwDLType;
  DWORD dwDLId;
  DWORD dwNumOffsetTableEntries;
  DWORD cbSize;
} DMUS_DOWNLOADINFO;

typedef struct _DMUS_OFFSETTABLE
{
  ULONG ulOffsetTable[DMUS_DEFAULT_SIZE_OFFSETTABLE];
} DMUS_OFFSETTABLE;

typedef struct _DMUS_INSTRUMENT
{
  ULONG ulPatch;
  ULONG ulFirstRegionIdx;
  ULONG ulGlobalArtIdx;
  ULONG ulFirstExtCkIdx;
  ULONG ulCopyrightIdx;
  ULONG ulFlags;
} DMUS_INSTRUMENT;

typedef struct _DMUS_REGION
{
  RGNRANGE RangeKey;
  RGNRANGE RangeVelocity;
  USHORT fusOptions;
  USHORT usKeyGroup;
  ULONG ulRegionArtIdx;
  ULONG ulNextRegionIdx;
  ULONG ulFirstExtCkIdx;
  WAVELINK WaveLink;
  WSMPL WSMP;
  WLOOP WLOOP[1];
} DMUS_REGION;

typedef struct _DMUS_NOTERANGE
{
  DWORD dwLowNote;
  DWORD dwHighNote;
} DMUS_NOTERANGE, *LPDMUS_NOTERANGE;

typedef struct _DMUS_COPYRIGHT
{
    ULONG cbSize;
    BYTE byCopyright[DMUS_MIN_DATA_SIZE];
} DMUS_COPYRIGHT;


typedef struct _DMUS_EXTENSIONCHUNK
{
  ULONG cbSize;
  ULONG ulNextExtCkIdx;
  FOURCC ExtCkID;
  BYTE byExtCk[DMUS_MIN_DATA_SIZE];
} DMUS_EXTENSIONCHUNK;


typedef struct _DMUS_WAVE
{
  ULONG ulFirstExtCkIdx;
  ULONG ulCopyrightIdx;
  ULONG ulWaveDataIdx;
  WAVEFORMATEX WaveformatEx;
} DMUS_WAVE;

typedef struct _DMUS_WAVEDATA
{
  ULONG cbSize;
  BYTE byData[DMUS_MIN_DATA_SIZE]; 
} DMUS_WAVEDATA;

typedef struct _DMUS_ARTICULATION
{
  ULONG ulArt1Idx;
  ULONG ulFirstExtCkIdx;
} DMUS_ARTICULATION;

typedef struct _DMUS_ARTICULATION2
{
  ULONG ulArtIdx;
  ULONG ulFirstExtCkIdx;
  ULONG ulNextArtIdx;
} DMUS_ARTICULATION2;

typedef struct _DMUS_WAVEDL
{
  ULONG cbWaveData;
}   DMUS_WAVEDL, *LPDMUS_WAVEDL;

typedef struct _DMUS_ARTICPARAMS
{
  DMUS_LFOPARAMS LFO;
  DMUS_VEGPARAMS VolEG;
  DMUS_PEGPARAMS PitchEG;
  DMUS_MSCPARAMS Misc;
} DMUS_ARTICPARAMS;

typedef struct _DMUS_WAVEARTDL
{
  ULONG ulDownloadIdIdx;
  ULONG ulBus;
  ULONG ulBuffers;
    ULONG ulMasterDLId;
  USHORT usOptions;
} DMUS_WAVEARTDL, *LPDMUS_WAVEARTDL;


#endif

