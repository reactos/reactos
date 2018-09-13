//*****************************************************************************
//
// DC Cacheing - header file
//
//     Support for misbehaved apps - which continue to use a DC that has been
//     Released. Well the problem is WIN30 allows it, so we need to be
//     compatible.
//
//
// 03-Feb-92  NanduriR   Created.
//
//*****************************************************************************


typedef struct _DCCACHE{
    struct _DCCACHE FAR *lpNext;
    BYTE   flState;
    HAND16 htask16;
    HAND16 hwnd16;
    HDC16  hdc16;
    HWND   hwnd32;
} DCCACHE, FAR *LPDCCACHE;


extern INT  iReleasedDCs;

#define CACHENOTEMPTY() (BOOL)(iReleasedDCs)

#define DCCACHE_STATE_INUSE        0x0001
#define DCCACHE_STATE_RELPENDING   0x0002

#define SRCHDC_TASK16_HWND16 0x0001
#define SRCHDC_TASK16_HWND32 0x0002
#define SRCHDC_TASK16        0x0004

BOOL ReleaseCachedDCs(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16,
                            HWND hwnd32, UINT flSearch);
BOOL StoreDC(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16);
BOOL CacheReleasedDC(HAND16 htask16, HAND16 hwnd16, HAND16 hdc16);
BOOL FreeCachedDCs(HAND16 htask16);

