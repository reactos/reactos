/*
 * Defines the COM interfaces and APIs related to ViewObject
 *
 */

#ifndef __WINE_WINE_OBJ_OLEVIEW_H
#define __WINE_WINE_OBJ_OLEVIEW_H

struct tagLOGPALETTE;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */


/*****************************************************************************
 * Predeclare the interfaces
 */

DEFINE_OLEGUID(IID_IViewObject,  0x0000010dL, 0, 0);
typedef struct IViewObject IViewObject, *LPVIEWOBJECT;

DEFINE_OLEGUID(IID_IViewObject2,  0x00000127L, 0, 0);
typedef struct IViewObject2 IViewObject2, *LPVIEWOBJECT2;

/*****************************************************************************
 * IViewObject interface
 */
typedef BOOL    CALLBACK (*IVO_ContCallback)(DWORD);

#define ICOM_INTERFACE IViewObject
#define IViewObject_METHODS \
	ICOM_METHOD10(HRESULT,Draw, DWORD,dwDrawAspect, LONG,lindex, void*,pvAspect, DVTARGETDEVICE*,ptd, HDC,hdcTargetDev, HDC,hdcDraw, LPCRECTL,lprcBounds, LPCRECTL,lprcWBounds, IVO_ContCallback, pfnContinue, DWORD,dwContinue) \
	ICOM_METHOD6(HRESULT,GetColorSet, DWORD,dwDrawAspect, LONG,lindex, void*,pvAspect, DVTARGETDEVICE*,ptd, HDC,hicTargetDevice, struct tagLOGPALETTE**,ppColorSet) \
	ICOM_METHOD4(HRESULT,Freeze, DWORD,dwDrawAspect, LONG,lindex, void*,pvAspect, DWORD*,pdwFreeze) \
	ICOM_METHOD1(HRESULT,Unfreeze, DWORD,dwFreeze) \
	ICOM_METHOD3(HRESULT,SetAdvise, DWORD,aspects, DWORD,advf, IAdviseSink*,pAdvSink) \
	ICOM_METHOD3(HRESULT,GetAdvise, DWORD*,pAspects, DWORD*,pAdvf, IAdviseSink**,ppAdvSink) 
#define IViewObject_IMETHODS \
	IUnknown_IMETHODS \
	IViewObject_METHODS
ICOM_DEFINE(IViewObject,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IViewObject_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IViewObject_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IViewObject_Release(p)                   ICOM_CALL (Release,p)
/*** IViewObject methods ***/
#define IViewObject_Draw(p,a,b,c,d,e,f,g,h,i,j)  ICOM_CALL10(Draw,p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject_GetColorSet(p,a,b,c,d,e,f)   ICOM_CALL6(GetColorSet,p,a,b,c,d,e,f)
#define IViewObject_Freeze(p,a,b,c,d)            ICOM_CALL4(Freeze,p,a,b,c,d)
#define IViewObject_Unfreeze(p,a)                ICOM_CALL1(Unfreeze,p,a)
#define IViewObject_SetAdvise(p,a,b,c)           ICOM_CALL3(SetAdvise,p,a,b,c)
#define IViewObject_GetAdvise(p,a,b,c)           ICOM_CALL3(GetAdvise,p,a,b,c)
				  


/*****************************************************************************
 * IViewObject2 interface
 */
#define ICOM_INTERFACE IViewObject2
#define IViewObject2_METHODS \
	ICOM_METHOD4(HRESULT,GetExtent, DWORD,dwDrawAspect, LONG,lindex, DVTARGETDEVICE*,ptd, LPSIZEL,lpsizel) 
#define IViewObject2_IMETHODS \
	IViewObject_IMETHODS \
	IViewObject2_METHODS
ICOM_DEFINE(IViewObject2,IViewObject)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IViewObject2_QueryInterface(p,a,b)        ICOM_CALL2(QueryInterface,p,a,b)
#define IViewObject2_AddRef(p)                    ICOM_CALL (AddRef,p)
#define IViewObject2_Release(p)                   ICOM_CALL (Release,p)
/*** IViewObject methods ***/
#define IViewObject2_Draw(p,a,b,c,d,e,f,g,h,i,j)  ICOM_CALL10(Draw,p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject2_GetColorSet(p,a,b,c,d,e,f)   ICOM_CALL6(GetColorSet,p,a,b,c,d,e,f)
#define IViewObject2_Freeze(p,a,b,c,d)            ICOM_CALL4(Freeze,p,a,b,c,d)
#define IViewObject2_Unfreeze(p,a)                ICOM_CALL1(Unfreeze,p,a)
#define IViewObject2_SetAdvise(p,a,b,c)           ICOM_CALL3(SetAdvise,p,a,b,c)
#define IViewObject2_GetAdvise(p,a,b,c)           ICOM_CALL3(GetAdvise,p,a,b,c)
/*** IViewObject2 methods ***/
#define IViewObject2_GetExtent(p,a,b,c,d)         ICOM_CALL4(GetExtent,p,a,b,c,d)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEVIEW_H */

