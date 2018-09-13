/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Jul 12 18:09:25 1996
 */
/* Compiler settings for wtypes.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __wtypes_h__
#define __wtypes_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Jul 12 18:09:25 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//--------------------------------------------------------------------------


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IWinTypes_INTERFACE_DEFINED__
#define __IWinTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWinTypes
 * at Fri Jul 12 18:09:25 1996
 * using MIDL 3.00.44
 ****************************************/
/* [auto_handle][unique][version][uuid] */ 


typedef struct  tagRemHGLOBAL
    {
    long fNullHGlobal;
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHGLOBAL;

typedef struct  tagRemHMETAFILEPICT
    {
    long mm;
    long xExt;
    long yExt;
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHMETAFILEPICT;

typedef struct  tagRemHENHMETAFILE
    {
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHENHMETAFILE;

typedef struct  tagRemHBITMAP
    {
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHBITMAP;

typedef struct  tagRemHPALETTE
    {
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHPALETTE;

typedef struct  tagRemBRUSH
    {
    unsigned long cbData;
    /* [size_is] */ byte data[ 1 ];
    }	RemHBRUSH;

#if !defined(_WIN32) && !defined(_MPPC_)
// The following code is for Win16 only
#ifndef WINAPI          // If not included with 3.1 headers...
#define FAR             _far
#define PASCAL          _pascal
#define CDECL           _cdecl
#define VOID            void
#define WINAPI      FAR PASCAL
#define CALLBACK    FAR PASCAL
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif // !FALSE
#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char BYTE;

#endif // !_BYTE_DEFINED
#ifndef _WORD_DEFINED
#define _WORD_DEFINED
typedef unsigned short WORD;

#endif // !_WORD_DEFINED
typedef unsigned int UINT;

typedef int INT;

typedef long BOOL;

#ifndef _LONG_DEFINED
#define _LONG_DEFINED
typedef long LONG;

#endif // !_LONG_DEFINED
#ifndef _WPARAM_DEFINED
#define _WPARAM_DEFINED
typedef UINT WPARAM;

#endif // _WPARAM_DEFINED
#ifndef _DWORD_DEFINED
#define _DWORD_DEFINED
typedef unsigned long DWORD;

#endif // !_DWORD_DEFINED
#ifndef _LPARAM_DEFINED
#define _LPARAM_DEFINED
typedef LONG LPARAM;

#endif // !_LPARAM_DEFINED
#ifndef _LRESULT_DEFINED
#define _LRESULT_DEFINED
typedef LONG LRESULT;

#endif // !_LRESULT_DEFINED
typedef void __RPC_FAR *HANDLE;

typedef void __RPC_FAR *HMODULE;

typedef void __RPC_FAR *HINSTANCE;

typedef void __RPC_FAR *HRGN;

typedef void __RPC_FAR *HTASK;

typedef void __RPC_FAR *HKEY;

typedef void __RPC_FAR *HDESK;

typedef void __RPC_FAR *HMF;

typedef void __RPC_FAR *HEMF;

typedef void __RPC_FAR *HPEN;

typedef void __RPC_FAR *HRSRC;

typedef void __RPC_FAR *HSTR;

typedef void __RPC_FAR *HWINSTA;

typedef void __RPC_FAR *HKL;

typedef void __RPC_FAR *HGDIOBJ;

typedef HANDLE HDWP;

#ifndef _HFILE_DEFINED
#define _HFILE_DEFINED
typedef INT HFILE;

#endif // !_HFILE_DEFINED
#ifndef _LPWORD_DEFINED
#define _LPWORD_DEFINED
typedef WORD __RPC_FAR *LPWORD;

#endif // !_LPWORD_DEFINED
#ifndef _LPDWORD_DEFINED
#define _LPDWORD_DEFINED
typedef DWORD __RPC_FAR *LPDWORD;

#endif // !_LPDWORD_DEFINED
typedef char CHAR;

typedef /* [string] */ CHAR __RPC_FAR *LPSTR;

typedef /* [string] */ const CHAR __RPC_FAR *LPCSTR;

#ifndef _WCHAR_DEFINED
#define _WCHAR_DEFINED
typedef wchar_t WCHAR;

typedef WCHAR TCHAR;

#endif // !_WCHAR_DEFINED
typedef /* [string] */ WCHAR __RPC_FAR *LPWSTR;

typedef /* [string] */ TCHAR __RPC_FAR *LPTSTR;

typedef /* [string] */ const WCHAR __RPC_FAR *LPCWSTR;

typedef /* [string] */ const TCHAR __RPC_FAR *LPCTSTR;

typedef struct  tagPALETTEENTRY
    {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
    }	PALETTEENTRY;

typedef struct tagPALETTEENTRY __RPC_FAR *PPALETTEENTRY;

typedef struct tagPALETTEENTRY __RPC_FAR *LPPALETTEENTRY;

#if 0
typedef struct  tagLOGPALETTE
    {
    WORD palVersion;
    WORD palNumEntries;
    /* [size_is] */ PALETTEENTRY palPalEntry[ 1 ];
    }	LOGPALETTE;

typedef struct tagLOGPALETTE __RPC_FAR *PLOGPALETTE;

typedef struct tagLOGPALETTE __RPC_FAR *LPLOGPALETTE;

#else
typedef struct tagLOGPALETTE {
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, *LPLOGPALETTE;
#endif
#ifndef _COLORREF_DEFINED
#define _COLORREF_DEFINED
typedef DWORD COLORREF;

#endif // !_COLORREF_DEFINED
#ifndef _LPCOLORREF_DEFINED
#define _LPCOLORREF_DEFINED
typedef DWORD __RPC_FAR *LPCOLORREF;

#endif // !_LPCOLORREF_DEFINED
typedef HANDLE __RPC_FAR *LPHANDLE;

typedef struct  _RECTL
    {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
    }	RECTL;

typedef struct _RECTL __RPC_FAR *PRECTL;

typedef struct _RECTL __RPC_FAR *LPRECTL;

typedef struct  tagPOINT
    {
    LONG x;
    LONG y;
    }	POINT;

typedef struct tagPOINT __RPC_FAR *PPOINT;

typedef struct tagPOINT __RPC_FAR *LPPOINT;

typedef struct  _POINTL
    {
    LONG x;
    LONG y;
    }	POINTL;

typedef struct _POINTL __RPC_FAR *PPOINTL;

#ifndef WIN16
typedef struct  tagSIZE
    {
    LONG cx;
    LONG cy;
    }	SIZE;

typedef struct tagSIZE __RPC_FAR *PSIZE;

typedef struct tagSIZE __RPC_FAR *LPSIZE;

#else // WIN16
typedef struct tagSIZE
{
    INT cx;
    INT cy;
} SIZE, *PSIZE, *LPSIZE;
#endif // WIN16
typedef struct  tagSIZEL
    {
    LONG cx;
    LONG cy;
    }	SIZEL;

typedef struct tagSIZEL __RPC_FAR *PSIZEL;

typedef struct tagSIZEL __RPC_FAR *LPSIZEL;

#endif  //WINAPI
#endif  //!WIN32 && !MPPC
#if defined(_WIN32) && !defined(OLE2ANSI)
typedef WCHAR OLECHAR;

typedef /* [string] */ OLECHAR __RPC_FAR *LPOLESTR;

typedef /* [string] */ const OLECHAR __RPC_FAR *LPCOLESTR;

#define OLESTR(str) L##str

#else

typedef char      OLECHAR;
typedef LPSTR     LPOLESTR;
typedef LPCSTR    LPCOLESTR;
#define OLESTR(str) str
#endif
#ifndef _WINDEF_
typedef const RECTL __RPC_FAR *LPCRECTL;

typedef void __RPC_FAR *PVOID;

typedef void __RPC_FAR *LPVOID;

typedef float FLOAT;

typedef struct  tagRECT
    {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
    }	RECT;

typedef struct tagRECT __RPC_FAR *PRECT;

typedef struct tagRECT __RPC_FAR *LPRECT;

typedef const RECT __RPC_FAR *LPCRECT;

#endif  //_WINDEF_
typedef unsigned char UCHAR;

typedef short SHORT;

typedef unsigned short USHORT;

typedef DWORD ULONG;

typedef double DOUBLE;

#ifndef _DWORDLONG_
typedef MIDL_uhyper DWORDLONG;

typedef DWORDLONG __RPC_FAR *PDWORDLONG;

#endif // !_DWORDLONG_
#ifndef _ULONGLONG_
typedef hyper LONGLONG;

typedef MIDL_uhyper ULONGLONG;

typedef LONGLONG __RPC_FAR *PLONGLONG;

typedef ULONGLONG __RPC_FAR *PULONGLONG;

#endif // _ULONGLONG_
#if 0
typedef struct  _LARGE_INTEGER
    {
    LONGLONG QuadPart;
    }	LARGE_INTEGER;

typedef LARGE_INTEGER __RPC_FAR *PLARGE_INTEGER;

typedef struct  _ULARGE_INTEGER
    {
    ULONGLONG QuadPart;
    }	ULARGE_INTEGER;

#endif // 0
#ifndef _WINBASE_
#ifndef _FILETIME_
#define _FILETIME_
typedef struct  _FILETIME
    {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
    }	FILETIME;

typedef struct _FILETIME __RPC_FAR *PFILETIME;

typedef struct _FILETIME __RPC_FAR *LPFILETIME;

#endif // !_FILETIME
#ifndef _SYSTEMTIME_
#define _SYSTEMTIME_
typedef struct  _SYSTEMTIME
    {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    }	SYSTEMTIME;

typedef struct _SYSTEMTIME __RPC_FAR *PSYSTEMTIME;

typedef struct _SYSTEMTIME __RPC_FAR *LPSYSTEMTIME;

#endif // !_SYSTEMTIME
#ifndef _SECURITY_ATTRIBUTES_
#define _SECURITY_ATTRIBUTES_
typedef struct  _SECURITY_ATTRIBUTES
    {
    DWORD nLength;
    /* [size_is] */ LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
    }	SECURITY_ATTRIBUTES;

typedef struct _SECURITY_ATTRIBUTES __RPC_FAR *PSECURITY_ATTRIBUTES;

typedef struct _SECURITY_ATTRIBUTES __RPC_FAR *LPSECURITY_ATTRIBUTES;

#endif // !_SECURITY_ATTRIBUTES_
#ifndef SECURITY_DESCRIPTOR_REVISION
typedef USHORT SECURITY_DESCRIPTOR_CONTROL;

typedef USHORT __RPC_FAR *PSECURITY_DESCRIPTOR_CONTROL;

typedef PVOID PSID;

typedef struct  _ACL
    {
    UCHAR AclRevision;
    UCHAR Sbz1;
    USHORT AclSize;
    USHORT AceCount;
    USHORT Sbz2;
    }	ACL;

typedef ACL __RPC_FAR *PACL;

typedef struct  _SECURITY_DESCRIPTOR
    {
    UCHAR Revision;
    UCHAR Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;
    }	SECURITY_DESCRIPTOR;

typedef struct _SECURITY_DESCRIPTOR __RPC_FAR *PISECURITY_DESCRIPTOR;

#endif // !SECURITY_DESCRIPTOR_REVISION
#endif //_WINBASE_
typedef struct  _COAUTHIDENTITY
    {
    /* [size_is] */ USHORT __RPC_FAR *User;
    ULONG UserLength;
    /* [size_is] */ USHORT __RPC_FAR *Domain;
    ULONG DomainLength;
    /* [size_is] */ USHORT __RPC_FAR *Password;
    ULONG PasswordLength;
    ULONG Flags;
    }	COAUTHIDENTITY;

typedef struct  _COAUTHINFO
    {
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    LPWSTR pwszServerPrincName;
    DWORD dwAuthnLevel;
    DWORD dwImpersonationLevel;
    COAUTHIDENTITY __RPC_FAR *pAuthIdentityData;
    DWORD dwCapabilities;
    }	COAUTHINFO;

typedef struct  _COSERVERINFO
    {
    DWORD dwReserved1;
    LPWSTR pwszName;
    COAUTHINFO __RPC_FAR *pAuthInfo;
    DWORD dwReserved2;
    }	COSERVERINFO;

typedef LONG SCODE;

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;

#endif // !_HRESULT_DEFINED
typedef SCODE __RPC_FAR *PSCODE;

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct  _GUID
    {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
    }	GUID;

#endif // !GUID_DEFINED
#if !defined( __LPGUID_DEFINED__ )
#define __LPGUID_DEFINED__
typedef GUID __RPC_FAR *LPGUID;

#endif // !__LPGUID_DEFINED__
#ifndef __OBJECTID_DEFINED
#define __OBJECTID_DEFINED
#define _OBJECTID_DEFINED
typedef struct  _OBJECTID
    {
    GUID Lineage;
    unsigned long Uniquifier;
    }	OBJECTID;

#endif // !_OBJECTID_DEFINED
#if !defined( __IID_DEFINED__ )
#define __IID_DEFINED__
typedef GUID IID;

typedef IID __RPC_FAR *LPIID;

#define IID_NULL            GUID_NULL
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
typedef GUID CLSID;

typedef CLSID __RPC_FAR *LPCLSID;

#define CLSID_NULL          GUID_NULL
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
typedef GUID FMTID;

typedef FMTID __RPC_FAR *LPFMTID;

#define FMTID_NULL          GUID_NULL
#define IsEqualFMTID(rfmtid1, rfmtid2) IsEqualGUID(rfmtid1, rfmtid2)
#if 0
typedef GUID __RPC_FAR *REFGUID;

typedef IID __RPC_FAR *REFIID;

typedef CLSID __RPC_FAR *REFCLSID;

typedef FMTID __RPC_FAR *REFFMTID;

#endif // 0
#if defined(__cplusplus)
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID &
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID &
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID &
#endif // !_REFCLSID_DEFINED
#ifndef _REFFMTID_DEFINED
#define _REFFMTID_DEFINED
#define REFFMTID            const FMTID &
#endif // !_REFFMTID_DEFINED
#else // !__cplusplus
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID * const
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID * const
#endif // !_REFCLSID_DEFINED
#ifndef _REFFMTID_DEFINED
#define _REFFMTID_DEFINED
#define REFFMTID            const FMTID * const
#endif // !_REFFMTID_DEFINED
#endif // !__cplusplus
#endif // !__IID_DEFINED__
typedef 
enum tagMEMCTX
    {	MEMCTX_TASK	= 1,
	MEMCTX_SHARED	= 2,
	MEMCTX_MACSYSTEM	= 3,
	MEMCTX_UNKNOWN	= -1,
	MEMCTX_SAME	= -2
    }	MEMCTX;

#ifndef _ROTFLAGS_DEFINED
#define _ROTFLAGS_DEFINED
#define ROTFLAGS_REGISTRATIONKEEPSALIVE 0x1
#define ROTFLAGS_ALLOWANYCLIENT 0x2
#endif // !_ROTFLAGS_DEFINED
#ifndef _ROT_COMPARE_MAX_DEFINED
#define _ROT_COMPARE_MAX_DEFINED
#define ROT_COMPARE_MAX 2048
#endif // !_ROT_COMPARE_MAX_DEFINED
typedef 
enum tagCLSCTX
    {	CLSCTX_INPROC_SERVER	= 0x1,
	CLSCTX_INPROC_HANDLER	= 0x2,
	CLSCTX_LOCAL_SERVER	= 0x4,
	CLSCTX_INPROC_SERVER16	= 0x8,
	CLSCTX_REMOTE_SERVER	= 0x10,
	CLSCTX_INPROC_HANDLER16	= 0x20,
	CLSCTX_INPROC_SERVERX86	= 0x40,
	CLSCTX_INPROC_HANDLERX86	= 0x80
    }	CLSCTX;

typedef 
enum tagMSHLFLAGS
    {	MSHLFLAGS_NORMAL	= 0,
	MSHLFLAGS_TABLESTRONG	= 1,
	MSHLFLAGS_TABLEWEAK	= 2,
	MSHLFLAGS_NOPING	= 4
    }	MSHLFLAGS;

typedef 
enum tagMSHCTX
    {	MSHCTX_LOCAL	= 0,
	MSHCTX_NOSHAREDMEM	= 1,
	MSHCTX_DIFFERENTMACHINE	= 2,
	MSHCTX_INPROC	= 3
    }	MSHCTX;

typedef 
enum tagDVASPECT
    {	DVASPECT_CONTENT	= 1,
	DVASPECT_THUMBNAIL	= 2,
	DVASPECT_ICON	= 4,
	DVASPECT_DOCPRINT	= 8
    }	DVASPECT;

typedef 
enum tagSTGC
    {	STGC_DEFAULT	= 0,
	STGC_OVERWRITE	= 1,
	STGC_ONLYIFCURRENT	= 2,
	STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE	= 4
    }	STGC;

typedef 
enum tagSTGMOVE
    {	STGMOVE_MOVE	= 0,
	STGMOVE_COPY	= 1,
	STGMOVE_SHALLOWCOPY	= 2
    }	STGMOVE;

typedef 
enum tagSTATFLAG
    {	STATFLAG_DEFAULT	= 0,
	STATFLAG_NONAME	= 1,
	STATFLAG_NOOPEN	= 2
    }	STATFLAG;

typedef /* [context_handle] */ void __RPC_FAR *HCONTEXT;

#ifndef _LCID_DEFINED
#define _LCID_DEFINED
typedef DWORD LCID;

#endif // !_LCID_DEFINED
typedef struct  _BYTE_BLOB
    {
    unsigned long clSize;
    /* [size_is] */ byte abData[ 1 ];
    }	BYTE_BLOB;

typedef /* [unique] */ BYTE_BLOB __RPC_FAR *UP_BYTE_BLOB;

typedef struct  _WORD_BLOB
    {
    unsigned long clSize;
    /* [size_is] */ unsigned short asData[ 1 ];
    }	WORD_BLOB;

typedef /* [unique] */ WORD_BLOB __RPC_FAR *UP_WORD_BLOB;

typedef struct  _DWORD_BLOB
    {
    unsigned long clSize;
    /* [size_is] */ unsigned long alData[ 1 ];
    }	DWORD_BLOB;

typedef /* [unique] */ DWORD_BLOB __RPC_FAR *UP_DWORD_BLOB;

typedef struct  _FLAGGED_BYTE_BLOB
    {
    unsigned long fFlags;
    unsigned long clSize;
    /* [size_is] */ byte abData[ 1 ];
    }	FLAGGED_BYTE_BLOB;

typedef /* [unique] */ FLAGGED_BYTE_BLOB __RPC_FAR *UP_FLAGGED_BYTE_BLOB;

typedef struct  _FLAGGED_WORD_BLOB
    {
    unsigned long fFlags;
    unsigned long clSize;
    /* [size_is] */ unsigned short asData[ 1 ];
    }	FLAGGED_WORD_BLOB;

typedef /* [unique] */ FLAGGED_WORD_BLOB __RPC_FAR *UP_FLAGGED_WORD_BLOB;

typedef struct  _BYTE_SIZEDARR
    {
    unsigned long clSize;
    /* [size_is] */ byte __RPC_FAR *pData;
    }	BYTE_SIZEDARR;

typedef struct  _SHORT_SIZEDARR
    {
    unsigned long clSize;
    /* [size_is] */ unsigned short __RPC_FAR *pData;
    }	WORD_SIZEDARR;

typedef struct  _LONG_SIZEDARR
    {
    unsigned long clSize;
    /* [size_is] */ unsigned long __RPC_FAR *pData;
    }	DWORD_SIZEDARR;

typedef struct  _HYPER_SIZEDARR
    {
    unsigned long clSize;
    /* [size_is] */ hyper __RPC_FAR *pData;
    }	HYPER_SIZEDARR;

#define	WDT_INPROC_CALL	( 0x48746457 )

#define	WDT_REMOTE_CALL	( 0x52746457 )

typedef struct  _userCLIPFORMAT
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0001
        {
        /* [case()] */ DWORD dwValue;
        /* [case()][string] */ wchar_t __RPC_FAR *pwszName;
        }	u;
    }	userCLIPFORMAT;

typedef /* [unique] */ userCLIPFORMAT __RPC_FAR *wireCLIPFORMAT;

typedef /* [wire_marshal] */ WORD CLIPFORMAT;

typedef struct  _GDI_NONREMOTE
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0002
        {
        /* [case()] */ long hInproc;
        /* [case()] */ DWORD_BLOB __RPC_FAR *hRemote;
        }	u;
    }	GDI_NONREMOTE;

typedef struct  _userHGLOBAL
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0003
        {
        /* [case()] */ long hInproc;
        /* [case()] */ FLAGGED_BYTE_BLOB __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHGLOBAL;

typedef /* [unique] */ userHGLOBAL __RPC_FAR *wireHGLOBAL;

typedef struct  _userHMETAFILE
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0004
        {
        /* [case()] */ long hInproc;
        /* [case()] */ BYTE_BLOB __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHMETAFILE;

typedef struct  _remoteMETAFILEPICT
    {
    long mm;
    long xExt;
    long yExt;
    userHMETAFILE __RPC_FAR *hMF;
    }	remoteMETAFILEPICT;

typedef struct  _userHMETAFILEPICT
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0005
        {
        /* [case()] */ long hInproc;
        /* [case()] */ remoteMETAFILEPICT __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHMETAFILEPICT;

typedef struct  _userHENHMETAFILE
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0006
        {
        /* [case()] */ long hInproc;
        /* [case()] */ BYTE_BLOB __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHENHMETAFILE;

typedef struct  _userBITMAP
    {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    ULONG cbSize;
    /* [size_is] */ byte pBuffer[ 1 ];
    }	userBITMAP;

typedef struct  _userHBITMAP
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0007
        {
        /* [case()] */ long hInproc;
        /* [case()] */ userBITMAP __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHBITMAP;

typedef struct  tagrpcLOGPALETTE
    {
    WORD palVersion;
    WORD palNumEntries;
    /* [size_is] */ PALETTEENTRY palPalEntry[ 1 ];
    }	rpcLOGPALETTE;

typedef struct  _userHPALETTE
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0008
        {
        /* [case()] */ long hInproc;
        /* [case()] */ rpcLOGPALETTE __RPC_FAR *hRemote;
        /* [default] */ long hGlobal;
        }	u;
    }	userHPALETTE;

typedef struct  _RemotableHandle
    {
    long fContext;
    /* [switch_is] */ /* [switch_type] */ union __MIDL_IWinTypes_0009
        {
        /* [case()] */ long hInproc;
        /* [case()] */ long hRemote;
        }	u;
    }	RemotableHandle;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHWND;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHMENU;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHACCEL;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHBRUSH;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHFONT;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHDC;

typedef /* [unique] */ RemotableHandle __RPC_FAR *wireHICON;

#if 0
typedef /* [wire_marshal] */ void __RPC_FAR *HWND;

typedef /* [wire_marshal] */ void __RPC_FAR *HMENU;

typedef /* [wire_marshal] */ void __RPC_FAR *HACCEL;

typedef /* [wire_marshal] */ void __RPC_FAR *HBRUSH;

typedef /* [wire_marshal] */ void __RPC_FAR *HFONT;

typedef /* [wire_marshal] */ void __RPC_FAR *HDC;

typedef /* [wire_marshal] */ void __RPC_FAR *HICON;

#ifndef _HCURSOR_DEFINED
#define _HCURSOR_DEFINED
typedef HICON HCURSOR;

#endif // !_HCURSOR_DEFINED
/* tagTEXTMETRICW was copied from wingdi.h for MIDL */
typedef struct  tagTEXTMETRICW
    {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    WCHAR tmFirstChar;
    WCHAR tmLastChar;
    WCHAR tmDefaultChar;
    WCHAR tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
    }	TEXTMETRICW;

#endif //0
#ifndef _WIN32           // The following code is for Win16 only
#ifndef WINAPI          // If not included with 3.1 headers...
typedef struct  tagMSG
    {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
    }	MSG;

typedef struct tagMSG __RPC_FAR *PMSG;

typedef struct tagMSG __RPC_FAR *NPMSG;

typedef struct tagMSG __RPC_FAR *LPMSG;

#endif // _WIN32
#endif // WINAPI
typedef /* [unique] */ userHBITMAP __RPC_FAR *wireHBITMAP;

typedef /* [unique] */ userHPALETTE __RPC_FAR *wireHPALETTE;

typedef /* [unique] */ userHENHMETAFILE __RPC_FAR *wireHENHMETAFILE;

typedef /* [unique] */ userHMETAFILE __RPC_FAR *wireHMETAFILE;

typedef /* [unique] */ userHMETAFILEPICT __RPC_FAR *wireHMETAFILEPICT;

#if 0
typedef /* [wire_marshal] */ void __RPC_FAR *HGLOBAL;

typedef HGLOBAL HLOCAL;

typedef /* [wire_marshal] */ void __RPC_FAR *HBITMAP;

typedef /* [wire_marshal] */ void __RPC_FAR *HPALETTE;

typedef /* [wire_marshal] */ void __RPC_FAR *HENHMETAFILE;

typedef /* [wire_marshal] */ void __RPC_FAR *HMETAFILE;

#endif //0
typedef /* [wire_marshal] */ void __RPC_FAR *HMETAFILEPICT;



extern RPC_IF_HANDLE IWinTypes_v0_1_c_ifspec;
extern RPC_IF_HANDLE IWinTypes_v0_1_s_ifspec;
#endif /* __IWinTypes_INTERFACE_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL__intf_0001
 * at Fri Jul 12 18:09:25 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


typedef double DATE;

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
#define _CY_DEFINED
#if 0
/* the following isn't the real definition of CY, but it is */
/* what RPC knows how to remote */
typedef struct  tagCY
    {
    LONGLONG int64;
    }	CY;

#else /* 0 */
/* real definition that makes the C++ compiler happy */
typedef union tagCY {
    struct {
#ifdef _MAC
        long      Hi;
        long Lo;
#else
        unsigned long Lo;
        long      Hi;
#endif
    };
    LONGLONG int64;
} CY;
#endif /* 0 */
#endif /* _tagCY_DEFINED */
#if 0 /* _tagDEC_DEFINED */
/* The following isn't the real definition of Decimal type, */
/* but it is what RPC knows how to remote */
typedef struct  tagDEC
    {
    USHORT wReserved;
    BYTE scale;
    BYTE sign;
    ULONG Hi32;
    ULONGLONG Lo64;
    }	DECIMAL;

#else /* _tagDEC_DEFINED */
/* real definition that makes the C++ compiler happy */
typedef struct tagDEC {
    USHORT wReserved;
    union {
        struct {
            BYTE scale;
            BYTE sign;
        };
        USHORT signscale;
    };
    ULONG Hi32;
    union {
        struct {
#ifdef _MAC
            ULONG Mid32;
            ULONG Lo32;
#else
            ULONG Lo32;
            ULONG Mid32;
#endif
        };
        ULONGLONG Lo64;
    };
} DECIMAL;
#define DECIMAL_NEG ((BYTE)0x80)
#define DECIMAL_SETZERO(dec) \
        {(dec).Lo64 = 0; (dec).Hi32 = 0; (dec).signscale = 0;}
#endif /* _tagDEC_DEFINED */
typedef /* [unique] */ FLAGGED_WORD_BLOB __RPC_FAR *wireBSTR;

typedef /* [wire_marshal] */ OLECHAR __RPC_FAR *BSTR;

typedef BSTR __RPC_FAR *LPBSTR;

/* 0 == FALSE, -1 == TRUE */
typedef short VARIANT_BOOL;

#if !__STDC__ && (_MSC_VER <= 1000)
/* For backward compatibility */
typedef VARIANT_BOOL _VARIANT_BOOL;

#else
/* ANSI C/C++ reserve bool as keyword */
#define _VARIANT_BOOL    /##/
#endif
typedef boolean BOOLEAN;

#define VARIANT_TRUE ((VARIANT_BOOL)0xffff)
#define VARIANT_FALSE ((VARIANT_BOOL)0)
#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED
typedef struct  tagBLOB
    {
    ULONG cbSize;
    /* [size_is] */ BYTE __RPC_FAR *pBlobData;
    }	BLOB;

typedef struct tagBLOB __RPC_FAR *LPBLOB;

#endif
typedef struct  tagCLIPDATA
    {
    ULONG cbSize;
    long ulClipFmt;
    /* [size_is] */ BYTE __RPC_FAR *pClipData;
    }	CLIPDATA;

// Macro to calculate the size of the above pClipData
#define CBPCLIPDATA(clipdata)    ( (clipdata).cbSize - sizeof((clipdata).ulClipFmt) )
typedef unsigned short VARTYPE;

/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 *
 *
 *  VT_EMPTY            [V]   [P]     nothing
 *  VT_NULL             [V]   [P]     SQL style Null
 *  VT_I2               [V][T][P][S]  2 byte signed int
 *  VT_I4               [V][T][P][S]  4 byte signed int
 *  VT_R4               [V][T][P][S]  4 byte real
 *  VT_R8               [V][T][P][S]  8 byte real
 *  VT_CY               [V][T][P][S]  currency
 *  VT_DATE             [V][T][P][S]  date
 *  VT_BSTR             [V][T][P][S]  OLE Automation string
 *  VT_DISPATCH         [V][T][P][S]  IDispatch *
 *  VT_ERROR            [V][T]   [S]  SCODE
 *  VT_BOOL             [V][T][P][S]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]  VARIANT *
 *  VT_UNKNOWN          [V][T]   [S]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S]  16 byte fixed point
 *  VT_I1                  [T]        signed char
 *  VT_UI1              [V][T][P][S]  unsigned char
 *  VT_UI2                 [T][P]     unsigned short
 *  VT_UI4                 [T][P]     unsigned short
 *  VT_I8                  [T][P]     signed 64-bit int
 *  VT_UI8                 [T][P]     unsigned 64-bit int
 *  VT_INT                 [T]        signed machine int
 *  VT_UINT                [T]        unsigned machine int
 *  VT_VOID                [T]        C style void
 *  VT_HRESULT             [T]        Standard return type
 *  VT_PTR                 [T]        pointer type
 *  VT_SAFEARRAY           [T]        (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]        C style array
 *  VT_USERDEFINED         [T]        user defined type
 *  VT_LPSTR               [T][P]     null terminated string
 *  VT_LPWSTR              [T][P]     wide null terminated string
 *  VT_FILETIME               [P]     FILETIME
 *  VT_BLOB                   [P]     Length prefixed bytes
 *  VT_STREAM                 [P]     Name of the stream follows
 *  VT_STORAGE                [P]     Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]     Stream contains an object
 *  VT_STORED_OBJECT          [P]     Storage contains an object
 *  VT_BLOB_OBJECT            [P]     Blob contains an object
 *  VT_CF                     [P]     Clipboard format
 *  VT_CLSID                  [P]     A Class ID
 *  VT_VECTOR                 [P]     simple counted array
 *  VT_ARRAY            [V]           SAFEARRAY*
 *  VT_BYREF            [V]           void* for local use
 */

enum VARENUM
    {	VT_EMPTY	= 0,
	VT_NULL	= 1,
	VT_I2	= 2,
	VT_I4	= 3,
	VT_R4	= 4,
	VT_R8	= 5,
	VT_CY	= 6,
	VT_DATE	= 7,
	VT_BSTR	= 8,
	VT_DISPATCH	= 9,
	VT_ERROR	= 10,
	VT_BOOL	= 11,
	VT_VARIANT	= 12,
	VT_UNKNOWN	= 13,
	VT_DECIMAL	= 14,
	VT_I1	= 16,
	VT_UI1	= 17,
	VT_UI2	= 18,
	VT_UI4	= 19,
	VT_I8	= 20,
	VT_UI8	= 21,
	VT_INT	= 22,
	VT_UINT	= 23,
	VT_VOID	= 24,
	VT_HRESULT	= 25,
	VT_PTR	= 26,
	VT_SAFEARRAY	= 27,
	VT_CARRAY	= 28,
	VT_USERDEFINED	= 29,
	VT_LPSTR	= 30,
	VT_LPWSTR	= 31,
	VT_FILETIME	= 64,
	VT_BLOB	= 65,
	VT_STREAM	= 66,
	VT_STORAGE	= 67,
	VT_STREAMED_OBJECT	= 68,
	VT_STORED_OBJECT	= 69,
	VT_BLOB_OBJECT	= 70,
	VT_CF	= 71,
	VT_CLSID	= 72,
	VT_VECTOR	= 0x1000,
	VT_ARRAY	= 0x2000,
	VT_BYREF	= 0x4000,
	VT_RESERVED	= 0x8000,
	VT_ILLEGAL	= 0xffff,
	VT_ILLEGALMASKED	= 0xfff,
	VT_TYPEMASK	= 0xfff
    };
typedef ULONG PROPID;



extern RPC_IF_HANDLE __MIDL__intf_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0001_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
