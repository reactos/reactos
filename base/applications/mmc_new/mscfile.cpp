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
HRESULT
MscFile::CreateAndAddCommentNode(PCWSTR wszComment)
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
HRESULT
MscFile::CreateAndAddAttributeNode(PCWSTR wszName, PCWSTR wszValue, IXMLDOMElement *pParent)
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
HRESULT
MscFile::CreateAndAddTextNode(PCWSTR wszText, IXMLDOMNode *pParent)
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
HRESULT
MscFile::CreateAndAddCDATANode(PCWSTR wszCDATA, IXMLDOMNode *pParent)
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
HRESULT
MscFile::CreateAndAddElementNode(PCWSTR wszName, IXMLDOMNode *pParent, IXMLDOMElement **ppElement)
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
    CHK_HR(CreateAndAddAttributeNode(L"ShowCommand", ::ShowCmdToString(wndpl.showCmd), pWindowPlacement));

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
MscFile::ParseWindowPlacement(CWindow *pWindow, IXMLDOMElement *pParentElement)
{
    IXMLDOMElement *pWindowPlacementElement = NULL;
    IXMLDOMNodeList *pChildList = NULL;
    IXMLDOMNode *pNextElement;
    DOMNodeType NodeType;
    BSTR NodeName = NULL;
    VARIANT ShowCommand;
    VARIANT NameValue;
    WINDOWPLACEMENT wndpl;
    HRESULT hr = S_OK;

    ZeroMemory(&wndpl, sizeof(wndpl));
    wndpl.length = sizeof(wndpl);
    wndpl.flags = 0;

    hr = GetElement(pParentElement, (LPWSTR)L"WindowPlacement", &pWindowPlacementElement);
    if (hr != S_OK)
        goto done;

    VariantInit(&ShowCommand);
    hr = GetAttribute(pWindowPlacementElement, (LPWSTR)L"ShowCommand", &ShowCommand);
    if (hr != S_OK)
        goto done;

    wndpl.showCmd = ShowCmdFromString(V_BSTR(&ShowCommand));

    VariantClear(&ShowCommand);

    hr = pWindowPlacementElement->get_childNodes(&pChildList);
    if (FAILED(hr))
        goto done;

    for (;;)
    {
        hr = pChildList->nextNode(&pNextElement);
        if (hr != S_OK)
            break;

        hr = pNextElement->get_nodeType(&NodeType);
        if (hr == S_OK)
        {
            if (NodeType == NODE_ELEMENT)
            {
                NodeName = NULL;
                hr = pNextElement->get_baseName(&NodeName);
                if (hr == S_OK)
                {
                    VariantInit(&NameValue);

                    hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Name", &NameValue);
                    if (hr == S_OK)
                    {
                        if (_wcsicmp(NodeName, L"Point") == 0)
                        {
                            VARIANT PosX, PosY;
                            VariantInit(&PosX);
                            VariantInit(&PosY);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"X", &PosX);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Y", &PosY);

                            if (_wcsicmp(V_BSTR(&NameValue), L"MinPosition") == 0)
                            {
                                wndpl.ptMinPosition.x = _wtol(V_BSTR(&PosX));
                                wndpl.ptMinPosition.y = _wtol(V_BSTR(&PosY));
                            }
                            else if (_wcsicmp(V_BSTR(&NameValue), L"MaxPosition") == 0)
                            {
                                wndpl.ptMaxPosition.x = _wtol(V_BSTR(&PosX));
                                wndpl.ptMaxPosition.y = _wtol(V_BSTR(&PosY));
                            }
                            VariantClear(&PosX);
                            VariantClear(&PosY);
                        }
                        else if (_wcsicmp(NodeName, L"Rectangle") == 0)
                        {
                            VARIANT PosTop, PosBottom, PosLeft, PosRight;
                            VariantInit(&PosTop);
                            VariantInit(&PosBottom);
                            VariantInit(&PosLeft);
                            VariantInit(&PosRight);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Top", &PosTop);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Bottom", &PosBottom);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Left", &PosLeft);
                            hr = GetAttribute((IXMLDOMElement *)pNextElement, (LPWSTR)L"Right", &PosRight);
                            if (_wcsicmp(V_BSTR(&NameValue), L"NormalPosition") == 0)
                            {
                                wndpl.rcNormalPosition.top = _wtol(V_BSTR(&PosTop));
                                wndpl.rcNormalPosition.bottom = _wtol(V_BSTR(&PosBottom));
                                wndpl.rcNormalPosition.left = _wtol(V_BSTR(&PosLeft));
                                wndpl.rcNormalPosition.right = _wtol(V_BSTR(&PosRight));
                            }
                            VariantClear(&PosTop);
                            VariantClear(&PosBottom);
                            VariantClear(&PosLeft);
                            VariantClear(&PosRight);
                        }

                        VariantClear(&NameValue);
                    }

                    SysFreeString(NodeName);
                    NodeName = NULL;
                }
            }
        }

        pNextElement->Release();
    }

    pWindow->SetWindowPlacement(&wndpl);

done:
    if (pChildList)
        pChildList->Release();

    if (pWindowPlacementElement)
        pWindowPlacementElement->Release();

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

HRESULT
MscFile::CheckMscFile(IXMLDOMElement **ppRootElement)
{
    IXMLDOMElement *pRootElement = NULL;
    BSTR rootName = NULL;
    VARIANT version;
    HRESULT hr = S_OK;

    *ppRootElement = NULL;

    VariantInit(&version);

    hr = m_pDocument->get_documentElement(&pRootElement);
    if (FAILED(hr))
        goto done;

    hr = pRootElement->get_baseName(&rootName);
    if (FAILED(hr))
        goto done;

    if (_wcsicmp(rootName, L"MMC_ConsoleFile"))
    {
        hr = S_FALSE;
        goto done;
    }

    hr = GetAttribute(pRootElement, (LPWSTR)L"ConsoleVersion", &version);
    if (FAILED(hr))
        goto done;

    if (V_VT(&version) == VT_NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    if (wcscmp(V_BSTR(&version), L"2.0") &&
        wcscmp(V_BSTR(&version), L"3.0"))
    {
        hr = S_FALSE;
        goto done;
    }

    *ppRootElement = pRootElement;

done:
    VariantClear(&version);

    if (rootName)
        SysFreeString(rootName);

    if ((hr != S_OK) && pRootElement != NULL)
        pRootElement->Release();

    return hr;
}

HRESULT
MscFile::GetAttribute(
    IXMLDOMElement *pElement,
    LPWSTR pszAttributeName,
    VARIANT *pAttributeValue)
{
    BSTR bstrAttribute = NULL;
    HRESULT hr = S_OK;

    bstrAttribute = SysAllocString(pszAttributeName);
    if (bstrAttribute == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = pElement->getAttribute(bstrAttribute, pAttributeValue); 

done:
    if (bstrAttribute)
        SysFreeString(bstrAttribute);

    return hr;
}

HRESULT
MscFile::GetElement(
    IXMLDOMElement *pParentElement,
    LPWSTR pszElementName,
    IXMLDOMElement **ppElement)
{
    IXMLDOMNodeList *pChildList = NULL;
    IXMLDOMNode *nextItem;
    DOMNodeType NodeType;
    BSTR NodeName = NULL;
    HRESULT hr = S_OK;

    *ppElement = NULL;

    hr = pParentElement->get_childNodes(&pChildList);
    if (FAILED(hr))
        goto done;

    for (;;)
    {
        hr = pChildList->nextNode(&nextItem);
        if (hr != S_OK)
            break;

        hr = nextItem->get_nodeType(&NodeType);
        if (hr == S_OK)
        {
            if (NodeType == NODE_ELEMENT)
            {
                NodeName = NULL;
                hr = nextItem->get_baseName(&NodeName);
                if (hr == S_OK)
                {
                    if (_wcsicmp(NodeName, pszElementName) == 0)
                    {
                        *ppElement = (IXMLDOMElement *)nextItem;
                        break;
                    }
                    SysFreeString(NodeName);
                    NodeName = NULL;
                }
            }
        }

        nextItem->Release();
    }

done:
    if (NodeName)
        SysFreeString(NodeName);

    if (pChildList)
        pChildList->Release();

    return hr;
}

VOID
MscFile::ReleaseNode(IXMLDOMNode *pNode)
{
    if (pNode)
    {
        pNode->Release();
        pNode = NULL;
    }
}
