//+----------------------------------------------------------------------------
//  File:   licmgrid.hxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


#ifndef	_LICMGRID_HXX
#define	_LICMGRID_HXX

// CLSID_LicenseManager {5220CB21-C88D-11cf-B347-00AA00A28331}
DEFINE_GUID(CLSID_LicenseManager, 0x5220cb21, 0xc88d, 0x11cf, 0xb3, 0x47, 0x00, 0xaa, 0x00, 0xa2, 0x83, 0x31);

// UUID_LPKVersion1 {3D25ABA1-CAEC-11cf-B34A-00AA00A28331}
DEFINE_GUID(UUID_LPKVersion1, 0x3d25aba1, 0xcaec, 0x11cf, 0xb3, 0x4a, 0x00, 0xaa, 0x00, 0xa2, 0x83, 0x31);

// SID_SLocalRegistry/IID_ILocalRegistry { 6d5140d3-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILocalRegistry, 0x6d5140d3, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLocalRegistry IID_ILocalRegistry

#undef  INTERFACE
#define INTERFACE  ILocalRegistry
DECLARE_INTERFACE_(ILocalRegistry, IUnknown)
{
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

// IID_IRequireClasses { 6d5140d0-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IRequireClasses, 0x6d5140d0, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IRequireClasses
DECLARE_INTERFACE_(IRequireClasses, IUnknown)
{
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

// SID_SLicensedClassManager/IID_ILicensedClassManager { 6d5140d4-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ILicensedClassManager, 0x6d5140d4, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SLicensedClassManager  IID_ILicensedClassManager

#undef  INTERFACE
#define INTERFACE  ILicensedClassManager
DECLARE_INTERFACE_(ILicensedClassManager, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ILicensedClassManager methods ***
    STDMETHOD(OnChangeInRequiredClasses)(THIS_
                     /* [in] */ IRequireClasses *pRequireClasses) PURE;
};


#endif // _LICMGRID_HXX
