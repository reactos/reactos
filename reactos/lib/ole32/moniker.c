/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 *      Copyright 1999  Noomen Hamza
 */

#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>
#include <compobj.h>

#include <debug.h>


#define  BLOCK_TAB_SIZE 20 /* represent the first size table and it's increment block size */

/* define the structure of the running object table elements */
typedef struct RunObject{

    IUnknown*  pObj; /* points on a running object*/
    IMoniker*  pmkObj; /* points on a moniker who identifies this object */
    FILETIME   lastModifObj;
    DWORD      identRegObj; /* registration key relative to this object */
    DWORD      regTypeObj; /* registration type : strong or weak */
}RunObject;

/* define the RunningObjectTableImpl structure */
typedef struct RunningObjectTableImpl{

    ICOM_VFIELD(IRunningObjectTable);
    ULONG      ref;

    RunObject* runObjTab;            /* pointer to the first object in the table       */
    DWORD      runObjTabSize;       /* current table size                            */
    DWORD      runObjTabLastIndx;  /* first free index element in the table.        */
    DWORD      runObjTabRegister; /* registration key of the next registered object */
    
} RunningObjectTableImpl;

RunningObjectTableImpl* runningObjectTableInstance=0;

/* IRunningObjectTable prototype functions : */
/* IUnknown functions*/
static HRESULT WINAPI RunningObjectTableImpl_QueryInterface(IRunningObjectTable* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI RunningObjectTableImpl_AddRef(IRunningObjectTable* iface);
static ULONG   WINAPI RunningObjectTableImpl_Release(IRunningObjectTable* iface);
/* IRunningObjectTable functions */
static HRESULT WINAPI RunningObjectTableImpl_Register(IRunningObjectTable* iface, DWORD grfFlags,IUnknown* punkObject,IMoniker* pmkObjectName,DWORD* pdwRegister);
static HRESULT WINAPI RunningObjectTableImpl_Revoke(IRunningObjectTable* iface, DWORD dwRegister);
static HRESULT WINAPI RunningObjectTableImpl_IsRunning(IRunningObjectTable* iface, IMoniker* pmkObjectName);
static HRESULT WINAPI RunningObjectTableImpl_GetObject(IRunningObjectTable* iface, IMoniker* pmkObjectName,IUnknown** ppunkObject);
static HRESULT WINAPI RunningObjectTableImpl_NoteChangeTime(IRunningObjectTable* iface, DWORD dwRegister,FILETIME* pfiletime);
static HRESULT WINAPI RunningObjectTableImpl_GetTimeOfLastChange(IRunningObjectTable* iface, IMoniker* pmkObjectName,FILETIME* pfiletime);
static HRESULT WINAPI RunningObjectTableImpl_EnumRunning(IRunningObjectTable* iface, IEnumMoniker** ppenumMoniker);
/* Local functions*/
HRESULT WINAPI RunningObjectTableImpl_Initialize();
HRESULT WINAPI RunningObjectTableImpl_UnInitialize();
HRESULT WINAPI RunningObjectTableImpl_Destroy();
HRESULT WINAPI RunningObjectTableImpl_GetObjectIndex(RunningObjectTableImpl* This,DWORD identReg,IMoniker* pmk,DWORD *indx);

/* Virtual function table for the IRunningObjectTable class. */
static ICOM_VTABLE(IRunningObjectTable) VT_RunningObjectTableImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    RunningObjectTableImpl_QueryInterface,
    RunningObjectTableImpl_AddRef,
    RunningObjectTableImpl_Release,
    RunningObjectTableImpl_Register,
    RunningObjectTableImpl_Revoke,
    RunningObjectTableImpl_IsRunning,
    RunningObjectTableImpl_GetObject,
    RunningObjectTableImpl_NoteChangeTime,
    RunningObjectTableImpl_GetTimeOfLastChange,
    RunningObjectTableImpl_EnumRunning
};

/***********************************************************************
 *        RunningObjectTable_QueryInterface
 */
HRESULT WINAPI RunningObjectTableImpl_QueryInterface(IRunningObjectTable* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",This,riid,ppvObject));

    /* validate arguments */
    if (This==0)
        return CO_E_NOTINITIALIZED;

    if (ppvObject==0)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid))
        *ppvObject = (IRunningObjectTable*)This;
    else
        if (IsEqualIID(&IID_IRunningObjectTable, riid))
            *ppvObject = (IRunningObjectTable*)This;

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    RunningObjectTableImpl_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_AddRef
 */
ULONG   WINAPI RunningObjectTableImpl_AddRef(IRunningObjectTable* iface)
{
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    return ++(This->ref);
}

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
HRESULT WINAPI RunningObjectTableImpl_Destroy()
{
    Print(MAX_TRACE, ("()\n"));
    
    if (runningObjectTableInstance==NULL)
        return E_INVALIDARG;

    /* free the ROT table memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance->runObjTab);

    /* free the ROT structure memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_Release
 */
ULONG   WINAPI RunningObjectTableImpl_Release(IRunningObjectTable* iface)
{
    DWORD i;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    This->ref--;

    /* unitialize ROT structure if there's no more reference to it*/
    if (This->ref==0){

        /* release all registered objects */
        for(i=0;i<This->runObjTabLastIndx;i++)
        {
            if (( This->runObjTab[i].regTypeObj &  ROTFLAGS_REGISTRATIONKEEPSALIVE) != 0)
                IUnknown_Release(This->runObjTab[i].pObj);
 
            IMoniker_Release(This->runObjTab[i].pmkObj);
        }
       /*  RunningObjectTable data structure will be not destroyed here ! the destruction will be done only
        *  when RunningObjectTableImpl_UnInitialize function is called
        */

        /* there's no more elements in the table */
        This->runObjTabRegister=0;
        This->runObjTabLastIndx=0;

        return 0;
    }

    return This->ref;
}

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
HRESULT WINAPI RunningObjectTableImpl_Initialize()
{
    Print(MAX_TRACE, ("()\n"));

    /* create the unique instance of the RunningObjectTableImpl structure */
    runningObjectTableInstance = HeapAlloc(GetProcessHeap(), 0, sizeof(RunningObjectTableImpl));

    if (runningObjectTableInstance == 0)
        return E_OUTOFMEMORY;

    /* initialize the virtual table function */
    ICOM_VTBL(runningObjectTableInstance) = &VT_RunningObjectTableImpl;

    /* the initial reference is set to "1" ! because if set to "0" it will be not practis when */
    /* the ROT refered many times  not in the same time (all the objects in the ROT will  */
    /* be removed every time the ROT is removed ) */
    runningObjectTableInstance->ref = 1;

    /* allocate space memory for the table which contains all the running objects */
    runningObjectTableInstance->runObjTab = HeapAlloc(GetProcessHeap(), 0, sizeof(RunObject[BLOCK_TAB_SIZE]));

    if (runningObjectTableInstance->runObjTab == NULL)
        return E_OUTOFMEMORY;

    runningObjectTableInstance->runObjTabSize=BLOCK_TAB_SIZE;
    runningObjectTableInstance->runObjTabRegister=1;
    runningObjectTableInstance->runObjTabLastIndx=0;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_UnInitialize
 */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize()
{
    Print(MAX_TRACE, ("()\n"));

    if (runningObjectTableInstance==NULL)
        return E_POINTER;
    
    RunningObjectTableImpl_Release((IRunningObjectTable*)runningObjectTableInstance);

    RunningObjectTableImpl_Destroy();

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_Register
 */
HRESULT WINAPI RunningObjectTableImpl_Register(IRunningObjectTable* iface,
                                               DWORD grfFlags,           /* Registration options */
                                               IUnknown *punkObject,     /* Pointer to the object being registered */
                                               IMoniker *pmkObjectName,  /* Pointer to the moniker of the object being registered */
                                               DWORD *pdwRegister)       /* Pointer to the value identifying the  registration */
{
    HRESULT res=S_OK;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%ld,%p,%p,%p)\n",This,grfFlags,punkObject,pmkObjectName,pdwRegister));

    /* there's only two types of register : strong and or weak registration (only one must be passed on parameter) */
    if ( ( (grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) || !(grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (!(grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) ||  (grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (grfFlags) )
        return E_INVALIDARG;

    if (punkObject==NULL || pmkObjectName==NULL || pdwRegister==NULL)
        return E_INVALIDARG;

    /* verify if the object to be registered was registered before */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,NULL)==S_OK)
        res = MK_S_MONIKERALREADYREGISTERED;

    /* put the new registered object in the first free element in the table */
    This->runObjTab[This->runObjTabLastIndx].pObj = punkObject;
    This->runObjTab[This->runObjTabLastIndx].pmkObj = pmkObjectName;
    This->runObjTab[This->runObjTabLastIndx].regTypeObj = grfFlags;
    This->runObjTab[This->runObjTabLastIndx].identRegObj = This->runObjTabRegister;
    CoFileTimeNow(&(This->runObjTab[This->runObjTabLastIndx].lastModifObj));
    
    /* gives a registration identifier to the registered object*/
    (*pdwRegister)= This->runObjTabRegister;

    if (This->runObjTabRegister == 0xFFFFFFFF){

        Print(MIN_TRACE, ("runObjTabRegister: %ld is out of data limite \n",This->runObjTabRegister));
	return E_FAIL;
}
    This->runObjTabRegister++;
    This->runObjTabLastIndx++;
    
    if (This->runObjTabLastIndx == This->runObjTabSize){ /* table is full ! so it must be resized */

        This->runObjTabSize+=BLOCK_TAB_SIZE; /* newsize table */
        This->runObjTab=HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->runObjTab,
                        This->runObjTabSize * sizeof(RunObject));
        if (!This->runObjTab)
            return E_OUTOFMEMORY;
    }
    /* add a reference to the object in the strong registration case */
    if ((grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) !=0 )
        IUnknown_AddRef(punkObject);

    IMoniker_AddRef(pmkObjectName);
    
    return res;
}

/***********************************************************************
 *        RunningObjectTable_Revoke
 */
HRESULT WINAPI RunningObjectTableImpl_Revoke(  IRunningObjectTable* iface,
                                               DWORD dwRegister)  /* Value identifying registration to be revoked*/
{

    DWORD index,j;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%ld)\n",This,dwRegister));

    /* verify if the object to be revoked was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,dwRegister,NULL,&index)==S_FALSE)

        return E_INVALIDARG;

    /* release the object if it was registered with a strong registrantion option */
    if ((This->runObjTab[index].regTypeObj & ROTFLAGS_REGISTRATIONKEEPSALIVE)!=0)
        IUnknown_Release(This->runObjTab[index].pObj);

    IMoniker_Release(This->runObjTab[index].pmkObj);
    
    /* remove the object from the table */
    for(j=index; j<This->runObjTabLastIndx-1; j++)
        This->runObjTab[j]= This->runObjTab[j+1];
    
    This->runObjTabLastIndx--;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_IsRunning
 */
HRESULT WINAPI RunningObjectTableImpl_IsRunning(  IRunningObjectTable* iface,
                                                  IMoniker *pmkObjectName)  /* Pointer to the moniker of the object whose status is desired */
{    
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%p)\n",This,pmkObjectName));

    return RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,NULL);
}

/***********************************************************************
 *        RunningObjectTable_GetObject
 */
HRESULT WINAPI RunningObjectTableImpl_GetObject(  IRunningObjectTable* iface,
                                                  IMoniker *pmkObjectName,/* Pointer to the moniker on the object */
                                                  IUnknown **ppunkObject) /* Address of output variable that receives the IUnknown interface pointer */
{
    DWORD index;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",This,pmkObjectName,ppunkObject));

    if (ppunkObject==NULL)
        return E_POINTER;
    
    *ppunkObject=0;

    /* verify if the object was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,&index)==S_FALSE)
        return MK_E_UNAVAILABLE;

    /* add a reference to the object then set output object argument */
    IUnknown_AddRef(This->runObjTab[index].pObj);
    *ppunkObject=This->runObjTab[index].pObj;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_NoteChangeTime
 */
HRESULT WINAPI RunningObjectTableImpl_NoteChangeTime(IRunningObjectTable* iface,
                                                     DWORD dwRegister,  /* Value identifying registration being updated */
                                                     FILETIME *pfiletime) /* Pointer to structure containing object's last change time */
{
    DWORD index=-1;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%ld,%p)\n",This,dwRegister,pfiletime));

    /* verify if the object to be changed was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,dwRegister,NULL,&index)==S_FALSE)
        return E_INVALIDARG;

    /* set the new value of the last time change */
    This->runObjTab[index].lastModifObj= (*pfiletime);

    return S_OK;
}
    
/***********************************************************************
 *        RunningObjectTable_GetTimeOfLastChange
 */
HRESULT WINAPI RunningObjectTableImpl_GetTimeOfLastChange(IRunningObjectTable* iface,
                                                          IMoniker *pmkObjectName,  /* Pointer to moniker on the object whose status is desired */
                                                          FILETIME *pfiletime)       /* Pointer to structure that receives object's last change time */
{
    DWORD index=-1;
    ICOM_THIS(RunningObjectTableImpl,iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",This,pmkObjectName,pfiletime));

    if (pmkObjectName==NULL || pfiletime==NULL)
        return E_INVALIDARG;

    /* verify if the object was registered before or not */
    if (RunningObjectTableImpl_GetObjectIndex(This,-1,pmkObjectName,&index)==S_FALSE)
        return MK_E_UNAVAILABLE;;

    (*pfiletime)= This->runObjTab[index].lastModifObj;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_EnumRunning
 */
HRESULT WINAPI RunningObjectTableImpl_EnumRunning(IRunningObjectTable* iface,
                                                  IEnumMoniker **ppenumMoniker) /* Address of output variable that receives the IEnumMoniker interface pointer */
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/***********************************************************************
 *        GetObjectIndex
 */
HRESULT WINAPI RunningObjectTableImpl_GetObjectIndex(RunningObjectTableImpl* This,
                                                     DWORD identReg,
                                                     IMoniker* pmk,
                                                     DWORD *indx)
{

    DWORD i;

    Print(MAX_TRACE, ("(%p,%ld,%p,%p)\n",This,identReg,pmk,indx));

    if (pmk!=NULL)
        /* search object identified by a moniker */
        for(i=0 ; (i < This->runObjTabLastIndx) &&(!IMoniker_IsEqual(This->runObjTab[i].pmkObj,pmk)==S_OK);i++);
    else
        /* search object identified by a register identifier */
        for(i=0;((i<This->runObjTabLastIndx)&&(This->runObjTab[i].identRegObj!=identReg));i++);
    
    if (i==This->runObjTabLastIndx)  return S_FALSE;

    if (indx != NULL)  *indx=i;

    return S_OK;
}

/******************************************************************************
 *		GetRunningObjectTable16	[OLE2.30]
 */
HRESULT WINAPI GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
	UNIMPLEMENTED;
    return E_NOTIMPL;
}

/***********************************************************************
 *           GetRunningObjectTable (OLE2.73)
 */
HRESULT WINAPI GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    IID riid=IID_IRunningObjectTable;
    HRESULT res;

    Print(MAX_TRACE, ("()\n"));

    if (reserved!=0)
        return E_UNEXPECTED;

    if(runningObjectTableInstance==NULL)
        return CO_E_NOTINITIALIZED;

    res = RunningObjectTableImpl_QueryInterface((IRunningObjectTable*)runningObjectTableInstance,&riid,(void**)pprot);

    return res;
}

/******************************************************************************
 *              OleRun        [OLE32.123]
 */
HRESULT WINAPI OleRun(LPUNKNOWN pUnknown)
{
  IRunnableObject	*runable;
  ICOM_THIS(IRunnableObject,pUnknown);
  LRESULT		ret;

  ret = IRunnableObject_QueryInterface(This,&IID_IRunnableObject,(LPVOID*)&runable);
  if (ret) 
	return 0; /* Appears to return no error. */
  ret  = IRunnableObject_Run(runable,NULL);
  IRunnableObject_Release(runable);
  return ret;
}

/******************************************************************************
 *              MkParseDisplayName        [OLE32.81]
 */
HRESULT WINAPI MkParseDisplayName(LPBC pbc, LPCOLESTR szUserName,
				LPDWORD pchEaten, LPMONIKER *ppmk)
{
    Print(MIN_TRACE, ("(%p, %S, %p, %p): stub.\n", pbc, szUserName, pchEaten, *ppmk));
    if (!(IsValidInterface((LPUNKNOWN) pbc)))
	return E_INVALIDARG;

    return MK_E_SYNTAX;
}
