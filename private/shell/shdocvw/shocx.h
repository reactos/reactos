#ifndef __SHOCX_H__
#define __SHOCX_H__

#include "cnctnpt.h"
#include "dspsprt.h"
#include "expdsprt.h"

//
// shocx.h
//

#define _INTERFACEOFOBJECT      1
#define _CLSIDOFOBJECT          2

class CShellOcx: public CShellEmbedding,        // IOleObject, IOleInPlacceObject, IOleInPlaceActiveObject,
                                                // IViewObject2, IPersistStorage
                 public IPersistStreamInit,
                 public IPersistPropertyBag,
                 public IOleControl,            // OnAmbientPropertyChange
                 public IDispatch,
                 public IProvideClassInfo2,
                 protected CImpIConnectionPointContainer,
                 protected CImpIDispatch
{
public:
    // *** IUnknown *** (we multiply inherit from IUnknown, disambiguate here)
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
        { return CShellEmbedding::QueryInterface(riid, ppvObj); }
    virtual STDMETHODIMP_(ULONG) AddRef(void)
        { return CShellEmbedding::AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return CShellEmbedding::Release(); }

    // *** IPersistStreamInit ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) {return CShellEmbedding::GetClassID(pClassID);} // IPersistStorage implementation
    virtual STDMETHODIMP IsDirty(void) {return _fDirty ? S_OK : S_FALSE;}
    virtual STDMETHODIMP Load(IStream *pStm) PURE;
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) PURE;
    virtual STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
    virtual STDMETHODIMP InitNew(void) PURE;

    // *** IPersistPropertyBag ***
    virtual STDMETHODIMP Load(IPropertyBag *pBag, IErrorLog *pErrorLog) PURE;
    virtual STDMETHODIMP Save(IPropertyBag *pBag, BOOL fClearDirty, BOOL fSaveAllProperties) PURE;

    // *** IOleControl ***
    virtual STDMETHODIMP GetControlInfo(LPCONTROLINFO pCI);
    virtual STDMETHODIMP OnMnemonic(LPMSG pMsg);
    virtual STDMETHODIMP OnAmbientPropertyChange(DISPID dispid);
    virtual STDMETHODIMP FreezeEvents(BOOL bFreeze);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT FAR* pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // *** CImpIConnectionPointContainer ***
    virtual STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);

    // *** IProvideClassInfo2 ***
    virtual STDMETHODIMP GetClassInfo(LPTYPEINFO * ppTI);
    virtual STDMETHODIMP GetGUID(DWORD dwGuidKind, GUID *pGUID);

    // IPropertyNotifySink stuff.
    //
    inline void  PropertyChanged(DISPID dispid) {
        m_cpPropNotify.OnChanged(dispid);
    }

    /*
    ** CShellEmbedding stuff
    */

    // *** IOleObject ***
    virtual STDMETHODIMP EnumVerbs(IEnumOLEVERB **ppEnumOleVerb);
    virtual STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);

    // *** IViewObject ***
    virtual STDMETHODIMP Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
        const RECTL *, const RECTL *, BOOL (*)(ULONG_PTR), ULONG_PTR);

    /*
    ** CShellOcx specific stuff
    */

    CShellOcx(IUnknown* punkOuter, LPCOBJECTINFO poi, const OLEVERB* pverbs=NULL, const OLEVERB* pdesignverbs=NULL);
    ~CShellOcx();

protected:

    // from CShellEmbedding
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid);

    ITypeInfo *_pClassTypeInfo; // ITypeInfo of class

    const OLEVERB* _pDesignVerbs;   // verb list for design mode -- run mode is in CShellEmbedding

    // Ambient Properties we care about
    IDispatch* _pDispAmbient;
    BOOL _GetAmbientProperty(DISPID dispid, VARTYPE vt, void *pData);
    int  _nDesignMode;          // MODE_UNKNOWN, MODE_TRUE, MODE_FALSE
    BOOL _IsDesignMode(void);   // TRUE means we have a design-mode container

    BOOL _fEventsFrozen:1;


    CConnectionPoint m_cpEvents;
    CConnectionPoint m_cpPropNotify;
} ;

// _nDesignMode,etc flags
#define MODE_UNKNOWN -1      // mode has not yet been determined
#define MODE_TRUE    1
#define MODE_FALSE   0

// CConnectionPoint types:
#define SINK_TYPE_EVENT      0
#define SINK_TYPE_PROPNOTIFY 1

#endif // __SHOCX_H__
