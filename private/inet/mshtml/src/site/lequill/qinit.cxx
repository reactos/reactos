//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       qinit.cxx
//
//  Contents:   Initialization of Quill DLL.
//
//  Notes:      So far contains only initialization code.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QUILLSITE_H_
#define X_QUILLSITE_H_
#include "quillsite.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif


// REVIEW sidda: manually copied from Quill project
const CLSID CLSID_QuillStoriesManager = {0x9ADC4B9F,0xD46C,0x11cf,{0x9D,0x6E,0x00,0xA0,0xC9,0x11,0xC8,0xE0}};

DeclareTag(tagQuillLayout, "Quill Initialization", "Quill Initialization methods")

//+---------------------------------------------------------------------------
//
//  Function:     DeinitQuillLayout
//
//----------------------------------------------------------------------------

void 
DeinitQuillLayout()
{
	TraceTag((tagQuillLayout, "DeinitQuillLayout"));
	
	if (TLS(_pQLM))
        ClearInterface(&TLS(_pQLM));
}

//+---------------------------------------------------------------------------
//
//  Function:     InitQuillLayout
//
//----------------------------------------------------------------------------

// don't hold the global lock because InitScriptDebugging makes RPC calls (bug 26308)
static CCriticalSection g_csInitQuillLayout;

HRESULT 
InitQuillLayout()
{
   TraceTag((tagQuillLayout, "InitQuillLayout"));

   if (TLS(fQuillInitFailed) || TLS(_pQLM))
        return S_OK;

    HRESULT hr = S_OK;

    g_csInitQuillLayout.Enter();

    // Need to check again after locking the globals.
    if (TLS(fQuillInitFailed) || TLS(_pQLM))
        goto Cleanup;

    hr = CoCreateInstance(CLSID_QuillStoriesManager, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITextLayoutManager, (LPVOID FAR *) &TLS(_pQLM));

    if (hr)
        goto Cleanup;

#if DBG==1
    TLS(_pQLM)->CheckDLLVersion(qsverRup, NULL);
#endif

Cleanup:
    if (hr)
    {
        TLS(fQuillInitFailed) = TRUE;
        DeinitQuillLayout();
    }

    g_csInitQuillLayout.Leave();

    RRETURN(hr);
}

