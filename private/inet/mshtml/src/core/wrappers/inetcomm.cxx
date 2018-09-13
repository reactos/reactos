//+---------------------------------------------------------------------------
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       inetcomm.cxx
//
//  Contents:   Dynamic wrappers for INETCOMM.DLL.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_WINCRYPT_H_
#define X_WINCRYPT_H_
#include "wincrypt.h"
#endif

// #define _WIN32_OE  0x0500 // BUGBUG: until this is globally visible - and only if your want MHTML
#ifndef X_MIMEOLE_H_
#define X_MIMEOLE_H_
#define _MIMEOLE_   // To avoid having DECLSPEC_IMPORT
#include "mimeole.h"
#endif

// Turn on DEFINE_GUID
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
  
// {35461E30-C488-11d1-960E-00C04FBD7C09}
DEFINE_GUID(CLSID_IMimeObjResolver, 0x35461e30, 0xc488, 0x11d1, 0x96, 0xe, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

// {FECEAFFD-C441-11d1-960E-00C04FBD7C09}
DEFINE_GUID(IID_IMimeObjResolver, 0xfeceaffd, 0xc441, 0x11d1, 0x96, 0xe, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

DYNLIB g_dynlibINETCOMM = { NULL, NULL, "INETCOMM.DLL" };        

//+---------------------------------------------------------------------------
//
//  Function:   MimeOleObjectFromMoniker
//
//  Synopsis:   Wraps either IMimeObjResolver.MimeOleObjectFromMoniker or 
//              imported function MimeOleObjectFromMoniker from INETCOMM.DLL
//              First, we attempt to cocreate an IMimeObjResolver. If this
//              fails, we try to call the INETCOMM.DLL directly. If this
//              fails, all hell will break loose.
//
//----------------------------------------------------------------------------

HRESULT WINAPI  MimeOleObjectFromMoniker(
            /* in */        BINDF               bindf,
            /* in */        IMoniker            *pmkOriginal,
            /* in */        IBindCtx            *pBindCtx,
            /* in */        REFIID              riid, 
            /* out */       LPVOID              *ppvObject,
            /* out */       IMoniker            **ppmkNew )
{
    HRESULT hr = E_FAIL;

    // Load IMimeObjResolver from CoCreateInstance
    IMimeObjResolver * pObjResolver = NULL;

    hr = THR( CoCreateInstance( CLSID_IMimeObjResolver, 
                                NULL, 
                                CLSCTX_INPROC_SERVER,
                                IID_IMimeObjResolver,
                                (void **) &pObjResolver ));

    if(! FAILED( hr ))
    {
        // If we sucessfully cocreated the object, dispatch to it
        hr = pObjResolver->MimeOleObjectFromMoniker( bindf, pmkOriginal, pBindCtx, riid, ppvObject, ppmkNew );
        pObjResolver->Release();
        pObjResolver = NULL;
    }
    else
    {
        // LoadLibrary and call the exported function directly
        static DYNPROC s_dynprocMOOFM = { NULL, &g_dynlibINETCOMM, "MimeOleObjectFromMoniker" };
        hr = THR( LoadProcedure( &s_dynprocMOOFM ));
        
        if (hr)
            goto Cleanup;
            
        // The following mess is defining, derefing, and calling a function pointer pfn                                    
        hr = THR((* ( HRESULT (WINAPI *) (  BINDF bindf, 
                                            IMoniker *pmkOriginal, 
                                            IBindCtx *pBindCtx, 
                                            REFIID riid, 
                                            LPVOID *ppvObject, 
                                            IMoniker **ppmkNew )        // api of fn ptr
                    ) s_dynprocMOOFM.pfn )( bindf,                      // call to fn ptr
                                            pmkOriginal, 
                                            pBindCtx, 
                                            riid, 
                                            ppvObject, 
                                            ppmkNew )  );
    }

Cleanup:
    RRETURN2( hr, S_FALSE, MK_S_ASYNCHRONOUS);
}

