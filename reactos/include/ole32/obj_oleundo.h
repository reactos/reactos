/*
 * Defines the COM interfaces and APIs from ocidl.h which pertain to Undo/Redo
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_OLEUNDO_H
#define __WINE_WINE_OBJ_OLEUNDO_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IQuickActivate, 0xcf51ed10, 0x62fe, 0x11cf, 0xbf, 0x86, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0x36);
typedef struct IQuickActivate IQuickActivate,*LPQUICKACTIVATE;
 
DEFINE_GUID(IID_IPointerInactive, 0x55980ba0, 0x35aa, 0x11cf, 0xb6, 0x71, 0x00, 0xaa, 0x00, 0x4c, 0xd6, 0xd8);
typedef struct IPointerInactive IPointerInactive,*LPPOINTERINACTIVE;

DEFINE_GUID(IID_IAdviseSinkEx, 0x3af24290, 0x0c96, 0x11ce, 0xa0, 0xcf, 0x00, 0xaa, 0x00, 0x60, 0x0a, 0xb8);
typedef struct IAdviseSinkEx IAdviseSinkEx,*LPADVISESINKEX;

DEFINE_GUID(IID_IOleUndoManager, 0xd001f200, 0xef97, 0x11ce, 0x9b, 0xc9, 0x00, 0xaa, 0x00, 0x60, 0x8e, 0x01);
typedef struct IOleUndoManager IOleUndoManager,*LPOLEUNDOMANAGER;

DEFINE_GUID(IID_IOleUndoUnit, 0x894ad3b0, 0xef97, 0x11ce, 0x9b, 0xc9, 0x00, 0xaa, 0x00, 0x60, 0x8e, 0x01);
typedef struct IOleUndoUnit IOleUndoUnit,*LPOLEUNDOUNIT;

DEFINE_GUID(IID_IOleParentUndoUnit, 0xa1faf330, 0xef97, 0x11ce, 0x9b, 0xc9, 0x00, 0xaa, 0x00, 0x60, 0x8e, 0x01);
typedef struct IOleParentUndoUnit IOleParentUndoUnit,*LPOLEPARENTUNDOUNIT;

DEFINE_GUID(IID_IEnumOleUndoUnits, 0xb3e7c340, 0xef97, 0x11ce, 0x9b, 0xc9, 0x00, 0xaa, 0x00, 0x60, 0x8e, 0x01);
typedef struct IEnumOleUndoUnits IEnumOleUndoUnits,*LPENUMOLEUNDOUNITS;

/*****************************************************************************
 * Declare the structures
 */
typedef enum tagQACONTAINERFLAGS
{
	QACONTAINER_SHOWHATCHING = 0x1,
	QACONTAINER_SHOWGRABHANDLES = 0x2,
	QACONTAINER_USERMODE = 0x4,
	QACONTAINER_DISPLAYASDEFAULT = 0x8,
	QACONTAINER_UIDEAD = 0x10,
	QACONTAINER_AUTOCLIP = 0x20,
	QACONTAINER_MESSAGEREFLECT = 0x40,
	QACONTAINER_SUPPORTSMNEMONICS = 0x80
} QACONTAINERFLAGS;

typedef DWORD OLE_COLOR;

typedef struct tagQACONTROL
{
	ULONG cbSize;
	DWORD dwMiscStatus;
	DWORD dwViewStatus;
	DWORD dwEventCookie;
	DWORD dwPropNotifyCookie;
	DWORD dwPointerActivationPolicy;
} QACONTROL;

typedef struct tagQACONTAINER
{
	ULONG cbSize;
	IOleClientSite *pClientSite;
	IAdviseSinkEx *pAdviseSink;
	IPropertyNotifySink *pPropertyNotifySink;
	IUnknown *pUnkEventSink;
	DWORD dwAmbientFlags;
	OLE_COLOR colorFore;
	OLE_COLOR colorBack;
	IFont *pFont;
	IOleUndoManager *pUndoMgr;
	DWORD dwAppearance;
	LONG lcid;
	HPALETTE hpal;
	struct IBindHost *pBindHost;
} QACONTAINER;

/*****************************************************************************
 * IQuickActivate interface
 */
#define ICOM_INTERFACE IQuickActivate
#define IQuickActivate_METHODS \
	ICOM_METHOD2(HRESULT,QuickActivate, QACONTAINER*,pQaContainer, QACONTROL*,pQaControl) \
	ICOM_METHOD1(HRESULT,SetContentExtent, LPSIZEL,pSizel) \
	ICOM_METHOD1(HRESULT,GetContentExtent, LPSIZEL,pSizel)
#define IQuickActivate_IMETHODS \
	IUnknown_IMETHODS \
	IQuickActivate_METHODS
ICOM_DEFINE(IQuickActivate,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IQuickActivate_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IQuickActivate_AddRef(p)             ICOM_CALL (AddRef,p)
#define IQuickActivate_Release(p)            ICOM_CALL (Release,p)
/*** IQuickActivate methods ***/
#define IQuickActivate_QuickActivate(p,a,b)  ICOM_CALL2(QuickActivate,p,a,b)
#define IQuickActivate_SetContentExtent(p,a) ICOM_CALL1(SetContentExtent,p,a)
#define IQuickActivate_GetContentExtent(p,a) ICOM_CALL1(GetContentExtent,p,a)


/*****************************************************************************
 * IPointerInactive interface
 */
#define ICOM_INTERFACE IPointerInactive
#define IPointerInactive_METHODS \
	ICOM_METHOD1(HRESULT,GetActivationPolicy, DWORD*,pdwPolicy) \
	ICOM_METHOD4(HRESULT,OnInactiveMouseMove, LPCRECT,pRectBounds, LONG,x, LONG,y, DWORD,grfKeyState) \
	ICOM_METHOD5(HRESULT,OnInactiveSetCursor, LPCRECT,pRectBounds, LONG,x, LONG,y, DWORD,dwMouseMsg, BOOL,fSetAlways) 
#define IPointerInactive_IMETHODS \
	IUnknown_IMETHODS \
	IPointerInactive_METHODS
ICOM_DEFINE(IPointerInactive,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPointerInactive_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPointerInactive_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPointerInactive_Release(p)            ICOM_CALL (Release,p)
/*** IPointerInactive methods ***/
#define IPointerInactive_GetActivationPolicy(p,a)         ICOM_CALL1(GetActivationPolicy,p,a)
#define IPointerInactive_OnInactiveMoveMouse(p,a,b,c,d)   ICOM_CALL4(OnInactiveMoveMouse,p,a,b,c,d) 
#define IPointerInactive_OnInactiveSetCursor(p,a,b,c,d,e) ICOM_CALL5(OnInactiveSetCursor,p,a,b,d,e)


/*****************************************************************************
 * IAdviseSinkEx interface
 */
#define ICOM_INTERFACE IAdviseSinkEx
#define IAdviseSinkEx_METHODS \
	ICOM_METHOD1(HRESULT,OnViewStatusChange, DWORD,dwViewStatus)
#define IAdviseSinkEx_IMETHODS \
	IAdviseSink_IMETHODS \
	IAdviseSinkEx_METHODS
ICOM_DEFINE(IAdviseSinkEx,IAdviseSink)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IAdviseSinkEx_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IAdviseSinkEx_AddRef(p)             ICOM_CALL (AddRef,p)
#define IAdviseSinkEx_Release(p)            ICOM_CALL (Release,p)
/*** IAdviseSink methods ***/
#define IAdviseSinkEx_OnDataChange(p,a,b)   ICOM_CALL2(OnDataChange,p,a,b)
#define IAdviseSinkEx_OnViewChange(p,a,b)   ICOM_CALL2(OnViewChange,p,a,b)
#define IAdviseSinkEx_OnRename(p,a)         ICOM_CALL1(OnRename,p,a)
#define IAdviseSinkEx_OnSave(p)             ICOM_CALL (OnSave,p)
#define IAdviseSinkEx_OnClose(p)            ICOM_CALL (OnClose,p)
/*** IAdviseSinkEx methods ***/
#define IAdviseSinkEx_OnViewStatusChange(p,a)  ICOM_CALL1(OnViewStatusChange,p,a)


/*****************************************************************************
 * IOleUndoManager interface
 */
#define ICOM_INTERFACE IOleUndoManager
#define IOleUndoManager_METHODS \
	ICOM_METHOD1(HRESULT,Open, IOleParentUndoUnit*,pPUU) \
	ICOM_METHOD2(HRESULT,Close, IOleParentUndoUnit*,pPUU, BOOL,fCommit) \
	ICOM_METHOD1(HRESULT,Add, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,GetOpenParentState, DWORD*,pdwState) \
	ICOM_METHOD1(HRESULT,DiscardFrom, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,UndoTo, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,RedoTo, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,EnumUndoable, IEnumOleUndoUnits**,ppEnum) \
	ICOM_METHOD1(HRESULT,EnumRedoable, IEnumOleUndoUnits**,ppEnum) \
	ICOM_METHOD1(HRESULT,GetLastUndoDescription, BSTR*,pBstr) \
	ICOM_METHOD1(HRESULT,GetLastRedoDescription, BSTR*,pBstr) \
	ICOM_METHOD1(HRESULT,Enable, BOOL,fEnable)
#define IOleUndoManager_IMETHODS \
	IUnknown_IMETHODS \
	IOleUndoManager_METHODS
ICOM_DEFINE(IOleUndoManager,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleUndoManager_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IOleUndoManager_AddRef(p)             ICOM_CALL (AddRef,p)
#define IOleUndoManager_Release(p)            ICOM_CALL (Release,p)
/*** IOleUndoManager methods ***/
#define IOleUndoManager_Open(p,a)                   ICOM_CALL1(Open,p,a)
#define IOleUndoManager_Close(p,a,b)                ICOM_CALL2(Close,p,a,b)
#define IOleUndoManager_Add(p,a)                    ICOM_CALL1(Add,p,a)
#define IOleUndoManager_GetOpenParentState(p,a)     ICOM_CALL1(GetOpenParentState,p,a)
#define IOleUndoManager_DiscardFrom(p,a)            ICOM_CALL1(DiscardFrom,p,a)
#define IOleUndoManager_UndoTo(p,a)                 ICOM_CALL1(UndoTo,p,a)
#define IOleUndoManager_RedoTo(p,a)                 ICOM_CALL1(RedoTo,p,a)
#define IOleUndoManager_EnumUndoable(p,a)           ICOM_CALL1(EnumUndoable,p,a)
#define IOleUndoManager_EnumRedoable(p,a)           ICOM_CALL1(EnumRedoable,p,a)
#define IOleUndoManager_GetLastUndoDescription(p,a) ICOM_CALL1(GetLastUndoDescription,p,a)
#define IOleUndoManager_GetLastRedoDescription(p,a) ICOM_CALL1(GetLastRedoDescription,p,a)
#define IOleUndoManager_Enable(p,a)                 ICOM_CALL1(Enable,p,a)


/*****************************************************************************
 * IOleUndoUnit interface
 */
#define ICOM_INTERFACE IOleUndoUnit
#define IOleUndoUnit_METHODS \
	ICOM_METHOD1(HRESULT,Do, IOleUndoManager*,pUndoManager) \
	ICOM_METHOD1(HRESULT,GetDescription, BSTR*,pBstr) \
	ICOM_METHOD2(HRESULT,GetUnitType, CLSID*,pClsid, LONG*,plID) \
	ICOM_METHOD (HRESULT,OnNextAdd)
#define IOleUndoUnit_IMETHODS \
	IUnknown_IMETHODS \
	IOleUndoUnit_METHODS
ICOM_DEFINE(IOleUndoUnit,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleUndoUnit_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IOleUndoUnit_AddRef(p)             ICOM_CALL (AddRef,p)
#define IOleUndoUnit_Release(p)            ICOM_CALL (Release,p)
/*** IOleUndoUnit methods ***/
#define IOleUndoUnit_Do(p,a)               ICOM_CALL1(Do,p,a)
#define IOleUndoUnit_GetDescription(p,a)   ICOM_CALL1(GetDescription,p,a)
#define IOleUndoUnit_GetUnitType(p,a,b)    ICOM_CALL2(GetUnitType,p,a,b)
#define IOleUndoUnit_OnNextAdd(p)          ICOM_CALL (OnNextAdd,p)



/*****************************************************************************
 * IOleUndoUnit interface
 */
#define ICOM_INTERFACE IOleParentUndoUnit
#define IOleParentUndoUnit_METHODS \
	ICOM_METHOD1(HRESULT,Open, IOleParentUndoUnit*,pPUU) \
	ICOM_METHOD2(HRESULT,Close, IOleParentUndoUnit*,pPUU, BOOL,fCommit) \
	ICOM_METHOD1(HRESULT,Add, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,FindUnit, IOleUndoUnit*,pUU) \
	ICOM_METHOD1(HRESULT,GetParentState, DWORD*,pdwState)
#define IOleParentUndoUnit_IMETHODS \
	IOleUndoUnit_IMETHODS \
	IOleParentUndoUnit_METHODS
ICOM_DEFINE(IOleParentUndoUnit,IOleUndoUnit)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleParentUndoUnit_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IOleParentUndoUnit_AddRef(p)             ICOM_CALL (AddRef,p)
#define IOleParentUndoUnit_Release(p)            ICOM_CALL (Release,p)
/*** IOleUndoUnit methods ***/
#define IOleParentUndoUnit_Do(p,a)               ICOM_CALL1(Do,p,a)
#define IOleParentUndoUnit_GetDescription(p,a)   ICOM_CALL1(GetDescription,p,a)
#define IOleParentUndoUnit_GetUnitType(p,a,b)    ICOM_CALL2(GetUnitType,p,a,b)
#define IOleParentUndoUnit_OnNextAdd(p)          ICOM_CALL (OnNextAdd,p)
/*** IOleParentUndoUnit methods ***/
#define IOleParentUndoUnit_Open(p,a)             ICOM_CALL1(Open,p,a)
#define IOleParentUndoUnit_Close(p,a,b)          ICOM_CALL2(Close,p,a,b)
#define IOleParentUndoUnit_Add(p,a)              ICOM_CALL1(Add,p,a)
#define IOleParentUndoUnit_FindUnit(p,a)         ICOM_CALL1(FindUnit,p,a)
#define IOleParentUndoUnit_GetParentState(p,a,b) ICOM_CALL1(GetParentState,p,a)


/*****************************************************************************
 * IEnumOleUndoUnits interface
 */
#define ICOM_INTERFACE IEnumOleUndoUnits
#define IEnumOleUndoUnits_METHODS \
	ICOM_METHOD3(HRESULT,Next, ULONG,cElt, IOleUndoUnit**,rgElt, ULONG*,pcEltFetched) \
	ICOM_METHOD1(HRESULT,Skip, ULONG,cElt) \
	ICOM_METHOD (HRESULT,Reset) \
	ICOM_METHOD1(HRESULT,Clone, IEnumOleUndoUnits**,ppEnum)
#define IEnumOleUndoUnits_IMETHODS \
	IUnknown_IMETHODS \
	IEnumOleUndoUnits_METHODS
ICOM_DEFINE(IEnumOleUndoUnits,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumOleUndoUnits_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumOleUndoUnits_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumOleUndoUnits_Release(p)            ICOM_CALL (Release,p)
/*** IEnumOleUndoUnits methods ***/
#define IEnumOleUndoUnits_Next(p,a,b,c)         ICOM_CALL3(Next,p,a,b,c)
#define IEnumOleUndoUnits_Skip(p,a)             ICOM_CALL1(Skip,p,a)
#define IEnumOleUndoUnits_Reset(p,a)            ICOM_CALL (Reset,p,a)
#define IEnumOleUndoUnits_Clone(p,a)            ICOM_CALL1(Clone,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEUNDO_H */

