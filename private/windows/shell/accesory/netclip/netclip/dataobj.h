// GenericDataObject.h : header file
//

// Note: the following interface is not an actual OLE interface, but is useful
//  for describing an abstract (not typesafe) enumerator.

// Stolen from MFC 4.0 (OLEIMPL2.H)
#undef  INTERFACE
#define INTERFACE   IEnumVOID

DECLARE_INTERFACE_(IEnumVOID, IUnknown)
{
	STDMETHOD(QueryInterface)(REFIID, LPVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)()  PURE;
	STDMETHOD_(ULONG,Release)() PURE;
	STDMETHOD(Next)(ULONG, void*, ULONG*) PURE;
	STDMETHOD(Skip)(ULONG) PURE;
	STDMETHOD(Reset)() PURE;
	STDMETHOD(Clone)(IEnumVOID**) PURE;
};

// Stolen from MFC 4.0 (OLEIMPL2.H)
class CEnumArray : public CCmdTarget
{
// Constructors
public:
	CEnumArray(size_t nSize,
		const void* pvEnum, UINT nCount, BOOL bNeedFree = FALSE);

// Implementation
public:
	virtual ~CEnumArray();

protected:
	size_t m_nSizeElem;     // size of each item in the array
	CCmdTarget* m_pClonedFrom;  // used to keep original alive for clones

	BYTE* m_pvEnum;     // pointer data to enumerate
	UINT m_nCurPos;     // current position in m_pvEnum
	UINT m_nSize;       // total number of items in m_pvEnum
	BOOL m_bNeedFree;   // free on release?

	virtual BOOL OnNext(void* pv);
	virtual BOOL OnSkip();
	virtual void OnReset();
	virtual CEnumArray* OnClone();

// Interface Maps
public:
	BEGIN_INTERFACE_PART(EnumVOID, IEnumVOID)
		INIT_INTERFACE_PART(CEnumArray, EnumVOID)
		STDMETHOD(Next)(ULONG, void*, ULONG*);
		STDMETHOD(Skip)(ULONG);
		STDMETHOD(Reset)();
		STDMETHOD(Clone)(IEnumVOID**);
	END_INTERFACE_PART(EnumVOID)
};


/////////////////////////////////////////////////////////////////////////////
// CGenericDataObject command target

struct _DATACACHE_ENTRY;
class CGenericDataObject : public CCmdTarget
{
public:
    CGenericDataObject(IDataObject* pdo=NULL);

    // Operations
	void Empty();   // empty cache (similar to ::EmptyClipboard)

// Overidables
	virtual BOOL OnRenderData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);

// Implementation
public:
	virtual ~CGenericDataObject();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	_DATACACHE_ENTRY* m_pDataCache;  // data cache itself
	UINT m_nMaxSize;    // current allocated size
	UINT m_nSize;       // current size of the cache
	UINT m_nGrowBy;     // number of cache elements to grow by for new allocs
    BOOL m_bModified;   // Data was modified since IPersistStorage::Load, Save, or InitNew
    BOOL m_bInitialized;  // IPersistStorage::InitNew was called

	_DATACACHE_ENTRY* Lookup(
		LPFORMATETC lpFormatEtc) const;
	_DATACACHE_ENTRY* GetCacheEntry(
		LPFORMATETC lpFormatEtc);

        // Interface Maps
public:
	BEGIN_INTERFACE_PART(DataObject, IDataObject)
		INIT_INTERFACE_PART(CGenericDataObject, DataObject)
		STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
		STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
		STDMETHOD(QueryGetData)(LPFORMATETC);
		STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
		STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
		STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC*);
		STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD);
		STDMETHOD(DUnadvise)(DWORD);
		STDMETHOD(EnumDAdvise)(LPENUMSTATDATA*);
	END_INTERFACE_PART(DataObject)

  	BEGIN_INTERFACE_PART(PersistStorage, IPersistStorage)
		INIT_INTERFACE_PART(CGenericDataObject, PersistStorage)
		STDMETHOD(GetClassID)(LPCLSID);
		STDMETHOD(IsDirty)();
		STDMETHOD(InitNew)(LPSTORAGE);
		STDMETHOD(Load)(LPSTORAGE);
		STDMETHOD(Save)(LPSTORAGE, BOOL);
		STDMETHOD(SaveCompleted)(LPSTORAGE);
		STDMETHOD(HandsOffStorage)();
	END_INTERFACE_PART(PersistStorage)

	DECLARE_INTERFACE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
