/*
 * Defines miscellaneous COM interfaces and APIs defined in objidl.h.
 * These did not really fit into the other categories, whould have 
 * required their own specific category or are too rarely used to be 
 * put in 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_MISC_H
#define __WINE_WINE_OBJ_MISC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumString,		0x00000101L, 0, 0);
typedef struct IEnumString IEnumString,*LPENUMSTRING;

DEFINE_OLEGUID(IID_IEnumUnknown,	0x00000100L, 0, 0);
typedef struct IEnumUnknown IEnumUnknown,*LPENUMUNKNOWN;

DEFINE_OLEGUID(IID_IMallocSpy,		0x0000001dL, 0, 0);
typedef struct IMallocSpy IMallocSpy,*LPMALLOCSPY;

DEFINE_OLEGUID(IID_IMultiQI,		0x00000020L, 0, 0);
typedef struct IMultiQI IMultiQI,*LPMULTIQI;


/*****************************************************************************
 * IEnumString interface
 */
#define ICOM_INTERFACE IEnumString
#define IEnumString_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, LPOLESTR*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT, Clone, IEnumString**, ppenum)
#define IEnumString_IMETHODS \
    IUnknown_IMETHODS \
    IEnumString_METHODS
ICOM_DEFINE(IEnumString,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumString_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumString_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumString_Release(p)            ICOM_CALL (Release,p)
/*** IEnumString methods ***/
#define IEnumString_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumString_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumString_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumString_Clone(p,a)    ICOM_CALL1(Clone,p,a)



/*****************************************************************************
 * IEnumUnknown interface
 */
#define ICOM_INTERFACE IEnumUnknown
#define IEnumUnknown_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, IUnknown**,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumUnknown**,ppenum)
#define IEnumUnknown_IMETHODS \
    IUnknown_IMETHODS \
    IEnumUnknown_METHODS
ICOM_DEFINE(IEnumUnknown,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumUnknown_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumUnknown_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumUnknown_Release(p)            ICOM_CALL (Release,p)
/*** IEnumUnknown methods ***/
#define IEnumUnknown_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumUnknown_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumUnknown_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumUnknown_Clone(p,a)    ICOM_CALL1(Clone,p,a)


/*****************************************************************************
 * IMallocSpy interface
 */
#define ICOM_INTERFACE IMallocSpy
#define IMallocSpy_METHODS \
    ICOM_METHOD1 (ULONG,PreAlloc,        ULONG,cbRequest) \
    ICOM_VMETHOD1(      PostAlloc,       void*,pActual) \
    ICOM_METHOD2 (PVOID,PreFree,         void*,pRequest, BOOL,fSpyed) \
    ICOM_VMETHOD1(      PostFree,        BOOL,fSpyed) \
    ICOM_METHOD4 (ULONG,PreRealloc,      void*,pRequest, ULONG,cbRequest, void**,ppNewRequest, BOOL,fSpyed) \
    ICOM_METHOD2 (PVOID,PostRealloc,     void*,pActual, BOOL,fSpyed) \
    ICOM_METHOD2 (PVOID,PreGetSize,      void*,pRequest, BOOL,fSpyed) \
    ICOM_METHOD2 (ULONG,PostGetSize,     ULONG,cbActual, BOOL,fSpyed) \
    ICOM_METHOD2 (PVOID,PreDidAlloc,     void*,pRequest, BOOL,fSpyed) \
    ICOM_METHOD3 (int,  PostDidAlloc,    void*,pRequest, BOOL,fSpyed, int,fActual) \
    ICOM_METHOD  (int,  PreHeapMinimize) \
    ICOM_METHOD  (int,  PostHeapMinimize)
#define IMallocSpy_IMETHODS \
    IUnknown_IMETHODS \
    IMallocSpy_METHODS
ICOM_DEFINE(IMallocSpy,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMallocSpy_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMallocSpy_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMallocSpy_Release(p)            ICOM_CALL (Release,p)
/*** IMallocSpy methods ***/
#define IMallocSpy_PreAlloc(p,a)         ICOM_CALL1(PreAlloc,p,a)
#define IMallocSpy_PostAlloc(p,a)        ICOM_CALL1(PostAlloc,p,a)
#define IMallocSpy_PreFree(p,a,b)        ICOM_CALL2(PreFree,p,a,b)
#define IMallocSpy_PostFree(p,a)         ICOM_CALL1(PostFree,p,a)
#define IMallocSpy_PreRealloc(p,a,b,c,d) ICOM_CALL4(PreRealloc,p,a,b,c,d)
#define IMallocSpy_PostRealloc(p,a,b)    ICOM_CALL2(PostRealloc,p,a,b)
#define IMallocSpy_PreGetSize(p,a,b)     ICOM_CALL2(PreGetSize,p,a,b)
#define IMallocSpy_PostGetSize(p,a,b)    ICOM_CALL2(PostGetSize,p,a,b)
#define IMallocSpy_PreDidAlloc(p,a,b)    ICOM_CALL2(PreDidAlloc,p,a,b)
#define IMallocSpy_PostDidAlloc(p,a,b,c) ICOM_CALL3(PostDidAlloc,p,a,b,c)
#define IMallocSpy_PreHeapMinimize(p)    ICOM_CALL (PreHeapMinimize,p)
#define IMallocSpy_PostHeapMinimize(p)   ICOM_CALL (PostHeapMinimize,p)

/* FIXME: not implemented */
HRESULT WINAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy);

/* FIXME: not implemented */
HRESULT WINAPI CoRevokeMallocSpy(void);

HRESULT WINAPI CoFileTimeNow(FILETIME* lpFileTime);


/*****************************************************************************
 * IMultiQI interface
 */
typedef struct tagMULTI_QI
{
    const IID* pIID;
    IUnknown* pItf;
    HRESULT hr;
} MULTI_QI;

#define ICOM_INTERFACE IMultiQI
#define IMultiQI_METHODS \
    ICOM_METHOD2(HRESULT,QueryMultipleInterfaces, ULONG,cMQIs, MULTI_QI*,pMQIs)
#define IMultiQI_IMETHODS \
    IUnknown_IMETHODS \
    IMultiQI_METHODS
ICOM_DEFINE(IMultiQI,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMultiQI_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMultiQI_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMultiQI_Release(p)            ICOM_CALL (Release,p)
/*** IMultiQI methods ***/
#define IMultiQI_QueryMultipleInterfaces(p,a,b) ICOM_CALL2(QueryMultipleInterfaces,p,a,b)


/*****************************************************************************
 * Additional API
 */

DWORD WINAPI CoBuildVersion(void);

DWORD WINAPI CoGetCurrentProcess(void);

/* FIXME: unimplemented */
HRESULT WINAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew);

/* FIXME: unimplemented */
HRESULT WINAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);

HRESULT WINAPI CoCreateInstance(
	REFCLSID rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD dwClsContext,
	REFIID iid,
	LPVOID *ppv);

HRESULT WINAPI CoCreateInstanceEx(REFCLSID      rclsid, 
				  LPUNKNOWN     pUnkOuter,
				  DWORD         dwClsContext, 
				  COSERVERINFO* pServerInfo,
				  ULONG         cmq,
				  MULTI_QI*     pResults);
#ifdef __cplusplus
} /*  extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_MISC_H */
