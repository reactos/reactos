/*
 * Defines the COM interfaces and APIs related to the moniker functionality.
 */
#ifndef __WINE_WINE_OBJ_MONIKER_H
#define __WINE_WINE_OBJ_MONIKER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IBindCtx,0xe,0,0);
typedef struct IBindCtx IBindCtx,*LPBINDCTX;
typedef LPBINDCTX LPBC;

DEFINE_OLEGUID(IID_IClassActivator,	0x00000140L, 0, 0);
typedef struct IClassActivator IClassActivator,*LPCLASSACTIVATOR;

DEFINE_OLEGUID(IID_IEnumMoniker,	0x00000102L, 0, 0);
typedef struct IEnumMoniker IEnumMoniker,*LPENUMMONIKER;

DEFINE_OLEGUID(IID_IMoniker,		0x0000000fL, 0, 0);
typedef struct IMoniker IMoniker,*LPMONIKER;

DEFINE_GUID   (IID_IROTData,		0xf29f6bc0L, 0x5021, 0x11ce, 0xaa, 0x15, 0x00, 0x00, 0x69, 0x01, 0x29, 0x3f);
typedef struct IROTData IROTData,*LPROTDATA;

DEFINE_OLEGUID(IID_IRunnableObject,	0x00000126L, 0, 0);
typedef struct IRunnableObject IRunnableObject,*LPRUNNABLEOBJECT;

DEFINE_OLEGUID(IID_IRunningObjectTable,	0x00000010L, 0, 0);
typedef struct IRunningObjectTable IRunningObjectTable,*LPRUNNINGOBJECTTABLE;

DEFINE_GUID( CLSID_FileMoniker,0x00000303,0,0,0xc0,0,0,0,0,0,0,0x46);

DEFINE_GUID( CLSID_ItemMoniker,0x00000304,0,0,0xc0,0,0,0,0,0,0,0x46);

DEFINE_GUID( CLSID_AntiMoniker,0x00000305,0,0,0xc0,0,0,0,0,0,0,0x46);

DEFINE_GUID( CLSID_CompositeMoniker,0x00000309,0,0,0xc0,0,0,0,0,0,0,0x46);

/*********************************************************************************
 *	BIND_OPTS and BIND_OPTS2 structures definition
 *	These structures contain parameters used during a moniker-binding operation.
 *********************************************************************************/
typedef struct tagBIND_OPTS
{
    DWORD cbStruct;
    DWORD grfFlags;
    DWORD grfMode;
    DWORD dwTickCountDeadline;
} BIND_OPTS, * LPBIND_OPTS;

typedef struct tagBIND_OPTS2
{
    DWORD cbStruct;
    DWORD grfFlags;
    DWORD grfMode;
    DWORD dwTickCountDeadline;
    DWORD dwTrackFlags;
    DWORD dwClassContext;
    LCID  locale;
    COSERVERINFO* pServerInfo;
} BIND_OPTS2, * LPBIND_OPTS2;

/*****************************************************************************
 * IBindCtx interface
 */
#define ICOM_INTERFACE IBindCtx
#define IBindCtx_METHODS \
    ICOM_METHOD1 (HRESULT, RegisterObjectBound,  IUnknown*,punk) \
    ICOM_METHOD1 (HRESULT, RevokeObjectBound,    IUnknown*,punk) \
    ICOM_METHOD  (HRESULT, ReleaseBoundObjects) \
    ICOM_METHOD1 (HRESULT, SetBindOptions,       LPBIND_OPTS2,pbindopts) \
    ICOM_METHOD1 (HRESULT, GetBindOptions,       LPBIND_OPTS2,pbindopts) \
    ICOM_METHOD1 (HRESULT, GetRunningObjectTable,IRunningObjectTable**,pprot) \
    ICOM_METHOD2 (HRESULT, RegisterObjectParam,  LPOLESTR,pszkey, IUnknown*,punk) \
    ICOM_METHOD2 (HRESULT, GetObjectParam,       LPOLESTR,pszkey, IUnknown**,punk) \
    ICOM_METHOD1 (HRESULT, EnumObjectParam,      IEnumString**,ppenum) \
    ICOM_METHOD1 (HRESULT, RevokeObjectParam,    LPOLESTR,pszkey)
#define IBindCtx_IMETHODS \
    IUnknown_IMETHODS \
    IBindCtx_METHODS
ICOM_DEFINE(IBindCtx,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IBindCtx_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IBindCtxr_AddRef(p)            ICOM_CALL (AddRef,p)
#define IBindCtx_Release(p)            ICOM_CALL (Release,p)
/* IBindCtx methods*/
#define IBindCtx_RegisterObjectBound(p,a)   ICOM_CALL1(RegisterObjectBound,p,a)
#define IBindCtx_RevokeObjectBound(p,a)     ICOM_CALL1(RevokeObjectBound,p,a)
#define IBindCtx_ReleaseBoundObjects(p)     ICOM_CALL (ReleaseBoundObjects,p)
#define IBindCtx_SetBindOptions(p,a)        ICOM_CALL1(SetBindOptions,p,a)
#define IBindCtx_GetBindOptions(p,a)        ICOM_CALL1(GetBindOptions,p,a)
#define IBindCtx_GetRunningObjectTable(p,a) ICOM_CALL1(GetRunningObjectTable,p,a)
#define IBindCtx_RegisterObjectParam(p,a,b) ICOM_CALL2(RegisterObjectParam,p,a,b)
#define IBindCtx_GetObjectParam(p,a,b)      ICOM_CALL2(GetObjectParam,p,a,b)
#define IBindCtx_EnumObjectParam(p,a)       ICOM_CALL1(EnumObjectParam,p,a)
#define IBindCtx_RevokeObjectParam(p,a)     ICOM_CALL1(RevokeObjectParam,p,a)

HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC* ppbc);
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC* ppbc);

/*****************************************************************************
 * IClassActivator interface
 */
#define ICOM_INTERFACE IClassActivator
#define IClassActivator_METHODS \
    ICOM_METHOD5(HRESULT,GetClassObject, REFCLSID,rclsid, DWORD,dwClassContext, LCID,locale, REFIID,riid, void**,ppv)
#define IClassActivator_IMETHODS \
    IUnknown_IMETHODS \
    IClassActivator_METHODS
ICOM_DEFINE(IClassActivator,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IClassActivator_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IClassActivator_AddRef(p)             ICOM_CALL (AddRef,p)
#define IClassActivator_Release(p)            ICOM_CALL (Release,p)
/*** IClassActivator methods ***/
#define IClassActivator_GetClassObject(p,a,b,c,d,e) ICOM_CALL5(GetClassObject,p,a,b,c,d,e)


/*****************************************************************************
 * IEnumMoniker interface
 */
#define ICOM_INTERFACE IEnumMoniker
#define IEnumMoniker_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, IMoniker**,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumMoniker**,ppenum)
#define IEnumMoniker_IMETHODS \
    IUnknown_IMETHODS \
    IEnumMoniker_METHODS
ICOM_DEFINE(IEnumMoniker,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumMoniker_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumMoniker_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumMoniker_Release(p)            ICOM_CALL (Release,p)
/*** IEnumMoniker methods ***/
#define IEnumMoniker_Next(p,a,b,c) ICOM_CALL3(Next,p,a,b,c)
#define IEnumMoniker_Skip(p,a)     ICOM_CALL1(Skip,p,a)
#define IEnumMoniker_Reset(p)      ICOM_CALL (Reset,p)
#define IEnumMoniker_Clone(p,a)    ICOM_CALL1(Clone,p,a)

/*****************************************************************************
 * IMoniker interface
 */

typedef enum tagMKSYS
{
    MKSYS_NONE              = 0,
    MKSYS_GENERICCOMPOSITE  = 1,
    MKSYS_FILEMONIKER       = 2,
    MKSYS_ANTIMONIKER       = 3,
    MKSYS_ITEMMONIKER       = 4,
    MKSYS_POINTERMONIKER    = 5,
    MKSYS_CLASSMONIKER      = 7
} MKSYS;

typedef enum tagMKREDUCE
{
    MKRREDUCE_ONE           = 3 << 16,
    MKRREDUCE_TOUSER        = 2 << 16,
    MKRREDUCE_THROUGHUSER   = 1 << 16,
    MKRREDUCE_ALL           = 0
} MKRREDUCE;

#define ICOM_INTERFACE IMoniker
#define IMoniker_METHODS \
    ICOM_METHOD4(HRESULT,BindToObject,        IBindCtx*,pbc, IMoniker*,pmkToLeft, REFIID,riidResult, void**,ppvResult) \
    ICOM_METHOD4(HRESULT,BindToStorage,       IBindCtx*,pbc, IMoniker*,pmkToLeft, REFIID,riid, void**,ppvObj) \
    ICOM_METHOD4(HRESULT,Reduce,              IBindCtx*,pbc, DWORD,dwReduceHowFar, IMoniker**,ppmkToLeft, IMoniker**,ppmkReduced) \
    ICOM_METHOD3(HRESULT,ComposeWith,         IMoniker*,pmkRight, BOOL,fOnlyIfNotGeneric, IMoniker**,ppmkComposite) \
    ICOM_METHOD2(HRESULT,Enum,                BOOL,fForward, IEnumMoniker**,ppenumMoniker) \
    ICOM_METHOD1(HRESULT,IsEqual,             IMoniker*,pmkOtherMoniker) \
    ICOM_METHOD1(HRESULT,Hash,                DWORD*,pdwHash) \
    ICOM_METHOD3(HRESULT,IsRunning,           IBindCtx*,pbc, IMoniker*,pmkToLeft, IMoniker*,pmkNewlyRunning) \
    ICOM_METHOD3(HRESULT,GetTimeOfLastChange, IBindCtx*,pbc, IMoniker*,pmkToLeft, FILETIME*,pFileTime) \
    ICOM_METHOD1(HRESULT,Inverse,             IMoniker**,ppmk) \
    ICOM_METHOD2(HRESULT,CommonPrefixWith,    IMoniker*,pmkOtherMoniker, IMoniker**,ppmkPrefix) \
    ICOM_METHOD2(HRESULT,RelativePathTo,      IMoniker*,pmkOther, IMoniker**,ppmkRelPath) \
    ICOM_METHOD3(HRESULT,GetDisplayName,      IBindCtx*,pbc, IMoniker*,pmkToLeft, LPOLESTR*,ppszDisplayName) \
    ICOM_METHOD5(HRESULT,ParseDisplayName,    IBindCtx*,pbc, IMoniker*,pmkToLeft, LPOLESTR,pszDisplayName, ULONG*,pchEaten, IMoniker**,ppmkOut) \
    ICOM_METHOD1(HRESULT,IsSystemMoniker,     DWORD*,pdwMksys)
#define IMoniker_IMETHODS \
    IPersistStream_IMETHODS \
    IMoniker_METHODS
ICOM_DEFINE(IMoniker,IPersistStream)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IMoniker_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IMoniker_AddRef(p)             ICOM_CALL (AddRef,p)
#define IMoniker_Release(p)            ICOM_CALL (Release,p)

/*** IPersist methods ***/
#define IMoniker_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistStream methods ***/
#define IMoniker_IsDirty(p)      ICOM_CALL (IsDirty,p)
#define IMoniker_Load(p,a)       ICOM_CALL1(Load,p,a)
#define IMoniker_Save(p,a,b)     ICOM_CALL2(Save,p,a,b)
#define IMoniker_GetSizeMax(p,a) ICOM_CALL1(GetSizeMax,p,a)
/*** IMoniker methods ***/
#define IMoniker_BindToObject(p,a,b,c,d)            ICOM_CALL4(BindToObject,p,a,b,c,d)
#define IMoniker_BindToStorage(p,a,b,c,d)           ICOM_CALL4(BindToStorage,p,a,b,c,d)
#define IMoniker_Reduce(p,a,b,c,d)                  ICOM_CALL4(Reduce,p,a,b,c,d)
#define IMoniker_ComposeWith(p,a,b,c)               ICOM_CALL3(ComposeWith,p,a,b,c)
#define IMoniker_Enum(p,a,b)                        ICOM_CALL2(Enum,p,a,b)
#define IMoniker_IsEqual(p,a)                       ICOM_CALL1(IsEqual,p,a)
#define IMoniker_Hash(p,a)                          ICOM_CALL1(Hash,p,a)
#define IMoniker_IsRunning(p,a,b,c)                 ICOM_CALL3(IsRunning,p,a,b,c)
#define IMoniker_GetTimeOfLastChange(p,a,b,c)       ICOM_CALL3(GetTimeOfLastChange,p,a,b,c)
#define IMoniker_Inverse(p,a)                       ICOM_CALL1(Inverse,p,a)
#define IMoniker_CommonPrefixWith(p,a,b)            ICOM_CALL2(CommonPrefixWith,p,a,b)
#define IMoniker_RelativePathTo(p,a,b)              ICOM_CALL2(RelativePathTo,p,a,b)
#define IMoniker_GetDisplayName(p,a,b,c)            ICOM_CALL3(GetDisplayName,p,a,b,c)
#define IMoniker_ParseDisplayName(p,a,b,c,d,e)      ICOM_CALL5(ParseDisplayName,p,a,b,c,d,e)
#define IMoniker_IsSystemMoniker(p,a)               ICOM_CALL1(IsSystemMoniker,p,a)

HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName, LPMONIKER* ppmk);
HRESULT WINAPI CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER* ppmk);

HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim, LPCOLESTR  lpszItem, LPMONIKER* ppmk);
HRESULT WINAPI CreateItemMoniker(LPCOLESTR lpszDelim, LPCOLESTR  lpszItem, LPMONIKER* ppmk);

HRESULT WINAPI CreateAntiMoniker(LPMONIKER * ppmk);

HRESULT WINAPI CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest, LPMONIKER* ppmkComposite);

/* FIXME: not implemented */
HRESULT WINAPI  BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID* ppvResult);

/* FIXME: not implemented */
HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, LPMONIKER* ppmk);

/* FIXME: not implemented */
HRESULT WINAPI CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER* ppmk);

/* FIXME: not implemented */
HRESULT WINAPI MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon);
/*****************************************************************************
 * IROTData interface
 */
#define ICOM_INTERFACE IROTData
#define IROTData_METHODS \
    ICOM_METHOD3(HRESULT,GetComparisonData, BYTE*,pbData, ULONG,cbMax, ULONG*,pcbData)
#define IROTData_IMETHODS \
    IUnknown_IMETHODS \
    IROTData_METHODS
ICOM_DEFINE(IROTData,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IROTData_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IROTData_AddRef(p)             ICOM_CALL (AddRef,p)
#define IROTData_Release(p)            ICOM_CALL (Release,p)
/*** IROTData methods ***/
#define IROTData_GetComparisonData(p,a,b,c) ICOM_CALL3(GetComparisonData,p,a,b,c)

#define ICOM_THIS_From_IROTData(class, name) class* This = (class*)(((char*)name)-sizeof(void*))

/*****************************************************************************
 * IRunnableObject interface
 */
#define ICOM_INTERFACE IRunnableObject
#define IRunnableObject_METHODS \
    ICOM_METHOD1(HRESULT,GetRunningClass,    LPCLSID,lpClsid) \
    ICOM_METHOD1(HRESULT,Run,                IBindCtx*,pbc) \
    ICOM_METHOD (BOOL,IsRunning) \
    ICOM_METHOD2(HRESULT,LockRunning,        BOOL,fLock, BOOL,fLastUnlockCloses) \
    ICOM_METHOD1(HRESULT,SetContainedObject, BOOL,fContained)
#define IRunnableObject_IMETHODS \
    IUnknown_IMETHODS \
    IRunnableObject_METHODS
ICOM_DEFINE(IRunnableObject,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRunnableObject_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRunnableObject_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRunnableObject_Release(p)            ICOM_CALL (Release,p)
/*** IRunnableObject methods ***/
#define IRunnableObject_GetRunningClass(p,a)    ICOM_CALL1(GetRunningClass,p,a)
#define IRunnableObject_Run(p,a)                ICOM_CALL1(Run,p,a)
#define IRunnableObject_IsRunning(p)            ICOM_CALL (IsRunning,p)
#define IRunnableObject_LockRunning(p,a,b)      ICOM_CALL2(LockRunning,p,a,b)
#define IRunnableObject_SetContainedObject(p,a) ICOM_CALL1(SetContainedObject,p,a)


/*****************************************************************************
 * IRunningObjectTable interface
 */
#ifdef __WINE__
#undef GetObject
#endif

#define ICOM_INTERFACE IRunningObjectTable
#define IRunningObjectTable_METHODS \
    ICOM_METHOD4(HRESULT,Register,            DWORD,grfFlags, IUnknown*,punkObject, IMoniker*,pmkObjectName, DWORD*,pdwRegister) \
    ICOM_METHOD1(HRESULT,Revoke,              DWORD,dwRegister) \
    ICOM_METHOD1(HRESULT,IsRunning,           IMoniker*,pmkObjectName) \
    ICOM_METHOD2(HRESULT,GetObject,           IMoniker*,pmkObjectName, IUnknown**,ppunkObject) \
    ICOM_METHOD2(HRESULT,NoteChangeTime,      DWORD,dwRegister, FILETIME*,pfiletime) \
    ICOM_METHOD2(HRESULT,GetTimeOfLastChange, IMoniker*,pmkObjectName, FILETIME*,pfiletime) \
    ICOM_METHOD1(HRESULT,EnumRunning,         IEnumMoniker**,ppenumMoniker)
#define IRunningObjectTable_IMETHODS \
    IUnknown_IMETHODS \
    IRunningObjectTable_METHODS
ICOM_DEFINE(IRunningObjectTable,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRunningObjectTable_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRunningObjectTable_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRunningObjectTable_Release(p)            ICOM_CALL (Release,p)
/*** IRunningObjectTable methods ***/
#define IRunningObjectTable_Register(p,a,b,c,d)        ICOM_CALL4(Register,p,a,b,c,d)
#define IRunningObjectTable_Revoke(p,a)                ICOM_CALL1(Revoke,p,a)
#define IRunningObjectTable_IsRunning(p,a)             ICOM_CALL1(IsRunning,p,a)
#define IRunningObjectTable_GetObject(p,a,b)           ICOM_CALL2(GetObject,p,a,b)
#define IRunningObjectTable_NoteChangeTime(p,a,b)      ICOM_CALL2(NoteChangeTime,p,a,b)
#define IRunningObjectTable_GetTimeOfLastChange(p,a,b) ICOM_CALL2(GetTimeOfLastChange,p,a,b)
#define IRunningObjectTable_EnumRunning(p,a)           ICOM_CALL1(EnumRunning,p,a)

HRESULT WINAPI GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot);
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot);

/*****************************************************************************
 * Additional API
 */

/* FIXME: not implemented */
HRESULT WINAPI CoGetInstanceFromFile(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter, DWORD dwClsCtx, DWORD grfMode, OLECHAR* pwszName, DWORD dwCount, MULTI_QI* pResults);

/* FIXME: not implemented */
HRESULT WINAPI CoGetInstanceFromIStorage(COSERVERINFO* pServerInfo, CLSID* pClsid, IUnknown* punkOuter, DWORD dwClsCtx, IStorage* pstg, DWORD dwCount, MULTI_QI* pResults);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_MONIKER_H */
