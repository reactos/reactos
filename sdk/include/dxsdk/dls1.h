

#ifndef _INC_DLS1
#define _INC_DLS1

#define CONN_SRC_NONE                   0x0000
#define CONN_SRC_LFO                    0x0001
#define CONN_SRC_KEYONVELOCITY          0x0002
#define CONN_SRC_KEYNUMBER              0x0003
#define CONN_SRC_EG1                    0x0004
#define CONN_SRC_EG2                    0x0005
#define CONN_SRC_PITCHWHEEL             0x0006
#define CONN_SRC_CC1                    0x0081
#define CONN_SRC_CC7                    0x0087
#define CONN_SRC_CC10                   0x008a
#define CONN_SRC_CC11                   0x008b
#define CONN_DST_NONE                   0x0000
#define CONN_DST_ATTENUATION            0x0001
#define CONN_DST_PITCH                  0x0003
#define CONN_DST_PAN                    0x0004
#define CONN_DST_LFO_FREQUENCY          0x0104
#define CONN_DST_LFO_STARTDELAY         0x0105
#define CONN_DST_EG1_ATTACKTIME         0x0206
#define CONN_DST_EG1_DECAYTIME          0x0207
#define CONN_DST_EG1_RELEASETIME        0x0209
#define CONN_DST_EG1_SUSTAINLEVEL       0x020a
#define CONN_DST_EG2_ATTACKTIME         0x030a
#define CONN_DST_EG2_DECAYTIME          0x030b
#define CONN_DST_EG2_RELEASETIME        0x030d
#define CONN_DST_EG2_SUSTAINLEVEL       0x030e
#define CONN_TRN_NONE                   0x0000
#define CONN_TRN_CONCAVE                0x0001
#define FOURCC_DLS                      mmioFOURCC('D','L','S',' ')
#define FOURCC_DLID                     mmioFOURCC('d','l','i','d')
#define FOURCC_COLH                     mmioFOURCC('c','o','l','h')
#define FOURCC_WVPL                     mmioFOURCC('w','v','p','l')
#define FOURCC_PTBL                     mmioFOURCC('p','t','b','l')
#define FOURCC_PATH                     mmioFOURCC('p','a','t','h')
#define FOURCC_wave                     mmioFOURCC('w','a','v','e')
#define FOURCC_LINS                     mmioFOURCC('l','i','n','s')
#define FOURCC_INS                      mmioFOURCC('i','n','s',' ')
#define FOURCC_INSH                     mmioFOURCC('i','n','s','h')
#define FOURCC_LRGN                     mmioFOURCC('l','r','g','n')
#define FOURCC_RGN                      mmioFOURCC('r','g','n',' ')
#define FOURCC_RGNH                     mmioFOURCC('r','g','n','h')
#define FOURCC_LART                     mmioFOURCC('l','a','r','t')
#define FOURCC_ART1                     mmioFOURCC('a','r','t','1')
#define FOURCC_WLNK                     mmioFOURCC('w','l','n','k')
#define FOURCC_WSMP                     mmioFOURCC('w','s','m','p')
#define FOURCC_VERS                     mmioFOURCC('v','e','r','s')
#define F_INSTRUMENT_DRUMS              0x80000000
#define F_RGN_OPTION_SELFNONEXCLUSIVE   0x0001
#define WAVELINK_CHANNEL_LEFT           0x0001
#define WAVELINK_CHANNEL_RIGHT          0x0002
#define F_WAVELINK_PHASE_MASTER         0x0001
#define POOL_CUE_NULL                   0xffffffff
#define F_WSMP_NO_TRUNCATION            0x0001l
#define F_WSMP_NO_COMPRESSION           0x0002l
#define WLOOP_TYPE_FORWARD              0

typedef struct _CONNECTION
{
  USHORT usSource;
  USHORT usControl;
  USHORT usDestination;
  USHORT usTransform;
  LONG lScale;
}CONNECTION, *LPCONNECTION;

typedef struct _CONNECTIONLIST
{
  ULONG cbSize;
  ULONG cConnections;
} CONNECTIONLIST, *LPCONNECTIONLIST;

typedef struct _DLSVERSION
{
  DWORD dwVersionMS;
  DWORD dwVersionLS;
} DLSVERSION, *LPDLSVERSION;

typedef struct _DLSHEADER
{
  ULONG cInstruments;
}DLSHEADER, *LPDLSHEADER;

typedef struct _DLSID
{
  ULONG ulData1;
  USHORT usData2;
  USHORT usData3;
  BYTE abData4[8];
} DLSID, FAR *LPDLSID;

typedef struct _MIDILOCALE {
  ULONG ulBank;
  ULONG ulInstrument;
} MIDILOCALE, *LPMIDILOCALE;

typedef struct _INSTHEADER
{
  ULONG cRegions;
  MIDILOCALE Locale;
}INSTHEADER, *LPINSTHEADER;

typedef struct _POOLCUE
{
  ULONG    ulOffset;
}POOLCUE, *LPPOOLCUE;

typedef struct _POOLTABLE
{
  ULONG    cbSize;
  ULONG    cCues;
} POOLTABLE, FAR *LPPOOLTABLE;

typedef struct _RGNRANGE
{
  USHORT usLow;
  USHORT usHigh;
} RGNRANGE, *LPRGNRANGE;

typedef struct _RGNHEADER
{
  RGNRANGE RangeKey;
  RGNRANGE RangeVelocity;
  USHORT fusOptions;
  USHORT usKeyGroup;
}RGNHEADER, *LPRGNHEADER;

typedef struct _rloop
{
  ULONG cbSize;
  ULONG ulType;
  ULONG ulStart;
  ULONG ulLength;
} WLOOP, *LPWLOOP;

typedef struct _rwsmp
{
  ULONG cbSize;
  USHORT usUnityNote;
  SHORT sFineTune;
  LONG lAttenuation;
  ULONG fulOptions;
  ULONG cSampleLoops;
} WSMPL, *LPWSMPL;

typedef struct _WAVELINK
{
  USHORT fusOptions;
  USHORT usPhaseGroup;
  ULONG ulChannel;
  ULONG ulTableIndex;
}WAVELINK, *LPWAVELINK;


#endif
