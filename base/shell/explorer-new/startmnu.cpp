/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

HRESULT
UpdateStartMenu(IN OUT IMenuPopup *pMenuPopup,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    CComPtr<IBanneredBar> pbb;
    HRESULT hRet;

    hRet = pMenuPopup->QueryInterface(IID_PPV_ARG(IBanneredBar, &pbb));
    if (SUCCEEDED(hRet))
    {
        hRet = pbb->SetBitmap(hbmBanner);

        /* Update the icon size */
        hRet = pbb->SetIconSize(bSmallIcons ? BMICON_SMALL : BMICON_LARGE);
    }

    return hRet;
}

IMenuPopup *
CreateStartMenu(IN ITrayWindow *Tray,
                OUT IMenuBand **ppMenuBand,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    HRESULT hr;
    IObjectWithSite *pOws = NULL;
    IMenuPopup *pMp = NULL;
    IUnknown *pSms = NULL;
    IMenuBand *pMb = NULL;
    IInitializeObject *pIo;
    IUnknown *pUnk = NULL;
    IBandSite *pBs = NULL;
    DWORD dwBandId = 0;

    hr = CreateStartMenuSite(Tray, IID_PPV_ARG(IUnknown, &pSms));
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

#if 0
    hr = CoCreateInstance(&CLSID_StartMenu,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_IMenuPopup,
                          (PVOID *)&pMp);
#else
    hr = _CStartMenu_Constructor(IID_PPV_ARG(IMenuPopup, &pMp));
#endif
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("CoCreateInstance failed: %x\n", hr);
        goto cleanup;
    }

    hr = pMp->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pOws));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_QueryInterface failed: %x\n", hr);
        goto cleanup;
    }

    /* Set the menu site so we can handle messages */
    hr = pOws->SetSite(pSms);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IObjectWithSite_SetSite failed: %x\n", hr);
        goto cleanup;
    }

    /* Initialize the menu object */
    hr = pMp->QueryInterface(IID_PPV_ARG(IInitializeObject, &pIo));
    if (SUCCEEDED(hr))
    {
        hr = pIo->Initialize();
        pIo->Release();
    }
    else
        hr = S_OK;

    /* Everything is initialized now. Let's get the IMenuBand interface. */
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_QueryInterface failed: %x\n", hr);
        goto cleanup;
    }

    hr = pMp->GetClient(&pUnk);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_GetClient failed: %x\n", hr);
        goto cleanup;
    }

    hr = pUnk->QueryInterface(IID_PPV_ARG(IBandSite, &pBs));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IUnknown_QueryInterface pBs failed: %x\n", hr);
        goto cleanup;
    }

    /* Finally we have the IBandSite interface, there's only one
       band in it that apparently provides the IMenuBand interface */
    hr = pBs->EnumBands(0, &dwBandId);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IBandSite_EnumBands failed: %x\n", hr);
        goto cleanup;
    }

    hr = pBs->GetBandObject(dwBandId, IID_PPV_ARG(IMenuBand, &pMb));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IBandSite_GetBandObject failed: %x\n", hr);
        goto cleanup;
    }

    UpdateStartMenu(pMp,
                    hbmBanner,
                    bSmallIcons);

cleanup:
    if (SUCCEEDED(hr))
        *ppMenuBand = pMb;
    else if (pMb != NULL)
        pMb->Release();

    if (pBs != NULL)
        pBs->Release();
    if (pUnk != NULL)
        pUnk->Release();
    if (pOws != NULL)
        pOws->Release();
    if (pMp != NULL)
        pMp->Release();
    if (pSms != NULL)
        pSms->Release();

    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;
    return pMp;
}
