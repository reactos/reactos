// IDataObjectImpl.h: interface for the CIDataObjectImpl class.
/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.
   Author: Leon Finker	11/2000
   Modifications: replaced ATL by STL, Martin Fuchs 7/2003
**************************************************************************/

#include <vector>
using std::vector;


typedef vector<FORMATETC> FormatArray;

 /// structure containing information for one format of EnumFormatEtcImpl
struct DataStorage {
	FORMATETC*	_format;
	STGMEDIUM*	_medium;
};

typedef vector<DataStorage> StorageArray;


 /// implementation of IEnumFORMATETC interface
class EnumFormatEtcImpl
 :	public IComSrvBase<IEnumFORMATETC, EnumFormatEtcImpl>, public SimpleComObject
{
	typedef IComSrvBase<IEnumFORMATETC, EnumFormatEtcImpl> super;

private:
	 ULONG			m_cRefCount;
	 FormatArray	m_pFmtEtc;
	 size_t 		m_iCur;

public:
	 EnumFormatEtcImpl(const FormatArray& ArrFE);
	 EnumFormatEtcImpl(const StorageArray& ArrFE);
	 virtual ~EnumFormatEtcImpl() {}

	 //IEnumFORMATETC members
	 virtual HRESULT STDMETHODCALLTYPE Next(ULONG, LPFORMATETC, ULONG*);
	 virtual HRESULT STDMETHODCALLTYPE Skip(ULONG);
	 virtual HRESULT STDMETHODCALLTYPE Reset(void);
	 virtual HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC**);
};

 /// implementation of IDropSource interface
class IDropSourceImpl
 :	public IComSrvBase<IDropSource, IDropSourceImpl>, public SimpleComObject
{
	typedef IComSrvBase<IDropSource, IDropSourceImpl> super;

	long m_cRefCount;

public:
	bool m_bDropped;

	IDropSourceImpl()
	 :	super(IID_IDropSource),
		m_cRefCount(0),
		m_bDropped(false)
	{
	}

	virtual ~IDropSourceImpl() {}

	//IDropSource
	virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(
		/* [in] */ BOOL fEscapePressed,
		/* [in] */ DWORD grfKeyState);

	virtual HRESULT STDMETHODCALLTYPE GiveFeedback(
		/* [in] */ DWORD dwEffect);
};

 /// implementation of IDataObject interface
class IDataObjectImpl
 :	public IComSrvBase<IDataObject, IDataObjectImpl>, public SimpleComObject
	//public IAsyncOperation
{
	typedef IComSrvBase<IDataObject, IDataObjectImpl> super;

	IDropSourceImpl* m_pDropSource;
	long m_cRefCount;

	StorageArray	_storage;

public:
	IDataObjectImpl(IDropSourceImpl* pDropSource);
	virtual ~IDataObjectImpl();

	void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);

	//IDataObject
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
		/* [out] */ STGMEDIUM __RPC_FAR *pmedium);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDataHere(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);

	virtual HRESULT STDMETHODCALLTYPE QueryGetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc);

	virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
		/* [out] */ FORMATETC __RPC_FAR *pformatetcOut);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetData(
		/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
		/* [in] */ BOOL fRelease);

	virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(
		/* [in] */ DWORD dwDirection,
		/* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc);

	virtual HRESULT STDMETHODCALLTYPE DAdvise(
		/* [in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [in] */ DWORD advf,
		/* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
		/* [out] */ DWORD __RPC_FAR *pdwConnection);

	virtual HRESULT STDMETHODCALLTYPE DUnadvise(
		/* [in] */ DWORD dwConnection);

	virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(
		/* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise);

	//IAsyncOperation
	//virtual HRESULT STDMETHODCALLTYPE SetAsyncMode(
	//	  /* [in] */ BOOL fDoOpAsync)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE GetAsyncMode(
	//	  /* [out] */ BOOL __RPC_FAR *pfIsOpAsync)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE StartOperation(
	//	  /* [optional][unique][in] */ IBindCtx __RPC_FAR *pbcReserved)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE InOperation(
	//	  /* [out] */ BOOL __RPC_FAR *pfInAsyncOp)
	//{
	//	return E_NOTIMPL;
	//}
	//
	//virtual HRESULT STDMETHODCALLTYPE EndOperation(
	//	  /* [in] */ HRESULT hResult,
	//	  /* [unique][in] */ IBindCtx __RPC_FAR *pbcReserved,
	//	  /* [in] */ DWORD dwEffects)
	//{
	//	return E_NOTIMPL;
	//}*/
};

 /// implementation of IDropTarget interface
class IDropTargetImpl : public IDropTarget
{
	DWORD m_cRefCount;
	bool m_bAllowDrop;
	IDropTargetHelper* m_pDropTargetHelper;

	FormatArray m_formatetc;
	FORMATETC* m_pSupportedFrmt;

protected:
	HWND m_hTargetWnd;

public:
	IDropTargetImpl(HWND m_hTargetWnd);
	virtual ~IDropTargetImpl();

	void AddSuportedFormat(FORMATETC& ftetc) {m_formatetc.push_back(ftetc);}

	//return values: true - release the medium. false - don't release the medium
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect) = 0;

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef() {return ++m_cRefCount;}
	virtual ULONG STDMETHODCALLTYPE Release();

	bool QueryDrop(DWORD grfKeyState, LPDWORD pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave();
	virtual HRESULT STDMETHODCALLTYPE Drop(
		/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect);
};

 /// implementation of IDragSourceHelper interface
class DragSourceHelper
{
	IDragSourceHelper* pDragSourceHelper;

public:
	DragSourceHelper()
	{
		if (FAILED(CoCreateInstance(CLSID_DragDropHelper,
						NULL,
						CLSCTX_INPROC_SERVER,
						IID_IDragSourceHelper,
						(void**)&pDragSourceHelper)))
			pDragSourceHelper = NULL;
	}

	virtual ~DragSourceHelper()
	{
		if (pDragSourceHelper != NULL)
		{
			pDragSourceHelper->Release();
			pDragSourceHelper=NULL;
		}
	}

	// IDragSourceHelper
	HRESULT InitializeFromBitmap(HBITMAP hBitmap,
		POINT& pt,	// cursor position in client coords of the window
		RECT& rc,	// selected item's bounding rect
		IDataObject* pDataObject,
		COLORREF crColorKey=GetSysColor(COLOR_WINDOW)// color of the window used for transparent effect.
		)
	{
		if (pDragSourceHelper == NULL)
			return E_FAIL;

			SHDRAGIMAGE di;
			BITMAP		bm;
			GetObject(hBitmap, sizeof(bm), &bm);
			di.sizeDragImage.cx = bm.bmWidth;
			di.sizeDragImage.cy = bm.bmHeight;
			di.hbmpDragImage = hBitmap;
			di.crColorKey = crColorKey;
			di.ptOffset.x = pt.x - rc.left;
			di.ptOffset.y = pt.y - rc.top;
		return pDragSourceHelper->InitializeFromBitmap(&di, pDataObject);
	}

	HRESULT InitializeFromWindow(HWND hwnd, POINT& pt,IDataObject* pDataObject)
	{
		if (pDragSourceHelper == NULL)
			return E_FAIL;
		return pDragSourceHelper->InitializeFromWindow(hwnd, &pt, pDataObject);
	}
};
