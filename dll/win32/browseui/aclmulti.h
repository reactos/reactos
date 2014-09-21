/*
 *  Multisource AutoComplete list
 *
 *  Copyright 2007  Mikolaj Zalewski
 *  Copyright 2009  Andrew Hill
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

#pragma once

class CACLMulti :
    public CComCoClass<CACLMulti, &CLSID_ACLMulti>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IObjMgr,
    public IACList
{
private:
    struct ACLMultiSublist
    {
        IUnknown                            *punk;
        IEnumString                         *pEnum;
        IACList                             *pACL;
    };

    INT                                     fObjectCount;
    INT                                     fCurrentObject;
    struct ACLMultiSublist                  *fObjects;
public:
    CACLMulti();
    ~CACLMulti();

    // *** IEnumString methods ***
    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    virtual HRESULT STDMETHODCALLTYPE Reset();
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum);

    // *** IACList methods ***
    virtual HRESULT STDMETHODCALLTYPE Expand(LPCOLESTR pszExpand);

    // *** IObjMgr methods ***
    virtual HRESULT STDMETHODCALLTYPE Append(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE Remove(IUnknown *punk);

private:
    void release_obj(struct ACLMultiSublist *obj);

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_ACLMULTI)
    DECLARE_NOT_AGGREGATABLE(CACLMulti)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CACLMulti)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
        COM_INTERFACE_ENTRY_IID(IID_IObjMgr, IObjMgr)
    END_COM_MAP()
};
