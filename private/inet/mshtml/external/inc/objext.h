#ifndef UNIX
#pragma pack(push, 8)
#endif 
//+------------------------------------------------------------------------
//  
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//  
//  File:       objext.h
//  
//  Contents:   header file for Object Extensions interfaces
//  
//              6/24/96 (dbau) removed IServiceProvider (to servprov.h)
//-------------------------------------------------------------------------

#ifndef __OBJEXT_H
#define __OBJEXT_H

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

///////////////////////////////////////////////////////////////////////////
//
// forward declares
//
///////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
interface IFilterMembers;
#else  // __cplusplus
typedef interface IFilterMembers IFilterMembers;
#endif  // __cplusplus

///////////////////////////////////////////////////////////////////////////
//
// Object Extension Interfaces
//
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
//
// Standard Services and Interfaces
//
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
//  SLicensedClassManager
//    VBA provides this service to it's components and hosts to optimize
//    registry access and to insulate them from licensing concerns
//
//  interfaces implemented:
//    ILicensedClassManager
//-------------------------------------------------------------------------
// { 6d5140d0-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IRequireClasses, 0x6d5140d0, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IRequireClasses
DECLARE_INTERFACE_(IRequireClasses, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IRequireClasses methods ***
    STDMETHOD(CountRequiredClasses)(THIS_
                    /* [out] */ ULONG * pcClasses ) PURE;
    STDMETHOD(GetRequiredClasses)(THIS_
                  /* [in]  */ ULONG index,
                  /* [out] */ CLSID * pclsid ) PURE;
};

// { 6d5140d4-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILicensedClassManager, 0x6d5140d4, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLicensedClassManager  IID_ILicensedClassManager

#undef  INTERFACE
#define INTERFACE  ILicensedClassManager
DECLARE_INTERFACE_(ILicensedClassManager, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ILicensedClassManager methods ***
    STDMETHOD(OnChangeInRequiredClasses)(THIS_
                     /* [in] */ IRequireClasses *pRequireClasses) PURE;
};


//-------------------------------------------------------------------------
//  SLocalRegistry Service
//    VBA provides this service to it's components and hosts to optimize
//    registry access and to insulate them from licensing concerns
//
//  interfaces implemented:
//    ILocalRegistry
//-------------------------------------------------------------------------

// { 6d5140d3-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILocalRegistry, 0x6d5140d3, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLocalRegistry IID_ILocalRegistry

#undef  INTERFACE
#define INTERFACE  ILocalRegistry
DECLARE_INTERFACE_(ILocalRegistry, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ILocalRegistry methods ***
    STDMETHOD(CreateInstance)(THIS_
                  /* [in]  */ CLSID      clsid,     
                  /* [in]  */ IUnknown * punkOuter,
                  /* [in]  */ REFIID     riid,
                  /* [in]  */ DWORD      dwFlags,
                  /* [out] */ void **    ppvObj ) PURE;
    STDMETHOD(GetTypeLibOfClsid)(THIS_
                 /* [in]  */ CLSID       clsid,
                 /* [out] */ ITypeLib ** ptlib ) PURE;
    STDMETHOD(GetClassObjectOfClsid)(THIS_
                     /* [in]  */ REFCLSID clsid,
                                 /* [in]  */ DWORD    dwClsCtx,
                     /* [in]  */ LPVOID   lpReserved,
                     /* [in]  */ REFIID   riid,
                     /* [out] */ void **  ppcClassObject ) PURE;
};


//-------------------------------------------------------------------------
//
//      SProfferService
//      IProfferService
//
//-------------------------------------------------------------------------

DEFINE_GUID(IID_IProfferService, 0xcb728b20, 0xf786, 0x11ce, 0x92, 0xad, 0x0, 0xaa, 0x0, 0xa7, 0x4c, 0xd0);
#define SID_SProfferService IID_IProfferService

#undef  INTERFACE
#define INTERFACE  IProfferService
DECLARE_INTERFACE_(IProfferService, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProfferService methods ***
    STDMETHOD(ProfferService)(THIS_ 
                  /* [in]  */ REFGUID rguidService,
                  /* [in]  */ IServiceProvider * psp,
                  /* [out] */ DWORD *pdwCookie) PURE;

    STDMETHOD(RevokeService)(THIS_ /* [in]  */ DWORD dwCookie) PURE;
};



#endif // __OBJEXT_H

#ifndef UNIX
#pragma pack(pop)
#endif 
