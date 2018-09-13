//+---------------------------------------------------------------------
//
//   File:       headers.hxx
//
//------------------------------------------------------------------------

#include <stdlib.h>
#include <io.h>         // for _filelength and _chsize in fatstg

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>

#include <ole2.h>

#define  INC_STATUS_BAR
#include <o2base.hxx>

//
// BUGBUG - this should go away when HRESULT_FROM_WIN32 macro becomes
// available.
//
#ifndef HRESULT_FROM_WIN32
#define HRESULT_FROM_WIN32(error)  \
    (error)
#endif



#if DBG
#   define DOUT( p )   OutputDebugString( p )
#else
#   define DOUT( p )   /* nothing */
#endif
