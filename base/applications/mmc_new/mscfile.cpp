/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Helper functions for MSXML3
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#include <initguid.h>

DEFINE_GUID(CLSID_DOMDocument30, 0xf5078f32, 0xc551, 0x11d3, 0x89,0xb9, 0x00,0x00,0xf8,0x1f,0xe2,0x21);


MscFile::MscFile(PCWSTR pszFileName)
{
    m_pDocument = NULL;
    VariantInit(&m_varFileName);
    VariantFromString(pszFileName, m_varFileName);
}

MscFile::~MscFile()
{
    VariantClear(&m_varFileName);
    SAFE_RELEASE(m_pDocument);
}


// Helper function to create a VT_BSTR variant from a null terminated string. 
HRESULT
MscFile::VariantFromString(PCWSTR wszValue, VARIANT &Variant)
{
    HRESULT hr = S_OK;
    BSTR bstr = SysAllocString(wszValue);
    CHK_ALLOC(bstr);
    
    V_VT(&Variant)   = VT_BSTR;
    V_BSTR(&Variant) = bstr;

CleanUp:
    return hr;
}

// Helper function to create a DOM instance. 
HRESULT
MscFile::CreateAndInitDOM()
{
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument30,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument,
                                  (void**)&m_pDocument);
    if (SUCCEEDED(hr))
    {
        m_pDocument->put_async(VARIANT_FALSE);  
        m_pDocument->put_validateOnParse(VARIANT_FALSE);
        m_pDocument->put_resolveExternals(VARIANT_FALSE);
        m_pDocument->put_preserveWhiteSpace(VARIANT_TRUE);
    }
    return hr;
}

// Helper that allocates the BSTR param for the caller.
HRESULT
MscFile::CreateElement(PCWSTR wszName, IXMLDOMElement **ppElement)
{
    HRESULT hr = S_OK;
    *ppElement = NULL;

    BSTR bstrName = SysAllocString(wszName);
    CHK_ALLOC(bstrName);
    CHK_HR(m_pDocument->createElement(bstrName, ppElement));

CleanUp:
    SysFreeString(bstrName);
    return hr;
}

// Helper function to append a child to a parent node.
HRESULT
MscFile::AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
    HRESULT hr = S_OK;
    IXMLDOMNode *pChildOut = NULL;
    if (pParent == NULL)
        CHK_HR(m_pDocument->appendChild(pChild, &pChildOut));
    else
        CHK_HR(pParent->appendChild(pChild, &pChildOut));

CleanUp:
    SAFE_RELEASE(pChildOut);
    return hr;
}

// Helper function to create and add a processing instruction to a document node.
HRESULT
MscFile::CreateAndAddPINode(PCWSTR wszTarget, PCWSTR wszData)
{
    HRESULT hr = S_OK;
    IXMLDOMProcessingInstruction *pPI = NULL;

    BSTR bstrTarget = SysAllocString(wszTarget);
    BSTR bstrData = SysAllocString(wszData);
    CHK_ALLOC(bstrTarget && bstrData);
    
    CHK_HR(m_pDocument->createProcessingInstruction(bstrTarget, bstrData, &pPI));
    CHK_HR(AppendChildToParent(pPI, m_pDocument));

CleanUp:
    SAFE_RELEASE(pPI);
    SysFreeString(bstrTarget);
    SysFreeString(bstrData);
    return hr;
}

    // Helper function to create and add a comment to a document node.
    HRESULT MscFile::CreateAndAddCommentNode(PCWSTR wszComment)
    {
        HRESULT hr = S_OK;
        IXMLDOMComment *pComment = NULL;

        BSTR bstrComment = SysAllocString(wszComment);
        CHK_ALLOC(bstrComment);
        
        CHK_HR(m_pDocument->createComment(bstrComment, &pComment));
        CHK_HR(AppendChildToParent(pComment, m_pDocument));

    CleanUp:
        SAFE_RELEASE(pComment);
        SysFreeString(bstrComment);
        return hr;
    }

    // Helper function to create and add an attribute to a parent node.
    HRESULT MscFile::CreateAndAddAttributeNode(PCWSTR wszName, PCWSTR wszValue, IXMLDOMElement *pParent)
    {
        HRESULT hr = S_OK;
        IXMLDOMAttribute *pAttribute = NULL;
        IXMLDOMAttribute *pAttributeOut = NULL; // Out param that is not used

        BSTR bstrName = NULL;
        VARIANT varValue;
        VariantInit(&varValue);

        bstrName = SysAllocString(wszName);
        CHK_ALLOC(bstrName);
        CHK_HR(VariantFromString(wszValue, varValue));

        CHK_HR(m_pDocument->createAttribute(bstrName, &pAttribute));
        CHK_HR(pAttribute->put_value(varValue));
        CHK_HR(pParent->setAttributeNode(pAttribute, &pAttributeOut));

    CleanUp:
        SAFE_RELEASE(pAttribute);
        SAFE_RELEASE(pAttributeOut);
        SysFreeString(bstrName);
        VariantClear(&varValue);
        return hr;
    }

    // Helper function to create and append a text node to a parent node.
    HRESULT MscFile::CreateAndAddTextNode(PCWSTR wszText, IXMLDOMNode *pParent)
    {
        HRESULT hr = S_OK;    
        IXMLDOMText *pText = NULL;

        BSTR bstrText = SysAllocString(wszText);
        CHK_ALLOC(bstrText);

        CHK_HR(m_pDocument->createTextNode(bstrText, &pText));
        CHK_HR(AppendChildToParent(pText, pParent));

    CleanUp:
        SAFE_RELEASE(pText);
        SysFreeString(bstrText);
        return hr;
    }

    // Helper function to create and append a CDATA node to a parent node.
    HRESULT MscFile::CreateAndAddCDATANode(PCWSTR wszCDATA, IXMLDOMNode *pParent)
    {
        HRESULT hr = S_OK;
        IXMLDOMCDATASection *pCDATA = NULL;

        BSTR bstrCDATA = SysAllocString(wszCDATA);
        CHK_ALLOC(bstrCDATA);

        CHK_HR(m_pDocument->createCDATASection(bstrCDATA, &pCDATA));
        CHK_HR(AppendChildToParent(pCDATA, pParent));

    CleanUp:
        SAFE_RELEASE(pCDATA);
        SysFreeString(bstrCDATA);
        return hr;
    }

    // Helper function to create and append an element node to a parent node, and pass the newly created
    // element node to caller if it wants.
    HRESULT MscFile::CreateAndAddElementNode(PCWSTR wszName, IXMLDOMNode *pParent, IXMLDOMElement **ppElement)
    {
        HRESULT hr = S_OK;
        IXMLDOMElement* pElement = NULL;

        CHK_HR(CreateElement(wszName, &pElement));
        CHK_HR(AppendChildToParent(pElement, pParent));

    CleanUp:
        if (ppElement)
            *ppElement = pElement;  // Caller is repsonsible to release this element.
        else
            SAFE_RELEASE(pElement); // Caller is not interested on this element, so release it.

        return hr;
    }


LPWSTR
MscFile::ShowCmdToString(
    int nShowCmd)
{
    switch (nShowCmd)
    {
        case SW_HIDE:
            return (LPWSTR)L"SW_HIDE";

        case SW_SHOWNORMAL:
            return (LPWSTR)L"SW_SHOWNORMAL";

        case SW_SHOWMINIMIZED:
            return (LPWSTR)L"SW_SHOWMINIMIZED";

        case SW_SHOWMAXIMIZED:
            return (LPWSTR)L"SW_SHOWMAXIMIZED";

        case SW_SHOWNOACTIVATE:
            return (LPWSTR)L"SW_SHOWNOACTIVATE";

        case SW_SHOW:
            return (LPWSTR)L"SW_SHOW";

        case SW_MINIMIZE:
            return (LPWSTR)L"SW_MINIMIZE";
        
        case SW_SHOWMINNOACTIVE:
            return (LPWSTR)L"SW_SHOWMINNOACTIVE";
        
        case SW_SHOWNA:
            return (LPWSTR)L"SW_SHOWNA";

        case SW_RESTORE:
            return (LPWSTR)L"SW_RESTORE";

        case SW_SHOWDEFAULT:
            return (LPWSTR)L"SW_SHOWDEFAULT";

        case SW_FORCEMINIMIZE:
            return (LPWSTR)L"SW_FORCEMINIMIZE";
    }

    return (LPWSTR)L"-1";
}

HRESULT
MscFile::SaveWindowPlacement(CWindow *pWindow, IXMLDOMElement *pParentNode)
{
    IXMLDOMElement *pWindowPlacement = NULL;
    IXMLDOMElement *pPointElement = NULL;
    IXMLDOMElement *pRectangleElement = NULL;
    WINDOWPLACEMENT wndpl;
    WCHAR szBuffer[32];
    HRESULT hr = S_OK;

    pWindow->GetWindowPlacement(&wndpl);

    CHK_HR(CreateAndAddElementNode(L"WindowPlacement", pParentNode, &pWindowPlacement));
    CHK_HR(CreateAndAddAttributeNode(L"ShowCommand", ShowCmdToString(wndpl.showCmd), pWindowPlacement));

    CHK_HR(CreateAndAddElementNode(L"Point", pWindowPlacement, &pPointElement));
    CHK_HR(CreateAndAddAttributeNode(L"Name", L"MinPosition", pPointElement));
    _swprintf(szBuffer, L"%ld", wndpl.ptMinPosition.x);
    CHK_HR(CreateAndAddAttributeNode(L"X", szBuffer, pPointElement));
    _swprintf(szBuffer, L"%ld", wndpl.ptMinPosition.y);
    CHK_HR(CreateAndAddAttributeNode(L"Y", szBuffer, pPointElement));
    SAFE_RELEASE(pPointElement);

    CHK_HR(CreateAndAddElementNode(L"Point", pWindowPlacement, &pPointElement));
    CHK_HR(CreateAndAddAttributeNode(L"Name", L"MaxPosition", pPointElement));
    _swprintf(szBuffer, L"%ld", wndpl.ptMaxPosition.x);
    CHK_HR(CreateAndAddAttributeNode(L"X", szBuffer, pPointElement));
    _swprintf(szBuffer, L"%ld", wndpl.ptMaxPosition.y);
    CHK_HR(CreateAndAddAttributeNode(L"Y", szBuffer, pPointElement));
    SAFE_RELEASE(pPointElement);

    /* <Rectangle Name="NormalPosition" Top="0" Bottom="508" Left="0" Right="1186"/> */
    CHK_HR(CreateAndAddElementNode(L"Rectangle", pWindowPlacement, &pRectangleElement));
    CHK_HR(CreateAndAddAttributeNode(L"Name", L"NormalPosition", pRectangleElement));
    _swprintf(szBuffer, L"%ld", wndpl.rcNormalPosition.top);
    CHK_HR(CreateAndAddAttributeNode(L"Top", szBuffer, pRectangleElement));
    _swprintf(szBuffer, L"%ld", wndpl.rcNormalPosition.bottom);
    CHK_HR(CreateAndAddAttributeNode(L"Bottom", szBuffer, pRectangleElement));
    _swprintf(szBuffer, L"%ld", wndpl.rcNormalPosition.left);
    CHK_HR(CreateAndAddAttributeNode(L"Left", szBuffer, pRectangleElement));
    _swprintf(szBuffer, L"%ld", wndpl.rcNormalPosition.right);
    CHK_HR(CreateAndAddAttributeNode(L"Right", szBuffer, pRectangleElement));
    SAFE_RELEASE(pRectangleElement);

CleanUp:
    SAFE_RELEASE(pWindowPlacement);

    return hr;
}

HRESULT
MscFile::LoadDOM()
{
    IXMLDOMParseError *pXMLErr = NULL;
    BSTR bstrErr = NULL;
    VARIANT_BOOL varStatus;
    HRESULT hr = S_OK;

    hr = m_pDocument->load(m_varFileName, &varStatus);
    if (varStatus != VARIANT_TRUE)
    {
        // Failed to load xml, get last parsing error
        CHK_HR(m_pDocument->get_parseError(&pXMLErr));
        CHK_HR(pXMLErr->get_reason(&bstrErr));
    }

CleanUp:
    SAFE_RELEASE(pXMLErr);
    SysFreeString(bstrErr);

    return hr;
}

HRESULT
MscFile::SaveDOM()
{
    return m_pDocument->save(m_varFileName);
}
