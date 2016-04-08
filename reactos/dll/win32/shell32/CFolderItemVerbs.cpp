/*
 * FolderItemVerb(s) implementation
 *
 * Copyright 2015 Mark Jansen
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CFolderItemVerb::CFolderItemVerb()
{
}

CFolderItemVerb::~CFolderItemVerb()
{
}

//void CFolderItemVerb::Init(LPITEMIDLIST idlist)
//{
//    m_idlist.Attach(idlist);
//}

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CFolderItemVerb::GetTypeInfoCount(UINT *pctinfo)
{
    TRACE("(%p, %p)\n", this, pctinfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%p, %lu, %lu, %p)\n", this, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    TRACE("(%p, %s, %p, %lu, %lu, %p)\n", this, wine_dbgstr_guid(&riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    TRACE("(%p, %lu, %s, %lu, %lu, %p, %p, %p, %p)\n", this, dispIdMember, wine_dbgstr_guid(&riid), lcid, (DWORD)wFlags,
        pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

// *** FolderItemVerb methods ***

HRESULT STDMETHODCALLTYPE CFolderItemVerb::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::get_Name(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);
    if (!pbs)
        return E_POINTER;
    // Terminating item:
    *pbs = SysAllocString(L"");
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerb::DoIt()
{
    TRACE("(%p, %p)\n", this);
    return E_NOTIMPL;
}






CFolderItemVerbs::CFolderItemVerbs()
{
}

CFolderItemVerbs::~CFolderItemVerbs()
{
}

//void CFolderItemVerbs::Init(LPITEMIDLIST idlist)
//{
//    m_idlist.Attach(idlist);
//}

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CFolderItemVerbs::GetTypeInfoCount(UINT *pctinfo)
{
    TRACE("(%p, %p)\n", this, pctinfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%p, %lu, %lu, %p)\n", this, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    TRACE("(%p, %s, %p, %lu, %lu, %p)\n", this, wine_dbgstr_guid(&riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    TRACE("(%p, %lu, %s, %lu, %lu, %p, %p, %p, %p)\n", this, dispIdMember, wine_dbgstr_guid(&riid), lcid, (DWORD)wFlags,
        pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

// *** FolderItemVerbs methods ***
HRESULT STDMETHODCALLTYPE CFolderItemVerbs::get_Count(LONG *plCount)
{
    TRACE("(%p, %p)\n", this, plCount);
    if (!plCount)
        return E_POINTER;
    *plCount = 0;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::Item(VARIANT index, FolderItemVerb **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_variant(&index), ppid);
    if (!ppid)
        return E_POINTER;

    /* FIXME! */
    CFolderItemVerb* verb = new CComObject<CFolderItemVerb>();
    verb->AddRef();
    *ppid = verb;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItemVerbs::_NewEnum(IUnknown **ppunk)
{
    TRACE("(%p, %p)\n", this, ppunk);
    return E_NOTIMPL;
}


