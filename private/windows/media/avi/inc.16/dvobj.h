/*****************************************************************************\
*                                                                             *
* dvobj.h -	Data/view value types, interface and APIs		      *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

#if !defined( _DVOBJ_H_ )
#define _DVOBJ_H_

/****** DV value types ******************************************************/

//      forward type declarations
#if defined(__cplusplus)
interface IStorage;
interface IStream;
interface IAdviseSink;
interface IMoniker;
#else 
typedef interface IStorage IStorage;
typedef interface IStream IStream;
typedef interface IAdviseSink IAdviseSink;
typedef interface IMoniker IMoniker;
#endif

typedef            IStorage FAR* LPSTORAGE;
typedef             IStream FAR* LPSTREAM;
typedef         IAdviseSink FAR* LPADVISESINK;
typedef             IMoniker FAR* LPMONIKER;


#if !defined(_MAC)
typedef WORD CLIPFORMAT;
#else
typedef unsigned long CLIPFORMAT;            // ResType
#endif
typedef  CLIPFORMAT FAR* LPCLIPFORMAT;


// Data/View aspect; specifies the desired aspect of the object when 
// drawing or getting data.
typedef enum tagDVASPECT
{
    DVASPECT_CONTENT = 1,
    DVASPECT_THUMBNAIL = 2,
    DVASPECT_ICON = 4,
    DVASPECT_DOCPRINT = 8
} DVASPECT;


// Data/View target device; determines the device for drawing or gettting data
typedef struct FARSTRUCT tagDVTARGETDEVICE
{
    DWORD tdSize;
    WORD tdDriverNameOffset;
    WORD tdDeviceNameOffset;
    WORD tdPortNameOffset;
    WORD tdExtDevmodeOffset;
    BYTE tdData[1];
} DVTARGETDEVICE;


// Format, etc.; completely specifices the kind of data desired, including tymed
typedef struct FARSTRUCT tagFORMATETC
{
    CLIPFORMAT          cfFormat;
    DVTARGETDEVICE FAR* ptd;
    DWORD               dwAspect;
    LONG                lindex;
    DWORD               tymed;
} FORMATETC, FAR* LPFORMATETC;


// TYpes of storage MEDiums; determines how data is stored or passed around
typedef enum tagTYMED
{
    TYMED_HGLOBAL = 1,
    TYMED_FILE = 2,
    TYMED_ISTREAM = 4,
    TYMED_ISTORAGE = 8,
    TYMED_GDI = 16,
    TYMED_MFPICT = 32,
    TYMED_NULL = 0
} TYMED;


// DATA format DIRection
typedef enum tagDATADIR
{
    DATADIR_GET = 1,
    DATADIR_SET = 2,
} DATADIR;


// SToraGe MEDIUM; a block of data on a particular medium 
typedef struct FARSTRUCT tagSTGMEDIUM
{
    DWORD   tymed;
    union
    {
        HANDLE  hGlobal;
        LPSTR   lpszFileName;
        IStream FAR* pstm;
        IStorage FAR* pstg;
    }
#ifdef NONAMELESSUNION
    u       // add a tag when name less unions not supported
#endif
    ;
    IUnknown FAR* pUnkForRelease;
} STGMEDIUM, FAR* LPSTGMEDIUM;


// Advise Flags
typedef enum tagADVF
{
    ADVF_NODATA = 1,
    ADVF_PRIMEFIRST = 2,
    ADVF_ONLYONCE = 4,
    ADVF_DATAONSTOP = 64,
    ADVFCACHE_NOHANDLER = 8,
    ADVFCACHE_FORCEBUILTIN = 16,
    ADVFCACHE_ONSAVE = 32
} ADVF;


// Stats for data; used by several enumerations and by at least one 
// implementation of IDataAdviseHolder; if a field is not used, it
// will be NULL.
typedef struct FARSTRUCT tagSTATDATA
{                                   // field used by:
    FORMATETC formatetc;            // EnumAdvise, EnumData (cache), EnumFormats
    DWORD advf;                     // EnumAdvise, EnumData (cache)
    IAdviseSink FAR* pAdvSink;      // EnumAdvise
    DWORD dwConnection;             // EnumAdvise
} STATDATA;
    
typedef  STATDATA FAR* LPSTATDATA;



/****** DV Interfaces ***************************************************/


#undef  INTERFACE
#define INTERFACE   IEnumFORMATETC

DECLARE_INTERFACE_(IEnumFORMATETC, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumFORMATETC methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, FORMATETC FAR * rgelt, ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumFORMATETC FAR* FAR* ppenum) PURE;
};
typedef        IEnumFORMATETC FAR* LPENUMFORMATETC;


#undef  INTERFACE
#define INTERFACE   IEnumSTATDATA

DECLARE_INTERFACE_(IEnumSTATDATA, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IEnumSTATDATA methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, STATDATA FAR * rgelt, ULONG FAR* pceltFetched) PURE;
    STDMETHOD(Skip) (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumSTATDATA FAR* FAR* ppenum) PURE;
};
typedef        IEnumSTATDATA FAR* LPENUMSTATDATA;



#undef  INTERFACE
#define INTERFACE   IDataObject

#define DATA_E_FORMATETC        DV_E_FORMATETC
#define DATA_S_SAMEFORMATETC    (DATA_S_FIRST + 0)

DECLARE_INTERFACE_(IDataObject, IUnknown)
{ 
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IDataObject methods ***
    STDMETHOD(GetData) (THIS_ LPFORMATETC pformatetcIn,
                            LPSTGMEDIUM pmedium ) PURE;
    STDMETHOD(GetDataHere) (THIS_ LPFORMATETC pformatetc,
                            LPSTGMEDIUM pmedium ) PURE;
    STDMETHOD(QueryGetData) (THIS_ LPFORMATETC pformatetc ) PURE;
    STDMETHOD(GetCanonicalFormatEtc) (THIS_ LPFORMATETC pformatetc,
                            LPFORMATETC pformatetcOut) PURE;
    STDMETHOD(SetData) (THIS_ LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
                            BOOL fRelease) PURE;
    STDMETHOD(EnumFormatEtc) (THIS_ DWORD dwDirection,
                            LPENUMFORMATETC FAR* ppenumFormatEtc) PURE;

    STDMETHOD(DAdvise) (THIS_ FORMATETC FAR* pFormatetc, DWORD advf, 
                    LPADVISESINK pAdvSink, DWORD FAR* pdwConnection) PURE;
    STDMETHOD(DUnadvise) (THIS_ DWORD dwConnection) PURE;
    STDMETHOD(EnumDAdvise) (THIS_ LPENUMSTATDATA FAR* ppenumAdvise) PURE;
};                 
typedef      IDataObject FAR* LPDATAOBJECT;



#undef  INTERFACE
#define INTERFACE   IViewObject

#define VIEW_E_DRAW             (VIEW_E_FIRST)
#define E_DRAW                  VIEW_E_DRAW

#define VIEW_S_ALREADY_FROZEN   (VIEW_S_FIRST)

DECLARE_INTERFACE_(IViewObject, IUnknown)
{ 
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IViewObject methods ***
    STDMETHOD(Draw) (THIS_ DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    HDC hdcDraw, 
                    const LPRECTL lprcBounds, 
                    const LPRECTL lprcWBounds,
                    BOOL (CALLBACK * pfnContinue) (DWORD), 
                    DWORD dwContinue) PURE;

    STDMETHOD(GetColorSet) (THIS_ DWORD dwDrawAspect, LONG lindex,
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                    HDC hicTargetDev,
                    LPLOGPALETTE FAR* ppColorSet) PURE;

    STDMETHOD(Freeze)(THIS_ DWORD dwDrawAspect, LONG lindex, 
                    void FAR* pvAspect,
                    DWORD FAR* pdwFreeze) PURE;
    STDMETHOD(Unfreeze) (THIS_ DWORD dwFreeze) PURE;
    STDMETHOD(SetAdvise) (THIS_ DWORD aspects, DWORD advf, 
                    LPADVISESINK pAdvSink) PURE;
    STDMETHOD(GetAdvise) (THIS_ DWORD FAR* pAspects, DWORD FAR* pAdvf, 
                    LPADVISESINK FAR* ppAdvSink) PURE;
};
typedef      IViewObject FAR* LPVIEWOBJECT;


#undef  INTERFACE
#define INTERFACE   IAdviseSink

DECLARE_INTERFACE_(IAdviseSink, IUnknown)
{ 
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAdviseSink methods ***
    STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC FAR* pFormatetc, 
                            STGMEDIUM FAR* pStgmed) PURE;
    STDMETHOD_(void,OnViewChange)(THIS_ DWORD dwAspect, LONG lindex) PURE;
    STDMETHOD_(void,OnRename)(THIS_ LPMONIKER pmk) PURE;
    STDMETHOD_(void,OnSave)(THIS) PURE;
    STDMETHOD_(void,OnClose)(THIS) PURE;
};
typedef      IAdviseSink FAR* LPADVISESINK;



#undef  INTERFACE
#define INTERFACE   IAdviseSink2

DECLARE_INTERFACE_(IAdviseSink2, IAdviseSink)
{ 
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAdviseSink methods ***
    STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC FAR* pFormatetc, 
                            STGMEDIUM FAR* pStgmed) PURE;
    STDMETHOD_(void,OnViewChange)(THIS_ DWORD dwAspect, LONG lindex) PURE;
    STDMETHOD_(void,OnRename)(THIS_ LPMONIKER pmk) PURE;
    STDMETHOD_(void,OnSave)(THIS) PURE;
    STDMETHOD_(void,OnClose)(THIS) PURE;

    // *** IAdviseSink2 methods ***
    STDMETHOD_(void,OnLinkSrcChange)(THIS_ LPMONIKER pmk) PURE;
};
typedef      IAdviseSink2 FAR* LPADVISESINK2;



#undef  INTERFACE
#define INTERFACE   IDataAdviseHolder

DECLARE_INTERFACE_(IDataAdviseHolder, IUnknown)
{ 
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDataAdviseHolder methods ***
    STDMETHOD(Advise)(THIS_ LPDATAOBJECT pDataObject, FORMATETC FAR* pFetc, 
            DWORD advf, LPADVISESINK pAdvise, DWORD FAR* pdwConnection) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) PURE;
    STDMETHOD(EnumAdvise)(THIS_ LPENUMSTATDATA FAR* ppenumAdvise) PURE;

    STDMETHOD(SendOnDataChange)(THIS_ LPDATAOBJECT pDataObject, DWORD dwReserved, DWORD advf) PURE;
};
typedef      IDataAdviseHolder FAR* LPDATAADVISEHOLDER;



#undef  INTERFACE
#define INTERFACE   IOleCache

#define CACHE_E_NOCACHE_UPDATED         (CACHE_E_FIRST)

#define CACHE_S_FORMATETC_NOTSUPPORTED  (CACHE_S_FIRST)
#define CACHE_S_SAMECACHE               (CACHE_S_FIRST+1)
#define CACHE_S_SOMECACHES_NOTUPDATED   (CACHE_S_FIRST+2)


DECLARE_INTERFACE_(IOleCache, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleCache methods ***
    STDMETHOD(Cache) (THIS_ LPFORMATETC lpFormatetc, DWORD advf, LPDWORD lpdwConnection) PURE;
    STDMETHOD(Uncache) (THIS_ DWORD dwConnection) PURE;
    STDMETHOD(EnumCache) (THIS_ LPENUMSTATDATA FAR* ppenumStatData) PURE;
    STDMETHOD(InitCache) (THIS_ LPDATAOBJECT pDataObject) PURE;
    STDMETHOD(SetData) (THIS_ LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
                            BOOL fRelease) PURE;
};
typedef         IOleCache FAR* LPOLECACHE;


/****** DV APIs ***********************************************************/


STDAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder);


#endif // _DVOBJ_H_
