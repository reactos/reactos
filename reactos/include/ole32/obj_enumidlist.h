/*
 * Defines the COM interfaces and APIs related to EnumIDList
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_ENUMIDLIST_H
#define __WINE_WINE_OBJ_ENUMIDLIST_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IEnumIDList IEnumIDList, *LPENUMIDLIST;

#define ICOM_INTERFACE IEnumIDList
#define IEnumIDList_METHODS \
    ICOM_METHOD3(HRESULT, Next, ULONG, celt, LPITEMIDLIST*, rgelt, ULONG*, pceltFetched) \
    ICOM_METHOD1(HRESULT, Skip, ULONG, celt) \
    ICOM_METHOD (HRESULT, Reset) \
    ICOM_METHOD1(HRESULT, Clone, IEnumIDList**, ppenum)
#define IEnumIDList_IMETHODS \
    IUnknown_IMETHODS \
    IEnumIDList_METHODS
ICOM_DEFINE(IEnumIDList,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumIDList_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumIDList_AddRef(p)			ICOM_CALL (AddRef,p)
#define IEnumIDList_Release(p)			ICOM_CALL (Release,p)
/*** IEnumIDList methods ***/
#define IEnumIDList_Next(p,a,b,c)		ICOM_CALL3(Next,p,a,b,c)
#define IEnumIDList_Skip(p,a)			ICOM_CALL1(Skip,p,a)
#define IEnumIDList_Reset(p)			ICOM_CALL(Reset,p)
#define IEnumIDList_Clone(p,a)			ICOM_CALL1(Clone,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_ENUMIDLIST_H */
