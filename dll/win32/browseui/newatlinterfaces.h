/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

template<class T>
class IProfferServiceImpl : public IProfferService
{
public:
    STDMETHODIMP ProfferService(REFGUID rguidService, IServiceProvider *psp, DWORD *pdwCookie)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP RevokeService(DWORD dwCookie)
    {
        return E_NOTIMPL;
    }

    HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
    {
        return E_FAIL;
    }
};

/*
This subclass corrects a problem with the ATL IConnectionPointImpl.
IConnectionPointImpl queries for the exact interface that is published, but at least one
implementor of IConnectionPoint (CShellBrowser) advertises DIID_DWebBrowserEvents,
but fires events to IDispatch.
*/
template<class T, const IID *piid, class CDV = CComDynamicUnkArray>
class MyIConnectionPointImpl : public IConnectionPointImpl<T, piid, CDV>
{
public:
    STDMETHODIMP Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
    {
        IConnectionPointImpl<T, piid, CDV>  *pThisCPImpl;
        T                                   *pThis;
        IUnknown                            *adviseSink;
        DWORD                               newCookie;
        HRESULT                             hResult;

        pThis = static_cast<T *>(this);
        pThisCPImpl = static_cast<IConnectionPointImpl<T, piid, CDV> *>(this);
        if (pdwCookie != NULL)
            *pdwCookie = 0;
        if (pUnkSink == NULL || pdwCookie == NULL)
            return E_POINTER;
        hResult = pUnkSink->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&adviseSink));
        if (FAILED(hResult))
        {
            if (hResult == E_NOINTERFACE)
                return CONNECT_E_CANNOTCONNECT;
            return hResult;
        }
        pThis->Lock();
        newCookie = pThisCPImpl->m_vec.Add(adviseSink);
        pThis->Unlock();
        *pdwCookie = newCookie;
        if (newCookie != 0)
            hResult = S_OK;
        else
        {
            adviseSink->Release();
            hResult = CONNECT_E_ADVISELIMIT;
        }
        return hResult;
    }
};
