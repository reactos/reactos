/*
 * SITE.H
 * View Site for Document Objects.
 *
 * Copyright (c)1995-1999 Microsoft Corporation, All Rights Reserved
 */


#ifndef _SITE_H_
#define _SITE_H_

#include "stdafx.h"
#include "mlang.h"


class CProxyFrame;
class CTriEditEventSink;

class CImpIOleClientSite : public IOleClientSite
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleClientSite(class CSite *, IUnknown *);
        ~CImpIOleClientSite(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP SaveObject(void);
        STDMETHODIMP GetMoniker(DWORD, DWORD, LPMONIKER *);
        STDMETHODIMP GetContainer(LPOLECONTAINER *);
        STDMETHODIMP ShowObject(void);
        STDMETHODIMP OnShowWindow(BOOL);
        STDMETHODIMP RequestNewObjectLayout(void);
};

typedef CImpIOleClientSite *PCImpIOleClientSite;



class CImpIAdviseSink : public IAdviseSink
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIAdviseSink(class CSite *, IUnknown *);
        ~CImpIAdviseSink(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP_(void)  OnDataChange(LPFORMATETC, LPSTGMEDIUM);
        STDMETHODIMP_(void)  OnViewChange(DWORD, LONG);
        STDMETHODIMP_(void)  OnRename(LPMONIKER);
        STDMETHODIMP_(void)  OnSave(void);
        STDMETHODIMP_(void)  OnClose(void);
};


typedef CImpIAdviseSink *PCImpIAdviseSink;


class CImplPropertyNotifySink : public IPropertyNotifySink
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImplPropertyNotifySink(class CSite *, IUnknown *);
        ~CImplPropertyNotifySink(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP OnChanged(DISPID dispid);
        STDMETHODIMP OnRequestEdit (DISPID dispid);
};


typedef CImplPropertyNotifySink *PCImplPropertyNotifySink;


class CImpIOleInPlaceSite : public IOleInPlaceSite
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleInPlaceSite(class CSite *, IUnknown *);
        ~CImpIOleInPlaceSite(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetWindow(HWND *);
        STDMETHODIMP ContextSensitiveHelp(BOOL);
        STDMETHODIMP CanInPlaceActivate(void);
        STDMETHODIMP OnInPlaceActivate(void);
        STDMETHODIMP OnUIActivate(void);
        STDMETHODIMP GetWindowContext(LPOLEINPLACEFRAME *
                        , LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT
                        , LPOLEINPLACEFRAMEINFO);
        STDMETHODIMP Scroll(SIZE);
        STDMETHODIMP OnUIDeactivate(BOOL);
        STDMETHODIMP OnInPlaceDeactivate(void);
        STDMETHODIMP DiscardUndoState(void);
        STDMETHODIMP DeactivateAndUndo(void);
        STDMETHODIMP OnPosRectChange(LPCRECT);
};

typedef CImpIOleInPlaceSite *PCImpIOleInPlaceSite;


class CImpIOleDocumentSite : public IOleDocumentSite
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleDocumentSite(class CSite *, IUnknown *);
        ~CImpIOleDocumentSite(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP ActivateMe(IOleDocumentView *);
};

typedef CImpIOleDocumentSite *PCImpIOleDocumentSite;



///////////////////////////////////////////////////
// MSHTML.DLL host integration interfaces
///////////////////////////////////////////////////
class CImpIDocHostUIHandler : public IDocHostUIHandler
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIDocHostUIHandler(class CSite *, IUnknown *);
        ~CImpIDocHostUIHandler(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		STDMETHODIMP GetHostInfo(DOCHOSTUIINFO * pInfo);
		STDMETHODIMP ShowUI(
				DWORD dwID, 
				IOleInPlaceActiveObject * pActiveObject,
				IOleCommandTarget * pCommandTarget,
				IOleInPlaceFrame * pFrame,
				IOleInPlaceUIWindow * pDoc);
		STDMETHODIMP HideUI(void);
		STDMETHODIMP UpdateUI(void);
		STDMETHODIMP EnableModeless(BOOL fEnable);
		STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
		STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
		STDMETHODIMP ResizeBorder(
				LPCRECT prcBorder, 
				IOleInPlaceUIWindow * pUIWindow, 
				BOOL fRameWindow);
		STDMETHODIMP ShowContextMenu(
				DWORD dwID, 
				POINT * pptPosition,
				IUnknown* pCommandTarget,
				IDispatch * pDispatchObjectHit);
		STDMETHODIMP TranslateAccelerator(
            /* [in] */ LPMSG lpMsg,
            /* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID);
		STDMETHODIMP GetOptionKeyPath(BSTR* pbstrKey, DWORD dw);
		STDMETHODIMP GetDropTarget( 
            /* [in] */ IDropTarget __RPC_FAR *pDropTarget,
            /* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget);

		STDMETHODIMP GetExternal( 
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);
        
        STDMETHODIMP TranslateUrl( 
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR __RPC_FAR *pchURLIn,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut);
        
        STDMETHODIMP FilterDataObject( 
            /* [in] */ IDataObject __RPC_FAR *pDO,
            /* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDORet);


};

typedef CImpIDocHostUIHandler* PCImpIDocHostUIHandler;



class CImpIDocHostShowUI : public IDocHostShowUI
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIDocHostShowUI(class CSite *, IUnknown *);
        ~CImpIDocHostShowUI(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		STDMETHODIMP ShowMessage(
								HWND hwnd,
								LPOLESTR lpstrText,
								LPOLESTR lpstrCaption, 
								DWORD dwType,
								LPOLESTR lpstrHelpFile,
								DWORD dwHelpContext,
								LRESULT * plResult);
		STDMETHODIMP ShowHelp(
								HWND hwnd,
								LPOLESTR pszHelpFile,
								UINT uCommand,
								DWORD dwData,
								POINT ptMouse,
								IDispatch * pDispatchObjectHit);
};

typedef CImpIDocHostShowUI* PCImpIDocHostShowUI;



/*
 * IDispatch - implements Ambient properties
 */
class CImpAmbientIDispatch : public IDispatch
{
    protected:
        ULONG           m_cRef;
        class CSite		*m_pSite;
        LPUNKNOWN       m_pUnkOuter;

    public:
        CImpAmbientIDispatch(class CSite *, IUnknown *);
        ~CImpAmbientIDispatch(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP GetTypeInfoCount(UINT *);
        STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **);
        STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT
            , LCID, DISPID *);
        STDMETHODIMP Invoke(DISPID, REFIID, LCID, USHORT
            , DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);
};

typedef class CImpAmbientIDispatch *PCImpAmbientIDispatch;


class CImpIOleControlSite : public IOleControlSite
{
    protected:
        ULONG               m_cRef;
        class CSite        *m_pSite;
        LPUNKNOWN           m_pUnkOuter;

    public:
        CImpIOleControlSite(class CSite *, IUnknown *);
        ~CImpIOleControlSite(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP OnControlInfoChanged(void) { return S_OK;}
        STDMETHODIMP LockInPlaceActive(BOOL) {return E_NOTIMPL;}
        STDMETHODIMP GetExtendedControl(IDispatch**) {return E_NOTIMPL;}
        STDMETHODIMP TransformCoords(POINTL*, POINTF*, DWORD) {return E_NOTIMPL;}
        STDMETHODIMP TranslateAccelerator(LPMSG, DWORD) {return E_NOTIMPL;}
        STDMETHODIMP OnFocus(BOOL) { return S_OK;}
        STDMETHODIMP ShowPropertyFrame(void) { return E_NOTIMPL; }
};

typedef class CImpIOleControlSite *PCImpIOleControlSite;



/*
 * The CSite class, a COM object with the interfaces IOleClientSite,
 * IAdviseSink, IOleInPlaceSite, and IOleDocumentSite.
 */


class CSite : public IUnknown
{
	
    private:
        ULONG						m_cRef;
        HWND						m_hWnd; //Client area window of parent
		DWORD						m_dwPropNotifyCookie;
		DWORD						m_dwOleObjectCookie;
        class CProxyFrame*			m_pFR;
		BOOL						m_bFiltered;

        //Object interfaces
        LPUNKNOWN					m_pObj;
        LPOLEOBJECT					m_pIOleObject;
        LPOLEINPLACEOBJECT			m_pIOleIPObject;
        LPOLEDOCUMENTVIEW			m_pIOleDocView;
		LPOLECOMMANDTARGET			m_pIOleCommandTarget;


        //Our interfaces
        PCImpIOleClientSite			m_pImpIOleClientSite;
        PCImpIAdviseSink			m_pImpIAdviseSink;
        PCImpIOleInPlaceSite		m_pImpIOleIPSite;
        PCImpIOleDocumentSite		m_pImpIOleDocumentSite;
        PCImpIDocHostUIHandler		m_pImpIDocHostUIHandler;
        PCImpIDocHostShowUI			m_pImpIDocHostShowUI;
		PCImpAmbientIDispatch		m_pImpAmbientIDispatch;
		PCImplPropertyNotifySink	m_pImpIPropertyNotifySink;
		PCImpIOleControlSite		m_pImpIOleControlSite;

		CTriEditEventSink*			m_pTriEdDocEvtSink;		
		CTriEditEventSink*			m_pTriEdWndEvtSink;		
		BOOL						m_bfSaveAsUnicode;
		UINT						m_cpCodePage;
		IMultiLanguage2*			m_piMLang;

    protected:

    public:
        CSite(CProxyFrame*);
        ~CSite(void);

        //BOOL    ObjectInitialize( TCHAR* pchPath );

        //Gotta have an IUnknown for delegation
        STDMETHODIMP QueryInterface(REFIID, void** );
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);


		// Other functions
        HRESULT		HrCreate(IUnknown* pUnk, IUnknown** ppUnkTriEdit);
		HRESULT		HrObjectInitialize();
		HRESULT		HrRegisterPropNotifySink(BOOL fRegister);
        void        Close(BOOL);
		void		InitialActivate(LONG, HWND hWnd);
		void		Activate(LONG);
        void        UpdateObjectRects(void);

		HRESULT		HrIsDirtyIPersistStreamInit(BOOL& bVal);

		HRESULT		HrSaveToFile(BSTR bstrPath, DWORD dwFilterFlags);
		HRESULT		HrSaveToBstr(BSTR* bstr, DWORD dwFilterFlags);

		HRESULT		HrSaveToStream(LPSTREAM pStream);
		HRESULT		HrSaveToStreamAndFilter(LPSTREAM* ppStream, DWORD dwFilterFlags);

		HRESULT		HrTestFileOpen(BSTR path);

		// Filtering methods

		HRESULT		HrFileToStream(LPCTSTR fileName, LPSTREAM* ppiStream);
		HRESULT		HrURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream);
		HRESULT		HrSecureURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream);
		HRESULT		HrNonSecureURLToStream(LPCTSTR szURL, LPSTREAM* ppiStream);
		HRESULT		HrStreamToFile(LPSTREAM pStream, LPCTSTR fileName);
		HRESULT		HrBstrToStream(BSTR bstrSrc, LPSTREAM* ppStream);
		HRESULT		HrStreamToBstr(LPSTREAM pStream, BSTR* pBstr, BOOL bfRetainByteOrderMark = FALSE);
		HRESULT		HrFilter(BOOL bDirection, LPSTREAM pSrcStream, LPSTREAM* ppFilteredStream, DWORD dwFilterFlags);

		HRESULT		HrConvertStreamToUnicode ( IStream* piStream );
		BOOL		BfFlipBytesIfBigEndianUnicode ( CHAR* pchData, int cbSize );
		BOOL		IsUnicode ( void* pData, int cbSize );

		// ReadyState property methods
		void OnReadyStateChanged();

		// helper functions

		HRESULT GetContainer ( LPOLECONTAINER* ppContainer );

		inline CProxyFrame*	GetFrame(void) {
							return m_pFR;
						}

		inline LPUNKNOWN		GetObjectUnknown(void ) {
							return m_pObj; 
						} 

		inline PCImpIOleInPlaceSite GetIPSite(void) {
							return m_pImpIOleIPSite;
						}

		inline HWND			GetWindow(void) {
							return m_hWnd;
						}

		inline void			SetWindow(HWND hwnd) {
							m_hWnd = hwnd;
						}

		inline LPOLECOMMANDTARGET GetCommandTarget(void) {
							return m_pIOleCommandTarget;
						}

		inline void			SetCommandTarget(LPOLECOMMANDTARGET pTarget) {
							m_pIOleCommandTarget = pTarget;
						}

		inline void			SetDocView(LPOLEDOCUMENTVIEW pDocView) {
							m_pIOleDocView = pDocView;
						}
		
		inline void			SetIPObject(LPOLEINPLACEOBJECT pIPObject) {
							m_pIOleIPObject = pIPObject;
						}

		inline LPOLEINPLACEOBJECT GetIPObject(void) {
							return m_pIOleIPObject; 
						}

		inline BOOL GetSaveAsUnicode ( void ) {
							return m_bfSaveAsUnicode;
						}

		inline BOOL SetSaveAsUnicode ( BOOL bfUnicode ) {
							BOOL bf = m_bfSaveAsUnicode;
							m_bfSaveAsUnicode = bfUnicode;
							return bf;
						}

		inline UINT GetCurrentCodePage ( void ) {
							return m_cpCodePage;
						}
};


typedef CSite* PCSite;



//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p)\
{\
            if (NULL!=p)\
            {\
                delete p;\
                p=NULL;\
            }\
}


//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p)\
{\
            IUnknown *pt=(IUnknown *)p;\
            p=NULL;\
            if (NULL!=pt)\
                pt->Release();\
}

#endif //_SITE_H_
