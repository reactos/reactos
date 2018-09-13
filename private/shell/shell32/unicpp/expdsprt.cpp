#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

#include "expdsprt.h"

HRESULT CImpIExpDispSupport::FindCIE4ConnectionPoint(REFIID riid, CIE4ConnectionPoint **ppccp)
{
    CConnectionPoint* pccp = _FindCConnectionPointNoRef(FALSE, riid);

    if (pccp)
    {
        pccp->AddRef();
        *ppccp = pccp;
        return S_OK;
    }
    else
    {
        *ppccp = NULL;
        return E_NOINTERFACE;
    }
}

HRESULT CImpIExpDispSupport::OnTranslateAccelerator(MSG __RPC_FAR *pMsg,DWORD grfModifiers)
{
    return E_NOTIMPL;
}

HRESULT CImpIExpDispSupport::OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                 VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr)
{
    return E_NOTIMPL;
}

HRESULT CImpIConnectionPointContainer::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT *ppCP)
{
    if (NULL == ppCP)
        return E_POINTER;

    CConnectionPoint *pccp = _FindCConnectionPointNoRef(TRUE, iid);
    if (pccp)
    {
        pccp->AddRef();
        *ppCP = pccp->CastToIConnectionPoint();
        return S_OK;
    }
    else
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }
}

#endif // POSTSPLIT
