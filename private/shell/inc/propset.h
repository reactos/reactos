#ifndef _PROPSET_H_
#define _PROPSET_H_

// BUGBUG (scotth): this is a placeholder header so we can
//  use this according to the spec \\ole\specs\release\properties.doc

// Don't define if OLE definitions are in place!
#ifndef __IPropertyStorage_INTERFACE_DEFINED__
#define __IPropertyStorage_INTERFACE_DEFINED__

#include <ole2.h>


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

typedef struct tagPROPVARIANT PROPVARIANT;

#define TYPEDEF_CA(type, name) \
	typedef struct tag ## name {\
		ULONG cElems;\
		type *pElems;\
		} name
TYPEDEF_CA(unsigned char,		CAUI1);
TYPEDEF_CA(short,			CAI2);
TYPEDEF_CA(USHORT,		CAUI2);
TYPEDEF_CA(long,				CAI4);
TYPEDEF_CA(ULONG,			CAUI4);
TYPEDEF_CA(LARGE_INTEGER,	CAI8);
TYPEDEF_CA(ULARGE_INTEGER,CAUI8);
TYPEDEF_CA(float,				CAR4);
TYPEDEF_CA(double,			CAR8);
TYPEDEF_CA(CURRENCY,		CACY);
TYPEDEF_CA(DATE,			CADATE);
TYPEDEF_CA(BSTR,			CABSTR);
TYPEDEF_CA(VARIANT_BOOL,	CABOOL);
TYPEDEF_CA(SCODE,			CASCODE);
TYPEDEF_CA(FILETIME,		CAFILETIME);
TYPEDEF_CA(LPSTR,			CALPSTR);
TYPEDEF_CA(LPWSTR,		CALPWSTR);
TYPEDEF_CA(CLSID,			CACLSID);
TYPEDEF_CA(CLIPDATA,		CACLIPDATA);
TYPEDEF_CA(PROPVARIANT,	CAPROPVARIANT);

typedef struct tagPROPVARIANT{
	VARTYPE	vt;				// value type tag
	WORD 		wReserved1;		// padding to achieve 4-byte alignment
	WORD 		wReserved2;
	WORD 		wReserved3;
    union {							
	// none						// VT_EMPTY, VT_NULL, VT_ILLEGAL
	unsigned char 		bVal;		// VT_UI1
	short         		iVal;           	// VT_I2
	USHORT			uiVal;		// VT_UI2
	long          		lVal;			// VT_I4
	ULONG			ulVal;		// VT_UI4
	LARGE_INTEGER	hVal;		// VT_I8
	ULARGE_INTEGER  uhVal;		// VT_UI8
	float				fltVal;		// VT_R4
	double			dblVal;		// VT_R8
	CY				cyVal;		// VT_CY
	DATE			date;			// VT_DATE
	BSTR			bstrVal;		// VT_BSTR			// string in the current Ansi code page
	VARIANT_BOOL	bool;			// VT_BOOL
	SCODE         		scode;          	// VT_ERROR
	FILETIME			filetime;	// VT_FILETIME
	LPSTR			pszVal;		// VT_LPSTR			// string in the current Ansi code page
	LPWSTR        		pwszVal;		// VT_LPWSTR		// string in Unicode
	CLSID*			puuid;		// VT_CLSID
	CLIPDATA*		pclipdata;		// VT_CF

	BLOB			blob;			// VT_BLOB, VT_BLOBOBJECT
	IStream*		pStream;		// VT_STREAM, VT_STREAMED_OBJECT
	IStorage*		pStorage;		// VT_STORAGE, VT_STORED_OBJECT

	CAUI1			cab;		// VT_VECTOR | VT_UI1
	CAI2           		cai;            	// VT_VECTOR | VT_I2
	CAUI2			caui;			// VT_VECTOR | VT_UI2
	CAI4           		cal;            	// VT_VECTOR | VT_I4
	CAUI4			caul;			// VT_VECTOR | VT_UI4
	CAI8				cah;			// VT_VECTOR | VT_I8
	CAUI8			cauh;		// VT_VECTOR | VT_UI8
	CAR4         		caflt;			// VT_VECTOR | VT_R4
	CAR8         		cadbl;		// VT_VECTOR | VT_R8
	CACY          		cacy;           	// VT_VECTOR | VT_CY
	CADATE        		cadate;         	// VT_VECTOR | VT_DATE
	CABSTR        		cabstr;         	// VT_VECTOR | VT_BSTR
	CABOOL			cabool;		// VT_VECTOR | VT_BOOL
	CASCODE		cascode;		// VT_VECTOR | VT_ERROR
	CALPSTR       		calpstr;        	// VT_VECTOR | VT_LPSTR
	CALPWSTR      	calpwstr;       	// VT_VECTOR | VT_LPWSTR
	CAFILETIME    	cafiletime;     	// VT_VECTOR | VT_FILETIME
	CACLSID       		cauuid;         	// VT_VECTOR | VT_CLSID
	CACLIPDATA		caclipdata;	// VT_VECTOR | VT_CF
	CAPROPVARIANT 	capropvar;	     	// VT_VECTOR | VT_VARIANT
	}DUMMYUNIONNAME;
} PROPVARIANT;

#if 0
typedef enum {
	VT_EMPTY=0,		VT_NULL=1,		VT_I2=2,			VT_I4=3,			VT_R4=4
	VT_R8=5,		VT_CY=6,		VT_DATE=7,		VT_BSTR=8,		VT_ERROR=10,	VT_BOOL=11,
	VT_VARIANT=12,	VT_UI1=17, 		VT_UI2=18,		VT_UI4=19,
	VT_I8=20,		VT_UI8=21,		VT_LPSTR=30,	VT_LPWSTR=31,
	VT_FILETIME=64,	VT_BLOB=65,		VT_STREAM=66,	VT_STORAGE=67,	VT_STREAMED_OBJECT=68
	VT_STORED_OBJECT=69,			VT_BLOB_OBJECT=70,				VT_CF=71
	VT_CLSID=72,		VT_VECTOR=0x1000,
	VT_ILLEGAL=0xFFFFFFFF,
	VT_TYPEMASK=0xFFF,		// a mask for masking VT_VECTOR and other modifiers to get the raw VT_ value.
	} PROPVARENUM;
#endif // VT_EMPTY

#define	VT_ILLEGAL 0xFFFF

typedef enum
    {
    PRSPEC_LPWSTR   = 0,
    PRSPEC_PROPID   = 1,
    } PRSPEC;

// typedef LONG	PROPID;

typedef struct tagPROPSPEC {
	ULONG ulKind;		// PRSPEC_LPWSTR or PRSPEC_PROPID
	union {
		LPWSTR	lpwstr;
        		PROPID	propid;
	}DUMMYUNIONNAME;
} PROPSPEC;

typedef GUID		FMTID;

typedef struct tagSTATPROPSETSTG {	// used in IPropertySetStorage::Enum and IPropertyStorage::Stat
	FMTID		fmtid;		// The fmtid name of this property set.
	CLSID		clsid;		// The class id of this property set.
	DWORD		grfFlags;		// The flag values of this property set as specified in IPropertySetStorage::Create.
	FILETIME		mtime;		// The time in UTC at which this property set was last modified
	FILETIME		ctime;		// The time in UTC at which this property set was created.
	FILETIME		atime;		// The time in UTC at which this property set was last accessed.
	} STATPROPSETSTG;

typedef struct  tagSTATPROPSTG
    {
    LPOLESTR lpwstrName;
    PROPID propid;
    VARTYPE vt;
    }	STATPROPSTG;

#undef  INTERFACE
#define INTERFACE   IEnumSTATPROPSTG

DECLARE_INTERFACE_(IEnumSTATPROPSTG, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Next) (THIS_ ULONG celt, STATPROPSTG *rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumSTATPROPSTG **ppenum) PURE;
};


#undef  INTERFACE
#define INTERFACE   IEnumSTATPROPSETSTG

DECLARE_INTERFACE_(IEnumSTATPROPSETSTG, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Next) (THIS_ ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumSTATPROPSTG **ppenum) PURE;
};


#undef  INTERFACE
#define INTERFACE   IPropertyStorage

DECLARE_INTERFACE_(IPropertyStorage, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(ReadMultiple)(THIS_ ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[]) PURE;
    STDMETHOD(WriteMultiple)(THIS_ ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst) PURE;
    STDMETHOD(DeleteMultiple)(THIS_ ULONG cpspec, const PROPSPEC rgpspec[]) PURE;
    STDMETHOD(ReadPropertyNames)(THIS_ ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[]) PURE;
    STDMETHOD(WritePropertyNames)(THIS_ ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[]) PURE;
    STDMETHOD(DeletePropertyNames)(THIS_ ULONG cpropid, const PROPID rgpropid[]) PURE;
    STDMETHOD(SetClass)(THIS_ REFCLSID clsid) PURE;
    STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags) PURE;
    STDMETHOD(Revert)(THIS) PURE;
    STDMETHOD(Enum)(THIS_ IEnumSTATPROPSTG** ppenm) PURE;
    STDMETHOD(Stat)(THIS_ STATPROPSETSTG* pstatpsstg) PURE;
    STDMETHOD(SetTimes)(THIS_ const FILETIME* pmtime, const FILETIME* pctime, const FILETIME* patime) PURE;
};

typedef REFGUID	REFFMTID;

#undef  INTERFACE
#define INTERFACE   IPropertySetStorage

DECLARE_INTERFACE_(IPropertySetStorage, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Create)(THIS_ REFFMTID fmtid, const CLSID* pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg)PURE;
    STDMETHOD(Open)(THIS_ REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg) PURE;
    STDMETHOD(Delete)(THIS_ REFFMTID fmtid) PURE;
    STDMETHOD(Enum)(THIS_ IEnumSTATPROPSETSTG** ppenum) PURE;
};

typedef enum PROPSETFLAG {
    PROPSETFLAG_NONSIMPLE	= 1,
    PROPSETFLAG_ANSI			= 2,
    } PROPSETFLAG;

extern  const IID IID_IPropertyStorage;
extern  const IID IID_IEnumSTATPROPSTG;
extern  const IID IID_IPropertySetStorage;
extern  const IID IID_IEnumSTATPROPSETSTG;


#ifdef __cplusplus
}

#endif  /* __cplusplus */

#endif // __IPropertyStorage_INTERFACE_DEFINED__

#endif // _PROPSET_H_



