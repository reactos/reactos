/*
 * Defines the COM interfaces and APIs related to saving properties to file.
 */

#ifndef __WINE_WINE_OBJ_PROPERTYSTORAGE_H
#define __WINE_WINE_OBJ_PROPERTYSTORAGE_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumSTATPROPSETSTG,	0x0000013bL, 0, 0);
typedef struct IEnumSTATPROPSETSTG IEnumSTATPROPSETSTG,*LPENUMSTATPROPSETSTG;

DEFINE_OLEGUID(IID_IEnumSTATPROPSTG,	0x00000139L, 0, 0);
typedef struct IEnumSTATPROPSTG IEnumSTATPROPSTG,*LPENUMSTATPROPSTG;

DEFINE_OLEGUID(IID_IPropertySetStorage,	0x0000013aL, 0, 0);
typedef struct IPropertySetStorage IPropertySetStorage,*LPPROPERTYSETSTORAGE;

DEFINE_OLEGUID(IID_IPropertyStorage,	0x00000138L, 0, 0);
typedef struct IPropertyStorage IPropertyStorage,*LPPROPERTYSTORAGE;


/*****************************************************************************
 * Predeclare the structures
 */

typedef struct tagSTATPROPSETSTG STATPROPSETSTG;
typedef struct tagSTATPROPSTG STATPROPSTG;

extern const FMTID FMTID_SummaryInformation;
extern const FMTID FMTID_DocSummaryInformation;
extern const FMTID FMTID_UserDefinedProperties;

/*****************************************************************************
 * PROPSPEC structure
 */

/* Reserved global Property IDs */
#define PID_DICTIONARY  ( 0 )

#define PID_CODEPAGE    ( 0x1 )

#define PID_FIRST_USABLE        ( 0x2 )

#define PID_FIRST_NAME_DEFAULT  ( 0xfff )

#define PID_LOCALE      ( 0x80000000 )

#define PID_MODIFY_TIME ( 0x80000001 )

#define PID_SECURITY    ( 0x80000002 )

#define PID_ILLEGAL     ( 0xffffffff )

/* Property IDs for the SummaryInformation Property Set */

#define PIDSI_TITLE               0x00000002L  /* VT_LPSTR */
#define PIDSI_SUBJECT             0x00000003L  /* VT_LPSTR */
#define PIDSI_AUTHOR              0x00000004L  /* VT_LPSTR */
#define PIDSI_KEYWORDS            0x00000005L  /* VT_LPSTR */
#define PIDSI_COMMENTS            0x00000006L  /* VT_LPSTR */
#define PIDSI_TEMPLATE            0x00000007L  /* VT_LPSTR */
#define PIDSI_LASTAUTHOR          0x00000008L  /* VT_LPSTR */
#define PIDSI_REVNUMBER           0x00000009L  /* VT_LPSTR */
#define PIDSI_EDITTIME            0x0000000aL  /* VT_FILETIME (UTC) */
#define PIDSI_LASTPRINTED         0x0000000bL  /* VT_FILETIME (UTC) */
#define PIDSI_CREATE_DTM          0x0000000cL  /* VT_FILETIME (UTC) */
#define PIDSI_LASTSAVE_DTM        0x0000000dL  /* VT_FILETIME (UTC) */
#define PIDSI_PAGECOUNT           0x0000000eL  /* VT_I4 */
#define PIDSI_WORDCOUNT           0x0000000fL  /* VT_I4 */
#define PIDSI_CHARCOUNT           0x00000010L  /* VT_I4 */
#define PIDSI_THUMBNAIL           0x00000011L  /* VT_CF */
#define PIDSI_APPNAME             0x00000012L  /* VT_LPSTR */
#define PIDSI_DOC_SECURITY        0x00000013L  /* VT_I4 */
#define PRSPEC_INVALID  ( 0xffffffff )


#define PRSPEC_LPWSTR   ( 0 )
#define PRSPEC_PROPID   ( 1 )

typedef struct tagPROPSPEC
{
    ULONG ulKind;
    union 
    {
        PROPID propid;
        LPOLESTR lpwstr;
    } u;
} PROPSPEC;


/*****************************************************************************
 * STATPROPSETSTG structure
 */
/* Macros for parsing the OS Version of the Property Set Header */
#define PROPSETHDR_OSVER_KIND(dwOSVer)      HIWORD( (dwOSVer) )
#define PROPSETHDR_OSVER_MAJOR(dwOSVer)     LOBYTE(LOWORD( (dwOSVer) ))
#define PROPSETHDR_OSVER_MINOR(dwOSVer)     HIBYTE(LOWORD( (dwOSVer) ))
#define PROPSETHDR_OSVERSION_UNKNOWN        0xFFFFFFFF

struct tagSTATPROPSETSTG
{
    FMTID fmtid;
    CLSID clsid;
    DWORD grfFlags;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD dwOSVersion;
};


/*****************************************************************************
 * STATPROPSTG structure
 */
struct tagSTATPROPSTG
{
    LPOLESTR lpwstrName;
    PROPID propid;
    VARTYPE vt;
};


/*****************************************************************************
 * IEnumSTATPROPSETSTG interface
 */
#define ICOM_INTERFACE IEnumSTATPROPSETSTG
#define IEnumSTATPROPSETSTG_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, STATPROPSETSTG*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumSTATPROPSETSTG**,ppenum)
#define IEnumSTATPROPSETSTG_IMETHODS \
		IUnknown_IMETHODS \
		IEnumSTATPROPSETSTG_METHODS
ICOM_DEFINE(IEnumSTATPROPSETSTG,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumSTATPROPSETSTG_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumSTATPROPSETSTG_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumSTATPROPSETSTG_Release(p)            ICOM_CALL (Release,p)
/*** IEnumSTATPROPSETSTG methods ***/
#define IEnumSTATPROPSETSTG_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumSTATPROPSETSTG_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumSTATPROPSETSTG_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumSTATPROPSETSTG_Clone(p,a)    ICOM_CALL1(Clone,p,a)


/*****************************************************************************
 * IEnumSTATPROPSTG interface
 */
#define ICOM_INTERFACE IEnumSTATPROPSTG
#define IEnumSTATPROPSTG_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, STATPROPSTG*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumSTATPROPSTG**,ppenum)
#define IEnumSTATPROPSTG_IMETHODS \
		IUnknown_IMETHODS \
		IEnumSTATPROPSTG_METHODS 
ICOM_DEFINE(IEnumSTATPROPSTG,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumSTATPROPSTG_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumSTATPROPSTG_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumSTATPROPSTG_Release(p)            ICOM_CALL (Release,p)
/*** IEnumSTATPROPSTG methods ***/
#define IEnumSTATPROPSTG_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumSTATPROPSTG_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumSTATPROPSTG_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumSTATPROPSTG_Clone(p,a)    ICOM_CALL1(Clone,p,a)


/*****************************************************************************
 * IPropertySetStorage interface
 */
#define ICOM_INTERFACE IPropertySetStorage
#define IPropertySetStorage_METHODS \
    ICOM_METHOD5(HRESULT,Create, REFFMTID,rfmtid, const CLSID*,pclsid, DWORD,grfFlags, DWORD,grfMode, IPropertyStorage**,ppprstg) \
    ICOM_METHOD3(HRESULT,Open,   REFFMTID,rfmtid, DWORD,grfMode, IPropertyStorage**,ppprstg) \
    ICOM_METHOD1(HRESULT,Delete, REFFMTID,rfmtid) \
    ICOM_METHOD1(HRESULT,Enum,   IEnumSTATPROPSETSTG**,ppenum)
#define IPropertySetStorage_IMETHODS \
		IUnknown_IMETHODS \
		IPropertySetStorage_METHODS
ICOM_DEFINE(IPropertySetStorage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertySetStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertySetStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPropertySetStorage_Release(p)            ICOM_CALL (Release,p)
/*** IPropertySetStorage methods ***/
#define IPropertySetStorage_Create(p,a,b,c,d,e) ICOM_CALL5(Create,p,a,b,c,d,e)
#define IPropertySetStorage_Open(p,a,b,c)       ICOM_CALL3(Open,p,a,b,c)
#define IPropertySetStorage_Delete(p,a)         ICOM_CALL1(Delete,p,a)
#define IPropertySetStorage_Enum(p,a)           ICOM_CALL1(Enum,p,a)


/*****************************************************************************
 * IPropertyStorage interface
 */
typedef struct tagPROPVARIANT PROPVARIANT,*LPPROPVARIANT;

/* Flags for IPropertySetStorage::Create */
#define PROPSETFLAG_DEFAULT     ( 0 )
#define PROPSETFLAG_NONSIMPLE   ( 1 )
#define PROPSETFLAG_ANSI        ( 2 )

typedef struct  tagCAUB
{
    ULONG cElems;
    unsigned char *pElems;
} CAUB;

typedef struct tagCAI
{
    ULONG cElems;
    short *pElems;
} CAI;

typedef struct tagCAUI
{
    ULONG cElems;
    USHORT *pElems;
} CAUI;

typedef struct tagCAL
{
    ULONG cElems;
    long *pElems;
} CAL;

typedef struct tagCAUL
{
    ULONG cElems;
    ULONG *pElems;
} CAUL;

typedef struct tagCAFLT
{
    ULONG cElems;
    float *pElems;
} CAFLT;

typedef struct tagCADBL
{
    ULONG cElems;
    double *pElems;
} CADBL;

typedef struct tagCACY
{
    ULONG cElems;
    CY *pElems;
} CACY;

typedef struct tagCADATE
{
    ULONG cElems;
    DATE *pElems;
} CADATE;

typedef struct tagCABSTR
{
    ULONG cElems;
    BSTR *pElems;
} CABSTR;

typedef struct tagCABOOL
{
    ULONG cElems;
    VARIANT_BOOL *pElems;
} CABOOL;

typedef struct tagCASCODE
{
    ULONG cElems;
    SCODE *pElems;
} CASCODE;

typedef struct tagCAPROPVARIANT
{
    ULONG cElems;
    PROPVARIANT *pElems;
} CAPROPVARIANT;

typedef struct tagCAH
{
    ULONG cElems;
    LARGE_INTEGER *pElems;
} CAH;

typedef struct tagCAUH
{
    ULONG cElems;
    ULARGE_INTEGER *pElems;
} CAUH;

typedef struct tagCALPSTR
{
    ULONG cElems;
    LPSTR *pElems;
} CALPSTR;

typedef struct tagCALPWSTR
{
    ULONG cElems;
    LPWSTR *pElems;
} CALPWSTR;

typedef struct tagCAFILETIME
{
    ULONG cElems;
    FILETIME *pElems;
} CAFILETIME;

typedef struct tagCACLIPDATA
{
    ULONG cElems;
    CLIPDATA *pElems;
} CACLIPDATA;

typedef struct tagCACLSID
{
    ULONG cElems;
    CLSID *pElems;
} CACLSID;

struct tagPROPVARIANT
{
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union 
    {
         /* Empty union arm */ 
        UCHAR bVal;
        short iVal;
        USHORT uiVal;
        VARIANT_BOOL boolVal;
#ifndef __cplusplus
       /* FIXME: bool is reserved in C++, how can we deal with that ? */
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
        IStream *pStream;
        IStorage *pStorage;
        BSTR bstrVal;
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
        CALPSTR calpstr;
        CALPWSTR calpwstr;
        CAPROPVARIANT capropvar;
    } u;
};


#define ICOM_INTERFACE IPropertyStorage
#define IPropertyStorage_METHODS \
    ICOM_METHOD3(HRESULT,ReadMultiple,        ULONG,cpspec, const PROPSPEC*,rgpspec, PROPVARIANT*,rgpropvar) \
    ICOM_METHOD4(HRESULT,WriteMultiple,       ULONG,cpspec, const PROPSPEC*,rgpspec, const PROPVARIANT*,rgpropvar, PROPID,propidNameFirst) \
    ICOM_METHOD2(HRESULT,DeleteMultiple,      ULONG,cpspec, const PROPSPEC*,rgpspec) \
    ICOM_METHOD2(HRESULT,ReadPropertyNames,   const PROPID*,rgpropid, LPOLESTR*,rglpwstrName) \
    ICOM_METHOD3(HRESULT,WritePropertyNames,  ULONG,cpropid, const PROPID*,rgpropid, LPOLESTR*,rglpwstrName) \
    ICOM_METHOD2(HRESULT,DeletePropertyNames, ULONG,cpropid, const PROPID*,rgpropid) \
    ICOM_METHOD1(HRESULT,Commit,              DWORD,grfCommitFlags) \
    ICOM_METHOD (HRESULT,Revert) \
    ICOM_METHOD1(HRESULT,Enum,                IEnumSTATPROPSTG**,ppenum) \
    ICOM_METHOD3(HRESULT,SetTimes,            const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime) \
    ICOM_METHOD1(HRESULT,SetClass,            REFCLSID,clsid) \
    ICOM_METHOD1(HRESULT,Stat,                STATPROPSETSTG*,pstatpsstg)
#define IPropertyStorage_IMETHODS \
		IUnknown_IMETHODS \
		IPropertyStorage_METHODS
ICOM_DEFINE(IPropertyStorage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertyStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPropertyStorage_Release(p)            ICOM_CALL (Release,p)
/*** IPropertyStorage methods ***/
#define IPropertyStorage_ReadMultiple(p,a,b,c)       ICOM_CALL3(ReadMultiple,p,a,b,c)
#define IPropertyStorage_WriteMultiple(p,a,b,c,d)    ICOM_CALL4(WriteMultiple,p,a,b,c,d)
#define IPropertyStorage_DeleteMultiple(p,a,b)       ICOM_CALL2(DeleteMultiple,p,a,b)
#define IPropertyStorage_ReadPropertyNames(p,a,b)    ICOM_CALL2(ReadPropertyNames,p,a,b)
#define IPropertyStorage_WritePropertyNames(p,a,b,c) ICOM_CALL3(WritePropertyNames,p,a,b,c)
#define IPropertyStorage_DeletePropertyNames(p,a,b)  ICOM_CALL2(DeletePropertyNames,p,a,b)
#define IPropertyStorage_Commit(p,a)                 ICOM_CALL1(Commit,p,a)
#define IPropertyStorage_Revert(p)                   ICOM_CALL (Revert,p)
#define IPropertyStorage_Enum(p,a)                   ICOM_CALL1(Enum,p,a)
#define IPropertyStorage_SetTimes(p,a,b,c)           ICOM_CALL3(SetTimes,p,a,b,c)
#define IPropertyStorage_SetClass(p,a)               ICOM_CALL1(SetClass,p,a)
#define IPropertyStorage_Stat(p,a)                   ICOM_CALL1(Stat,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PROPERTYSTORAGE_H */
