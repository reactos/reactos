/*
 * Defines the COM interfaces and APIs related to structured data storage.
 * 
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_INPLACE_H
#define __WINE_WINE_OBJ_INPLACE_H

struct tagMSG;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the structures
 */
typedef struct  tagOleMenuGroupWidths
{
	LONG width[ 6 ];
} OLEMENUGROUPWIDTHS, *LPOLEMENUGROUPWIDTHS;


typedef struct tagOleInPlaceFrameInfo
{
	UINT cb;
	BOOL fMDIApp;
	HWND hwndFrame;
	HACCEL haccel;
	UINT cAccelEntries;
} OLEINPLACEFRAMEINFO, *LPOLEINPLACEFRAMEINFO;

typedef enum tagOLEGETMONIKER
{
	OLEGETMONIKER_ONLYIFTHERE = 1,
	OLEGETMONIKER_FORCEASSIGN = 2,
	OLEGETMONIKER_UNASSIGN = 3,
	OLEGETMONIKER_TEMPFORUSER = 4
} OLEGETMONIKER;

typedef enum tagOLERENDER
{
	OLERENDER_NONE = 0,
	OLERENDER_DRAW = 1,
	OLERENDER_FORMAT = 2,
	OLERENDER_ASIS = 3
} OLERENDER;

typedef enum tagUSERCLASSTYPE
{
	USERCLASSTYPE_FULL = 1,
	USERCLASSTYPE_SHORT = 2,
	USERCLASSTYPE_APPNAME = 3
} USERCLASSTYPE;

typedef enum tagOLECLOSE
{
	OLECLOSE_SAVEIFDIRTY = 1,
	OLECLOSE_NOSAVE = 2,
	OLECLOSE_PROMPTSAVE = 3
} OLECLOSE;

typedef enum tagOLEUPDATE
{
	OLEUPDATE_ALWAYS = 1,
	OLEUPDATE_ONCALL = 3
} OLEUPDATE, *LPOLEUPDATE;

typedef struct tagOLEVERB
{
	LONG lVerb;
	LPOLESTR lpszVerbName;
	DWORD fuFlags;
	DWORD grfAttribs;
} OLEVERB, *LPOLEVERB;
	
typedef enum tagOLELINKBIND
{
	OLELINKBIND_EVENIFCLASSDIFF = 1
} OLELINKBIND;

typedef enum tagOLEWHICHMK
{
	OLEWHICHMK_CONTAINER = 1,
	OLEWHICHMK_OBJREL = 2,
	OLEWHICHMK_OBJFULL = 3
} OLEWHICHMK;

typedef enum tagBINDSPEED
{
	BINDSPEED_INDEFINITE = 1,
	BINDSPEED_MODERATE = 2,
	BINDSPEED_IMMEDIATE = 3
} BINDSPEED;

typedef enum tagOLECONTF
{
	OLECONTF_EMBEDDINGS = 1,
	OLECONTF_LINKS = 2,
	OLECONTF_OTHERS = 4,
	OLECONTF_OLNYUSER = 8,
	OLECONTF_ONLYIFRUNNING = 16
} OLECONTF;
 
typedef HGLOBAL HOLEMENU;
typedef LPRECT LPBORDERWIDTHS;
typedef LPCRECT LPCBORDERWIDTHS;
	
	
/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IOleWindow,  0x00000114L, 0, 0);
typedef struct IOleWindow IOleWindow, *LPOLEWINDOW;

DEFINE_OLEGUID(IID_IOleInPlaceObject,  0x00000113L, 0, 0);
typedef struct IOleInPlaceObject IOleInPlaceObject, *LPOLEINPLACEOBJECT;

DEFINE_OLEGUID(IID_IOleInPlaceActiveObject,  0x00000117L, 0, 0);
typedef struct IOleInPlaceActiveObject IOleInPlaceActiveObject, *LPOLEINPLACEACTIVEOBJECT;

DEFINE_OLEGUID(IID_IOleInPlaceUIWindow,  0x00000115L, 0, 0);
typedef struct IOleInPlaceUIWindow IOleInPlaceUIWindow, *LPOLEINPLACEUIWINDOW;

DEFINE_OLEGUID(IID_IOleInPlaceFrame,  0x00000116L, 0, 0);
typedef struct IOleInPlaceFrame IOleInPlaceFrame, *LPOLEINPLACEFRAME;

DEFINE_OLEGUID(IID_IOleInPlaceSite,  0x00000119L, 0, 0);
typedef struct IOleInPlaceSite IOleInPlaceSite, *LPOLEINPLACESITE;

DEFINE_OLEGUID(IID_IOleClientSite,  0x00000118L, 0, 0);
typedef struct IOleClientSite IOleClientSite, *LPOLECLIENTSITE;

DEFINE_OLEGUID(IID_IOleContainer,  0x0000011bL, 0, 0);
typedef struct IOleContainer IOleContainer, *LPOLECONTAINER;

DEFINE_OLEGUID(IID_IParseDisplayName,  0x0000011aL, 0, 0);
typedef struct IParseDisplayName IParseDisplayName, *LPPARSEDISPLAYNAME;

DEFINE_OLEGUID(IID_IOleItemContainer,  0x0000011cL, 0, 0);
typedef struct IOleItemContainer IOleItemContainer, *LPOLEITEMCONTAINER;

DEFINE_OLEGUID(IID_IOleLink,  0x0000011dL, 0, 0);
typedef struct IOleLink IOleLink, *LPOLELINK;

/*****************************************************************************
 * IOleWindow interface
 */
#define ICOM_INTERFACE IOleWindow
#define IOleWindow_METHODS \
	ICOM_METHOD1(HRESULT,GetWindow, HWND*,phwnd) \
	ICOM_METHOD1(HRESULT,ContextSensitiveHelp, BOOL,fEnterMode)
#define IOleWindow_IMETHODS \
	IUnknown_IMETHODS \
	IOleWindow_METHODS
ICOM_DEFINE(IOleWindow,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleWindow_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleWindow_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleWindow_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleWindow_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleWindow_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)


/*****************************************************************************
 * IOleInPlaceObject interface
 */
#define ICOM_INTERFACE IOleInPlaceObject
#define IOleInPlaceObject_METHODS \
	ICOM_METHOD (HRESULT,InPlaceDeactivate) \
	ICOM_METHOD (HRESULT,UIDeactivate) \
	ICOM_METHOD2(HRESULT,SetObjectRects, LPCRECT,lprcPosRect, LPCRECT,lprcClipRect) \
	ICOM_METHOD (HRESULT,ReactivateAndUndo)
#define IOleInPlaceObject_IMETHODS \
	IOleWindow_IMETHODS \
	IOleInPlaceObject_METHODS
ICOM_DEFINE(IOleInPlaceObject,IOleWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceObject_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceObject_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleInPlaceObject_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceObject_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceObject_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceObject methods ***/
#define IOleInPlaceObject_InPlaceDeactivate(p)       ICOM_CALL (InPlaceDeactivate,p)
#define IOleInPlaceObject_UIDeactivate(p)            ICOM_CALL (UIDeactivate,p)
#define IOleInPlaceObject_SetObjectRects(p,a,b)      ICOM_CALL2(SetObjectRects,p,a,b)
#define IOleInPlaceObject_ReactivateAndUndo(p)       ICOM_CALL (ReactivateAndUndo,p)

/*****************************************************************************
 * IOleInPlaceActiveObject interface
 */
#define ICOM_INTERFACE IOleInPlaceActiveObject
#define IOleInPlaceActiveObject_METHODS \
	ICOM_METHOD1(HRESULT,TranslateAccelerator, struct tagMSG*,lpmsg) \
	ICOM_METHOD1(HRESULT,OnFrameWindowActivate, BOOL,fActivate) \
	ICOM_METHOD1(HRESULT,OnDocWindowActivate, BOOL,fActivate) \
	ICOM_METHOD3(HRESULT,ResizeBorder, LPCRECT,prcBorder, IOleInPlaceUIWindow*,pUIWindow, BOOL,fWindowFrame) \
	ICOM_METHOD1(HRESULT,EnableModeless, BOOL,fEnable)
#define IOleInPlaceActiveObject_IMETHODS \
	IOleWindow_IMETHODS \
	IOleInPlaceActiveObject_METHODS
ICOM_DEFINE(IOleInPlaceActiveObject,IOleWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceActiveObject_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceActiveObject_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleInPlaceActiveObject_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceActiveObject_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceActiveObject_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceActiveObject methods ***/
#define IOleInPlaceActiveObject_TranslateAccelerator(p,a)  ICOM_CALL1(TranslateAccelerator,p,a)
#define IOleInPlaceActiveObject_OnFrameWindowActivate(p,a) ICOM_CALL1(OnFrameWindowActivate,p,a)
#define IOleInPlaceActiveObject_OnDocWindowActivate(p,a)   ICOM_CALL1(OnDocWindowActivate,p,a)
#define IOleInPlaceActiveObject_ResizeBorder(p,a,b,c)      ICOM_CALL3(ResizeBorder,p,a,b,c)
#define IOleInPlaceActiveObject_EnableModeless(p,a)        ICOM_CALL1(EnableModeless,p,a)

/*****************************************************************************
 * IOleInPlaceUIWindow interface
 */
#define ICOM_INTERFACE IOleInPlaceUIWindow
#define IOleInPlaceUIWindow_METHODS \
	ICOM_METHOD1(HRESULT,GetBorder, LPRECT,lprectBorder) \
	ICOM_METHOD1(HRESULT,RequestBorderSpace, LPCBORDERWIDTHS,pborderwidths) \
	ICOM_METHOD1(HRESULT,SetBorderSpace, LPCBORDERWIDTHS,pborderwidths) \
	ICOM_METHOD2(HRESULT,SetActiveObject, IOleInPlaceActiveObject*,pActiveObject, LPCOLESTR,pszObjName)
#define IOleInPlaceUIWindow_IMETHODS \
	IOleWindow_IMETHODS \
	IOleInPlaceUIWindow_METHODS
ICOM_DEFINE(IOleInPlaceUIWindow,IOleWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceUIWindow_QueryInterface(p,a,b)     ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceUIWindow_AddRef(p)                 ICOM_CALL (AddRef,p)
#define IOleInPlaceUIWindow_Release(p)                ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceUIWindow_GetWindow(p,a)            ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceUIWindow_ContextSensitiveHelp(p,a) ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceUIWindow methods ***/
#define IOleInPlaceUIWindow_GetBorder(p,a)            ICOM_CALL1(GetBorder,p,a)
#define IOleInPlaceUIWindow_RequestBorderSpace(p,a)   ICOM_CALL1(RequestBorderSpace,p,a)
#define IOleInPlaceUIWindow_SetBorderSpace(p,a)       ICOM_CALL1(SetBorderSpace,p,a)
#define IOleInPlaceUIWindow_SetActiveObject(p,a,b)    ICOM_CALL2(SetActiveObject,p,a,b)
				  

/*****************************************************************************
 * IOleInPlaceFrame interface
 */
#define ICOM_INTERFACE IOleInPlaceFrame
#define IOleInPlaceFrame_METHODS \
	ICOM_METHOD2 (HRESULT,InsertMenus, HMENU,hemnuShared, LPOLEMENUGROUPWIDTHS,lpMenuWidths) \
	ICOM_METHOD3 (HRESULT,SetMenu, HMENU,hemnuShared, HOLEMENU,holemenu, HWND,hwndActiveObject) \
	ICOM_METHOD1 (HRESULT,RemoveMenus, HMENU,hemnuShared) \
	ICOM_METHOD1 (HRESULT,SetStatusText, LPCOLESTR,pszStatusText) \
	ICOM_METHOD1 (HRESULT,EnableModeless, BOOL,fEnable) \
	ICOM_METHOD2 (HRESULT,TranslateAccelerator, struct tagMSG*,lpmsg, WORD,wID)
#define IOleInPlaceFrame_IMETHODS \
	IOleInPlaceUIWindow_IMETHODS \
	IOleInPlaceFrame_METHODS
ICOM_DEFINE(IOleInPlaceFrame,IOleInPlaceUIWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceFrame_QueryInterface(p,a,b)    ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceFrame_AddRef(p)                ICOM_CALL (AddRef,p)
#define IOleInPlaceFrame_Release(p)               ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceFrame_GetWindow                ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceFrame_ContextSensitiveHelp     ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceUIWindow methods ***/
#define IOleInPlaceFrame_GetBorder                ICOM_CALL1(GetBorder,p,a)
#define IOleInPlaceFrame_RequestBorderSpace       ICOM_CALL1(RequestBorderSpace,p,a)
#define IOleInPlaceFrame_SetBorderSpace           ICOM_CALL1(SetBorderSpace,p,a)
#define IOleInPlaceFrame_SetActiveObject          ICOM_CALL2(SetActiveObject,p,a,b)
/*** IOleInPlaceFrame methods ***/
#define IOleInPlaceFrame_InsertMenus              ICOM_CALL2(InsertMenus,p,a,b)
#define IOleInPlaceFrame_SetMenu                  ICOM_CALL3(SetMenu,p,a,b,c)
#define IOleInPlaceFrame_RemoveMenus              ICOM_CALL1(RemoveMenus,p,a)
#define IOleInPlaceFrame_SetStatusText            ICOM_CALL1(SetStatusText,p,a)
#define IOleInPlaceFrame_EnableModeless           ICOM_CALL1(EnableModeless,p,a)
#define IOleInPlaceFrame_TranslateAccelerator     ICOM_CALL2(TranslateAccelerator,p,a,b)
				 

/*****************************************************************************
 * IOleInPlaceSite interface
 */
#define ICOM_INTERFACE IOleInPlaceSite
#define IOleInPlaceSite_METHODS \
	ICOM_METHOD (HRESULT,CanInPlaceActivate) \
	ICOM_METHOD (HRESULT,OnInPlaceActivate) \
	ICOM_METHOD (HRESULT,OnUIActivate) \
	ICOM_METHOD5(HRESULT,GetWindowContext, IOleInPlaceFrame**,ppFrame, IOleInPlaceUIWindow**,ppDoc, LPRECT,lprcPosRect, LPRECT,lprcClipRect, LPOLEINPLACEFRAMEINFO,lpFrameInfo) \
	ICOM_METHOD1(HRESULT,Scroll, SIZE,scrollExtant) \
	ICOM_METHOD1(HRESULT,OnUIDeactivate, BOOL,fUndoable) \
	ICOM_METHOD (HRESULT,OnInPlaceDeactivate) \
	ICOM_METHOD (HRESULT,DiscardUndoState) \
	ICOM_METHOD (HRESULT,DeactivateAndUndo) \
	ICOM_METHOD1(HRESULT,OnPosRectChange, LPCRECT,lprcPosRect)
#define IOleInPlaceSite_IMETHODS \
	IOleWindow_IMETHODS \
	IOleInPlaceSite_METHODS
ICOM_DEFINE(IOleInPlaceSite,IOleWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleInPlaceSite_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleInPlaceSite_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleInPlaceSite_Release(p)                 ICOM_CALL (Release,p)
/*** IOleWindow methods ***/
#define IOleInPlaceSite_GetWindow(p,a)             ICOM_CALL1(GetWindow,p,a)
#define IOleInPlaceSite_ContextSensitiveHelp(p,a)  ICOM_CALL1(ContextSensitiveHelp,p,a)
/*** IOleInPlaceSite methods ***/
#define IOleInPlaceSite_CanInPlaceActivate(p)      ICOM_CALL (CanInPlaceActivate,p)
#define IOleInPlaceSite_OnInPlaceActivate(p)       ICOM_CALL (OnInPlaceActivate,p)
#define IOleInPlaceSite_OnUIActivate(p)            ICOM_CALL (OnUIActivate,p)
#define IOleInPlaceSite_GetWindowContext(p,a,b,c,d,e) ICOM_CALL5(GetWindowContext,p,a,b,c,d,e)
#define IOleInPlaceSite_Scroll(p,a)                ICOM_CALL1(Scroll,p,a)
#define IOleInPlaceSite_OnUIDeactivate(p,a)        ICOM_CALL1(OnUIDeactivate,p,a)
#define IOleInPlaceSite_OnInPlaceDeactivate(p)     ICOM_CALL (OnInPlaceDeactivate,p)
#define IOleInPlaceSite_DiscardUndoState(p)        ICOM_CALL (DiscardUndoState,p)
#define IOleInPlaceSite_DeactivateAndUndo(p)       ICOM_CALL (DeactivateAndUndo,p)
#define IOleInPlaceSite_OnPosRectChange(p,a)       ICOM_CALL1(OnPosRectChange,p,a)


/*****************************************************************************
 * IOleClientSite interface
 */
#define ICOM_INTERFACE IOleClientSite
#define IOleClientSite_METHODS \
	ICOM_METHOD (HRESULT,SaveObject) \
	ICOM_METHOD3(HRESULT,GetMoniker, DWORD,dwAssign, DWORD,dwWhichMoniker, IMoniker**,ppmk) \
	ICOM_METHOD1(HRESULT,GetContainer, IOleContainer**,ppContainer) \
	ICOM_METHOD (HRESULT,ShowObject) \
	ICOM_METHOD1(HRESULT,OnShowWindow, BOOL,fShow) \
	ICOM_METHOD (HRESULT,RequestNewObjectLayout)
#define IOleClientSite_IMETHODS \
	IUnknown_IMETHODS \
	IOleClientSite_METHODS
ICOM_DEFINE(IOleClientSite,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleClientSite_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleClientSite_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleClientSite_Release(p)                 ICOM_CALL (Release,p)
/*** IOleClientSite methods ***/
#define IOleClientSite_SaveObject(p)              ICOM_CALL (SaveObject,p)
#define IOleClientSite_GetMoniker(p,a,b,c)        ICOM_CALL3(GetMoniker,p,a,b,c)
#define IOleClientSite_GetContainer(p,a)          ICOM_CALL1(GetContainer,p,a)
#define IOleClientSite_ShowObject(p)              ICOM_CALL (ShowObject,p)
#define IOleClientSite_OnShowWindow(p,a)          ICOM_CALL1(OnShowWindow,p,a)
#define IOleClientSite_RequestNewObjectLayout(p)  ICOM_CALL (RequestNewObjectLayout(p)


/*****************************************************************************
 * IParseDisplayName interface
 */
#define ICOM_INTERFACE IParseDisplayName
#define IParseDisplayName_METHODS \
	ICOM_METHOD4(HRESULT,ParseDisplayName, IBindCtx*,pbc, LPOLESTR,pszDisplayName, ULONG*,pchEaten, IMoniker**,ppmkOut)
#define IParseDisplayName_IMETHODS \
	IUnknown_IMETHODS \
	IParseDisplayName_METHODS
ICOM_DEFINE(IParseDisplayName,IUnknown)
#undef ICOM_INTERFACE
				
/*** IUnknown methods ***/
#define IParseDisplayName_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IParseDisplayName_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IParseDisplayName_Release(p)                 ICOM_CALL (Release,p)
/*** IParseDisplayName methods ***/
#define IParseDisplayName_ParseDisplayName(p,a,b,c,d) ICOM_CALL4(ParseDisplayName,p,a,b,c,d)
				 
				
/*****************************************************************************
 * IOleContainer interface
 */
#define ICOM_INTERFACE IOleContainer
#define IOleContainer_METHODS \
	ICOM_METHOD2(HRESULT,EnumObjects, DWORD,grfFlags, IEnumUnknown**,ppenum) \
	ICOM_METHOD1(HRESULT,LockContainer, BOOL,fLock)
#define IOleContainer_IMETHODS \
	IParseDisplayName_IMETHODS \
	IOleContainer_METHODS
ICOM_DEFINE(IOleContainer,IParseDisplayName)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleContainer_QueryInterface(p,a,b)       ICOM_CALL2(QueryInterface,p,a,b)
#define IOleContainer_AddRef(p)                   ICOM_CALL (AddRef,p)
#define IOleContainer_Release(p)                  ICOM_CALL (Release,p)
/*** IParseDisplayName methods ***/
#define IOleContainer_ParseDisplayName(p,a,b,c,d) ICOM_CALL4(ParseDisplayName,p,a,b,c,d)
/*** IOleClientSite methods ***/
#define IOleContainer_EnumObjects(p,a,b)          ICOM_CALL (EnumObjects,p,a,b)
#define IOleContainer_LockContainer(p,a)          ICOM_CALL3(LockContainer,p,a)


/*****************************************************************************
 * IOleItemContainer interface
 */
#ifdef __WINE__
#undef GetObject
#endif

#define ICOM_INTERFACE IOleItemContainer
#define IOleItemContainer_METHODS \
	ICOM_METHOD5(HRESULT,GetObject, LPOLESTR,pszItem, DWORD,dwSpeedNeeded, IBindCtx*,pbc, REFIID,riid, void**,ppvObject) \
	ICOM_METHOD4(HRESULT,GetObjectStorage, LPOLESTR,pszItem, IBindCtx*,pbc, REFIID,riid, void**,ppvStorage) \
	ICOM_METHOD1(HRESULT,IsRunning, LPOLESTR,pszItem)
#define IOleItemContainer_IMETHODS \
	IOleContainer_IMETHODS \
	IOleItemContainer_METHODS
ICOM_DEFINE(IOleItemContainer,IOleContainer)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleItemContainer_QueryInterface(p,a,b)       ICOM_CALL2(QueryInterface,p,a,b)
#define IOleItemContainer_AddRef(p)                   ICOM_CALL (AddRef,p)
#define IOleItemContainer_Release(p)                  ICOM_CALL (Release,p)
/*** IParseDisplayName methods ***/
#define IOleItemContainer_GetObject(p,a,b,c,d,e)      ICOM_CALL5(GetObject,p,a,b,c,d,e)
#define IOleItemContainer_GetObjectStorage(p,a,b,c,d) ICOM_CALL4(GetObjectStorage,p,a,b,c,d)
#define IOleItemContainer_IsRunning(p,a)              ICOM_CALL1(IsRunning,p,a)


/*****************************************************************************
 * IOleItemContainer interface
 */
#define ICOM_INTERFACE IOleLink
#define IOleLink_METHODS \
	ICOM_METHOD1(HRESULT,SetUpdateOptions, DWORD,dwUpdateOpt) \
	ICOM_METHOD1(HRESULT,GetUpdateOptions, DWORD*,pdwUpdateOpt) \
	ICOM_METHOD2(HRESULT,SetSourceMoniker, IMoniker*,pmk, REFCLSID,rclsid) \
	ICOM_METHOD1(HRESULT,GetSourceMoniker, IMoniker**,ppmk) \
	ICOM_METHOD1(HRESULT,SetSourceDisplayName, LPCOLESTR,pszStatusText) \
	ICOM_METHOD1(HRESULT,GetSourceDisplayName, LPOLESTR*,ppszDisplayName) \
	ICOM_METHOD2(HRESULT,BindToSource, DWORD,bindflags, IBindCtx*,pbc) \
	ICOM_METHOD (HRESULT,BindIfRunning) \
	ICOM_METHOD1(HRESULT,GetBoundSource, IUnknown**,ppunk) \
	ICOM_METHOD (HRESULT,UnBindSource) \
	ICOM_METHOD1(HRESULT,Update, IBindCtx*,pbc)
#define IOleLink_IMETHODS \
	IUnknown_IMETHODS \
	IOleLink_METHODS
ICOM_DEFINE(IOleLink,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleLink_QueryInterface(p,a,b)       ICOM_CALL2(QueryInterface,p,a,b)
#define IOleLink_AddRef(p)                   ICOM_CALL (AddRef,p)
#define IOleLink_Release(p)                  ICOM_CALL (Release,p)
/*** IOleLink methods ***/
#define IOleLink_SetUpdateOptions(p,a)       ICOM_CALL1(SetUpdateOptions,p,a)
#define IOleLink_GetUpdateOptions(p,a)       ICOM_CALL1(GetUpdateOptions,p,a)
#define IOleLink_SetSourceMoniker(p,a,b)     ICOM_CALL2(SetSourceMoniker,p,a,b)
#define IOleLink_GetSourceMoniker(p,a)       ICOM_CALL1(GetSourceMoniker,p,a)
#define IOleLink_SetSourceDisplayName(p,a)   ICOM_CALL1(SetSourceDisplayName,p,a)
#define IOleLink_GetSourceDisplayName(p,a)   ICOM_CALL1(GetSourceDisplayName,p,a)
#define IOleLink_BindToSource(p,a,b)         ICOM_CALL2(BindToSource,p,a,b)
#define IOleLink_BindIfRunning(p)            ICOM_CALL (BindIfRunning,p)
#define IOleLink_GetBoundSource(p,a)         ICOM_CALL1(GetBoundSource,p,a)
#define IOleLink_UnBindSource(p)             ICOM_CALL (UnBindSource,p)
#define IOleLink_Update(p,a)                 ICOM_CALL1(Update,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_INPLACE_H */


