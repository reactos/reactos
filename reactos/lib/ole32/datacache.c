/*
 *	OLE 2 Data cache
 *
 *      Copyright 1999  Francis Beaudet
 *      Copyright 2000  Abey George
 *
 * NOTES:
 *    The OLE2 data cache supports a whole whack of
 *    interfaces including:
 *       IDataObject, IPersistStorage, IViewObject2,
 *       IOleCache2 and IOleCacheControl.
 *
 *    Most of the implementation details are taken from: Inside OLE
 *    second edition by Kraig Brockschmidt,
 *
 * NOTES
 *  -  This implementation of the datacache will let your application
 *     load documents that have embedded OLE objects in them and it will
 *     also retrieve the metafile representation of those objects. 
 *  -  This implementation of the datacache will also allow your
 *     application to save new documents with OLE objects in them.
 *  -  The main thing that it doesn't do is allow you to activate 
 *     or modify the OLE objects in any way.
 *  -  I haven't found any good documentation on the real usage of
 *     the streams created by the data cache. In particular, How to
 *     determine what the XXX stands for in the stream name
 *     "\002OlePresXXX". It appears to just be a counter.
 *  -  Also, I don't know the real content of the presentation stream
 *     header. I was able to figure-out where the extent of the object 
 *     was stored and the aspect, but that's about it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>


/****************************************************************************
 * PresentationDataHeader
 *
 * This structure represents the header of the \002OlePresXXX stream in
 * the OLE object strorage.
 *
 * Most fields are still unknown.
 */
typedef struct PresentationDataHeader
{
  DWORD unknown1;	/* -1 */
  DWORD unknown2;	/* 3, possibly CF_METAFILEPICT */
  DWORD unknown3;	/* 4, possibly TYMED_ISTREAM */
  DVASPECT dvAspect;
  DWORD unknown5;	/* -1 */

  DWORD unknown6;
  DWORD unknown7;	/* 0 */
  DWORD dwObjectExtentX;
  DWORD dwObjectExtentY;
  DWORD dwSize;
} PresentationDataHeader;

/****************************************************************************
 * DataCache
 */
struct DataCache
{
  /*
   * List all interface VTables here
   */
  ICOM_VTABLE(IDataObject)*      lpvtbl1; 
  ICOM_VTABLE(IUnknown)*         lpvtbl2;
  ICOM_VTABLE(IPersistStorage)*  lpvtbl3;
  ICOM_VTABLE(IViewObject2)*     lpvtbl4;  
  ICOM_VTABLE(IOleCache2)*       lpvtbl5;
  ICOM_VTABLE(IOleCacheControl)* lpvtbl6;

  /*
   * Reference count of this object
   */
  ULONG ref;

  /*
   * IUnknown implementation of the outer object.
   */
  IUnknown* outerUnknown;

  /*
   * This storage pointer is set through a call to
   * IPersistStorage_Load. This is where the visual
   * representation of the object is stored.
   */
  IStorage* presentationStorage;

  /*
   * The user of this object can setup ONE advise sink
   * connection with the object. These parameters describe
   * that connection.
   */
  DWORD        sinkAspects;
  DWORD        sinkAdviseFlag;
  IAdviseSink* sinkInterface;

};

typedef struct DataCache DataCache;

/*
 * Here, I define utility macros to help with the casting of the 
 * "this" parameter.
 * There is a version to accomodate all of the VTables implemented
 * by this object.
 */
#define _ICOM_THIS_From_IDataObject(class,name)       class* this = (class*)name;
#define _ICOM_THIS_From_NDIUnknown(class, name)       class* this = (class*)(((char*)name)-sizeof(void*)); 
#define _ICOM_THIS_From_IPersistStorage(class, name)  class* this = (class*)(((char*)name)-2*sizeof(void*)); 
#define _ICOM_THIS_From_IViewObject2(class, name)     class* this = (class*)(((char*)name)-3*sizeof(void*)); 
#define _ICOM_THIS_From_IOleCache2(class, name)       class* this = (class*)(((char*)name)-4*sizeof(void*)); 
#define _ICOM_THIS_From_IOleCacheControl(class, name) class* this = (class*)(((char*)name)-5*sizeof(void*)); 

/*
 * Prototypes for the methods of the DataCache class.
 */
static DataCache* DataCache_Construct(REFCLSID  clsid,
				      LPUNKNOWN pUnkOuter);
static void       DataCache_Destroy(DataCache* ptrToDestroy);
static HRESULT    DataCache_ReadPresentationData(DataCache*              this,
						 DWORD                   drawAspect,
						 PresentationDataHeader* header);
static HRESULT    DataCache_OpenPresStream(DataCache *this,
					   DWORD      drawAspect,
					   IStream  **pStm);
static HMETAFILE  DataCache_ReadPresMetafile(DataCache* this,
					     DWORD      drawAspect);
static void       DataCache_FireOnViewChange(DataCache* this,
					     DWORD      aspect,
					     LONG       lindex);

/*
 * Prototypes for the methods of the DataCache class
 * that implement non delegating IUnknown methods.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject);
static ULONG WINAPI DataCache_NDIUnknown_AddRef( 
            IUnknown*      iface);
static ULONG WINAPI DataCache_NDIUnknown_Release( 
            IUnknown*      iface);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IDataObject methods.
 */
static HRESULT WINAPI DataCache_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IDataObject_AddRef( 
            IDataObject*     iface);
static ULONG WINAPI DataCache_IDataObject_Release( 
            IDataObject*     iface);
static HRESULT WINAPI DataCache_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn, 
	    STGMEDIUM*       pmedium);
static HRESULT WINAPI DataCache_GetDataHere(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium);
static HRESULT WINAPI DataCache_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc);
static HRESULT WINAPI DataCache_GetCanonicalFormatEtc(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatectIn, 
	    LPFORMATETC      pformatetcOut);
static HRESULT WINAPI DataCache_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc, 
	    STGMEDIUM*       pmedium, 
	    BOOL             fRelease);
static HRESULT WINAPI DataCache_EnumFormatEtc(
	    IDataObject*     iface,       
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc);
static HRESULT WINAPI DataCache_DAdvise(
	    IDataObject*     iface, 
	    FORMATETC*       pformatetc, 
	    DWORD            advf, 
	    IAdviseSink*     pAdvSink, 
	    DWORD*           pdwConnection);
static HRESULT WINAPI DataCache_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection);
static HRESULT WINAPI DataCache_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_IPersistStorage_QueryInterface(
            IPersistStorage* iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IPersistStorage_AddRef( 
            IPersistStorage* iface);
static ULONG WINAPI DataCache_IPersistStorage_Release( 
            IPersistStorage* iface);
static HRESULT WINAPI DataCache_GetClassID( 
            IPersistStorage* iface,
	    CLSID*           pClassID);
static HRESULT WINAPI DataCache_IsDirty( 
            IPersistStorage* iface);
static HRESULT WINAPI DataCache_InitNew( 
            IPersistStorage* iface, 
	    IStorage*        pStg);
static HRESULT WINAPI DataCache_Load( 
            IPersistStorage* iface,
	    IStorage*        pStg);
static HRESULT WINAPI DataCache_Save( 
            IPersistStorage* iface,
	    IStorage*        pStg, 
	    BOOL             fSameAsLoad);
static HRESULT WINAPI DataCache_SaveCompleted( 
            IPersistStorage* iface,  
	    IStorage*        pStgNew);
static HRESULT WINAPI DataCache_HandsOffStorage(
            IPersistStorage* iface);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_IViewObject2_QueryInterface(
            IViewObject2* iface,
            REFIID           riid,
            void**           ppvObject);
static ULONG WINAPI DataCache_IViewObject2_AddRef( 
            IViewObject2* iface);
static ULONG WINAPI DataCache_IViewObject2_Release( 
            IViewObject2* iface);
static HRESULT WINAPI DataCache_Draw(
            IViewObject2*    iface,
	    DWORD            dwDrawAspect,
	    LONG             lindex,
	    void*            pvAspect,
	    DVTARGETDEVICE*  ptd, 
	    HDC              hdcTargetDev, 
	    HDC              hdcDraw,
	    LPCRECTL         lprcBounds,
	    LPCRECTL         lprcWBounds,
	    IVO_ContCallback pfnContinue,
	    DWORD            dwContinue);
static HRESULT WINAPI DataCache_GetColorSet(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    void*           pvAspect, 
	    DVTARGETDEVICE* ptd, 
	    HDC             hicTargetDevice, 
	    LOGPALETTE**    ppColorSet);
static HRESULT WINAPI DataCache_Freeze(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect, 
	    DWORD*          pdwFreeze);
static HRESULT WINAPI DataCache_Unfreeze(
            IViewObject2*   iface,
	    DWORD           dwFreeze);
static HRESULT WINAPI DataCache_SetAdvise(
            IViewObject2*   iface,
	    DWORD           aspects, 
	    DWORD           advf, 
	    IAdviseSink*    pAdvSink);
static HRESULT WINAPI DataCache_GetAdvise(
            IViewObject2*   iface, 
	    DWORD*          pAspects, 
	    DWORD*          pAdvf, 
	    IAdviseSink**   ppAdvSink);
static HRESULT WINAPI DataCache_GetExtent(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    DVTARGETDEVICE* ptd, 
	    LPSIZEL         lpsizel);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IOleCache2 methods.
 */
static HRESULT WINAPI DataCache_IOleCache2_QueryInterface(
            IOleCache2*     iface,
            REFIID          riid,
            void**          ppvObject);
static ULONG WINAPI DataCache_IOleCache2_AddRef( 
            IOleCache2*     iface);
static ULONG WINAPI DataCache_IOleCache2_Release( 
            IOleCache2*     iface);
static HRESULT WINAPI DataCache_Cache(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    DWORD           advf,
	    DWORD*          pdwConnection);
static HRESULT WINAPI DataCache_Uncache(
	    IOleCache2*     iface,
	    DWORD           dwConnection);
static HRESULT WINAPI DataCache_EnumCache(
            IOleCache2*     iface,
	    IEnumSTATDATA** ppenumSTATDATA);
static HRESULT WINAPI DataCache_InitCache(
	    IOleCache2*     iface,
	    IDataObject*    pDataObject);
static HRESULT WINAPI DataCache_IOleCache2_SetData(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    STGMEDIUM*      pmedium,
	    BOOL            fRelease);
static HRESULT WINAPI DataCache_UpdateCache(
            IOleCache2*     iface,
	    LPDATAOBJECT    pDataObject, 
	    DWORD           grfUpdf,
	    LPVOID          pReserved);
static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions);

/*
 * Prototypes for the methods of the DataCache class
 * that implement IOleCacheControl methods.
 */
static HRESULT WINAPI DataCache_IOleCacheControl_QueryInterface(
            IOleCacheControl* iface,
            REFIID            riid,
            void**            ppvObject);
static ULONG WINAPI DataCache_IOleCacheControl_AddRef( 
            IOleCacheControl* iface);
static ULONG WINAPI DataCache_IOleCacheControl_Release( 
            IOleCacheControl* iface);
static HRESULT WINAPI DataCache_OnRun(
	    IOleCacheControl* iface,
	    LPDATAOBJECT      pDataObject);
static HRESULT WINAPI DataCache_OnStop(
	    IOleCacheControl* iface);

/*
 * Virtual function tables for the DataCache class.
 */
static ICOM_VTABLE(IUnknown) DataCache_NDIUnknown_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_NDIUnknown_QueryInterface,
  DataCache_NDIUnknown_AddRef,
  DataCache_NDIUnknown_Release
};

static ICOM_VTABLE(IDataObject) DataCache_IDataObject_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_IDataObject_QueryInterface,
  DataCache_IDataObject_AddRef,
  DataCache_IDataObject_Release,
  DataCache_GetData,
  DataCache_GetDataHere,
  DataCache_QueryGetData,
  DataCache_GetCanonicalFormatEtc,
  DataCache_IDataObject_SetData,
  DataCache_EnumFormatEtc,
  DataCache_DAdvise,
  DataCache_DUnadvise,
  DataCache_EnumDAdvise
};

static ICOM_VTABLE(IPersistStorage) DataCache_IPersistStorage_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_IPersistStorage_QueryInterface,
  DataCache_IPersistStorage_AddRef,
  DataCache_IPersistStorage_Release,
  DataCache_GetClassID,
  DataCache_IsDirty,
  DataCache_InitNew,
  DataCache_Load,
  DataCache_Save,
  DataCache_SaveCompleted,
  DataCache_HandsOffStorage
};

static ICOM_VTABLE(IViewObject2) DataCache_IViewObject2_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_IViewObject2_QueryInterface,
  DataCache_IViewObject2_AddRef,
  DataCache_IViewObject2_Release,
  DataCache_Draw,
  DataCache_GetColorSet,
  DataCache_Freeze,
  DataCache_Unfreeze,
  DataCache_SetAdvise,
  DataCache_GetAdvise,
  DataCache_GetExtent
};

static ICOM_VTABLE(IOleCache2) DataCache_IOleCache2_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_IOleCache2_QueryInterface,
  DataCache_IOleCache2_AddRef,
  DataCache_IOleCache2_Release,
  DataCache_Cache,
  DataCache_Uncache,
  DataCache_EnumCache,
  DataCache_InitCache,
  DataCache_IOleCache2_SetData,
  DataCache_UpdateCache,
  DataCache_DiscardCache
};

static ICOM_VTABLE(IOleCacheControl) DataCache_IOleCacheControl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  DataCache_IOleCacheControl_QueryInterface,
  DataCache_IOleCacheControl_AddRef,
  DataCache_IOleCacheControl_Release,
  DataCache_OnRun,
  DataCache_OnStop
};

/******************************************************************************
 *              CreateDataCache        [OLE32.54]
 */
HRESULT WINAPI CreateDataCache(
  LPUNKNOWN pUnkOuter, 
  REFCLSID  rclsid, 
  REFIID    riid, 
  LPVOID*   ppvObj)
{
  DataCache* newCache = NULL;
  HRESULT    hr       = S_OK;

  Print(MAX_TRACE, ("(%s, %p, %s, %p)\n", debugstr_guid(rclsid), pUnkOuter, debugstr_guid(riid), ppvObj));

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = 0;

  /*
   * If this cache is constructed for aggregation, make sure
   * the caller is requesting the IUnknown interface.
   * This is necessary because it's the only time the non-delegating
   * IUnknown pointer can be returned to the outside.
   */
  if ( (pUnkOuter!=NULL) && 
       (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) != 0) )
    return CLASS_E_NOAGGREGATION;

  /*
   * Try to construct a new instance of the class.
   */
  newCache = DataCache_Construct(rclsid, 
				 pUnkOuter);

  if (newCache == 0)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IUnknown_QueryInterface((IUnknown*)&(newCache->lpvtbl2), riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IUnknown_Release((IUnknown*)&(newCache->lpvtbl2));

  return hr;
}

/*********************************************************
 * Method implementation for DataCache class.
 */
static DataCache* DataCache_Construct(
  REFCLSID  clsid,
  LPUNKNOWN pUnkOuter)
{
  DataCache* newObject = 0;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(DataCache));

  if (newObject==0)
    return newObject;
  
  /*
   * Initialize the virtual function table.
   */
  newObject->lpvtbl1 = &DataCache_IDataObject_VTable;
  newObject->lpvtbl2 = &DataCache_NDIUnknown_VTable;
  newObject->lpvtbl3 = &DataCache_IPersistStorage_VTable;
  newObject->lpvtbl4 = &DataCache_IViewObject2_VTable;
  newObject->lpvtbl5 = &DataCache_IOleCache2_VTable;
  newObject->lpvtbl6 = &DataCache_IOleCacheControl_VTable;
  
  /*
   * Start with one reference count. The caller of this function 
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  /*
   * Initialize the outer unknown
   * We don't keep a reference on the outer unknown since, the way 
   * aggregation works, our lifetime is at least as large as it's
   * lifetime.
   */
  if (pUnkOuter==NULL)
    pUnkOuter = (IUnknown*)&(newObject->lpvtbl2);

  newObject->outerUnknown = pUnkOuter;

  /*
   * Initialize the other members of the structure.
   */
  newObject->presentationStorage = NULL;
  newObject->sinkAspects = 0;
  newObject->sinkAdviseFlag = 0;
  newObject->sinkInterface = 0;

  return newObject;
}

static void DataCache_Destroy(
  DataCache* ptrToDestroy)
{
  Print(MAX_TRACE, ("()\n"));

  if (ptrToDestroy->sinkInterface != NULL)
  {
    IAdviseSink_Release(ptrToDestroy->sinkInterface);
    ptrToDestroy->sinkInterface = NULL;
  }

  if (ptrToDestroy->presentationStorage != NULL)
  {
    IStorage_Release(ptrToDestroy->presentationStorage);
    ptrToDestroy->presentationStorage = NULL;
  }

  /*
   * Free the datacache pointer.
   */
  HeapFree(GetProcessHeap(), 0, ptrToDestroy);
}

/************************************************************************
 * DataCache_ReadPresentationData
 *
 * This method will read information for the requested presentation 
 * into the given structure.
 *
 * Param:
 *   this       - Pointer to the DataCache object
 *   drawAspect - The aspect of the object that we wish to draw.
 *   header     - The structure containing information about this
 *                aspect of the object.
 */
static HRESULT DataCache_ReadPresentationData(
  DataCache*              this,
  DWORD                   drawAspect,
  PresentationDataHeader* header)
{
  IStream* presStream = NULL;
  HRESULT  hres;

  /*
   * Open the presentation stream.
   */
  hres = DataCache_OpenPresStream(
           this,
	   drawAspect,
	   &presStream);

  if (FAILED(hres))
    return hres;

  /*
   * Read the header.
   */

  hres = IStream_Read(
           presStream,
	   header,
	   sizeof(PresentationDataHeader),
	   NULL);

  /*
   * Cleanup.
   */
  IStream_Release(presStream);

  /*
   * We don't want to propagate any other error
   * code than a failure.
   */
  if (hres!=S_OK)
    hres = E_FAIL;

  return hres;
}

/************************************************************************
 * DataCache_FireOnViewChange
 *
 * This method will fire an OnViewChange notification to the advise
 * sink registered with the datacache.
 *
 * See IAdviseSink::OnViewChange for more details.
 */
static void DataCache_FireOnViewChange(
  DataCache* this,
  DWORD      aspect,
  LONG       lindex)
{
  Print(MAX_TRACE, ("(%p, %lx, %ld)\n", this, aspect, lindex));

  /*
   * The sink supplies a filter when it registers
   * we make sure we only send the notifications when that
   * filter matches.
   */
  if ((this->sinkAspects & aspect) != 0)
  {
    if (this->sinkInterface != NULL)
    {
      IAdviseSink_OnViewChange(this->sinkInterface,
			       aspect,
			       lindex);

      /*
       * Some sinks want to be unregistered automatically when
       * the first notification goes out.
       */
      if ( (this->sinkAdviseFlag & ADVF_ONLYONCE) != 0)
      {
	IAdviseSink_Release(this->sinkInterface);

	this->sinkInterface  = NULL;
	this->sinkAspects    = 0;
	this->sinkAdviseFlag = 0;
      }
    }
  }
}

/* Helper for DataCache_OpenPresStream */
static BOOL DataCache_IsPresentationStream(const STATSTG *elem)
{
#if 0
    /* The presentation streams have names of the form "\002OlePresXXX",
     * where XXX goes from 000 to 999. */
    static const WCHAR OlePres[] = { 2,'O','l','e','P','r','e','s' };

    LPCWSTR name = elem->pwcsName;

    return (elem->type == STGTY_STREAM)
	&& (elem->cbSize.u.LowPart >= sizeof(PresentationDataHeader))
	&& (lstrlenW(name) == 11)
	&& (strncmpW(name, OlePres, 8) == 0)
	&& (name[8] >= '0') && (name[8] <= '9')
	&& (name[9] >= '0') && (name[9] <= '9')
	&& (name[10] >= '0') && (name[10] <= '9');
#else
  UNIMPLEMENTED;
  return FALSE;
#endif
}

/************************************************************************
 * DataCache_OpenPresStream
 *
 * This method will find the stream for the given presentation. It makes
 * no attempt at fallback.
 *
 * Param:
 *   this       - Pointer to the DataCache object
 *   drawAspect - The aspect of the object that we wish to draw.
 *   pStm       - A returned stream. It points to the beginning of the
 *              - presentation data, including the header.
 *
 * Errors:
 *   S_OK		The requested stream has been opened.
 *   OLE_E_BLANK	The requested stream could not be found.
 *   Quite a few others I'm too lazy to map correctly.
 *
 * Notes:
 *   Algorithm:	Scan the elements of the presentation storage, looking
 *		for presentation streams. For each presentation stream,
 *		load the header and check to see if the aspect maches.
 *
 *   If a fallback is desired, just opening the first presentation stream
 *   is a possibility.
 */
static HRESULT DataCache_OpenPresStream(
  DataCache *this,
  DWORD      drawAspect,
  IStream  **ppStm)
{
    STATSTG elem;
    IEnumSTATSTG *pEnum;
    HRESULT hr;

    if (!ppStm) return E_POINTER;

    hr = IStorage_EnumElements(this->presentationStorage, 0, NULL, 0, &pEnum);
    if (FAILED(hr)) return hr;

    while ((hr = IEnumSTATSTG_Next(pEnum, 1, &elem, NULL)) == S_OK)
    {
	if (DataCache_IsPresentationStream(&elem))
	{
	    IStream *pStm;

	    hr = IStorage_OpenStream(this->presentationStorage, elem.pwcsName,
				     NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0,
				     &pStm);
	    if (SUCCEEDED(hr))
	    {
		PresentationDataHeader header;
		ULONG actual_read;

		hr = IStream_Read(pStm, &header, sizeof(header), &actual_read);

		/* can't use SUCCEEDED(hr): S_FALSE counts as an error */
		if (hr == S_OK && actual_read == sizeof(header)
		    && header.dvAspect == drawAspect)
		{
		    /* Rewind the stream before returning it. */
		    LARGE_INTEGER offset;
		    offset.u.LowPart = 0;
		    offset.u.HighPart = 0;
		    IStream_Seek(pStm, offset, STREAM_SEEK_SET, NULL);

		    *ppStm = pStm;

		    CoTaskMemFree(elem.pwcsName);
		    IEnumSTATSTG_Release(pEnum);

		    return S_OK;
		}

		IStream_Release(pStm);
	    }
	}

	CoTaskMemFree(elem.pwcsName);
    }

    IEnumSTATSTG_Release(pEnum);

    return (hr == S_FALSE ? OLE_E_BLANK : hr);
}

/************************************************************************
 * DataCache_ReadPresentationData
 *
 * This method will read information for the requested presentation 
 * into the given structure.
 *
 * Param:
 *   this       - Pointer to the DataCache object
 *   drawAspect - The aspect of the object that we wish to draw.
 *
 * Returns:
 *   This method returns a metafile handle if it is successful.
 *   it will return 0 if not.
 */
static HMETAFILE DataCache_ReadPresMetafile(
  DataCache* this,
  DWORD      drawAspect)
{
  LARGE_INTEGER offset;
  IStream*      presStream = NULL;
  HRESULT       hres;
  void*         metafileBits;
  STATSTG       streamInfo;
  HMETAFILE     newMetafile = 0;

  /*
   * Open the presentation stream.
   */
  hres = DataCache_OpenPresStream(
           this, 
	   drawAspect,
	   &presStream);

  if (FAILED(hres))
    return newMetafile;

  /*
   * Get the size of the stream.
   */
  hres = IStream_Stat(presStream,
		      &streamInfo,
		      STATFLAG_NONAME);

  /*
   * Skip the header
   */
  offset.u.HighPart = 0;
  offset.u.LowPart  = sizeof(PresentationDataHeader);

  hres = IStream_Seek(
           presStream,
	   offset,
	   STREAM_SEEK_SET,
	   NULL);

  streamInfo.cbSize.u.LowPart -= offset.u.LowPart;

  /*
   * Allocate a buffer for the metafile bits.
   */
  metafileBits = HeapAlloc(GetProcessHeap(), 
			   0, 
			   streamInfo.cbSize.u.LowPart);

  /*
   * Read the metafile bits.
   */
  hres = IStream_Read(
	   presStream,
	   metafileBits,
	   streamInfo.cbSize.u.LowPart,
	   NULL);

  /*
   * Create a metafile with those bits.
   */
  if (SUCCEEDED(hres))
  {
    newMetafile = SetMetaFileBitsEx(streamInfo.cbSize.u.LowPart, metafileBits);
  }

  /*
   * Cleanup.
   */
  HeapFree(GetProcessHeap(), 0, metafileBits);
  IStream_Release(presStream);

  if (newMetafile==0)
    hres = E_FAIL;

  return newMetafile;
}

/*********************************************************
 * Method implementation for the  non delegating IUnknown
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_NDIUnknown_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static HRESULT WINAPI DataCache_NDIUnknown_QueryInterface(
            IUnknown*      iface,
            REFIID         riid,
            void**         ppvObject)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (this==0) || (ppvObject==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) 
  {
    *ppvObject = iface;
  }
  else if (memcmp(&IID_IDataObject, riid, sizeof(IID_IDataObject)) == 0) 
  {
    *ppvObject = (IDataObject*)&(this->lpvtbl1);
  }
  else if ( (memcmp(&IID_IPersistStorage, riid, sizeof(IID_IPersistStorage)) == 0)  ||
	    (memcmp(&IID_IPersist, riid, sizeof(IID_IPersist)) == 0) )
  {
    *ppvObject = (IPersistStorage*)&(this->lpvtbl3);
  }
  else if ( (memcmp(&IID_IViewObject, riid, sizeof(IID_IViewObject)) == 0) ||
	    (memcmp(&IID_IViewObject2, riid, sizeof(IID_IViewObject2)) == 0) )
  {
    *ppvObject = (IViewObject2*)&(this->lpvtbl4);
  }
  else if ( (memcmp(&IID_IOleCache, riid, sizeof(IID_IOleCache)) == 0) ||
	    (memcmp(&IID_IOleCache2, riid, sizeof(IID_IOleCache2)) == 0) )
  {
    *ppvObject = (IOleCache2*)&(this->lpvtbl5);
  }
  else if (memcmp(&IID_IOleCacheControl, riid, sizeof(IID_IOleCacheControl)) == 0) 
  {
    *ppvObject = (IOleCacheControl*)&(this->lpvtbl6);
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    Print(MID_TRACE, ( "() : asking for unsupported interface %s\n", PRINT_GUID(riid)));
    return E_NOINTERFACE;
  }
  
  /*
   * Query Interface always increases the reference count by one when it is
   * successful. 
   */
  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;;  
}

/************************************************************************
 * DataCache_NDIUnknown_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_AddRef( 
            IUnknown*      iface)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  this->ref++;

  return this->ref;
}

/************************************************************************
 * DataCache_NDIUnknown_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 *
 * This version of QueryInterface will not delegate it's implementation
 * to the outer unknown.
 */
static ULONG WINAPI DataCache_NDIUnknown_Release( 
            IUnknown*      iface)
{
  _ICOM_THIS_From_NDIUnknown(DataCache, iface);

  /*
   * Decrease the reference count on this object.
   */
  this->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (this->ref==0)
  {
    DataCache_Destroy(this);

    return 0;
  }
  
  return this->ref;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IDataObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IDataObject_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IDataObject_AddRef( 
            IDataObject*     iface)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IDataObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IDataObject_Release( 
            IDataObject*     iface)
{
  _ICOM_THIS_From_IDataObject(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

/************************************************************************
 * DataCache_GetData
 *
 * Get Data from a source dataobject using format pformatetcIn->cfFormat
 * See Windows documentation for more details on GetData.
 * TODO: Currently only CF_METAFILEPICT is implemented
 */
static HRESULT WINAPI DataCache_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn, 
	    STGMEDIUM*       pmedium)
{
  HRESULT hr = 0;
  HRESULT hrRet = E_UNEXPECTED;
  IPersistStorage *pPersistStorage = 0;
  IStorage *pStorage = 0;
  IStream *pStream = 0;
  OLECHAR name[]={ 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};
  HGLOBAL hGlobalMF = 0;
  void *mfBits = 0;
  PresentationDataHeader pdh;
  METAFILEPICT *mfPict;
  HMETAFILE hMetaFile = 0;

  if (pformatetcIn->cfFormat == CF_METAFILEPICT)
  {
    /* Get the Persist Storage */

    hr = IDataObject_QueryInterface(iface, &IID_IPersistStorage, (void**)&pPersistStorage);

    if (hr != S_OK)
      goto cleanup;

    /* Create a doc file to copy the doc to a storage */

    hr = StgCreateDocfile(NULL, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStorage);

    if (hr != S_OK)
      goto cleanup;

    /* Save it to storage */
#if 0
    hr = OleSave(pPersistStorage, pStorage, FALSE);
#else
    Print(MIN_TRACE, ("OleSave() not found\n"));
#endif

    if (hr != S_OK)
      goto cleanup;

    /* Open the Presentation data srteam */

    hr = IStorage_OpenStream(pStorage, name, 0, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &pStream);

    if (hr != S_OK)
      goto cleanup;

    /* Read the presentation header */

    hr = IStream_Read(pStream, &pdh, sizeof(PresentationDataHeader), NULL);

    if (hr != S_OK)
      goto cleanup;

    mfBits = HeapAlloc(GetProcessHeap(), 0, pdh.dwSize);

    /* Read the Metafile bits */

    hr = IStream_Read(pStream, mfBits, pdh.dwSize, NULL);

    if (hr != S_OK)
      goto cleanup;

    /* Create the metafile and place it in the STGMEDIUM structure */

    hMetaFile = SetMetaFileBitsEx(pdh.dwSize, mfBits);

    hGlobalMF = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT));
    mfPict = (METAFILEPICT *)GlobalLock(hGlobalMF);
#if 0
    mfPict->hMF = hMetaFile;
#else
    Print(MIN_TRACE, ("Depending on MetaFile implementation\n"));
#endif

    GlobalUnlock(hGlobalMF);

    pmedium->u.hGlobal = hGlobalMF;
    pmedium->tymed = TYMED_MFPICT;
    hrRet = S_OK;

cleanup:

    if (mfBits)
      HeapFree(GetProcessHeap(), 0, mfBits);

    if (pStream)
      IStream_Release(pStream);

    if (pStorage)
      IStorage_Release(pStorage);

    if (pPersistStorage)
      IPersistStorage_Release(pPersistStorage);

    return hrRet;
  }

  /* TODO: Other formats are not implemented */

  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_GetDataHere(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_GetCanonicalFormatEtc(
	    IDataObject*     iface, 
	    LPFORMATETC      pformatectIn, 
	    LPFORMATETC      pformatetcOut)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_IDataObject_SetData (IDataObject)
 *
 * This method is delegated to the IOleCache2 implementation.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc, 
	    STGMEDIUM*       pmedium, 
	    BOOL             fRelease)
{
  IOleCache2* oleCache = NULL;
  HRESULT     hres;

  Print(MAX_TRACE, ("(%p, %p, %p, %d)\n", iface, pformatetc, pmedium, fRelease));

  hres = IDataObject_QueryInterface(iface, &IID_IOleCache2, (void**)&oleCache);

  if (FAILED(hres))
    return E_UNEXPECTED;

  hres = IOleCache2_SetData(oleCache, pformatetc, pmedium, fRelease);

  IOleCache2_Release(oleCache);

  return hres;;
}

/************************************************************************
 * DataCache_EnumFormatEtc (IDataObject)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_EnumFormatEtc(
	    IDataObject*     iface,       
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_DAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_DAdvise(
	    IDataObject*     iface, 
	    FORMATETC*       pformatetc, 
	    DWORD            advf, 
	    IAdviseSink*     pAdvSink, 
	    DWORD*           pdwConnection)
{
  UNIMPLEMENTED;
  return OLE_E_ADVISENOTSUPPORTED;
}

/************************************************************************
 * DataCache_DUnadvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection)
{
  UNIMPLEMENTED;
  return OLE_E_NOCONNECTION;
}

/************************************************************************
 * DataCache_EnumDAdvise (IDataObject)
 *
 * The data cache doesn't support connections.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI DataCache_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise)
{
  UNIMPLEMENTED;
  return OLE_E_ADVISENOTSUPPORTED;
}

/*********************************************************
 * Method implementation for the IDataObject
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IPersistStorage_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IPersistStorage_QueryInterface(
            IPersistStorage* iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IPersistStorage_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IPersistStorage_AddRef( 
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IPersistStorage_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IPersistStorage_Release( 
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

/************************************************************************
 * DataCache_GetClassID (IPersistStorage)
 *
 * The data cache doesn't implement this method.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_GetClassID( 
            IPersistStorage* iface,
	    CLSID*           pClassID)
{
  Print(MAX_TRACE, ("(%p, %p)\n", iface, pClassID));
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_IsDirty (IPersistStorage)
 *
 * Until we actully connect to a running object and retrieve new 
 * information to it, we never get dirty.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_IsDirty( 
            IPersistStorage* iface)
{
  Print(MAX_TRACE, ("(%p)\n", iface));

  return S_FALSE;
}

/************************************************************************
 * DataCache_InitNew (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_InitNew simply stores
 * the storage pointer.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_InitNew( 
            IPersistStorage* iface, 
	    IStorage*        pStg)
{
  Print(MAX_TRACE, ("(%p, %p)\n", iface, pStg));

  return DataCache_Load(iface, pStg);
}

/************************************************************************
 * DataCache_Load (IPersistStorage)
 *
 * The data cache implementation of IPersistStorage_Load doesn't 
 * actually load anything. Instead, it holds on to the storage pointer
 * and it will load the presentation information when the 
 * IDataObject_GetData or IViewObject2_Draw methods are called.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_Load( 
            IPersistStorage* iface,
	    IStorage*        pStg)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %p)\n", iface, pStg));

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
  }

  this->presentationStorage = pStg;

  if (this->presentationStorage != NULL)
  {
    IStorage_AddRef(this->presentationStorage);
  }
  return S_OK;
}

/************************************************************************
 * DataCache_Save (IPersistStorage)
 *
 * Until we actully connect to a running object and retrieve new 
 * information to it, we never have to save anything. However, it is
 * our responsability to copy the information when saving to a new
 * storage.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_Save( 
            IPersistStorage* iface,
	    IStorage*        pStg, 
	    BOOL             fSameAsLoad)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %p, %d)\n", iface, pStg, fSameAsLoad));

  if ( (!fSameAsLoad) && 
       (this->presentationStorage!=NULL) )
  {
    return IStorage_CopyTo(this->presentationStorage,
			   0,
			   NULL,
			   NULL,
			   pStg);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_SaveCompleted (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currentlu holding.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_SaveCompleted( 
            IPersistStorage* iface,  
	    IStorage*        pStgNew)
{
  Print(MAX_TRACE, ("(%p, %p)\n", iface, pStgNew));

  if (pStgNew)
  {
  /*
   * First, make sure we get our hands off any storage we have.
   */

  DataCache_HandsOffStorage(iface);

  /*
   * Then, attach to the new storage.
   */

  DataCache_Load(iface, pStgNew);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_HandsOffStorage (IPersistStorage)
 *
 * This method is called to tell the cache to release the storage
 * pointer it's currentlu holding.
 *
 * See Windows documentation for more details on IPersistStorage methods.
 */
static HRESULT WINAPI DataCache_HandsOffStorage(
            IPersistStorage* iface)
{
  _ICOM_THIS_From_IPersistStorage(DataCache, iface);

  Print(MAX_TRACE, ("(%p)\n", iface));

  if (this->presentationStorage != NULL)
  {
    IStorage_Release(this->presentationStorage);
    this->presentationStorage = NULL;
  }

  return S_OK;
}

/*********************************************************
 * Method implementation for the IViewObject2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IViewObject2_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IViewObject2_QueryInterface(
            IViewObject2* iface,
            REFIID           riid,
            void**           ppvObject)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IViewObject2_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IViewObject2_AddRef( 
            IViewObject2* iface)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IViewObject2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IViewObject2_Release( 
            IViewObject2* iface)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

/************************************************************************
 * DataCache_Draw (IViewObject2)
 *
 * This method will draw the cached representation of the object
 * to the given device context.
 *
 * See Windows documentation for more details on IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_Draw(
            IViewObject2*    iface,
	    DWORD            dwDrawAspect,
	    LONG             lindex,
	    void*            pvAspect,
	    DVTARGETDEVICE*  ptd, 
	    HDC              hdcTargetDev, 
	    HDC              hdcDraw,
	    LPCRECTL         lprcBounds,
	    LPCRECTL         lprcWBounds,
	    IVO_ContCallback pfnContinue,
	    DWORD            dwContinue)
{
  PresentationDataHeader presData;
  HMETAFILE              presMetafile = 0;
  HRESULT                hres;

  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %lx, %ld, %p, %x, %x, %p, %p, %p, %lx)\n",
	iface,
	dwDrawAspect,
	lindex,
	pvAspect,
	hdcTargetDev, 
	hdcDraw,
	lprcBounds,
	lprcWBounds,
	pfnContinue,
	dwContinue));

  /*
   * Sanity check
   */
  if (lprcBounds==NULL)
    return E_INVALIDARG;

  /*
   * First, we need to retrieve the dimensions of the
   * image in the metafile.
   */
  hres = DataCache_ReadPresentationData(this,
					dwDrawAspect,
					&presData);

  if (FAILED(hres))
    return hres;

  /*
   * Then, we can extract the metafile itself from the cached
   * data.
   *
   * FIXME Unless it isn't a metafile. I think it could be any CF_XXX type,
   * particularly CF_DIB.
   */
  presMetafile = DataCache_ReadPresMetafile(this,
					    dwDrawAspect);

  /*
   * If we have a metafile, just draw baby...
   * We have to be careful not to modify the state of the
   * DC.
   */
  if (presMetafile!=0)
  {
    INT   prevMapMode = SetMapMode(hdcDraw, MM_ANISOTROPIC);
    SIZE  oldWindowExt;
    SIZE  oldViewportExt;
    POINT oldViewportOrg;

    SetWindowExtEx(hdcDraw,
		   presData.dwObjectExtentX,
		   presData.dwObjectExtentY,
		   &oldWindowExt);

    SetViewportExtEx(hdcDraw, 
		     lprcBounds->right - lprcBounds->left,
		     lprcBounds->bottom - lprcBounds->top,
		     &oldViewportExt);

    SetViewportOrgEx(hdcDraw,
		     lprcBounds->left,
		     lprcBounds->top,
		     &oldViewportOrg);

    PlayMetaFile(hdcDraw, presMetafile);

    SetWindowExtEx(hdcDraw,
		   oldWindowExt.cx,
		   oldWindowExt.cy,
		   NULL);

    SetViewportExtEx(hdcDraw, 
		     oldViewportExt.cx,
		     oldViewportExt.cy,
		     NULL);

    SetViewportOrgEx(hdcDraw,
		     oldViewportOrg.x,
		     oldViewportOrg.y,
		     NULL);

    SetMapMode(hdcDraw, prevMapMode);

    DeleteMetaFile(presMetafile);
  }

  return S_OK;
}

static HRESULT WINAPI DataCache_GetColorSet(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    void*           pvAspect, 
	    DVTARGETDEVICE* ptd, 
	    HDC             hicTargetDevice, 
	    LOGPALETTE**    ppColorSet)
{
UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Freeze(
            IViewObject2*   iface,
	    DWORD           dwDrawAspect,
	    LONG            lindex,
	    void*           pvAspect, 
	    DWORD*          pdwFreeze)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Unfreeze(
            IViewObject2*   iface,
	    DWORD           dwFreeze)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/************************************************************************
 * DataCache_SetAdvise (IViewObject2)
 *
 * This sets-up an advisory sink with the data cache. When the object's
 * view changes, this sink is called.
 *
 * See Windows documentation for more details on IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_SetAdvise(
            IViewObject2*   iface,
	    DWORD           aspects, 
	    DWORD           advf, 
	    IAdviseSink*    pAdvSink)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %lx, %lx, %p)\n", iface, aspects, advf, pAdvSink));

  /*
   * A call to this function removes the previous sink
   */
  if (this->sinkInterface != NULL)
  {
    IAdviseSink_Release(this->sinkInterface);
    this->sinkInterface  = NULL;
    this->sinkAspects    = 0; 
    this->sinkAdviseFlag = 0;
  }

  /*
   * Now, setup the new one.
   */
  if (pAdvSink!=NULL)
  {
    this->sinkInterface  = pAdvSink;
    this->sinkAspects    = aspects; 
    this->sinkAdviseFlag = advf;    

    IAdviseSink_AddRef(this->sinkInterface);
  }

  /*
   * When the ADVF_PRIMEFIRST flag is set, we have to advise the
   * sink immediately.
   */
  if (advf & ADVF_PRIMEFIRST)
  {
    DataCache_FireOnViewChange(this,
			       DVASPECT_CONTENT,
			       -1);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_GetAdvise (IViewObject2)
 *
 * This method queries the current state of the advise sink 
 * installed on the data cache.
 *
 * See Windows documentation for more details on IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_GetAdvise(
            IViewObject2*   iface, 
	    DWORD*          pAspects, 
	    DWORD*          pAdvf, 
	    IAdviseSink**   ppAdvSink)
{
  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %p, %p, %p)\n", iface, pAspects, pAdvf, ppAdvSink));

  /*
   * Just copy all the requested values.
   */
  if (pAspects!=NULL)
    *pAspects = this->sinkAspects;

  if (pAdvf!=NULL)
    *pAdvf = this->sinkAdviseFlag;

  if (ppAdvSink!=NULL)
  {
    IAdviseSink_QueryInterface(this->sinkInterface, 
			       &IID_IAdviseSink, 
			       (void**)ppAdvSink);
  }

  return S_OK;
}

/************************************************************************
 * DataCache_GetExtent (IViewObject2)
 *
 * This method retrieves the "natural" size of this cached object.
 *
 * See Windows documentation for more details on IViewObject2 methods.
 */
static HRESULT WINAPI DataCache_GetExtent(
            IViewObject2*   iface, 
	    DWORD           dwDrawAspect, 
	    LONG            lindex, 
	    DVTARGETDEVICE* ptd, 
	    LPSIZEL         lpsizel)
{
  PresentationDataHeader presData;
  HRESULT                hres = E_FAIL;

  _ICOM_THIS_From_IViewObject2(DataCache, iface);

  Print(MAX_TRACE, ("(%p, %lx, %ld, %p, %p)\n", 
	iface, dwDrawAspect, lindex, ptd, lpsizel));

  /*
   * Sanity check
   */
  if (lpsizel==NULL)
    return E_POINTER;

  /*
   * Initialize the out parameter.
   */
  lpsizel->cx = 0;
  lpsizel->cy = 0;

  /*
   * This flag should be set to -1.
   */
  if (lindex!=-1)
    Print(MIN_TRACE, ("Unimplemented flag lindex = %ld\n", lindex));

  /*
   * Right now, we suport only the callback from
   * the default handler.
   */
  if (ptd!=NULL)
    Print(MIN_TRACE, ("Unimplemented ptd = %p\n", ptd));
  
  /*
   * Get the presentation information from the 
   * cache.
   */
  hres = DataCache_ReadPresentationData(this,
					dwDrawAspect,
					&presData);

  if (SUCCEEDED(hres))
  {
    lpsizel->cx = presData.dwObjectExtentX;
    lpsizel->cy = presData.dwObjectExtentY;
  }

  /*
   * This method returns OLE_E_BLANK when it fails.
   */
  if (FAILED(hres))
    hres = OLE_E_BLANK;

  return hres;
}


/*********************************************************
 * Method implementation for the IOleCache2
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCache2_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IOleCache2_QueryInterface(
            IOleCache2*     iface,
            REFIID          riid,
            void**          ppvObject)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IOleCache2_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_AddRef( 
            IOleCache2*     iface)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IOleCache2_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCache2_Release( 
            IOleCache2*     iface)
{
  _ICOM_THIS_From_IOleCache2(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_Cache(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    DWORD           advf,
	    DWORD*          pdwConnection)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_Uncache(
	    IOleCache2*     iface,
	    DWORD           dwConnection)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_EnumCache(
            IOleCache2*     iface,
	    IEnumSTATDATA** ppenumSTATDATA)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_InitCache(
	    IOleCache2*     iface,
	    IDataObject*    pDataObject)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_IOleCache2_SetData(
            IOleCache2*     iface,
	    FORMATETC*      pformatetc,
	    STGMEDIUM*      pmedium,
	    BOOL            fRelease)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_UpdateCache(
            IOleCache2*     iface,
	    LPDATAOBJECT    pDataObject, 
	    DWORD           grfUpdf,
	    LPVOID          pReserved)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_DiscardCache(
            IOleCache2*     iface,
	    DWORD           dwDiscardOptions)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}


/*********************************************************
 * Method implementation for the IOleCacheControl
 * part of the DataCache class.
 */

/************************************************************************
 * DataCache_IOleCacheControl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI DataCache_IOleCacheControl_QueryInterface(
            IOleCacheControl* iface,
            REFIID            riid,
            void**            ppvObject)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_QueryInterface(this->outerUnknown, riid, ppvObject);  
}

/************************************************************************
 * DataCache_IOleCacheControl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_AddRef( 
            IOleCacheControl* iface)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_AddRef(this->outerUnknown);  
}

/************************************************************************
 * DataCache_IOleCacheControl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI DataCache_IOleCacheControl_Release( 
            IOleCacheControl* iface)
{
  _ICOM_THIS_From_IOleCacheControl(DataCache, iface);

  return IUnknown_Release(this->outerUnknown);  
}

static HRESULT WINAPI DataCache_OnRun(
	    IOleCacheControl* iface,
	    LPDATAOBJECT      pDataObject)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

static HRESULT WINAPI DataCache_OnStop(
	    IOleCacheControl* iface)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}


