/*
 * Defines the COM interfaces and APIs related to IServiceProvider
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_SERVICEPROVIDER_H
#define __WINE_WINE_OBJ_SERVICEPROVIDER_H

#include "wine/obj_base.h"
#include "winbase.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IServiceProvider, 0x6d5140c1, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
typedef struct IServiceProvider IServiceProvider, *LPSERVICEPROVIDER;


/*****************************************************************************
 * IServiceProvider interface
 */
#define ICOM_INTERFACE IServiceProvider
#define IServiceProvider_METHODS \
    ICOM_METHOD3( HRESULT, QueryService, REFGUID, guidService, REFIID, riid, void**, ppv)
#define IServiceProvider_IMETHODS \
    IUnknown_IMETHODS \
    IServiceProvider_METHODS
ICOM_DEFINE(IServiceProvider,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IServiceProvider_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IServiceProvider_AddRef(p)			ICOM_CALL (AddRef,p)
#define IServiceProvider_Release(p)			ICOM_CALL (Release,p)
/*** IServiceProvider methods ***/
#define IServiceProvider_QueryService(p,a,b,c)		ICOM_CALL3(QueryService,p,a,b,c)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SERVICEPROVIDER_H */
