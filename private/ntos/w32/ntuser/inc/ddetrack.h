/****************************** Module Header ******************************\
* Module Name: ddetrack.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Stuff for dde tracking
*
* History:
* 9-3-91    sanfords    Created
\***************************************************************************/

typedef struct tagDDEPACK {
    UINT_PTR uiLo;
    UINT_PTR uiHi;
} DDEPACK, *PDDEPACK;

typedef struct tagDDE_DATA {    // useful for sanely manipulating DDE data
    WORD wStatus;
    WORD wFmt;
    ULONG_PTR Data;     // often cast to a HANDLE so keep 32 bits.
} DDE_DATA, *PDDE_DATA;

//
// This structure heads the single server side object used to hold DDE Data.
// Its complexity derives from the fact that we may need to copy huge and
// complex DDE data across the CSR barrier. (TYPE_DDEDATA object)
//
typedef struct tagINTDDEINFO {
    DDEPACK     DdePack;            // original dde pack struct
    DWORD       flags;              // XS_ flags describing the data
    HANDLE      hDirect;            // handle to direct DDE data
    PBYTE       pDirect;            // pointer to source buffer for direct data
    int         cbDirect;           // size of direct data total
    HANDLE      hIndirect;          // handle referenced by direct data
    PBYTE       pIndirect;          // pointer to source of indirect data - if being copied
    int         cbIndirect;         // amount of indirect data total
                                    // Directly following this struct is the
                                    // raw DDE data being copied between processes
} INTDDEINFO, *PINTDDEINFO;

// values for flags fields

#define XS_PACKED         0x0001  // this transaction has a packed lParam
#define XS_DATA           0x0002  // this transaction has data w/status-format info.
#define XS_METAFILEPICT   0x0004  // the data in this transaction has a METAFILEPICT
#define XS_BITMAP         0x0008  // the data in this transaction has a HBITMAP
#define XS_DIB            0x0010  // the data in this transaction has a DIB
#define XS_ENHMETAFILE    0x0020  // the data in this transaction has a HMF
#define XS_PALETTE        0x0040  // the data in this transaction has a HPALETTE
#define XS_LOHANDLE       0x0080  // the uiLo part has the data handle
#define XS_HIHANDLE       0x0100  // the uiHi part has the data handle
#define XS_FREEPXS        0x0200  // DDETrackGetMessageHook() should free pxs.
#define XS_FRELEASE       0x0400  // DDE_FRELEASE bit was set in the data msg.
#define XS_EXECUTE        0x0800  // execute data handle
#define XS_FREESRC        0x1000  // free source after copy.
#define XS_PUBLICOBJ      0x2000  // object being shared is public - cleanup if needed.
#define XS_GIVEBACKONNACK 0x4000  // object was given and may need to be returned.
#define XS_DUMPMSG        0x8000  // used for backing out PostMessages.
#define XS_UNICODE       0x10000  // execute string is expected to be UNICODE

#define FAIL_POST       0       // return values from DDETrackPostHook()
#define FAKE_POST       1
#define DO_POST         2
#define FAILNOFREE_POST 3

