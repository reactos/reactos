#ifndef _COMCAT_H
#define _COMCAT_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include <ole2.h>
#endif

#ifndef _RC_INVOKED

#ifdef __cplusplus
extern "C"{
#endif

EXTERN_C const IID IID_IEnumGUID;
typedef interface IEnumGUID *LPENUMGUID;

#undef INTERFACE
#define INTERFACE IEnumGUID
DECLARE_INTERFACE_(IEnumGUID,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Next)(THIS_ ULONG,GUID*,ULONG*) PURE;
	STDMETHOD(Skip)(THIS_ ULONG) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMGUID*) PURE;
};
#undef INTERFACE
#if (!defined (__cplusplus) || defined (CINTERFACE)) \
    && defined (COBJMACROS)
#define IEnumGUID_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumGUID_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IEnumGUID_Release(p)            (p)->lpVtbl->Release(p)
#define IEnumGUID_Next(p,a,b,c)         (p)->lpVtbl->Next(p,a,b,c)
#define IEnumGUID_Skip(p,a)             (p)->lpVtbl->Skip(p,a)
#define IEnumGUID_Reset(p)              (p)->lpVtbl->Reset(p)
#define IEnumGUID_Clone(p,a)            (p)->lpVtbl->Clone(p,a)
#endif

typedef GUID CATID;
typedef REFGUID REFCATID;
#define CATID_NULL GUID_NULL
#define IsEqualCATID(a, b) IsEqualGUID(a, b)

typedef struct tagCATEGORYINFO {
    CATID   catid;		/* category identifier for component */
    LCID    lcid;		/* locale identifier */
    OLECHAR szDescription[128];	/* description of the category */
} CATEGORYINFO, *LPCATEGORYINFO;

EXTERN_C const CATID CATID_Insertable;
EXTERN_C const CATID CATID_Control;
EXTERN_C const CATID CATID_Programmable;
EXTERN_C const CATID CATID_IsShortcut;
EXTERN_C const CATID CATID_NeverShowExt;
EXTERN_C const CATID CATID_DocObject;
EXTERN_C const CATID CATID_Printable;
EXTERN_C const CATID CATID_RequiresDataPathHost;
EXTERN_C const CATID CATID_PersistsToMoniker;
EXTERN_C const CATID CATID_PersistsToStorage;
EXTERN_C const CATID CATID_PersistsToStreamInit;
EXTERN_C const CATID CATID_PersistsToStream;
EXTERN_C const CATID CATID_PersistsToMemory;
EXTERN_C const CATID CATID_PersistsToFile;
EXTERN_C const CATID CATID_PersistsToPropertyBag;
EXTERN_C const CATID CATID_InternetAware;
EXTERN_C const CATID CATID_DesignTimeUIActivatableControl;

#define IEnumCATID IEnumGUID
#define LPENUMCATID LPENUMGUID
#define IID_IEnumCATID IID_IEnumGUID

#define IEnumCLSID IEnumGUID
#define LPENUMCLSID LPENUMGUID
#define IID_IEnumCLSID IID_IEnumGUID

EXTERN_C const IID IID_ICatInformation;
typedef interface ICatInformation *LPCATINFORMATION;

EXTERN_C const IID IID_ICatRegister;
typedef interface ICatRegister *LPCATREGISTER;

EXTERN_C const IID IID_IEnumCATEGORYINFO;
typedef interface IEnumCATEGORYINFO *LPENUMCATEGORYINFO;

EXTERN_C const CLSID CLSID_StdComponentCategoriesMgr;

#define INTERFACE ICatInformation
DECLARE_INTERFACE_(ICatInformation,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(EnumCategories)(THIS_ LCID,LPENUMCATEGORYINFO*) PURE;
	STDMETHOD(GetCategoryDesc)(THIS_ REFCATID,LCID,PWCHAR*) PURE;
	STDMETHOD(EnumClassesOfCategories)(THIS_ ULONG,CATID*,ULONG,CATID*,LPENUMCLSID*) PURE;
	STDMETHOD(IsClassOfCategories)(THIS_ REFCLSID,ULONG,CATID*,ULONG,CATID*) PURE;
	STDMETHOD(EnumImplCategoriesOfClass)(THIS_ REFCLSID,LPENUMCATID*) PURE;
	STDMETHOD(EnumReqCategoriesOfClass)(THIS_ REFCLSID,LPENUMCATID*) PURE;
};
#undef INTERFACE
#if (!defined (__cplusplus) || defined (CINTERFACE)) \
    && defined (COBJMACROS)
#define ICatInformation_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ICatInformation_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ICatInformation_Release(p)            (p)->lpVtbl->Release(p)
#define ICatInformation_EnumCategories(p,a,b) (p)->lpVtbl->EnumCategories(p,a,b)
#define ICatInformation_GetCategoryDesc(p,a,b,c) (p)->lpVtbl->GetCategoryDesc(p,a,b,c)
#define ICatInformation_EnumClassesOfCategories(p,a,b,c,d,e) (p)->lpVtbl->EnumClassesOfCategories(p,a,b,c,d,e)
#define ICatInformation_IsClassOfCategories(p,a,b,c,d,e) (p)->lpVtbl->IsClassOfCategories(p,a,b,c,d,e)
#define ICatInformation_EnumImplCategoriesOfClass(p,a,b) (p)->lpVtbl->EnumImplCategoriesOfClass(p,a,b)
#define ICatInformation_EnumReqCategoriesOfClass(p,a,b) (p)->lpVtbl->EnumReqCategoriesOfClass(p,a,b)
#endif

#define INTERFACE ICatRegister
DECLARE_INTERFACE_(ICatRegister,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(RegisterCategories)(THIS_ ULONG,CATEGORYINFO*) PURE;
	STDMETHOD(UnRegisterCategories)(THIS_ ULONG,CATID*) PURE;
	STDMETHOD(RegisterClassImplCategories)(THIS_ REFCLSID,ULONG,CATID*) PURE;
	STDMETHOD(UnRegisterClassImplCategories)(THIS_ REFCLSID,ULONG,CATID*) PURE;
	STDMETHOD(RegisterClassReqCategories)(THIS_ REFCLSID,ULONG,CATID*) PURE;
	STDMETHOD(UnRegisterClassReqCategories)(THIS_ REFCLSID,ULONG,CATID*) PURE;
};
#undef INTERFACE
#if (!defined (__cplusplus) || defined (CINTERFACE)) \
    && defined (COBJMACROS)
#define ICatRegister_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ICatRegister_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ICatRegister_Release(p)            (p)->lpVtbl->Release(p)
#define ICatRegister_RegisterCategories(p,a,b) (p)->lpVtbl->RegisterCategories(p,a,b)
#define ICatRegister_UnRegisterCategories(p,a,b) (p)->lpVtbl->UnRegisterCategories(p,a,b)
#define ICatRegister_RegisterClassImplCategories(p,a,b,c) (p)->lpVtbl->RegisterClassImplCategories(p,a,b,c)
#define ICatRegister_UnRegisterClassImplCategories(p,a,b,c) (p)->lpVtbl->UnRegisterClassImplCategories(p,a,b,c)
#define ICatRegister_RegisterClassReqCategories(p,a,b,c) (p)->lpVtbl->RegisterClassReqCategories(p,a,b,c)
#define ICatRegister_UnRegisterClassReqCategories(p,a,b,c) (p)->lpVtbl->UnRegisterClassReqCategories(p,a,b,c)
#endif

EXTERN_C const IID IID_IEnumCATEGORYINFO;
#undef INTERFACE
#define INTERFACE IEnumCATEGORYINFO
DECLARE_INTERFACE_(IEnumCATEGORYINFO,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Next)(THIS_ ULONG,CATEGORYINFO*,ULONG*) PURE;
	STDMETHOD(Skip)(THIS_ ULONG) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ LPENUMCATEGORYINFO*) PURE;
};
#undef INTERFACE
#if (!defined (__cplusplus) || defined (CINTERFACE)) \
    && defined (COBJMACROS)
#define IEnumCATEGORYINFO_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IEnumCATEGORYINFO_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IEnumCATEGORYINFO_Release(p)            (p)->lpVtbl->Release(p)
#define IEnumCATEGORYINFO_Next(p,a,b,c)         (p)->lpVtbl->Next(p,a,b,c)
#define IEnumCATEGORYINFO_Skip(p,a)             (p)->lpVtbl->Skip(p,a)
#define IEnumCATEGORYINFO_Reset(p)              (p)->lpVtbl->Reset(p)
#define IEnumCATEGORYINFO_Clone(p,a)            (p)->lpVtbl->Clone(p,a)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _RC_INVOKED */
#endif
