#ifndef _OBJIDL_H
#define _OBJIDL_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <objfwd.h>

typedef struct  tagSTATSTG {
	LPOLESTR pwcsName;
	DWORD type;
	ULARGE_INTEGER cbSize;
	FILETIME mtime;
	FILETIME ctime;
	FILETIME atime;
	DWORD grfMode;
	DWORD grfLocksSupported;
	CLSID clsid;
	DWORD grfStateBits;
	DWORD reserved;
} STATSTG;
typedef enum tagSTGTY {
	STGTY_STORAGE=1,
	STGTY_STREAM,
	STGTY_LOCKBYTES,
	STGTY_PROPERTY
} STGTY;
typedef enum tagSTREAM_SEEK {
	STREAM_SEEK_SET,
	STREAM_SEEK_CUR,
	STREAM_SEEK_END
} STREAM_SEEK;
typedef struct tagINTERFACEINFO {
	LPUNKNOWN pUnk;
	IID iid;
	WORD wMethod;
} INTERFACEINFO,*LPINTERFACEINFO;
typedef enum tagCALLTYPE {
	CALLTYPE_TOPLEVEL=1,
	CALLTYPE_NESTED,
	CALLTYPE_ASYNC,
	CALLTYPE_TOPLEVEL_CALLPENDING,
	CALLTYPE_ASYNC_CALLPENDING
} CALLTYPE;
typedef enum tagPENDINGTYPE {
	PENDINGTYPE_TOPLEVEL=1,
	PENDINGTYPE_NESTED
} PENDINGTYPE;
typedef enum tagPENDINGMSG {
	PENDINGMSG_CANCELCALL=0,
	PENDINGMSG_WAITNOPROCESS,
	PENDINGMSG_WAITDEFPROCESS
} PENDINGMSG;
typedef OLECHAR **SNB;
typedef enum tagDATADIR	{
	DATADIR_GET=1,
	DATADIR_SET
} DATADIR;
typedef WORD CLIPFORMAT,*LPCLIPFORMAT;
typedef struct tagDVTARGETDEVICE {
	DWORD tdSize;
	WORD tdDriverNameOffset;
	WORD tdDeviceNameOffset;
	WORD tdPortNameOffset;
	WORD tdExtDevmodeOffset;
	BYTE tdData[1];
} DVTARGETDEVICE;
typedef struct tagFORMATETC {
	CLIPFORMAT cfFormat;
	DVTARGETDEVICE*ptd;
	DWORD dwAspect;
	LONG lindex;
	DWORD tymed;
} FORMATETC,*LPFORMATETC;
typedef struct tagRemSTGMEDIUM {
	DWORD tymed;
	DWORD dwHandleType;
	ULONG pData;
	unsigned long pUnkForRelease;
	unsigned long cbData;
	BYTE data[1];
} RemSTGMEDIUM;
typedef struct tagHLITEM {
	ULONG uHLID;
	LPWSTR pwzFriendlyName;
} HLITEM;
typedef struct tagSTATDATA {
	FORMATETC formatetc;
	DWORD grfAdvf;
	struct IAdviseSink *pAdvSink;
	DWORD dwConnection;
} STATDATA;
typedef struct tagSTATPROPSETSTG {
	FMTID fmtid;
	CLSID clsid;
	DWORD grfFlags;
	FILETIME mtime;
	FILETIME ctime;
	FILETIME atime;
} STATPROPSETSTG;
typedef enum tagEXTCONN {
	EXTCONN_STRONG=1,
	EXTCONN_WEAK=2,
	EXTCONN_CALLABLE=4
} EXTCONN;
typedef struct tagMULTI_QI {
	const IID *pIID;
	IUnknown *pItf;
	HRESULT	hr;
} MULTI_QI;
typedef struct _AUTH_IDENTITY {
	USHORT *User;
	ULONG UserLength;
	USHORT *Domain;
	ULONG DomainLength;
	USHORT *Password;
	ULONG PasswordLength;
	ULONG Flags;
} AUTH_IDENTITY;
typedef struct _COAUTHINFO{
	DWORD dwAuthnSvc;
	DWORD dwAuthzSvc;
	LPWSTR pwszServerPrincName;
	DWORD dwAuthnLevel;
	DWORD dwImpersonationLevel;
	AUTH_IDENTITY *pAuthIdentityData;
	DWORD dwCapabilities;
} COAUTHINFO;
typedef struct  _COSERVERINFO {
	DWORD dwReserved1;
	LPWSTR pwszName;
	COAUTHINFO *pAuthInfo;
	DWORD dwReserved2;
} COSERVERINFO;
typedef struct tagBIND_OPTS {
	DWORD cbStruct;
	DWORD grfFlags;
	DWORD grfMode;
	DWORD dwTickCountDeadline;
} BIND_OPTS,*LPBIND_OPTS;
typedef struct tagBIND_OPTS2 {
	DWORD cbStruct;
	DWORD grfFlags;
	DWORD grfMode;
	DWORD dwTickCountDeadline;
	DWORD dwTrackFlags;
	DWORD dwClassContext;
	LCID locale;
	COSERVERINFO *pServerInfo;
} BIND_OPTS2,*LPBIND_OPTS2;
typedef enum tagBIND_FLAGS {
	BIND_MAYBOTHERUSER=1,
	BIND_JUSTTESTEXISTENCE
} BIND_FLAGS;
typedef struct tagSTGMEDIUM {
	DWORD tymed;
	_ANONYMOUS_UNION union {
		HBITMAP hBitmap;
		PVOID hMetaFilePict;
		HENHMETAFILE hEnhMetaFile;
		HGLOBAL hGlobal;
		LPWSTR lpszFileName;
		LPSTREAM pstm;
		LPSTORAGE pstg;
	} DUMMYUNIONNAME;
	LPUNKNOWN pUnkForRelease;
} STGMEDIUM,*LPSTGMEDIUM;
typedef enum tagLOCKTYPE {
	LOCK_WRITE=1,
	LOCK_EXCLUSIVE=2,
	LOCK_ONLYONCE=4
} LOCKTYPE;
typedef unsigned long RPCOLEDATAREP;
typedef struct  tagRPCOLEMESSAGE {
	PVOID reserved1;
	RPCOLEDATAREP dataRepresentation;
	PVOID Buffer;
	ULONG cbBuffer;
	ULONG iMethod;
	PVOID reserved2[5];
	ULONG rpcFlags;
} RPCOLEMESSAGE, *PRPCOLEMESSAGE;
typedef enum tagMKSYS {
	MKSYS_NONE,
	MKSYS_GENERICCOMPOSITE,
	MKSYS_FILEMONIKER,
	MKSYS_ANTIMONIKER,
	MKSYS_ITEMMONIKER,
	MKSYS_POINTERMONIKER
} MKSYS;
typedef enum tagMKREDUCE {
	MKRREDUCE_ALL,
	MKRREDUCE_ONE=196608,
	MKRREDUCE_TOUSER=131072,
	MKRREDUCE_THROUGHUSER=65536
} MKRREDUCE;
typedef struct tagRemSNB {
	unsigned long ulCntStr;
	unsigned long ulCntChar;
	OLECHAR rgString[1];
} RemSNB;
typedef enum tagADVF {
	ADVF_NODATA=1,ADVF_PRIMEFIRST=2,ADVF_ONLYONCE=4,ADVF_DATAONSTOP=64,
	ADVFCACHE_NOHANDLER=8,ADVFCACHE_FORCEBUILTIN=16,ADVFCACHE_ONSAVE=32
} ADVF;
typedef enum tagTYMED {
	TYMED_HGLOBAL=1,TYMED_FILE=2,TYMED_ISTREAM=4,TYMED_ISTORAGE=8,
	TYMED_GDI=16,TYMED_MFPICT=32,TYMED_ENHMF=64,TYMED_NULL=0
} TYMED;
typedef enum tagSERVERCALL {
	SERVERCALL_ISHANDLED,SERVERCALL_REJECTED,SERVERCALL_RETRYLATER
} SERVERCALL;
typedef struct tagCAUB {
	ULONG cElems;
	unsigned char *pElems;
}CAUB;
typedef struct tagCAI {
	ULONG cElems;
	short *pElems;
}CAI;
typedef struct tagCAUI {
	ULONG cElems;
	USHORT *pElems;
}CAUI;
typedef struct tagCAL {
	ULONG cElems;
	long *pElems;
}CAL;
typedef struct tagCAUL {
	ULONG cElems;
	ULONG *pElems;
}CAUL;
typedef struct tagCAFLT {
	ULONG cElems;
	float *pElems;
}CAFLT;
typedef struct tagCADBL {
	ULONG cElems;
	double *pElems;
}CADBL;
typedef struct tagCACY {
	ULONG cElems;
	CY *pElems;
}CACY;
typedef struct tagCADATE {
	ULONG cElems;
	DATE *pElems;
}CADATE;
typedef struct tagCABSTR {
	ULONG cElems;
	BSTR  *pElems;
}CABSTR;
typedef struct tagCABSTRBLOB {
	ULONG cElems;
	BSTRBLOB *pElems;
}CABSTRBLOB;
typedef struct tagCABOOL {
	ULONG cElems;
	VARIANT_BOOL *pElems;
}CABOOL;
typedef struct tagCASCODE {
	ULONG cElems;
	SCODE *pElems;
}CASCODE;
typedef struct tagCAH {
	ULONG cElems;
	LARGE_INTEGER *pElems;
}CAH;
typedef struct tagCAUH {
	ULONG cElems;
	ULARGE_INTEGER *pElems;
}CAUH;
typedef struct tagCALPSTR {
	ULONG cElems;
	LPSTR *pElems;
}CALPSTR;
typedef struct tagCALPWSTR {
	ULONG cElems;
	LPWSTR *pElems;
}CALPWSTR;
typedef struct tagCAFILETIME {
	ULONG cElems;
	FILETIME *pElems;
}CAFILETIME;
typedef struct tagCACLIPDATA {
	ULONG cElems;
	CLIPDATA *pElems;
}CACLIPDATA;
typedef struct tagCACLSID {
	ULONG cElems;
	CLSID *pElems;
}CACLSID;
typedef struct tagPROPVARIANT *LPPROPVARIANT;
typedef struct tagCAPROPVARIANT {
	ULONG cElems;
	LPPROPVARIANT pElems;
}CAPROPVARIANT;
typedef struct tagPROPVARIANT {
	VARTYPE vt;
	WORD wReserved1;
	WORD wReserved2;
	WORD wReserved3;
	_ANONYMOUS_UNION union {
		CHAR cVal;
		UCHAR bVal;
		short iVal;
		USHORT uiVal;
		VARIANT_BOOL boolVal;
#if 0
/* bool is a standard type in C++, and a standard macro expanding
   to the _Bool type in C99 (see stdbool.h) */   
		_VARIANT_BOOL bool;
#endif
		long lVal;
		ULONG ulVal;
		float fltVal;
		SCODE scode;
		LARGE_INTEGER hVal;
		ULARGE_INTEGER uhVal;
		double dblVal;
		CY cyVal;
		DATE date;
		FILETIME filetime;
		CLSID *puuid;
		BLOB blob;
		CLIPDATA *pclipdata;
		LPSTREAM pStream;
		LPSTORAGE pStorage;
		BSTR bstrVal;
		BSTRBLOB bstrblobVal;
		LPSTR pszVal;
		LPWSTR pwszVal;
		CAUB caub;
		CAI cai;
		CAUI caui;
		CABOOL cabool;
		CAL cal;
		CAUL caul;
		CAFLT caflt;
		CASCODE cascode;
		CAH cah;
		CAUH cauh;
		CADBL cadbl;
		CACY cacy;
		CADATE cadate;
		CAFILETIME cafiletime;
		CACLSID cauuid;
		CACLIPDATA caclipdata;
		CABSTR cabstr;
		CABSTRBLOB cabstrblob;
		CALPSTR calpstr;
		CALPWSTR calpwstr;
		CAPROPVARIANT capropvar;
	} DUMMYUNIONNAME;
} PROPVARIANT;
typedef struct tagPROPSPEC {
	ULONG ulKind;
	_ANONYMOUS_UNION union {
		PROPID propid;
		LPOLESTR lpwstr;
	} DUMMYUNIONNAME;
}PROPSPEC;
typedef struct  tagSTATPROPSTG {
	LPOLESTR lpwstrName;
	PROPID propid;
	VARTYPE vt;
} STATPROPSTG;
typedef enum PROPSETFLAG {
	PROPSETFLAG_DEFAULT,PROPSETFLAG_NONSIMPLE,PROPSETFLAG_ANSI,
	PROPSETFLAG_UNBUFFERED=4
} PROPSETFLAG;
typedef struct tagSTORAGELAYOUT {
	DWORD LayoutType;
	OLECHAR* pwcsElementName;
	LARGE_INTEGER cOffset;
	LARGE_INTEGER cBytes;
} STORAGELAYOUT;
typedef struct tagSOLE_AUTHENTICATION_SERVICE {
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    OLECHAR *pPrincipalName;
    HRESULT hr;
} SOLE_AUTHENTICATION_SERVICE;
#define COLE_DEFAULT_PRINCIPAL ( ( OLECHAR* )-1 )
typedef enum tagEOLE_AUTHENTICATION_CAPABILITIES {
	EOAC_NONE = 0,
	EOAC_MUTUAL_AUTH = 0x1,
	EOAC_STATIC_CLOAKING = 0x20,
	EOAC_DYNAMIC_CLOAKING = 0x40,
	EOAC_ANY_AUTHORITY = 0x80,
	EOAC_MAKE_FULLSIC = 0x100,
	EOAC_DEFAULT = 0x800,
	EOAC_SECURE_REFS = 0x2,
	EOAC_ACCESS_CONTROL = 0x4,
	EOAC_APPID = 0x8,
	EOAC_DYNAMIC = 0x10,
	EOAC_REQUIRE_FULLSIC = 0x200,
	EOAC_AUTO_IMPERSONATE = 0x400,
	EOAC_NO_CUSTOM_MARSHAL = 0x2000,
	EOAC_DISABLE_AAA = 0x1000
} EOLE_AUTHENTICATION_CAPABILITIES;
typedef struct tagSOLE_AUTHENTICATION_INFO {
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    void* pAuthInfo;
} SOLE_AUTHENTICATION_INFO;
#define COLE_DEFAULT_AUTHINFO ( ( void* )-1 )
typedef struct tagSOLE_AUTHENTICATION_LIST {
    DWORD cAuthInfo;
    SOLE_AUTHENTICATION_INFO* aAuthInfo;
} SOLE_AUTHENTICATION_LIST;

EXTERN_C const FMTID FMTID_SummaryInformation;
EXTERN_C const FMTID FMTID_DocSummaryInformation;
EXTERN_C const FMTID FMTID_UserDefinedProperties;

DECLARE_ENUMERATOR(FORMATETC);
DECLARE_ENUMERATOR(HLITEM);
DECLARE_ENUMERATOR(STATDATA);
DECLARE_ENUMERATOR(STATPROPSETSTG);
DECLARE_ENUMERATOR(STATPROPSTG);
DECLARE_ENUMERATOR(STATSTG);
DECLARE_ENUMERATOR_(IEnumString,LPOLESTR);
DECLARE_ENUMERATOR_(IEnumMoniker,interface IMoniker*);
DECLARE_ENUMERATOR_(IEnumUnknown,IUnknown*);

EXTERN_C const IID IID_ISequentialStream;
#define INTERFACE ISequentialStream
DECLARE_INTERFACE_(ISequentialStream,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Read)(THIS_ void*,ULONG,ULONG*) PURE;
	STDMETHOD(Write)(THIS_ void const*,ULONG,ULONG*) PURE;
};

EXTERN_C const IID IID_IStream;
#define INTERFACE IStream
DECLARE_INTERFACE_(IStream,ISequentialStream)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Read)(THIS_ void*,ULONG,ULONG*) PURE;
	STDMETHOD(Write)(THIS_ void const*,ULONG,ULONG*) PURE;
	STDMETHOD(Seek)(THIS_ LARGE_INTEGER,DWORD,ULARGE_INTEGER*) PURE;
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER) PURE;
	STDMETHOD(CopyTo)(THIS_ IStream*,ULARGE_INTEGER,ULARGE_INTEGER*,ULARGE_INTEGER*) PURE;
	STDMETHOD(Commit)(THIS_ DWORD) PURE;
	STDMETHOD(Revert)(THIS) PURE;
	STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER,ULARGE_INTEGER,DWORD) PURE;
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER,ULARGE_INTEGER,DWORD) PURE;
	STDMETHOD(Stat)(THIS_ STATSTG*,DWORD) PURE;
	STDMETHOD(Clone)(THIS_ LPSTREAM*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMarshal;
#define INTERFACE IMarshal
DECLARE_INTERFACE_(IMarshal,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetUnmarshalClass) (THIS_ REFIID,PVOID,DWORD,PVOID,DWORD,CLSID*) PURE;
	STDMETHOD(GetMarshalSizeMax) (THIS_ REFIID,PVOID,DWORD,PVOID,PDWORD,ULONG*) PURE;
	STDMETHOD(MarshalInterface) (THIS_ IStream*,REFIID,PVOID,DWORD,PVOID,DWORD) PURE;
	STDMETHOD(UnmarshalInterface) (THIS_ IStream*,REFIID,void**) PURE;
	STDMETHOD(ReleaseMarshalData) (THIS_ IStream*) PURE;
	STDMETHOD(DisconnectObject) (THIS_ DWORD) PURE;
};

EXTERN_C const IID IID_IStdMarshalInfo;
#define INTERFACE IStdMarshalInfo
DECLARE_INTERFACE_(IStdMarshalInfo,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassForHandler)(THIS_ DWORD,PVOID,CLSID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMalloc;
#define INTERFACE IMalloc
DECLARE_INTERFACE_(IMalloc,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(void*,Alloc)(THIS_ ULONG) PURE;
	STDMETHOD_(void*,Realloc)(THIS_ void*,ULONG) PURE;
	STDMETHOD_(void,Free)(THIS_ void*) PURE;
	STDMETHOD_(ULONG,GetSize)(THIS_ void*) PURE;
	STDMETHOD_(int,DidAlloc)(THIS_ void*) PURE;
	STDMETHOD_(void,HeapMinimize)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMallocSpy;
#define INTERFACE IMallocSpy
DECLARE_INTERFACE_(IMallocSpy,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(ULONG,PreAlloc)(THIS_ ULONG) PURE;
	STDMETHOD_(void*,PostAlloc)(THIS_ void*) PURE;
	STDMETHOD_(void*,PreFree)(THIS_ void*,BOOL) PURE;
	STDMETHOD_(void,PostFree)(THIS_ BOOL) PURE;
	STDMETHOD_(ULONG,PreRealloc)(THIS_ void*,ULONG,void**,BOOL) PURE;
	STDMETHOD_(void*,PostRealloc)(THIS_ void*,BOOL) PURE;
	STDMETHOD_(void*,PreGetSize)(THIS_ void*,BOOL) PURE;
	STDMETHOD_(ULONG,PostGetSize)(THIS_ ULONG,BOOL) PURE;
	STDMETHOD_(void*,PreDidAlloc)(THIS_ void*,BOOL) PURE;
	STDMETHOD_(int,PostDidAlloc)(THIS_ void*,BOOL,int) PURE;
	STDMETHOD_(void,PreHeapMinimize)(THIS) PURE;
	STDMETHOD_(void,PostHeapMinimize)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMessageFilter;
#define INTERFACE IMessageFilter
DECLARE_INTERFACE_(IMessageFilter,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(DWORD,HandleInComingCall)(THIS_ DWORD,HTASK,DWORD,LPINTERFACEINFO) PURE;
	STDMETHOD_(DWORD,RetryRejectedCall)(THIS_ HTASK,DWORD,DWORD) PURE;
	STDMETHOD_(DWORD,MessagePending)(THIS_ HTASK,DWORD,DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersist;
#define INTERFACE IPersist
DECLARE_INTERFACE_(IPersist,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ CLSID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersistStream;
#define INTERFACE IPersistStream
DECLARE_INTERFACE_(IPersistStream,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ IStream*) PURE;
	STDMETHOD(Save)(THIS_ IStream*,BOOL) PURE;
	STDMETHOD(GetSizeMax)(THIS_ PULARGE_INTEGER) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IRunningObjectTable;
#define INTERFACE IRunningObjectTable
DECLARE_INTERFACE_(IRunningObjectTable,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Register)(THIS_ DWORD,LPUNKNOWN,LPMONIKER,PDWORD) PURE;
	STDMETHOD(Revoke)(THIS_ DWORD) PURE;
	STDMETHOD(IsRunning)(THIS_ LPMONIKER) PURE;
	STDMETHOD(GetObject)(THIS_ LPMONIKER,LPUNKNOWN*) PURE;
	STDMETHOD(NoteChangeTime)(THIS_ DWORD,LPFILETIME) PURE;
	STDMETHOD(GetTimeOfLastChange)(THIS_ LPMONIKER,LPFILETIME) PURE;
	STDMETHOD(EnumRunning)(THIS_ IEnumMoniker**) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IBindCtx;
#define INTERFACE IBindCtx
DECLARE_INTERFACE_(IBindCtx,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(RegisterObjectBound)(THIS_ LPUNKNOWN) PURE;
	STDMETHOD(RevokeObjectBound)(THIS_ LPUNKNOWN) PURE;
	STDMETHOD(ReleaseBoundObjects)(THIS) PURE;
	STDMETHOD(SetBindOptions)(THIS_ LPBIND_OPTS) PURE;
	STDMETHOD(GetBindOptions)(THIS_ LPBIND_OPTS) PURE;
	STDMETHOD(GetRunningObjectTable)(THIS_ IRunningObjectTable**) PURE;
	STDMETHOD(RegisterObjectParam)(THIS_ LPOLESTR,IUnknown*) PURE;
	STDMETHOD(GetObjectParam)(THIS_ LPOLESTR,IUnknown**) PURE;
	STDMETHOD(EnumObjectParam)(THIS_ IEnumString**) PURE;
	STDMETHOD(RevokeObjectParam)(THIS_ LPOLESTR) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMoniker;
#define INTERFACE IMoniker
DECLARE_INTERFACE_(IMoniker,IPersistStream)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ LPCLSID) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ IStream*) PURE;
	STDMETHOD(Save)(THIS_ IStream*,BOOL) PURE;
	STDMETHOD(GetSizeMax)(THIS_ PULARGE_INTEGER) PURE;
	STDMETHOD(BindToObject)(THIS_ IBindCtx*,IMoniker*,REFIID,PVOID*) PURE;
	STDMETHOD(BindToStorage)(THIS_ IBindCtx*,IMoniker*,REFIID,PVOID*) PURE;
	STDMETHOD(Reduce)(THIS_ IBindCtx*,DWORD,IMoniker**,IMoniker**) PURE;
	STDMETHOD(ComposeWith)(THIS_ IMoniker*,BOOL,IMoniker**) PURE;
	STDMETHOD(Enum)(THIS_ BOOL,IEnumMoniker**) PURE;
	STDMETHOD(IsEqual)(THIS_ IMoniker*) PURE;
	STDMETHOD(Hash)(THIS_ PDWORD) PURE;
	STDMETHOD(IsRunning)(THIS_ IBindCtx*,IMoniker*,IMoniker*) PURE;
	STDMETHOD(GetTimeOfLastChange)(THIS_ IBindCtx*,IMoniker*,LPFILETIME) PURE;
	STDMETHOD(Inverse)(THIS_ IMoniker**) PURE;
	STDMETHOD(CommonPrefixWith)(THIS_ IMoniker*,IMoniker**) PURE;
	STDMETHOD(RelativePathTo)(THIS_ IMoniker*,IMoniker**) PURE;
	STDMETHOD(GetDisplayName)(THIS_ IBindCtx*,IMoniker*,LPOLESTR*) PURE;
	STDMETHOD(ParseDisplayName)(THIS_ IBindCtx*,IMoniker*,LPOLESTR,ULONG*,IMoniker**) PURE;
	STDMETHOD(IsSystemMoniker)(THIS_ PDWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersistStorage;
#define INTERFACE IPersistStorage
DECLARE_INTERFACE_(IPersistStorage,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ CLSID*) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(InitNew)(THIS_ LPSTORAGE) PURE;
	STDMETHOD(Load)(THIS_ LPSTORAGE) PURE;
	STDMETHOD(Save)(THIS_ LPSTORAGE,BOOL) PURE;
	STDMETHOD(SaveCompleted)(THIS_ LPSTORAGE) PURE;
	STDMETHOD(HandsOffStorage)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPersistFile;
#define INTERFACE IPersistFile
DECLARE_INTERFACE_(IPersistFile,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassID)(THIS_ CLSID*) PURE;
	STDMETHOD(IsDirty)(THIS) PURE;
	STDMETHOD(Load)(THIS_ LPCOLESTR,DWORD) PURE;
	STDMETHOD(Save)(THIS_ LPCOLESTR,BOOL) PURE;
	STDMETHOD(SaveCompleted)(THIS_ LPCOLESTR) PURE;
	STDMETHOD(GetCurFile)(THIS_ LPOLESTR*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IAdviseSink;
#define INTERFACE IAdviseSink
DECLARE_INTERFACE_(IAdviseSink,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC*,STGMEDIUM*) PURE;
	STDMETHOD_(void,OnViewChange)(THIS_ DWORD,LONG) PURE;
	STDMETHOD_(void,OnRename)(THIS_ IMoniker*) PURE;
	STDMETHOD_(void,OnSave)(THIS) PURE;
	STDMETHOD_(void,OnClose)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IAdviseSink2;
#define INTERFACE IAdviseSink2
DECLARE_INTERFACE_(IAdviseSink2,IAdviseSink)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC*,STGMEDIUM*) PURE;
	STDMETHOD_(void,OnViewChange)(THIS_ DWORD,LONG) PURE;
	STDMETHOD_(void,OnRename)(THIS_ IMoniker*) PURE;
	STDMETHOD_(void,OnSave)(THIS) PURE;
	STDMETHOD_(void,OnClose)(THIS) PURE;
	STDMETHOD_(void,OnLinkSrcChange)(THIS_ IMoniker*);
};
#undef INTERFACE

EXTERN_C const IID IID_IDataObject;
#define INTERFACE IDataObject
DECLARE_INTERFACE_(IDataObject,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetData)(THIS_ FORMATETC*,STGMEDIUM*) PURE;
	STDMETHOD(GetDataHere)(THIS_ FORMATETC*,STGMEDIUM*) PURE;
	STDMETHOD(QueryGetData)(THIS_ FORMATETC*) PURE;
	STDMETHOD(GetCanonicalFormatEtc)(THIS_ FORMATETC*,FORMATETC*) PURE;
	STDMETHOD(SetData)(THIS_ FORMATETC*,STGMEDIUM*,BOOL) PURE;
	STDMETHOD(EnumFormatEtc)(THIS_ DWORD,IEnumFORMATETC**) PURE;
	STDMETHOD(DAdvise)(THIS_ FORMATETC*,DWORD,IAdviseSink*,PDWORD) PURE;
	STDMETHOD(DUnadvise)(THIS_ DWORD) PURE;
	STDMETHOD(EnumDAdvise)(THIS_ IEnumSTATDATA**) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IDataAdviseHolder;
#define INTERFACE IDataAdviseHolder
DECLARE_INTERFACE_(IDataAdviseHolder,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Advise)(THIS_ IDataObject*,FORMATETC*,DWORD,IAdviseSink*,PDWORD) PURE;
	STDMETHOD(Unadvise)(THIS_ DWORD) PURE;
	STDMETHOD(EnumAdvise)(THIS_ IEnumSTATDATA**) PURE;
	STDMETHOD(SendOnDataChange)(THIS_ IDataObject*,DWORD,DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IStorage;
#define INTERFACE IStorage
DECLARE_INTERFACE_(IStorage,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(CreateStream)(THIS_ LPCWSTR,DWORD,DWORD,DWORD,IStream**) PURE;
	STDMETHOD(OpenStream)(THIS_ LPCWSTR,PVOID,DWORD,DWORD,IStream**) PURE;
	STDMETHOD(CreateStorage)(THIS_ LPCWSTR,DWORD,DWORD,DWORD,IStorage**) PURE;
	STDMETHOD(OpenStorage)(THIS_ LPCWSTR,IStorage*,DWORD,SNB,DWORD,IStorage**) PURE;
	STDMETHOD(CopyTo)(THIS_ DWORD,IID const*,SNB,IStorage*) PURE;
	STDMETHOD(MoveElementTo)(THIS_ LPCWSTR,IStorage*,LPCWSTR,DWORD) PURE;
	STDMETHOD(Commit)(THIS_ DWORD) PURE;
	STDMETHOD(Revert)(THIS) PURE;
	STDMETHOD(EnumElements)(THIS_ DWORD,PVOID,DWORD,IEnumSTATSTG**) PURE;
	STDMETHOD(DestroyElement)(THIS_ LPCWSTR) PURE;
	STDMETHOD(RenameElement)(THIS_ LPCWSTR,LPCWSTR) PURE;
	STDMETHOD(SetElementTimes)(THIS_ LPCWSTR,FILETIME const*,FILETIME const*,FILETIME const*) PURE;
	STDMETHOD(SetClass)(THIS_ REFCLSID) PURE;
	STDMETHOD(SetStateBits)(THIS_ DWORD,DWORD) PURE;
	STDMETHOD(Stat)(THIS_ STATSTG*,DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IRootStorage;
#define INTERFACE IRootStorage
DECLARE_INTERFACE_(IRootStorage,IPersist)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(SwitchToFile)(THIS_ LPOLESTR) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IRpcChannelBuffer;
#define INTERFACE IRpcChannelBuffer
DECLARE_INTERFACE_(IRpcChannelBuffer,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetBuffer)(THIS_ RPCOLEMESSAGE*,REFIID) PURE;
	STDMETHOD(SendReceive)(THIS_ RPCOLEMESSAGE*,PULONG) PURE;
	STDMETHOD(FreeBuffer)(THIS_ RPCOLEMESSAGE*) PURE;
	STDMETHOD(GetDestCtx)(THIS_ PDWORD,PVOID*) PURE;
	STDMETHOD(IsConnected)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IRpcProxyBuffer;
#define INTERFACE IRpcProxyBuffer
DECLARE_INTERFACE_(IRpcProxyBuffer,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Connect)(THIS_ IRpcChannelBuffer*) PURE;
	STDMETHOD_(void,Disconnect)(THIS) PURE;

};
#undef INTERFACE

EXTERN_C const IID IID_IRpcStubBuffer;
#define INTERFACE IRpcStubBuffer
DECLARE_INTERFACE_(IRpcStubBuffer,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Connect)(THIS_ LPUNKNOWN) PURE;
	STDMETHOD_(void,Disconnect)(THIS) PURE;
	STDMETHOD(Invoke)(THIS_ RPCOLEMESSAGE*,LPRPCSTUBBUFFER) PURE;
	STDMETHOD_(LPRPCSTUBBUFFER,IsIIDSupported)(THIS_ REFIID) PURE;
	STDMETHOD_(ULONG,CountRefs)(THIS) PURE;
	STDMETHOD(DebugServerQueryInterface)(THIS_ PVOID*) PURE;
	STDMETHOD(DebugServerRelease)(THIS_ PVOID) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPSFactoryBuffer;
#define INTERFACE IPSFactoryBuffer
DECLARE_INTERFACE_(IPSFactoryBuffer,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(CreateProxy)(THIS_ LPUNKNOWN,REFIID,LPRPCPROXYBUFFER*,PVOID*) PURE;
	STDMETHOD(CreateStub)(THIS_ REFIID,LPUNKNOWN,LPRPCSTUBBUFFER*) PURE;
};
#undef INTERFACE
typedef interface IPSFactoryBuffer *LPPSFACTORYBUFFER;

EXTERN_C const IID IID_ILockBytes;
#define INTERFACE ILockBytes
DECLARE_INTERFACE_(ILockBytes,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ReadAt)(THIS_ ULARGE_INTEGER,PVOID,ULONG,ULONG*) PURE;
	STDMETHOD(WriteAt)(THIS_ ULARGE_INTEGER,PCVOID,ULONG,ULONG*) PURE;
	STDMETHOD(Flush)(THIS) PURE;
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER) PURE;
	STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER,ULARGE_INTEGER,DWORD) PURE;
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER,ULARGE_INTEGER,DWORD) PURE;
	STDMETHOD(Stat)(THIS_ STATSTG*,DWORD) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IExternalConnection;
#define INTERFACE IExternalConnection
DECLARE_INTERFACE_(IExternalConnection,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(AddConnection)(THIS_ DWORD,DWORD) PURE;
	STDMETHOD(ReleaseConnection)(THIS_ DWORD,DWORD,BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IRunnableObject;
#define INTERFACE IRunnableObject
DECLARE_INTERFACE_(IRunnableObject,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetRunningClass)(THIS_ LPCLSID) PURE;
	STDMETHOD(Run)(THIS_ LPBC) PURE;
	STDMETHOD_(BOOL,IsRunning)(THIS) PURE;
	STDMETHOD(LockRunning)(THIS_ BOOL,BOOL) PURE;
	STDMETHOD(SetContainedObject)(THIS_ BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IROTData;
#define INTERFACE IROTData
DECLARE_INTERFACE_(IROTData,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetComparisonData)(THIS_ PVOID,ULONG,PULONG) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IChannelHook;
#define INTERFACE IChannelHook
DECLARE_INTERFACE_(IChannelHook,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD_(void,ClientGetSize)(THIS_ REFGUID,REFIID,PULONG) PURE;
	STDMETHOD_(void,ClientFillBuffer)(THIS_ REFGUID,REFIID,PULONG,PVOID) PURE;
	STDMETHOD_(void,ClientNotify)(THIS_ REFGUID,REFIID,ULONG,PVOID,DWORD,HRESULT) PURE;
	STDMETHOD_(void,ServerNotify)(THIS_ REFGUID,REFIID,ULONG,PVOID,DWORD) PURE;
	STDMETHOD_(void,ServerGetSize)(THIS_ REFGUID,REFIID,HRESULT,PULONG) PURE;
	STDMETHOD_(void,ServerFillBuffer)(THIS_ REFGUID,REFIID,PULONG,PVOID,HRESULT) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertyStorage;
#define INTERFACE IPropertyStorage
DECLARE_INTERFACE_(IPropertyStorage,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(ReadMultiple)(THIS_ ULONG,PROPSPEC const*,PROPVARIANT*) PURE;
	STDMETHOD(WriteMultiple)(THIS_ ULONG,PROPSPEC const*,PROPVARIANT*,PROPID) PURE;
	STDMETHOD(DeleteMultiple)(THIS_ ULONG,PROPSPEC const*) PURE;
	STDMETHOD(ReadPropertyNames)(THIS_ ULONG,PROPID const*,LPWSTR*) PURE;
	STDMETHOD(WritePropertyNames)(THIS_ ULONG,PROPID const*,LPWSTR const*) PURE;
	STDMETHOD(DeletePropertyNames)(THIS_ ULONG,PROPID const*) PURE;
	STDMETHOD(SetClass)(THIS_ REFCLSID) PURE;
	STDMETHOD(Commit)(THIS_ DWORD) PURE;
	STDMETHOD(Revert)(THIS) PURE;
	STDMETHOD(Enum)(THIS_ IEnumSTATPROPSTG**) PURE;
	STDMETHOD(Stat)(THIS_ STATPROPSTG*) PURE;
	STDMETHOD(SetTimes)(THIS_ FILETIME const*,FILETIME const*,FILETIME const*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IPropertySetStorage;
#define INTERFACE IPropertySetStorage
DECLARE_INTERFACE_(IPropertySetStorage,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Create)(THIS_ REFFMTID,CLSID*,DWORD,DWORD,LPPROPERTYSTORAGE*) PURE;
	STDMETHOD(Open)(THIS_ REFFMTID,DWORD,LPPROPERTYSTORAGE*) PURE;
	STDMETHOD(Delete)(THIS_ REFFMTID) PURE;
	STDMETHOD(Enum)(THIS_ IEnumSTATPROPSETSTG**) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IClientSecurity;
#define INTERFACE IClientSecurity
DECLARE_INTERFACE_(IClientSecurity,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(QueryBlanket)(THIS_ PVOID,PDWORD,PDWORD,OLECHAR**,PDWORD,PDWORD,RPC_AUTH_IDENTITY_HANDLE**,PDWORD*) PURE;
	STDMETHOD(SetBlanket)(THIS_ PVOID,DWORD,DWORD,LPWSTR,DWORD,DWORD,RPC_AUTH_IDENTITY_HANDLE*,DWORD) PURE;
	STDMETHOD(CopyProxy)(THIS_ LPUNKNOWN,LPUNKNOWN*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IServerSecurity;
#define INTERFACE IServerSecurity
DECLARE_INTERFACE_(IServerSecurity,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(QueryBlanket)(THIS_ PDWORD,PDWORD,OLECHAR**,PDWORD,PDWORD,RPC_AUTHZ_HANDLE*,PDWORD*) PURE;
	STDMETHOD(ImpersonateClient)(THIS) PURE;
	STDMETHOD(RevertToSelf)(THIS) PURE;
	STDMETHOD(IsImpersonating)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IClassActivator;
#define INTERFACE IClassActivator
DECLARE_INTERFACE_(IClassActivator,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(GetClassObject)(THIS_ REFCLSID,DWORD,LCID,REFIID,PVOID*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IFillLockBytes;
#define INTERFACE IFillLockBytes
DECLARE_INTERFACE_(IFillLockBytes,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(FillAppend)(THIS_ void const*,ULONG,PULONG) PURE;
	STDMETHOD(FillAt)(THIS_ ULARGE_INTEGER,void const*,ULONG,PULONG) PURE;
	STDMETHOD(SetFillSize)(THIS_ ULARGE_INTEGER) PURE;
	STDMETHOD(Terminate)(THIS_ BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IProgressNotify;
#define INTERFACE IProgressNotify
DECLARE_INTERFACE_(IProgressNotify,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(OnProgress)(THIS_ DWORD,DWORD,BOOL,BOOL) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_ILayoutStorage;
#define INTERFACE ILayoutStorage
DECLARE_INTERFACE_(ILayoutStorage,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(LayoutScript)(THIS_ STORAGELAYOUT*,DWORD,DWORD) PURE;
	STDMETHOD(BeginMonitor)(THIS) PURE;
	STDMETHOD(EndMonitor)(THIS) PURE;
	STDMETHOD(ReLayoutDocfile)(THIS_ OLECHAR*) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IGlobalInterfaceTable;
#define INTERFACE IGlobalInterfaceTable
DECLARE_INTERFACE_(IGlobalInterfaceTable,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(RegisterInterfaceInGlobal)(THIS_ IUnknown*,REFIID,DWORD*) PURE;
	STDMETHOD(RevokeInterfaceFromGlobal)(THIS_ DWORD) PURE;
	STDMETHOD(GetInterfaceFromGlobal)(THIS_ DWORD,REFIID,void**) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IGlobalInterfaceTable_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IGlobalInterfaceTable_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IGlobalInterfaceTable_Release(T) (T)->lpVtbl->Release(T)
#define IGlobalInterfaceTable_RegisterInterfaceInGlobal(T,a,b,c) (T)->lpVtbl->RegisterInterfaceInGlobal(T,a,b,c)
#define IGlobalInterfaceTable_RevokeInterfaceFromGlobal(T,a) (T)->lpVtbl->RevokeInterfaceFromGlobal(T,a)
#define IGlobalInterfaceTable_GetInterfaceFromGlobal(T,a,b,c) (T)->lpVtbl->GetInterfaceFromGlobal(T,a,b,c)
#endif

HRESULT STDMETHODCALLTYPE IMarshal_GetUnmarshalClass_Proxy(IMarshal*,REFIID,void*,DWORD,void*,DWORD,CLSID*);
void STDMETHODCALLTYPE IMarshal_GetUnmarshalClass_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMarshal_GetMarshalSizeMax_Proxy(IMarshal*,REFIID,void*,DWORD,void*,DWORD,DWORD*);
void STDMETHODCALLTYPE IMarshal_GetMarshalSizeMax_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMarshal_MarshalInterface_Proxy(IMarshal*,IStream*,REFIID,void*,DWORD,void*,DWORD);
void STDMETHODCALLTYPE IMarshal_MarshalInterface_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMarshal_UnmarshalInterface_Proxy(IMarshal*,IStream*,REFIID,void**);
void STDMETHODCALLTYPE IMarshal_UnmarshalInterface_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMarshal_ReleaseMarshalData_Proxy(IMarshal*,IStream*);
void STDMETHODCALLTYPE IMarshal_ReleaseMarshalData_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMarshal_DisconnectObject_Proxy(IMarshal*,DWORD);
void STDMETHODCALLTYPE IMarshal_DisconnectObject_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMalloc_Alloc_Proxy(IMalloc*,ULONG);
void STDMETHODCALLTYPE IMalloc_Alloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMalloc_Realloc_Proxy(IMalloc*,void*,ULONG);
void STDMETHODCALLTYPE IMalloc_Realloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IMalloc_Free_Proxy(IMalloc*,void*);
void STDMETHODCALLTYPE IMalloc_Free_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
ULONG STDMETHODCALLTYPE IMalloc_GetSize_Proxy(IMalloc*,void*);
void STDMETHODCALLTYPE IMalloc_GetSize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
int STDMETHODCALLTYPE IMalloc_DidAlloc_Proxy(IMalloc*,void*);
void STDMETHODCALLTYPE IMalloc_DidAlloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IMalloc_HeapMinimize_Proxy(IMalloc*);
void STDMETHODCALLTYPE IMalloc_HeapMinimize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
ULONG STDMETHODCALLTYPE IMallocSpy_PreAlloc_Proxy(IMallocSpy*,ULONG cbRequest);
void STDMETHODCALLTYPE IMallocSpy_PreAlloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMallocSpy_PostAlloc_Proxy(IMallocSpy*,void*);
void STDMETHODCALLTYPE IMallocSpy_PostAlloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMallocSpy_PreFree_Proxy(IMallocSpy*,void*,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PreFree_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IMallocSpy_PostFree_Proxy(IMallocSpy*,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PostFree_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
ULONG STDMETHODCALLTYPE IMallocSpy_PreRealloc_Proxy(IMallocSpy*,void*,ULONG,void**,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PreRealloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMallocSpy_PostRealloc_Proxy(IMallocSpy*,void*,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PostRealloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMallocSpy_PreGetSize_Proxy(IMallocSpy*,void*,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PreGetSize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
ULONG STDMETHODCALLTYPE IMallocSpy_PostGetSize_Proxy(IMallocSpy*,ULONG,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PostGetSize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void* STDMETHODCALLTYPE IMallocSpy_PreDidAlloc_Proxy(IMallocSpy*,void*,BOOL);
void STDMETHODCALLTYPE IMallocSpy_PreDidAlloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
int STDMETHODCALLTYPE IMallocSpy_PostDidAlloc_Proxy(IMallocSpy*,void*,BOOL,int);
void STDMETHODCALLTYPE IMallocSpy_PostDidAlloc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IMallocSpy_PreHeapMinimize_Proxy(IMallocSpy* );
void STDMETHODCALLTYPE IMallocSpy_PreHeapMinimize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IMallocSpy_PostHeapMinimize_Proxy(IMallocSpy*);
void STDMETHODCALLTYPE IMallocSpy_PostHeapMinimize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStdMarshalInfo_GetClassForHandler_Proxy(IStdMarshalInfo*,DWORD,void*,CLSID*);
void STDMETHODCALLTYPE IStdMarshalInfo_GetClassForHandler_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
DWORD STDMETHODCALLTYPE IExternalConnection_AddConnection_Proxy(IExternalConnection*,DWORD,DWORD);
void STDMETHODCALLTYPE IExternalConnection_AddConnection_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
DWORD STDMETHODCALLTYPE IExternalConnection_ReleaseConnection_Proxy(IExternalConnection*,DWORD,DWORD,BOOL);
void STDMETHODCALLTYPE IExternalConnection_ReleaseConnection_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumUnknown_RemoteNext_Proxy(IEnumUnknown*,ULONG,IUnknown**,ULONG*);
void STDMETHODCALLTYPE IEnumUnknown_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumUnknown_Skip_Proxy(IEnumUnknown*,ULONG);
void STDMETHODCALLTYPE IEnumUnknown_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumUnknown_Reset_Proxy(IEnumUnknown* );
void STDMETHODCALLTYPE IEnumUnknown_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumUnknown_Clone_Proxy(IEnumUnknown*,IEnumUnknown**);
void STDMETHODCALLTYPE IEnumUnknown_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_RegisterObjectBound_Proxy(IBindCtx*,IUnknown*punk);
void STDMETHODCALLTYPE IBindCtx_RegisterObjectBound_Stub(IRpcStubBuffer*,IRpcChannelBuffer*_pRpcChannelBuffer,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_RevokeObjectBound_Proxy(IBindCtx*,IUnknown*punk);
void STDMETHODCALLTYPE IBindCtx_RevokeObjectBound_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_ReleaseBoundObjects_Proxy(IBindCtx*);
void STDMETHODCALLTYPE IBindCtx_ReleaseBoundObjects_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_SetBindOptions_Proxy(IBindCtx*,BIND_OPTS*);
void STDMETHODCALLTYPE IBindCtx_SetBindOptions_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_GetBindOptions_Proxy(IBindCtx*,BIND_OPTS*pbindopts);
void STDMETHODCALLTYPE IBindCtx_GetBindOptions_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_GetRunningObjectTable_Proxy(IBindCtx*,IRunningObjectTable**);
void STDMETHODCALLTYPE IBindCtx_GetRunningObjectTable_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_RegisterObjectParam_Proxy(IBindCtx*,LPCSTR,IUnknown*);
void STDMETHODCALLTYPE IBindCtx_RegisterObjectParam_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_GetObjectParam_Proxy(IBindCtx*,LPCSTR,IUnknown**);
void STDMETHODCALLTYPE IBindCtx_GetObjectParam_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_EnumObjectParam_Proxy(IBindCtx*,IEnumString**);
void STDMETHODCALLTYPE IBindCtx_EnumObjectParam_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IBindCtx_RevokeObjectParam_Proxy(IBindCtx*,LPCSTR);
void STDMETHODCALLTYPE IBindCtx_RevokeObjectParam_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumMoniker_RemoteNext_Proxy(IEnumMoniker*,ULONG,IMoniker**,ULONG*);
void STDMETHODCALLTYPE IEnumMoniker_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumMoniker_Skip_Proxy(IEnumMoniker*,ULONG);
void STDMETHODCALLTYPE IEnumMoniker_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumMoniker_Reset_Proxy(IEnumMoniker*);
void STDMETHODCALLTYPE IEnumMoniker_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumMoniker_Clone_Proxy(IEnumMoniker*,IEnumMoniker**);
void STDMETHODCALLTYPE IEnumMoniker_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunnableObject_GetRunningClass_Proxy(IRunnableObject*,LPCLSID);
void STDMETHODCALLTYPE IRunnableObject_GetRunningClass_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunnableObject_Run_Proxy(IRunnableObject*,LPBINDCTX);
void STDMETHODCALLTYPE IRunnableObject_Run_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
BOOL STDMETHODCALLTYPE IRunnableObject_IsRunning_Proxy(IRunnableObject*);
void STDMETHODCALLTYPE IRunnableObject_IsRunning_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunnableObject_LockRunning_Proxy(IRunnableObject*,BOOL,BOOL);
void STDMETHODCALLTYPE IRunnableObject_LockRunning_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunnableObject_SetContainedObject_Proxy(IRunnableObject*,BOOL);
void STDMETHODCALLTYPE IRunnableObject_SetContainedObject_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_Register_Proxy(IRunningObjectTable*,DWORD,IUnknown*,IMoniker*,DWORD*);
void STDMETHODCALLTYPE IRunningObjectTable_Register_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_Revoke_Proxy(IRunningObjectTable*,DWORD);
void STDMETHODCALLTYPE IRunningObjectTable_Revoke_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_IsRunning_Proxy(IRunningObjectTable*,IMoniker*);
void STDMETHODCALLTYPE IRunningObjectTable_IsRunning_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_GetObject_Proxy(IRunningObjectTable*,IMoniker*,IUnknown**);
void STDMETHODCALLTYPE IRunningObjectTable_GetObject_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_NoteChangeTime_Proxy(IRunningObjectTable*,DWORD,FILETIME*);
void STDMETHODCALLTYPE IRunningObjectTable_NoteChangeTime_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_GetTimeOfLastChange_Proxy(IRunningObjectTable*,IMoniker*,FILETIME*);
void STDMETHODCALLTYPE IRunningObjectTable_GetTimeOfLastChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRunningObjectTable_EnumRunning_Proxy(IRunningObjectTable*,IEnumMoniker**);
void STDMETHODCALLTYPE IRunningObjectTable_EnumRunning_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersist_GetClassID_Proxy(IPersist*,CLSID*);
void STDMETHODCALLTYPE IPersist_GetClassID_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStream_IsDirty_Proxy(IPersistStream*);
void STDMETHODCALLTYPE IPersistStream_IsDirty_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStream_Load_Proxy(IPersistStream*,IStream*);
void STDMETHODCALLTYPE IPersistStream_Load_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStream_Save_Proxy(IPersistStream*,IStream*,BOOL);
void STDMETHODCALLTYPE IPersistStream_Save_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStream_GetSizeMax_Proxy(IPersistStream*,ULARGE_INTEGER*);
void STDMETHODCALLTYPE IPersistStream_GetSizeMax_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_RemoteBindToObject_Proxy(IMoniker*,IBindCtx*,IMoniker*,REFIID,IUnknown**);
void STDMETHODCALLTYPE IMoniker_RemoteBindToObject_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_RemoteBindToStorage_Proxy(IMoniker*,IBindCtx*,IMoniker*,REFIID,IUnknown**);
void STDMETHODCALLTYPE IMoniker_RemoteBindToStorage_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_Reduce_Proxy(IMoniker*,IBindCtx*,DWORD,IMoniker**,IMoniker**);
void STDMETHODCALLTYPE IMoniker_Reduce_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_ComposeWith_Proxy(IMoniker*,IMoniker*,BOOL,IMoniker**);
void STDMETHODCALLTYPE IMoniker_ComposeWith_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_Enum_Proxy(IMoniker*,BOOL,IEnumMoniker**);
void STDMETHODCALLTYPE IMoniker_Enum_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_IsEqual_Proxy(IMoniker*,IMoniker*);
void STDMETHODCALLTYPE IMoniker_IsEqual_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_Hash_Proxy(IMoniker*,DWORD*);
void STDMETHODCALLTYPE IMoniker_Hash_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_IsRunning_Proxy(IMoniker*,IBindCtx*,IMoniker*,IMoniker*);
void STDMETHODCALLTYPE IMoniker_IsRunning_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_GetTimeOfLastChange_Proxy(IMoniker*,IBindCtx*,IMoniker*,FILETIME*);
void STDMETHODCALLTYPE IMoniker_GetTimeOfLastChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_Inverse_Proxy(IMoniker*,IMoniker**);
void STDMETHODCALLTYPE IMoniker_Inverse_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_CommonPrefixWith_Proxy(IMoniker*,IMoniker*,IMoniker**);
void STDMETHODCALLTYPE IMoniker_CommonPrefixWith_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_RelativePathTo_Proxy(IMoniker*,IMoniker*,IMoniker**);
void STDMETHODCALLTYPE IMoniker_RelativePathTo_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_GetDisplayName_Proxy(IMoniker*,IBindCtx*,IMoniker*,LPCSTR*);
void STDMETHODCALLTYPE IMoniker_GetDisplayName_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_ParseDisplayName_Proxy(IMoniker*,IBindCtx*,IMoniker*,LPCSTR,ULONG*,IMoniker**);
void STDMETHODCALLTYPE IMoniker_ParseDisplayName_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IMoniker_IsSystemMoniker_Proxy(IMoniker*,DWORD*);
void STDMETHODCALLTYPE IMoniker_IsSystemMoniker_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IROTData_GetComparisonData_Proxy(IROTData*,BYTE*,ULONG cbMax,ULONG*);
void STDMETHODCALLTYPE IROTData_GetComparisonData_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumString_RemoteNext_Proxy(IEnumString*,ULONG,LPCSTR*rgelt,ULONG*);
void STDMETHODCALLTYPE IEnumString_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumString_Skip_Proxy(IEnumString*,ULONG);
void STDMETHODCALLTYPE IEnumString_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumString_Reset_Proxy(IEnumString*);
void STDMETHODCALLTYPE IEnumString_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumString_Clone_Proxy(IEnumString*,IEnumString**);
void STDMETHODCALLTYPE IEnumString_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_RemoteRead_Proxy(IStream*,BYTE*,ULONG,ULONG*);
void STDMETHODCALLTYPE IStream_RemoteRead_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_RemoteWrite_Proxy(IStream*,BYTE*pv,ULONG,ULONG*);
void STDMETHODCALLTYPE IStream_RemoteWrite_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_RemoteSeek_Proxy(IStream*,LARGE_INTEGER,DWORD,ULARGE_INTEGER*);
void STDMETHODCALLTYPE IStream_RemoteSeek_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_SetSize_Proxy(IStream*,ULARGE_INTEGER);
void STDMETHODCALLTYPE IStream_SetSize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_RemoteCopyTo_Proxy(IStream*,IStream*,ULARGE_INTEGER,ULARGE_INTEGER*,ULARGE_INTEGER*);
void STDMETHODCALLTYPE IStream_RemoteCopyTo_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_Commit_Proxy(IStream*,DWORD);
void STDMETHODCALLTYPE IStream_Commit_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_Revert_Proxy(IStream*);
void STDMETHODCALLTYPE IStream_Revert_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_LockRegion_Proxy(IStream*,ULARGE_INTEGER,ULARGE_INTEGER,DWORD);
void STDMETHODCALLTYPE IStream_LockRegion_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_UnlockRegion_Proxy(IStream*,ULARGE_INTEGER,ULARGE_INTEGER,DWORD);
void STDMETHODCALLTYPE IStream_UnlockRegion_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_Stat_Proxy(IStream*,STATSTG*,DWORD);
void STDMETHODCALLTYPE IStream_Stat_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStream_Clone_Proxy(IStream*,IStream**);
void STDMETHODCALLTYPE IStream_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_RemoteNext_Proxy(IEnumSTATSTG*,ULONG,STATSTG*,ULONG*);
void STDMETHODCALLTYPE IEnumSTATSTG_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Skip_Proxy(IEnumSTATSTG*,ULONG celt);
void STDMETHODCALLTYPE IEnumSTATSTG_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Reset_Proxy(IEnumSTATSTG*);
void STDMETHODCALLTYPE IEnumSTATSTG_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Clone_Proxy(IEnumSTATSTG*,IEnumSTATSTG**);
void STDMETHODCALLTYPE IEnumSTATSTG_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_CreateStream_Proxy(IStorage*,OLECHAR*,DWORD,DWORD,DWORD,IStream**);
void STDMETHODCALLTYPE IStorage_CreateStream_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_RemoteOpenStream_Proxy(IStorage*,const OLECHAR*,unsigned long,BYTE*,DWORD,DWORD,IStream**);
void STDMETHODCALLTYPE IStorage_RemoteOpenStream_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_CreateStorage_Proxy(IStorage*,OLECHAR*,DWORD,DWORD,DWORD,IStorage**);
void STDMETHODCALLTYPE IStorage_CreateStorage_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_OpenStorage_Proxy(IStorage*,OLECHAR*,IStorage*,DWORD,SNB,DWORD,IStorage**);
void STDMETHODCALLTYPE IStorage_OpenStorage_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_CopyTo_Proxy(IStorage*,DWORD,const IID*,SNB,IStorage*);
void STDMETHODCALLTYPE IStorage_CopyTo_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_MoveElementTo_Proxy(IStorage*,const OLECHAR*,IStorage*,const OLECHAR*,DWORD);
void STDMETHODCALLTYPE IStorage_MoveElementTo_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_Commit_Proxy(IStorage*,DWORD);
void STDMETHODCALLTYPE IStorage_Commit_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_Revert_Proxy(IStorage*);
void STDMETHODCALLTYPE IStorage_Revert_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_RemoteEnumElements_Proxy(IStorage*,DWORD,unsigned long,BYTE*,DWORD,IEnumSTATSTG**);
void STDMETHODCALLTYPE IStorage_RemoteEnumElements_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_DestroyElement_Proxy(IStorage*,OLECHAR*);
void STDMETHODCALLTYPE IStorage_DestroyElement_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_RenameElement_Proxy(IStorage*,const OLECHAR*,const OLECHAR*);
void STDMETHODCALLTYPE IStorage_RenameElement_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_SetElementTimes_Proxy(IStorage*,const OLECHAR*,const FILETIME*,const FILETIME*,const FILETIME*);
void STDMETHODCALLTYPE IStorage_SetElementTimes_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_SetClass_Proxy(IStorage*,REFCLSID);
void STDMETHODCALLTYPE IStorage_SetClass_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_SetStateBits_Proxy(IStorage*,DWORD,DWORD);
void STDMETHODCALLTYPE IStorage_SetStateBits_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IStorage_Stat_Proxy(IStorage*,STATSTG*,DWORD);
void STDMETHODCALLTYPE IStorage_Stat_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistFile_IsDirty_Proxy(IPersistFile*);
void STDMETHODCALLTYPE IPersistFile_IsDirty_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistFile_Load_Proxy(IPersistFile*,LPCOLESTR,DWORD);
void STDMETHODCALLTYPE IPersistFile_Load_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistFile_Save_Proxy(IPersistFile*,LPCOLESTR pszFileName,BOOL);
void STDMETHODCALLTYPE IPersistFile_Save_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistFile_SaveCompleted_Proxy(IPersistFile*,LPCOLESTR);
void STDMETHODCALLTYPE IPersistFile_SaveCompleted_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistFile_GetCurFile_Proxy(IPersistFile*,LPCSTR*);
void STDMETHODCALLTYPE IPersistFile_GetCurFile_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_IsDirty_Proxy(IPersistStorage*);
void STDMETHODCALLTYPE IPersistStorage_IsDirty_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_InitNew_Proxy(IPersistStorage*,IStorage*);
void STDMETHODCALLTYPE IPersistStorage_InitNew_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_Load_Proxy(IPersistStorage*,IStorage*);
void STDMETHODCALLTYPE IPersistStorage_Load_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_Save_Proxy(IPersistStorage*,IStorage*,BOOL);
void STDMETHODCALLTYPE IPersistStorage_Save_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_SaveCompleted_Proxy(IPersistStorage*,IStorage*);
void STDMETHODCALLTYPE IPersistStorage_SaveCompleted_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPersistStorage_HandsOffStorage_Proxy(IPersistStorage*);
void STDMETHODCALLTYPE IPersistStorage_HandsOffStorage_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_RemoteReadAt_Proxy(ILockBytes*,ULARGE_INTEGER,BYTE*,ULONG,ULONG*);
void STDMETHODCALLTYPE ILockBytes_RemoteReadAt_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_RemoteWriteAt_Proxy(ILockBytes*,ULARGE_INTEGER,BYTE*pv,ULONG,ULONG*);
void STDMETHODCALLTYPE ILockBytes_RemoteWriteAt_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_Flush_Proxy(ILockBytes*);
void STDMETHODCALLTYPE ILockBytes_Flush_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_SetSize_Proxy(ILockBytes*,ULARGE_INTEGER);
void STDMETHODCALLTYPE ILockBytes_SetSize_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_LockRegion_Proxy(ILockBytes*,ULARGE_INTEGER,ULARGE_INTEGER,DWORD);
void STDMETHODCALLTYPE ILockBytes_LockRegion_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_UnlockRegion_Proxy(ILockBytes*,ULARGE_INTEGER,ULARGE_INTEGER,DWORD);
void STDMETHODCALLTYPE ILockBytes_UnlockRegion_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE ILockBytes_Stat_Proxy(ILockBytes*,STATSTG*,DWORD);
void STDMETHODCALLTYPE ILockBytes_Stat_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_RemoteNext_Proxy(IEnumFORMATETC*,ULONG,FORMATETC*,ULONG*);
void STDMETHODCALLTYPE IEnumFORMATETC_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Skip_Proxy(IEnumFORMATETC*,ULONG);
void STDMETHODCALLTYPE IEnumFORMATETC_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Reset_Proxy(IEnumFORMATETC*);
void STDMETHODCALLTYPE IEnumFORMATETC_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Clone_Proxy(IEnumFORMATETC*,IEnumFORMATETC**);
void STDMETHODCALLTYPE IEnumFORMATETC_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Next_Proxy(IEnumFORMATETC*,ULONG,FORMATETC*,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumFORMATETC_Next_Stub(IEnumFORMATETC*,ULONG,FORMATETC*,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_RemoteNext_Proxy(IEnumSTATDATA*,ULONG,STATDATA*,ULONG*);
void STDMETHODCALLTYPE IEnumSTATDATA_RemoteNext_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Skip_Proxy(IEnumSTATDATA*,ULONG);
void STDMETHODCALLTYPE IEnumSTATDATA_Skip_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Reset_Proxy(IEnumSTATDATA*);
void STDMETHODCALLTYPE IEnumSTATDATA_Reset_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Clone_Proxy(IEnumSTATDATA*,IEnumSTATDATA**);
void STDMETHODCALLTYPE IEnumSTATDATA_Clone_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Next_Proxy(IEnumSTATDATA*,ULONG,STATDATA*,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumSTATDATA_Next_Stub(IEnumSTATDATA*,ULONG,STATDATA*,ULONG*);
HRESULT STDMETHODCALLTYPE IRootStorage_SwitchToFile_Proxy(IRootStorage*,LPCSTR);
void STDMETHODCALLTYPE IRootStorage_SwitchToFile_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnDataChange_Proxy(IAdviseSink*,FORMATETC*,RemSTGMEDIUM*);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnDataChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnViewChange_Proxy(IAdviseSink*,DWORD,LONG);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnViewChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnRename_Proxy(IAdviseSink*,IMoniker*);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnRename_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnSave_Proxy(IAdviseSink*);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnSave_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IAdviseSink_RemoteOnClose_Proxy(IAdviseSink*);
void STDMETHODCALLTYPE IAdviseSink_RemoteOnClose_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink_OnDataChange_Proxy(IAdviseSink*,FORMATETC*,STGMEDIUM*);
void STDMETHODCALLTYPE IAdviseSink_OnDataChange_Stub(IAdviseSink*,FORMATETC*,RemSTGMEDIUM*);
void STDMETHODCALLTYPE IAdviseSink_OnViewChange_Proxy(IAdviseSink*,DWORD,LONG);
void STDMETHODCALLTYPE IAdviseSink_OnViewChange_Stub(IAdviseSink*,DWORD,LONG);
void STDMETHODCALLTYPE IAdviseSink_OnRename_Proxy(IAdviseSink*,IMoniker*);
void STDMETHODCALLTYPE IAdviseSink_OnRename_Stub(IAdviseSink*,IMoniker*);
void STDMETHODCALLTYPE IAdviseSink_OnSave_Proxy(IAdviseSink*);
void STDMETHODCALLTYPE IAdviseSink_OnSave_Stub(IAdviseSink*);
void STDMETHODCALLTYPE IAdviseSink_OnClose_Proxy(IAdviseSink*);
HRESULT STDMETHODCALLTYPE IAdviseSink_OnClose_Stub(IAdviseSink*);
void STDMETHODCALLTYPE IAdviseSink2_RemoteOnLinkSrcChange_Proxy(IAdviseSink2*,IMoniker*);
void STDMETHODCALLTYPE IAdviseSink2_RemoteOnLinkSrcChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IAdviseSink2_OnLinkSrcChange_Proxy(IAdviseSink2*,IMoniker*);
void STDMETHODCALLTYPE IAdviseSink2_OnLinkSrcChange_Stub(IAdviseSink2*,IMoniker*);
HRESULT STDMETHODCALLTYPE IDataObject_RemoteGetData_Proxy(IDataObject*,FORMATETC*,RemSTGMEDIUM**);
void STDMETHODCALLTYPE IDataObject_RemoteGetData_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_RemoteGetDataHere_Proxy(IDataObject*,FORMATETC*,RemSTGMEDIUM**);
void STDMETHODCALLTYPE IDataObject_RemoteGetDataHere_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_QueryGetData_Proxy(IDataObject*,FORMATETC*);
void STDMETHODCALLTYPE IDataObject_QueryGetData_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_GetCanonicalFormatEtc_Proxy(IDataObject*,FORMATETC*,FORMATETC*);
void STDMETHODCALLTYPE IDataObject_GetCanonicalFormatEtc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_RemoteSetData_Proxy(IDataObject*,FORMATETC*,RemSTGMEDIUM*,BOOL);
void STDMETHODCALLTYPE IDataObject_RemoteSetData_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_EnumFormatEtc_Proxy(IDataObject*,DWORD,IEnumFORMATETC**);
void STDMETHODCALLTYPE IDataObject_EnumFormatEtc_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_DAdvise_Proxy(IDataObject*,FORMATETC*,DWORD,IAdviseSink*,DWORD*);
void STDMETHODCALLTYPE IDataObject_DAdvise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_DUnadvise_Proxy(IDataObject*,DWORD);
void STDMETHODCALLTYPE IDataObject_DUnadvise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_EnumDAdvise_Proxy(IDataObject*,IEnumSTATDATA**);
void STDMETHODCALLTYPE IDataObject_EnumDAdvise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataObject_GetData_Proxy(IDataObject*,FORMATETC*,STGMEDIUM*);
HRESULT STDMETHODCALLTYPE IDataObject_GetData_Stub(IDataObject*,FORMATETC*,RemSTGMEDIUM**);
HRESULT STDMETHODCALLTYPE IDataObject_GetDataHere_Proxy(IDataObject*,FORMATETC*,STGMEDIUM*);
HRESULT STDMETHODCALLTYPE IDataObject_GetDataHere_Stub(IDataObject*,FORMATETC*,RemSTGMEDIUM**);
HRESULT STDMETHODCALLTYPE IDataObject_SetData_Proxy(IDataObject*,FORMATETC*,STGMEDIUM*,BOOL);
HRESULT STDMETHODCALLTYPE IDataObject_SetData_Stub(IDataObject*,FORMATETC*,RemSTGMEDIUM*,BOOL);
HRESULT STDMETHODCALLTYPE IDataAdviseHolder_Advise_Proxy(IDataAdviseHolder*,IDataObject*,FORMATETC*,DWORD,IAdviseSink*,DWORD*);
void STDMETHODCALLTYPE IDataAdviseHolder_Advise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataAdviseHolder_Unadvise_Proxy(IDataAdviseHolder*,DWORD);
void STDMETHODCALLTYPE IDataAdviseHolder_Unadvise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataAdviseHolder_EnumAdvise_Proxy(IDataAdviseHolder*,IEnumSTATDATA**);
void STDMETHODCALLTYPE IDataAdviseHolder_EnumAdvise_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IDataAdviseHolder_SendOnDataChange_Proxy(IDataAdviseHolder*,IDataObject*,DWORD,DWORD);
void STDMETHODCALLTYPE IDataAdviseHolder_SendOnDataChange_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
DWORD STDMETHODCALLTYPE IMessageFilter_HandleInComingCall_Proxy(IMessageFilter*,DWORD,HTASK,DWORD,LPINTERFACEINFO);
void STDMETHODCALLTYPE IMessageFilter_HandleInComingCall_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
DWORD STDMETHODCALLTYPE IMessageFilter_RetryRejectedCall_Proxy(IMessageFilter*,HTASK,DWORD,DWORD);
void STDMETHODCALLTYPE IMessageFilter_RetryRejectedCall_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
DWORD STDMETHODCALLTYPE IMessageFilter_MessagePending_Proxy(IMessageFilter*,HTASK,DWORD,DWORD);
void STDMETHODCALLTYPE IMessageFilter_MessagePending_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcChannelBuffer_GetBuffer_Proxy(IRpcChannelBuffer*,RPCOLEMESSAGE*,REFIID);
void STDMETHODCALLTYPE IRpcChannelBuffer_GetBuffer_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcChannelBuffer_SendReceive_Proxy(IRpcChannelBuffer*,RPCOLEMESSAGE*,ULONG*);
void STDMETHODCALLTYPE IRpcChannelBuffer_SendReceive_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcChannelBuffer_FreeBuffer_Proxy(IRpcChannelBuffer*,RPCOLEMESSAGE*);
void STDMETHODCALLTYPE IRpcChannelBuffer_FreeBuffer_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcChannelBuffer_GetDestCtx_Proxy(IRpcChannelBuffer*,DWORD*,void**);
void STDMETHODCALLTYPE IRpcChannelBuffer_GetDestCtx_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcChannelBuffer_IsConnected_Proxy(IRpcChannelBuffer*);
void STDMETHODCALLTYPE IRpcChannelBuffer_IsConnected_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcProxyBuffer_Connect_Proxy(IRpcProxyBuffer*,IRpcChannelBuffer*pRpcChannelBuffer);
void STDMETHODCALLTYPE IRpcProxyBuffer_Connect_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IRpcProxyBuffer_Disconnect_Proxy(IRpcProxyBuffer*);
void STDMETHODCALLTYPE IRpcProxyBuffer_Disconnect_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcStubBuffer_Connect_Proxy(IRpcStubBuffer*,IUnknown*);
void STDMETHODCALLTYPE IRpcStubBuffer_Connect_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IRpcStubBuffer_Disconnect_Proxy(IRpcStubBuffer*);
void STDMETHODCALLTYPE IRpcStubBuffer_Disconnect_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcStubBuffer_Invoke_Proxy(IRpcStubBuffer*,RPCOLEMESSAGE*,IRpcChannelBuffer*);
void STDMETHODCALLTYPE IRpcStubBuffer_Invoke_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
IRpcStubBuffer*STDMETHODCALLTYPE IRpcStubBuffer_IsIIDSupported_Proxy(IRpcStubBuffer*,REFIID);
void STDMETHODCALLTYPE IRpcStubBuffer_IsIIDSupported_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
ULONG STDMETHODCALLTYPE IRpcStubBuffer_CountRefs_Proxy(IRpcStubBuffer*);
void STDMETHODCALLTYPE IRpcStubBuffer_CountRefs_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IRpcStubBuffer_DebugServerQueryInterface_Proxy(IRpcStubBuffer*,void**);
void STDMETHODCALLTYPE IRpcStubBuffer_DebugServerQueryInterface_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE IRpcStubBuffer_DebugServerRelease_Proxy(IRpcStubBuffer*,void*);
void STDMETHODCALLTYPE IRpcStubBuffer_DebugServerRelease_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPSFactoryBuffer_CreateProxy_Proxy(IPSFactoryBuffer*,IUnknown*,REFIID,IRpcProxyBuffer**,void**);
void STDMETHODCALLTYPE IPSFactoryBuffer_CreateProxy_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
HRESULT STDMETHODCALLTYPE IPSFactoryBuffer_CreateStub_Proxy(IPSFactoryBuffer*,REFIID,IUnknown*,IRpcStubBuffer**);
void STDMETHODCALLTYPE IPSFactoryBuffer_CreateStub_Stub(IRpcStubBuffer*,IRpcChannelBuffer*,PRPC_MESSAGE,PDWORD);
void STDMETHODCALLTYPE SNB_to_xmit(SNB*,RemSNB**);
void STDMETHODCALLTYPE SNB_from_xmit(RemSNB*,SNB*);
void STDMETHODCALLTYPE SNB_free_inst(SNB*);
void STDMETHODCALLTYPE SNB_free_xmit(RemSNB*);
HRESULT STDMETHODCALLTYPE IEnumUnknown_Next_Proxy(IEnumUnknown*,ULONG,IUnknown**,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumUnknown_Next_Stub(IEnumUnknown*,ULONG,IUnknown**,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumMoniker_Next_Proxy(IEnumMoniker*,ULONG,IMoniker**,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumMoniker_Next_Stub(IEnumMoniker*,ULONG,IMoniker**,ULONG*);
HRESULT STDMETHODCALLTYPE IMoniker_BindToObject_Proxy(IMoniker*,IBindCtx*,IMoniker*,REFIID,void**);
HRESULT STDMETHODCALLTYPE IMoniker_BindToObject_Stub(IMoniker*,IBindCtx*,IMoniker*,REFIID,IUnknown**);
HRESULT STDMETHODCALLTYPE IMoniker_BindToStorage_Proxy(IMoniker*,IBindCtx*,IMoniker*,REFIID,void**);
HRESULT STDMETHODCALLTYPE IMoniker_BindToStorage_Stub(IMoniker*,IBindCtx*,IMoniker*,REFIID,IUnknown**);
HRESULT STDMETHODCALLTYPE IEnumString_Next_Proxy(IEnumString*,ULONG,LPCSTR*,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumString_Next_Stub(IEnumString*,ULONG,LPCSTR*,ULONG*);
HRESULT STDMETHODCALLTYPE IStream_Read_Proxy(IStream*,void*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE IStream_Read_Stub(IStream*,BYTE*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE IStream_Write_Proxy(IStream*,void*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE IStream_Write_Stub(IStream*,BYTE*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE IStream_Seek_Proxy(IStream*,LARGE_INTEGER,DWORD,ULARGE_INTEGER*);
HRESULT STDMETHODCALLTYPE IStream_Seek_Stub(IStream*,LARGE_INTEGER,DWORD,ULARGE_INTEGER*);
HRESULT STDMETHODCALLTYPE IStream_CopyTo_Proxy(IStream*,IStream*,ULARGE_INTEGER,ULARGE_INTEGER*,ULARGE_INTEGER*);
HRESULT STDMETHODCALLTYPE IStream_CopyTo_Stub(IStream*,IStream*,ULARGE_INTEGER,ULARGE_INTEGER*,ULARGE_INTEGER*);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Next_Proxy(IEnumSTATSTG*,ULONG,STATSTG*,ULONG*);
HRESULT STDMETHODCALLTYPE IEnumSTATSTG_Next_Stub(IEnumSTATSTG*,ULONG,STATSTG*,ULONG*);
HRESULT STDMETHODCALLTYPE IStorage_OpenStream_Proxy(IStorage*,OLECHAR*,void*,DWORD,DWORD,IStream**);
HRESULT STDMETHODCALLTYPE IStorage_OpenStream_Stub(IStorage*,OLECHAR*,unsigned long,BYTE*,DWORD,DWORD,IStream** );
HRESULT STDMETHODCALLTYPE IStorage_EnumElements_Proxy(IStorage*,DWORD,void*,DWORD,IEnumSTATSTG**);
HRESULT STDMETHODCALLTYPE IStorage_EnumElements_Stub(IStorage*,DWORD,unsigned long,BYTE*,DWORD,IEnumSTATSTG**);
HRESULT STDMETHODCALLTYPE ILockBytes_ReadAt_Proxy(ILockBytes*,ULARGE_INTEGER,void*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE ILockBytes_ReadAt_Stub(ILockBytes*,ULARGE_INTEGER,BYTE*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE ILockBytes_WriteAt_Proxy(ILockBytes*,ULARGE_INTEGER,const void*,ULONG,ULONG*);
HRESULT STDMETHODCALLTYPE ILockBytes_WriteAt_Stub(ILockBytes*,ULARGE_INTEGER,BYTE*,ULONG,ULONG*);

#if (!defined (__cplusplus) || defined (CINTERFACE)) \
    && defined (COBJMACROS)
#define IMarshal_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IMarshal_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IMarshal_Release(This) (This)->lpVtbl->Release(This)
#define IMarshal_GetUnmarshalClass(T,r,pv,dw,pvD,m,pC) (T)->lpVtbl->GetUnmarshalClass(T,r,pv,dw,pvD,m,pC)
#define IMarshal_GetMarshalSizeMax(T,r,pv,dw,pD,m,p) (T)->lpVtbl->GetMarshalSizeMax(T,r,pv,dw,pD,m,p)
#define IMarshal_MarshalInterface(T,p,r,pv,dw,pvD,m) (T)->lpVtbl->MarshalInterface(T,p,r,pv,dw,pv,m)
#define IMarshal_UnmarshalInterface(T,p,r,pp) (T)->lpVtbl->UnmarshalInterface(T,p,r,pp)
#define IMarshal_ReleaseMarshalData(T,p) (T)->lpVtbl->ReleaseMarshalData(T,p)
#define IMarshal_DisconnectObject(T,d) (T)->lpVtbl->DisconnectObject(T,d)
#define IMalloc_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IMalloc_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IMalloc_Release(This) (This)->lpVtbl->Release(This)
#define IMalloc_Alloc(This,cb) (This)->lpVtbl->Alloc(This,cb)
#define IMalloc_Realloc(This,pv,cb)	(This)->lpVtbl->Realloc(This,pv,cb)
#define IMalloc_Free(This,pv) (This)->lpVtbl->Free(This,pv)
#define IMalloc_GetSize(This,pv) (This)->lpVtbl->GetSize(This,pv)
#define IMalloc_DidAlloc(This,pv) (This)->lpVtbl->DidAlloc(This,pv)
#define IMalloc_HeapMinimize(This) (This)->lpVtbl->HeapMinimize(This)
#define IMallocSpy_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IMallocSpy_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IMallocSpy_Release(This) (This)->lpVtbl->Release(This)
#define IMallocSpy_PreAlloc(T,c) (T)->lpVtbl->PreAlloc(T,c)
#define IMallocSpy_PostAlloc(This,p) (This)->lpVtbl->PostAlloc(This,p)
#define IMallocSpy_PreFree(This,p,f) (This)->lpVtbl->PreFree(This,p,f)
#define IMallocSpy_PostFree(This,fSpyed) (This)->lpVtbl->PostFree(This,fSpyed)
#define IMallocSpy_PreRealloc(T,p,c,pp,f) (T)->lpVtbl->PreRealloc(T,p,c,pp,f)
#define IMallocSpy_PostRealloc(T,p,f) (T)->lpVtbl->PostRealloc(T,p,f)
#define IMallocSpy_PreGetSize(This,p,f)	(This)->lpVtbl->PreGetSize(This,p,f)
#define IMallocSpy_PostGetSize(This,cbActual,fSpyed) (This)->lpVtbl->PostGetSize(This,cbActual,fSpyed)
#define IMallocSpy_PreDidAlloc(This,pRequest,fSpyed) (This)->lpVtbl->PreDidAlloc(This,pRequest,fSpyed)
#define IMallocSpy_PostDidAlloc(This,pRequest,fSpyed,fActual) (This)->lpVtbl->PostDidAlloc(This,pRequest,fSpyed,fActual)
#define IMallocSpy_PreHeapMinimize(T) (T)->lpVtbl->PreHeapMinimize(T)
#define IMallocSpy_PostHeapMinimize(T) (T)->lpVtbl->PostHeapMinimize(T)
#define IStdMarshalInfo_QueryInterface(T,r,p) (This)->lpVtbl->QueryInterface(T,r,p)
#define IStdMarshalInfo_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IStdMarshalInfo_Release(This) (This)->lpVtbl->Release(This)
#define IStdMarshalInfo_GetClassForHandler(This,D,p,C) (This)->lpVtbl->GetClassForHandler(This,D,p,C)
#define IExternalConnection_QueryInterface(This,riid,ppvObject)	(This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IExternalConnection_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IExternalConnection_Release(This) (This)->lpVtbl->Release(This)
#define IExternalConnection_AddConnection(T,e,r) (T)->lpVtbl->AddConnection(T,e,r)
#define IExternalConnection_ReleaseConnection(This,e,r,f) (This)->lpVtbl->ReleaseConnection(This,e,r,f)
#define IEnumUnknown_QueryInterface(T,r,p) (This)->lpVtbl->QueryInterface(T,r,p)
#define IEnumUnknown_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumUnknown_Release(This) (This)->lpVtbl->Release(This)
#define IEnumUnknown_Next(This,celt,rgelt,p) (This)->lpVtbl->Next(This,celt,rgelt,p)
#define IEnumUnknown_Skip(This,celt) (This)->lpVtbl->Skip(This,celt)
#define IEnumUnknown_Reset(This) (This)->lpVtbl->Reset(This)
#define IEnumUnknown_Clone(This,ppenum)	 (This)->lpVtbl->Clone(This,ppenum)
#define IBindCtx_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IBindCtx_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IBindCtx_Release(This)	(This)->lpVtbl->Release(This)
#define IBindCtx_RegisterObjectBound(T,p) (T)->lpVtbl->RegisterObjectBound(T,p)
#define IBindCtx_RevokeObjectBound(T,p)	(T)->lpVtbl->RevokeObjectBound(T,p)
#define IBindCtx_ReleaseBoundObjects(T) (T)->lpVtbl->ReleaseBoundObjects(T)
#define IBindCtx_SetBindOptions(T,p) (T)->lpVtbl->SetBindOptions(T,p)
#define IBindCtx_GetBindOptions(This,pbindopts)	(This)->lpVtbl->GetBindOptions(This,pbindopts)
#define IBindCtx_GetRunningObjectTable(This,pprot) (This)->lpVtbl->GetRunningObjectTable(This,pprot)
#define IBindCtx_RegisterObjectParam(This,pszKey,punk) (This)->lpVtbl->RegisterObjectParam(This,pszKey,punk)
#define IBindCtx_GetObjectParam(This,pszKey,ppunk) (This)->lpVtbl->GetObjectParam(This,pszKey,ppunk)
#define IBindCtx_EnumObjectParam(This,ppenum) (This)->lpVtbl->EnumObjectParam(This,ppenum)
#define IBindCtx_RevokeObjectParam(This,pszKey)	(This)->lpVtbl->RevokeObjectParam(This,pszKey)
#define IEnumMoniker_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IEnumMoniker_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumMoniker_Release(This) (This)->lpVtbl->Release(This)
#define IEnumMoniker_Next(This,celt,rgelt,pceltFetched)	(This)->lpVtbl->Next(This,celt,rgelt,pceltFetched)
#define IEnumMoniker_Skip(This,celt) (This)->lpVtbl->Skip(This,celt)
#define IEnumMoniker_Reset(This) (This)->lpVtbl->Reset(This)
#define IEnumMoniker_Clone(This,ppenum) (This)->lpVtbl->Clone(This,ppenum)
#define IRunnableObject_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IRunnableObject_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IRunnableObject_Release(This)	(This)->lpVtbl->Release(This)
#define IRunnableObject_GetRunningClass(This,lpClsid)	(This)->lpVtbl->GetRunningClass(This,lpClsid)
#define IRunnableObject_Run(This,pbc)	(This)->lpVtbl->Run(This,pbc)
#define IRunnableObject_IsRunning(This)	(This)->lpVtbl->IsRunning(This)
#define IRunnableObject_LockRunning(This,fLock,fLastUnlockCloses) (This)->lpVtbl->LockRunning(This,fLock,fLastUnlockCloses)
#define IRunnableObject_SetContainedObject(This,fContained) (This)->lpVtbl->SetContainedObject(This,fContained)
#define IRunningObjectTable_QueryInterface(This,riid,ppvObject)	(This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IRunningObjectTable_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IRunningObjectTable_Release(This) (This)->lpVtbl->Release(This)
#define IRunningObjectTable_Register(This,grfFlags,punkObject,pmkObjectName,pdwRegister) (This)->lpVtbl->Register(This,grfFlags,punkObject,pmkObjectName,pdwRegister)
#define IRunningObjectTable_Revoke(This,dwRegister) (This)->lpVtbl->Revoke(This,dwRegister)
#define IRunningObjectTable_IsRunning(This,pmkObjectName) (This)->lpVtbl->IsRunning(This,pmkObjectName)
#define IRunningObjectTable_GetObject(This,pmkObjectName,ppunkObject) (This)->lpVtbl->GetObject(This,pmkObjectName,ppunkObject)
#define IRunningObjectTable_NoteChangeTime(This,dwRegister,pfiletime) (This)->lpVtbl->NoteChangeTime(This,dwRegister,pfiletime)
#define IRunningObjectTable_GetTimeOfLastChange(This,pmkObjectName,pfiletime) (This)->lpVtbl->GetTimeOfLastChange(This,pmkObjectName,pfiletime)
#define IRunningObjectTable_EnumRunning(This,ppenumMoniker) (This)->lpVtbl->EnumRunning(This,ppenumMoniker)
#define IPersist_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IPersist_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IPersist_Release(This)	(This)->lpVtbl->Release(This)
#define IPersist_GetClassID(This,pClassID) (This)->lpVtbl->GetClassID(This,pClassID)
#define IPersistStream_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IPersistStream_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IPersistStream_Release(This)	(This)->lpVtbl->Release(This)
#define IPersistStream_GetClassID(T,p) (T)->lpVtbl->GetClassID(T,p)
#define IPersistStream_IsDirty(This)	(This)->lpVtbl->IsDirty(This)
#define IPersistStream_Load(This,pStm)	(This)->lpVtbl->Load(This,pStm)
#define IPersistStream_Save(T,p,f) (T)->lpVtbl->Save(T,p,f)
#define IPersistStream_GetSizeMax(T,p) (T)->lpVtbl->GetSizeMax(T,p)
#define IMoniker_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IMoniker_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IMoniker_Release(This)	(This)->lpVtbl->Release(This)
#define IMoniker_GetClassID(This,pClassID) (This)->lpVtbl->GetClassID(This,pClassID)
#define IMoniker_IsDirty(This)	(This)->lpVtbl->IsDirty(This)
#define IMoniker_Load(This,pStm) (This)->lpVtbl->Load(This,pStm)
#define IMoniker_Save(This,pStm,fClearDirty) (This)->lpVtbl->Save(This,pStm,fClearDirty)
#define IMoniker_GetSizeMax(This,pcbSize) (This)->lpVtbl->GetSizeMax(This,pcbSize)
#define IMoniker_BindToObject(T,p,pm,r,pp) (T)->lpVtbl->BindToObject(T,p,pm,r,pp)
#define IMoniker_BindToStorage(This,pbc,pmkToLeft,riid,ppvObj) (This)->lpVtbl->BindToStorage(This,pbc,pmkToLeft,riid,ppvObj)
#define IMoniker_Reduce(This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced)	(This)->lpVtbl->Reduce(This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced)
#define IMoniker_ComposeWith(This,pmkRight,fOnlyIfNotGeneric,ppmkComposite) (This)->lpVtbl->ComposeWith(This,pmkRight,fOnlyIfNotGeneric,ppmkComposite)
#define IMoniker_Enum(T,f,pp) (T)->lpVtbl->Enum(T,f,pp)
#define IMoniker_IsEqual(This,p) (This)->lpVtbl->IsEqual(This,p)
#define IMoniker_Hash(This,pdwHash) (This)->lpVtbl->Hash(This,pdwHash)
#define IMoniker_IsRunning(T,pbc,Left,N) (T)->lpVtbl->IsRunning(T,pbc,Left,N)
#define IMoniker_GetTimeOfLastChange(This,pbc,pmkToLeft,pFileTime) (This)->lpVtbl->GetTimeOfLastChange(This,pbc,pmkToLeft,pFileTime)
#define IMoniker_Inverse(This,ppmk) (This)->lpVtbl->Inverse(This,ppmk)
#define IMoniker_CommonPrefixWith(This,pmkOther,ppmkPrefix) (This)->lpVtbl->CommonPrefixWith(This,pmkOther,ppmkPrefix)
#define IMoniker_RelativePathTo(This,pmkOther,ppmkRelPath) (This)->lpVtbl->RelativePathTo(This,pmkOther,ppmkRelPath)
#define IMoniker_GetDisplayName(This,pbc,pmkToLeft,ppszDisplayName) (This)->lpVtbl->GetDisplayName(This,pbc,pmkToLeft,ppszDisplayName)
#define IMoniker_ParseDisplayName(This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut) (This)->lpVtbl->ParseDisplayName(This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut)
#define IMoniker_IsSystemMoniker(This,pdwMksys)	(This)->lpVtbl->IsSystemMoniker(This,pdwMksys)
#define IROTData_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IROTData_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IROTData_Release(This) (This)->lpVtbl->Release(This)
#define IROTData_GetComparisonData(This,pbData,cbMax,pcbData) (This)->lpVtbl->GetComparisonData(This,pbData,cbMax,pcbData)
#define IEnumString_QueryInterface(This,riid,ppvObject)	(This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IEnumString_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumString_Release(This) (This)->lpVtbl->Release(This)
#define IEnumString_Next(This,celt,rgelt,pceltFetched) (This)->lpVtbl->Next(This,celt,rgelt,pceltFetched)
#define IEnumString_Skip(This,celt) (This)->lpVtbl->Skip(This,celt)
#define IEnumString_Reset(This)	(This)->lpVtbl->Reset(This)
#define IEnumString_Clone(This,ppenum) (This)->lpVtbl->Clone(This,ppenum)
#define IStream_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IStream_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IStream_Release(This) (This)->lpVtbl->Release(This)
#define IStream_Read(This,pv,cb,pcbRead) (This)->lpVtbl->Read(This,pv,cb,pcbRead)
#define IStream_Write(This,pv,cb,pcbWritten) (This)->lpVtbl->Write(This,pv,cb,pcbWritten)
#define IStream_Seek(This,dlibMove,dwOrigin,plibNewPosition) (This)->lpVtbl->Seek(This,dlibMove,dwOrigin,plibNewPosition)
#define IStream_SetSize(This,libNewSize) (This)->lpVtbl->SetSize(This,libNewSize)
#define IStream_CopyTo(This,pstm,cb,pcbRead,pcbWritten)	(This)->lpVtbl->CopyTo(This,pstm,cb,pcbRead,pcbWritten)
#define IStream_Commit(This,grfCommitFlags) (This)->lpVtbl->Commit(This,grfCommitFlags)
#define IStream_Revert(This) (This)->lpVtbl->Revert(This)
#define IStream_LockRegion(This,libOffset,cb,dwLockType) (This)->lpVtbl->LockRegion(This,libOffset,cb,dwLockType)
#define IStream_UnlockRegion(This,libOffset,cb,dwLockType) (This)->lpVtbl->UnlockRegion(This,libOffset,cb,dwLockType)
#define IStream_Stat(This,pstatstg,grfStatFlag)	(This)->lpVtbl->Stat(This,pstatstg,grfStatFlag)
#define IStream_Clone(This,ppstm) (This)->lpVtbl->Clone(This,ppstm)
#define IEnumSTATSTG_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IEnumSTATSTG_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumSTATSTG_Release(This) (This)->lpVtbl->Release(This)
#define IEnumSTATSTG_Next(T,c,r,p) (T)->lpVtbl->Next(T,c,r,p)
#define IEnumSTATSTG_Skip(This,celt)	(This)->lpVtbl->Skip(This,celt)
#define IEnumSTATSTG_Reset(This) (This)->lpVtbl->Reset(This)
#define IEnumSTATSTG_Clone(This,ppenum)	 (This)->lpVtbl->Clone(This,ppenum)
#define IStorage_QueryInterface(T,r,p) (T)->lpVtbl->QueryInterface(T,r,p)
#define IStorage_AddRef(This)	 (This)->lpVtbl->AddRef(This)
#define IStorage_Release(This)	 (This)->lpVtbl->Release(This)
#define IStorage_CreateStream(T,p,g,r1,r2,pp) (T)->lpVtbl->CreateStream(T,p,g,r1,r2,pp)
#define IStorage_OpenStream(T,p,r1,g,r2,pp) (T)->lpVtbl->OpenStream(T,p,r1,g,r2,pp)
#define IStorage_CreateStorage(T,p,g,d,r2,pp) (T)->lpVtbl->CreateStorage(T,p,g,d,r2,pp)
#define IStorage_OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg) (This)->lpVtbl->OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg)
#define IStorage_CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest) (This)->lpVtbl->CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest)
#define IStorage_MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags) (This)->lpVtbl->MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags)
#define IStorage_Commit(This,g) (This)->lpVtbl->Commit(This,g)
#define IStorage_Revert(This) (This)->lpVtbl->Revert(This)
#define IStorage_EnumElements(This,reserved1,reserved2,reserved3,ppenum) (This)->lpVtbl->EnumElements(This,reserved1,reserved2,reserved3,ppenum)
#define IStorage_DestroyElement(This,pwcsName)	(This)->lpVtbl->DestroyElement(This,pwcsName)
#define IStorage_RenameElement(This,pwcsOldName,pwcsNewName) (This)->lpVtbl->RenameElement(This,pwcsOldName,pwcsNewName)
#define IStorage_SetElementTimes(This,pwcsName,pctime,patime,pmtime) (This)->lpVtbl->SetElementTimes(This,pwcsName,pctime,patime,pmtime)
#define IStorage_SetClass(This,clsid) (This)->lpVtbl->SetClass(This,clsid)
#define IStorage_SetStateBits(This,grfStateBits,grfMask) (This)->lpVtbl->SetStateBits(This,grfStateBits,grfMask)
#define IStorage_Stat(This,p,g) (This)->lpVtbl->Stat(This,p,g)
#define IPersistFile_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IPersistFile_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IPersistFile_Release(This) (This)->lpVtbl->Release(This)
#define IPersistFile_GetClassID(This,pClassID)	(This)->lpVtbl->GetClassID(This,pClassID)
#define IPersistFile_IsDirty(This) (This)->lpVtbl->IsDirty(This)
#define IPersistFile_Load(This,pszFileName,dwMode) (This)->lpVtbl->Load(This,pszFileName,dwMode)
#define IPersistFile_Save(This,pszFileName,fRemember) (This)->lpVtbl->Save(This,pszFileName,fRemember)
#define IPersistFile_SaveCompleted(This,pszFileName) (This)->lpVtbl->SaveCompleted(This,pszFileName)
#define IPersistFile_GetCurFile(This,ppszFileName) (This)->lpVtbl->GetCurFile(This,ppszFileName)
#define IPersistStorage_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IPersistStorage_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IPersistStorage_Release(This) (This)->lpVtbl->Release(This)
#define IPersistStorage_GetClassID(This,pClassID) (This)->lpVtbl->GetClassID(This,pClassID)
#define IPersistStorage_IsDirty(This) (This)->lpVtbl->IsDirty(This)
#define IPersistStorage_InitNew(This,pStg) (This)->lpVtbl->InitNew(This,pStg)
#define IPersistStorage_Load(This,pStg)	(This)->lpVtbl->Load(This,pStg)
#define IPersistStorage_Save(This,pStgSave,fSameAsLoad)	(This)->lpVtbl->Save(This,pStgSave,fSameAsLoad)
#define IPersistStorage_SaveCompleted(This,pStgNew) (This)->lpVtbl->SaveCompleted(This,pStgNew)
#define IPersistStorage_HandsOffStorage(This) (This)->lpVtbl->HandsOffStorage(This)
#define ILockBytes_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define ILockBytes_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define ILockBytes_Release(This) (This)->lpVtbl->Release(This)
#define ILockBytes_ReadAt(This,ulOffset,pv,cb,pcbRead) (This)->lpVtbl->ReadAt(This,ulOffset,pv,cb,pcbRead)
#define ILockBytes_WriteAt(This,ulOffset,pv,cb,pcbWritten) (This)->lpVtbl->WriteAt(This,ulOffset,pv,cb,pcbWritten)
#define ILockBytes_Flush(This) (This)->lpVtbl->Flush(This)
#define ILockBytes_SetSize(This,cb) (This)->lpVtbl->SetSize(This,cb)
#define ILockBytes_LockRegion(This,libOffset,cb,dwLockType) (This)->lpVtbl->LockRegion(This,libOffset,cb,dwLockType)
#define ILockBytes_UnlockRegion(This,libOffset,cb,dwLockType) (This)->lpVtbl->UnlockRegion(This,libOffset,cb,dwLockType)
#define ILockBytes_Stat(This,pstatstg,grfStatFlag) (This)->lpVtbl->Stat(This,pstatstg,grfStatFlag)
#define IEnumFORMATETC_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IEnumFORMATETC_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumFORMATETC_Release(This) (This)->lpVtbl->Release(This)
#define IEnumFORMATETC_Next(This,celt,rgelt,pceltFetched) (This)->lpVtbl->Next(This,celt,rgelt,pceltFetched)
#define IEnumFORMATETC_Skip(This,celt) (This)->lpVtbl->Skip(This,celt)
#define IEnumFORMATETC_Reset(This) (This)->lpVtbl->Reset(This)
#define IEnumFORMATETC_Clone(This,ppenum) (This)->lpVtbl->Clone(This,ppenum)
#define IEnumSTATDATA_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IEnumSTATDATA_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IEnumSTATDATA_Release(This) (This)->lpVtbl->Release(This)
#define IEnumSTATDATA_Next(This,celt,rgelt,pceltFetched) (This)->lpVtbl->Next(This,celt,rgelt,pceltFetched)
#define IEnumSTATDATA_Skip(This,celt) (This)->lpVtbl->Skip(This,celt)
#define IEnumSTATDATA_Reset(This) (This)->lpVtbl->Reset(This)
#define IEnumSTATDATA_Clone(This,ppenum) (This)->lpVtbl->Clone(This,ppenum)
#define IRootStorage_QueryInterface(T,r,O) (T)->lpVtbl->QueryInterface(T,r,O)
#define IRootStorage_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IRootStorage_Release(This)	 (This)->lpVtbl->Release(This)
#define IRootStorage_SwitchToFile(This,pszFile)	 (This)->lpVtbl->SwitchToFile(This,pszFile)
#define IAdviseSink_QueryInterface(This,riid,ppvObject)	(This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IAdviseSink_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IAdviseSink_Release(This) (This)->lpVtbl->Release(This)
#define IAdviseSink_OnDataChange(This,pFormatetc,pStgmed) (This)->lpVtbl->OnDataChange(This,pFormatetc,pStgmed)
#define IAdviseSink_OnViewChange(This,dwAspect,lindex) (This)->lpVtbl->OnViewChange(This,dwAspect,lindex)
#define IAdviseSink_OnRename(This,pmk) (This)->lpVtbl->OnRename(This,pmk)
#define IAdviseSink_OnSave(This) (This)->lpVtbl->OnSave(This)
#define IAdviseSink_OnClose(This) (This)->lpVtbl->OnClose(This)
#define IAdviseSink2_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IAdviseSink2_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IAdviseSink2_Release(This) (This)->lpVtbl->Release(This)
#define IAdviseSink2_OnDataChange(This,pFormatetc,pStgmed) (This)->lpVtbl->OnDataChange(This,pFormatetc,pStgmed)
#define IAdviseSink2_OnViewChange(This,dwAspect,lindex)	(This)->lpVtbl->OnViewChange(This,dwAspect,lindex)
#define IAdviseSink2_OnRename(This,pmk)	(This)->lpVtbl->OnRename(This,pmk)
#define IAdviseSink2_OnSave(This) (This)->lpVtbl->OnSave(This)
#define IAdviseSink2_OnClose(This) (This)->lpVtbl->OnClose(This)
#define IAdviseSink2_OnLinkSrcChange(This,pmk) (This)->lpVtbl->OnLinkSrcChange(This,pmk)
#define IDataObject_QueryInterface(This,riid,ppvObject)	(This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDataObject_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IDataObject_Release(This) (This)->lpVtbl->Release(This)
#define IDataObject_GetData(This,pformatetcIn,pmedium) (This)->lpVtbl->GetData(This,pformatetcIn,pmedium)
#define IDataObject_GetDataHere(This,pformatetc,pmedium) (This)->lpVtbl->GetDataHere(This,pformatetc,pmedium)
#define IDataObject_QueryGetData(This,pformatetc) (This)->lpVtbl->QueryGetData(This,pformatetc)
#define IDataObject_GetCanonicalFormatEtc(This,pformatectIn,pformatetcOut) (This)->lpVtbl->GetCanonicalFormatEtc(This,pformatectIn,pformatetcOut)
#define IDataObject_SetData(This,pformatetc,pmedium,fRelease) (This)->lpVtbl->SetData(This,pformatetc,pmedium,fRelease)
#define IDataObject_EnumFormatEtc(This,dwDirection,ppenumFormatEtc) (This)->lpVtbl->EnumFormatEtc(This,dwDirection,ppenumFormatEtc)
#define IDataObject_DAdvise(This,pformatetc,advf,pAdvSink,pdwConnection) (This)->lpVtbl->DAdvise(This,pformatetc,advf,pAdvSink,pdwConnection)
#define IDataObject_DUnadvise(This,dwConnection) (This)->lpVtbl->DUnadvise(This,dwConnection)
#define IDataObject_EnumDAdvise(This,ppenumAdvise) (This)->lpVtbl->EnumDAdvise(This,ppenumAdvise)
#define IDataAdviseHolder_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IDataAdviseHolder_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IDataAdviseHolder_Release(This)	(This)->lpVtbl->Release(This)
#define IDataAdviseHolder_Advise(This,pDataObject,pFetc,advf,pAdvise,pdwConnection) (This)->lpVtbl->Advise(This,pDataObject,pFetc,advf,pAdvise,pdwConnection)
#define IDataAdviseHolder_Unadvise(This,dwConnection) (This)->lpVtbl->Unadvise(This,dwConnection)
#define IDataAdviseHolder_EnumAdvise(This,ppenumAdvise)	(This)->lpVtbl->EnumAdvise(This,ppenumAdvise)
#define IDataAdviseHolder_SendOnDataChange(This,pDataObject,dwReserved,advf) (This)->lpVtbl->SendOnDataChange(This,pDataObject,dwReserved,advf)
#define IMessageFilter_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IMessageFilter_AddRef(This)	 (This)->lpVtbl->AddRef(This)
#define IMessageFilter_Release(This)	 (This)->lpVtbl->Release(This)
#define IMessageFilter_HandleInComingCall(T,d,h,dw,lp) (T)->lpVtbl->HandleInComingCall(T,d,h,dw,lp)
#define IMessageFilter_RetryRejectedCall(s,C,T,R) (s)->lpVtbl->RetryRejectedCall(s,C,T,R)
#define IMessageFilter_MessagePending(s,C,T,P) (s)->lpVtbl->MessagePending(This,C,T,P)
#define IRpcChannelBuffer_QueryInterface(T,r,p)	(T)->lpVtbl->QueryInterface(T,r,p)
#define IRpcChannelBuffer_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IRpcChannelBuffer_Release(This)	(This)->lpVtbl->Release(This)
#define IRpcChannelBuffer_GetBuffer(This,pMessage,riid)	(This)->lpVtbl->GetBuffer(This,pMessage,riid)
#define IRpcChannelBuffer_SendReceive(T,p,pS) (T)->lpVtbl->SendReceive(T,p,pS)
#define IRpcChannelBuffer_FreeBuffer(T,p) (T)->lpVtbl->FreeBuffer(T,p)
#define IRpcChannelBuffer_GetDestCtx(This,pdwDestContext,ppvDestContext) (This)->lpVtbl->GetDestCtx(This,pdwDestContext,ppvDestContext)
#define IRpcChannelBuffer_IsConnected(This) (This)->lpVtbl->IsConnected(This)
#define IRpcProxyBuffer_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IRpcProxyBuffer_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IRpcProxyBuffer_Release(This) (This)->lpVtbl->Release(This)
#define IRpcProxyBuffer_Connect(This,pRpcChannelBuffer)	(This)->lpVtbl->Connect(This,pRpcChannelBuffer)
#define IRpcProxyBuffer_Disconnect(This) (This)->lpVtbl->Disconnect(This)
#define IRpcStubBuffer_QueryInterface(T,r,pp) (T)->lpVtbl->QueryInterface(T,r,pp)
#define IRpcStubBuffer_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IRpcStubBuffer_Release(This)	(This)->lpVtbl->Release(This)
#define IRpcStubBuffer_Connect(This,p)	 (This)->lpVtbl->Connect(This,p)
#define IRpcStubBuffer_Disconnect(This)	 (This)->lpVtbl->Disconnect(This)
#define IRpcStubBuffer_Invoke(T,_prpcmsg,_p)	 (T)->lpVtbl->Invoke(T,_prpcmsg,_p)
#define IRpcStubBuffer_IsIIDSupported(T,d) (T)->lpVtbl->IsIIDSupported(T,d)
#define IRpcStubBuffer_CountRefs(This)	 (This)->lpVtbl->CountRefs(This)
#define IRpcStubBuffer_DebugServerQueryInterface(T,p) (T)->lpVtbl->DebugServerQueryInterface(T,p)
#define IRpcStubBuffer_DebugServerRelease(T,p) (T)->lpVtbl->DebugServerRelease(T,p)
#define IPSFactoryBuffer_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IPSFactoryBuffer_AddRef(This)	(This)->lpVtbl->AddRef(This)
#define IPSFactoryBuffer_Release(This)	(This)->lpVtbl->Release(This)
#define IPSFactoryBuffer_CreateProxy(T,U,r,P,p) (T)->lpVtbl->CreateProxy(T,U,r,P,p)
#define IPSFactoryBuffer_CreateStub(T,r,U,p) (T)->lpVtbl->CreateStub(T,r,U,p)
#define IClassActivator_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IClassActivator_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IClassActivator_Release(T) (T)->lpVtbl->Release(T)
#define IClassActivator_GetClassObject(T,a,b,c,d,e) (T)->lpVtbl->GetClassObject(T,a,b,c,d,e)
#endif /* COBJMACROS */

#endif
