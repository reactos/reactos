/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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

#pragma once

#include "atlcore.h"


#ifdef _MSC_VER
// It is common to use this in ATL constructors. They only store this for later use, so the usage is safe.
#pragma warning(disable:4355)
#endif

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif

#ifndef _ATL_FREE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_SINGLE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#ifndef ATLTRY
#define ATLTRY(x) x;
#endif

#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

namespace ATL
{

inline HRESULT AtlHresultFromLastError() throw()
{
    DWORD dwError = ::GetLastError();
    return HRESULT_FROM_WIN32(dwError);
}


template<class T>
class CComPtr
{
public:
    T *p;
public:
    CComPtr()
    {
        p = NULL;
    }

    CComPtr(T *lp)
    {
        p = lp;
        if (p != NULL)
            p->AddRef();
    }

    CComPtr(const CComPtr<T> &lp)
    {
        p = lp.p;
        if (p != NULL)
            p->AddRef();
    }

    ~CComPtr()
    {
        if (p != NULL)
            p->Release();
    }

    T *operator = (T *lp)
    {
        T* pOld = p;

        p = lp;
        if (p != NULL)
            p->AddRef();

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }

    T *operator = (const CComPtr<T> &lp)
    {
        T* pOld = p;

        p = lp.p;
        if (p != NULL)
            p->AddRef();

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }

    // We cannot enable this until gcc starts supporting __uuidof
    // See CORE-12710
#if 0
    template <typename Q>
    T* operator=(const CComPtr<Q>& lp)
    {
        T* pOld = p;

        if (!lp.p || FAILED(lp.p->QueryInterface(__uuidof(T), (void**)(IUnknown**)&p)))
            p = NULL;

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }
#endif

    void Release()
    {
        if (p != NULL)
        {
            p->Release();
            p = NULL;
        }
    }

    void Attach(T *lp)
    {
        if (p != NULL)
            p->Release();
        p = lp;
    }

    T *Detach()
    {
        T *saveP;

        saveP = p;
        p = NULL;
        return saveP;
    }

    T **operator & ()
    {
        ATLASSERT(p == NULL);
        return &p;
    }

    operator T * ()
    {
        return p;
    }

    T *operator -> ()
    {
        ATLASSERT(p != NULL);
        return p;
    }
};


//CComQIIDPtr<I_ID(Itype)> is the gcc compatible version of CComQIPtr<Itype>
#define I_ID(Itype) Itype,&IID_##Itype

template <class T, const IID* piid>
class CComQIIDPtr :
    public CComPtr<T>
{
public:
    // Let's tell GCC how to find a symbol.
    using CComPtr<T>::p;

    CComQIIDPtr()
    {
    }
    CComQIIDPtr(_Inout_opt_ T* lp) :
        CComPtr<T>(lp)
    {
    }
    CComQIIDPtr(_Inout_ const CComQIIDPtr<T,piid>& lp):
        CComPtr<T>(lp.p)
    {
    }
    CComQIIDPtr(_Inout_opt_ IUnknown* lp)
    {
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(*piid, (void**)(IUnknown**)&p)))
                p = NULL;
        }
    }
    T *operator = (T *lp)
    {
        T* pOld = p;

        p = lp;
        if (p != NULL)
            p->AddRef();

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }

    T *operator = (const CComQIIDPtr<T,piid> &lp)
    {
        T* pOld = p;

        p = lp.p;
        if (p != NULL)
            p->AddRef();

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }

    T * operator=(IUnknown* lp)
    {
        T* pOld = p;

        if (!lp || FAILED(lp->QueryInterface(*piid, (void**)(IUnknown**)&p)))
            p = NULL;

        if (pOld != NULL)
            pOld->Release();

        return *this;
    }
};


class CComBSTR
{
public:
    BSTR m_str;
public:
    CComBSTR() :
        m_str(NULL)
    {
    }

    CComBSTR(LPCOLESTR pSrc)
    {
        if (pSrc == NULL)
            m_str = NULL;
        else
            m_str = ::SysAllocString(pSrc);
    }

    CComBSTR(int length)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(NULL, length);
    }

    CComBSTR(int length, LPCOLESTR pSrc)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(pSrc, length);
    }

    CComBSTR(PCSTR pSrc)
    {
        if (pSrc)
        {
            int len = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, NULL, 0);
            m_str = ::SysAllocStringLen(NULL, len - 1);
            if (m_str)
            {
                int res = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, m_str, len);
                ATLASSERT(res == len);
                if (res != len)
                {
                    ::SysFreeString(m_str);
                    m_str = NULL;
                }
            }
        }
        else
        {
            m_str = NULL;
        }
    }

    CComBSTR(const CComBSTR &other)
    {
        m_str = other.Copy();
    }

    CComBSTR(REFGUID guid)
    {
        OLECHAR szGuid[40];
        ::StringFromGUID2(guid, szGuid, 40);
        m_str = ::SysAllocString(szGuid);
    }

    ~CComBSTR()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
    }

    operator BSTR () const
    {
        return m_str;
    }

    BSTR *operator & ()
    {
        return &m_str;
    }

    CComBSTR &operator = (const CComBSTR &other)
    {
        ::SysFreeString(m_str);
        m_str = other.Copy();
        return *this;
    }

    void Attach(BSTR bstr)
    {
        ::SysFreeString(m_str);
        m_str = bstr;
    }

    BSTR Detach()
    {
        BSTR str = m_str;
        m_str = NULL;
        return str;
    }

    BSTR Copy() const
    {
        if (!m_str)
            return NULL;
        return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
    }

    HRESULT CopyTo(BSTR *other) const
    {
        if (!other)
            return E_POINTER;
        *other = Copy();
        return S_OK;
    }

    bool LoadString(HMODULE module, DWORD uID)
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        const wchar_t *ptr = NULL;
        int len = ::LoadStringW(module, uID, (PWSTR)&ptr, 0);
        if (len)
            m_str = ::SysAllocStringLen(ptr, len);
        return m_str != NULL;
    }

    unsigned int Length() const
    {
        return ::SysStringLen(m_str);
    }

    unsigned int ByteLength() const
    {
        return ::SysStringByteLen(m_str);
    }
};


class CComVariant : public tagVARIANT
{
public:
    CComVariant()
    {
        ::VariantInit(this);
    }

    CComVariant(const CComVariant& other)
    {
        V_VT(this) = VT_EMPTY;
        Copy(&other);
    }

    ~CComVariant()
    {
        Clear();
    }

    CComVariant(LPCOLESTR lpStr)
    {
        V_VT(this) = VT_BSTR;
        V_BSTR(this) = ::SysAllocString(lpStr);
    }

    CComVariant(LPCSTR lpStr)
    {
        V_VT(this) = VT_BSTR;
        CComBSTR str(lpStr);
        V_BSTR(this) = str.Detach();
    }

    CComVariant(bool value)
    {
        V_VT(this) = VT_BOOL;
        V_BOOL(this) = value ? VARIANT_TRUE : VARIANT_FALSE;
    }

    CComVariant(char value)
    {
        V_VT(this) = VT_I1;
        V_I1(this) = value;
    }

    CComVariant(BYTE value)
    {
        V_VT(this) = VT_UI1;
        V_UI1(this) = value;
    }

    CComVariant(short value)
    {
        V_VT(this) = VT_I2;
        V_I2(this) = value;
    }

    CComVariant(unsigned short value)
    {
        V_VT(this) = VT_UI2;
        V_UI2(this) = value;
    }

    CComVariant(int value, VARENUM type = VT_I4)
    {
        if (type == VT_I4 || type == VT_INT)
        {
            V_VT(this) = type;
            V_I4(this) = value;
        }
        else
        {
            V_VT(this) = VT_ERROR;
            V_ERROR(this) = E_INVALIDARG;
        }
    }

    CComVariant(unsigned int value, VARENUM type = VT_UI4)
    {
        if (type == VT_UI4 || type == VT_UINT)
        {
            V_VT(this) = type;
            V_UI4(this) = value;
        }
        else
        {
            V_VT(this) = VT_ERROR;
            V_ERROR(this) = E_INVALIDARG;
        }
    }

    CComVariant(long value, VARENUM type = VT_I4)
    {
        if (type == VT_I4 || type == VT_ERROR)
        {
            V_VT(this) = type;
            V_I4(this) = value;
        }
        else
        {
            V_VT(this) = VT_ERROR;
            V_ERROR(this) = E_INVALIDARG;
        }
    }

    CComVariant(unsigned long value)
    {
        V_VT(this) = VT_UI4;
        V_UI4(this) = value;
    }

    CComVariant(float value)
    {
        V_VT(this) = VT_R4;
        V_R4(this) = value;
    }

    CComVariant(double value, VARENUM type = VT_R8)
    {
        if (type == VT_R8 || type == VT_DATE)
        {
            V_VT(this) = type;
            V_R8(this) = value;
        }
        else
        {
            V_VT(this) = VT_ERROR;
            V_ERROR(this) = E_INVALIDARG;
        }
    }

    CComVariant(const LONGLONG& value)
    {
        V_VT(this) = VT_I8;
        V_I8(this) = value;
    }

    CComVariant(const ULONGLONG& value)
    {
        V_VT(this) = VT_UI8;
        V_UI8(this) = value;
    }

    CComVariant(const CY& value)
    {
        V_VT(this) = VT_CY;
        V_I8(this) = value.int64;
    }


    HRESULT Clear()
    {
        return ::VariantClear(this);
    }

    HRESULT Copy(_In_ const VARIANT* src)
    {
        return ::VariantCopy(this, const_cast<VARIANT*>(src));
    }

    HRESULT ChangeType(_In_ VARTYPE newType, _In_opt_ const LPVARIANT src = NULL)
    {
        const LPVARIANT lpSrc = src ? src : this;
        return ::VariantChangeType(this, lpSrc, 0, newType);
    }
};



}; // namespace ATL

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

