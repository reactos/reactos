/*
 * Defines the COM interfaces and APIs from ocidl.h related to property
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_PROPERTY_H
#define __WINE_WINE_OBJ_PROPERTY_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */
typedef struct tagPROPPAGEINFO
{
	ULONG cb;
	LPOLESTR pszTitle;
	SIZE size;
	LPOLESTR pszDocString;
	LPOLESTR pszHelpFile;
	DWORD dwHelpContext;
} PROPPAGEINFO, *LPPROPPAGEINFO;

typedef enum tagPROPPAGESTATUS
{
	PROPPAGESTATUS_DIRTY = 0x1,
	PROPPAGESTATUS_VALIDATE = 0x2,
	PROPPAGESTATUS_CLEAN = 0x4
} PROPPAGESTATUS;

typedef struct tagCAUUID
{
	ULONG cElems;
	GUID* pElems;
} CAUUID, *LPCAUUID;

typedef struct tagCALPOLESTR
{
	ULONG cElems;
	LPOLESTR *pElems;
} CALPOLESTR, *LPCALPOLESTR;

typedef struct tagCADWORD
{
	ULONG cElems;
	DWORD *pElems;
} CADWORD, *LPCADWORD;


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IPropertyPage, 0xb196b28dL, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyPage IPropertyPage, *LPPROPERTYPAGE;

DEFINE_GUID(IID_IPropertyPage2, 0x01e44665L, 0x24ac, 0x101b, 0x84, 0xed, 0x08, 0x00, 0x2b, 0x2e, 0xc7, 0x13);
typedef struct IPropertyPage2 IPropertyPage2, *LPPROPERTYPAGE2;

DEFINE_GUID(IID_IPropertyPageSite, 0xb196b28cL, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyPageSite IPropertyPageSite, *LPPROPERTYPAGESITE;

DEFINE_GUID(IID_IPropertyNotifySink, 0x9bfbbc02L, 0xeff1, 0x101a, 0x84, 0xed, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPropertyNotifySink IPropertyNotifySink, *LPPROPERTYNOTIFYSINK;

DEFINE_GUID(IID_ISimpleFrameSite, 0x742b0e01L, 0x14e6, 0x101b, 0x91, 0x4e, 0x00, 0xaa, 0x00, 0x30, 0x0c, 0xab);
typedef struct ISimpleFrameSite ISimpleFrameSite, *LPSIMPLEFRAMESITE;

DEFINE_GUID(IID_IPersistStreamInit, 0x7fd52380L, 0x4e07, 0x101b, 0xae, 0x2d, 0x08, 0x00, 0x2b, 0x2e, 0xc7, 0x13);
typedef struct IPersistStreamInit IPersistStreamInit,*LPPERSISTSTREAMINIT;

DEFINE_GUID(IID_IPersistMemory, 0xbd1ae5e0L, 0xa6ae, 0x11ce, 0xbd, 0x37, 0x50, 0x42, 0x00, 0xc1, 0x00, 0x00);
typedef struct IPersistMemory IPersistMemory,*LPPERSISTMEMORY;
 
DEFINE_GUID(IID_IPersistPropertyBag, 0x37d84f60, 0x42cb, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IPersistPropertyBag IPersistPropertyBag,*LPPERSISTPROPERTYBAG;
 
DEFINE_GUID(IID_IErrorLog, 0x3127ca40L, 0x446e, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IErrorLog IErrorLog,*LPERRORLOG;

DEFINE_GUID(IID_IPropertyBag, 0x55272a00L, 0x42cb, 0x11ce, 0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51);
typedef struct IPropertyBag IPropertyBag,*LPPROPERTYBAG;
 
DEFINE_GUID(IID_ISpecifyPropertyPages, 0xb196b28b, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct ISpecifyPropertyPages ISpecifyPropertyPages,*LPSPECIFYPROPERTYPAGES;
 
DEFINE_GUID(IID_IPerPropertyBrowsing, 0xb196b28b, 0xbab4, 0x101a, 0xb6, 0x9c, 0x00, 0xaa, 0x00, 0x34, 0x1d, 0x07);
typedef struct IPerPropertyBrowsing IPerPropertyBrowsing,*LPPERPROPERTYBROWSING;
 

/*****************************************************************************
 * IPropertPage interface
 */
#define ICOM_INTERFACE IPropertyPage
#define IPropertyPage_METHODS \
 	ICOM_METHOD1(HRESULT,SetPageSite, IPropertyPageSite*,pPageSite) \
 	ICOM_METHOD3(HRESULT,Activate, HWND,hWndParent, LPCRECT,pRect, BOOL,bModal) \
 	ICOM_METHOD (HRESULT,Deactivate) \
 	ICOM_METHOD1(HRESULT,GetPageInfo, PROPPAGEINFO*,pPageInfo) \
 	ICOM_METHOD2(HRESULT,SetObjects, ULONG,cObjects, IUnknown**,ppUnk) \
 	ICOM_METHOD1(HRESULT,Show, UINT,nCmdShow) \
 	ICOM_METHOD1(HRESULT,Move, LPCRECT,pRect) \
 	ICOM_METHOD (HRESULT,IsPageDirty) \
 	ICOM_METHOD (HRESULT,Apply) \
 	ICOM_METHOD1(HRESULT,Help, LPCOLESTR,pszHelpDir) \
 	ICOM_METHOD1(HRESULT,TranslateAccelerator, MSG*,pMsg) 
#define IPropertyPage_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyPage_METHODS
ICOM_DEFINE(IPropertyPage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertyPage_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPage_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPage_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPage methods ***/
#define IPropertyPage_SetPageSite(p,a)          ICOM_CALL1(SetPageSite,p,a)
#define IPropertyPage_Activate(p,a,b,c)         ICOM_CALL3(Activate,p,a,b,c)
#define IPropertyPage_Deactivate(p)             ICOM_CALL (Deactivate,p)
#define IPropertyPage_GetPageInfo(p,a)          ICOM_CALL1(GetPageInfo,p,a)
#define IPropertyPage_SetObjects(p,a,b)         ICOM_CALL2(SetObjects,p,a,b)
#define IPropertyPage_Show(p,a)                 ICOM_CALL1(Show,p,a)
#define IPropertyPage_Move(p,a)                 ICOM_CALL1(Move,p,a)
#define IPropertyPage_IsPageDirty(p)            ICOM_CALL (IsPageDirty,p)
#define IPropertyPage_Apply(p)                  ICOM_CALL (Apply,p)
#define IPropertyPage_Help(p,a)                 ICOM_CALL1(Help,p,a)
#define IPropertyPage_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)
				 

/*****************************************************************************
 * IPropertPage2 interface
 */
#define ICOM_INTERFACE IPropertyPage2
#define IPropertyPage2_METHODS \
	ICOM_METHOD1(HRESULT,EditProperty, DISPID,dispID)
#define IPropertyPage2_IMETHODS \
	IPropertyPage_IMETHODS \
	IPropertyPage2_METHODS
ICOM_DEFINE(IPropertyPage2,IPropertyPage)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertyPage2_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPage2_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPage2_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPage methods ***/
#define IPropertyPage2_SetPageSite(p,a)          ICOM_CALL1(SetPageSite,p,a)
#define IPropertyPage2_Activate(p,a,b,c)         ICOM_CALL3(Activate,p,a,b,c)
#define IPropertyPage2_Deactivate(p)             ICOM_CALL (Deactivate,p)
#define IPropertyPage2_GetPageInfo(p,a)          ICOM_CALL1(GetPageInfo,p,a)
#define IPropertyPage2_SetObjects(p,a,b)         ICOM_CALL2(SetObjects,p,a,b)
#define IPropertyPage2_Show(p,a)                 ICOM_CALL1(Show,p,a)
#define IPropertyPage2_Move(p,a)                 ICOM_CALL1(Move,p,a)
#define IPropertyPage2_IsPageDirty(p)            ICOM_CALL (IsPageDirty,p)
#define IPropertyPage2_Apply(p)                  ICOM_CALL (Apply,p)
#define IPropertyPage2_Help(p,a)                 ICOM_CALL1(Help,p,a)
#define IPropertyPage2_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)
/*** IPropertyPage2 methods ***/
#define IPropertyPage2_EditProperty(p,a)         ICOM_CALL1(EditProperty,p,a)
				 

/*****************************************************************************
 * IPropertPageSite interface
 */
#define ICOM_INTERFACE IPropertyPageSite
#define IPropertyPageSite_METHODS \
	ICOM_METHOD1(HRESULT,OnStatusChange, DWORD,dwFlags) \
	ICOM_METHOD1(HRESULT,GetLocaleID, LCID*,pLocaleID) \
	ICOM_METHOD1(HRESULT,GetPageContainer, IUnknown**,ppUnk) \
	ICOM_METHOD1(HRESULT,TranslateAccelerator, MSG*,pMsg)
#define IPropertyPageSite_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyPageSite_METHODS
ICOM_DEFINE(IPropertyPageSite,IUnknown)
#undef ICOM_INTERFACE 

/*** IUnknown methods ***/
#define IPropertyPageSite_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyPageSite_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyPageSite_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyPageSite methods ***/
#define IPropertyPageSite_OnStatusChange(p,a)       ICOM_CALL1(OnStatusChange,p,a)
#define IPropertyPageSite_GetLocaleID(p,a)          ICOM_CALL1(GetLocaleID,p,a)
#define IPropertyPageSite_GetPageContainer(p,a)     ICOM_CALL1(GetPageContainer,p,a)
#define IPropertyPageSite_TranslateAccelerator(p,a) ICOM_CALL1(TranslateAccelerator,p,a)
						 

/*****************************************************************************
 * IPropertyNotifySink interface
 */
#define ICOM_INTERFACE IPropertyNotifySink
#define IPropertyNotifySink_METHODS \
	ICOM_METHOD1(HRESULT,OnChanged, DISPID,dispID) \
	ICOM_METHOD1(HRESULT,OnRequestEdit, DISPID,dispID)
#define IPropertyNotifySink_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyNotifySink_METHODS
ICOM_DEFINE(IPropertyNotifySink,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertyNotifySink_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyNotifySink_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IPropertyNotifySink_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyNotifySink methods ***/
#define IPropertyNotifySink_OnChanged(p,a)            ICOM_CALL1(OnChanged,p,a)
#define IPropertyNotifySink_OnRequestEdit(p,a)        ICOM_CALL1(OnRequestEdit,p,a)

				 
/*****************************************************************************
 * IPropertyNotifySink interface
 */
#define ICOM_INTERFACE ISimpleFrameSite
#define ISimpleFrameSite_METHODS \
	ICOM_METHOD6(HRESULT,PreMessageFilter, HWND,hWnd, UINT,msg, WPARAM,wp, LPARAM,lp, LRESULT*,plResult, DWORD*,pwdCookie) \
	ICOM_METHOD6(HRESULT,PostMessageFilter, HWND,hWnd, UINT,msg, WPARAM,wp, LPARAM,lp, LRESULT*,plResult, DWORD,pwdCookie) 
#define ISimpleFrameSite_IMETHODS \
	IUnknown_IMETHODS \
	ISimpleFrameSite_METHODS
ICOM_DEFINE(ISimpleFrameSite,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ISimpleFrameSite_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define ISimpleFrameSite_AddRef(p)                 ICOM_CALL (AddRef,p)
#define ISimpleFrameSite_Release(p)                ICOM_CALL (Release,p)
/*** IPropertyNotifySink methods ***/
#define ISimpleFrameSite_PreMessageFilter(p,a,b,c,d,e,f) ICOM_CALL1(PreMessageFilter,p,a,b,c,d,e,f)
#define ISimpleFrameSite_PostMessageFilter(p,a,b,c,d,e,f) ICOM_CALL1(PostMessageFilter,p,a,b,c,d,e,f)


/*****************************************************************************
 * IPersistStreamInit interface
 */
#define ICOM_INTERFACE IPersistStreamInit
#define IPersistStreamInit_METHODS \
	ICOM_METHOD (HRESULT,IsDirty) \
	ICOM_METHOD1(HRESULT,Load,       LPSTREAM,pStm) \
	ICOM_METHOD2(HRESULT,Save,       LPSTREAM,pStm, BOOL,fClearDirty) \
	ICOM_METHOD1(HRESULT,GetSizeMax, ULARGE_INTEGER*,pcbSize) \
	ICOM_METHOD (HRESULT,InitNew)
#define IPersistStreamInit_IMETHODS \
	IPersist_IMETHODS \
	IPersistStreamInit_METHODS
ICOM_DEFINE(IPersistStreamInit,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistStreamInit_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistStreamInit_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistStreamInit_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistStreamInit_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistStreamInit methods ***/
#define IPersistStreamInit_IsDirty(p)      ICOM_CALL (IsDirty,p)
#define IPersistStreamInit_Load(p,a)       ICOM_CALL1(Load,p,a)
#define IPersistStreamInit_Save(p,a,b)     ICOM_CALL2(Save,p,a,b)
#define IPersistStreamInit_GetSizeMax(p,a) ICOM_CALL1(GetSizeMax,p,a)
#define IPersistStreamInit_InitNew(p)      ICOM_CALL (InitNew,p)

				 
/*****************************************************************************
 * IPersistMemory interface
 */
#define ICOM_INTERFACE IPersistMemory
#define IPersistMemory_METHODS \
	ICOM_METHOD (HRESULT,IsDirty) \
	ICOM_METHOD2(HRESULT,Load, LPVOID,pMem, ULONG,cbSize) \
	ICOM_METHOD3(HRESULT,Save, LPVOID,pMem, BOOL,fClearDirty, ULONG,cbSize) \
	ICOM_METHOD1(HRESULT,GetSizeMax, ULONG*,pCbSize) \
	ICOM_METHOD (HRESULT,InitNew)
#define IPersistMemory_IMETHODS \
	IPersist_IMETHODS \
	IPersistMemory_METHODS
ICOM_DEFINE(IPersistMemory,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistMemory_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistMemory_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistMemory_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistMemory_GetClassID(p,a)       ICOM_CALL1(GetClassID,p,a)
/*** IPersistMemory methods ***/
#define IPersistMemory_IsDirty(p)            ICOM_CALL (IsDirty,p)
#define IPersistMemory_Load(p,a,b)           ICOM_CALL2(Load,p,a,b)
#define IPersistMemory_Save(p,a,b,c)         ICOM_CALL3(Save,p,a,b,c)
#define IPersistMemory_GetSizeMax(p,a)       ICOM_CALL1(GetSizeMax,p,a)
#define IPersistMemory_InitNew(p)            ICOM_CALL (InitNew,p)


/*****************************************************************************
 * IPersistPropertyBag interface
 */
#define ICOM_INTERFACE IPersistPropertyBag
#define IPersistPropertyBag_METHODS \
	ICOM_METHOD (HRESULT,InitNew) \
	ICOM_METHOD2(HRESULT,Load, IPropertyBag*,pPropBag, IErrorLog*,pErrorLog) \
	ICOM_METHOD3(HRESULT,Save, IPropertyBag*,pPropBag, BOOL,fClearDirty, BOOL,fSaveAllProperties)
#define IPersistPropertyBag_IMETHODS \
	IPersist_IMETHODS \
	IPersistPropertyBag_METHODS
ICOM_DEFINE(IPersistPropertyBag,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistPropertyBag_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistPropertyBag_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistPropertyBag_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistPropertyBag_GetClassID(p,a)       ICOM_CALL1(GetClassID,p,a)
/*** IPersistPropertyBag methods ***/
#define IPersistPropertyBag_InitNew(p)            ICOM_CALL (InitNew,p)
#define IPersistPropertyBag_Load(p,a,b)           ICOM_CALL2(Load,p,a,b)
#define IPersistPropertyBag_Save(p,a,b,c)         ICOM_CALL3(Save,p,a,b,c)


/*****************************************************************************
 * IErrorLog interface
 */
#define ICOM_INTERFACE IErrorLog
#define IErrorLog_METHODS \
	ICOM_METHOD2(HRESULT,AddError, LPCOLESTR,pszPropName, EXCEPINFO*,pExcepInfo)
#define IErrorLog_IMETHODS \
	IUnknown_IMETHODS \
	IErrorLog_METHODS
ICOM_DEFINE(IErrorLog,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IErrorLog_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IErrorLog_AddRef(p)             ICOM_CALL (AddRef,p)
#define IErrorLog_Release(p)            ICOM_CALL (Release,p)
/*** IErrorLog methods ***/
#define IErrorLog_AddError(p,a,b)       ICOM_CALL2(GetClassID,p,a,b)
				 

/*****************************************************************************
 * IPropertyBag interface
 */
#define ICOM_INTERFACE IPropertyBag
#define IPropertyBag_METHODS \
	ICOM_METHOD3(HRESULT,Read, LPCOLESTR,pszPropName, VARIANT*,pVar, IErrorLog*,pErrorLog) \
	ICOM_METHOD2(HRESULT,Write, LPCOLESTR,pszPropName, VARIANT*,pVar)
#define IPropertyBag_IMETHODS \
	IUnknown_IMETHODS \
	IPropertyBag_METHODS
ICOM_DEFINE(IPropertyBag,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPropertyBag_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPropertyBag_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPropertyBag_Release(p)            ICOM_CALL (Release,p)
/*** IPropertyBag methods ***/
#define IPropertyBag_Read(p,a,b,c)         ICOM_CALL3(Read,p,a,b,c)
#define IPropertyBag_Write(p,a,b)          ICOM_CALL2(Write,p,a,b)


/*****************************************************************************
 * ISpecifyPropertyPages interface
 */
#define ICOM_INTERFACE ISpecifyPropertyPages
#define ISpecifyPropertyPages_METHODS \
	ICOM_METHOD1(HRESULT,GetPages, CAUUID*,pPages) 
#define ISpecifyPropertyPages_IMETHODS \
	IUnknown_IMETHODS \
	ISpecifyPropertyPages_METHODS
ICOM_DEFINE(ISpecifyPropertyPages,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ISpecifyPropertyPages_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define ISpecifyPropertyPages_AddRef(p)             ICOM_CALL (AddRef,p)
#define ISpecifyPropertyPages_Release(p)            ICOM_CALL (Release,p)
/*** ISpecifyPropertyPages methods ***/
#define ISpecifyPropertyPages_GetPages(p,a)         ICOM_CALL3(GetPages,p,a)
				  

/*****************************************************************************
 * IPerPropertyBrowsing interface
 */
#define ICOM_INTERFACE IPerPropertyBrowsing
#define IPerPropertyBrowsing_METHODS \
	ICOM_METHOD2(HRESULT,GetDisplayString, DISPID,dispID, BSTR*,pBstr) \
	ICOM_METHOD2(HRESULT,MapPropertyToPage, DISPID,dispID, CLSID*,pClsid) \
	ICOM_METHOD3(HRESULT,GetPredefinedStrings, DISPID,dispID, CALPOLESTR*,pCaStringsOut, CADWORD*,pCaCookiesOut) \
	ICOM_METHOD3(HRESULT,GetPredefinedValue, DISPID,dispID, DWORD,dwCookie, VARIANT*,pVarOut) 
#define IPerPropertyBrowsing_IMETHODS \
	IUnknown_IMETHODS \
	IPerPropertyBrowsing_METHODS
ICOM_DEFINE(IPerPropertyBrowsing,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPerPropertyBrowsing_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPerPropertyBrowsing_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPerPropertyBrowsing_Release(p)            ICOM_CALL (Release,p)
/*** IPerPropertyBrowsing methods ***/
#define IPerPropertyBrowsing_GetDisplayString(p,a,b)       ICOM_CALL2(GetDisplayString,p,a,b)
#define IPerPropertyBrowsing_MapPropertyToPage(p,a,b)      ICOM_CALL2(MapPropertyToPage,p,a,b)
#define IPerPropertyBrowsing_GetPredefinedStrings(p,a,b,c) ICOM_CALL3(GetPredefinedStrings,p,a,b,c)
#define IPerPropertyBrowsing_GetPredefinedValue(p,a,b,c)   ICOM_CALL3(GetPredefinedValue,p,a,b,c)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PROPERTY_H */

