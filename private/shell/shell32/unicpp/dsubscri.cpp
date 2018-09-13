#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

// This function checks if a given URL already has a subscription.
// Returns TRUE: if it aleady has a subscription 
//         FALSE: Otherwise.
//
BOOL CheckForExistingSubscription(LPCTSTR lpcszURL)
{
    HRESULT hr;
    ISubscriptionMgr *psm;
    BOOL    fRet = FALSE;  //Assume failure.

    //Create the subscription Manager.
    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionMgr,
                          (void**)&psm);

    if (SUCCEEDED(hr))
    {
        BSTR bstrURL = SysAllocStringT(lpcszURL);
        if (bstrURL)
        {
            psm->IsSubscribed(bstrURL, &fRet);
            SysFreeString(bstrURL);
        }

        psm->Release();
    }

    return(fRet);
}

BOOL DeleteFromSubscriptionList(LPCTSTR pszURL)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    ISubscriptionMgr *psm;

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionMgr,
                          (void**)&psm);

    if (SUCCEEDED(hr))
    {
        BSTR bstrURL = SysAllocStringT(pszURL);     // Call TSTR version
        if (bstrURL)
        {
            //  Looks like all code paths going through this has already
            //  put up some UI.
            if (SUCCEEDED(psm->DeleteSubscription(bstrURL, NULL)))
            {
                fRet = TRUE;
            }

            SysFreeString(bstrURL);
        }

        psm->Release();
    }

    return(fRet);
}

BOOL UpdateSubscription(LPCTSTR pszURL)
{
    BOOL fRet = FALSE;
    HRESULT hr;
    ISubscriptionMgr *psm;

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionMgr,
                          (void**)&psm);

    if (SUCCEEDED(hr))
    {
        BSTR bstrURL = SysAllocStringT(pszURL);     // Call TSTR version
        if (bstrURL)
        {
            if (SUCCEEDED(psm->UpdateSubscription(bstrURL)))
            {
                fRet = TRUE;
            }

            SysFreeString(bstrURL);
        }

        psm->Release();
    }

    return(fRet);
}

//
//
// This function enumerates the URLs of all the desktop components and then
// calls webcheck to see if they are subcribed to and if so asks webcheck to
// deliver those subscriptions right now.
//
//

BOOL UpdateAllDesktopSubscriptions()
{
    IActiveDesktop  *pActiveDesktop;
    ISubscriptionMgr *psm;
    int     iCount; //Count of components.
    HRESULT     hres;
    BOOL        fRet = TRUE;  //Assume success!

    if(FAILED(hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&pActiveDesktop, IID_IActiveDesktop)))
    {
        TraceMsg(TF_WARNING, "Could not instantiate CActiveDesktop COM object");
        return FALSE;
    }

    pActiveDesktop->GetDesktopItemCount(&iCount, 0);

    if(iCount <= 0)
    {
        TraceMsg(DM_TRACE, "No desktop components to update!");
        return TRUE; //No components to enumerate!
    }

    //Create the subscription Manager.
    hres = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ISubscriptionMgr,
                            (void**)&psm);

    if(SUCCEEDED(hres))
    {
        int iIndex;
        BSTR bstrURL;

        //Enumerate the desktop components one by one.
        for(iIndex = 0; iIndex < iCount; iIndex++)
        {
            COMPONENT   Comp;   //We are using the public structure here.

            Comp.dwSize = sizeof(COMPONENT);
            if(SUCCEEDED(pActiveDesktop->GetDesktopItem(iIndex, &Comp, 0)) && 
                        Comp.fChecked)  //Is this component enabled?
            {
                BOOL    fSubscribed;

                fSubscribed = FALSE;  //Assume that it is NOT subscribed!

                bstrURL = SysAllocString(Comp.wszSubscribedURL);
                if(!bstrURL)
                {
                    fRet = FALSE;
                    break;  //Out of memory!
                }

                psm->IsSubscribed(bstrURL, &fSubscribed);

                if(fSubscribed)
                    psm->UpdateSubscription(bstrURL);

                SysFreeString(bstrURL);
            }
            else
                TraceMsg(TF_WARNING, "Component# %d either failed or not enabled!", iIndex);
        }
        psm->Release();
    }
    else
    {
        TraceMsg(TF_WARNING, "Could not create CLSID_SubscriptionMgr");
    }
        
    pActiveDesktop->Release();

    return (fRet);
}
#endif
