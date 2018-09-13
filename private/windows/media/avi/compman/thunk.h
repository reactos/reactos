/****************************************************************************
    thunk.h

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

//
//
//
#ifdef _WIN32

#ifdef CHICAGO
//
//  Following pasted from wownt32.h
//

//
// 16 <--> 32 Handle mapping functions.
//
// NOTE:  While some of these functions perform trivial
// conversions, these functions must be used to maintain
// compatibility with future versions of Windows NT which
// may require different handle mapping.
//

typedef enum _WOW_HANDLE_TYPE { /* WOW */
    WOW_TYPE_HWND,
    WOW_TYPE_HMENU,
    WOW_TYPE_HDWP,
    WOW_TYPE_HDROP,
    WOW_TYPE_HDC,
    WOW_TYPE_HFONT,
    WOW_TYPE_HMETAFILE,
    WOW_TYPE_HRGN,
    WOW_TYPE_HBITMAP,
    WOW_TYPE_HBRUSH,
    WOW_TYPE_HPALETTE,
    WOW_TYPE_HPEN,
    WOW_TYPE_HACCEL,
    WOW_TYPE_HTASK,
    WOW_TYPE_FULLHWND
} WOW_HANDLE_TYPE;

#define ThunkHWND(h16) ((HWND)h16)
#define ThunkHDC(h16)  ((HDC)h16)
#define ThunkHPAL(h16) ((HPALETTE)h16)

#else

#include <wownt32.h>

#define ThunkHWND(h16) ((HWND)lpWOWHandle32((WORD)h16, WOW_TYPE_HWND))
#define ThunkHDC(h16)  ((HDC) lpWOWHandle32((WORD)h16, WOW_TYPE_HDC))
#define ThunkHPAL(h16) ((HPALETTE)lpWOWHandle32((WORD)h16, WOW_TYPE_HPALETTE))

#endif	// !CHICAGO

//
//  Thunking support
//

#define GET_VDM_POINTER_NAME            "WOWGetVDMPointer"
#define GET_HANDLE_MAPPER16             "WOWHandle16"
#define GET_HANDLE_MAPPER32             "WOWHandle32"
#define GET_CALLBACK16                  "WOWCallback16"
#define GET_MAPPING_MODULE_NAME         TEXT("wow32.dll")

typedef LPVOID (APIENTRY *LPGETVDMPOINTER)( DWORD Address, DWORD dwBytes, BOOL fProtectMode );
typedef HANDLE (APIENTRY *LPWOWHANDLE32)(WORD, WOW_HANDLE_TYPE);
typedef WORD   (APIENTRY *LPWOWHANDLE16)(HANDLE, WOW_HANDLE_TYPE);
typedef DWORD  (APIENTRY *LPWOWCALLBACK16)(DWORD vpfn16, DWORD dwParam);



#define StartThunk(Function)                           \
          LRESULT  ReturnCode = 0;                        \
	  DPFS(dbgThunks, 2, "Entering function %s", #Function);

#define EndThunk()                                     \
          DPFS(dbgThunks, 2, "Returned %4X :%4X",      \
                   HIWORD(ReturnCode),                 \
                   LOWORD(ReturnCode));                \
          return ReturnCode;

#endif // _WIN32

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

   compThunkICInfoInternal32,
   compThunkICSendMessage32,
   compThunkICOpen32,
   compThunkICClose32,
   compThunkICOpenFunction32,
   compThunkICSetStatusProc32
};

#ifndef _WIN32
typedef struct _VIDTHUNK
{
//
//  Thunking stuff
//
    DWORD           (FAR PASCAL *lpfnCallproc32W)(DWORD, DWORD, DWORD,
                                                  DWORD, DWORD,
                                                  LPVOID, DWORD, DWORD);
    LPVOID          lpvThunkEntry;
    DWORD           dwVideo32Handle;


} VIDTHUNK, *PVIDTHUNK, FAR *LPVIDTHUNK;
#endif // !_WIN32

//
//  Some typedefs to assist with the ICM_SET_STATUS_PROC
//  thunk and callback thunk.
//
//  Convention:
//	"S" suffix indicates a 16:16 ptr (Segmented)
//	"L" suffix indicates a  0:32 ptr (Linear)
//	 no suffix indicates a native bitness pointer
//
typedef LONG (CALLBACK *ICSTATUSPROC)(LPARAM lParam, UINT uMsg, LONG l);
typedef struct tICSTATUSTHUNKDESC FAR * LPICSTATUSTHUNKDESC;


#ifdef _WIN32
typedef DWORD			LPVOIDS;
typedef LPICSTATUSTHUNKDESC	LPICSTATUSTHUNKDESCL;
typedef DWORD			LPICSTATUSTHUNKDESCS;
typedef ICSTATUSPROC		ICSTATUSPROCL;
typedef DWORD			ICSTATUSPROCS;

#else
typedef LPVOID			LPVOIDS;
typedef DWORD			LPICSTATUSTHUNKDESCL;
typedef LPICSTATUSTHUNKDESC	LPICSTATUSTHUNKDESCS;
typedef DWORD			ICSTATUSPROCL;
typedef ICSTATUSPROC		ICSTATUSPROCS;
#endif

typedef struct tICSTATUSTHUNKDESC {
    //
    //	segmented ptr to this struct
    //
    LPICSTATUSTHUNKDESCS    lpstdS;

    //
    //	segmented ptr to 16 bit callback stub
    //
    LPVOIDS		    pfnthkStatusProc16S;

    //
    //	from client's ICSETSTATUSPROC
    //
    DWORD		    dwFlags;
    LPARAM		    lParam;
    ICSTATUSPROCS	    fnStatusProcS;

    //
    //	to be sent to client's callback
    //
    DWORD		    uMsg;
    LONG		    l;
} ICSTATUSTHUNKDESC;


//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

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

#ifdef _WIN32
LRESULT FAR PASCAL ICInfoInternal32(DWORD fccType, DWORD fccHandler, ICINFO16 FAR * lpicinfo, ICINFOI FAR * lpicinfoi);
#else
BOOL    FAR PASCAL ICInfoInternal32(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo, ICINFOI FAR * lpicinfoi);
#endif
LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2);
LRESULT FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode);
LRESULT FAR PASCAL ICOpenFunction32(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler);
LRESULT FAR PASCAL ICClose32(DWORD hic);
LRESULT FAR PASCAL ICSendSetStatusProc32(HIC hic, ICSETSTATUSPROC FAR* lpissp, DWORD cbStruct);

#endif // _INC_COMPMAN

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
