/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
    
// XMLDocument.h : defines global function for creating new IXMLDocuments.

#ifndef __XMLDOCUMENT_H_
#define __XMLDOCUMENT_H_

#ifndef __msxml_h__
#include    "msxml.h"
#endif

extern DLLEXPORT HRESULT __stdcall CreateDocument(REFIID iid, void **ppvObj);

extern DLLEXPORT HRESULT __stdcall CreateDOMDocument(REFIID iid, void **ppvObj);

extern DLLEXPORT HRESULT __stdcall CreateXMLIslandPeer(REFIID iid, void **ppvObj);

extern DLLEXPORT HRESULT __stdcall CreateXMLScriptIsland(REFIID iid, void **ppvObj);

#endif //__XMLDOCUMENT_H_
