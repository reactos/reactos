#ifndef _HTML_H
#define _HTML_H

interface IHTMLDocument2;

//
// a class host for trident so that we can control what it downloads 
// and what it doesn't...
//
class CTridentHost : public IOleClientSite,
                     public IDispatch,
                     public IDocHostUIHandler
{
    public:
        CTridentHost();
        ~CTridentHost();

        HRESULT SetTrident( IOleObject * pTrident );

        // IUnknown
        STDMETHOD ( QueryInterface )( REFIID riid, LPVOID * ppvObj );
        STDMETHOD_( ULONG, AddRef ) ( void );
        STDMETHOD_( ULONG, Release ) ( void );
        
        // IDispatch (ambient properties)
        STDMETHOD( GetTypeInfoCount ) (UINT *pctinfo);
        STDMETHOD( GetTypeInfo )(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
        STDMETHOD( GetIDsOfNames )(REFIID riid, OLECHAR **rgszNames, UINT cNames,
                                   LCID lcid, DISPID *rgdispid);
        STDMETHOD( Invoke )(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                            DISPPARAMS *pdispparams, VARIANT *pvarResult,
                            EXCEPINFO *pexcepinfo, UINT *puArgErr);

        // IOleClientSite
        STDMETHOD( SaveObject )(void);
        STDMETHOD( GetMoniker )(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
        STDMETHOD( GetContainer )(IOleContainer **ppContainer);
        STDMETHOD( ShowObject )(void);
        STDMETHOD( OnShowWindow )(BOOL fShow);
        STDMETHOD( RequestNewObjectLayout )(void);

        // IDocHostUIHandler
        STDMETHOD( ShowContextMenu )( DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
        STDMETHOD( GetHostInfo )( DOCHOSTUIINFO *pInfo);
        STDMETHOD( ShowUI )( DWORD dwID, IOleInPlaceActiveObject *pActiveObject,IOleCommandTarget *pCommandTarget,
            IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
        STDMETHOD( HideUI )( void);
        STDMETHOD( UpdateUI )( void);
        STDMETHOD( EnableModeless )( BOOL fEnable);
        STDMETHOD( OnDocWindowActivate )( BOOL fActivate);
        STDMETHOD( OnFrameWindowActivate )( BOOL fActivate);
        STDMETHOD( ResizeBorder )( LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
        STDMETHOD( TranslateAccelerator )( LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
        STDMETHOD( GetOptionKeyPath )( LPOLESTR *pchKey, DWORD dw);
        STDMETHOD( GetDropTarget )( IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
        STDMETHOD( GetExternal )( IDispatch **ppDispatch);
        STDMETHOD( TranslateUrl )( DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
        STDMETHOD( FilterDataObject )( IDataObject *pDO, IDataObject **ppDORet);

    public:
        BITBOOL m_fOffline : 1;
        
   protected:
        long m_cRef;
};

class CHtmlThumb : public IExtractImage,
                   public IRunnableTask,
                   public IPropertyNotifySink,
                   public IPersistFile,
                   public IPersistMoniker,
                   public CComObjectRoot,
                   public CComCoClass< CHtmlThumb,&CLSID_HtmlThumbnailExtractor >
{
    public:
        CHtmlThumb();
        ~CHtmlThumb();

        BEGIN_COM_MAP( CHtmlThumb )
            COM_INTERFACE_ENTRY( IExtractImage)
            COM_INTERFACE_ENTRY( IRunnableTask )
            COM_INTERFACE_ENTRY( IPropertyNotifySink )
            COM_INTERFACE_ENTRY( IPersistFile )
            COM_INTERFACE_ENTRY( IPersistMoniker )
        END_COM_MAP( )

        DECLARE_REGISTRY( CHtmlThumb,
                          _T("Shell.ThumbnailExtract.HTML.1"),
                          _T("Shell.ThumbnailExtract.HTML.1"),
                          IDS_HTMLTHUMBEXTRACT_DESC,
                          THREADFLAGS_APARTMENT);

        DECLARE_NOT_AGGREGATABLE( CHtmlThumb );

        // IExtractImage
        STDMETHOD (GetLocation) ( LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );
     
        STDMETHOD (Extract)( HBITMAP * phBmpThumbnail );

        // IRunnableTask 
        STDMETHOD (Run)( void ) ;
        STDMETHOD (Kill)( BOOL fWait );
        STDMETHOD (Suspend)( );
        STDMETHOD (Resume)( );
        STDMETHOD_( ULONG, IsRunning )( void );

        // IPropertyNotifySink
        STDMETHOD (OnChanged)( DISPID dispID);
        STDMETHOD (OnRequestEdit) ( DISPID dispID);

        // IPersistFile
        STDMETHOD (GetClassID )(CLSID *pClassID);
        STDMETHOD (IsDirty )();
        STDMETHOD (Load )( LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD (Save )( LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD (SaveCompleted )( LPCOLESTR pszFileName);
        STDMETHOD (GetCurFile )( LPOLESTR *ppszFileName);

        // IPersistMoniker
        STDMETHOD( Load )( BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode);
        STDMETHOD( Save )( IMoniker *pimkName, LPBC pbc, BOOL fRemember);
        STDMETHOD( SaveCompleted )( IMoniker *pimkName, LPBC pibc);
        STDMETHOD( GetCurMoniker )( IMoniker **ppimkName);

    protected:
        HRESULT InternalResume( void );
        HRESULT Create_URL_Moniker( LPMONIKER * ppMoniker );
        HRESULT WaitForRender( void );
        HRESULT Finish( HBITMAP * pBmp, const SIZE * prgSize, DWORD dwClrDepth );
        HRESULT CheckReadyState( );
        void ReportError( LPVOID * pMsgArgs );
        
        LONG m_lState;
        BITBOOL m_fAsync : 1;
        BITBOOL m_fAspect : 1;
        HANDLE m_hDone;
        CTridentHost m_Host;
        IHTMLDocument2 * m_pHTML;
        IOleObject * m_pOleObject;
        IConnectionPoint * m_pConPt;
        IViewObject * m_pViewObject;
        DWORD m_dwTimeout;
        DWORD m_dwCurrentTick;
        DWORD m_dwPropNotifyCookie;
        WCHAR m_szPath[MAX_PATH];
        SIZE m_rgSize;
        HBITMAP * m_phBmp;
        DWORD m_dwClrDepth;
        UINT m_idErrorIcon;
        UINT m_idError;
        DECLAREWAITCURSOR;
        DWORD m_dwXRenderSize;
        DWORD m_dwYRenderSize;
        IMoniker * m_pMoniker;
};

// time we wait before asking the internet explorer if it is done yet ...
#define TIME_PAUSE 200

// default timeout (seconds)
#define TIME_DEFAULT 90

HRESULT RegisterHTMLExtractor( void );
HRESULT UnregisterHTMLExtractor( void );

UINT FormatMessageBox( HWND hwnd, UINT idMsg, UINT idTitle, 
                       LPVOID * pArgsMsg, LPVOID * pArgsTitle,
                       UINT idIconResource, UINT uFlags );

#endif

