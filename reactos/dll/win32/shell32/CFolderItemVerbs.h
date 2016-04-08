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

#ifndef _FOLDERITEMVERBS_H_
#define _FOLDERITEMVERBS_H_

class CFolderItemVerb:
    public CComCoClass<CFolderItemVerb>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public FolderItemVerb
{
private:

public:
    CFolderItemVerb();
    ~CFolderItemVerb();

    //void Init(LPITEMIDLIST idlist);


    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

    // *** FolderItemVerb methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *pbs);
    virtual HRESULT STDMETHODCALLTYPE DoIt();


DECLARE_NOT_AGGREGATABLE(CFolderItemVerb)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItemVerb)
    COM_INTERFACE_ENTRY_IID(IID_FolderItemVerb, FolderItemVerb)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()
};


class CFolderItemVerbs:
    public CComCoClass<CFolderItemVerbs>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public FolderItemVerbs
{
private:

public:
    CFolderItemVerbs();
    ~CFolderItemVerbs();

    //void Init(LPITEMIDLIST idlist);


    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

    // *** FolderItemVerbs methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Count(LONG *plCount);
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE Item(VARIANT index, FolderItemVerb **ppid);
    virtual HRESULT STDMETHODCALLTYPE _NewEnum(IUnknown **ppunk);

DECLARE_NOT_AGGREGATABLE(CFolderItemVerbs)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItemVerbs)
    COM_INTERFACE_ENTRY_IID(IID_FolderItemVerbs, FolderItemVerbs)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()
};

#endif
