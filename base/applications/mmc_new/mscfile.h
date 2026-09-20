/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Helper functions for MSXML3
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

// Macro that calls a COM method returning HRESULT value.
#define CHK_HR(stmt)        do { hr=(stmt); if (FAILED(hr)) goto CleanUp; } while(0)

// Macro to verify memory allcation.
#define CHK_ALLOC(p)        do { if (!(p)) { hr = E_OUTOFMEMORY; goto CleanUp; } } while(0)

// Macro that releases a COM object if not NULL.
#define SAFE_RELEASE(p)     do { if ((p)) { (p)->Release(); (p) = NULL; } } while(0)

class MscFile
{
private:
    VARIANT m_varFileName;
    IXMLDOMDocument *m_pDocument;

    HRESULT VariantFromString(PCWSTR wszValue, VARIANT &Variant);
    LPWSTR ShowCmdToString(int nShowCmd);

public:
    MscFile(PCWSTR pszFileName);
    ~MscFile();
    HRESULT CreateAndInitDOM();
    HRESULT CreateElement(PCWSTR wszName, IXMLDOMElement **ppElement);
    HRESULT AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent = NULL);
    HRESULT CreateAndAddPINode(PCWSTR wszTarget, PCWSTR wszData);
    HRESULT CreateAndAddCommentNode(PCWSTR wszComment);
    HRESULT CreateAndAddAttributeNode(PCWSTR wszName, PCWSTR wszValue, IXMLDOMElement *pParent);
    HRESULT CreateAndAddTextNode(PCWSTR wszText, IXMLDOMNode *pParent);
    HRESULT CreateAndAddCDATANode(PCWSTR wszCDATA, IXMLDOMNode *pParent);
    HRESULT CreateAndAddElementNode(PCWSTR wszName, IXMLDOMNode *pParent, IXMLDOMElement **ppElement = NULL);

    HRESULT SaveWindowPlacement(CWindow *pWindow, IXMLDOMElement *pParentNode);

    HRESULT LoadDOM();
    HRESULT SaveDOM();
};
