#ifndef _INC_DLS2
#define _INC_DLS2

DEFINE_GUID(DLSID_GMInHardware,       0x178F2F24, 0xC364, 0x11D1, 0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(DLSID_GSInHardware,       0x178F2F25, 0xC364, 0x11D1, 0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(DLSID_XGInHardware,       0x178F2F26, 0xC364, 0x11D1, 0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(DLSID_SupportsDLS1,       0x178F2F27, 0xC364, 0x11D1, 0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(DLSID_SupportsDLS2,       0xF14599E5, 0x4689, 0x11D2, 0xAF, 0xA6, 0x00, 0xAA, 0x00, 0x24, 0xd8, 0xB6);
DEFINE_GUID(DLSID_SampleMemorySize,   0x178F2F28, 0xC364, 0x11D1, 0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(DLSID_ManufacturersID,    0xB03E1181, 0x8095, 0x11D2, 0xA1, 0xEF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8);
DEFINE_GUID(DLSID_ProductID,          0xB03E1182, 0x8095, 0x11D2, 0xA1, 0xEF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8);
DEFINE_GUID(DLSID_SamplePlaybackRate, 0x2A91F713, 0xA4BF, 0x11D2, 0xBB, 0xDF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8);

#define CONN_SRC_POLYPRESSURE       0x0007
#define CONN_SRC_CHANNELPRESSURE    0x0008
#define CONN_SRC_VIBRATO            0x0009
#define CONN_SRC_MONOPRESSURE       0x000A
#define CONN_SRC_CC91               0x00DB
#define CONN_SRC_CC93               0x00DD

#define CONN_DST_GAIN               0x0001
#define CONN_DST_KEYNUMBER          0x0005
#define CONN_DST_LEFT               0x0010
#define CONN_DST_RIGHT              0x0011
#define CONN_DST_CENTER             0x0012
#define CONN_DST_LEFTREAR           0x0013
#define CONN_DST_RIGHTREAR          0x0014
#define CONN_DST_LFE_CHANNEL        0x0015
#define CONN_DST_CHORUS             0x0080
#define CONN_DST_REVERB             0x0081
#define CONN_DST_VIB_FREQUENCY      0x0114
#define CONN_DST_VIB_STARTDELAY     0x0115
#define CONN_DST_EG1_DELAYTIME      0x020B
#define CONN_DST_EG1_HOLDTIME       0x020C
#define CONN_DST_EG1_SHUTDOWNTIME   0x020D
#define CONN_DST_EG2_DELAYTIME      0x030F
#define CONN_DST_EG2_HOLDTIME       0x0310
#define CONN_DST_FILTER_CUTOFF      0x0500
#define CONN_DST_FILTER_Q           0x0501

#define DLS_CDL_AND                 0x0001
#define DLS_CDL_OR                  0x0002
#define DLS_CDL_XOR                 0x0003
#define DLS_CDL_ADD                 0x0004
#define DLS_CDL_SUBTRACT            0x0005
#define DLS_CDL_MULTIPLY            0x0006
#define DLS_CDL_DIVIDE              0x0007
#define DLS_CDL_LOGICAL_AND         0x0008
#define DLS_CDL_LOGICAL_OR          0x0009
#define DLS_CDL_LT                  0x000A
#define DLS_CDL_LE                  0x000B
#define DLS_CDL_GT                  0x000C
#define DLS_CDL_GE                  0x000D
#define DLS_CDL_EQ                  0x000E
#define DLS_CDL_NOT                 0x000F
#define DLS_CDL_CONST               0x0010
#define DLS_CDL_QUERY               0x0011
#define DLS_CDL_QUERYSUPPORTED      0x0012

#define CONN_TRN_CONVEX             0x0002
#define CONN_TRN_SWITCH             0x0003

#define FOURCC_RGN2                 mmioFOURCC('r','g','n','2')
#define FOURCC_LAR2                 mmioFOURCC('l','a','r','2')
#define FOURCC_ART2                 mmioFOURCC('a','r','t','2')
#define FOURCC_CDL                  mmioFOURCC('c','d','l',' ')
#define FOURCC_DLID                 mmioFOURCC('d','l','i','d')

#define F_WAVELINK_MULTICHANNEL     0x0002
#define WLOOP_TYPE_RELEASE          1

#endif

