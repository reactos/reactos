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
    CComPtr<IMenuPopup> pMp;
    CComPtr<IUnknown> pSms;
    CComPtr<IMenuBand> pMb;
    CComPtr<IInitializeObject> pIo;
    CComPtr<IUnknown> pUnk;
    CComPtr<IBandSite> pBs;
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
        return NULL;

    /* Set the menu site so we can handle messages */
    hr = IUnknown_SetSite(pMp, pSms);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Initialize the menu object */
    hr = pMp->QueryInterface(IID_PPV_ARG(IInitializeObject, &pIo));
    if (SUCCEEDED(hr))
        hr = pIo->Initialize();
    else
        hr = S_OK;

    /* Everything is initialized now. Let's get the IMenuBand interface. */
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    hr = pMp->GetClient(&pUnk);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    hr = pUnk->QueryInterface(IID_PPV_ARG(IBandSite, &pBs));
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    /* Finally we have the IBandSite interface, there's only one
       band in it that apparently provides the IMenuBand interface */
    hr = pBs->EnumBands(0, &dwBandId);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    hr = pBs->GetBandObject(dwBandId, IID_PPV_ARG(IMenuBand, &pMb));
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    UpdateStartMenu(pMp,
                    hbmBanner,
                    bSmallIcons);

    *ppMenuBand = pMb.Detach();

    return pMp.Detach();
}
