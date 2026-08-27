/*
 * PROJECT:     ReactOS MMC
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     .msc file parser
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#define COBJMACROS
#include "precomp.h"
#include <oleauto.h>
#include <oaidl.h>
#include <shlwapi.h>
#include <initguid.h> // For CLSID_DOMDocument30
#include <msxml2.h>

static HRESULT xmldomnode_getattributevalue(IXMLDOMNode *pnode, LPCWSTR name, BSTR *pout)
{
    IXMLDOMNamedNodeMap *pmap;
    HRESULT hr = E_OUTOFMEMORY;
    BSTR bsname = SysAllocString(name);
    *pout = NULL;
    if (bsname && SUCCEEDED(hr = IXMLDOMNode_get_attributes(pnode, &pmap)))
    {
        if (SUCCEEDED(hr = IXMLDOMNamedNodeMap_getNamedItem(pmap, bsname, &pnode)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            if (pnode)
            {
                hr = IXMLDOMNode_get_text(pnode, pout);
                if (SUCCEEDED(hr) && !*pout)
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                IXMLDOMNode_Release(pnode);
            }
        }
        IXMLDOMNamedNodeMap_Release(pmap);
    }
    SysFreeString(bsname);
    return hr;
}

static HRESULT xmldomelem_getelembytag(IXMLDOMElement *pelem, LPCWSTR name, long index, IXMLDOMNode**ppout)
{
    HRESULT hr = E_OUTOFMEMORY;
    IXMLDOMNodeList *pnl;
    BSTR bsname = SysAllocString(name);
    *ppout = NULL;
    if (bsname && SUCCEEDED(hr = IXMLDOMElement_getElementsByTagName(pelem, bsname, &pnl)))
    {
        hr = IXMLDOMNodeList_get_item(pnl, index, ppout);
        if (SUCCEEDED(hr) && !*ppout)
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        IUnknown_Release(pnl);
    }
    SysFreeString(bsname);
    return hr;
}

static HRESULT xmldomunk_getelembytag(IUnknown *punk, LPCWSTR name, long index, IXMLDOMNode**ppout)
{
    IXMLDOMElement *pelem;
    HRESULT hr = IUnknown_QueryInterface(punk, &IID_IXMLDOMElement, (void**)&pelem);
    if (FAILED(hr))
    {
        *ppout = NULL;
        return hr;
    }
    hr = xmldomelem_getelembytag(pelem, name, index, ppout);
    IUnknown_Release(pelem);
    return hr;
}

static HRESULT LoadXmlFromVariant(VARIANT *pVarUrl, IXMLDOMElement **ppDocElm)
{
    VARIANT_BOOL succ = VARIANT_FALSE;
    IXMLDOMDocument *pDoc;
    HRESULT hr = CoCreateInstance(&CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IXMLDOMDocument, (void**)&pDoc);
    if (FAILED(hr))
        return hr;
    else if (SUCCEEDED(hr = IXMLDOMDocument_load(pDoc, *pVarUrl, &succ)) && succ)
        hr = IXMLDOMDocument_get_documentElement(pDoc, ppDocElm);
    else if (SUCCEEDED(hr))
        hr = E_FAIL;
    IUnknown_Release(pDoc);
    return hr;
}

static HRESULT GetMscRootNode(PCWSTR pszFilePath, IXMLDOMNode **ppRoot)
{
    HRESULT hr;
    DWORD len = lstrlenW(pszFilePath);
    DWORD cch = sizeof("file:///") + (len * 2); // *2 is overkill but we don't know how many escaped characters there are
    PWSTR pszUrl = LocalAlloc(LPTR, cch * sizeof(*pszUrl));
    if (!pszUrl)
        return E_OUTOFMEMORY;
    hr = UrlCreateFromPathW(pszFilePath, pszUrl, &cch, 0);
    if (SUCCEEDED(hr))
    {
        IXMLDOMElement *pDocElm;
        BSTR bsurl = SysAllocString(pszUrl);
        VARIANT v;
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = bsurl;
        hr = bsurl ? LoadXmlFromVariant(&v, &pDocElm) : E_OUTOFMEMORY;
        VariantClear(&v);
        LocalFree(pszUrl);
        if (SUCCEEDED(hr))
        {
            BSTR bs = 0;
            hr = IXMLDOMElement_get_nodeName(pDocElm, &bs);
            if (SUCCEEDED(hr))
            {
                hr = lstrcmpiW(bs, L"MMC_ConsoleFile") ? HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) : S_OK;
                SysFreeString(bs);
                if (SUCCEEDED(hr))
                    hr = IUnknown_QueryInterface(pDocElm, &IID_IXMLDOMNode, (void**)ppRoot);
            }
            IUnknown_Release(pDocElm);
        }
    }
    return hr;
}

EXTERN_C BOOL HandleRosMscLaunch(PCWSTR pszCmdLine)
{
    BOOL result = FALSE;
    IXMLDOMNode *pRoot;
    HRESULT hrCom = CoInitialize(NULL);

    int argc;
    LPWSTR *argv = CommandLineToArgvW(pszCmdLine, &argc);
    if (argv && argc > 0)
    {
        if (SUCCEEDED(GetMscRootNode(argv[0], &pRoot)))
        {
            IXMLDOMNode *pNode;
            HRESULT hr = xmldomunk_getelembytag((IUnknown*)pRoot, L"RosLaunch", 0, &pNode);
            IUnknown_Release(pRoot);
            if (SUCCEEDED(hr))
            {
                UINT flags = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
                SHELLEXECUTEINFOW sei = { sizeof(sei), flags, NULL, NULL, NULL, NULL, NULL, SW_SHOW };
                if (SUCCEEDED(xmldomnode_getattributevalue(pNode, L"File", (BSTR*)&sei.lpFile)))
                {
                    xmldomnode_getattributevalue(pNode, L"Parameters", (BSTR*)&sei.lpParameters);
                    result = sei.lpFile && *sei.lpFile && ShellExecuteExW(&sei);
                    SysFreeString((BSTR)sei.lpFile);
                    SysFreeString((BSTR)sei.lpParameters);
                }
                IUnknown_Release(pNode);
            }
        }
        LocalFree(argv);
    }

    if (SUCCEEDED(hrCom))
        CoUninitialize();
    return result;
}
