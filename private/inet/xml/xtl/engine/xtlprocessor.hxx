/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef XTL_ENGINE_XTLPROCESSOR
#define XTL_ENGINE_XTLPROCESSOR

class NOVTABLE IXTLProcessor : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Init(IXMLDOMNode *pStyleSheet, IXMLDOMNode *pNode, IStream * pstm) = 0;
    virtual HRESULT STDMETHODCALLTYPE Execute(BSTR *errMsg) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;
};

HRESULT CreateXTLProcessor(IXTLProcessor ** ppxtl);
#endif