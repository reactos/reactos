#ifndef __EXPDSPRT_H__
#define __EXPDSPRT_H__

#include "cnctnpt.h"

#ifdef TF_SHDLIFE
#undef TF_SHDLIFE
#endif

#define TF_SHDLIFE       TF_CUSTOM1

//
// Helper C++ class used to share code for the IExpDispSupport...
//
// 
class CImpIExpDispSupport : public IExpDispSupport
{
    public:
        // We need access to the virtual QI -- define it PURE here
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;
        virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
        virtual STDMETHODIMP_(ULONG) Release(void) PURE;

        // *** IExpDispSupport specific methods ***
        virtual STDMETHODIMP FindCIE4ConnectionPoint(REFIID riid, CIE4ConnectionPoint **ppccp);
        virtual STDMETHODIMP OnTranslateAccelerator(MSG __RPC_FAR *pMsg,DWORD grfModifiers);
        virtual STDMETHODIMP OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                            VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr);

    protected:
        virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID riid) PURE;

        CImpIExpDispSupport() { TraceMsg(TF_SHDLIFE, "ctor CImpIExpDispSupport %x", this); }
        ~CImpIExpDispSupport() { TraceMsg(TF_SHDLIFE, "dtor CImpIExpDispSupport %x", this); }
};

// CImpIExpDispSupport implements half of IConnectionPoint
// 
class CImpIConnectionPointContainer : public IConnectionPointContainer
{
    public:
        // We need access to the virtual QI -- define it PURE here
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;
        virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
        virtual STDMETHODIMP_(ULONG) Release(void) PURE;

        // *** IConnectionPointContainer ***
        virtual STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS FAR* ppEnum) PURE;
        virtual STDMETHODIMP FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT FAR* ppCP);

    protected:
        virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid) PURE;

        CImpIConnectionPointContainer() { TraceMsg(TF_SHDLIFE, "ctor CImpIExpDispSupport %x", this); }
        ~CImpIConnectionPointContainer() { TraceMsg(TF_SHDLIFE, "dtor CImpIExpDispSupport %x", this); }
};

#endif // __EXPDSPRT_H__
