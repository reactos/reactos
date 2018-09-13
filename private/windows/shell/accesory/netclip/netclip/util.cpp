// util.cpp
#include "stdafx.h"
#include "util.h"

#define new DEBUG_NEW


// Stolen from OLEMISC.CPP in the MFC 3.0 source.  Function names
// changed from Afx* to _*.
//
#define _countof(array)	(sizeof(array)/sizeof(array[0]))
LPCTSTR AFXAPI _GetScodeString(SCODE sc)
{
	struct SCODE_ENTRY
	{
		SCODE sc;
		LPCTSTR lpszName;
	};
	#define MAKE_SCODE_ENTRY(sc)    { sc, _T(#sc) }
	static const SCODE_ENTRY scNameTable[] =
	{
		MAKE_SCODE_ENTRY(S_OK),
		MAKE_SCODE_ENTRY(S_FALSE),

		MAKE_SCODE_ENTRY(CACHE_S_FORMATETC_NOTSUPPORTED),
		MAKE_SCODE_ENTRY(CACHE_S_SAMECACHE),
		MAKE_SCODE_ENTRY(CACHE_S_SOMECACHES_NOTUPDATED),
		MAKE_SCODE_ENTRY(CONVERT10_S_NO_PRESENTATION),
		MAKE_SCODE_ENTRY(DATA_S_SAMEFORMATETC),
		MAKE_SCODE_ENTRY(DRAGDROP_S_CANCEL),
		MAKE_SCODE_ENTRY(DRAGDROP_S_DROP),
		MAKE_SCODE_ENTRY(DRAGDROP_S_USEDEFAULTCURSORS),
		MAKE_SCODE_ENTRY(INPLACE_S_TRUNCATED),
		MAKE_SCODE_ENTRY(MK_S_HIM),
		MAKE_SCODE_ENTRY(MK_S_ME),
		MAKE_SCODE_ENTRY(MK_S_MONIKERALREADYREGISTERED),
		MAKE_SCODE_ENTRY(MK_S_REDUCED_TO_SELF),
		MAKE_SCODE_ENTRY(MK_S_US),
		MAKE_SCODE_ENTRY(OLE_S_MAC_CLIPFORMAT),
		MAKE_SCODE_ENTRY(OLE_S_STATIC),
		MAKE_SCODE_ENTRY(OLE_S_USEREG),
		MAKE_SCODE_ENTRY(OLEOBJ_S_CANNOT_DOVERB_NOW),
		MAKE_SCODE_ENTRY(OLEOBJ_S_INVALIDHWND),
		MAKE_SCODE_ENTRY(OLEOBJ_S_INVALIDVERB),
		MAKE_SCODE_ENTRY(OLEOBJ_S_LAST),
		MAKE_SCODE_ENTRY(STG_S_CONVERTED),
		MAKE_SCODE_ENTRY(VIEW_S_ALREADY_FROZEN),

		MAKE_SCODE_ENTRY(E_UNEXPECTED),
		MAKE_SCODE_ENTRY(E_NOTIMPL),
		MAKE_SCODE_ENTRY(E_OUTOFMEMORY),
		MAKE_SCODE_ENTRY(E_INVALIDARG),
		MAKE_SCODE_ENTRY(E_NOINTERFACE),
		MAKE_SCODE_ENTRY(E_POINTER),
		MAKE_SCODE_ENTRY(E_HANDLE),
		MAKE_SCODE_ENTRY(E_ABORT),
		MAKE_SCODE_ENTRY(E_FAIL),
		MAKE_SCODE_ENTRY(E_ACCESSDENIED),

		MAKE_SCODE_ENTRY(CACHE_E_NOCACHE_UPDATED),
		MAKE_SCODE_ENTRY(CLASS_E_CLASSNOTAVAILABLE),
		MAKE_SCODE_ENTRY(CLASS_E_NOAGGREGATION),
		MAKE_SCODE_ENTRY(CLIPBRD_E_BAD_DATA),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_CLOSE),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_EMPTY),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_OPEN),
		MAKE_SCODE_ENTRY(CLIPBRD_E_CANT_SET),
		MAKE_SCODE_ENTRY(CO_E_ALREADYINITIALIZED),
		MAKE_SCODE_ENTRY(CO_E_APPDIDNTREG),
		MAKE_SCODE_ENTRY(CO_E_APPNOTFOUND),
		MAKE_SCODE_ENTRY(CO_E_APPSINGLEUSE),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_BAD_PATH),
		#endif
		MAKE_SCODE_ENTRY(CO_E_CANTDETERMINECLASS),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_CLASS_CREATE_FAILED),
		#endif
		MAKE_SCODE_ENTRY(CO_E_CLASSSTRING),
		MAKE_SCODE_ENTRY(CO_E_DLLNOTFOUND),
		MAKE_SCODE_ENTRY(CO_E_ERRORINAPP),
		MAKE_SCODE_ENTRY(CO_E_ERRORINDLL),
		MAKE_SCODE_ENTRY(CO_E_IIDSTRING),
		MAKE_SCODE_ENTRY(CO_E_NOTINITIALIZED),
		MAKE_SCODE_ENTRY(CO_E_OBJISREG),
		MAKE_SCODE_ENTRY(CO_E_OBJNOTCONNECTED),
		MAKE_SCODE_ENTRY(CO_E_OBJNOTREG),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(CO_E_OBJSRV_RPC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SCM_ERROR),
		MAKE_SCODE_ENTRY(CO_E_SCM_RPC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SERVER_EXEC_FAILURE),
		MAKE_SCODE_ENTRY(CO_E_SERVER_STOPPING),
		#endif
		MAKE_SCODE_ENTRY(CO_E_WRONGOSFORAPP),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_FMT),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_GET),
		MAKE_SCODE_ENTRY(CONVERT10_E_OLESTREAM_PUT),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_DIB_TO_BITMAP),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_FMT),
		MAKE_SCODE_ENTRY(CONVERT10_E_STG_NO_STD_STREAM),
		MAKE_SCODE_ENTRY(DISP_E_ARRAYISLOCKED),
		MAKE_SCODE_ENTRY(DISP_E_BADCALLEE),
		MAKE_SCODE_ENTRY(DISP_E_BADINDEX),
		MAKE_SCODE_ENTRY(DISP_E_BADPARAMCOUNT),
		MAKE_SCODE_ENTRY(DISP_E_BADVARTYPE),
		MAKE_SCODE_ENTRY(DISP_E_EXCEPTION),
		MAKE_SCODE_ENTRY(DISP_E_MEMBERNOTFOUND),
		MAKE_SCODE_ENTRY(DISP_E_NONAMEDARGS),
		MAKE_SCODE_ENTRY(DISP_E_NOTACOLLECTION),
		MAKE_SCODE_ENTRY(DISP_E_OVERFLOW),
		MAKE_SCODE_ENTRY(DISP_E_PARAMNOTFOUND),
		MAKE_SCODE_ENTRY(DISP_E_PARAMNOTOPTIONAL),
		MAKE_SCODE_ENTRY(DISP_E_TYPEMISMATCH),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNINTERFACE),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNLCID),
		MAKE_SCODE_ENTRY(DISP_E_UNKNOWNNAME),
		MAKE_SCODE_ENTRY(DRAGDROP_E_ALREADYREGISTERED),
		MAKE_SCODE_ENTRY(DRAGDROP_E_INVALIDHWND),
		MAKE_SCODE_ENTRY(DRAGDROP_E_NOTREGISTERED),
		MAKE_SCODE_ENTRY(DV_E_CLIPFORMAT),
		MAKE_SCODE_ENTRY(DV_E_DVASPECT),
		MAKE_SCODE_ENTRY(DV_E_DVTARGETDEVICE),
		MAKE_SCODE_ENTRY(DV_E_DVTARGETDEVICE_SIZE),
		MAKE_SCODE_ENTRY(DV_E_FORMATETC),
		MAKE_SCODE_ENTRY(DV_E_LINDEX),
		MAKE_SCODE_ENTRY(DV_E_NOIVIEWOBJECT),
		MAKE_SCODE_ENTRY(DV_E_STATDATA),
		MAKE_SCODE_ENTRY(DV_E_STGMEDIUM),
		MAKE_SCODE_ENTRY(DV_E_TYMED),
		MAKE_SCODE_ENTRY(INPLACE_E_NOTOOLSPACE),
		MAKE_SCODE_ENTRY(INPLACE_E_NOTUNDOABLE),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MEM_E_INVALID_LINK),
		MAKE_SCODE_ENTRY(MEM_E_INVALID_ROOT),
		MAKE_SCODE_ENTRY(MEM_E_INVALID_SIZE),
		#endif		
		MAKE_SCODE_ENTRY(MK_E_CANTOPENFILE),
		MAKE_SCODE_ENTRY(MK_E_CONNECTMANUALLY),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MK_E_ENUMERATION_FAILED),
		#endif
		MAKE_SCODE_ENTRY(MK_E_EXCEEDEDDEADLINE),
		MAKE_SCODE_ENTRY(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED),
		MAKE_SCODE_ENTRY(MK_E_INVALIDEXTENSION),
		MAKE_SCODE_ENTRY(MK_E_MUSTBOTHERUSER),
		MAKE_SCODE_ENTRY(MK_E_NEEDGENERIC),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(MK_E_NO_NORMALIZED),
		#endif
		MAKE_SCODE_ENTRY(MK_E_NOINVERSE),
		MAKE_SCODE_ENTRY(MK_E_NOOBJECT),
		MAKE_SCODE_ENTRY(MK_E_NOPREFIX),
		MAKE_SCODE_ENTRY(MK_E_NOSTORAGE),
		MAKE_SCODE_ENTRY(MK_E_NOTBINDABLE),
		MAKE_SCODE_ENTRY(MK_E_NOTBOUND),
		MAKE_SCODE_ENTRY(MK_E_SYNTAX),
		MAKE_SCODE_ENTRY(MK_E_UNAVAILABLE),
		MAKE_SCODE_ENTRY(OLE_E_ADVF),
		MAKE_SCODE_ENTRY(OLE_E_ADVISENOTSUPPORTED),
		MAKE_SCODE_ENTRY(OLE_E_BLANK),
		MAKE_SCODE_ENTRY(OLE_E_CANT_BINDTOSOURCE),
		MAKE_SCODE_ENTRY(OLE_E_CANT_GETMONIKER),
		MAKE_SCODE_ENTRY(OLE_E_CANTCONVERT),
		MAKE_SCODE_ENTRY(OLE_E_CLASSDIFF),
		MAKE_SCODE_ENTRY(OLE_E_ENUM_NOMORE),
		MAKE_SCODE_ENTRY(OLE_E_INVALIDHWND),
		MAKE_SCODE_ENTRY(OLE_E_INVALIDRECT),
		MAKE_SCODE_ENTRY(OLE_E_NOCACHE),
		MAKE_SCODE_ENTRY(OLE_E_NOCONNECTION),
		MAKE_SCODE_ENTRY(OLE_E_NOSTORAGE),
		MAKE_SCODE_ENTRY(OLE_E_NOT_INPLACEACTIVE),
		MAKE_SCODE_ENTRY(OLE_E_NOTRUNNING),
		MAKE_SCODE_ENTRY(OLE_E_OLEVERB),
		MAKE_SCODE_ENTRY(OLE_E_PROMPTSAVECANCELLED),
		MAKE_SCODE_ENTRY(OLE_E_STATIC),
		MAKE_SCODE_ENTRY(OLE_E_WRONGCOMPOBJ),
		MAKE_SCODE_ENTRY(OLEOBJ_E_INVALIDVERB),
		MAKE_SCODE_ENTRY(OLEOBJ_E_NOVERBS),
		MAKE_SCODE_ENTRY(REGDB_E_CLASSNOTREG),
		MAKE_SCODE_ENTRY(REGDB_E_IIDNOTREG),
		MAKE_SCODE_ENTRY(REGDB_E_INVALIDVALUE),
		MAKE_SCODE_ENTRY(REGDB_E_KEYMISSING),
		MAKE_SCODE_ENTRY(REGDB_E_READREGDB),
		MAKE_SCODE_ENTRY(REGDB_E_WRITEREGDB),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_ATTEMPTED_MULTITHREAD),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CALL_CANCELED),
		MAKE_SCODE_ENTRY(RPC_E_CALL_REJECTED),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_AGAIN),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_INASYNCCALL),
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_INEXTERNALCALL),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_CANTCALLOUT_ININPUTSYNCCALL),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CANTPOST_INSENDCALL),
		MAKE_SCODE_ENTRY(RPC_E_CANTTRANSMIT_CALL),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_CHANGED_MODE),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_CANTMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_CANTUNMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_CLIENT_DIED),
		MAKE_SCODE_ENTRY(RPC_E_CONNECTION_TERMINATED),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_DISCONNECTED),
		MAKE_SCODE_ENTRY(RPC_E_FAULT),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_CALLDATA),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_INVALID_DATA),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_DATAPACKET),
		MAKE_SCODE_ENTRY(RPC_E_INVALID_PARAMETER),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_INVALIDMETHOD),
		MAKE_SCODE_ENTRY(RPC_E_NOT_REGISTERED),
		MAKE_SCODE_ENTRY(RPC_E_OUT_OF_RESOURCES),
		MAKE_SCODE_ENTRY(RPC_E_RETRY),
		#endif		
		MAKE_SCODE_ENTRY(RPC_E_SERVER_CANTMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_SERVER_CANTUNMARSHAL_DATA),
		MAKE_SCODE_ENTRY(RPC_E_SERVER_DIED),
		#ifdef _WIN32
		MAKE_SCODE_ENTRY(RPC_E_SERVER_DIED_DNE),
		MAKE_SCODE_ENTRY(RPC_E_SERVERCALL_REJECTED),
		MAKE_SCODE_ENTRY(RPC_E_SERVERCALL_RETRYLATER),
		MAKE_SCODE_ENTRY(RPC_E_SERVERFAULT),
		MAKE_SCODE_ENTRY(RPC_E_SYS_CALL_FAILED),
		MAKE_SCODE_ENTRY(RPC_E_THREAD_NOT_INIT),
		MAKE_SCODE_ENTRY(RPC_E_WRONG_THREAD),
		#endif
		MAKE_SCODE_ENTRY(RPC_E_UNEXPECTED),
		MAKE_SCODE_ENTRY(STG_E_ABNORMALAPIEXIT),
		MAKE_SCODE_ENTRY(STG_E_ACCESSDENIED),
		MAKE_SCODE_ENTRY(STG_E_CANTSAVE),
		MAKE_SCODE_ENTRY(STG_E_DISKISWRITEPROTECTED),
		MAKE_SCODE_ENTRY(STG_E_EXTANTMARSHALLINGS),
		MAKE_SCODE_ENTRY(STG_E_FILEALREADYEXISTS),
		MAKE_SCODE_ENTRY(STG_E_FILENOTFOUND),
		MAKE_SCODE_ENTRY(STG_E_INSUFFICIENTMEMORY),
		MAKE_SCODE_ENTRY(STG_E_INUSE),
		MAKE_SCODE_ENTRY(STG_E_INVALIDFLAG),
		MAKE_SCODE_ENTRY(STG_E_INVALIDFUNCTION),
		MAKE_SCODE_ENTRY(STG_E_INVALIDHANDLE),
		MAKE_SCODE_ENTRY(STG_E_INVALIDHEADER),
		MAKE_SCODE_ENTRY(STG_E_INVALIDNAME),
		MAKE_SCODE_ENTRY(STG_E_INVALIDPARAMETER),
		MAKE_SCODE_ENTRY(STG_E_INVALIDPOINTER),
		MAKE_SCODE_ENTRY(STG_E_LOCKVIOLATION),
		MAKE_SCODE_ENTRY(STG_E_MEDIUMFULL),
		MAKE_SCODE_ENTRY(STG_E_NOMOREFILES),
		MAKE_SCODE_ENTRY(STG_E_NOTCURRENT),
		MAKE_SCODE_ENTRY(STG_E_NOTFILEBASEDSTORAGE),
		MAKE_SCODE_ENTRY(STG_E_OLDDLL),
		MAKE_SCODE_ENTRY(STG_E_OLDFORMAT),
		MAKE_SCODE_ENTRY(STG_E_PATHNOTFOUND),
		MAKE_SCODE_ENTRY(STG_E_READFAULT),
		MAKE_SCODE_ENTRY(STG_E_REVERTED),
		MAKE_SCODE_ENTRY(STG_E_SEEKERROR),
		MAKE_SCODE_ENTRY(STG_E_SHAREREQUIRED),
		MAKE_SCODE_ENTRY(STG_E_SHAREVIOLATION),
		MAKE_SCODE_ENTRY(STG_E_TOOMANYOPENFILES),
		MAKE_SCODE_ENTRY(STG_E_UNIMPLEMENTEDFUNCTION),
		MAKE_SCODE_ENTRY(STG_E_UNKNOWN),
		MAKE_SCODE_ENTRY(STG_E_WRITEFAULT),
		MAKE_SCODE_ENTRY(TYPE_E_AMBIGUOUSNAME),
		MAKE_SCODE_ENTRY(TYPE_E_BADMODULEKIND),
		MAKE_SCODE_ENTRY(TYPE_E_BUFFERTOOSMALL),
		MAKE_SCODE_ENTRY(TYPE_E_CANTCREATETMPFILE),
		MAKE_SCODE_ENTRY(TYPE_E_CANTLOADLIBRARY),
		MAKE_SCODE_ENTRY(TYPE_E_CIRCULARTYPE),
		MAKE_SCODE_ENTRY(TYPE_E_DLLFUNCTIONNOTFOUND),
		MAKE_SCODE_ENTRY(TYPE_E_DUPLICATEID),
		MAKE_SCODE_ENTRY(TYPE_E_ELEMENTNOTFOUND),
		MAKE_SCODE_ENTRY(TYPE_E_INCONSISTENTPROPFUNCS),
		MAKE_SCODE_ENTRY(TYPE_E_INVALIDSTATE),
		MAKE_SCODE_ENTRY(TYPE_E_INVDATAREAD),
		MAKE_SCODE_ENTRY(TYPE_E_IOERROR),
		MAKE_SCODE_ENTRY(TYPE_E_LIBNOTREGISTERED),
		MAKE_SCODE_ENTRY(TYPE_E_NAMECONFLICT),
		MAKE_SCODE_ENTRY(TYPE_E_OUTOFBOUNDS),
		MAKE_SCODE_ENTRY(TYPE_E_QUALIFIEDNAMEDISALLOWED),
		MAKE_SCODE_ENTRY(TYPE_E_REGISTRYACCESS),
		MAKE_SCODE_ENTRY(TYPE_E_SIZETOOBIG),
		MAKE_SCODE_ENTRY(TYPE_E_TYPEMISMATCH),
		MAKE_SCODE_ENTRY(TYPE_E_UNDEFINEDTYPE),
		MAKE_SCODE_ENTRY(TYPE_E_UNKNOWNLCID),
		MAKE_SCODE_ENTRY(TYPE_E_UNSUPFORMAT),
		MAKE_SCODE_ENTRY(TYPE_E_WRONGTYPEKIND),
		MAKE_SCODE_ENTRY(VIEW_E_DRAW),
	};
	#undef MAKE_SCODE_ENTRY

	// look for it in the table
	for (int i = 0; i < _countof(scNameTable); i++)
	{
		if (sc == scNameTable[i].sc)
			return scNameTable[i].lpszName;
	}
	return NULL;    // not found
}

LPCTSTR AFXAPI _GetScodeRangeString(SCODE sc)
{
	struct RANGE_ENTRY
	{
		SCODE scFirst;
		SCODE scLast;
		LPCTSTR lpszName;
	};
	#define MAKE_RANGE_ENTRY(scRange) \
		{ scRange##_FIRST, scRange##_LAST, \
			_T(#scRange) _T("_FIRST...") _T(#scRange) _T("_LAST") }

	static const RANGE_ENTRY scRangeTable[] =
	{
		MAKE_RANGE_ENTRY(CACHE_E),
		MAKE_RANGE_ENTRY(CACHE_S),
		MAKE_RANGE_ENTRY(CLASSFACTORY_E),
		MAKE_RANGE_ENTRY(CLASSFACTORY_S),
		MAKE_RANGE_ENTRY(CLIENTSITE_E),
		MAKE_RANGE_ENTRY(CLIENTSITE_S),
		MAKE_RANGE_ENTRY(CLIPBRD_E),
		MAKE_RANGE_ENTRY(CLIPBRD_S),
		MAKE_RANGE_ENTRY(CONVERT10_E),
		MAKE_RANGE_ENTRY(CONVERT10_S),
		MAKE_RANGE_ENTRY(CO_E),
		MAKE_RANGE_ENTRY(CO_S),
		MAKE_RANGE_ENTRY(DATA_E),
		MAKE_RANGE_ENTRY(DATA_S),
		MAKE_RANGE_ENTRY(DRAGDROP_E),
		MAKE_RANGE_ENTRY(DRAGDROP_S),
		MAKE_RANGE_ENTRY(ENUM_E),
		MAKE_RANGE_ENTRY(ENUM_S),
		MAKE_RANGE_ENTRY(INPLACE_E),
		MAKE_RANGE_ENTRY(INPLACE_S),
		MAKE_RANGE_ENTRY(MARSHAL_E),
		MAKE_RANGE_ENTRY(MARSHAL_S),
		MAKE_RANGE_ENTRY(MK_E),
		MAKE_RANGE_ENTRY(MK_S),
		MAKE_RANGE_ENTRY(OLEOBJ_E),
		MAKE_RANGE_ENTRY(OLEOBJ_S),
		MAKE_RANGE_ENTRY(OLE_E),
		MAKE_RANGE_ENTRY(OLE_S),
		MAKE_RANGE_ENTRY(REGDB_E),
		MAKE_RANGE_ENTRY(REGDB_S),
		MAKE_RANGE_ENTRY(VIEW_E),
		MAKE_RANGE_ENTRY(VIEW_S),
	};
	#undef MAKE_RANGE_ENTRY

	// look for it in the table
	for (int i = 0; i < _countof(scRangeTable); i++)
	{
		if (sc >= scRangeTable[i].scFirst && sc <= scRangeTable[i].scLast)
			return scRangeTable[i].lpszName;
	}
	return NULL;    // not found
}

LPCTSTR AFXAPI _GetSeverityString(SCODE sc)
{
	static const TCHAR* rgszSEVERITY[] =
	{
		_T("SEVERITY_SUCCESS"),
		_T("SEVERITY_ERROR"),
	};
	return rgszSEVERITY[SCODE_SEVERITY(sc)];
}

LPCTSTR AFXAPI _GetFacilityString(SCODE sc)
{
	static const TCHAR* rgszFACILITY[] =
	{
		_T("FACILITY_NULL"),
		_T("FACILITY_RPC"),
		_T("FACILITY_DISPATCH"),
		_T("FACILITY_STORAGE"),
		_T("FACILITY_ITF"),
		_T("FACILITY_0x05"),
		_T("FACILITY_0x06"),
		_T("FACILITY_WIN32"),
		_T("FACILITY_WINDOWS"),
	};
	if (SCODE_FACILITY(sc) >= _countof(rgszFACILITY))
		return _T("<Unknown Facility>");

	return rgszFACILITY[SCODE_FACILITY(sc)];
}

LPCTSTR AFXAPI _GetFullScodeString(SCODE sc)
{
	static TCHAR szBuf[128];
	LPCTSTR lpsz;
	if ((lpsz = _GetScodeString(sc)) != NULL)
	{
		// found exact match
		wsprintf(szBuf, _T("%s ($%08lX)"), lpsz, sc);
	}
	else if ((lpsz = _GetScodeRangeString(sc)) != NULL)
	{
		// found suitable range
		wsprintf(szBuf, _T("range: %s ($%08lX)"), lpsz, sc);
	}
	else
	{
		// not found at all -- split it up into its parts
		wsprintf(szBuf, _T("severity: %s, facility: %s ($%08lX)"),
			_GetSeverityString(sc), _GetFacilityString(sc), sc);
	}
	return szBuf;
}

void ErrorMessageID( const UINT id, HRESULT hr)
{
    CString str;
    str.LoadString(id);
    ErrorMessage(str, hr);
}

void ErrorMessage( const CString& str, HRESULT hr )
{
//#ifdef _DEBUG
    LPVOID lpMessageBuffer = NULL ;
#ifndef _MAC
    if (FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          hr,
          MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
          (LPTSTR) &lpMessageBuffer,
          0,
          NULL ))
    {
        CString str2 ;
        str2.Format( _T("\n%s %s"), (LPCTSTR)lpMessageBuffer, (LPCTSTR)_GetFullScodeString(hr) ) ;
        str2 = str + str2 ;
        LocalFree(lpMessageBuffer) ;
        AfxMessageBox( str2 ) ;
    }
    else
#endif // !_MAC
    {
        CString str2 ;
        str2.Format( _T("\n<No system message defined> %s"), (LPCTSTR)_GetFullScodeString(hr) ) ;
        str2 = str + str2 ;
        AfxMessageBox( str2 ) ;
    }
//#else // _DEBUG
//    AfxMessageBox(str) ;
//#endif
}

LPTSTR HRtoString( HRESULT hr )
{   
    SCODE   sc ;
    sc = GetScode( hr ) ;

#ifndef _MAC
#ifdef FORMATMSG
    LPVOID lpMessageBuffer ;
    if (FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |
          FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          sc,
          MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
          (LPTSTR) &lpMessageBuffer,
          0,
          NULL ))
    {
        wsprintf( sz, _T( "%s (0x%lx)" ), (LPTSTR)lpMessageBuffer, sc);
        LocalFree(lpMessageBuffer) ;
    }
    else
    {
        wsprintf( sz, _T( "Unknown Scode (0x%lx)" ), sc);
    }
    

#endif // !FORMATMSG
#endif // !_MAC

    return (LPTSTR)_GetFullScodeString(sc) ;
}   

static TCHAR * g_rgszVT[] =
{
    _T( "Void" ),             //VT_EMPTY           = 0,   /* [V]   [P]  nothing                     */
    _T( "Null" ),             //VT_NULL            = 1,   /* [V]        SQL style Null              */
    _T( "Integer" ),          //VT_I2              = 2,   /* [V][T][P]  2 byte signed int           */
    _T( "Long" ),             //VT_I4              = 3,   /* [V][T][P]  4 byte signed int           */
    _T( "Single" ),           //VT_R4              = 4,   /* [V][T][P]  4 byte real                 */
    _T( "Double" ),           //VT_R8              = 5,   /* [V][T][P]  8 byte real                 */
    _T( "Currency" ),         //VT_CY              = 6,   /* [V][T][P]  currency                    */
    _T( "Date" ),             //VT_DATE            = 7,   /* [V][T][P]  date                        */
    _T( "String" ),           //VT_BSTR            = 8,   /* [V][T][P]  binary string               */
    _T( "Object" ),           //VT_DISPATCH        = 9,   /* [V][T]     IDispatch FAR*              */
    _T( "SCODE" ),            //VT_ERROR           = 10,  /* [V][T]     SCODE                       */
    _T( "Boolean" ),          //VT_BOOL            = 11,  /* [V][T][P]  True=-1, False=0            */
    _T( "Variant" ),          //VT_VARIANT         = 12,  /* [V][T][P]  VARIANT FAR*                */
    _T( "pIUnknown" ),        //VT_UNKNOWN         = 13,  /* [V][T]     IUnknown FAR*               */
    _T( "Unicode" ),          //VT_WBSTR           = 14,  /* [V][T]     wide binary string          */
    _T( "" ),                 //                   = 15,
    _T( "BYTE" ),             //VT_I1              = 16,  /*    [T]     signed char                 */
    _T( "char" ),             //VT_UI1             = 17,  /*    [T]     unsigned char               */
    _T( "USHORT" ),           //VT_UI2             = 18,  /*    [T]     unsigned short              */
    _T( "ULONG" ),            //VT_UI4             = 19,  /*    [T]     unsigned short              */
    _T( "int64" ),            //VT_I8              = 20,  /*    [T][P]  signed 64-bit int           */
    _T( "uint64" ),           //VT_UI8             = 21,  /*    [T]     unsigned 64-bit int         */
    _T( "int" ),              //VT_INT             = 22,  /*    [T]     signed machine int          */
    _T( "UINT" ),             //VT_UINT            = 23,  /*    [T]     unsigned machine int        */
    _T( "VOID" ),             //VT_VOID            = 24,  /*    [T]     C style void                */
    _T( "HRESULT" ),          //VT_HRESULT         = 25,  /*    [T]                                 */
    _T( "PTR" ),              //VT_PTR             = 26,  /*    [T]     pointer type                */
    _T( "SAFEARRAY" ),        //VT_SAFEARRAY       = 27,  /*    [T]     (use VT_ARRAY in VARIANT)   */
    _T( "CARRAY" ),           //VT_CARRAY          = 28,  /*    [T]     C style array               */
    _T( "USERDEFINED" ),      //VT_USERDEFINED     = 29,  /*    [T]     user defined type         */
    _T( "LPTSTR" ),            //VT_LPTSTR           = 30,  /*    [T][P]  null terminated string      */
    _T( "LPWSTR" ),           //VT_LPWSTR          = 31,  /*    [T][P]  wide null terminated string */
    _T( "" ),                 //                   = 32,
    _T( "" ),                 //                   = 33,
    _T( "" ),                 //                   = 34,
    _T( "" ),                 //                   = 35,
    _T( "" ),                 //                   = 36,
    _T( "" ),                 //                   = 37,
    _T( "" ),                 //                   = 38,
    _T( "" ),                 //                   = 39,
    _T( "" ),                 //                   = 40,
    _T( "" ),                 //                   = 41,
    _T( "" ),                 //                   = 42,
    _T( "" ),                 //                   = 43,
    _T( "" ),                 //                   = 44,
    _T( "" ),                 //                   = 45,
    _T( "" ),                 //                   = 46,
    _T( "" ),                 //                   = 47,
    _T( "" ),                 //                   = 48,
    _T( "" ),                 //                   = 49,
    _T( "" ),                 //                   = 50,
    _T( "" ),                 //                   = 51,
    _T( "" ),                 //                   = 52,
    _T( "" ),                 //                   = 53,
    _T( "" ),                 //                   = 54,
    _T( "" ),                 //                   = 55,
    _T( "" ),                 //                   = 56,
    _T( "" ),                 //                   = 57,
    _T( "" ),                 //                   = 58,
    _T( "" ),                 //                   = 59,
    _T( "" ),                 //                   = 60,
    _T( "" ),                 //                   = 61,
    _T( "" ),                 //                   = 62,
    _T( "" ),                 //                   = 63,
    _T( "FILETIME" ),         //VT_FILETIME        = 64,  /*       [P]  FILETIME                    */
    _T( "BLOB" ),             //VT_BLOB            = 65,  /*       [P]  Length prefixed bytes       */
    _T( "STREAM" ),           //VT_STREAM          = 66,  /*       [P]  Name of the stream follows  */
    _T( "STORAGE" ),          //VT_STORAGE         = 67,  /*       [P]  Name of the storage follows */
    _T( "STREAMED_OBJECT" ),  //VT_STREAMED_OBJECT = 68,  /*       [P]  Stream contains an object   */
    _T( "STORED_OBJECT" ),    //VT_STORED_OBJECT   = 69,  /*       [P]  Storage contains an object  */
    _T( "BLOB_OBJECT" ),      //VT_BLOB_OBJECT     = 70,  /*       [P]  Blob contains an object     */
    _T( "CF" ),               //VT_CF              = 71,  /*       [P]  Clipboard format            */
    _T( "CLSID" ),            //VT_CLSID           = 72   /*       [P]  A Class ID                  */
};

LPTSTR VTtoString( VARTYPE vt )
{
    static TCHAR szBuf[64];

    if (vt <= VT_CLSID)
        return (LPTSTR)g_rgszVT[vt] ;

    if (vt & VT_VECTOR)
    {
        vt &= ~VT_VECTOR ;
        if (vt <= VT_CLSID)
            wsprintf( szBuf, _T("VECTOR of %s"), (LPTSTR)g_rgszVT[vt] ) ;
        else
            wsprintf( szBuf, _T("<Unknown %08lX>"), vt & VT_VECTOR ) ;
        return (LPTSTR)szBuf ;
    }

    if (vt & VT_ARRAY)
    {
        vt &= ~VT_ARRAY ;
        if (vt <= VT_CLSID)
            wsprintf( szBuf, _T("Array of %s"), (LPTSTR)g_rgszVT[vt] ) ;
        else
            wsprintf( szBuf, _T("<Unknown %08lX>"), vt & VT_ARRAY ) ;
        return (LPTSTR)szBuf ;
    }

    if (vt & VT_BYREF)
    {
        vt &= ~VT_BYREF ;
        if (vt <= VT_CLSID)
            wsprintf( szBuf, _T("%s BYREF "), (LPTSTR)g_rgszVT[vt] ) ;
        else
            wsprintf( szBuf, _T("<Unknown %08lX>"), vt & VT_BYREF ) ;
        return (LPTSTR)szBuf ;
    }

    if (vt & VT_RESERVED)
    {
        vt &= ~VT_RESERVED ;
        if (vt <= VT_CLSID)
            wsprintf( szBuf, _T("RESERVED (%s)"), (LPTSTR)g_rgszVT[vt] ) ;
        else
            wsprintf( szBuf, _T("<Unknown %08lX>"), vt & VT_RESERVED ) ;
        return (LPTSTR)szBuf ;
    }

    wsprintf( szBuf, _T("<Unknown %08lX>"), vt ) ;
            
    return (LPTSTR)szBuf ;
}

/* Remove the filespec portion from a path (including the backslash).
 */
VOID WINAPI StripFilespec( LPTSTR lpszPath )
{
    LPTSTR     p;

    p = lpszPath + lstrlen( lpszPath ) ;
    while ((*p != '\\') && (*p != ':') && (p != lpszPath))
        p-- ;

    if (*p == ':')
        p++ ;

    /* Don't strip backslash from root directory entry. */
    if (p != lpszPath)
    {
        if ((*p == '\\') && (*(p-1) == ':'))
            p++ ;
    }

    *p = NULL ;
}

/* Ensures that a path ends with a backslash.
 */
VOID WINAPI AddBackslash( LPTSTR lpszPath )
{
    if (lpszPath[lstrlen( lpszPath ) - 1] == '\\')
        return ;
    lstrcat( lpszPath, _T("\\") ) ;
}

 #include <ctype.h>
// Very useful API that parses a LONG out of a string, updating
// the string pointer before it returns.
// 
LONG WINAPI ParseOffNumber( LPTSTR FAR *lplp, LPINT lpConv )
{
    LPTSTR lp = *lplp;
    LONG  lValue=0;
    int   sign=1;

    while( isspace(*lp) )
        lp++;
    if( *lp=='-' )
    {               
        sign = -1;
        lp++;
    }
    if (*lp=='\0')
    {
        *lpConv = FALSE;
        return 0L;
    }

    // Is it a decimal number
    if (!(*lp == '0' && (*(lp+1) == 'x' || *(lp+1) == 'X')))
    {
        
        while( isdigit(*lp) )
        {
            lValue *= 10;
            lValue += (*lp - (TCHAR)'0');
            lp++;
        }
    }    
    else
    {
        lp+=2;

        while( isxdigit(*lp) )
        {
            lValue *= 16;
            if( isdigit(*lp) )
                lValue += (*lp - (TCHAR)'0');
            else
                lValue += (toupper(*lp) - (TCHAR)'A' + 10);
            lp++;
        }
    }
    while( isspace(*lp) )
        lp++;

    lValue *= (long)sign;

    if (*lp==(TCHAR)',')
    {
        lp++;
        while( isspace(*lp) )
            lp++;
        *lplp = lp;
        if (lpConv)
            *lpConv = (int)TRUE;
    }
    else
    {
        *lplp = lp;
        if (lpConv)
            *lpConv = (int)(*lp=='\0');
    }
    return lValue;

} //*** ParseOffNumber
