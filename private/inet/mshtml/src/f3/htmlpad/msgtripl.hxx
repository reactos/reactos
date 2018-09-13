//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msgtripl.hxx
//
//  Contents:   CTriple, CTripleData, CTripCall
//
//-------------------------------------------------------------------------

#include "richole.h"

BOOL FInitTripole(VOID);

class CPadMessage;


#ifndef PFNGETCONTEXTMENU
typedef HRESULT (STDAPICALLTYPE * PFNGETCONTEXTMENU)
					(DWORD dwGetContextMenuCookie, WORD seltype,
						LPOLEOBJECT lpoleobj, CHARRANGE FAR * lpchrg,
						HMENU FAR * lphmenu);
#endif	// PFNGETCONTEXTMENU


class CTriple : public IOleObject,
                public IPersist,
                public IViewObject
{
public:
    //Constructor
    CTriple(CPadMessage * pPad, LPSRow prw);

    VOID FindTripleProperties();

    // IUnknown

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    // IPersist
    STDMETHOD(GetClassID)(LPCLSID pClassID);

    // IOleClientSite			
    STDMETHOD(SetClientSite)(IOleClientSite * pClientSite);

    //IOleObject
    STDMETHOD(GetClientSite)(IOleClientSite ** ppClientSite);
    STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    STDMETHOD(Close)(DWORD dwSaveOption);
    STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk);
    STDMETHOD(InitFromData)(LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved);
    STDMETHOD(GetClipboardData)(DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject);
    STDMETHOD(DoVerb)(LONG iVerb,LPMSG lpmsg, IOleClientSite * pActivateSite,
				LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHOD(EnumVerbs)(IEnumOLEVERB ** ppenumOleVerb);
    STDMETHOD(Update)();
    STDMETHOD(IsUpToDate)();
    STDMETHOD(GetUserClassID)(CLSID * pclsid);
    STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR * lpszUserType);
    STDMETHOD(SetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(GetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(Advise)(IAdviseSink * pAdvSink, DWORD * pdwconnection);
    STDMETHOD(Unadvise)(DWORD dwConnection);
    STDMETHOD(EnumAdvise)(LPENUMSTATDATA * ppenumAdvise);
    STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD FAR* pdwStatus);
    STDMETHOD(SetColorScheme)(LPLOGPALETTE lpLogpal);

    //IViewObject
    STDMETHOD(Draw)(DWORD dwDrawAspect, 
                    LONG lindex, 
                    void *pvAspect,
                    DVTARGETDEVICE *ptd,
                    HDC hdcTargetDev,
                    HDC hdcDraw,
                    LPCRECTL lprcBounds,
                    LPCRECTL lprcWBounds,
                    BOOL (STDMETHODCALLTYPE *pfnContinue)(DWORD dwContinue),
                    DWORD dwContinue);
    STDMETHOD(GetColorSet)(
                    DWORD dwDrawAspect, 
                    LONG lindex, 
                    void FAR* pvAspect,
				    DVTARGETDEVICE FAR * ptd, 
                    HDC hicTargetDev,
				    LPLOGPALETTE FAR* ppColorSet);
    STDMETHOD(Freeze)(DWORD dwDrawAspect,
				    LONG lindex, void FAR* pvAspect, DWORD FAR* pdwFreeze);
    STDMETHOD(Unfreeze)(DWORD dwFreeze);
    STDMETHOD(SetAdvise)(DWORD aspects,
				DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise)(DWORD FAR* pAspects, DWORD FAR* pAdvf, 
				    LPADVISESINK FAR* ppAdvSink);


public:
	SRow				_rw;				// row containing properties

private:
	ULONG				_cRef;				// Reference count

	ULONG				_iName;				// index of display name
	ULONG				_iEid;				// index of entryid
	ULONG				_iType;				// index of recipient type
	BOOL				_fUnderline;	    // Do we draw the underline?
    CPadMessage *       _pPad;              // CPadMessage that instanciated this
											// used for displaying details
											// Note: not ref counted
	LPOLECLIENTSITE		_pClientSite;
	LPSTORAGE			_pstg;				// Associated IStorage

	LPADVISESINK		_padvisesink;
	LPOLEADVISEHOLDER	_poleadviseholder;
    
};

class CTripData: public IDataObject
{
public:
    //Constructor
    CTripData(LPSRowSet prws);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    //IDataObject
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHOD(SetData)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc);
    STDMETHOD(DAdvise)(LPFORMATETC pFormatetc, DWORD advf,
				        LPADVISESINK pAdvSink, DWORD * pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA * ppenumAdvise);
    
    HRESULT GetDataHereOrThere(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);


private:
	ULONG				_cRef;				// Reference count
	LPSRowSet			_prws;				// row set containing data

public:
    static TCHAR        s_szClipFormatTriple[];
    static FORMATETC    s_rgformatetcTRIPOLE[];
};

class CTripCall: public IRichEditOleCallback
{
public:
    CTripCall(CPadMessage * pPad, 
              HWND hwndEdit, 
              BOOL fUnderline);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  // Functions for RICHEDIT callback
    STDMETHOD(GetNewStorage)(LPSTORAGE FAR * ppstg);
    STDMETHOD(GetInPlaceContext)(LPOLEINPLACEFRAME FAR * ppipframe,
								   LPOLEINPLACEUIWINDOW FAR* ppipuiDoc,
								   LPOLEINPLACEFRAMEINFO pipfinfo);
    STDMETHOD(ShowContainerUI)(BOOL fShow);
    STDMETHOD(QueryInsertObject)(LPCLSID pclsid, LPSTORAGE pstg, LONG cp);
    STDMETHOD(DeleteObject)(LPOLEOBJECT poleobj);
    STDMETHOD(QueryAcceptData)(LPDATAOBJECT pdataobj,
                    			CLIPFORMAT *pcfFormat, DWORD reco, BOOL fReally,
				                HGLOBAL hMetaPict);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);
    STDMETHOD(GetClipboardData)(CHARRANGE * pchrg, DWORD reco, LPDATAOBJECT * ppdataobj);
    STDMETHOD(GetDragDropEffect)(BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect);
    STDMETHOD(GetContextMenu)(WORD seltype, LPOLEOBJECT poleobj, CHARRANGE * pchrg, 
                                HMENU * phmenu);

    BOOL GetUnderline() {return _fUnderline;}

private:
	ULONG               _cRef;						// Reference count
    CPadMessage *       _pPad;                      // Pointer back to the pad that instanciated this       
	HWND                _hwndEdit;					// HWND of the edit control
	BOOL                _fUnderline;				// Do we underline the triples
};


