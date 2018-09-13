#ifndef __MEDIA_H__
#define __MEDIA_H__

#define MAX_DESCRIPTOR  256

#include <mmsystem.h>
#include <vfw.h>

//-------------------------------------------------------------------------//
//  FMTID_AudioSummaryInformation property identifiers
#define PIDASI_FORMAT           0x00000002 // VT_BSTR
#define PIDASI_TIMELENGTH       0x00000003 // VT_UI4, milliseconds
#define PIDASI_AVG_DATA_RATE    0x00000004 // VT_UI4,  Hz
#define PIDASI_SAMPLE_RATE      0x00000005 // VT_UI4,  bits
#define PIDASI_SAMPLE_SIZE      0x00000006 // VT_UI4,  bits
#define PIDASI_CHANNEL_COUNT    0x00000007 // VT_UI4

//  FMTID_VideoSummaryInformation property identifiers
#define PIDVSI_STREAM_NAME      0x00000002 // "StreamName", VT_BSTR
#define PIDVSI_FRAME_WIDTH      0x00000003 // "FrameWidth", VT_UI4
#define PIDVSI_FRAME_HEIGHT     0x00000004 // "FrameHeight", VT_UI4
#define PIDVSI_TIMELENGTH       0x00000007 // "TimeLength", VT_UI4, milliseconds
#define PIDVSI_FRAME_COUNT      0x00000005 // "FrameCount". VT_UI4
#define PIDVSI_FRAME_RATE       0x00000006 // "FrameRate", VT_UI4, frames/millisecond
#define PIDVSI_DATA_RATE        0x00000008 // "DataRate", VT_UI4, bytes/second
#define PIDVSI_SAMPLE_SIZE      0x00000009 // "SampleSize", VT_UI4
#define PIDVSI_COMPRESSION      0x0000000A // "Compression", VT_BSTR

//  FMT


//-------------------------------------------------------------------------//
typedef struct tagWaveDesc 
{
    DWORD           dwSize;
    LONG            nLength;   // milliseconds
    TCHAR           szWaveFormat[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    PWAVEFORMATEX   pwfx ;
}   WAVEDESC,* PWAVEDESC;

typedef struct tagAviDesc
{
    DWORD   dwSize;
    LONG    nLength;     // milliseconds
    LONG    nWidth;      // pixels
    LONG    nHeight;     // pixels
    LONG    nBitDepth;   
    LONG    cFrames ;  
    LONG    nFrameRate;  // frames/1000 seconds
    LONG    nDataRate;   // bytes/second
    TCHAR   szCompression[MAX_DESCRIPTOR] ;
    TCHAR   szStreamName[MAX_DESCRIPTOR] ;
    TCHAR   szWaveFormat[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    PWAVEFORMATEX  pwfx ;
} AVIDESC, *PAVIDESC ;

typedef struct tagMidiInfo
{
    DWORD   dwSize;
    LONG    nLength;
    TCHAR   szMidiCopyright[MAX_DESCRIPTOR] ;
    TCHAR   szMidiSequenceName[MAX_DESCRIPTOR] ;
} MIDIDESC, *PMIDIDESC ;

//-------------------------------------------------------------------------//
STDMETHODIMP  GetWaveInfo( IN LPCTSTR pszFile, OUT PWAVEDESC p ) ;
STDMETHODIMP  FreeWaveInfo( IN OUT PWAVEDESC p ) ;
STDMETHODIMP  GetWaveProperty( IN REFFMTID reffmtid, IN PROPID propid, IN const WAVEDESC* pWave, OUT PROPVARIANT* pVar ) ;

//-------------------------------------------------------------------------//
STDMETHODIMP  GetMidiInfo( IN LPCTSTR pszFile, OUT PMIDIDESC p ) ;
STDMETHODIMP  FreeMidiInfo( IN OUT PMIDIDESC p ) ;
STDMETHODIMP  GetMidiProperty( IN REFFMTID reffmtid, IN PROPID propid, IN const MIDIDESC* pMidi, OUT PROPVARIANT* pVar ) ;

//-------------------------------------------------------------------------//
STDMETHODIMP  GetAviInfo( IN LPCTSTR pszFile, OUT PAVIDESC p ) ;
STDMETHODIMP  FreeAviInfo( IN OUT PAVIDESC p ) ;
STDMETHODIMP  GetAviProperty( IN REFFMTID reffmtid, IN PROPID propid, IN const AVIDESC* pAvi, OUT PROPVARIANT* pVar ) ;



#endif  __MEDIA_H__


