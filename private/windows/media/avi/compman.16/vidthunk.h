/****************************************************************************
    vidthunk.h

    Contains definitions for msvideo thunks (16/32 bit)

    Copyright (c) Microsoft Corporation 1994. All rights reserved

****************************************************************************/

//
// NOTE - 32bit handles have 0x8000 'or'ed in - this makes a BIG ASSUMPTION
// about how handles are generated on the 32-bit side.  We ASSUME here
// that :
//
//    32bit msvideo.dll always uses OpenDriver to create handles
//
//    The OpenDriver returns indices into its table (ie small positive
//    numbers).
//

#define  Is32bitHandle(h) (((h) & 0x8000) != 0)
#define  Make32bitHandle(h) ((h) | 0x8000)
#define  Map32bitHandle(h) ((h) & 0x7FFF)

#ifdef WIN32
#include <wownt32.h>
//
//  Thunking support
//

#define GET_VDM_POINTER_NAME            "WOWGetVDMPointer"
#define GET_HANDLE_MAPPER16             "WOWHandle16"
#define GET_HANDLE_MAPPER32             "WOWHandle32"
#define GET_CALLBACK16                  "WOWCallback16"
#define GET_MAPPING_MODULE_NAME         TEXT("wow32.dll")

typedef LPVOID (APIENTRY *LPGETVDMPOINTER)( DWORD Address, DWORD dwBytes, BOOL fProtectMode );
#define WOW32ResolveMemory( p ) (LPVOID)(GetVdmPointer( (DWORD)(p), 0, TRUE ))

typedef HANDLE  (APIENTRY *LPWOWHANDLE32)(WORD, WOW_HANDLE_TYPE);
typedef WORD    (APIENTRY *LPWOWHANDLE16)(HANDLE, WOW_HANDLE_TYPE);
typedef DWORD   (APIENTRY *LPWOWCALLBACK16)(DWORD vpfn16, DWORD dwParam);

#define StartThunk(Function)                           \
          DWORD ReturnCode = 0;                        \
          DPF2(("Entering function %s", #Function));

#define EndThunk()                                     \
          DPF2(("Returned %4X :%4X",                   \
                   HIWORD(ReturnCode),                 \
                   LOWORD(ReturnCode)));               \
          return ReturnCode;

#define ThunkHWND(h16) ((HWND)lpWOWHandle32(WOW_TYPE_HWND, h16))
#define ThunkHDC(h16)  ((HDC) lpWOWHandle32(WOW_TYPE_HDC , h16))
#define ThunkHPAL(h16) ((HPALETTE)lpWOWHandle32(WOW_TYPE_HPALETTE, h16))

#endif // WIN32

/*
 *  Useful structures and mapping
 */

typedef struct {
    short left, top, right, bottom;
} RECT_SHORT;


#define SHORT_RECT_TO_RECT(OutRect, InRect)  \
    OutRect.left = (LONG)InRect.left;        \
    OutRect.top = (LONG)InRect.top;          \
    OutRect.right = (LONG)InRect.right;      \
    OutRect.bottom = (LONG)InRect.bottom;

#define RECT_TO_SHORT_RECT(OutRect, InRect)  \
    OutRect.left = (short)InRect.left;       \
    OutRect.top = (short)InRect.top;         \
    OutRect.right = (short)InRect.right;     \
    OutRect.bottom = (short)InRect.bottom;


//
//  Function ids across the thunking layer (used by 32 and 16 bit)
//
enum {
   vidThunkvideoMessage32=1,
   vidThunkvideoGetNumDevs32,
   vidThunkvideoOpen32,
   vidThunkvideoClose32,

   compThunkICInfo32,
   compThunkICSendMessage32,
   compThunkICOpen32,
   compThunkICClose32
};

#ifndef WIN32
typedef struct _VIDTHUNK
{
//
//  Thunking stuff
//
    DWORD           (FAR PASCAL *lpfnCallproc32W)(DWORD, DWORD, DWORD,
                                                  DWORD, DWORD,
                                                  LPVOID, DWORD, DWORD);
    LPVOID          lpVideoThunkEntry;
    DWORD           dwVideo32Handle;
    LPVOID          lpICMThunkEntry;
    DWORD           dwICM32Handle;


} VIDTHUNK, *PVIDTHUNK, FAR *LPVIDTHUNK;
#endif // !WIN32

//
// The following functions generate calls to the 32-bit side
//

#ifdef _INC_MSVIDEO

DWORD FAR PASCAL videoMessage32(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2);
DWORD FAR PASCAL videoGetNumDevs32(void);
DWORD FAR PASCAL videoClose32(HVIDEO hVideo);
DWORD FAR PASCAL videoOpen32(LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags);

#endif // _INC_MSVIDEO

#ifdef _INC_COMPMAN

BOOL FAR PASCAL ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicInfo);
LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2);
DWORD FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode);
LRESULT FAR PASCAL ICClose32(DWORD hic);

#endif // _INC_COMPMAN
