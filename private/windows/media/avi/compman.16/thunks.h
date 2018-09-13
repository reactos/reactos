/****************************************************************************
    thunks.h

    Contains definitions for thunking msvideo.dll (16bit) to the 32bit
    msvfw32.dll running on NT.

    Copyright (c) Microsoft Corporation 1994. All rights reserved

****************************************************************************/

//
// NOTE - 32bit handles have 0x8000 'or'ed in - this makes a BIG ASSUMPTION
// about how handles are generated on the 32-bit side.  We ASSUME here
// that :
//
//    msvfw32.dll always uses OpenDriver to create handles
//
//    The OpenDriver returns indices into its table (ie small positive
//    numbers).
//

#define  Is32bitHandle(h) (((h) & 0x8000) != 0)
#define  Make32bitHandle(h) ((h) | 0x8000)
#define  Map32bitHandle(h) ((h) & 0x7FFF)


//
// Functions to link and unlink to 32-bit side

BOOL _loadds FAR InitThunks(void);

//
// The following functions generate calls to the 32-bit side
//

#ifdef _INC_MSVIDEO

//
// The prototypes for setting up thunks for the video api set (in AVICAP32)
//

DWORD FAR PASCAL videoMessage32(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2);
DWORD FAR PASCAL videoGetNumDevs32(void);
DWORD FAR PASCAL videoClose32(HVIDEO hVideo);
DWORD FAR PASCAL videoOpen32(LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags);

#endif // _INC_MSVIDEO

#ifdef _INC_COMPMAN

//
// The prototypes for setting up thunks for the ICM_ api set (in MSVFW32)
//

BOOL FAR PASCAL ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicInfo);
LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2);
DWORD FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode);
LRESULT FAR PASCAL ICClose32(DWORD hic);

#endif // _INC_COMPMAN

