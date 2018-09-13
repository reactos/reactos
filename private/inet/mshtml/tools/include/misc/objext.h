#pragma pack(push, 8) 
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


//-------------------------------------------------------------------------
//  IClassDesigner Interface
//    This interface is implemented by a class object that can be customized
//
//-------------------------------------------------------------------------

// { 6d5140d3-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IClassDesigner, 0x6d5140d3, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

typedef DWORD ACTFLAG;
#define ACT_DEFAULT 0x00000000
#define ACT_SHOW    0x00000001

#undef  INTERFACE
#define INTERFACE  IClassDesigner
DECLARE_INTERFACE_(IClassDesigner, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IClassDesigner methods ***
    STDMETHOD(SetSite)(THIS_
               /* [in]  */ IServiceProvider * pSP) PURE;
    STDMETHOD(GetSite)(THIS_
               /* [out] */ IServiceProvider** ppSP) PURE;
    STDMETHOD(GetCompiler)(THIS_
               /* [in]  */ REFIID iid,
               /* [out] */ void **ppvObj) PURE;
    STDMETHOD(ActivateObject)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(IsObjectShowable)(THIS) PURE;
    STDMETHOD(GetExtensibilityObject)(THIS_ 
                      /* [out] */ IDispatch ** ppDisp) PURE;
};

//-------------------------------------------------------------------------
//  IProvideUnmergedClassInfo Interface
//    This interface is implemented by an object that is composed of two
//    objects. This interface is used to get the typeinfo of the two objects.
//
//-------------------------------------------------------------------------
// { 6d5140da-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IProvideUnmergedClassInfo, 0x6d5140da, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IProvideUnmergedClassInfo
DECLARE_INTERFACE_(IProvideUnmergedClassInfo, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProvideUnmergedClassInfo methods ***
    STDMETHOD(GetClassInfos)(THIS_
                 /* [out] */ ITypeInfo **pptinfoBase, 
                 /* [out] */ ITypeInfo **pptinfoExtender,
		 /* [out] */ DWORD *pdwCookie) PURE;
};

//UNDONE UNDONE: rip this
// { 6d5140dd-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IProvideUnmergedClassInfoOLD, 0x6d5140dd, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IProvideUnmergedClassInfoOLD
DECLARE_INTERFACE_(IProvideUnmergedClassInfoOLD, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProvideUnmergedClassInfo methods ***
    STDMETHOD(GetClassInfos)(THIS_
                 /* [out] */ ITypeInfo **pptinfoBase, 
                 /* [out] */ ITypeInfo **pptinfoExtender) PURE;
};


//-------------------------------------------------------------------------
//  IUnmergedClassFactory Interface
//    This interface is implemented by an object that is composed of two
//    objects. This interface is used to create instances of the two objects.
//
//-------------------------------------------------------------------------
// { 6d5140d5-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IUnmergedClassFactory, 0x6d5140d5, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IUnmergedClassFactory
DECLARE_INTERFACE_(IUnmergedClassFactory, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IUnmergedClassFactory methods ***
    STDMETHOD(CreateInstance)(THIS_
                  /* [in]  */ IUnknown *punkOuter,
                  /* [out] */ IUnknown **ppunkBase, 
                  /* [out] */ IUnknown **ppunkExtender) PURE;
};

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
//  IFilterMembers
//    This interface is used during processing of typeinfos
//-------------------------------------------------------------------------
// {49F21D20-1870-11cf-80E8-00AA004BA1C8}
DEFINE_GUID(IID_IFilterMembers,0x49f21d20L, 0x1870, 0x11cf, 0x80, 0xe8, 0x00, 0xaa, 0x00, 0x4b, 0xa1, 0xc8);

// used in IFilterMembers indicating the passed in ptinfo is a source or not
#define FILTERMEMBERS_FSOURCE       0x0001

// used in IFilterMembers indicating the passed in ptinfo is an extender or not
#define FILTERMEMBERS_FEXTENDER     0x0002

#undef  INTERFACE
#define INTERFACE IFilterMembers
DECLARE_INTERFACE_(IFilterMembers, IUnknown)
{
    BEGIN_INTERFACE
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IVbaProvideStorage methods
    STDMETHOD(FilterVarDesc)(THIS_ DWORD dwFlags,
                             ITypeInfo *ptinfo,
                             UINT index,
                             VARDESC *pvardesc,
                             DWORD dwReserved) PURE;
    STDMETHOD(FilterFuncDesc)(THIS_ DWORD dwFlags,
                             ITypeInfo *ptinfo,
                             UINT index,
                             FUNCDESC *pfuncdesc,
                             DWORD dwReserved) PURE;
    STDMETHOD(BeginMerge)(THIS_ DWORD dwFlags,
                          ITypeInfo *ptinfo) PURE;
    STDMETHOD(EndMerge)(THIS) PURE;
    STDMETHOD(UpdateMergedCoClass)(THIS_ ICreateTypeInfo *pictinfo) PURE;
};

//-------------------------------------------------------------------------
//  SCreateExtendedTypeLib Service
//    This service is used by components to create a typelib
//    describing controls merged with their extender
//
//  interfaces implemented:
//    ICreateExtendedTypeLib
//-------------------------------------------------------------------------
// { 6d5140d6-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IExtendedTypeLib, 0x6d5140d6, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SExtendedTypeLib IID_IExtendedTypeLib

#undef  INTERFACE
#define INTERFACE  IExtendedTypeLib
DECLARE_INTERFACE_(IExtendedTypeLib, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IExtendedTypeLib ***
    STDMETHOD(CreateExtendedTypeLib)(THIS_
                     /* [in]  */ LPCOLESTR lpstrCtrlLibFileName,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ IFilterMembers *pfilter,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeLib  **pptlib) PURE;

    STDMETHOD(AddRefExtendedTypeLib)(THIS_
                     /* [in]  */ LPCOLESTR lpstrCtrlLibFileName,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ IFilterMembers *pfilter,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeLib  **pptlib) PURE;
    STDMETHOD(AddRefExtendedTypeLibOfClsid)(THIS_
                     /* [in]  */ REFCLSID rclsidControl,
                     /* [in]  */ LPCOLESTR lpstrLibNamePrepend,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ IFilterMembers *pfilter,
                     /* [in]  */ DWORD     dwFlags,
                     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [out] */ ITypeInfo **pptinfo) PURE;
    STDMETHOD(SetExtenderInfo)(THIS_ 
		     /* [in]  */ LPCOLESTR lpstrDirectoryName,
                     /* [in]  */ ITypeInfo *ptinfoExtender,
                     /* [in]  */ IFilterMembers *pfilter) PURE;
};

//-------------------------------------------------------------------------
//  SCreateExtension Service
//    This service is used by Instance customized objects to 
//    create the VBA extension
//
//  interfaces implemented:
//    ICreateExtension
//-------------------------------------------------------------------------

// { 6d5140d2-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ICreateExtension, 0x6d5140d2, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SCreateExtension IID_ICreateExtension

#undef  INTERFACE
#define INTERFACE  ICreateExtension
DECLARE_INTERFACE_(ICreateExtension, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICreateExtension methods ***
    STDMETHOD(CreateExtension)(THIS_
                   /* [in]  */ IUnknown *punkOuter,
                   /* [in]  */ IUnknown *punkBase,
                   /* [in]  */ IUnknown *punkExtender,
                   /* [out] */ IUnknown **ppunkExtension) PURE;
};

//-------------------------------------------------------------------------
//  SCodeNavigate Service.
//    This service let's an extended object show the code module
//    behind it.
//
//  interfaces implemented:
//    ICodeNavigate
//-------------------------------------------------------------------------

// { 6d5140c4-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ICodeNavigate, 0x6d5140c4, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SCodeNavigate IID_ICodeNavigate

#undef  INTERFACE
#define INTERFACE  ICodeNavigate
DECLARE_INTERFACE_(ICodeNavigate, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICodeNavigate methods ***
    STDMETHOD(DisplayDefaultEventHandler)(THIS_ /* [in] */ LPCOLESTR lpstrObjectName) PURE;
};

//-------------------------------------------------------------------------
//  STrackSelection Service
//    This service is used by the VBA host to help VBA track the
//    currently selected object in the host
//
//  interfaces implemented:
//    ITrackSelection
//-------------------------------------------------------------------------
#define GETOBJS_ALL         1
#define GETOBJS_SELECTED    2

#define SELOBJS_ACTIVATE_WINDOW   1

// { 6d5140c6-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ISelectionContainer, 0x6d5140c6, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  ISelectionContainer
DECLARE_INTERFACE_(ISelectionContainer, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ISelectionContainer methods ***
    STDMETHOD(CountObjects)(THIS_
                /* [in]  */ DWORD dwFlags, 
                /* [out] */ ULONG * pc) PURE;
    STDMETHOD(GetObjects)(THIS_
              /* [in]  */ DWORD dwFlags, 
              /* [in]  */ ULONG cObjects,
              /* [out] */ IUnknown **apUnkObjects) PURE;
    STDMETHOD(SelectObjects)(THIS_
              /* [in] */ ULONG cSelect,
              /* [in] */ IUnknown **apUnkSelect,
              /* [in] */ DWORD dwFlags) PURE;
};

// { 6d5140c5-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_ITrackSelection, 0x6d5140c5, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_STrackSelection IID_ITrackSelection

#undef  INTERFACE
#define INTERFACE  ITrackSelection
DECLARE_INTERFACE_(ITrackSelection, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ITrackSelection methods ***
    STDMETHOD(OnSelectChange)(THIS_ 
                  /* [in] */ ISelectionContainer * pSC) PURE;
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
//  IUIElement interface
//    components can implement services to allow external control of pieces 
//    of their UI by implementing this interface
//
//-------------------------------------------------------------------------
// { 759d0500-d979-11ce-84ec-00aa00614f3e }
DEFINE_GUID(IID_IUIElement, 0x759d0500, 0xd979, 0x11ce, 0x84, 0xec, 0x00, 0xaa, 0x00, 0x61, 0x4f, 0x3e);

#undef  INTERFACE
#define INTERFACE  IUIElement
DECLARE_INTERFACE_(IUIElement, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ****
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IUIElement methods ****
    STDMETHOD(Show)(THIS) PURE;
    STDMETHOD(Hide)(THIS) PURE;
    STDMETHOD(IsVisible)(THIS) PURE;
};

//-------------------------------------------------------------------------
//  SProfferService Service
//    VBA provides this service to it's components and hosts to allow
//    them to dynamically provide services.
//
//  interfaces implemented:
//    IProfferService
//-------------------------------------------------------------------------

// {CB728B20-F786-11ce-92AD-00AA00A74CD0}
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

//-------------------------------------------------------------------------
//  SProfferTypelib Service
//    VBA provides this service to it's components and hosts to allow
//    them to add typelibs to the project
//
//  interfaces implemented:
//    IProfferTypelib
//-------------------------------------------------------------------------

// { 718cc500-0a76-11cf-8045-00aa006009fa }
DEFINE_GUID(IID_IProfferTypeLib, 0x718cc500, 0x0A76, 0x11cf, 0x80, 0x45, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
#define SID_SProfferTypeLib IID_IProfferTypeLib

#define CONTROLTYPELIB	(0x00000001)

#undef  INTERFACE
#define INTERFACE  IProfferTypeLib
DECLARE_INTERFACE_(IProfferTypeLib, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProfferTypelib methods ***
    STDMETHOD(ProfferTypeLib)(THIS_ 
              /* [in]  */ REFGUID guidTypeLib,
              /* [in]  */ UINT    uVerMaj,
              /* [in]  */ UINT    uVerMin,
              /* [in]  */ DWORD   dwFlags) PURE;
};

// UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE
//   These interfaces need to be moved to the new olectl.h
//
// UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE UNDONE

// { 6d5140c0-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IGangConnectWithDefault, 0x6d5140c0, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IGangConnectWithDefault
DECLARE_INTERFACE_(IGangConnectWithDefault, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IGangConnectWithDefault ***
    STDMETHOD(AdviseWithDefault)(THIS_
            ULONG   cSinks, 
            DISPID *    adispid,
            IUnknown ** apUnkSink,
            IUnknown ** apUnkDefaultBO) PURE;
};

// { 6d5140d1-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IProvideDynamicClassInfo, 0x6d5140d1, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IProvideDynamicClassInfo
DECLARE_INTERFACE_(IProvideDynamicClassInfo, IProvideClassInfo)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IProvideDynamicClassInfo ***
    STDMETHOD(GetDynamicClassInfo_RIP)(THIS_ ITypeInfo ** ppTI) PURE;
    STDMETHOD(GetDynamicClassInfo)(THIS_ ITypeInfo ** ppTI, DWORD * pdwCookie) PURE;
    STDMETHOD(FreezeShape)(void) PURE;
};



// {4D07FC10-F931-11ce-B001-00AA006884E5}
DEFINE_GUID(IID_ICategorizeProperties, 0x4d07fc10, 0xf931, 0x11ce, 0xb0, 0x1, 0x0, 0xaa, 0x0, 0x68, 0x84, 0xe5);

// NOTE : CATID should no longer be used.  Use PROPCAT instead.
// UNDONE,erikc,1/22/96 : remove #ifdef when all components have updated to new typedef.
#ifdef OBJEXT_OLD_CATID
typedef int CATID;
#else
typedef int PROPCAT;
#endif

#undef  INTERFACE
#define INTERFACE  ICategorizeProperties
DECLARE_INTERFACE_(ICategorizeProperties, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** ICategorizeProperties ***
    STDMETHOD(MapPropertyToCategory)(THIS_ 
                                     /* [in]  */ DISPID dispid,
                                     /* [out] */ PROPCAT* ppropcat) PURE;
    STDMETHOD(GetCategoryName)(THIS_
                               /* [in]  */ PROPCAT propcat, 
                               /* [in]  */ LCID lcid,
                               /* [out] */ BSTR* pbstrName) PURE;
};

typedef ICategorizeProperties FAR* LPCATEGORIZEPROPERTIES;

// category ID: negative values are 'standard' categories,  positive are control-specific
// Note! This is a temporary list!
#ifdef OBJEXT_OLD_CATID
// NOTE : The following #defines should no longer be used.  Use PROPCAT_ instead.
// UNDONE,erikc,1/22/96 : remove #ifdef when all components have updated to new #defines.
#define CI_Nil -1
#define CI_Misc -2
#define CI_Font -3
#define CI_Position -4
#define CI_Appearance -5
#define CI_Behavior -6
#define CI_Data -7
#define CI_List -8
#define CI_Text -9
#define CI_Scale -10
#define CI_DDE -11
#else
#define PROPCAT_Nil -1
#define PROPCAT_Misc -2
#define PROPCAT_Font -3
#define PROPCAT_Position -4
#define PROPCAT_Appearance -5
#define PROPCAT_Behavior -6
#define PROPCAT_Data -7
#define PROPCAT_List -8
#define PROPCAT_Text -9
#define PROPCAT_Scale -10
#define PROPCAT_DDE -11
#endif

//
//  Extra interfaces (chrisz)
//

//+-------------------------------------------------------------------------
//
//  Help service. (robbear)
//
//--------------------------------------------------------------------------

#define HELPINFO_WHATS_THIS_MODE_ON     1

// { 6d5140c7-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(SID_SHelp, 0x6d5140c7, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

// { 6d5140c8-7436-11ce-8034-00aa006009fa }
DEFINE_GUID(IID_IHelp, 0x6d5140c8, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);

#undef  INTERFACE
#define INTERFACE  IHelp
DECLARE_INTERFACE_(IHelp, IUnknown)
{
    BEGIN_INTERFACE
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IHelp methods ***
    STDMETHOD(GetHelpFile) (THIS_ BSTR * pbstr) PURE;
    STDMETHOD(GetHelpInfo) (THIS_ DWORD * pdwHelpInfo) PURE;
    STDMETHOD(ShowHelp) (THIS_
                         LPOLESTR szHelp,
                         UINT fuCommand,
                         DWORD dwHelpContext) PURE;
};


//-------------------------------------------------------------------------
//  SApplicationObject Service
//    Host applications proffer their application object as this service.  
//    Various objects implement the "Application" property by returning 
//    this service.
//      
//-------------------------------------------------------------------------

// { 0c539790-12e4-11cf-b661-00aa004cd6d8 }
DEFINE_GUID(SID_SApplicationObject, 0x0c539790, 0x12e4, 0x11cf, 0xb6, 0x61, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8);

#endif // __OBJEXT_H

#pragma pack(pop) 
