/*
 * Defines IOleObject COM and other oleidl.h interfaces
 * 
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_OLEOBJ_H
#define __WINE_WINE_OBJ_OLEOBJ_H

struct tagMSG;
struct tagLOGPALETTE;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */
typedef struct tagOBJECTDESCRIPTOR
{
	ULONG cbSize;
	CLSID clsid;
	DWORD dwDrawAspect;
	SIZEL sizel;
	POINTL pointl;
	DWORD dwStatus;
	DWORD dwFullUserTypeName;
	DWORD dwSrcOfCopy;
} OBJECTDESCRIPTOR, *LPOBJECTDESCRIPTOR;
	
typedef enum tagOLEMISC
{
	OLEMISC_RECOMPOSEONRESIZE = 0x1,
	OLEMISC_ONLYICONIC = 0x2,
	OLEMISC_INSERTNOTREPLACE = 0x4,
	OLEMISC_STATIC = 0x8,
	OLEMISC_CANTLINKINSIDE = 0x10,
	OLEMISC_CANLINKBYOLE1 = 0x20,
	OLEMISC_ISLINKOBJECT = 0x40,
	OLEMISC_INSIDEOUT = 0x80,
	OLEMISC_ACTIVATEWHENVISIBLE = 0x100,
	OLEMISC_RENDERINGISDEVICEINDEPENDENT = 0x200,
	OLEMISC_INVISIBLEATRUNTIME = 0x400,
	OLEMISC_ALWAYSRUN = 0x800,
	OLEMISC_ACTSLIKEBUTTON = 0x1000,
	OLEMISC_ACTSLIKELABEL = 0x2000,
	OLEMISC_NOUIACTIVATE = 0x4000,
	OLEMISC_ALIGNABLE = 0x8000,
	OLEMISC_SIMPLEFRAME = 0x10000,
	OLEMISC_SETCLIENTSITEFIRST = 0x20000,
	OLEMISC_IMEMODE = 0x40000,
	OLEMISC_IGNOREACTIVATEWHENVISIBLE = 0x80000,
	OLEMISC_WANTSTOMENUMERGE = 0x100000,
	OLEMISC_SUPPORTSMULTILEVELUNDO = 0x200000
} OLEMISC;

typedef enum tagOLEVERBATTRIB
{
	OLEVERBATTRIB_NEVERDIRTIES = 1,
	OLEVERBATTRIB_ONCONTAINERMENU = 2
} OLEVERBATTRIB;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IOleObject,  0x00000112L, 0, 0);
typedef struct IOleObject IOleObject, *LPOLEOBJECT;

DEFINE_OLEGUID(IID_IOleAdviseHolder,  0x00000111L, 0, 0);
typedef struct IOleAdviseHolder IOleAdviseHolder, *LPOLEADVISEHOLDER;

DEFINE_OLEGUID(IID_IEnumOLEVERB,  0x00000104L, 0, 0);
typedef struct IEnumOLEVERB IEnumOLEVERB, *LPENUMOLEVERB;
  
/*****************************************************************************
 * IOleObject interface
 */
#define ICOM_INTERFACE IOleObject
#define IOleObject_METHODS \
	ICOM_METHOD1(HRESULT,SetClientSite, IOleClientSite*,pClientSite) \
	ICOM_METHOD1(HRESULT,GetClientSite, IOleClientSite**,ppClientSite) \
	ICOM_METHOD2(HRESULT,SetHostNames, LPCOLESTR,szContainerApp, LPCOLESTR,szContainerObj) \
	ICOM_METHOD1(HRESULT,Close, DWORD,dwSaveOption) \
	ICOM_METHOD2(HRESULT,SetMoniker, DWORD,dwWhichMoniker, IMoniker*,pmk) \
	ICOM_METHOD3(HRESULT,GetMoniker, DWORD,dwAssign, DWORD,dwWhichMoniker, IMoniker**,ppmk) \
	ICOM_METHOD3(HRESULT,InitFromData, IDataObject*,pDataObject, BOOL,fCreation, DWORD,dwReserved) \
	ICOM_METHOD2(HRESULT,GetClipboardData, DWORD,dwReserved, IDataObject**,ppDataObject) \
	ICOM_METHOD6(HRESULT,DoVerb, LONG,iVerb, struct tagMSG*,lpmsg, IOleClientSite*,pActiveSite, LONG,lindex, HWND,hwndParent, LPCRECT,lprcPosRect) \
	ICOM_METHOD1(HRESULT,EnumVerbs, IEnumOLEVERB**,ppEnumOleVerb) \
	ICOM_METHOD (HRESULT,Update) \
	ICOM_METHOD (HRESULT,IsUpToDate) \
	ICOM_METHOD1(HRESULT,GetUserClassID, CLSID*,pClsid) \
	ICOM_METHOD2(HRESULT,GetUserType, DWORD,dwFormOfType, LPOLESTR*,pszUserType) \
	ICOM_METHOD2(HRESULT,SetExtent, DWORD,dwDrawAspect, SIZEL*,psizel) \
	ICOM_METHOD2(HRESULT,GetExtent, DWORD,dwDrawAspect, SIZEL*,psizel) \
	ICOM_METHOD2(HRESULT,Advise, IAdviseSink*,pAdvSink, DWORD*,pdwConnection) \
	ICOM_METHOD1(HRESULT,Unadvise, DWORD,dwConnection) \
	ICOM_METHOD1(HRESULT,EnumAdvise, IEnumSTATDATA**,ppenumAdvise) \
	ICOM_METHOD2(HRESULT,GetMiscStatus, DWORD,dwAspect, DWORD*,pdwStatus) \
	ICOM_METHOD1(HRESULT,SetColorScheme, struct tagLOGPALETTE*,pLogpal)
#define IOleObject_IMETHODS \
	IUnknown_IMETHODS \
	IOleObject_METHODS
ICOM_DEFINE(IOleObject,IUnknown)
#undef ICOM_INTERFACE
				 
/*** IUnknown methods ***/
#define IOleObject_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleObject_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleObject_Release(p)                 ICOM_CALL (Release,p)
/*** IOleObject methods ***/
#define IOleObject_SetClientSite(p,a)       ICOM_CALL1(SetClientSite,p,a)
#define IOleObject_GetClientSite(p,a,b)     ICOM_CALL1(GetClientSite,p,a)
#define IOleObject_SetHostNames(p,a,b)      ICOM_CALL2(SetHostNames,p,a,b)
#define IOleObject_Close(p,a,b)             ICOM_CALL1(Close,p,a)
#define IOleObject_SetMoniker(p,a,b)        ICOM_CALL2(SetMoniker,p,a,b)
#define IOleObject_GetMoniker(p,a,b)        ICOM_CALL3(GetMoniker,p,a,b,c)
#define IOleObject_InitFromData(p,a,b)      ICOM_CALL3(InitFromData,p,a,b,c)
#define IOleObject_GetClipboardData(p,a,b)  ICOM_CALL2(GetClipboardData,p,a,b)
#define IOleObject_DoVerb(p,a,b)            ICOM_CALL6(DoVerb,p,a,b,c,d,e,f)
#define IOleObject_EnumVerbs(p,a,b)         ICOM_CALL1(EnumVerbs,p,a)
#define IOleObject_Update(p,a,b)            ICOM_CALL (Update,p)
#define IOleObject_IsUpToDate(p,a,b)        ICOM_CALL (IsUpToDate,p)
#define IOleObject_GetUserClassID(p,a,b)    ICOM_CALL1(GetUserClassID,p,a)
#define IOleObject_GetUserType(p,a,b)       ICOM_CALL2(GetUserType,p,a,b)
#define IOleObject_SetExtent(p,a,b)         ICOM_CALL2(SetExtent,p,a,b)
#define IOleObject_GetExtent(p,a,b)         ICOM_CALL2(GetExtent,p,a,b)
#define IOleObject_Advise(p,a,b)            ICOM_CALL2(Advise,p,a,b)
#define IOleObject_Unadvise(p,a,b)          ICOM_CALL1(Unadvise,p,a)
#define IOleObject_EnumAdvise(p,a,b)        ICOM_CALL1(EnumAdvise,p,a)
#define IOleObject_GetMiscStatus(p,a,b)     ICOM_CALL2(GetMiscStatus,p,a,b)
#define IOleObject_SetColorScheme(p,a,b)    ICOM_CALL1(SetColorScheme,p,a)
				 

/*****************************************************************************
 * IOleAdviseHolder interface
 */
#define ICOM_INTERFACE IOleAdviseHolder
#define IOleAdviseHolder_METHODS \
	ICOM_METHOD2(HRESULT,Advise, IAdviseSink*,pAdvise, DWORD*,pdwConnection) \
	ICOM_METHOD1(HRESULT,Unadvise, DWORD,dwConnection) \
	ICOM_METHOD1(HRESULT,EnumAdvise, IEnumSTATDATA**,ppenumAdvise) \
	ICOM_METHOD1(HRESULT,SendOnRename, IMoniker*,pmk) \
	ICOM_METHOD (HRESULT,SendOnSave) \
	ICOM_METHOD (HRESULT,SendOnClose)
#define IOleAdviseHolder_IMETHODS \
	IUnknown_IMETHODS \
	IOleAdviseHolder_METHODS
ICOM_DEFINE(IOleAdviseHolder,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IOleAdviseHolder_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IOleAdviseHolder_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IOleAdviseHolder_Release(p)                 ICOM_CALL (Release,p)
/*** IOleAdviseHolder methods ***/
#define IOleAdviseHolder_Advise(p,a,b)              ICOM_CALL2(Advise,p,a,b)
#define IOleAdviseHolder_Unadvise(p,a)              ICOM_CALL1(Unadvise,p,a)
#define IOleAdviseHolder_EnumAdvise(p,a)            ICOM_CALL1(EnumAdvise,p,a)
#define IOleAdviseHolder_SendOnRename(p,a)          ICOM_CALL1(SendOnRename,p,a)
#define IOleAdviseHolder_SendOnSave(p)              ICOM_CALL (SendOnSave,p)
#define IOleAdviseHolder_SendOnClose(p)             ICOM_CALL (SendOnClose,p)
				 

/*****************************************************************************
 *  IEnumOLEVERB interface
 */
#define ICOM_INTERFACE IEnumOLEVERB
#define IEnumOLEVERB_METHODS \
	ICOM_METHOD3(HRESULT,Next, ULONG,celt, LPOLEVERB,rgelt, ULONG*,pceltFetched) \
	ICOM_METHOD1(HRESULT,Skip, ULONG,celt) \
	ICOM_METHOD (HRESULT,Reset) \
	ICOM_METHOD1(HRESULT,Clone, IEnumOLEVERB**,ppenum)
#define IEnumOLEVERB_IMETHODS \
	IUnknown_IMETHODS \
	IEnumOLEVERB_METHODS
ICOM_DEFINE(IEnumOLEVERB,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumOLEVERB_QueryInterface(p,a,b)  ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IEnumOLEVERB_AddRef(p)              ICOM_ICALL (IUnknown,AddRef,p)
#define IEnumOLEVERB_Release(p)             ICOM_ICALL (IUnknown,Release,p)
/*** IEnumOLEVERB methods ***/
#define IEnumOLEVERB_Next(p,a,b,c)          ICOM_CALL3(Next,p,a,b,c)
#define IEnumOLEVERB_Skip(p,a)              ICOM_CALL1(Skip,p,a)
#define IEnumOLEVERB_Reset(p,a)             ICOM_CALL (Reset,p)
#define IEnumOLEVERB_Clone(p,a)             ICOM_CALL1(Clone,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */
				  
#endif /* __WINE_WINE_OBJ_OLEOBJ_H */

