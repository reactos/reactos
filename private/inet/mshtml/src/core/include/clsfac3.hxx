//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       clsfac3.hxx
//
//  Contents:   IClassFactoryEx support
//
//----------------------------------------------------------------------------

#ifndef I_CLSFAC3_HXX_
#define I_CLSFAC3_HXX_
#pragma INCMSG("--- Beg 'clsfac3.hxx'")

//+---------------------------------------------------------------------------
//
//  Class:      CStaticCF3
//
//  Purpose:    Implementation of IClassFactoryEx (for scriptoid handlers)
//              object declared as a static variable.  The implementation
//              of Release does not call delete.
//
//              To use this class, declare a variable of type CStaticCF3
//              and initialize it with an instance creation function and
//              and optional DWORD context.  The instance creation function
//              is of type FNCREATE defined below.
//
//+---------------------------------------------------------------------------

class CStaticCF3 : public CClassFactory
{
#ifdef UNIX //IEUNIX needs this to resolve PFNTEAROFF
    DECLARE_CLASS_TYPES(CStaticCF3, CClassFactory)
#endif

public:
    typedef HRESULT (FNCREATE3)(
            IUnknown *punkContext,  // context for instanciation
            IUnknown *pUnkOuter,    // pUnkOuter for aggregation
            IUnknown **ppUnkObj);   // the created object.

    CStaticCF3(FNCREATE3 *pfnCreate)
            { _pfnCreate = pfnCreate; }

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppv);

    // IClassFactory methods
    STDMETHOD(CreateInstance)(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);

    // IClassFactoryEx methods
    DECLARE_TEAROFF_TABLE(IClassFactoryEx)

    NV_DECLARE_TEAROFF_METHOD(CreateInstanceWithContext,
                              createinstancewithcontext,
                              (IUnknown *punkContext,
                               IUnknown *pUnkOuter,
                               REFIID iid,
                               void **ppvObj));

protected:
    FNCREATE3 *_pfnCreate;
};

#pragma INCMSG("--- End 'clsfac3.hxx'")
#else
#pragma INCMSG("*** Dup 'clsfac3.hxx'")
#endif
