/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <storage32.h>

#include <debug.h>

#if 1

/* From wingdi16.h of WINE */

typedef HANDLE HMETAFILE16;

typedef struct
{
    INT16        mm;
    INT16        xExt;
    INT16        yExt;
    HMETAFILE16  hMF;
} METAFILEPICT16, *LPMETAFILEPICT16;

#endif

/* Used for OleConvertIStorageToOLESTREAM and OleConvertOLESTREAMToIStorage */
#define OLESTREAM_ID 0x501
#define OLESTREAM_MAX_STR_LEN 255

static const char rootPropertyName[] = "Root Entry";


/* OLESTREAM memory structure to use for Get and Put Routines */
/* Used for OleConvertIStorageToOLESTREAM and OleConvertOLESTREAMToIStorage */
typedef struct 
{
    DWORD dwOleID;
    DWORD dwTypeID;
    DWORD dwOleTypeNameLength;
    CHAR  strOleTypeName[OLESTREAM_MAX_STR_LEN];
    CHAR  *pstrOleObjFileName;
    DWORD dwOleObjFileNameLength;
    DWORD dwMetaFileWidth;
    DWORD dwMetaFileHeight;
    CHAR  strUnknown[8]; /* don't know what is this 8 byts information in OLE stream. */
    DWORD dwDataLength;
    BYTE *pData;
}OLECONVERT_OLESTREAM_DATA;

/* CompObj Stream structure */
/* Used for OleConvertIStorageToOLESTREAM and OleConvertOLESTREAMToIStorage */
typedef struct
{
    BYTE byUnknown1[12];
    CLSID clsid;
    DWORD dwCLSIDNameLength;
    CHAR strCLSIDName[OLESTREAM_MAX_STR_LEN];
    DWORD dwOleTypeNameLength;
    CHAR strOleTypeName[OLESTREAM_MAX_STR_LEN];
    DWORD dwProgIDNameLength;
    CHAR strProgIDName[OLESTREAM_MAX_STR_LEN];
    BYTE byUnknown2[16];
}OLECONVERT_ISTORAGE_COMPOBJ;


/* Ole Presention Stream structure */
/* Used for OleConvertIStorageToOLESTREAM and OleConvertOLESTREAMToIStorage */
typedef struct
{
    BYTE byUnknown1[28];
    DWORD dwExtentX;
    DWORD dwExtentY;
    DWORD dwSize;  
    BYTE *pData;
}OLECONVERT_ISTORAGE_OLEPRES;



/***********************************************************************
 * Forward declaration of internal functions used by the method DestroyElement
 */
static HRESULT deleteStorageProperty(
  StorageImpl *parentStorage,
  ULONG        foundPropertyIndexToDelete,
  StgProperty  propertyToDelete);

static HRESULT deleteStreamProperty(
  StorageImpl *parentStorage,
  ULONG         foundPropertyIndexToDelete,
  StgProperty   propertyToDelete);

static HRESULT findPlaceholder(
  StorageImpl *storage,
  ULONG         propertyIndexToStore,
  ULONG         storagePropertyIndex,
  INT         typeOfRelation);

static HRESULT adjustPropertyChain( 
  StorageImpl *This,
  StgProperty   propertyToDelete,
  StgProperty   parentProperty,
  ULONG         parentPropertyId,
  INT         typeOfRelation);

/***********************************************************************
 * Declaration of the functions used to manipulate StgProperty
 */

static ULONG getFreeProperty(
  StorageImpl *storage);

static void updatePropertyChain(
  StorageImpl *storage,
  ULONG       newPropertyIndex,
  StgProperty newProperty);

static LONG propertyNameCmp(
  OLECHAR *newProperty,
  OLECHAR *currentProperty);


/***********************************************************************
 * Declaration of miscellaneous functions...
 */
static HRESULT validateSTGM(DWORD stgmValue); 

static DWORD GetShareModeFromSTGM(DWORD stgm);
static DWORD GetAccessModeFromSTGM(DWORD stgm);
static DWORD GetCreationModeFromSTGM(DWORD stgm);

/*
 * Virtual function table for the IStorage32Impl class.
 */
static ICOM_VTABLE(IStorage) Storage32Impl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageImpl_CopyTo,
    StorageImpl_MoveElementTo,
    StorageImpl_Commit,
    StorageImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageImpl_SetStateBits,
    StorageBaseImpl_Stat
};

/*
 * Virtual function table for the Storage32InternalImpl class.
 */
static ICOM_VTABLE(IStorage) Storage32InternalImpl_Vtbl =
  {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    StorageBaseImpl_QueryInterface,
    StorageBaseImpl_AddRef,
    StorageBaseImpl_Release,
    StorageBaseImpl_CreateStream,
    StorageBaseImpl_OpenStream,
    StorageImpl_CreateStorage,
    StorageBaseImpl_OpenStorage,
    StorageImpl_CopyTo,
    StorageImpl_MoveElementTo,
    StorageInternalImpl_Commit,
    StorageInternalImpl_Revert,
    StorageBaseImpl_EnumElements,
    StorageImpl_DestroyElement,
    StorageBaseImpl_RenameElement,
    StorageImpl_SetElementTimes,
    StorageBaseImpl_SetClass,
    StorageImpl_SetStateBits,
    StorageBaseImpl_Stat
};

/*
 * Virtual function table for the IEnumSTATSTGImpl class.
 */
static ICOM_VTABLE(IEnumSTATSTG) IEnumSTATSTGImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IEnumSTATSTGImpl_QueryInterface,
    IEnumSTATSTGImpl_AddRef,
    IEnumSTATSTGImpl_Release,
    IEnumSTATSTGImpl_Next,
    IEnumSTATSTGImpl_Skip,
    IEnumSTATSTGImpl_Reset,
    IEnumSTATSTGImpl_Clone
};





/************************************************************************
** Storage32BaseImpl implementatiion
*/

/************************************************************************
 * Storage32BaseImpl_QueryInterface (IUnknown)
 *
 * This method implements the common QueryInterface for all IStorage32
 * implementations contained in this file.
 * 
 * See Windows documentation for more details on IUnknown methods.
 */
HRESULT WINAPI StorageBaseImpl_QueryInterface(
  IStorage*        iface,
  REFIID             riid,
  void**             ppvObject)
{
  ICOM_THIS(StorageBaseImpl,iface);
  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;
  
  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) 
  {
    *ppvObject = (IStorage*)This;
  }
  else if (memcmp(&IID_IStorage, riid, sizeof(IID_IStorage)) == 0) 
  {
    *ppvObject = (IStorage*)This;
  }
  
  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
    return E_NOINTERFACE;
  
  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  StorageBaseImpl_AddRef(iface);

  return S_OK;
}
        
/************************************************************************
 * Storage32BaseImpl_AddRef (IUnknown)
 *
 * This method implements the common AddRef for all IStorage32
 * implementations contained in this file.
 * 
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI StorageBaseImpl_AddRef( 
            IStorage* iface)
{
  ICOM_THIS(StorageBaseImpl,iface);
  This->ref++;

  return This->ref;
}
        
/************************************************************************
 * Storage32BaseImpl_Release (IUnknown)
 *
 * This method implements the common Release for all IStorage32
 * implementations contained in this file.
 * 
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI StorageBaseImpl_Release( 
      IStorage* iface)
{
  ICOM_THIS(StorageBaseImpl,iface);
  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    /*
     * Since we are using a system of base-classes, we want to call the 
     * destructor of the appropriate derived class. To do this, we are 
     * using virtual functions to implement the destructor.
     */
    This->v_destructor(This);

    return 0;
  }
  
  return This->ref;
}

/************************************************************************
 * Storage32BaseImpl_OpenStream (IStorage)
 *
 * This method will open the specified stream object from the current storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
HRESULT WINAPI StorageBaseImpl_OpenStream( 
  IStorage*        iface,
  const OLECHAR*   pwcsName,  /* [string][in] */
  void*            reserved1, /* [unique][in] */
  DWORD            grfMode,   /* [in]  */        
  DWORD            reserved2, /* [in]  */        
  IStream**        ppstm)     /* [out] */       
{
  ICOM_THIS(StorageBaseImpl,iface);
  IEnumSTATSTGImpl* propertyEnumeration;
  StgStreamImpl*    newStream;
  StgProperty       currentProperty;
  ULONG             foundPropertyIndex;

  Print(MAX_TRACE, ("(%p, %S, %p, %lx, %ld, %p)\n", 
	iface, pwcsName, reserved1, grfMode, reserved2, ppstm));

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (pwcsName==NULL) || (ppstm==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the out parameter
   */
  *ppstm = 0;
  
  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * As documented.
   */
  if ( !(grfMode & STGM_SHARE_EXCLUSIVE) ||
        (grfMode & STGM_DELETEONRELEASE) ||
        (grfMode & STGM_TRANSACTED) )
    return STG_E_INVALIDFUNCTION;

  /*
   * Create a property enumeration to search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(
    This->ancestorStorage, 
    This->rootPropertySetIndex);
  
  /*
   * Search the enumeration for the property with the given name
   */
  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(
    propertyEnumeration,
    pwcsName,
    &currentProperty);
  
  /*
   * Delete the property enumeration since we don't need it anymore
   */
  IEnumSTATSTGImpl_Destroy(propertyEnumeration);
  
  /*
   * If it was found, construct the stream object and return a pointer to it.
   */
  if ( (foundPropertyIndex!=PROPERTY_NULL) && 
       (currentProperty.propertyType==PROPTYPE_STREAM) )
  {
    newStream = StgStreamImpl_Construct(This, grfMode, foundPropertyIndex);
    
    if (newStream!=0)
    {
      newStream->grfMode = grfMode;
      *ppstm = (IStream*)newStream;

      /*
       * Since we are returning a pointer to the interface, we have to 
       * nail down the reference.
       */
      StgStreamImpl_AddRef(*ppstm);
      
      return S_OK;
    }
    
    return E_OUTOFMEMORY;
  }
  
  return STG_E_FILENOTFOUND;
}

/************************************************************************
 * Storage32BaseImpl_OpenStorage (IStorage)
 *
 * This method will open a new storage object from the current storage.
 * 
 * See Windows documentation for more details on IStorage methods.
 */        
HRESULT WINAPI StorageBaseImpl_OpenStorage( 
  IStorage*        iface,
  const OLECHAR*   pwcsName,      /* [string][unique][in] */ 
  IStorage*        pstgPriority,  /* [unique][in] */         
  DWORD            grfMode,       /* [in] */                 
  SNB              snbExclude,    /* [unique][in] */         
  DWORD            reserved,      /* [in] */                 
  IStorage**       ppstg)         /* [out] */                        
{
  ICOM_THIS(StorageBaseImpl,iface);
  StorageInternalImpl* newStorage;
  IEnumSTATSTGImpl*      propertyEnumeration;
  StgProperty            currentProperty;
  ULONG                  foundPropertyIndex;

  Print(MAX_TRACE, ("(%p, %S, %p, %lx, %p, %ld, %p)\n", 
	iface, pwcsName, pstgPriority, 
	grfMode, snbExclude, reserved, ppstg));
  
  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (pwcsName==NULL) || (ppstg==0) )
    return E_INVALIDARG;
  
  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * As documented.
   */
  if ( !(grfMode & STGM_SHARE_EXCLUSIVE) || 
        (grfMode & STGM_DELETEONRELEASE) ||
        (grfMode & STGM_PRIORITY) )
    return STG_E_INVALIDFUNCTION;

  /*
   * Initialize the out parameter
   */
  *ppstg = 0;
  
  /*
   * Create a property enumeration to search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(
                          This->ancestorStorage, 
                          This->rootPropertySetIndex);
  
  /*
   * Search the enumeration for the property with the given name
   */
  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(
                         propertyEnumeration,
                         pwcsName,
                         &currentProperty);
  
  /*
   * Delete the property enumeration since we don't need it anymore
   */
  IEnumSTATSTGImpl_Destroy(propertyEnumeration);
  
  /*
   * If it was found, construct the stream object and return a pointer to it.
   */
  if ( (foundPropertyIndex!=PROPERTY_NULL) && 
       (currentProperty.propertyType==PROPTYPE_STORAGE) )
  {
    /*
     * Construct a new Storage object
     */
    newStorage = StorageInternalImpl_Construct(
                   This->ancestorStorage,
                   foundPropertyIndex);
    
    if (newStorage != 0)
    {
      *ppstg = (IStorage*)newStorage;

      /*
       * Since we are returning a pointer to the interface, 
       * we have to nail down the reference.
       */
      StorageBaseImpl_AddRef(*ppstg);
      
      return S_OK;
    }
    
    return STG_E_INSUFFICIENTMEMORY;
  }
  
  return STG_E_FILENOTFOUND;
}

/************************************************************************
 * Storage32BaseImpl_EnumElements (IStorage)
 *
 * This method will create an enumerator object that can be used to 
 * retrieve informatino about all the properties in the storage object.
 * 
 * See Windows documentation for more details on IStorage methods.
 */        
HRESULT WINAPI StorageBaseImpl_EnumElements( 
  IStorage*       iface,
  DWORD           reserved1, /* [in] */                  
  void*           reserved2, /* [size_is][unique][in] */ 
  DWORD           reserved3, /* [in] */                  
  IEnumSTATSTG**  ppenum)    /* [out] */                 
{
  ICOM_THIS(StorageBaseImpl,iface);
  IEnumSTATSTGImpl* newEnum;

  Print(MAX_TRACE, ("(%p, %ld, %p, %ld, %p)\n", 
	iface, reserved1, reserved2, reserved3, ppenum));

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppenum==0))
    return E_INVALIDARG;
  
  /*
   * Construct the enumerator.
   */
  newEnum = IEnumSTATSTGImpl_Construct(
              This->ancestorStorage,
              This->rootPropertySetIndex);

  if (newEnum!=0)
  {
    *ppenum = (IEnumSTATSTG*)newEnum;

    /*
     * Don't forget to nail down a reference to the new object before
     * returning it.
     */
    IEnumSTATSTGImpl_AddRef(*ppenum);
    
    return S_OK;
  }

  return E_OUTOFMEMORY;
}

/************************************************************************
 * Storage32BaseImpl_Stat (IStorage)
 *
 * This method will retrieve information about this storage object.
 * 
 * See Windows documentation for more details on IStorage methods.
 */        
HRESULT WINAPI StorageBaseImpl_Stat( 
  IStorage*        iface,
  STATSTG*         pstatstg,     /* [out] */ 
  DWORD            grfStatFlag)  /* [in] */  
{
  ICOM_THIS(StorageBaseImpl,iface);
  StgProperty    curProperty;
  BOOL         readSuccessful;

  Print(MAX_TRACE, ("(%p, %p, %lx)\n", 
	iface, pstatstg, grfStatFlag));

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (pstatstg==0))
    return E_INVALIDARG;

  /*
   * Read the information from the property.
   */
  readSuccessful = StorageImpl_ReadProperty(
                    This->ancestorStorage,
                    This->rootPropertySetIndex,
                    &curProperty);

  if (readSuccessful)
  {
    StorageUtl_CopyPropertyToSTATSTG(
      pstatstg, 
      &curProperty, 
      grfStatFlag);
    
    return S_OK;
  }
  
  return E_FAIL;
}

/************************************************************************
 * Storage32BaseImpl_RenameElement (IStorage)
 *
 * This method will rename the specified element. 
 *
 * See Windows documentation for more details on IStorage methods.
 * 
 * Implementation notes: The method used to rename consists of creating a clone 
 *    of the deleted StgProperty object setting it with the new name and to 
 *    perform a DestroyElement of the old StgProperty.
 */
HRESULT WINAPI StorageBaseImpl_RenameElement(
            IStorage*        iface,
            const OLECHAR*   pwcsOldName,  /* [in] */
            const OLECHAR*   pwcsNewName)  /* [in] */
{
  ICOM_THIS(StorageBaseImpl,iface);
  IEnumSTATSTGImpl* propertyEnumeration;
  StgProperty       currentProperty;
  ULONG             foundPropertyIndex;

  Print(MAX_TRACE, ("(%p, %S, %S)\n", 
	iface, pwcsOldName, pwcsNewName));

  /*
   * Create a property enumeration to search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(This->ancestorStorage,
                                                   This->rootPropertySetIndex);

  /*
   * Search the enumeration for the new property name
   */
  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(propertyEnumeration,
                                                     pwcsNewName,
                                                     &currentProperty);

  if (foundPropertyIndex != PROPERTY_NULL)
  {
    /*
     * There is already a property with the new name
     */
    IEnumSTATSTGImpl_Destroy(propertyEnumeration);
    return STG_E_FILEALREADYEXISTS;
  }

  IEnumSTATSTGImpl_Reset((IEnumSTATSTG*)propertyEnumeration);

  /*
   * Search the enumeration for the old property name
   */
  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(propertyEnumeration,
                                                     pwcsOldName,
                                                     &currentProperty);

  /*
   * Delete the property enumeration since we don't need it anymore
   */
  IEnumSTATSTGImpl_Destroy(propertyEnumeration);

  if (foundPropertyIndex != PROPERTY_NULL)
  {
    StgProperty renamedProperty;
    ULONG       renamedPropertyIndex;

    /*
     * Setup a new property for the renamed property
     */
    renamedProperty.sizeOfNameString = 
      ( lstrlenW(pwcsNewName)+1 ) * sizeof(WCHAR);
  
    if (renamedProperty.sizeOfNameString > PROPERTY_NAME_BUFFER_LEN)
      return STG_E_INVALIDNAME;
  
    lstrcpyW(renamedProperty.name, pwcsNewName);
 
    renamedProperty.propertyType  = currentProperty.propertyType;
    renamedProperty.startingBlock = currentProperty.startingBlock;
    renamedProperty.size.u.LowPart  = currentProperty.size.u.LowPart;
    renamedProperty.size.u.HighPart = currentProperty.size.u.HighPart;
  
    renamedProperty.previousProperty = PROPERTY_NULL;
    renamedProperty.nextProperty     = PROPERTY_NULL;
  
    /*
     * Bring the dirProperty link in case it is a storage and in which
     * case the renamed storage elements don't require to be reorganized.
     */
    renamedProperty.dirProperty = currentProperty.dirProperty;
  
    /* call CoFileTime to get the current time 
    renamedProperty.timeStampS1
    renamedProperty.timeStampD1
    renamedProperty.timeStampS2
    renamedProperty.timeStampD2
    renamedProperty.propertyUniqueID 
    */
  
    /* 
     * Obtain a free property in the property chain
     */
    renamedPropertyIndex = getFreeProperty(This->ancestorStorage);
  
    /*
     * Save the new property into the new property spot
     */  
    StorageImpl_WriteProperty(
      This->ancestorStorage,
      renamedPropertyIndex, 
      &renamedProperty);
  
    /* 
     * Find a spot in the property chain for our newly created property.
     */
    updatePropertyChain(
      (StorageImpl*)This,
      renamedPropertyIndex, 
      renamedProperty);

    /*
     * At this point the renamed property has been inserted in the tree, 
     * now, before to Destroy the old property we must zeroed it's dirProperty 
     * otherwise the DestroyProperty below will zap it all and we do not want 
     * this to happen.
     * Also, we fake that the old property is a storage so the DestroyProperty
     * will not do a SetSize(0) on the stream data.
     * 
     * This means that we need to tweek the StgProperty if it is a stream or a
     * non empty storage.
     */
    StorageImpl_ReadProperty(This->ancestorStorage,
                             foundPropertyIndex,
                             &currentProperty);

    currentProperty.dirProperty  = PROPERTY_NULL;
    currentProperty.propertyType = PROPTYPE_STORAGE;
    StorageImpl_WriteProperty(
      This->ancestorStorage,
      foundPropertyIndex, 
      &currentProperty);

    /* 
     * Invoke Destroy to get rid of the ole property and automatically redo 
     * the linking of it's previous and next members... 
     */ 
    StorageImpl_DestroyElement((IStorage*)This->ancestorStorage, pwcsOldName); 

  }
  else
  {
    /*
     * There is no property with the old name
     */
    return STG_E_FILENOTFOUND;
  }

  return S_OK;
}

/************************************************************************
 * Storage32BaseImpl_CreateStream (IStorage)
 *
 * This method will create a stream object within this storage 
 *
 * See Windows documentation for more details on IStorage methods.
 */
HRESULT WINAPI StorageBaseImpl_CreateStream(
            IStorage*        iface,
            const OLECHAR*   pwcsName,  /* [string][in] */
            DWORD            grfMode,   /* [in] */
            DWORD            reserved1, /* [in] */
            DWORD            reserved2, /* [in] */
            IStream**        ppstm)     /* [out] */
{
  ICOM_THIS(StorageBaseImpl,iface);
  IEnumSTATSTGImpl* propertyEnumeration;
  StgStreamImpl*    newStream;
  StgProperty       currentProperty, newStreamProperty;
  ULONG             foundPropertyIndex, newPropertyIndex;

  Print(MAX_TRACE, ("(%p, %S, %lx, %ld, %ld, %p)\n", 
	iface, pwcsName, grfMode, 
	reserved1, reserved2, ppstm));

  /*
   * Validate parameters
   */
  if (ppstm == 0)
    return STG_E_INVALIDPOINTER;

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * As documented.
   */
  if ( !(grfMode & STGM_SHARE_EXCLUSIVE) ||
        (grfMode & STGM_DELETEONRELEASE) ||
        (grfMode & STGM_TRANSACTED) )
    return STG_E_INVALIDFUNCTION;

  /*
   * Initialize the out parameter
   */
  *ppstm = 0;

  /*
   * Create a property enumeration to search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(This->ancestorStorage,
                                                   This->rootPropertySetIndex);

  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(propertyEnumeration,
                                                     pwcsName,
                                                     &currentProperty);

  IEnumSTATSTGImpl_Destroy(propertyEnumeration);

  if (foundPropertyIndex != PROPERTY_NULL)
  {
    /*
     * An element with this name already exists 
     */
    if (grfMode & STGM_CREATE)
    {
      IStorage_DestroyElement(iface, pwcsName); 
    }
    else 
      return STG_E_FILEALREADYEXISTS;
  }

  /* 
   * memset the empty property 
   */
  memset(&newStreamProperty, 0, sizeof(StgProperty));

  newStreamProperty.sizeOfNameString =
      ( lstrlenW(pwcsName)+1 ) * sizeof(WCHAR);

  if (newStreamProperty.sizeOfNameString > PROPERTY_NAME_BUFFER_LEN)
    return STG_E_INVALIDNAME;

  lstrcpyW(newStreamProperty.name, pwcsName);

  newStreamProperty.propertyType  = PROPTYPE_STREAM;
  newStreamProperty.startingBlock = BLOCK_END_OF_CHAIN;
  newStreamProperty.size.u.LowPart  = 0;
  newStreamProperty.size.u.HighPart = 0;

  newStreamProperty.previousProperty = PROPERTY_NULL;
  newStreamProperty.nextProperty     = PROPERTY_NULL;
  newStreamProperty.dirProperty      = PROPERTY_NULL;

  /* call CoFileTime to get the current time 
  newStreamProperty.timeStampS1
  newStreamProperty.timeStampD1
  newStreamProperty.timeStampS2
  newStreamProperty.timeStampD2
  */

  /*  newStreamProperty.propertyUniqueID */

  /*
   * Get a free property or create a new one 
   */
  newPropertyIndex = getFreeProperty(This->ancestorStorage);

  /*
   * Save the new property into the new property spot
   */  
  StorageImpl_WriteProperty(
    This->ancestorStorage,
    newPropertyIndex, 
    &newStreamProperty);

  /* 
   * Find a spot in the property chain for our newly created property.
   */
  updatePropertyChain(
    (StorageImpl*)This,
    newPropertyIndex, 
    newStreamProperty);

  /* 
   * Open the stream to return it.
   */
  newStream = StgStreamImpl_Construct(This, grfMode, newPropertyIndex);

  if (newStream != 0)
  {
    *ppstm = (IStream*)newStream;

    /*
     * Since we are returning a pointer to the interface, we have to nail down
     * the reference.
     */
    StgStreamImpl_AddRef(*ppstm);
  }
  else
  {
    return STG_E_INSUFFICIENTMEMORY;
  }

  return S_OK;
}

/************************************************************************
 * Storage32BaseImpl_SetClass (IStorage)
 *
 * This method will write the specified CLSID in the property of this 
 * storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
HRESULT WINAPI StorageBaseImpl_SetClass(
  IStorage*        iface,
  REFCLSID         clsid) /* [in] */
{
  ICOM_THIS(StorageBaseImpl,iface);
  HRESULT hRes = E_FAIL;
  StgProperty curProperty;
  BOOL success;

  Print(MAX_TRACE, ("(%p, %p)\n", iface, clsid));
  
  success = StorageImpl_ReadProperty(This->ancestorStorage,
                                       This->rootPropertySetIndex,
                                       &curProperty);
  if (success)
  {
    curProperty.propertyUniqueID = *clsid;

    success =  StorageImpl_WriteProperty(This->ancestorStorage,
                                           This->rootPropertySetIndex,
                                           &curProperty);
    if (success)
      hRes = S_OK;
  }

  return hRes;
}

/************************************************************************
** Storage32Impl implementation
*/
        
/************************************************************************
 * Storage32Impl_CreateStorage (IStorage)
 *
 * This method will create the storage object within the provided storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
HRESULT WINAPI StorageImpl_CreateStorage( 
  IStorage*      iface,
  const OLECHAR  *pwcsName, /* [string][in] */ 
  DWORD            grfMode,   /* [in] */ 
  DWORD            reserved1, /* [in] */ 
  DWORD            reserved2, /* [in] */ 
  IStorage       **ppstg)   /* [out] */ 
{
  StorageImpl* const This=(StorageImpl*)iface;

  IEnumSTATSTGImpl *propertyEnumeration;
  StgProperty      currentProperty;
  StgProperty      newProperty;
  ULONG            foundPropertyIndex;
  ULONG            newPropertyIndex;
  HRESULT          hr;

  Print(MAX_TRACE, ("(%p, %S, %lx, %ld, %ld, %p)\n", 
	iface, pwcsName, grfMode, 
	reserved1, reserved2, ppstg));
  
  /*
   * Validate parameters
   */
  if (ppstg == 0)
    return STG_E_INVALIDPOINTER;

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ) ||
       (grfMode & STGM_DELETEONRELEASE) )
    return STG_E_INVALIDFLAG;

  /*
   * Initialize the out parameter
   */
  *ppstg = 0;

  /*
   * Create a property enumeration and search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct( This->ancestorStorage,
                                                    This->rootPropertySetIndex);

  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(propertyEnumeration,
                                                     pwcsName,
                                                     &currentProperty);
  IEnumSTATSTGImpl_Destroy(propertyEnumeration);

  if (foundPropertyIndex != PROPERTY_NULL)
  {
    /*
     * An element with this name already exists 
     */
    if (grfMode & STGM_CREATE)
      IStorage_DestroyElement(iface, pwcsName); 
    else 
      return STG_E_FILEALREADYEXISTS;
  }

  /* 
   * memset the empty property 
   */
  memset(&newProperty, 0, sizeof(StgProperty));

  newProperty.sizeOfNameString = (lstrlenW(pwcsName)+1)*sizeof(WCHAR);

  if (newProperty.sizeOfNameString > PROPERTY_NAME_BUFFER_LEN)
    return STG_E_INVALIDNAME;

  lstrcpyW(newProperty.name, pwcsName);

  newProperty.propertyType  = PROPTYPE_STORAGE;
  newProperty.startingBlock = BLOCK_END_OF_CHAIN;
  newProperty.size.u.LowPart  = 0;
  newProperty.size.u.HighPart = 0;

  newProperty.previousProperty = PROPERTY_NULL;
  newProperty.nextProperty     = PROPERTY_NULL;
  newProperty.dirProperty      = PROPERTY_NULL;

  /* call CoFileTime to get the current time 
  newProperty.timeStampS1
  newProperty.timeStampD1
  newProperty.timeStampS2
  newProperty.timeStampD2
  */

  /*  newStorageProperty.propertyUniqueID */

  /* 
   * Obtain a free property in the property chain
   */
  newPropertyIndex = getFreeProperty(This->ancestorStorage);

  /*
   * Save the new property into the new property spot
   */  
  StorageImpl_WriteProperty(
    This->ancestorStorage,
    newPropertyIndex, 
    &newProperty);

  /* 
   * Find a spot in the property chain for our newly created property.
   */
  updatePropertyChain(
    This,
    newPropertyIndex, 
    newProperty);

  /* 
   * Open it to get a pointer to return.
   */
  hr = IStorage_OpenStorage(
         iface,
         (OLECHAR*)pwcsName,
         0,
         grfMode,
         0,
         0,
         ppstg);

  if( (hr != S_OK) || (*ppstg == NULL))
  {
    return hr;
  }

  
  return S_OK;
}


/***************************************************************************
 *
 * Internal Method
 *
 * Get a free property or create a new one.
 */
static ULONG getFreeProperty(
  StorageImpl *storage)
{
  ULONG       currentPropertyIndex = 0;
  ULONG       newPropertyIndex     = PROPERTY_NULL;
  BOOL      readSuccessful        = TRUE;
  StgProperty currentProperty;

  do
  {
    /*
     * Start by reading the root property
     */
    readSuccessful = StorageImpl_ReadProperty(storage->ancestorStorage,
                                               currentPropertyIndex,
                                               &currentProperty);
    if (readSuccessful)
    {
      if (currentProperty.sizeOfNameString == 0)
      {
        /* 
         * The property existis and is available, we found it.
         */
        newPropertyIndex = currentPropertyIndex;
      }
    }
    else
    {
      /*
       * We exhausted the property list, we will create more space below
       */
      newPropertyIndex = currentPropertyIndex;
    }
    currentPropertyIndex++;

  } while (newPropertyIndex == PROPERTY_NULL);

  /* 
   * grow the property chain 
   */
  if (! readSuccessful)
  {
    StgProperty    emptyProperty;
    ULARGE_INTEGER newSize;
    ULONG          propertyIndex;
    ULONG          lastProperty  = 0;
    ULONG          blockCount    = 0;

    /* 
     * obtain the new count of property blocks 
     */
    blockCount = BlockChainStream_GetCount(
                   storage->ancestorStorage->rootBlockChain)+1;

    /* 
     * initialize the size used by the property stream 
     */
    newSize.u.HighPart = 0;
    newSize.u.LowPart  = storage->bigBlockSize * blockCount;

    /* 
     * add a property block to the property chain 
     */
    BlockChainStream_SetSize(storage->ancestorStorage->rootBlockChain, newSize);

    /* 
     * memset the empty property in order to initialize the unused newly 
     * created property
     */
    memset(&emptyProperty, 0, sizeof(StgProperty));

    /* 
     * initialize them
     */
    lastProperty = storage->bigBlockSize / PROPSET_BLOCK_SIZE * blockCount; 
    
    for(
      propertyIndex = newPropertyIndex;
      propertyIndex < lastProperty;
      propertyIndex++)
    {
      StorageImpl_WriteProperty(
        storage->ancestorStorage,
        propertyIndex, 
        &emptyProperty);
    }
  }

  return newPropertyIndex;
}

/****************************************************************************
 *
 * Internal Method
 *
 * Case insensitive comparaison of StgProperty.name by first considering 
 * their size.
 *
 * Returns <0 when newPrpoerty < currentProperty
 *         >0 when newPrpoerty > currentProperty
 *          0 when newPrpoerty == currentProperty
 */
static LONG propertyNameCmp(
  OLECHAR *newProperty,
  OLECHAR *currentProperty)
{
  LONG diff      = lstrlenW(newProperty) - lstrlenW(currentProperty);

  if (diff == 0) 
  {
    /* 
     * We compare the string themselves only when they are of the same lenght
     */
    diff = lstrcmpiW( newProperty, currentProperty);
  }

  return diff;  
}

/****************************************************************************
 *
 * Internal Method
 *
 * Properly link this new element in the property chain.
 */
static void updatePropertyChain(
  StorageImpl *storage,
  ULONG         newPropertyIndex,
  StgProperty   newProperty) 
{
  StgProperty currentProperty;

  /*
   * Read the root property
   */
  StorageImpl_ReadProperty(storage->ancestorStorage,
                             storage->rootPropertySetIndex,
                             &currentProperty);

  if (currentProperty.dirProperty != PROPERTY_NULL)
  {
    /* 
     * The root storage contains some element, therefore, start the research
     * for the appropriate location.
     */
    BOOL found = 0;
    ULONG  current, next, previous, currentPropertyId;

    /*
     * Keep the StgProperty sequence number of the storage first property
     */
    currentPropertyId = currentProperty.dirProperty;

    /*
     * Read 
     */
    StorageImpl_ReadProperty(storage->ancestorStorage,
                               currentProperty.dirProperty,
                               &currentProperty);

    previous = currentProperty.previousProperty;
    next     = currentProperty.nextProperty;
    current  = currentPropertyId;

    while (found == 0)
    {
      LONG diff = propertyNameCmp( newProperty.name, currentProperty.name);
  
      if (diff < 0)
      {
        if (previous != PROPERTY_NULL)
        {
          StorageImpl_ReadProperty(storage->ancestorStorage,
                                     previous,
                                     &currentProperty);
          current = previous;
        }
        else
        {
          currentProperty.previousProperty = newPropertyIndex;
          StorageImpl_WriteProperty(storage->ancestorStorage,
                                      current,
                                      &currentProperty);
          found = 1;
        }
      }
      else if (diff > 0)
      {
        if (next != PROPERTY_NULL)
        {
          StorageImpl_ReadProperty(storage->ancestorStorage,
                                     next,
                                     &currentProperty);
          current = next;
        }
        else
        {
          currentProperty.nextProperty = newPropertyIndex;
          StorageImpl_WriteProperty(storage->ancestorStorage,
                                      current,
                                      &currentProperty);
          found = 1;
        }
      }
      else
      {
	/*
	 * Trying to insert an item with the same name in the
	 * subtree structure.
	 */
	assert(FALSE);
      }

      previous = currentProperty.previousProperty;
      next     = currentProperty.nextProperty;
    }
  }
  else
  {
    /* 
     * The root storage is empty, link the new property to it's dir property
     */
    currentProperty.dirProperty = newPropertyIndex;
    StorageImpl_WriteProperty(storage->ancestorStorage,
                                storage->rootPropertySetIndex,
                                &currentProperty);
  }
}

      
/*************************************************************************
 * CopyTo (IStorage)
 */
HRESULT WINAPI StorageImpl_CopyTo( 
  IStorage*   iface,
  DWORD       ciidExclude,  /* [in] */ 
  const IID*  rgiidExclude, /* [size_is][unique][in] */ 
  SNB         snbExclude,   /* [unique][in] */ 
  IStorage*   pstgDest)     /* [unique][in] */ 
{
  IEnumSTATSTG *elements     = 0;
  STATSTG      curElement, strStat;
  HRESULT      hr;
  IStorage     *pstgTmp, *pstgChild;
  IStream      *pstrTmp, *pstrChild;

  if ((ciidExclude != 0) || (rgiidExclude != NULL) || (snbExclude != NULL))
    UNIMPLEMENTED;

  Print(MAX_TRACE, ("(%p, %ld, %p, %p, %p)\n", 
	iface, ciidExclude, rgiidExclude, 
	snbExclude, pstgDest));

  /*
   * Perform a sanity check
   */
  if ( pstgDest == 0 )
    return STG_E_INVALIDPOINTER;

  /* 
   * Enumerate the elements
   */
  hr = IStorage_EnumElements( iface, 0, 0, 0, &elements );

  if ( hr != S_OK )
    return hr;

  /*
   * set the class ID
   */
  IStorage_Stat( iface, &curElement, STATFLAG_NONAME);
  IStorage_SetClass( pstgDest, &curElement.clsid );
  
  do
  {
    /*
     * Obtain the next element
     */
    hr = IEnumSTATSTG_Next( elements, 1, &curElement, NULL );

    if ( hr == S_FALSE )
    {
      hr = S_OK;   /* done, every element has been copied */
      break;
    }

    if (curElement.type == STGTY_STORAGE)
    {
      /*
       * open child source storage
       */
      hr = IStorage_OpenStorage( iface, curElement.pwcsName, NULL,
				 STGM_READ|STGM_SHARE_EXCLUSIVE,
				 NULL, 0, &pstgChild );

      if (hr != S_OK)
        break;

      /*
       * Check if destination storage is not a child of the source
       * storage, which will cause an infinite loop
       */
      if (pstgChild == pstgDest)
      {
	IEnumSTATSTG_Release(elements);

	return STG_E_ACCESSDENIED;
      }
            
      /*
       * create a new storage in destination storage
       */
      hr = IStorage_CreateStorage( pstgDest, curElement.pwcsName,
                                   STGM_FAILIFTHERE|STGM_WRITE|STGM_SHARE_EXCLUSIVE,
				   0, 0,
                                   &pstgTmp );
      /*
       * if it already exist, don't create a new one use this one
       */
      if (hr == STG_E_FILEALREADYEXISTS)
      {
        hr = IStorage_OpenStorage( pstgDest, curElement.pwcsName, NULL,
                                   STGM_WRITE|STGM_SHARE_EXCLUSIVE,
                                   NULL, 0, &pstgTmp );
      }
        
      if (hr != S_OK)
        break;

        
      /*
       * do the copy recursively
       */
      hr = IStorage_CopyTo( pstgChild, ciidExclude, rgiidExclude,
                               snbExclude, pstgTmp );
                                
      IStorage_Release( pstgTmp );
      IStorage_Release( pstgChild );
    }
    else if (curElement.type == STGTY_STREAM)
    {
      /*
       * create a new stream in destination storage. If the stream already
       * exist, it will be deleted and a new one will be created.
       */
      hr = IStorage_CreateStream( pstgDest, curElement.pwcsName,
                                  STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE,
                                  0, 0, &pstrTmp );

      if (hr != S_OK)
        break;

      /*
       * open child stream storage
       */
      hr = IStorage_OpenStream( iface, curElement.pwcsName, NULL,
				STGM_READ|STGM_SHARE_EXCLUSIVE,
				0, &pstrChild );

      if (hr != S_OK)
        break;

      /*
       * Get the size of the source stream
       */
      IStream_Stat( pstrChild, &strStat, STATFLAG_NONAME );

      /*
       * Set the size of the destination stream.
       */
      IStream_SetSize(pstrTmp, strStat.cbSize);
      
      /*
       * do the copy
       */
      hr = IStream_CopyTo( pstrChild, pstrTmp, strStat.cbSize,
                           NULL, NULL );
                                
      IStream_Release( pstrTmp );
      IStream_Release( pstrChild );
    }
    else
    {
      Print(MIN_TRACE, ("unknown element type: %ld\n", curElement.type));
    }

  } while (hr == S_OK);

  /*
   * Clean-up
   */
  IEnumSTATSTG_Release(elements);
  
  return hr;
}
        
/*************************************************************************
 * MoveElementTo (IStorage)
 */
HRESULT WINAPI StorageImpl_MoveElementTo( 
  IStorage*     iface,
  const OLECHAR *pwcsName,   /* [string][in] */ 
  IStorage      *pstgDest,   /* [unique][in] */ 
  const OLECHAR *pwcsNewName,/* [string][in] */ 
  DWORD           grfFlags)    /* [in] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}
        
/*************************************************************************
 * Commit (IStorage)
 */
HRESULT WINAPI StorageImpl_Commit( 
  IStorage*   iface,
  DWORD         grfCommitFlags)/* [in] */ 
{
  UNIMPLEMENTED;
  return S_OK;
}
        
/*************************************************************************
 * Revert (IStorage)
 */
HRESULT WINAPI StorageImpl_Revert( 
  IStorage* iface)
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/*************************************************************************
 * DestroyElement (IStorage)
 *
 * Stategy: This implementation is build this way for simplicity not for speed. 
 *          I always delete the top most element of the enumeration and adjust
 *          the deleted element pointer all the time.  This takes longer to 
 *          do but allow to reinvoke DestroyElement whenever we encounter a 
 *          storage object.  The optimisation reside in the usage of another
 *          enumeration stategy that would give all the leaves of a storage 
 *          first. (postfix order)
 */
HRESULT WINAPI StorageImpl_DestroyElement( 
  IStorage*     iface,
  const OLECHAR *pwcsName)/* [string][in] */ 
{
  StorageImpl* const This=(StorageImpl*)iface;

  IEnumSTATSTGImpl* propertyEnumeration;
  HRESULT           hr = S_OK;
  BOOL            res;
  StgProperty       propertyToDelete;
  StgProperty       parentProperty;
  ULONG             foundPropertyIndexToDelete;
  ULONG             typeOfRelation;
  ULONG             parentPropertyId;

  Print(MAX_TRACE, ("(%p, %S)\n", 
	iface, pwcsName));

  /*
   * Perform a sanity check on the parameters.
   */
  if (pwcsName==NULL) 
    return STG_E_INVALIDPOINTER;
  
  /*
   * Create a property enumeration to search the property with the given name
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(
    This->ancestorStorage, 
    This->rootPropertySetIndex);
  
  foundPropertyIndexToDelete = IEnumSTATSTGImpl_FindProperty(
    propertyEnumeration,
    pwcsName,
    &propertyToDelete);

  IEnumSTATSTGImpl_Destroy(propertyEnumeration);

  if ( foundPropertyIndexToDelete == PROPERTY_NULL )  
  {
    return STG_E_FILENOTFOUND;
  }

  /* 
   * Find the parent property of the property to delete (the one that 
   * link to it).  If This->dirProperty == foundPropertyIndexToDelete, 
   * the parent is This. Otherwise, the parent is one of it's sibling...
   */

  /* 
   * First, read This's StgProperty..
   */
  res = StorageImpl_ReadProperty( 
          This->ancestorStorage,
          This->rootPropertySetIndex,
          &parentProperty);

  assert(res==TRUE);

  /* 
   * Second, check to see if by any chance the actual storage (This) is not
   * the parent of the property to delete... We never know...
   */
  if ( parentProperty.dirProperty == foundPropertyIndexToDelete )
  {
    /* 
     * Set data as it would have been done in the else part...
     */
    typeOfRelation   = PROPERTY_RELATION_DIR;
    parentPropertyId = This->rootPropertySetIndex;
  }
  else 
  { 
    /*
     * Create a property enumeration to search the parent properties, and 
     * delete it once done.
     */
    IEnumSTATSTGImpl* propertyEnumeration2;

    propertyEnumeration2 = IEnumSTATSTGImpl_Construct(
      This->ancestorStorage, 
      This->rootPropertySetIndex);
  
    typeOfRelation = IEnumSTATSTGImpl_FindParentProperty(
      propertyEnumeration2,
      foundPropertyIndexToDelete,
      &parentProperty,
      &parentPropertyId);

    IEnumSTATSTGImpl_Destroy(propertyEnumeration2);
  }

  if ( propertyToDelete.propertyType == PROPTYPE_STORAGE ) 
  {
    hr = deleteStorageProperty(
           This, 
           foundPropertyIndexToDelete,
           propertyToDelete);
  } 
  else if ( propertyToDelete.propertyType == PROPTYPE_STREAM )
  {
    hr = deleteStreamProperty(
           This, 
           foundPropertyIndexToDelete,
           propertyToDelete);
  }

  if (hr!=S_OK) 
    return hr;

  /*
   * Adjust the property chain
   */
  hr = adjustPropertyChain(
        This,
        propertyToDelete, 
        parentProperty,
        parentPropertyId,
        typeOfRelation);

  return hr;
}


/*********************************************************************
 *
 * Internal Method
 *
 * Perform the deletion of a complete storage node
 *
 */
static HRESULT deleteStorageProperty(
  StorageImpl *parentStorage,
  ULONG        indexOfPropertyToDelete,
  StgProperty  propertyToDelete)
{
  IEnumSTATSTG *elements     = 0;
  IStorage   *childStorage = 0;
  STATSTG      currentElement;
  HRESULT      hr;
  HRESULT      destroyHr = S_OK;

  /*
   * Open the storage and enumerate it
   */
  hr = StorageBaseImpl_OpenStorage(
        (IStorage*)parentStorage,
        propertyToDelete.name,
        0,
        STGM_SHARE_EXCLUSIVE,
        0,
        0,
        &childStorage);

  if (hr != S_OK)
  {
    return hr;
  }

  /* 
   * Enumerate the elements
   */
  IStorage_EnumElements( childStorage, 0, 0, 0, &elements);

  do
  {
    /*
     * Obtain the next element
     */
    hr = IEnumSTATSTG_Next(elements, 1, &currentElement, NULL);
    if (hr==S_OK)
    {
      destroyHr = StorageImpl_DestroyElement(
                    (IStorage*)childStorage, 
                    (OLECHAR*)currentElement.pwcsName);

      CoTaskMemFree(currentElement.pwcsName);
    }

    /*
     * We need to Reset the enumeration every time because we delete elements
     * and the enumeration could be invalid
     */
    IEnumSTATSTG_Reset(elements);

  } while ((hr == S_OK) && (destroyHr == S_OK));

  /*
   * Invalidate the property by zeroing it's name member.
   */
  propertyToDelete.sizeOfNameString = 0;

  StorageImpl_WriteProperty(parentStorage->ancestorStorage,
                            indexOfPropertyToDelete,
                            &propertyToDelete);

  IStorage_Release(childStorage);
  IEnumSTATSTG_Release(elements);
    
  return destroyHr;
}

/*********************************************************************
 *
 * Internal Method
 *
 * Perform the deletion of a stream node
 *
 */
static HRESULT deleteStreamProperty(
  StorageImpl *parentStorage,
  ULONG         indexOfPropertyToDelete,
  StgProperty   propertyToDelete)
{
  IStream      *pis;
  HRESULT        hr;
  ULARGE_INTEGER size;

  size.u.HighPart = 0;
  size.u.LowPart = 0;

  hr = StorageBaseImpl_OpenStream(
         (IStorage*)parentStorage,
         (OLECHAR*)propertyToDelete.name,
         NULL,
         STGM_WRITE | STGM_SHARE_EXCLUSIVE,
         0,
         &pis);
    
  if (hr!=S_OK)
  {
    return(hr);
  }

  /* 
   * Zap the stream 
   */ 
  hr = IStream_SetSize(pis, size); 

  if(hr != S_OK)
  {
    return hr;
  }

  /*
   * Release the stream object.
   */
  IStream_Release(pis);

  /* 
   * Invalidate the property by zeroing it's name member.
   */
  propertyToDelete.sizeOfNameString = 0;

  /* 
   * Here we should re-read the property so we get the updated pointer
   * but since we are here to zap it, I don't do it...
   */
  StorageImpl_WriteProperty(
    parentStorage->ancestorStorage, 
    indexOfPropertyToDelete,
    &propertyToDelete);

  return S_OK;
}

/*********************************************************************
 *
 * Internal Method
 *
 * Finds a placeholder for the StgProperty within the Storage
 *
 */
static HRESULT findPlaceholder(
  StorageImpl *storage,
  ULONG         propertyIndexToStore,
  ULONG         storePropertyIndex,
  INT         typeOfRelation)
{
  StgProperty storeProperty;
  HRESULT     hr = S_OK;
  BOOL      res = TRUE;

  /*
   * Read the storage property
   */
  res = StorageImpl_ReadProperty(
          storage->ancestorStorage,
          storePropertyIndex, 
          &storeProperty);

  if(! res)
  {
    return E_FAIL;
  }

  if (typeOfRelation == PROPERTY_RELATION_PREVIOUS)
  {
    if (storeProperty.previousProperty != PROPERTY_NULL)
    {
      return findPlaceholder(
               storage,
               propertyIndexToStore, 
               storeProperty.previousProperty,
               typeOfRelation);
    }
    else
    {
      storeProperty.previousProperty = propertyIndexToStore;
    }
  }
  else if (typeOfRelation == PROPERTY_RELATION_NEXT)
  {
    if (storeProperty.nextProperty != PROPERTY_NULL)
    {
      return findPlaceholder(
               storage,
               propertyIndexToStore, 
               storeProperty.nextProperty,
               typeOfRelation);
    }
    else
    {
      storeProperty.nextProperty = propertyIndexToStore;
    }
  } 
  else if (typeOfRelation == PROPERTY_RELATION_DIR)
  {
    if (storeProperty.dirProperty != PROPERTY_NULL)
    {
      return findPlaceholder(
               storage,
               propertyIndexToStore, 
               storeProperty.dirProperty,
               typeOfRelation);
    }
    else
    {
      storeProperty.dirProperty = propertyIndexToStore;
    }
  }

  hr = StorageImpl_WriteProperty(
         storage->ancestorStorage,
         storePropertyIndex, 
         &storeProperty);

  if(! hr)
  {
    return E_FAIL;
  }

  return S_OK;
}

/*************************************************************************
 *
 * Internal Method
 *
 * This method takes the previous and the next property link of a property 
 * to be deleted and find them a place in the Storage.
 */
static HRESULT adjustPropertyChain( 
  StorageImpl *This,
  StgProperty   propertyToDelete,
  StgProperty   parentProperty,
  ULONG         parentPropertyId,
  INT         typeOfRelation)
{
  ULONG   newLinkProperty        = PROPERTY_NULL;
  BOOL  needToFindAPlaceholder = FALSE;
  ULONG   storeNode              = PROPERTY_NULL;
  ULONG   toStoreNode            = PROPERTY_NULL;
  INT   relationType           = 0;
  HRESULT hr                     = S_OK;
  BOOL  res                    = TRUE;
  
  if (typeOfRelation == PROPERTY_RELATION_PREVIOUS) 
  {
    if (propertyToDelete.previousProperty != PROPERTY_NULL)  
    {
      /* 
       * Set the parent previous to the property to delete previous
       */
      newLinkProperty = propertyToDelete.previousProperty;

      if (propertyToDelete.nextProperty != PROPERTY_NULL)  
      {
        /*
         * We also need to find a storage for the other link, setup variables 
         * to do this at the end...
         */      
        needToFindAPlaceholder = TRUE;
        storeNode              = propertyToDelete.previousProperty;
        toStoreNode            = propertyToDelete.nextProperty;
        relationType           = PROPERTY_RELATION_NEXT;
      }
    } 
    else if (propertyToDelete.nextProperty != PROPERTY_NULL)  
    {
      /* 
       * Set the parent previous to the property to delete next
       */
      newLinkProperty = propertyToDelete.nextProperty;
    }
   
    /* 
     * Link it for real...
     */ 
    parentProperty.previousProperty = newLinkProperty;
  
  } 
  else if (typeOfRelation == PROPERTY_RELATION_NEXT) 
  {
    if (propertyToDelete.previousProperty != PROPERTY_NULL)  
    {
      /* 
       * Set the parent next to the property to delete next previous
       */
      newLinkProperty = propertyToDelete.previousProperty;
      
      if (propertyToDelete.nextProperty != PROPERTY_NULL)  
      {
        /*
         * We also need to find a storage for the other link, setup variables 
         * to do this at the end...
         */      
        needToFindAPlaceholder = TRUE;
        storeNode              = propertyToDelete.previousProperty;
        toStoreNode            = propertyToDelete.nextProperty;
        relationType           = PROPERTY_RELATION_NEXT;
      }
    } 
    else if (propertyToDelete.nextProperty != PROPERTY_NULL)  
    {
      /* 
       * Set the parent next to the property to delete next
       */
      newLinkProperty = propertyToDelete.nextProperty;
    }

    /* 
     * Link it for real...
     */ 
    parentProperty.nextProperty = newLinkProperty;
  } 
  else /* (typeOfRelation == PROPERTY_RELATION_DIR) */
  {
    if (propertyToDelete.previousProperty != PROPERTY_NULL) 
    {
      /* 
       * Set the parent dir to the property to delete previous
       */
      newLinkProperty = propertyToDelete.previousProperty;

      if (propertyToDelete.nextProperty != PROPERTY_NULL)  
      {
        /*
         * We also need to find a storage for the other link, setup variables 
         * to do this at the end...
         */      
        needToFindAPlaceholder = TRUE;
        storeNode              = propertyToDelete.previousProperty;
        toStoreNode            = propertyToDelete.nextProperty;
        relationType           = PROPERTY_RELATION_NEXT;
      }
    } 
    else if (propertyToDelete.nextProperty != PROPERTY_NULL)  
    {
      /* 
       * Set the parent dir to the property to delete next
       */
      newLinkProperty = propertyToDelete.nextProperty;
    }

    /* 
     * Link it for real...
     */ 
    parentProperty.dirProperty = newLinkProperty;
  }

  /* 
   * Write back the parent property    
   */
  res = StorageImpl_WriteProperty(
          This->ancestorStorage, 
          parentPropertyId,
          &parentProperty);
  if(! res)
  {
    return E_FAIL;
  }

  /*
   * If a placeholder is required for the other link, then, find one and 
   * get out of here...
   */
  if (needToFindAPlaceholder) 
  {
    hr = findPlaceholder(
           This, 
           toStoreNode, 
           storeNode,
           relationType);
  }

  return hr;
}


/******************************************************************************
 * SetElementTimes (IStorage)
 */
HRESULT WINAPI StorageImpl_SetElementTimes( 
  IStorage*     iface,
  const OLECHAR *pwcsName,/* [string][in] */ 
  const FILETIME  *pctime,  /* [in] */ 
  const FILETIME  *patime,  /* [in] */ 
  const FILETIME  *pmtime)  /* [in] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/******************************************************************************
 * SetStateBits (IStorage)
 */
HRESULT WINAPI StorageImpl_SetStateBits( 
  IStorage*   iface,
  DWORD         grfStateBits,/* [in] */ 
  DWORD         grfMask)     /* [in] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

HRESULT StorageImpl_Construct(
  StorageImpl* This,
  HANDLE       hFile,
  ILockBytes*  pLkbyt,
  DWORD        openFlags,
  BOOL         fileBased,
  BOOL         fileCreate)
{
  HRESULT     hr = S_OK;
  StgProperty currentProperty;
  BOOL      readSuccessful;
  ULONG       currentPropertyIndex;
  
  if ( FAILED( validateSTGM(openFlags) ))
    return STG_E_INVALIDFLAG;

  memset(This, 0, sizeof(StorageImpl));
  
  /*
   * Initialize the virtual fgunction table.
   */
  ICOM_VTBL(This)    = &Storage32Impl_Vtbl;
  This->v_destructor = &StorageImpl_Destroy;
  
  /*
   * This is the top-level storage so initialize the ancester pointer
   * to this.
   */
  This->ancestorStorage = This;
  
  /*
   * Initialize the physical support of the storage.
   */
  This->hFile = hFile;
  
  /*
   * Initialize the big block cache.
   */
  This->bigBlockSize   = DEF_BIG_BLOCK_SIZE;
  This->smallBlockSize = DEF_SMALL_BLOCK_SIZE;
  This->bigBlockFile   = BIGBLOCKFILE_Construct(hFile,
                                                pLkbyt,
                                                openFlags,
                                                This->bigBlockSize,
                                                fileBased);

  if (This->bigBlockFile == 0)
    return E_FAIL;
 
  if (fileCreate)
  {
    ULARGE_INTEGER size;
    BYTE* bigBlockBuffer;

    /*
     * Initialize all header variables:
     * - The big block depot consists of one block and it is at block 0
     * - The properties start at block 1
     * - There is no small block depot
     */
    memset( This->bigBlockDepotStart,     
            BLOCK_UNUSED, 
            sizeof(This->bigBlockDepotStart));

    This->bigBlockDepotCount    = 1;
    This->bigBlockDepotStart[0] = 0;
    This->rootStartBlock        = 1;
    This->smallBlockDepotStart  = BLOCK_END_OF_CHAIN;
    This->bigBlockSizeBits      = DEF_BIG_BLOCK_SIZE_BITS;
    This->smallBlockSizeBits    = DEF_SMALL_BLOCK_SIZE_BITS;
    This->extBigBlockDepotStart = BLOCK_END_OF_CHAIN;
    This->extBigBlockDepotCount = 0;

    StorageImpl_SaveFileHeader(This);

    /*
     * Add one block for the big block depot and one block for the properties
     */
    size.u.HighPart = 0;
    size.u.LowPart  = This->bigBlockSize * 3;
    BIGBLOCKFILE_SetSize(This->bigBlockFile, size);

    /*
     * Initialize the big block depot
     */
    bigBlockBuffer = StorageImpl_GetBigBlock(This, 0);
    memset(bigBlockBuffer, BLOCK_UNUSED, This->bigBlockSize);
    StorageUtl_WriteDWord(bigBlockBuffer, 0, BLOCK_SPECIAL);
    StorageUtl_WriteDWord(bigBlockBuffer, sizeof(ULONG), BLOCK_END_OF_CHAIN);
    StorageImpl_ReleaseBigBlock(This, bigBlockBuffer);
  }
  else
  {
    /*
     * Load the header for the file.
     */
    hr = StorageImpl_LoadFileHeader(This);

    if (FAILED(hr))
    {
      BIGBLOCKFILE_Destructor(This->bigBlockFile);

      return hr;
    }
  }

  /*
   * There is no block depot cached yet.
   */
  This->indexBlockDepotCached = 0xFFFFFFFF;

  /*
   * Start searching for free blocks with block 0.
   */
  This->prevFreeBlock = 0;
  
  /*
   * Create the block chain abstractions.
   */
  This->rootBlockChain = 
    BlockChainStream_Construct(This, &This->rootStartBlock, PROPERTY_NULL);

  This->smallBlockDepotChain = BlockChainStream_Construct(
                                 This, 
                                 &This->smallBlockDepotStart, 
                                 PROPERTY_NULL);

  /*
   * Write the root property 
   */
  if (fileCreate)
  {
    StgProperty rootProp;
    /*
     * Initialize the property chain
     */
    memset(&rootProp, 0, sizeof(rootProp));
    MultiByteToWideChar( CP_ACP, 0, rootPropertyName, -1, rootProp.name,
                         sizeof(rootProp.name)/sizeof(WCHAR) );
    rootProp.sizeOfNameString = (lstrlenW(rootProp.name)+1) * sizeof(WCHAR);
    rootProp.propertyType     = PROPTYPE_ROOT;
    rootProp.previousProperty = PROPERTY_NULL;
    rootProp.nextProperty     = PROPERTY_NULL;
    rootProp.dirProperty      = PROPERTY_NULL;
    rootProp.startingBlock    = BLOCK_END_OF_CHAIN;
    rootProp.size.u.HighPart    = 0;
    rootProp.size.u.LowPart     = 0;

    StorageImpl_WriteProperty(This, 0, &rootProp);
  }

  /*
   * Find the ID of the root int he property sets.
   */
  currentPropertyIndex = 0;
  
  do
  {
    readSuccessful = StorageImpl_ReadProperty(
                      This, 
                      currentPropertyIndex, 
                      &currentProperty);
    
    if (readSuccessful)
    {
      if ( (currentProperty.sizeOfNameString != 0 ) &&
           (currentProperty.propertyType     == PROPTYPE_ROOT) )
      {
        This->rootPropertySetIndex = currentPropertyIndex;
      }
    }

    currentPropertyIndex++;
    
  } while (readSuccessful && (This->rootPropertySetIndex == PROPERTY_NULL) );
  
  if (!readSuccessful)
  {
    /* TODO CLEANUP */
    return E_FAIL;
  }

  /*
   * Create the block chain abstraction for the small block root chain.
   */
  This->smallBlockRootChain = BlockChainStream_Construct(
                                This, 
                                NULL, 
                                This->rootPropertySetIndex);
  
  return hr;
}

void StorageImpl_Destroy(
  StorageImpl* This)
{
  Print(MAX_TRACE, ("(%p)\n", This));

  BlockChainStream_Destroy(This->smallBlockRootChain);
  BlockChainStream_Destroy(This->rootBlockChain);
  BlockChainStream_Destroy(This->smallBlockDepotChain);

  BIGBLOCKFILE_Destructor(This->bigBlockFile);
  return;
}

/******************************************************************************
 *      Storage32Impl_GetNextFreeBigBlock
 *
 * Returns the index of the next free big block.
 * If the big block depot is filled, this method will enlarge it.
 *
 */
ULONG StorageImpl_GetNextFreeBigBlock(
  StorageImpl* This)
{
  ULONG depotBlockIndexPos;
  void  *depotBuffer;
  ULONG depotBlockOffset;
  ULONG blocksPerDepot    = This->bigBlockSize / sizeof(ULONG);
  ULONG nextBlockIndex    = BLOCK_SPECIAL;
  int   depotIndex        = 0;
  ULONG freeBlock         = BLOCK_UNUSED;

  depotIndex = This->prevFreeBlock / blocksPerDepot;
  depotBlockOffset = (This->prevFreeBlock % blocksPerDepot) * sizeof(ULONG);

  /*
   * Scan the entire big block depot until we find a block marked free
   */
  while (nextBlockIndex != BLOCK_UNUSED)
  {
    if (depotIndex < COUNT_BBDEPOTINHEADER)
    {
      depotBlockIndexPos = This->bigBlockDepotStart[depotIndex];

      /*
       * Grow the primary depot.
       */
      if (depotBlockIndexPos == BLOCK_UNUSED)
      {
        depotBlockIndexPos = depotIndex*blocksPerDepot;

        /*
         * Add a block depot.
         */
        Storage32Impl_AddBlockDepot(This, depotBlockIndexPos);
        This->bigBlockDepotCount++;
        This->bigBlockDepotStart[depotIndex] = depotBlockIndexPos;

        /*
         * Flag it as a block depot.
         */
        StorageImpl_SetNextBlockInChain(This,
                                          depotBlockIndexPos,
                                          BLOCK_SPECIAL);

        /* Save new header information.
         */
        StorageImpl_SaveFileHeader(This);
      }
    }
    else
    {
      depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotIndex);

      if (depotBlockIndexPos == BLOCK_UNUSED)
      {
        /*
         * Grow the extended depot.
         */
        ULONG extIndex       = BLOCK_UNUSED;
        ULONG numExtBlocks   = depotIndex - COUNT_BBDEPOTINHEADER;
        ULONG extBlockOffset = numExtBlocks % (blocksPerDepot - 1);

        if (extBlockOffset == 0)
        {
          /* We need an extended block.
           */
          extIndex = Storage32Impl_AddExtBlockDepot(This);
          This->extBigBlockDepotCount++;
          depotBlockIndexPos = extIndex + 1;
        }
        else
          depotBlockIndexPos = depotIndex * blocksPerDepot;

        /*
         * Add a block depot and mark it in the extended block.
         */
        Storage32Impl_AddBlockDepot(This, depotBlockIndexPos);
        This->bigBlockDepotCount++;
        Storage32Impl_SetExtDepotBlock(This, depotIndex, depotBlockIndexPos);

        /* Flag the block depot.
         */
        StorageImpl_SetNextBlockInChain(This,
                                          depotBlockIndexPos,
                                          BLOCK_SPECIAL);

        /* If necessary, flag the extended depot block.
         */
        if (extIndex != BLOCK_UNUSED)
          StorageImpl_SetNextBlockInChain(This, extIndex, BLOCK_EXTBBDEPOT);

        /* Save header information.
         */
        StorageImpl_SaveFileHeader(This);
      }
    }

    depotBuffer = StorageImpl_GetROBigBlock(This, depotBlockIndexPos);

    if (depotBuffer != 0)
    {
      while ( ( (depotBlockOffset/sizeof(ULONG) ) < blocksPerDepot) &&
              ( nextBlockIndex != BLOCK_UNUSED))
      {
        StorageUtl_ReadDWord(depotBuffer, depotBlockOffset, &nextBlockIndex);

        if (nextBlockIndex == BLOCK_UNUSED)
        {
          freeBlock = (depotIndex * blocksPerDepot) +
                      (depotBlockOffset/sizeof(ULONG));
        }

        depotBlockOffset += sizeof(ULONG);
      }

      StorageImpl_ReleaseBigBlock(This, depotBuffer);
    }

    depotIndex++;
    depotBlockOffset = 0;
  }

  This->prevFreeBlock = freeBlock;

  return freeBlock;
}

/******************************************************************************
 *      Storage32Impl_AddBlockDepot
 *
 * This will create a depot block, essentially it is a block initialized
 * to BLOCK_UNUSEDs.
 */
void Storage32Impl_AddBlockDepot(StorageImpl* This, ULONG blockIndex)
{
  BYTE* blockBuffer;

  blockBuffer = StorageImpl_GetBigBlock(This, blockIndex);

  /*
   * Initialize blocks as free
   */
  memset(blockBuffer, BLOCK_UNUSED, This->bigBlockSize);

  StorageImpl_ReleaseBigBlock(This, blockBuffer);
}

/******************************************************************************
 *      Storage32Impl_GetExtDepotBlock
 *
 * Returns the index of the block that corresponds to the specified depot
 * index. This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
ULONG Storage32Impl_GetExtDepotBlock(StorageImpl* This, ULONG depotIndex)
{
  ULONG depotBlocksPerExtBlock = (This->bigBlockSize / sizeof(ULONG)) - 1;
  ULONG numExtBlocks           = depotIndex - COUNT_BBDEPOTINHEADER;
  ULONG extBlockCount          = numExtBlocks / depotBlocksPerExtBlock;
  ULONG extBlockOffset         = numExtBlocks % depotBlocksPerExtBlock;
  ULONG blockIndex             = BLOCK_UNUSED;
  ULONG extBlockIndex          = This->extBigBlockDepotStart;

  assert(depotIndex >= COUNT_BBDEPOTINHEADER);

  if (This->extBigBlockDepotStart == BLOCK_END_OF_CHAIN)
    return BLOCK_UNUSED;

  while (extBlockCount > 0)
  {
    extBlockIndex = Storage32Impl_GetNextExtendedBlock(This, extBlockIndex);
    extBlockCount--;
  }

  if (extBlockIndex != BLOCK_UNUSED)
  {
    BYTE* depotBuffer;

    depotBuffer = StorageImpl_GetROBigBlock(This, extBlockIndex);

    if (depotBuffer != 0)
    {
      StorageUtl_ReadDWord(depotBuffer,
                           extBlockOffset * sizeof(ULONG),
                           &blockIndex);

      StorageImpl_ReleaseBigBlock(This, depotBuffer);
    }
  }

  return blockIndex;
}

/******************************************************************************
 *      Storage32Impl_SetExtDepotBlock
 *
 * Associates the specified block index to the specified depot index.
 * This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
void Storage32Impl_SetExtDepotBlock(StorageImpl* This,
                                    ULONG depotIndex,
                                    ULONG blockIndex)
{
  ULONG depotBlocksPerExtBlock = (This->bigBlockSize / sizeof(ULONG)) - 1;
  ULONG numExtBlocks           = depotIndex - COUNT_BBDEPOTINHEADER;
  ULONG extBlockCount          = numExtBlocks / depotBlocksPerExtBlock;
  ULONG extBlockOffset         = numExtBlocks % depotBlocksPerExtBlock;
  ULONG extBlockIndex          = This->extBigBlockDepotStart;

  assert(depotIndex >= COUNT_BBDEPOTINHEADER);

  while (extBlockCount > 0)
  {
    extBlockIndex = Storage32Impl_GetNextExtendedBlock(This, extBlockIndex);
    extBlockCount--;
  }

  if (extBlockIndex != BLOCK_UNUSED)
  {
    BYTE* depotBuffer;

    depotBuffer = StorageImpl_GetBigBlock(This, extBlockIndex);

    if (depotBuffer != 0)
    {
      StorageUtl_WriteDWord(depotBuffer,
                            extBlockOffset * sizeof(ULONG),
                            blockIndex);

      StorageImpl_ReleaseBigBlock(This, depotBuffer);
    }
  }
}

/******************************************************************************
 *      Storage32Impl_AddExtBlockDepot
 *
 * Creates an extended depot block.
 */
ULONG Storage32Impl_AddExtBlockDepot(StorageImpl* This)
{
  ULONG numExtBlocks           = This->extBigBlockDepotCount;
  ULONG nextExtBlock           = This->extBigBlockDepotStart;
  BYTE* depotBuffer            = NULL;
  ULONG index                  = BLOCK_UNUSED;
  ULONG nextBlockOffset        = This->bigBlockSize - sizeof(ULONG);
  ULONG blocksPerDepotBlock    = This->bigBlockSize / sizeof(ULONG);
  ULONG depotBlocksPerExtBlock = blocksPerDepotBlock - 1;

  index = (COUNT_BBDEPOTINHEADER + (numExtBlocks * depotBlocksPerExtBlock)) *
          blocksPerDepotBlock;

  if ((numExtBlocks == 0) && (nextExtBlock == BLOCK_END_OF_CHAIN))
  {
    /*
     * The first extended block.
     */
    This->extBigBlockDepotStart = index;
  }
  else
  {
    int i;
    /*
     * Follow the chain to the last one.
     */
    for (i = 0; i < (numExtBlocks - 1); i++)
    {
      nextExtBlock = Storage32Impl_GetNextExtendedBlock(This, nextExtBlock);
    }

    /*
     * Add the new extended block to the chain.
     */
    depotBuffer = StorageImpl_GetBigBlock(This, nextExtBlock);
    StorageUtl_WriteDWord(depotBuffer, nextBlockOffset, index);
    StorageImpl_ReleaseBigBlock(This, depotBuffer);
  }

  /*
   * Initialize this block.
   */
  depotBuffer = StorageImpl_GetBigBlock(This, index);
  memset(depotBuffer, BLOCK_UNUSED, This->bigBlockSize);
  StorageImpl_ReleaseBigBlock(This, depotBuffer);

  return index;
}

/******************************************************************************
 *      Storage32Impl_FreeBigBlock
 *
 * This method will flag the specified block as free in the big block depot.
 */
void  StorageImpl_FreeBigBlock(
  StorageImpl* This,
  ULONG          blockIndex)
{
  StorageImpl_SetNextBlockInChain(This, blockIndex, BLOCK_UNUSED);

  if (blockIndex < This->prevFreeBlock)
    This->prevFreeBlock = blockIndex;
}

/************************************************************************
 * Storage32Impl_GetNextBlockInChain
 *
 * This method will retrieve the block index of the next big block in
 * in the chain.
 *
 * Params:  This       - Pointer to the Storage object.
 *          blockIndex - Index of the block to retrieve the chain
 *                       for.
 *
 * Returns: This method returns the index of the next block in the chain.
 *          It will return the constants:
 *              BLOCK_SPECIAL - If the block given was not part of a
 *                              chain.
 *              BLOCK_END_OF_CHAIN - If the block given was the last in
 *                                   a chain.
 *              BLOCK_UNUSED - If the block given was not past of a chain
 *                             and is available.
 *              BLOCK_EXTBBDEPOT - This block is part of the extended
 *                                 big block depot.
 *
 * See Windows documentation for more details on IStorage methods.
 */
ULONG StorageImpl_GetNextBlockInChain(
  StorageImpl* This,
  ULONG          blockIndex)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  ULONG nextBlockIndex   = BLOCK_SPECIAL;
  void* depotBuffer;
  ULONG depotBlockIndexPos;

  assert(depotBlockCount < This->bigBlockDepotCount);

  /*
   * Cache the currently accessed depot block.
   */
  if (depotBlockCount != This->indexBlockDepotCached)
  {
    This->indexBlockDepotCached = depotBlockCount;

    if (depotBlockCount < COUNT_BBDEPOTINHEADER)
    {
      depotBlockIndexPos = This->bigBlockDepotStart[depotBlockCount];
    }
    else
    {
      /*
       * We have to look in the extended depot.
       */
      depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotBlockCount);
    }

    depotBuffer = StorageImpl_GetROBigBlock(This, depotBlockIndexPos);

    if (depotBuffer!=0)
    {
      int index;

      for (index = 0; index < NUM_BLOCKS_PER_DEPOT_BLOCK; index++)
      {
        StorageUtl_ReadDWord(depotBuffer, index*sizeof(ULONG), &nextBlockIndex);
        This->blockDepotCached[index] = nextBlockIndex;
      }

      StorageImpl_ReleaseBigBlock(This, depotBuffer);
    }
  }

  nextBlockIndex = This->blockDepotCached[depotBlockOffset/sizeof(ULONG)];

  return nextBlockIndex;
}

/******************************************************************************
 *      Storage32Impl_GetNextExtendedBlock
 *
 * Given an extended block this method will return the next extended block.
 *
 * NOTES:
 * The last ULONG of an extended block is the block index of the next
 * extended block. Extended blocks are marked as BLOCK_EXTBBDEPOT in the
 * depot.
 *
 * Return values:
 *    - The index of the next extended block
 *    - BLOCK_UNUSED: there is no next extended block.
 *    - Any other return values denotes failure.
 */
ULONG Storage32Impl_GetNextExtendedBlock(StorageImpl* This, ULONG blockIndex)
{
  ULONG nextBlockIndex   = BLOCK_SPECIAL;
  ULONG depotBlockOffset = This->bigBlockSize - sizeof(ULONG);
  void* depotBuffer;

  depotBuffer = StorageImpl_GetROBigBlock(This, blockIndex);

  if (depotBuffer!=0)
  {
    StorageUtl_ReadDWord(depotBuffer, depotBlockOffset, &nextBlockIndex);

    StorageImpl_ReleaseBigBlock(This, depotBuffer);
  }

  return nextBlockIndex;
}

/******************************************************************************
 *      Storage32Impl_SetNextBlockInChain
 *
 * This method will write the index of the specified block's next block
 * in the big block depot.
 *
 * For example: to create the chain 3 -> 1 -> 7 -> End of Chain
 *              do the following
 *
 * Storage32Impl_SetNextBlockInChain(This, 3, 1);
 * Storage32Impl_SetNextBlockInChain(This, 1, 7);
 * Storage32Impl_SetNextBlockInChain(This, 7, BLOCK_END_OF_CHAIN);
 *
 */
void  StorageImpl_SetNextBlockInChain(
          StorageImpl* This,
          ULONG          blockIndex,
          ULONG          nextBlock)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  ULONG depotBlockIndexPos;
  void* depotBuffer;

  assert(depotBlockCount < This->bigBlockDepotCount);
  assert(blockIndex != nextBlock);

  if (depotBlockCount < COUNT_BBDEPOTINHEADER)
  {
    depotBlockIndexPos = This->bigBlockDepotStart[depotBlockCount];
  }
  else
  {
    /*
     * We have to look in the extended depot.
     */
    depotBlockIndexPos = Storage32Impl_GetExtDepotBlock(This, depotBlockCount);
  }

  depotBuffer = StorageImpl_GetBigBlock(This, depotBlockIndexPos);

  if (depotBuffer!=0)
  {
    StorageUtl_WriteDWord(depotBuffer, depotBlockOffset, nextBlock);
    StorageImpl_ReleaseBigBlock(This, depotBuffer);
  }

  /*
   * Update the cached block depot, if necessary.
   */
  if (depotBlockCount == This->indexBlockDepotCached)
  {
    This->blockDepotCached[depotBlockOffset/sizeof(ULONG)] = nextBlock;
  }
}

/******************************************************************************
 *      Storage32Impl_LoadFileHeader
 *
 * This method will read in the file header, i.e. big block index -1.
 */
HRESULT StorageImpl_LoadFileHeader(
          StorageImpl* This)
{
  HRESULT hr = STG_E_FILENOTFOUND;
  void*   headerBigBlock = NULL;
  int     index;

  /*
   * Get a pointer to the big block of data containing the header.
   */
  headerBigBlock = StorageImpl_GetROBigBlock(This, -1);

  /*
   * Extract the information from the header.
   */
  if (headerBigBlock!=0)
  {
    /*
     * Check for the "magic number" signature and return an error if it is not
     * found.
     */
    if (memcmp(headerBigBlock, STORAGE_oldmagic, sizeof(STORAGE_oldmagic))==0)
    {
      StorageImpl_ReleaseBigBlock(This, headerBigBlock);
      return STG_E_OLDFORMAT;
    }

    if (memcmp(headerBigBlock, STORAGE_magic, sizeof(STORAGE_magic))!=0)
    {
      StorageImpl_ReleaseBigBlock(This, headerBigBlock);
      return STG_E_INVALIDHEADER;
    }

    StorageUtl_ReadWord(
      headerBigBlock, 
      OFFSET_BIGBLOCKSIZEBITS,   
      &This->bigBlockSizeBits);

    StorageUtl_ReadWord(
      headerBigBlock, 
      OFFSET_SMALLBLOCKSIZEBITS, 
      &This->smallBlockSizeBits);

    StorageUtl_ReadDWord(
      headerBigBlock, 
      OFFSET_BBDEPOTCOUNT,      
      &This->bigBlockDepotCount);

    StorageUtl_ReadDWord(
      headerBigBlock, 
      OFFSET_ROOTSTARTBLOCK,    
      &This->rootStartBlock);

    StorageUtl_ReadDWord(
      headerBigBlock, 
      OFFSET_SBDEPOTSTART,      
      &This->smallBlockDepotStart);

    StorageUtl_ReadDWord( 
      headerBigBlock, 
      OFFSET_EXTBBDEPOTSTART,   
      &This->extBigBlockDepotStart);

    StorageUtl_ReadDWord(
      headerBigBlock, 
      OFFSET_EXTBBDEPOTCOUNT,   
      &This->extBigBlockDepotCount);
    
    for (index = 0; index < COUNT_BBDEPOTINHEADER; index ++)
    {
      StorageUtl_ReadDWord(
        headerBigBlock, 
        OFFSET_BBDEPOTSTART + (sizeof(ULONG)*index),
        &(This->bigBlockDepotStart[index]));
    }
    
    /*
     * Make the bitwise arithmetic to get the size of the blocks in bytes.
     */
    if ((1 << 2) == 4)
    {
      This->bigBlockSize   = 0x000000001 << (DWORD)This->bigBlockSizeBits;
      This->smallBlockSize = 0x000000001 << (DWORD)This->smallBlockSizeBits;
    }
    else
    {
      This->bigBlockSize   = 0x000000001 >> (DWORD)This->bigBlockSizeBits;
      This->smallBlockSize = 0x000000001 >> (DWORD)This->smallBlockSizeBits;
    }
    
    /*
     * Right now, the code is making some assumptions about the size of the 
     * blocks, just make sure they are what we're expecting.
     */
    assert( (This->bigBlockSize==DEF_BIG_BLOCK_SIZE) && 
            (This->smallBlockSize==DEF_SMALL_BLOCK_SIZE));
    
    /*
     * Release the block.
     */
    StorageImpl_ReleaseBigBlock(This, headerBigBlock);

    hr = S_OK;
  }
  
  return hr;
}

/******************************************************************************
 *      Storage32Impl_SaveFileHeader
 *
 * This method will save to the file the header, i.e. big block -1.
 */
void StorageImpl_SaveFileHeader(
          StorageImpl* This)
{
  BYTE   headerBigBlock[BIG_BLOCK_SIZE];
  int    index;
  BOOL success;

  /*
   * Get a pointer to the big block of data containing the header.
   */
  success = StorageImpl_ReadBigBlock(This, -1, headerBigBlock);
  
  /*
   * If the block read failed, the file is probably new.
   */
  if (!success)
  {
    /*
     * Initialize for all unknown fields.
     */
    memset(headerBigBlock, 0, BIG_BLOCK_SIZE);
    
    /*
     * Initialize the magic number.
     */
    memcpy(headerBigBlock, STORAGE_magic, sizeof(STORAGE_magic));
    
    /*
     * And a bunch of things we don't know what they mean
     */
    StorageUtl_WriteWord(headerBigBlock,  0x18, 0x3b);
    StorageUtl_WriteWord(headerBigBlock,  0x1a, 0x3);
    StorageUtl_WriteWord(headerBigBlock,  0x1c, (WORD)-2);
    StorageUtl_WriteDWord(headerBigBlock, 0x38, (DWORD)0x1000);
    StorageUtl_WriteDWord(headerBigBlock, 0x40, (DWORD)0x0001);
  }
  
  /*
   * Write the information to the header.
   */
  if (headerBigBlock!=0)
  {
    StorageUtl_WriteWord(
      headerBigBlock, 
      OFFSET_BIGBLOCKSIZEBITS,   
      This->bigBlockSizeBits);

    StorageUtl_WriteWord(
      headerBigBlock, 
      OFFSET_SMALLBLOCKSIZEBITS, 
      This->smallBlockSizeBits);

    StorageUtl_WriteDWord(
      headerBigBlock, 
      OFFSET_BBDEPOTCOUNT,      
      This->bigBlockDepotCount);

    StorageUtl_WriteDWord(
      headerBigBlock, 
      OFFSET_ROOTSTARTBLOCK,    
      This->rootStartBlock);

    StorageUtl_WriteDWord(
      headerBigBlock, 
      OFFSET_SBDEPOTSTART,      
      This->smallBlockDepotStart);

    StorageUtl_WriteDWord(
      headerBigBlock, 
      OFFSET_EXTBBDEPOTSTART,   
      This->extBigBlockDepotStart);

    StorageUtl_WriteDWord(
      headerBigBlock, 
      OFFSET_EXTBBDEPOTCOUNT,   
      This->extBigBlockDepotCount);

    for (index = 0; index < COUNT_BBDEPOTINHEADER; index ++)
    {
      StorageUtl_WriteDWord(
        headerBigBlock, 
        OFFSET_BBDEPOTSTART + (sizeof(ULONG)*index),
        (This->bigBlockDepotStart[index]));
    }
  }
  
  /*
   * Write the big block back to the file.
   */
  StorageImpl_WriteBigBlock(This, -1, headerBigBlock);
}

/******************************************************************************
 *      Storage32Impl_ReadProperty
 *
 * This method will read the specified property from the property chain.
 */
BOOL StorageImpl_ReadProperty(
  StorageImpl* This,
  ULONG          index,
  StgProperty*   buffer)
{
  BYTE           currentProperty[PROPSET_BLOCK_SIZE];
  ULARGE_INTEGER offsetInPropSet;
  BOOL         readSuccessful;
  ULONG          bytesRead;

  offsetInPropSet.u.HighPart = 0;
  offsetInPropSet.u.LowPart  = index * PROPSET_BLOCK_SIZE;
  
  readSuccessful = BlockChainStream_ReadAt(
                    This->rootBlockChain,
                    offsetInPropSet,
                    PROPSET_BLOCK_SIZE,
                    currentProperty,
                    &bytesRead);
  
  if (readSuccessful)
  {
    memset(buffer->name, 0, sizeof(buffer->name));
    memcpy(
      buffer->name, 
      currentProperty+OFFSET_PS_NAME, 
      PROPERTY_NAME_BUFFER_LEN );

    memcpy(&buffer->propertyType, currentProperty + OFFSET_PS_PROPERTYTYPE, 1);
    
    StorageUtl_ReadWord(
      currentProperty,  
      OFFSET_PS_NAMELENGTH,  
      &buffer->sizeOfNameString);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_PREVIOUSPROP, 
      &buffer->previousProperty);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_NEXTPROP,     
      &buffer->nextProperty);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_DIRPROP,      
      &buffer->dirProperty);

    StorageUtl_ReadGUID(
      currentProperty,  
      OFFSET_PS_GUID,        
      &buffer->propertyUniqueID);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_TSS1,         
      &buffer->timeStampS1);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_TSD1,         
      &buffer->timeStampD1);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_TSS2,         
      &buffer->timeStampS2);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_TSD2,         
      &buffer->timeStampD2);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_STARTBLOCK,   
      &buffer->startingBlock);

    StorageUtl_ReadDWord(
      currentProperty, 
      OFFSET_PS_SIZE,         
      &buffer->size.u.LowPart);

    buffer->size.u.HighPart = 0;
  }

  return readSuccessful;
}

/*********************************************************************
 * Write the specified property into the property chain
 */
BOOL StorageImpl_WriteProperty(
  StorageImpl* This,
  ULONG          index,
  StgProperty*   buffer)
{
  BYTE           currentProperty[PROPSET_BLOCK_SIZE];
  ULARGE_INTEGER offsetInPropSet;
  BOOL         writeSuccessful;
  ULONG          bytesWritten;

  offsetInPropSet.u.HighPart = 0;
  offsetInPropSet.u.LowPart  = index * PROPSET_BLOCK_SIZE;

  memset(currentProperty, 0, PROPSET_BLOCK_SIZE);

  memcpy(
    currentProperty + OFFSET_PS_NAME, 
    buffer->name, 
    PROPERTY_NAME_BUFFER_LEN );

  memcpy(currentProperty + OFFSET_PS_PROPERTYTYPE, &buffer->propertyType, 1);

  StorageUtl_WriteWord(
    currentProperty,  
      OFFSET_PS_NAMELENGTH,   
      buffer->sizeOfNameString);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_PREVIOUSPROP, 
      buffer->previousProperty);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_NEXTPROP,     
      buffer->nextProperty);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_DIRPROP,      
      buffer->dirProperty);

  StorageUtl_WriteGUID(
    currentProperty,  
      OFFSET_PS_GUID,        
      &buffer->propertyUniqueID);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_TSS1,         
      buffer->timeStampS1);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_TSD1,         
      buffer->timeStampD1);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_TSS2,         
      buffer->timeStampS2);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_TSD2,         
      buffer->timeStampD2);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_STARTBLOCK,   
      buffer->startingBlock);

  StorageUtl_WriteDWord(
    currentProperty, 
      OFFSET_PS_SIZE,         
      buffer->size.u.LowPart);

  writeSuccessful = BlockChainStream_WriteAt(This->rootBlockChain,
                                            offsetInPropSet,
                                            PROPSET_BLOCK_SIZE,
                                            currentProperty,
                                            &bytesWritten);
  return writeSuccessful;
}

BOOL StorageImpl_ReadBigBlock(
  StorageImpl* This,
  ULONG          blockIndex,
  void*          buffer)
{
  void* bigBlockBuffer;

  bigBlockBuffer = StorageImpl_GetROBigBlock(This, blockIndex);

  if (bigBlockBuffer!=0)
  {
    memcpy(buffer, bigBlockBuffer, This->bigBlockSize);

    StorageImpl_ReleaseBigBlock(This, bigBlockBuffer);

    return TRUE;
  }

  return FALSE;
}

BOOL StorageImpl_WriteBigBlock(
  StorageImpl* This,
  ULONG          blockIndex,
  void*          buffer)
{
  void* bigBlockBuffer;

  bigBlockBuffer = StorageImpl_GetBigBlock(This, blockIndex);

  if (bigBlockBuffer!=0)
  {
    memcpy(bigBlockBuffer, buffer, This->bigBlockSize);

    StorageImpl_ReleaseBigBlock(This, bigBlockBuffer);
    
    return TRUE;
  }
  
  return FALSE;
}

void* StorageImpl_GetROBigBlock(
  StorageImpl* This,
  ULONG          blockIndex)
{
  return BIGBLOCKFILE_GetROBigBlock(This->bigBlockFile, blockIndex);
}

void* StorageImpl_GetBigBlock(
  StorageImpl* This,
  ULONG          blockIndex)
{
  return BIGBLOCKFILE_GetBigBlock(This->bigBlockFile, blockIndex);
}

void StorageImpl_ReleaseBigBlock(
  StorageImpl* This,
  void*          pBigBlock)
{
  BIGBLOCKFILE_ReleaseBigBlock(This->bigBlockFile, pBigBlock);
}

/******************************************************************************
 *              Storage32Impl_SmallBlocksToBigBlocks
 *
 * This method will convert a small block chain to a big block chain.
 * The small block chain will be destroyed.
 */
BlockChainStream* Storage32Impl_SmallBlocksToBigBlocks(
                      StorageImpl* This,
                      SmallBlockChainStream** ppsbChain)
{
  ULONG bbHeadOfChain = BLOCK_END_OF_CHAIN;
  ULARGE_INTEGER size, offset;
  ULONG cbRead, cbWritten, cbTotalRead, cbTotalWritten;
  ULONG propertyIndex;
  BOOL successRead, successWrite;
  StgProperty chainProperty;
  BYTE *buffer;
  BlockChainStream *bbTempChain = NULL;
  BlockChainStream *bigBlockChain = NULL;

  /*
   * Create a temporary big block chain that doesn't have
   * an associated property. This temporary chain will be
   * used to copy data from small blocks to big blocks.
   */
  bbTempChain = BlockChainStream_Construct(This,
                                           &bbHeadOfChain,
                                           PROPERTY_NULL);

  /*
   * Grow the big block chain.
   */
  size = SmallBlockChainStream_GetSize(*ppsbChain);
  BlockChainStream_SetSize(bbTempChain, size);

  /*
   * Copy the contents of the small block chain to the big block chain
   * by small block size increments.
   */
  offset.u.LowPart = 0;
  offset.u.HighPart = 0;
  cbTotalRead = 0;
  cbTotalWritten = 0;

  buffer = (BYTE *) HeapAlloc(GetProcessHeap(),0,DEF_SMALL_BLOCK_SIZE);
  do
  {
    successRead = SmallBlockChainStream_ReadAt(*ppsbChain,
                                               offset,
                                               DEF_SMALL_BLOCK_SIZE,
                                               buffer,
                                               &cbRead);
    cbTotalRead += cbRead;

    successWrite = BlockChainStream_WriteAt(bbTempChain,
                                            offset,
                                            cbRead,
                                            buffer,
                                            &cbWritten);
    cbTotalWritten += cbWritten;

    offset.u.LowPart += This->smallBlockSize;

  } while (successRead && successWrite);
  HeapFree(GetProcessHeap(),0,buffer);

  assert(cbTotalRead == cbTotalWritten);

  /*
   * Destroy the small block chain.
   */
  propertyIndex = (*ppsbChain)->ownerPropertyIndex;
  size.u.HighPart = 0;
  size.u.LowPart  = 0;
  SmallBlockChainStream_SetSize(*ppsbChain, size);
  SmallBlockChainStream_Destroy(*ppsbChain);
  *ppsbChain = 0;

  /*
   * Change the property information. This chain is now a big block chain
   * and it doesn't reside in the small blocks chain anymore.
   */
  StorageImpl_ReadProperty(This, propertyIndex, &chainProperty);

  chainProperty.startingBlock = bbHeadOfChain;

  StorageImpl_WriteProperty(This, propertyIndex, &chainProperty);

  /*
   * Destroy the temporary propertyless big block chain.
   * Create a new big block chain associated with this property.
   */
  BlockChainStream_Destroy(bbTempChain);
  bigBlockChain = BlockChainStream_Construct(This,
                                             NULL,
                                             propertyIndex);

  return bigBlockChain;
}

/******************************************************************************
** Storage32InternalImpl implementation
*/

StorageInternalImpl* StorageInternalImpl_Construct(
  StorageImpl* ancestorStorage,  
  ULONG          rootPropertyIndex)
{
  StorageInternalImpl* newStorage;

  /*
   * Allocate space for the new storage object
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageInternalImpl));

  if (newStorage!=0)
  {
    memset(newStorage, 0, sizeof(StorageInternalImpl));

    /*
     * Initialize the virtual function table.
     */
    ICOM_VTBL(newStorage)    = &Storage32InternalImpl_Vtbl;
    newStorage->v_destructor = &StorageInternalImpl_Destroy;

    /*
     * Keep the ancestor storage pointer and nail a reference to it.
     */
    newStorage->ancestorStorage = ancestorStorage;
    StorageBaseImpl_AddRef((IStorage*)(newStorage->ancestorStorage));

    /*
     * Keep the index of the root property set for this storage,
     */
    newStorage->rootPropertySetIndex = rootPropertyIndex;

    return newStorage;
  }

  return 0;
}

void StorageInternalImpl_Destroy(
  StorageInternalImpl* This)
{
  StorageBaseImpl_Release((IStorage*)This->ancestorStorage);
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
**
** Storage32InternalImpl_Commit
**
** The non-root storages cannot be opened in transacted mode thus this function
** does nothing.
*/
HRESULT WINAPI StorageInternalImpl_Commit( 
  IStorage*            iface,
  DWORD                  grfCommitFlags)  /* [in] */ 
{
  return S_OK;
}
        
/******************************************************************************
**
** Storage32InternalImpl_Revert
**
** The non-root storages cannot be opened in transacted mode thus this function
** does nothing.
*/
HRESULT WINAPI StorageInternalImpl_Revert( 
  IStorage*            iface)
{
  return S_OK;
}

/******************************************************************************
** IEnumSTATSTGImpl implementation
*/

IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(
  StorageImpl* parentStorage,
  ULONG          firstPropertyNode)
{
  IEnumSTATSTGImpl* newEnumeration;

  newEnumeration = HeapAlloc(GetProcessHeap(), 0, sizeof(IEnumSTATSTGImpl));
  
  if (newEnumeration!=0)
  {
    /*
     * Set-up the virtual function table and reference count.
     */
    ICOM_VTBL(newEnumeration) = &IEnumSTATSTGImpl_Vtbl;
    newEnumeration->ref       = 0;
    
    /*
     * We want to nail-down the reference to the storage in case the
     * enumeration out-lives the storage in the client application.
     */
    newEnumeration->parentStorage = parentStorage;
    IStorage_AddRef((IStorage*)newEnumeration->parentStorage);
    
    newEnumeration->firstPropertyNode   = firstPropertyNode;
    
    /*
     * Initialize the search stack
     */
    newEnumeration->stackSize    = 0;
    newEnumeration->stackMaxSize = ENUMSTATSGT_SIZE_INCREMENT;
    newEnumeration->stackToVisit = 
      HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG)*ENUMSTATSGT_SIZE_INCREMENT);
    
    /*
     * Make sure the current node of the iterator is the first one.
     */
    IEnumSTATSTGImpl_Reset((IEnumSTATSTG*)newEnumeration);
  }
  
  return newEnumeration;
}

void IEnumSTATSTGImpl_Destroy(IEnumSTATSTGImpl* This)
{
  IStorage_Release((IStorage*)This->parentStorage);
  HeapFree(GetProcessHeap(), 0, This->stackToVisit);
  HeapFree(GetProcessHeap(), 0, This);
}

HRESULT WINAPI IEnumSTATSTGImpl_QueryInterface(
  IEnumSTATSTG*     iface,
  REFIID            riid,
  void**            ppvObject)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  /*
   * Perform a sanity check on the parameters.
   */
  if (ppvObject==0)
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) 
  {
    *ppvObject = (IEnumSTATSTG*)This;
  }
  else if (memcmp(&IID_IStorage, riid, sizeof(IID_IEnumSTATSTG)) == 0) 
  {
    *ppvObject = (IEnumSTATSTG*)This;
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
    return E_NOINTERFACE;

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  IEnumSTATSTGImpl_AddRef((IEnumSTATSTG*)This);

  return S_OK;
}
        
ULONG   WINAPI IEnumSTATSTGImpl_AddRef(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  This->ref++;
  return This->ref;
}
        
ULONG   WINAPI IEnumSTATSTGImpl_Release(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  ULONG newRef;

  This->ref--;
  newRef = This->ref;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (newRef==0)
  {
    IEnumSTATSTGImpl_Destroy(This);
  }

  return newRef;;
}

HRESULT WINAPI IEnumSTATSTGImpl_Next(
  IEnumSTATSTG* iface,
  ULONG             celt,
  STATSTG*          rgelt,
  ULONG*            pceltFetched)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  StgProperty currentProperty;
  STATSTG*    currentReturnStruct = rgelt;
  ULONG       objectFetched       = 0;
  ULONG      currentSearchNode;

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (rgelt==0) || ( (celt!=1) && (pceltFetched==0) ) )
    return E_INVALIDARG;  
  
  /*
   * To avoid the special case, get another pointer to a ULONG value if
   * the caller didn't supply one.
   */
  if (pceltFetched==0)
    pceltFetched = &objectFetched;
  
  /*
   * Start the iteration, we will iterate until we hit the end of the
   * linked list or until we hit the number of items to iterate through
   */
  *pceltFetched = 0;

  /*
   * Start with the node at the top of the stack.
   */
  currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);

  while ( ( *pceltFetched < celt) && 
          ( currentSearchNode!=PROPERTY_NULL) )
  {
    /* 
     * Remove the top node from the stack
     */
    IEnumSTATSTGImpl_PopSearchNode(This, TRUE);

    /*
     * Read the property from the storage.
     */
    StorageImpl_ReadProperty(This->parentStorage,
      currentSearchNode, 
      &currentProperty);

    /*
     * Copy the information to the return buffer.
     */
    StorageUtl_CopyPropertyToSTATSTG(currentReturnStruct,
      &currentProperty,
      STATFLAG_DEFAULT);
        
    /*
     * Step to the next item in the iteration
     */
    (*pceltFetched)++;
    currentReturnStruct++;

    /*
     * Push the next search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, currentProperty.nextProperty);

    /*
     * continue the iteration.
     */
    currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);
  }

  if (*pceltFetched == celt)
    return S_OK;

  return S_FALSE;
}

        
HRESULT WINAPI IEnumSTATSTGImpl_Skip(
  IEnumSTATSTG* iface,
  ULONG             celt)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  StgProperty currentProperty;
  ULONG       objectFetched       = 0;
  ULONG       currentSearchNode;

  /*
   * Start with the node at the top of the stack.
   */
  currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);

  while ( (objectFetched < celt) && 
          (currentSearchNode!=PROPERTY_NULL) )
  {
    /* 
     * Remove the top node from the stack
     */
    IEnumSTATSTGImpl_PopSearchNode(This, TRUE);

    /*
     * Read the property from the storage.
     */
    StorageImpl_ReadProperty(This->parentStorage,
      currentSearchNode, 
      &currentProperty);
    
    /*
     * Step to the next item in the iteration
     */
    objectFetched++;

    /*
     * Push the next search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, currentProperty.nextProperty);

    /*
     * continue the iteration.
     */
    currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);
  }

  if (objectFetched == celt)
    return S_OK;

  return S_FALSE;
}
        
HRESULT WINAPI IEnumSTATSTGImpl_Reset(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  StgProperty rootProperty;
  BOOL      readSuccessful;

  /*
   * Re-initialize the search stack to an empty stack
   */
  This->stackSize = 0;

  /*
   * Read the root property from the storage.
   */
  readSuccessful = StorageImpl_ReadProperty(
                    This->parentStorage,
                    This->firstPropertyNode, 
                    &rootProperty);

  if (readSuccessful)
  {
    assert(rootProperty.sizeOfNameString!=0);

    /*
     * Push the search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, rootProperty.dirProperty);
  }

  return S_OK;
}
        
HRESULT WINAPI IEnumSTATSTGImpl_Clone(
  IEnumSTATSTG* iface,
  IEnumSTATSTG**    ppenum)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  IEnumSTATSTGImpl* newClone;

  /*
   * Perform a sanity check on the parameters.
   */
  if (ppenum==0)
    return E_INVALIDARG;
  
  newClone = IEnumSTATSTGImpl_Construct(This->parentStorage,
               This->firstPropertyNode);

  
  /*
   * The new clone enumeration must point to the same current node as
   * the ole one.
   */
  newClone->stackSize    = This->stackSize    ;
  newClone->stackMaxSize = This->stackMaxSize ;
  newClone->stackToVisit = 
    HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG) * newClone->stackMaxSize);

  memcpy(
    newClone->stackToVisit, 
    This->stackToVisit, 
    sizeof(ULONG) * newClone->stackSize);

  *ppenum = (IEnumSTATSTG*)newClone;

  /*
   * Don't forget to nail down a reference to the clone before
   * returning it.
   */
  IEnumSTATSTGImpl_AddRef(*ppenum);

  return S_OK;
}

INT IEnumSTATSTGImpl_FindParentProperty(
  IEnumSTATSTGImpl *This,
  ULONG             childProperty, 
  StgProperty      *currentProperty,
  ULONG            *thisNodeId)
{
  ULONG currentSearchNode;
  ULONG foundNode;

  /*
   * To avoid the special case, get another pointer to a ULONG value if
   * the caller didn't supply one.
   */
  if (thisNodeId==0)
    thisNodeId = &foundNode;

  /*
   * Start with the node at the top of the stack.
   */
  currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);
  

  while (currentSearchNode!=PROPERTY_NULL)
  {
    /*
     * Store the current node in the returned parameters
     */
    *thisNodeId = currentSearchNode;

    /* 
     * Remove the top node from the stack
     */
    IEnumSTATSTGImpl_PopSearchNode(This, TRUE);

    /*
     * Read the property from the storage.
     */
    StorageImpl_ReadProperty(
      This->parentStorage,
      currentSearchNode, 
      currentProperty);
      
    if (currentProperty->previousProperty == childProperty)
      return PROPERTY_RELATION_PREVIOUS;

    else if (currentProperty->nextProperty == childProperty)  
      return PROPERTY_RELATION_NEXT;
  
    else if (currentProperty->dirProperty == childProperty)
      return PROPERTY_RELATION_DIR;
       
    /*
     * Push the next search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, currentProperty->nextProperty);

    /*
     * continue the iteration.
     */
    currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);
  }

  return PROPERTY_NULL;
}

ULONG IEnumSTATSTGImpl_FindProperty(
  IEnumSTATSTGImpl* This,
  const OLECHAR*  lpszPropName,
  StgProperty*      currentProperty)
{
  ULONG currentSearchNode;

  /*
   * Start with the node at the top of the stack.
   */
  currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);

  while (currentSearchNode!=PROPERTY_NULL)
  {
    /* 
     * Remove the top node from the stack
     */
    IEnumSTATSTGImpl_PopSearchNode(This, TRUE);

    /*
     * Read the property from the storage.
     */
    StorageImpl_ReadProperty(This->parentStorage,
      currentSearchNode, 
      currentProperty);

    if ( propertyNameCmp(
          (OLECHAR*)currentProperty->name, 
          (OLECHAR*)lpszPropName) == 0)
      return currentSearchNode;

    /*
     * Push the next search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, currentProperty->nextProperty);

    /*
     * continue the iteration.
     */
    currentSearchNode = IEnumSTATSTGImpl_PopSearchNode(This, FALSE);
  }

  return PROPERTY_NULL;
}

void IEnumSTATSTGImpl_PushSearchNode(
  IEnumSTATSTGImpl* This,
  ULONG             nodeToPush)
{
  StgProperty rootProperty;
  BOOL      readSuccessful;

  /*
   * First, make sure we're not trying to push an unexisting node.
   */
  if (nodeToPush==PROPERTY_NULL)
    return;

  /*
   * First push the node to the stack
   */
  if (This->stackSize == This->stackMaxSize)
  {
    This->stackMaxSize += ENUMSTATSGT_SIZE_INCREMENT;

    This->stackToVisit = HeapReAlloc(
                           GetProcessHeap(), 
                           0,
                           This->stackToVisit,
                           sizeof(ULONG) * This->stackMaxSize);
  }

  This->stackToVisit[This->stackSize] = nodeToPush;
  This->stackSize++;

  /*
   * Read the root property from the storage.
   */
  readSuccessful = StorageImpl_ReadProperty(
                    This->parentStorage,
                    nodeToPush, 
                    &rootProperty);

  if (readSuccessful)
  {
    assert(rootProperty.sizeOfNameString!=0);

    /*
     * Push the previous search node in the search stack.
     */
    IEnumSTATSTGImpl_PushSearchNode(This, rootProperty.previousProperty);
  }
}

ULONG IEnumSTATSTGImpl_PopSearchNode(
  IEnumSTATSTGImpl* This,
  BOOL            remove)
{
  ULONG topNode;

  if (This->stackSize == 0)
    return PROPERTY_NULL;

  topNode = This->stackToVisit[This->stackSize-1];

  if (remove)
    This->stackSize--;

  return topNode;
}

/******************************************************************************
** StorageUtl implementation
*/

void StorageUtl_ReadWord(void* buffer, ULONG offset, WORD* value)
{
  memcpy(value, (BYTE*)buffer+offset, sizeof(WORD));
}

void StorageUtl_WriteWord(void* buffer, ULONG offset, WORD value)
{
  memcpy((BYTE*)buffer+offset, &value, sizeof(WORD));
}

void StorageUtl_ReadDWord(void* buffer, ULONG offset, DWORD* value)
{
  memcpy(value, (BYTE*)buffer+offset, sizeof(DWORD));
}

void StorageUtl_WriteDWord(void* buffer, ULONG offset, DWORD value)
{
  memcpy((BYTE*)buffer+offset, &value, sizeof(DWORD));
}

void StorageUtl_ReadGUID(void* buffer, ULONG offset, GUID* value)
{
  StorageUtl_ReadDWord(buffer, offset,   &(value->Data1));
  StorageUtl_ReadWord(buffer,  offset+4, &(value->Data2));
  StorageUtl_ReadWord(buffer,  offset+6, &(value->Data3));

  memcpy(value->Data4, (BYTE*)buffer+offset+8, sizeof(value->Data4));
}

void StorageUtl_WriteGUID(void* buffer, ULONG offset, GUID* value)
{
  StorageUtl_WriteDWord(buffer, offset,   value->Data1);
  StorageUtl_WriteWord(buffer,  offset+4, value->Data2);
  StorageUtl_WriteWord(buffer,  offset+6, value->Data3);

  memcpy((BYTE*)buffer+offset+8, value->Data4, sizeof(value->Data4));
}

void StorageUtl_CopyPropertyToSTATSTG(
  STATSTG*     destination,
  StgProperty* source,
  int          statFlags)
{
  /*
   * The copy of the string occurs only when the flag is not set
   */
  if ((statFlags & STATFLAG_NONAME) != 0)
  {
    destination->pwcsName = 0;
  }
  else
  {
    destination->pwcsName = 
      CoTaskMemAlloc((lstrlenW(source->name)+1)*sizeof(WCHAR));

    lstrcpyW((LPWSTR)destination->pwcsName, source->name);
  }
  
  switch (source->propertyType)
  {
    case PROPTYPE_STORAGE:
    case PROPTYPE_ROOT:
      destination->type = STGTY_STORAGE;
      break;
    case PROPTYPE_STREAM:
      destination->type = STGTY_STREAM;
      break;
    default:
      destination->type = STGTY_STREAM;
      break;        
  }

  destination->cbSize            = source->size;
/*    
  currentReturnStruct->mtime     = {0}; TODO
  currentReturnStruct->ctime     = {0};
  currentReturnStruct->atime     = {0}; 
*/
  destination->grfMode           = 0;
  destination->grfLocksSupported = 0; 
  destination->clsid             = source->propertyUniqueID;
  destination->grfStateBits      = 0; 
  destination->reserved          = 0; 
}

/******************************************************************************
** BlockChainStream implementation
*/

BlockChainStream* BlockChainStream_Construct(
  StorageImpl* parentStorage,  
  ULONG*         headOfStreamPlaceHolder,
  ULONG          propertyIndex)
{
  BlockChainStream* newStream;
  ULONG blockIndex;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(BlockChainStream));

  newStream->parentStorage           = parentStorage;
  newStream->headOfStreamPlaceHolder = headOfStreamPlaceHolder;
  newStream->ownerPropertyIndex      = propertyIndex;
  newStream->lastBlockNoInSequence   = 0xFFFFFFFF;
  newStream->tailIndex               = BLOCK_END_OF_CHAIN;
  newStream->numBlocks               = 0;

  blockIndex = BlockChainStream_GetHeadOfChain(newStream);

  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    newStream->numBlocks++;
    newStream->tailIndex = blockIndex;

    blockIndex = StorageImpl_GetNextBlockInChain(
                   parentStorage,
                   blockIndex);
  }

  return newStream;
}

void BlockChainStream_Destroy(BlockChainStream* This)
{
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BlockChainStream_GetHeadOfChain
 *
 * Returns the head of this stream chain.
 * Some special chains don't have properties, their heads are kept in
 * This->headOfStreamPlaceHolder.
 *
 */
ULONG BlockChainStream_GetHeadOfChain(BlockChainStream* This)
{
  StgProperty chainProperty;
  BOOL      readSuccessful;

  if (This->headOfStreamPlaceHolder != 0)
    return *(This->headOfStreamPlaceHolder);

  if (This->ownerPropertyIndex != PROPERTY_NULL)
  {
    readSuccessful = StorageImpl_ReadProperty(
                      This->parentStorage,
                      This->ownerPropertyIndex,
                      &chainProperty);

    if (readSuccessful)
    {
      return chainProperty.startingBlock;
    }
  }

  return BLOCK_END_OF_CHAIN;
}

/******************************************************************************
 *       BlockChainStream_GetCount
 *
 * Returns the number of blocks that comprises this chain.
 * This is not the size of the stream as the last block may not be full!
 * 
 */
ULONG BlockChainStream_GetCount(BlockChainStream* This)
{
  ULONG blockIndex;
  ULONG count = 0;

  blockIndex = BlockChainStream_GetHeadOfChain(This);

  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    count++;

    blockIndex = StorageImpl_GetNextBlockInChain(
                   This->parentStorage, 
                   blockIndex);
  }

  return count;
}

/******************************************************************************
 *      BlockChainStream_ReadAt 
 *
 * Reads a specified number of bytes from this chain at the specified offset.
 * bytesRead may be NULL.
 * Failure will be returned if the specified number of bytes has not been read.
 */
BOOL BlockChainStream_ReadAt(BlockChainStream* This,
  ULARGE_INTEGER offset,
  ULONG          size,
  void*          buffer,
  ULONG*         bytesRead)
{
  ULONG blockNoInSequence = offset.u.LowPart / This->parentStorage->bigBlockSize;
  ULONG offsetInBlock     = offset.u.LowPart % This->parentStorage->bigBlockSize;
  ULONG bytesToReadInBuffer;
  ULONG blockIndex;
  BYTE* bufferWalker;
  BYTE* bigBlockBuffer;

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  if ( (This->lastBlockNoInSequence == 0xFFFFFFFF) ||
       (This->lastBlockNoInSequenceIndex == BLOCK_END_OF_CHAIN) ||
       (blockNoInSequence < This->lastBlockNoInSequence) )
  {
    blockIndex = BlockChainStream_GetHeadOfChain(This);
    This->lastBlockNoInSequence = blockNoInSequence;
  }
  else
  {
    ULONG temp = blockNoInSequence;

    blockIndex = This->lastBlockNoInSequenceIndex;
    blockNoInSequence -= This->lastBlockNoInSequence;
    This->lastBlockNoInSequence = temp;
  }

  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    blockIndex = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);
    
    blockNoInSequence--;
  }

  This->lastBlockNoInSequenceIndex = blockIndex;

  /*
   * Start reading the buffer.
   */
  *bytesRead   = 0;
  bufferWalker = buffer;
  
  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy from this big block.
     */
    bytesToReadInBuffer = 
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);
    
    /*
     * Copy those bytes to the buffer
     */
    bigBlockBuffer = 
      StorageImpl_GetROBigBlock(This->parentStorage, blockIndex);
    
    memcpy(bufferWalker, bigBlockBuffer + offsetInBlock, bytesToReadInBuffer);
    
    StorageImpl_ReleaseBigBlock(This->parentStorage, bigBlockBuffer);
    
    /*
     * Step to the next big block.
     */
    blockIndex    = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);

    bufferWalker += bytesToReadInBuffer;
    size         -= bytesToReadInBuffer;
    *bytesRead   += bytesToReadInBuffer;
    offsetInBlock = 0;  /* There is no offset on the next block */

  }
  
  return (size == 0);
}

/******************************************************************************
 *      BlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * bytesWritten may be NULL.
 * Will fail if not all specified number of bytes have been written.
 */
BOOL BlockChainStream_WriteAt(BlockChainStream* This,
  ULARGE_INTEGER    offset,
  ULONG             size,
  const void*       buffer,
  ULONG*            bytesWritten)
{
  ULONG blockNoInSequence = offset.u.LowPart / This->parentStorage->bigBlockSize;
  ULONG offsetInBlock     = offset.u.LowPart % This->parentStorage->bigBlockSize;
  ULONG bytesToWrite;
  ULONG blockIndex;
  BYTE* bufferWalker;
  BYTE* bigBlockBuffer;

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  if ( (This->lastBlockNoInSequence == 0xFFFFFFFF) ||
       (This->lastBlockNoInSequenceIndex == BLOCK_END_OF_CHAIN) ||
       (blockNoInSequence < This->lastBlockNoInSequence) )
  {
    blockIndex = BlockChainStream_GetHeadOfChain(This);
    This->lastBlockNoInSequence = blockNoInSequence;
  }
  else
  {
    ULONG temp = blockNoInSequence;

    blockIndex = This->lastBlockNoInSequenceIndex;
    blockNoInSequence -= This->lastBlockNoInSequence;
    This->lastBlockNoInSequence = temp;
  }

  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    blockIndex = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);
    
    blockNoInSequence--;
  }

  This->lastBlockNoInSequenceIndex = blockIndex;

  /*
   * Here, I'm casting away the constness on the buffer variable
   * This is OK since we don't intend to modify that buffer.
   */
  *bytesWritten   = 0;
  bufferWalker = (BYTE*)buffer;

  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy from this big block.
     */
    bytesToWrite = 
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);
    
    /*
     * Copy those bytes to the buffer
     */
    bigBlockBuffer = StorageImpl_GetBigBlock(This->parentStorage, blockIndex);
    
    memcpy(bigBlockBuffer + offsetInBlock, bufferWalker, bytesToWrite);
    
    StorageImpl_ReleaseBigBlock(This->parentStorage, bigBlockBuffer);
    
    /*
     * Step to the next big block.
     */
    blockIndex    = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);

    bufferWalker  += bytesToWrite;
    size          -= bytesToWrite;
    *bytesWritten += bytesToWrite;
    offsetInBlock  = 0;      /* There is no offset on the next block */
  }
  
  return (size == 0);
}

/******************************************************************************
 *      BlockChainStream_Shrink
 *
 * Shrinks this chain in the big block depot.
 */
BOOL BlockChainStream_Shrink(BlockChainStream* This,
                               ULARGE_INTEGER    newSize)
{
  ULONG blockIndex, extraBlock;
  ULONG numBlocks;
  ULONG count = 1;

  /*
   * Reset the last accessed block cache.
   */
  This->lastBlockNoInSequence = 0xFFFFFFFF;
  This->lastBlockNoInSequenceIndex = BLOCK_END_OF_CHAIN;

  /*
   * Figure out how many blocks are needed to contain the new size
   */
  numBlocks = newSize.u.LowPart / This->parentStorage->bigBlockSize;

  if ((newSize.u.LowPart % This->parentStorage->bigBlockSize) != 0)
    numBlocks++;

  blockIndex = BlockChainStream_GetHeadOfChain(This);

  /*
   * Go to the new end of chain
   */
  while (count < numBlocks)
  {
    blockIndex = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);

    count++;
  }

  /* Get the next block before marking the new end */
  extraBlock = 
    StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex);

  /* Mark the new end of chain */
  StorageImpl_SetNextBlockInChain(
    This->parentStorage, 
    blockIndex, 
    BLOCK_END_OF_CHAIN);

  This->tailIndex = blockIndex;
  This->numBlocks = numBlocks;

  /*
   * Mark the extra blocks as free
   */
  while (extraBlock != BLOCK_END_OF_CHAIN)
  {
    blockIndex = 
      StorageImpl_GetNextBlockInChain(This->parentStorage, extraBlock);

    StorageImpl_FreeBigBlock(This->parentStorage, extraBlock);
    extraBlock = blockIndex;
  }

  return TRUE;
}

/******************************************************************************
 *      BlockChainStream_Enlarge
 *
 * Grows this chain in the big block depot.
 */
BOOL BlockChainStream_Enlarge(BlockChainStream* This,
                                ULARGE_INTEGER    newSize)
{
  ULONG blockIndex, currentBlock;
  ULONG newNumBlocks;
  ULONG oldNumBlocks = 0;

  blockIndex = BlockChainStream_GetHeadOfChain(This);

  /*
   * Empty chain. Create the head.
   */
  if (blockIndex == BLOCK_END_OF_CHAIN)
  {
    blockIndex = StorageImpl_GetNextFreeBigBlock(This->parentStorage);
    StorageImpl_SetNextBlockInChain(This->parentStorage,
                                      blockIndex,
                                      BLOCK_END_OF_CHAIN);

    if (This->headOfStreamPlaceHolder != 0)
    {
      *(This->headOfStreamPlaceHolder) = blockIndex;
    }
    else
    {
      StgProperty chainProp;
      assert(This->ownerPropertyIndex != PROPERTY_NULL);

      StorageImpl_ReadProperty(
        This->parentStorage, 
        This->ownerPropertyIndex,
        &chainProp);

      chainProp.startingBlock = blockIndex; 

      StorageImpl_WriteProperty(
        This->parentStorage, 
        This->ownerPropertyIndex,
        &chainProp);
    }

    This->tailIndex = blockIndex;
    This->numBlocks = 1;
  }

  /*
   * Figure out how many blocks are needed to contain this stream
   */
  newNumBlocks = newSize.u.LowPart / This->parentStorage->bigBlockSize;

  if ((newSize.u.LowPart % This->parentStorage->bigBlockSize) != 0)
    newNumBlocks++;

  /*
   * Go to the current end of chain
   */
  if (This->tailIndex == BLOCK_END_OF_CHAIN)
  {
    currentBlock = blockIndex;

    while (blockIndex != BLOCK_END_OF_CHAIN)
    {
      This->numBlocks++;
      currentBlock = blockIndex;

      blockIndex =
        StorageImpl_GetNextBlockInChain(This->parentStorage, currentBlock);
    }

    This->tailIndex = currentBlock;
  }

  currentBlock = This->tailIndex;
  oldNumBlocks = This->numBlocks;

  /*
   * Add new blocks to the chain
   */
  if (oldNumBlocks < newNumBlocks)
  {
    while (oldNumBlocks < newNumBlocks)
    {
      blockIndex = StorageImpl_GetNextFreeBigBlock(This->parentStorage);

      StorageImpl_SetNextBlockInChain(
	This->parentStorage, 
	currentBlock, 
	blockIndex);

      StorageImpl_SetNextBlockInChain(
        This->parentStorage, 
	blockIndex, 
	BLOCK_END_OF_CHAIN);

      currentBlock = blockIndex;
      oldNumBlocks++;
    }

    This->tailIndex = blockIndex;
    This->numBlocks = newNumBlocks;
  }

  return TRUE;
}

/******************************************************************************
 *      BlockChainStream_SetSize
 *
 * Sets the size of this stream. The big block depot will be updated.
 * The file will grow if we grow the chain.
 *
 * TODO: Free the actual blocks in the file when we shrink the chain.
 *       Currently, the blocks are still in the file. So the file size
 *       doesn't shrink even if we shrink streams. 
 */
BOOL BlockChainStream_SetSize(
  BlockChainStream* This,
  ULARGE_INTEGER    newSize)
{
  ULARGE_INTEGER size = BlockChainStream_GetSize(This);

  if (newSize.u.LowPart == size.u.LowPart)
    return TRUE;

  if (newSize.u.LowPart < size.u.LowPart)
  {
    BlockChainStream_Shrink(This, newSize);
  }
  else
  {
    ULARGE_INTEGER fileSize = 
      BIGBLOCKFILE_GetSize(This->parentStorage->bigBlockFile);

    ULONG diff = newSize.u.LowPart - size.u.LowPart;

    /*
     * Make sure the file stays a multiple of blocksize
     */
    if ((diff % This->parentStorage->bigBlockSize) != 0)
      diff += (This->parentStorage->bigBlockSize - 
                (diff % This->parentStorage->bigBlockSize) );

    fileSize.u.LowPart += diff;
    BIGBLOCKFILE_SetSize(This->parentStorage->bigBlockFile, fileSize);

    BlockChainStream_Enlarge(This, newSize);
  }

  return TRUE;
}

/******************************************************************************
 *      BlockChainStream_GetSize
 *
 * Returns the size of this chain.
 * Will return the block count if this chain doesn't have a property.
 */
ULARGE_INTEGER BlockChainStream_GetSize(BlockChainStream* This)
{
  StgProperty chainProperty;

  if(This->headOfStreamPlaceHolder == NULL)
  {
    /* 
     * This chain is a data stream read the property and return 
     * the appropriate size
     */
    StorageImpl_ReadProperty(
      This->parentStorage,
      This->ownerPropertyIndex,
      &chainProperty);

    return chainProperty.size;
  }
  else
  {
    /*
     * this chain is a chain that does not have a property, figure out the 
     * size by making the product number of used blocks times the 
     * size of them
     */
    ULARGE_INTEGER result;
    result.u.HighPart = 0;

    result.u.LowPart  = 
      BlockChainStream_GetCount(This) * 
      This->parentStorage->bigBlockSize;

    return result;
  }
}

/******************************************************************************
** SmallBlockChainStream implementation
*/

SmallBlockChainStream* SmallBlockChainStream_Construct(
  StorageImpl* parentStorage,  
  ULONG          propertyIndex)
{
  SmallBlockChainStream* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(SmallBlockChainStream));

  newStream->parentStorage      = parentStorage;
  newStream->ownerPropertyIndex = propertyIndex;

  return newStream;
}

void SmallBlockChainStream_Destroy(
  SmallBlockChainStream* This)
{
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      SmallBlockChainStream_GetHeadOfChain
 *
 * Returns the head of this chain of small blocks.
 */
ULONG SmallBlockChainStream_GetHeadOfChain(
  SmallBlockChainStream* This)
{
  StgProperty chainProperty;
  BOOL      readSuccessful;

  if (This->ownerPropertyIndex)
  {
    readSuccessful = StorageImpl_ReadProperty(
                      This->parentStorage,
                      This->ownerPropertyIndex,
                      &chainProperty);

    if (readSuccessful)
    {
      return chainProperty.startingBlock;
    }

  }

  return BLOCK_END_OF_CHAIN;
}

/******************************************************************************
 *      SmallBlockChainStream_GetNextBlockInChain
 *
 * Returns the index of the next small block in this chain.
 * 
 * Return Values:
 *    - BLOCK_END_OF_CHAIN: end of this chain
 *    - BLOCK_UNUSED: small block 'blockIndex' is free
 */
ULONG SmallBlockChainStream_GetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  nextBlockInChain = BLOCK_END_OF_CHAIN;
  ULONG  bytesRead;
  BOOL success;

  offsetOfBlockInDepot.u.HighPart = 0;
  offsetOfBlockInDepot.u.LowPart  = blockIndex * sizeof(ULONG);

  /*
   * Read those bytes in the buffer from the small block file.
   */
  success = BlockChainStream_ReadAt(
              This->parentStorage->smallBlockDepotChain,
              offsetOfBlockInDepot,
              sizeof(DWORD),
              &buffer,
              &bytesRead);

  if (success)
  {
    StorageUtl_ReadDWord(&buffer, 0, &nextBlockInChain);
  }

  return nextBlockInChain;
}

/******************************************************************************
 *       SmallBlockChainStream_SetNextBlockInChain
 *
 * Writes the index of the next block of the specified block in the small
 * block depot.
 * To set the end of chain use BLOCK_END_OF_CHAIN as nextBlock.
 * To flag a block as free use BLOCK_UNUSED as nextBlock.
 */
void SmallBlockChainStream_SetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex,
  ULONG                  nextBlock)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  bytesWritten;

  offsetOfBlockInDepot.u.HighPart = 0;
  offsetOfBlockInDepot.u.LowPart  = blockIndex * sizeof(ULONG);

  StorageUtl_WriteDWord(&buffer, 0, nextBlock);

  /*
   * Read those bytes in the buffer from the small block file.
   */
  BlockChainStream_WriteAt(
    This->parentStorage->smallBlockDepotChain,
    offsetOfBlockInDepot,
    sizeof(DWORD),
    &buffer,
    &bytesWritten);
}

/******************************************************************************
 *      SmallBlockChainStream_FreeBlock
 *
 * Flag small block 'blockIndex' as free in the small block depot.
 */
void SmallBlockChainStream_FreeBlock(
  SmallBlockChainStream* This,
  ULONG                  blockIndex)
{
  SmallBlockChainStream_SetNextBlockInChain(This, blockIndex, BLOCK_UNUSED);
}

/******************************************************************************
 *      SmallBlockChainStream_GetNextFreeBlock
 *
 * Returns the index of a free small block. The small block depot will be
 * enlarged if necessary. The small block chain will also be enlarged if
 * necessary.
 */
ULONG SmallBlockChainStream_GetNextFreeBlock(
  SmallBlockChainStream* This)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD buffer;
  ULONG bytesRead;
  ULONG blockIndex = 0;
  ULONG nextBlockIndex = BLOCK_END_OF_CHAIN;
  BOOL success = TRUE;
  ULONG smallBlocksPerBigBlock;

  offsetOfBlockInDepot.u.HighPart = 0;

  /*
   * Scan the small block depot for a free block
   */
  while (nextBlockIndex != BLOCK_UNUSED)
  {
    offsetOfBlockInDepot.u.LowPart = blockIndex * sizeof(ULONG);

    success = BlockChainStream_ReadAt(
                This->parentStorage->smallBlockDepotChain,
                offsetOfBlockInDepot,
                sizeof(DWORD),
                &buffer,
                &bytesRead);

    /*
     * If we run out of space for the small block depot, enlarge it
     */
    if (success)
    {
      StorageUtl_ReadDWord(&buffer, 0, &nextBlockIndex);

      if (nextBlockIndex != BLOCK_UNUSED)
        blockIndex++;
    }
    else
    {
      ULONG count = 
        BlockChainStream_GetCount(This->parentStorage->smallBlockDepotChain);

      ULONG sbdIndex = This->parentStorage->smallBlockDepotStart;
      ULONG nextBlock, newsbdIndex;
      BYTE* smallBlockDepot;

      nextBlock = sbdIndex;
      while (nextBlock != BLOCK_END_OF_CHAIN)
      {
        sbdIndex = nextBlock;
        nextBlock = 
          StorageImpl_GetNextBlockInChain(This->parentStorage, sbdIndex);
      }

      newsbdIndex = StorageImpl_GetNextFreeBigBlock(This->parentStorage);
      if (sbdIndex != BLOCK_END_OF_CHAIN)
        StorageImpl_SetNextBlockInChain(
          This->parentStorage, 
          sbdIndex, 
          newsbdIndex);

      StorageImpl_SetNextBlockInChain(
        This->parentStorage, 
        newsbdIndex, 
        BLOCK_END_OF_CHAIN);

      /*
       * Initialize all the small blocks to free
       */
      smallBlockDepot = 
        StorageImpl_GetBigBlock(This->parentStorage, newsbdIndex);

      memset(smallBlockDepot, BLOCK_UNUSED, This->parentStorage->bigBlockSize);
      StorageImpl_ReleaseBigBlock(This->parentStorage, smallBlockDepot);

      if (count == 0)
      {
        /*
         * We have just created the small block depot.
         */
        StgProperty rootProp;
        ULONG sbStartIndex; 

        /*
         * Save it in the header
         */
        This->parentStorage->smallBlockDepotStart = newsbdIndex;
        StorageImpl_SaveFileHeader(This->parentStorage);

        /*
         * And allocate the first big block that will contain small blocks 
         */
        sbStartIndex = 
          StorageImpl_GetNextFreeBigBlock(This->parentStorage);

        StorageImpl_SetNextBlockInChain(
          This->parentStorage, 
          sbStartIndex, 
          BLOCK_END_OF_CHAIN);

        StorageImpl_ReadProperty(
          This->parentStorage, 
          This->parentStorage->rootPropertySetIndex, 
          &rootProp);

        rootProp.startingBlock = sbStartIndex;
        rootProp.size.u.HighPart = 0;
        rootProp.size.u.LowPart  = This->parentStorage->bigBlockSize;

        StorageImpl_WriteProperty(
          This->parentStorage, 
          This->parentStorage->rootPropertySetIndex, 
          &rootProp);
      }
    }
  }

  smallBlocksPerBigBlock = 
    This->parentStorage->bigBlockSize / This->parentStorage->smallBlockSize;

  /*
   * Verify if we have to allocate big blocks to contain small blocks
   */
  if (blockIndex % smallBlocksPerBigBlock == 0)
  {
    StgProperty rootProp;
    ULONG blocksRequired = (blockIndex / smallBlocksPerBigBlock) + 1;

    StorageImpl_ReadProperty(
      This->parentStorage, 
      This->parentStorage->rootPropertySetIndex, 
      &rootProp);

    if (rootProp.size.u.LowPart < 
       (blocksRequired * This->parentStorage->bigBlockSize))
    {
      rootProp.size.u.LowPart += This->parentStorage->bigBlockSize;

      BlockChainStream_SetSize(
        This->parentStorage->smallBlockRootChain, 
        rootProp.size);

      StorageImpl_WriteProperty(
        This->parentStorage, 
        This->parentStorage->rootPropertySetIndex, 
        &rootProp);
    }
  }

  return blockIndex;
}

/******************************************************************************
 *      SmallBlockChainStream_ReadAt
 *
 * Reads a specified number of bytes from this chain at the specified offset.
 * bytesRead may be NULL.
 * Failure will be returned if the specified number of bytes has not been read. 
 */
BOOL SmallBlockChainStream_ReadAt(
  SmallBlockChainStream* This,
  ULARGE_INTEGER         offset,
  ULONG                  size,
  void*                  buffer,
  ULONG*                 bytesRead)
{
  ULARGE_INTEGER offsetInBigBlockFile;
  ULONG blockNoInSequence = 
    offset.u.LowPart / This->parentStorage->smallBlockSize;

  ULONG offsetInBlock = offset.u.LowPart % This->parentStorage->smallBlockSize;
  ULONG bytesToReadInBuffer;
  ULONG blockIndex;
  ULONG bytesReadFromBigBlockFile;
  BYTE* bufferWalker;

  /*
   * This should never happen on a small block file.
   */
  assert(offset.u.HighPart==0);

  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);

    blockNoInSequence--;
  }

  /*
   * Start reading the buffer.
   */
  *bytesRead   = 0;
  bufferWalker = buffer;

  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy from this small block.
     */
    bytesToReadInBuffer = 
      min(This->parentStorage->smallBlockSize - offsetInBlock, size);

    /*
     * Calculate the offset of the small block in the small block file.
     */
    offsetInBigBlockFile.u.HighPart  = 0;
    offsetInBigBlockFile.u.LowPart   = 
      blockIndex * This->parentStorage->smallBlockSize;

    offsetInBigBlockFile.u.LowPart  += offsetInBlock;

    /*
     * Read those bytes in the buffer from the small block file.
     */
    BlockChainStream_ReadAt(This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToReadInBuffer,
      bufferWalker,
      &bytesReadFromBigBlockFile);

    assert(bytesReadFromBigBlockFile == bytesToReadInBuffer);

    /*
     * Step to the next big block.
     */
    blockIndex    = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);
    bufferWalker += bytesToReadInBuffer;
    size         -= bytesToReadInBuffer;
    *bytesRead   += bytesToReadInBuffer;
    offsetInBlock = 0;  /* There is no offset on the next block */
  }

  return (size == 0);
}

/******************************************************************************
 *       SmallBlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * bytesWritten may be NULL.
 * Will fail if not all specified number of bytes have been written.
 */
BOOL SmallBlockChainStream_WriteAt(
  SmallBlockChainStream* This,
  ULARGE_INTEGER offset,
  ULONG          size,
  const void*    buffer,
  ULONG*         bytesWritten)
{
  ULARGE_INTEGER offsetInBigBlockFile;
  ULONG blockNoInSequence = 
    offset.u.LowPart / This->parentStorage->smallBlockSize;

  ULONG offsetInBlock = offset.u.LowPart % This->parentStorage->smallBlockSize;
  ULONG bytesToWriteInBuffer;
  ULONG blockIndex;
  ULONG bytesWrittenFromBigBlockFile;
  BYTE* bufferWalker;
  
  /*
   * This should never happen on a small block file.
   */
  assert(offset.u.HighPart==0);
  
  /*
   * Find the first block in the stream that contains part of the buffer.
   */
  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);
  
  while ( (blockNoInSequence > 0) &&  (blockIndex != BLOCK_END_OF_CHAIN))
  {
    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);
    
    blockNoInSequence--;
  }
  
  /*
   * Start writing the buffer.
   *
   * Here, I'm casting away the constness on the buffer variable
   * This is OK since we don't intend to modify that buffer.
   */
  *bytesWritten   = 0;
  bufferWalker = (BYTE*)buffer;
  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    /*
     * Calculate how many bytes we can copy to this small block.
     */
    bytesToWriteInBuffer = 
      min(This->parentStorage->smallBlockSize - offsetInBlock, size);
    
    /*
     * Calculate the offset of the small block in the small block file.
     */
    offsetInBigBlockFile.u.HighPart  = 0;
    offsetInBigBlockFile.u.LowPart   = 
      blockIndex * This->parentStorage->smallBlockSize;

    offsetInBigBlockFile.u.LowPart  += offsetInBlock;
    
    /*
     * Write those bytes in the buffer to the small block file.
     */
    BlockChainStream_WriteAt(This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToWriteInBuffer,
      bufferWalker,
      &bytesWrittenFromBigBlockFile);
    
    assert(bytesWrittenFromBigBlockFile == bytesToWriteInBuffer);
    
    /*
     * Step to the next big block.
     */
    blockIndex    = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);
    bufferWalker  += bytesToWriteInBuffer;
    size          -= bytesToWriteInBuffer;
    *bytesWritten += bytesToWriteInBuffer;
    offsetInBlock  = 0;     /* There is no offset on the next block */
  }
  
  return (size == 0);
}

/******************************************************************************
 *       SmallBlockChainStream_Shrink
 *
 * Shrinks this chain in the small block depot. 
 */
BOOL SmallBlockChainStream_Shrink(
  SmallBlockChainStream* This,
  ULARGE_INTEGER newSize)
{
  ULONG blockIndex, extraBlock;
  ULONG numBlocks;
  ULONG count = 0;

  numBlocks = newSize.u.LowPart / This->parentStorage->smallBlockSize;

  if ((newSize.u.LowPart % This->parentStorage->smallBlockSize) != 0)
    numBlocks++;

  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  /*
   * Go to the new end of chain
   */
  while (count < numBlocks)
  {
    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);
    count++;
  }

  /*
   * If the count is 0, we have a special case, the head of the chain was
   * just freed. 
   */
  if (count == 0)
  {
    StgProperty chainProp;

    StorageImpl_ReadProperty(This->parentStorage, 
			     This->ownerPropertyIndex,
			     &chainProp);

    chainProp.startingBlock = BLOCK_END_OF_CHAIN;

    StorageImpl_WriteProperty(This->parentStorage,
			      This->ownerPropertyIndex,
			      &chainProp);

    /*
     * We start freeing the chain at the head block.
     */
    extraBlock = blockIndex;
  }
  else
  {
    /* Get the next block before marking the new end */
    extraBlock = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);

    /* Mark the new end of chain */
    SmallBlockChainStream_SetNextBlockInChain(
      This, 
      blockIndex, 
      BLOCK_END_OF_CHAIN);
  }

  /*
   * Mark the extra blocks as free
   */
  while (extraBlock != BLOCK_END_OF_CHAIN)
  {
    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, extraBlock);
    SmallBlockChainStream_FreeBlock(This, extraBlock);
    extraBlock = blockIndex;
  }

  return TRUE;  
}

/******************************************************************************
 *      SmallBlockChainStream_Enlarge
 *
 * Grows this chain in the small block depot.
 */
BOOL SmallBlockChainStream_Enlarge(
  SmallBlockChainStream* This,
  ULARGE_INTEGER newSize)
{
  ULONG blockIndex, currentBlock;
  ULONG newNumBlocks;
  ULONG oldNumBlocks = 0;

  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  /*
   * Empty chain
   */
  if (blockIndex == BLOCK_END_OF_CHAIN)
  {

    StgProperty chainProp;

    StorageImpl_ReadProperty(This->parentStorage, This->ownerPropertyIndex,
                               &chainProp);

    chainProp.startingBlock = SmallBlockChainStream_GetNextFreeBlock(This);

    StorageImpl_WriteProperty(This->parentStorage, This->ownerPropertyIndex,
                                &chainProp);

    blockIndex = chainProp.startingBlock;
    SmallBlockChainStream_SetNextBlockInChain(
      This, 
      blockIndex, 
      BLOCK_END_OF_CHAIN);
  }

  currentBlock = blockIndex;

  /*
   * Figure out how many blocks are needed to contain this stream
   */
  newNumBlocks = newSize.u.LowPart / This->parentStorage->smallBlockSize;

  if ((newSize.u.LowPart % This->parentStorage->smallBlockSize) != 0)
    newNumBlocks++;

  /*
   * Go to the current end of chain
   */
  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    oldNumBlocks++;
    currentBlock = blockIndex;
    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, currentBlock);
  }

  /*
   * Add new blocks to the chain
   */
  while (oldNumBlocks < newNumBlocks)
  {
    blockIndex = SmallBlockChainStream_GetNextFreeBlock(This);
    SmallBlockChainStream_SetNextBlockInChain(This, currentBlock, blockIndex);

    SmallBlockChainStream_SetNextBlockInChain(
      This, 
      blockIndex, 
      BLOCK_END_OF_CHAIN);

    currentBlock = blockIndex;
    oldNumBlocks++;
  }

  return TRUE;
}

/******************************************************************************
 *      SmallBlockChainStream_GetCount
 *
 * Returns the number of blocks that comprises this chain.
 * This is not the size of this chain as the last block may not be full!
 */
ULONG SmallBlockChainStream_GetCount(SmallBlockChainStream* This)
{
  ULONG blockIndex;
  ULONG count = 0;

  blockIndex = SmallBlockChainStream_GetHeadOfChain(This);

  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    count++;

    blockIndex = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex);
  }

  return count;
}

/******************************************************************************
 *      SmallBlockChainStream_SetSize
 *
 * Sets the size of this stream.
 * The file will grow if we grow the chain.
 *
 * TODO: Free the actual blocks in the file when we shrink the chain.
 *       Currently, the blocks are still in the file. So the file size
 *       doesn't shrink even if we shrink streams. 
 */
BOOL SmallBlockChainStream_SetSize(
                SmallBlockChainStream* This,
                ULARGE_INTEGER    newSize)
{
  ULARGE_INTEGER size = SmallBlockChainStream_GetSize(This);

  if (newSize.u.LowPart == size.u.LowPart)
    return TRUE;

  if (newSize.u.LowPart < size.u.LowPart)
  {
    SmallBlockChainStream_Shrink(This, newSize);
  }
  else
  {
    SmallBlockChainStream_Enlarge(This, newSize);
  }

  return TRUE;
}

/******************************************************************************
 *      SmallBlockChainStream_GetSize
 *
 * Returns the size of this chain.
 */
ULARGE_INTEGER SmallBlockChainStream_GetSize(SmallBlockChainStream* This)
{
  StgProperty chainProperty;

  StorageImpl_ReadProperty(
    This->parentStorage,
    This->ownerPropertyIndex,
    &chainProperty);

  return chainProperty.size;
}

/******************************************************************************
 *    StgCreateDocfile  [OLE32.144]
 */
HRESULT WINAPI StgCreateDocfile(
  LPCOLESTR pwcsName,
  DWORD       grfMode,
  DWORD       reserved,
  IStorage  **ppstgOpen)
{
  StorageImpl* newStorage = 0;
  HANDLE       hFile      = INVALID_HANDLE_VALUE;
  HRESULT        hr         = S_OK;
  DWORD          shareMode;
  DWORD          accessMode;
  DWORD          creationMode;
  DWORD          fileAttributes;
  WCHAR          tempFileName[MAX_PATH];

  Print(MAX_TRACE, ("(%S, %lx, %ld, %p)\n", 
	pwcsName, grfMode, 
	reserved, ppstgOpen));

  /*
   * Validate the parameters
   */
  if (ppstgOpen == 0)
    return STG_E_INVALIDPOINTER;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * Generate a unique name.
   */
  if (pwcsName == 0)
  {
    WCHAR tempPath[MAX_PATH];
    WCHAR prefix[] = { 'S', 'T', 'O', 0 };

    if (!(grfMode & STGM_SHARE_EXCLUSIVE))
      return STG_E_INVALIDFLAG;
    if (!(grfMode & (STGM_WRITE|STGM_READWRITE)))
      return STG_E_INVALIDFLAG;

    memset(tempPath, 0, sizeof(tempPath));
    memset(tempFileName, 0, sizeof(tempFileName));

    if ((GetTempPathW(MAX_PATH, tempPath)) == 0 )
      tempPath[0] = '.';

    if (GetTempFileNameW(tempPath, prefix, 0, tempFileName) != 0)
      pwcsName = tempFileName;
    else
      return STG_E_INSUFFICIENTMEMORY;

    creationMode = TRUNCATE_EXISTING;
  }
  else
  {
    creationMode = GetCreationModeFromSTGM(grfMode);
  }

  /*
   * Interpret the STGM value grfMode 
   */
  shareMode    = GetShareModeFromSTGM(grfMode);
  accessMode   = GetAccessModeFromSTGM(grfMode);

  if (grfMode & STGM_DELETEONRELEASE)
    fileAttributes = FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_DELETE_ON_CLOSE;
  else
    fileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;

  if (grfMode & STGM_TRANSACTED)
    UNIMPLEMENTED;

  /*
   * Initialize the "out" parameter.
   */
  *ppstgOpen = 0;

  hFile = CreateFileW(pwcsName,
                        accessMode,
                        shareMode,
            NULL,
                        creationMode,
                        fileAttributes,
            0);
 
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return E_FAIL;
  }

  /*
   * Allocate and initialize the new IStorage32object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));
 
  if (newStorage == 0)
    return STG_E_INSUFFICIENTMEMORY;

  hr = StorageImpl_Construct(
         newStorage,
         hFile,
         NULL,
         grfMode,
         TRUE,
         TRUE);
 
  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    return hr;
  }

  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);

  return hr;
}

/******************************************************************************
 *              StgOpenStorage        [OLE32.148]
 */
HRESULT WINAPI StgOpenStorage(
  const OLECHAR *pwcsName,
  IStorage      *pstgPriority,
  DWORD           grfMode,
  SNB           snbExclude,
  DWORD           reserved, 
  IStorage      **ppstgOpen)
{
  StorageImpl* newStorage = 0;
  HRESULT        hr = S_OK;
  HANDLE       hFile = 0;
  DWORD          shareMode;
  DWORD          accessMode;

  Print(MAX_TRACE, ("(%S, %p, %lx, %p, %ld, %p)\n", 
	pwcsName, pstgPriority, grfMode,
	snbExclude, reserved, ppstgOpen));

  /*
   * Perform a sanity check
   */
  if (( pwcsName == 0) || (ppstgOpen == 0) )
    return STG_E_INVALIDPOINTER;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * Interpret the STGM value grfMode
   */
  shareMode    = GetShareModeFromSTGM(grfMode);
  accessMode   = GetAccessModeFromSTGM(grfMode);

  /*
   * Initialize the "out" parameter.
   */
  *ppstgOpen = 0;
  
  hFile = CreateFileW( pwcsName, 
                       accessMode,
                       shareMode,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       0);
  
  
  if (hFile==INVALID_HANDLE_VALUE)
  {
    HRESULT hr = E_FAIL;
    DWORD last_error = GetLastError();

    switch (last_error)
    {
      case ERROR_FILE_NOT_FOUND:
        hr = STG_E_FILENOTFOUND;
        break;

      case ERROR_PATH_NOT_FOUND:
        hr = STG_E_PATHNOTFOUND;
        break;

      case ERROR_ACCESS_DENIED:
      case ERROR_WRITE_PROTECT:
        hr = STG_E_ACCESSDENIED;
        break;

      case ERROR_SHARING_VIOLATION:
        hr = STG_E_SHAREVIOLATION;
        break;

      default:
        hr = E_FAIL;
    }

    return hr;
  }

  /*
   * Allocate and initialize the new IStorage32object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));
  
  if (newStorage == 0)
    return STG_E_INSUFFICIENTMEMORY;

  hr = StorageImpl_Construct(
         newStorage,
         hFile,
         NULL,
         grfMode,
         TRUE,
         FALSE);
  
  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    /*
     * According to the docs if the file is not a storage, return STG_E_FILEALREADYEXISTS
     */
    if(hr == STG_E_INVALIDHEADER)
	return STG_E_FILEALREADYEXISTS;
    return hr;
  }
  
  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);
  
  return hr;
}

/******************************************************************************
 *    StgCreateDocfileOnILockBytes    [OLE32.145]
 */
HRESULT WINAPI StgCreateDocfileOnILockBytes(
      ILockBytes *plkbyt,
      DWORD grfMode,
      DWORD reserved,
      IStorage** ppstgOpen)
{
  StorageImpl*   newStorage = 0;
  HRESULT        hr         = S_OK;

  /*
   * Validate the parameters
   */
  if ((ppstgOpen == 0) || (plkbyt == 0))
    return STG_E_INVALIDPOINTER;

  /*
   * Allocate and initialize the new IStorage object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));

  if (newStorage == 0)
    return STG_E_INSUFFICIENTMEMORY;

  hr = StorageImpl_Construct(
         newStorage,
         0,
         plkbyt,
         grfMode,
         FALSE,
         TRUE);

  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    return hr;
  }

  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);

  return hr;  
}

/******************************************************************************
 *    StgOpenStorageOnILockBytes    [OLE32.149]
 */
HRESULT WINAPI StgOpenStorageOnILockBytes(
      ILockBytes *plkbyt,
      IStorage *pstgPriority,
      DWORD grfMode,
      SNB snbExclude,
      DWORD reserved,
      IStorage **ppstgOpen)
{
  StorageImpl* newStorage = 0;
  HRESULT        hr = S_OK;

  /*
   * Perform a sanity check
   */
  if ((plkbyt == 0) || (ppstgOpen == 0))
    return STG_E_INVALIDPOINTER;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  /*
   * Initialize the "out" parameter.
   */
  *ppstgOpen = 0;

  /*
   * Allocate and initialize the new IStorage object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));
 
  if (newStorage == 0)
    return STG_E_INSUFFICIENTMEMORY;

  hr = StorageImpl_Construct(
         newStorage,
         0,
         plkbyt,
         grfMode,
         FALSE,
         FALSE);

  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    return hr;
  }

  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);

  return hr;
}

/******************************************************************************
 *              StgSetTimes [ole32.150]
 *
 *
 */
HRESULT WINAPI StgSetTimes(WCHAR * str, FILETIME * a, FILETIME * b, FILETIME *c )
{
 
  UNIMPLEMENTED;
  return FALSE;
}

/******************************************************************************
 *              StgIsStorageILockBytes        [OLE32.147]
 *
 * Determines if the ILockBytes contains a storage object.
 */
HRESULT WINAPI StgIsStorageILockBytes(ILockBytes *plkbyt)
{
  BYTE sig[8];
  ULARGE_INTEGER offset;

  offset.u.HighPart = 0;
  offset.u.LowPart  = 0;

  ILockBytes_ReadAt(plkbyt, offset, sig, sizeof(sig), NULL);

  if (memcmp(sig, STORAGE_magic, sizeof(STORAGE_magic)) == 0)
    return S_OK;

  return S_FALSE;
}

/******************************************************************************
 *              WriteClassStg        [OLE32.158]
 *
 * This method will store the specified CLSID in the specified storage object
 */
HRESULT WINAPI WriteClassStg(IStorage* pStg, REFCLSID rclsid)
{
  HRESULT hRes;

  assert(pStg != 0);

  hRes = IStorage_SetClass(pStg, rclsid);

  return hRes;
}

/***********************************************************************
 *    ReadClassStg
 *
 * This method reads the CLSID previously written to a storage object with the WriteClassStg.
 */
HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid){

    STATSTG pstatstg;
    HRESULT hRes;
    
    Print(MAX_TRACE, ("()\n"));

    if(pclsid==NULL)
        return E_POINTER;
   /*
    * read a STATSTG structure (contains the clsid) from the storage
    */
    hRes=IStorage_Stat(pstg,&pstatstg,STATFLAG_DEFAULT);

    if(SUCCEEDED(hRes))
        *pclsid=pstatstg.clsid;

    return hRes;
}

/***********************************************************************
 *    OleLoadFromStream
 *
 * This function loads an object from stream
 */
HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj)
{
    CLSID	clsid;
    HRESULT	res;
    LPPERSISTSTREAM	xstm;

    Print(MAX_TRACE, ("(%p,%s,%p)\n",pStm,PRINT_GUID(iidInterface),ppvObj));

    res=ReadClassStm(pStm,&clsid);
    if (!SUCCEEDED(res))
	return res;
    res=CoCreateInstance(&clsid,NULL,CLSCTX_INPROC_SERVER,iidInterface,ppvObj);
    if (!SUCCEEDED(res))
	return res;
    res=IUnknown_QueryInterface((IUnknown*)*ppvObj,&IID_IPersistStream,(LPVOID*)&xstm);
    if (!SUCCEEDED(res)) {
	IUnknown_Release((IUnknown*)*ppvObj);
	return res;
    }
    res=IPersistStream_Load(xstm,pStm);
    IPersistStream_Release(xstm);
    /* FIXME: all refcounts ok at this point? I think they should be:
     * 		pStm	: unchanged
     *		ppvObj	: 1
     *		xstm	: 0 (released)
     */
    return res;
}

/***********************************************************************
 *    OleSaveToStream
 *
 * This function saves an object with the IPersistStream interface on it
 * to the specified stream.
 */
HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm)
{

    CLSID clsid;
    HRESULT res;
    
    Print(MAX_TRACE, ("(%p,%p)\n",pPStm,pStm));

    res=IPersistStream_GetClassID(pPStm,&clsid);

    if (SUCCEEDED(res)){
        
        res=WriteClassStm(pStm,&clsid);

        if (SUCCEEDED(res))

            res=IPersistStream_Save(pPStm,pStm,TRUE);
    }

    Print(MAX_TRACE, ("Finished Save\n"));
    return res;
}

/****************************************************************************
 * This method validate a STGM parameter that can contain the values below
 *
 * STGM_DIRECT               0x00000000
 * STGM_TRANSACTED           0x00010000
 * STGM_SIMPLE               0x08000000
 * 
 * STGM_READ                 0x00000000
 * STGM_WRITE                0x00000001
 * STGM_READWRITE            0x00000002
 * 
 * STGM_SHARE_DENY_NONE      0x00000040
 * STGM_SHARE_DENY_READ      0x00000030
 * STGM_SHARE_DENY_WRITE     0x00000020
 * STGM_SHARE_EXCLUSIVE      0x00000010
 * 
 * STGM_PRIORITY             0x00040000
 * STGM_DELETEONRELEASE      0x04000000
 *
 * STGM_CREATE               0x00001000
 * STGM_CONVERT              0x00020000
 * STGM_FAILIFTHERE          0x00000000
 *
 * STGM_NOSCRATCH            0x00100000
 * STGM_NOSNAPSHOT           0x00200000
 */
static HRESULT validateSTGM(DWORD stgm)
{
  BOOL bSTGM_TRANSACTED       = ((stgm & STGM_TRANSACTED) == STGM_TRANSACTED);
  BOOL bSTGM_SIMPLE           = ((stgm & STGM_SIMPLE) == STGM_SIMPLE);
  BOOL bSTGM_DIRECT           = ! (bSTGM_TRANSACTED || bSTGM_SIMPLE);
   
  BOOL bSTGM_WRITE            = ((stgm & STGM_WRITE) == STGM_WRITE);
  BOOL bSTGM_READWRITE        = ((stgm & STGM_READWRITE) == STGM_READWRITE);
  BOOL bSTGM_READ             = ! (bSTGM_WRITE || bSTGM_READWRITE);
   
  BOOL bSTGM_SHARE_DENY_NONE  =
                     ((stgm & STGM_SHARE_DENY_NONE)  == STGM_SHARE_DENY_NONE);

  BOOL bSTGM_SHARE_DENY_READ  =
                     ((stgm & STGM_SHARE_DENY_READ)  == STGM_SHARE_DENY_READ);

  BOOL bSTGM_SHARE_DENY_WRITE =
                     ((stgm & STGM_SHARE_DENY_WRITE) == STGM_SHARE_DENY_WRITE);

  BOOL bSTGM_SHARE_EXCLUSIVE  =
                     ((stgm & STGM_SHARE_EXCLUSIVE)  == STGM_SHARE_EXCLUSIVE);

  BOOL bSTGM_CREATE           = ((stgm & STGM_CREATE) == STGM_CREATE);
  BOOL bSTGM_CONVERT          = ((stgm & STGM_CONVERT) == STGM_CONVERT);
   
  BOOL bSTGM_NOSCRATCH        = ((stgm & STGM_NOSCRATCH) == STGM_NOSCRATCH);
  BOOL bSTGM_NOSNAPSHOT       = ((stgm & STGM_NOSNAPSHOT) == STGM_NOSNAPSHOT);

  /* 
   * STGM_DIRECT | STGM_TRANSACTED | STGM_SIMPLE
   */
  if ( ! bSTGM_DIRECT )
    if( bSTGM_TRANSACTED && bSTGM_SIMPLE )
      return E_FAIL;

  /* 
   * STGM_WRITE |  STGM_READWRITE | STGM_READ
   */
  if ( ! bSTGM_READ )
    if( bSTGM_WRITE && bSTGM_READWRITE )
      return E_FAIL;

  /*
   * STGM_SHARE_DENY_NONE | others 
   * (I assume here that DENY_READ implies DENY_WRITE)
   */
  if ( bSTGM_SHARE_DENY_NONE )
    if ( bSTGM_SHARE_DENY_READ ||
         bSTGM_SHARE_DENY_WRITE || 
         bSTGM_SHARE_EXCLUSIVE) 
      return E_FAIL;

  /*
   * STGM_CREATE | STGM_CONVERT
   * if both are false, STGM_FAILIFTHERE is set to TRUE
   */
  if ( bSTGM_CREATE && bSTGM_CONVERT )
    return E_FAIL;

  /*
   * STGM_NOSCRATCH requires STGM_TRANSACTED
   */
  if ( bSTGM_NOSCRATCH && ! bSTGM_TRANSACTED )
    return E_FAIL;
  
  /*
   * STGM_NOSNAPSHOT requires STGM_TRANSACTED and 
   * not STGM_SHARE_EXCLUSIVE or STGM_SHARE_DENY_WRITE`
   */
  if (bSTGM_NOSNAPSHOT)
  {
    if ( ! ( bSTGM_TRANSACTED && 
           !(bSTGM_SHARE_EXCLUSIVE || bSTGM_SHARE_DENY_WRITE)) )
    return E_FAIL;
  }

  return S_OK;
}

/****************************************************************************
 *      GetShareModeFromSTGM
 *
 * This method will return a share mode flag from a STGM value.
 * The STGM value is assumed valid. 
 */
static DWORD GetShareModeFromSTGM(DWORD stgm)
{
  DWORD dwShareMode = 0;
  BOOL bSTGM_SHARE_DENY_NONE  =
                     ((stgm & STGM_SHARE_DENY_NONE)  == STGM_SHARE_DENY_NONE);

  BOOL bSTGM_SHARE_DENY_READ  =
                     ((stgm & STGM_SHARE_DENY_READ)  == STGM_SHARE_DENY_READ);

  BOOL bSTGM_SHARE_DENY_WRITE =
                     ((stgm & STGM_SHARE_DENY_WRITE) == STGM_SHARE_DENY_WRITE);

  BOOL bSTGM_SHARE_EXCLUSIVE  =
                     ((stgm & STGM_SHARE_EXCLUSIVE)  == STGM_SHARE_EXCLUSIVE);

  if ((bSTGM_SHARE_EXCLUSIVE) || (bSTGM_SHARE_DENY_READ))
    dwShareMode = 0;

  if (bSTGM_SHARE_DENY_NONE)
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

  if (bSTGM_SHARE_DENY_WRITE)
    dwShareMode = FILE_SHARE_READ;

  return dwShareMode;
}

/****************************************************************************
 *      GetAccessModeFromSTGM
 *
 * This method will return an access mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetAccessModeFromSTGM(DWORD stgm)
{
  DWORD dwDesiredAccess = GENERIC_READ;
  BOOL bSTGM_WRITE     = ((stgm & STGM_WRITE) == STGM_WRITE);
  BOOL bSTGM_READWRITE = ((stgm & STGM_READWRITE) == STGM_READWRITE);
  BOOL bSTGM_READ      = ! (bSTGM_WRITE || bSTGM_READWRITE);

  if (bSTGM_READ)
    dwDesiredAccess = GENERIC_READ;

  if (bSTGM_WRITE)
    dwDesiredAccess |= GENERIC_WRITE;

  if (bSTGM_READWRITE)
    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;

  return dwDesiredAccess;
}

/****************************************************************************
 *      GetCreationModeFromSTGM
 *
 * This method will return a creation mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetCreationModeFromSTGM(DWORD stgm)
{
  if ( stgm & STGM_CREATE)
    return CREATE_ALWAYS;
  if (stgm & STGM_CONVERT) {
    UNIMPLEMENTED;
    return CREATE_NEW;
  }
  /* All other cases */
  if (stgm & ~ (STGM_CREATE|STGM_CONVERT))
    Print(MIN_TRACE, ("unhandled storage mode : 0x%08lx\n",stgm & ~ (STGM_CREATE|STGM_CONVERT)));
  return CREATE_NEW;
}


/*************************************************************************
 * OLECONVERT_LoadOLE10 [Internal] 
 *
 * Loads the OLE10 STREAM to memory 
 *
 * PARAMS
 *     pOleStream   [I] The OLESTREAM
 *     pData        [I] Data Structure for the OLESTREAM Data
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_GET for invalid Get
 *               CONVERT10_E_OLESTREAM_FMT if the OLEID is invalide
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *     
 *     Memory allocated for pData must be freed by the caller
 */
HRESULT OLECONVERT_LoadOLE10(LPOLESTREAM pOleStream, OLECONVERT_OLESTREAM_DATA *pData, BOOL bStrem1)
{
	DWORD dwSize;
	HRESULT hRes = S_OK;
	int nTryCnt=0;
	int max_try = 6;

	pData->pData = NULL;
	pData->pstrOleObjFileName = (CHAR *) NULL;

	for( nTryCnt=0;nTryCnt < max_try; nTryCnt++)
	{
	/* Get the OleID */
	dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwOleID), sizeof(pData->dwOleID));
	if(dwSize != sizeof(pData->dwOleID))
	{
		hRes = CONVERT10_E_OLESTREAM_GET;
	}
	else if(pData->dwOleID != OLESTREAM_ID)
	{
		hRes = CONVERT10_E_OLESTREAM_FMT;
	}
		else
		{
			hRes = S_OK;
			break;
		}
	}

	if(hRes == S_OK)
	{
		/* Get the TypeID...more info needed for this field */
		dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwTypeID), sizeof(pData->dwTypeID));
		if(dwSize != sizeof(pData->dwTypeID))
		{
			hRes = CONVERT10_E_OLESTREAM_GET;
		}
	}
	if(hRes == S_OK)
	{
		if(pData->dwTypeID != 0)
		{
			/* Get the lenght of the OleTypeName */
			dwSize = pOleStream->lpstbl->Get(pOleStream, (void *) &(pData->dwOleTypeNameLength), sizeof(pData->dwOleTypeNameLength));
			if(dwSize != sizeof(pData->dwOleTypeNameLength))
			{
				hRes = CONVERT10_E_OLESTREAM_GET;
			}

			if(hRes == S_OK)
			{
				if(pData->dwOleTypeNameLength > 0)
				{
					/* Get the OleTypeName */
					dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)pData->strOleTypeName, pData->dwOleTypeNameLength);
					if(dwSize != pData->dwOleTypeNameLength)
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
			if(bStrem1)
			{
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwOleObjFileNameLength), sizeof(pData->dwOleObjFileNameLength));
				if(dwSize != sizeof(pData->dwOleObjFileNameLength))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			if(hRes == S_OK)
			{
					if(pData->dwOleObjFileNameLength < 1) /* there is no file name exist */
						pData->dwOleObjFileNameLength = sizeof(pData->dwOleObjFileNameLength);
					pData->pstrOleObjFileName = (CHAR *)malloc(pData->dwOleObjFileNameLength);
					if(pData->pstrOleObjFileName)
					{
						dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)(pData->pstrOleObjFileName),pData->dwOleObjFileNameLength);
						if(dwSize != pData->dwOleObjFileNameLength)
						{
							hRes = CONVERT10_E_OLESTREAM_GET;
						}
					}
					else
						hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}
			else
			{
				/* Get the Width of the Metafile */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwMetaFileWidth), sizeof(pData->dwMetaFileWidth));
				if(dwSize != sizeof(pData->dwMetaFileWidth))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			if(hRes == S_OK)
			{
				/* Get the Height of the Metafile */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwMetaFileHeight), sizeof(pData->dwMetaFileHeight));
				if(dwSize != sizeof(pData->dwMetaFileHeight))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}
			}
			if(hRes == S_OK)
			{
				/* Get the Lenght of the Data */
				dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)&(pData->dwDataLength), sizeof(pData->dwDataLength));
				if(dwSize != sizeof(pData->dwDataLength))
				{
					hRes = CONVERT10_E_OLESTREAM_GET;
				}
			}

			if(hRes == S_OK) /* I don't know what is this 8 byts information is we have to figure out */
			{
				if(!bStrem1) /* if it is a second OLE stream data */
				{
					pData->dwDataLength -= 8;
					dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)(pData->strUnknown), sizeof(pData->strUnknown));
					if(dwSize != sizeof(pData->strUnknown))
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
			if(hRes == S_OK)
			{
				if(pData->dwDataLength > 0)
				{
					pData->pData = (BYTE *)HeapAlloc(GetProcessHeap(),0,pData->dwDataLength);

					/* Get Data (ex. IStorage, Metafile, or BMP) */
					if(pData->pData)
					{
						dwSize = pOleStream->lpstbl->Get(pOleStream, (void *)pData->pData, pData->dwDataLength);
						if(dwSize != pData->dwDataLength)
						{
							hRes = CONVERT10_E_OLESTREAM_GET;
						}
					}
					else
					{
						hRes = CONVERT10_E_OLESTREAM_GET;
					}
				}
			}
		}
	}
	return hRes;
}

/*************************************************************************
 * OLECONVERT_SaveOLE10 [Internal] 
 *
 * Saves the OLE10 STREAM From memory 
 *
 * PARAMS
 *     pData        [I] Data Structure for the OLESTREAM Data
 *     pOleStream   [I] The OLESTREAM to save
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_PUT for invalid Put
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *     
 */
HRESULT OLECONVERT_SaveOLE10(OLECONVERT_OLESTREAM_DATA *pData, LPOLESTREAM pOleStream)
{
    DWORD dwSize;
    HRESULT hRes = S_OK;


   /* Set the OleID */
    dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwOleID), sizeof(pData->dwOleID));
    if(dwSize != sizeof(pData->dwOleID))
    {
        hRes = CONVERT10_E_OLESTREAM_PUT;
    }

    if(hRes == S_OK)
    {
        /* Set the TypeID */
        dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwTypeID), sizeof(pData->dwTypeID));
        if(dwSize != sizeof(pData->dwTypeID))
        {
            hRes = CONVERT10_E_OLESTREAM_PUT;
        }
    }

    if(pData->dwOleID == OLESTREAM_ID && pData->dwTypeID != 0 && hRes == S_OK)
    {
        /* Set the Lenght of the OleTypeName */
        dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwOleTypeNameLength), sizeof(pData->dwOleTypeNameLength));
        if(dwSize != sizeof(pData->dwOleTypeNameLength))
        {
            hRes = CONVERT10_E_OLESTREAM_PUT;
        }

        if(hRes == S_OK)
        {
            if(pData->dwOleTypeNameLength > 0)
            {
                /* Set the OleTypeName */
                dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)  pData->strOleTypeName, pData->dwOleTypeNameLength);
                if(dwSize != pData->dwOleTypeNameLength)
                {
                    hRes = CONVERT10_E_OLESTREAM_PUT;
                }
            }
        }

        if(hRes == S_OK)
        {
            /* Set the width of the Metafile */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwMetaFileWidth), sizeof(pData->dwMetaFileWidth));
            if(dwSize != sizeof(pData->dwMetaFileWidth))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            /* Set the height of the Metafile */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwMetaFileHeight), sizeof(pData->dwMetaFileHeight));
            if(dwSize != sizeof(pData->dwMetaFileHeight))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            /* Set the lenght of the Data */
            dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)&(pData->dwDataLength), sizeof(pData->dwDataLength));
            if(dwSize != sizeof(pData->dwDataLength))
            {
                hRes = CONVERT10_E_OLESTREAM_PUT;
            }
        }

        if(hRes == S_OK)
        {
            if(pData->dwDataLength > 0)
            {
                /* Set the Data (eg. IStorage, Metafile, Bitmap) */
                dwSize = pOleStream->lpstbl->Put(pOleStream, (void *)  pData->pData, pData->dwDataLength);
                if(dwSize != pData->dwDataLength)
                {
                    hRes = CONVERT10_E_OLESTREAM_PUT;
                }
            }
        }
    }
    return hRes;
}

/*************************************************************************
 * OLECONVERT_GetOLE20FromOLE10[Internal] 
 *
 * This function copies OLE10 Data (the IStorage in the OLESTREAM) to disk,
 * opens it, and copies the content to the dest IStorage for 
 * OleConvertOLESTREAMToIStorage
 * 
 *
 * PARAMS
 *     pDestStorage  [I] The IStorage to copy the data to
 *     pBuffer       [I] Buffer that contains the IStorage from the OLESTREAM
 *     nBufferLength [I] The size of the buffer
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     
 *     
 */
void OLECONVERT_GetOLE20FromOLE10(LPSTORAGE pDestStorage, BYTE *pBuffer, DWORD nBufferLength)
{
    HRESULT hRes;
    HANDLE hFile;
    IStorage *pTempStorage;
    DWORD dwNumOfBytesWritten;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];
    WCHAR wstrPrefix[] = {'s', 'i', 's', 0};

    /* Create a temp File */
    GetTempPathW(MAX_PATH, wstrTempDir);
    GetTempFileNameW(wstrTempDir, wstrPrefix, 0, wstrTempFile);
    hFile = CreateFileW(wstrTempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        /* Write IStorage Data to File */
        WriteFile(hFile, pBuffer, nBufferLength, &dwNumOfBytesWritten, NULL);
        CloseHandle(hFile);

        /* Open and copy temp storage to the Dest Storage */
        hRes = StgOpenStorage(wstrTempFile, NULL, STGM_READ, NULL, 0, &pTempStorage);
        if(hRes == S_OK)
        {
            hRes = StorageImpl_CopyTo(pTempStorage, 0, NULL, NULL, pDestStorage);
            StorageBaseImpl_Release(pTempStorage);
        }
        DeleteFileW(wstrTempFile);
    }
}


/*************************************************************************
 * OLECONVERT_WriteOLE20ToBuffer [Internal] 
 *
 * Saves the OLE10 STREAM From memory 
 *
 * PARAMS
 *     pStorage  [I] The Src IStorage to copy
 *     pData     [I] The Dest Memory to write to.
 *
 * RETURNS
 *     The size in bytes allocated for pData
 *
 * NOTES
 *     Memory allocated for pData must be freed by the caller
 *
 *     Used by OleConvertIStorageToOLESTREAM only.
 *     
 */
DWORD OLECONVERT_WriteOLE20ToBuffer(LPSTORAGE pStorage, BYTE **pData)
{
    HANDLE hFile;
    HRESULT hRes;
    DWORD nDataLength = 0;
    IStorage *pTempStorage;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];
    WCHAR wstrPrefix[] = {'s', 'i', 's', 0};

    *pData = NULL;
    
    /* Create temp Storage */
    GetTempPathW(MAX_PATH, wstrTempDir);
    GetTempFileNameW(wstrTempDir, wstrPrefix, 0, wstrTempFile);
    hRes = StgCreateDocfile(wstrTempFile, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pTempStorage);

    if(hRes == S_OK)
    {
        /* Copy Src Storage to the Temp Storage */
        StorageImpl_CopyTo(pStorage, 0, NULL, NULL, pTempStorage);
        StorageBaseImpl_Release(pTempStorage);

        /* Open Temp Storage as a file and copy to memory */
        hFile = CreateFileW(wstrTempFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            nDataLength = GetFileSize(hFile, NULL);
            *pData = (BYTE *) HeapAlloc(GetProcessHeap(),0,nDataLength);
            ReadFile(hFile, *pData, nDataLength, &nDataLength, 0);
            CloseHandle(hFile);
        }
        DeleteFileW(wstrTempFile);
    }
    return nDataLength;
}

/*************************************************************************
 * OLECONVERT_CreateOleStream [Internal] 
 *
 * Creates the "\001OLE" stream in the IStorage if neccessary.
 *
 * PARAMS
 *     pStorage     [I] Dest storage to create the stream in
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     This stream is still unknown, MS Word seems to have extra data
 *     but since the data is stored in the OLESTREAM there should be
 *     no need to recreate the stream.  If the stream is manually 
 *     deleted it will create it with this default data.
 *     
 */
void OLECONVERT_CreateOleStream(LPSTORAGE pStorage)
{
    HRESULT hRes;
    IStream *pStream;
    WCHAR wstrStreamName[] = {1,'O', 'l', 'e', 0};
    BYTE pOleStreamHeader [] = 
    {
        0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00 
    };
    
    /* Create stream if not present */
    hRes = IStorage_CreateStream(pStorage, wstrStreamName, 
        STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );

    if(hRes == S_OK)
    {
        /* Write default Data */
        hRes = IStream_Write(pStream, pOleStreamHeader, sizeof(pOleStreamHeader), NULL);
        IStream_Release(pStream);
    }
}


/*************************************************************************
 * OLECONVERT_CreateCompObjStream [Internal] 
 *
 * Creates a "\001CompObj" is the destination IStorage if necessary.
 *
 * PARAMS
 *     pStorage       [I] The dest IStorage to create the CompObj Stream 
 *                        if necessary.
 *     strOleTypeName [I] The ProgID
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  REGDB_E_CLASSNOTREG if cannot reconstruct the stream
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     The stream data is stored in the OLESTREAM and there should be
 *     no need to recreate the stream.  If the stream is manually 
 *     deleted it will attempt to create it by querying the registry.
 *
 *     
 */
HRESULT OLECONVERT_CreateCompObjStream(LPSTORAGE pStorage, LPCSTR strOleTypeName)
{
    IStream *pStream;
    HRESULT hStorageRes, hRes = S_OK;
    OLECONVERT_ISTORAGE_COMPOBJ IStorageCompObj;
    WCHAR wstrStreamName[] = {1,'C', 'o', 'm', 'p', 'O', 'b', 'j', 0};

    BYTE pCompObjUnknown1[] = {0x01, 0x00, 0xFE, 0xFF, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    BYTE pCompObjUnknown2[] = {0xF4, 0x39, 0xB2, 0x71};

    /* Initialize the CompObj structure */
    memset(&IStorageCompObj, 0, sizeof(IStorageCompObj));
    memcpy(&(IStorageCompObj.byUnknown1), pCompObjUnknown1, sizeof(pCompObjUnknown1));
    memcpy(&(IStorageCompObj.byUnknown2), pCompObjUnknown2, sizeof(pCompObjUnknown2));


    /*  Create a CompObj stream if it doesn't exist */
    hStorageRes = IStorage_CreateStream(pStorage, wstrStreamName, 
        STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );
    if(hStorageRes == S_OK)
    {
        /* copy the OleTypeName to the compobj struct */
        IStorageCompObj.dwOleTypeNameLength = strlen(strOleTypeName)+1;
        strcpy(IStorageCompObj.strOleTypeName, strOleTypeName);

        /* copy the OleTypeName to the compobj struct */
        /* Note: in the test made, these were Identical      */
        IStorageCompObj.dwProgIDNameLength = strlen(strOleTypeName)+1;
        strcpy(IStorageCompObj.strProgIDName, strOleTypeName);

        /* Get the CLSID */
        hRes = CLSIDFromProgID16(IStorageCompObj.strProgIDName, &(IStorageCompObj.clsid));

        if(hRes == S_OK)
        {
            HKEY hKey;
            LONG hErr;
            /* Get the CLSID Default Name from the Registry */
            hErr = RegOpenKeyA(HKEY_CLASSES_ROOT, IStorageCompObj.strProgIDName, &hKey); 
            if(hErr == ERROR_SUCCESS)
            {
                char strTemp[OLESTREAM_MAX_STR_LEN];
                IStorageCompObj.dwCLSIDNameLength = OLESTREAM_MAX_STR_LEN;
                hErr = RegQueryValueA(hKey, NULL, strTemp, &(IStorageCompObj.dwCLSIDNameLength));
                if(hErr == ERROR_SUCCESS)
                {
                    strcpy(IStorageCompObj.strCLSIDName, strTemp);
                }
                RegCloseKey(hKey);
            }
        }

        /* Write CompObj Structure to stream */
        hRes = IStream_Write(pStream, IStorageCompObj.byUnknown1, sizeof(IStorageCompObj.byUnknown1), NULL);

        WriteClassStm(pStream,&(IStorageCompObj.clsid));

        hRes = IStream_Write(pStream, &(IStorageCompObj.dwCLSIDNameLength), sizeof(IStorageCompObj.dwCLSIDNameLength), NULL);
        if(IStorageCompObj.dwCLSIDNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strCLSIDName, IStorageCompObj.dwCLSIDNameLength, NULL);
        }
        hRes = IStream_Write(pStream, &(IStorageCompObj.dwOleTypeNameLength) , sizeof(IStorageCompObj.dwOleTypeNameLength), NULL);
        if(IStorageCompObj.dwOleTypeNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strOleTypeName , IStorageCompObj.dwOleTypeNameLength, NULL);
        }
        hRes = IStream_Write(pStream, &(IStorageCompObj.dwProgIDNameLength) , sizeof(IStorageCompObj.dwProgIDNameLength), NULL);
        if(IStorageCompObj.dwProgIDNameLength > 0)
        {
            hRes = IStream_Write(pStream, IStorageCompObj.strProgIDName , IStorageCompObj.dwProgIDNameLength, NULL);
        }
        hRes = IStream_Write(pStream, IStorageCompObj.byUnknown2 , sizeof(IStorageCompObj.byUnknown2), NULL);
        IStream_Release(pStream);
    }
    return hRes;
}


/*************************************************************************
 * OLECONVERT_CreateOlePresStream[Internal] 
 *
 * Creates the "\002OlePres000" Stream with the Metafile data
 *
 * PARAMS
 *     pStorage     [I] The dest IStorage to create \002OLEPres000 stream in.
 *     dwExtentX    [I] Width of the Metafile
 *     dwExtentY    [I] Height of the Metafile 
 *     pData        [I] Metafile data
 *     dwDataLength [I] Size of the Metafile data
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  CONVERT10_E_OLESTREAM_PUT for invalid Put
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *     
 */
void OLECONVERT_CreateOlePresStream(LPSTORAGE pStorage, DWORD dwExtentX, DWORD dwExtentY , BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;
    WCHAR wstrStreamName[] = {2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};
    BYTE pOlePresStreamHeader [] = 
    {
        0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 
        0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    BYTE pOlePresStreamHeaderEmpty [] = 
    {
        0x00, 0x00, 0x00, 0x00, 
        0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
     
    /* Create the OlePres000 Stream */
    hRes = IStorage_CreateStream(pStorage, wstrStreamName, 
        STGM_CREATE | STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );

    if(hRes == S_OK)
    {
        DWORD nHeaderSize;
        OLECONVERT_ISTORAGE_OLEPRES OlePres;

        memset(&OlePres, 0, sizeof(OlePres));
        /* Do we have any metafile data to save */
        if(dwDataLength > 0)
        {
            memcpy(OlePres.byUnknown1, pOlePresStreamHeader, sizeof(pOlePresStreamHeader));
            nHeaderSize = sizeof(pOlePresStreamHeader);
        }
        else
        {
            memcpy(OlePres.byUnknown1, pOlePresStreamHeaderEmpty, sizeof(pOlePresStreamHeaderEmpty));
            nHeaderSize = sizeof(pOlePresStreamHeaderEmpty);
        }
        /* Set width and height of the metafile */
        OlePres.dwExtentX = dwExtentX;
        OlePres.dwExtentY = -dwExtentY;

        /* Set Data and Lenght */
        if(dwDataLength > sizeof(METAFILEPICT16))
        {
            OlePres.dwSize = dwDataLength - sizeof(METAFILEPICT16);
            OlePres.pData = &(pData[8]);
        }
        /* Save OlePres000 Data to Stream */
        hRes = IStream_Write(pStream, OlePres.byUnknown1, nHeaderSize, NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwExtentX), sizeof(OlePres.dwExtentX), NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwExtentY), sizeof(OlePres.dwExtentY), NULL);
        hRes = IStream_Write(pStream, &(OlePres.dwSize), sizeof(OlePres.dwSize), NULL);
        if(OlePres.dwSize > 0)
        {
            hRes = IStream_Write(pStream, OlePres.pData, OlePres.dwSize, NULL);
        }
        IStream_Release(pStream);
    }
}

/*************************************************************************
 * OLECONVERT_CreateOle10NativeStream [Internal] 
 *
 * Creates the "\001Ole10Native" Stream (should contain a BMP)
 *
 * PARAMS
 *     pStorage     [I] Dest storage to create the stream in
 *     pData        [I] Ole10 Native Data (ex. bmp)
 *     dwDataLength [I] Size of the Ole10 Native Data
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertOLESTREAMToIStorage only.
 *
 *     Might need to verify the data and return appropriate error message
 *     
 */
void OLECONVERT_CreateOle10NativeStream(LPSTORAGE pStorage, BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;
    WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};
    
    /* Create the Ole10Native Stream */
    hRes = IStorage_CreateStream(pStorage, wstrStreamName, 
        STGM_CREATE | STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );

    if(hRes == S_OK)
    {
        /* Write info to stream */
        hRes = IStream_Write(pStream, &dwDataLength, sizeof(dwDataLength), NULL);
        hRes = IStream_Write(pStream, pData, dwDataLength, NULL);
        IStream_Release(pStream);
    }

}

/*************************************************************************
 * OLECONVERT_GetOLE10ProgID [Internal] 
 *
 * Finds the ProgID (or OleTypeID) from the IStorage
 *
 * PARAMS
 *     pStorage        [I] The Src IStorage to get the ProgID
 *     strProgID       [I] the ProgID string to get
 *     dwSize          [I] the size of the string
 *
 * RETURNS
 *     Success:  S_OK
 *     Failure:  REGDB_E_CLASSNOTREG if cannot reconstruct the stream
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 *     
 */
HRESULT OLECONVERT_GetOLE10ProgID(LPSTORAGE pStorage, char *strProgID, DWORD *dwSize)
{
    HRESULT hRes;
    IStream *pStream;
    LARGE_INTEGER iSeekPos;
    OLECONVERT_ISTORAGE_COMPOBJ CompObj;
    WCHAR wstrStreamName[] = {1,'C', 'o', 'm', 'p', 'O', 'b', 'j', 0};

    /* Open the CompObj Stream */
    hRes = IStorage_OpenStream(pStorage, wstrStreamName, NULL,  
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {

        /*Get the OleType from the CompObj Stream */
        iSeekPos.u.LowPart = sizeof(CompObj.byUnknown1) + sizeof(CompObj.clsid);
        iSeekPos.u.HighPart = 0;

        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_SET, NULL);
        IStream_Read(pStream, &CompObj.dwCLSIDNameLength, sizeof(CompObj.dwCLSIDNameLength), NULL);
        iSeekPos.u.LowPart = CompObj.dwCLSIDNameLength;
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_CUR , NULL);
        IStream_Read(pStream, &CompObj.dwOleTypeNameLength, sizeof(CompObj.dwOleTypeNameLength), NULL);
        iSeekPos.u.LowPart = CompObj.dwOleTypeNameLength;
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_CUR , NULL);

        IStream_Read(pStream, dwSize, sizeof(*dwSize), NULL);
        if(*dwSize > 0)
        {
            IStream_Read(pStream, strProgID, *dwSize, NULL);
        }
        IStream_Release(pStream);
    }
    else
    {
        STATSTG stat;
        LPOLESTR wstrProgID;

        /* Get the OleType from the registry */
        REFCLSID clsid = &(stat.clsid);
        IStorage_Stat(pStorage, &stat, STATFLAG_NONAME);
        hRes = ProgIDFromCLSID(clsid, &wstrProgID);
        if(hRes == S_OK)
        {
            *dwSize = WideCharToMultiByte(CP_ACP, 0, wstrProgID, -1, strProgID, *dwSize, NULL, FALSE);
        }
 
    }
    return hRes;
}

/*************************************************************************
 * OLECONVERT_GetOle10PresData [Internal] 
 *
 * Converts IStorage "/001Ole10Native" stream to a OLE10 Stream
 *
 * PARAMS
 *     pStorage     [I] Src IStroage
 *     pOleStream   [I] Dest OleStream Mem Struct
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *
 *     Memory allocated for pData must be freed by the caller
 *      
 *     
 */
void OLECONVERT_GetOle10PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{

    HRESULT hRes;
    IStream *pStream;
    WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};

    /* Initialize Default data for OLESTREAM */
    pOleStreamData[0].dwOleID = OLESTREAM_ID;
    pOleStreamData[0].dwTypeID = 2;
    pOleStreamData[1].dwOleID = OLESTREAM_ID;
    pOleStreamData[1].dwTypeID = 0;
    pOleStreamData[0].dwMetaFileWidth = 0;
    pOleStreamData[0].dwMetaFileHeight = 0;
    pOleStreamData[0].pData = NULL;
    pOleStreamData[1].pData = NULL;

    /* Open Ole10Native Stream */
    hRes = IStorage_OpenStream(pStorage, wstrStreamName, NULL,  
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {

        /* Read Size and Data */
        IStream_Read(pStream, &(pOleStreamData->dwDataLength), sizeof(pOleStreamData->dwDataLength), NULL);
        if(pOleStreamData->dwDataLength > 0)
        {
            pOleStreamData->pData = (LPSTR) HeapAlloc(GetProcessHeap(),0,pOleStreamData->dwDataLength);
            IStream_Read(pStream, pOleStreamData->pData, pOleStreamData->dwDataLength, NULL);
        }
        IStream_Release(pStream);
    }

}


/*************************************************************************
 * OLECONVERT_GetOle20PresData[Internal] 
 *
 * Converts IStorage "/002OlePres000" stream to a OLE10 Stream
 *
 * PARAMS
 *     pStorage         [I] Src IStroage
 *     pOleStreamData   [I] Dest OleStream Mem Struct
 *
 * RETURNS
 *     Nothing
 *
 * NOTES
 *     This function is used by OleConvertIStorageToOLESTREAM only.
 *     
 *     Memory allocated for pData must be freed by the caller
 */
void OLECONVERT_GetOle20PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{
    HRESULT hRes;
    IStream *pStream;
    OLECONVERT_ISTORAGE_OLEPRES olePress;
    WCHAR wstrStreamName[] = {2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};

    /* Initialize Default data for OLESTREAM */
    pOleStreamData[0].dwOleID = OLESTREAM_ID;
    pOleStreamData[0].dwTypeID = 2;
    pOleStreamData[0].dwMetaFileWidth = 0;
    pOleStreamData[0].dwMetaFileHeight = 0;
    pOleStreamData[0].dwDataLength = OLECONVERT_WriteOLE20ToBuffer(pStorage, &(pOleStreamData[0].pData));
    pOleStreamData[1].dwOleID = OLESTREAM_ID;
    pOleStreamData[1].dwTypeID = 0;
    pOleStreamData[1].dwOleTypeNameLength = 0;
    pOleStreamData[1].strOleTypeName[0] = 0;
    pOleStreamData[1].dwMetaFileWidth = 0;
    pOleStreamData[1].dwMetaFileHeight = 0;
    pOleStreamData[1].pData = NULL;
    pOleStreamData[1].dwDataLength = 0;


    /* Open OlePress000 stream */
    hRes = IStorage_OpenStream(pStorage, wstrStreamName, NULL,  
        STGM_READ  | STGM_SHARE_EXCLUSIVE, 0, &pStream );
    if(hRes == S_OK)
    {
        LARGE_INTEGER iSeekPos;
        METAFILEPICT16 MetaFilePict;
        char strMetafilePictName[] = "METAFILEPICT";

        /* Set the TypeID for a Metafile */
        pOleStreamData[1].dwTypeID = 5;

        /* Set the OleTypeName to Metafile */
        pOleStreamData[1].dwOleTypeNameLength = strlen(strMetafilePictName) +1;
        strcpy(pOleStreamData[1].strOleTypeName, strMetafilePictName);

        iSeekPos.u.HighPart = 0;
        iSeekPos.u.LowPart = sizeof(olePress.byUnknown1);

        /* Get Presentation Data */
        IStream_Seek(pStream, iSeekPos, STREAM_SEEK_SET, NULL);
        IStream_Read(pStream, &(olePress.dwExtentX), sizeof(olePress.dwExtentX), NULL);
        IStream_Read(pStream, &(olePress.dwExtentY), sizeof(olePress.dwExtentY), NULL);
        IStream_Read(pStream, &(olePress.dwSize), sizeof(olePress.dwSize), NULL);

        /*Set width and Height */
        pOleStreamData[1].dwMetaFileWidth = olePress.dwExtentX;
        pOleStreamData[1].dwMetaFileHeight = -olePress.dwExtentY;
        if(olePress.dwSize > 0)
        {
            /* Set Length */
            pOleStreamData[1].dwDataLength  = olePress.dwSize + sizeof(METAFILEPICT16);

            /* Set MetaFilePict struct */
            MetaFilePict.mm = 8;
            MetaFilePict.xExt = olePress.dwExtentX;
            MetaFilePict.yExt = olePress.dwExtentY;
            MetaFilePict.hMF = 0;

            /* Get Metafile Data */
            pOleStreamData[1].pData = (BYTE *) HeapAlloc(GetProcessHeap(),0,pOleStreamData[1].dwDataLength);
            memcpy(pOleStreamData[1].pData, &MetaFilePict, sizeof(MetaFilePict));
            IStream_Read(pStream, &(pOleStreamData[1].pData[sizeof(MetaFilePict)]), pOleStreamData[1].dwDataLength-sizeof(METAFILEPICT16), NULL);
        }
        IStream_Release(pStream);
    }
}

/*************************************************************************
 * OleConvertOLESTREAMToIStorage [OLE32.87] 
 *
 * Read info on MSDN
 *
 * TODO
 *      DVTARGETDEVICE paramenter is not handled
 *      Still unsure of some mem fields for OLE 10 Stream
 *      Still some unknowns for the IStorage: "\002OlePres000", "\001CompObj",
 *      and "\001OLE" streams
 *     
 */
HRESULT WINAPI OleConvertOLESTREAMToIStorage (
    LPOLESTREAM pOleStream, 
    LPSTORAGE pstg, 
    const DVTARGETDEVICE* ptd)
{
    int i;
    HRESULT hRes=S_OK;
    OLECONVERT_OLESTREAM_DATA pOleStreamData[2];

    memset(pOleStreamData, 0, sizeof(pOleStreamData));

    if(ptd != NULL)
    {
        Print(MIN_TRACE, ("DVTARGETDEVICE is not NULL, unhandled parameter\n"));
    }

    if(pstg == NULL || pOleStream == NULL)
    {
        hRes = E_INVALIDARG;
    }

    if(hRes == S_OK)
    {
        /* Load the OLESTREAM to Memory */
        hRes = OLECONVERT_LoadOLE10(pOleStream, &pOleStreamData[0], TRUE);
    }

    if(hRes == S_OK)
    {
        /* Load the OLESTREAM to Memory (part 2)*/
        hRes = OLECONVERT_LoadOLE10(pOleStream, &pOleStreamData[1], FALSE);
    }

    if(hRes == S_OK)
    {

        if(pOleStreamData[0].dwDataLength > sizeof(STORAGE_magic))
        {
            /* Do we have the IStorage Data in the OLESTREAM */
            if(memcmp(pOleStreamData[0].pData, STORAGE_magic, sizeof(STORAGE_magic)) ==0)
            {
                OLECONVERT_GetOLE20FromOLE10(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
                OLECONVERT_CreateOlePresStream(pstg, pOleStreamData[1].dwMetaFileWidth, pOleStreamData[1].dwMetaFileHeight, pOleStreamData[1].pData, pOleStreamData[1].dwDataLength);
            }
            else
            {
                /* It must be an original OLE 1.0 source */
                OLECONVERT_CreateOle10NativeStream(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
            }
        }
        else
        {
            /* It must be an original OLE 1.0 source */
            OLECONVERT_CreateOle10NativeStream(pstg, pOleStreamData[0].pData, pOleStreamData[0].dwDataLength);
        }

        /* Create CompObj Stream if necessary */
        hRes = OLECONVERT_CreateCompObjStream(pstg, pOleStreamData[0].strOleTypeName);
        if(hRes == S_OK)
        {
            /*Create the Ole Stream if necessary */
            OLECONVERT_CreateOleStream(pstg);
        }
    }


    /* Free allocated memory */
    for(i=0; i < 2; i++)
    {
        if(pOleStreamData[i].pData != NULL)
        {
            HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
        }
        if(pOleStreamData[i].pstrOleObjFileName != NULL)
	{
        	HeapFree(GetProcessHeap(),0,pOleStreamData[i].pstrOleObjFileName);
        	pOleStreamData[i].pstrOleObjFileName = NULL;
	}
    }
    return hRes;
}

/*************************************************************************
 * OleConvertIStorageToOLESTREAM [OLE32.85]
 *
 * Read info on MSDN
 *
 * Read info on MSDN
 *
 * TODO
 *      Still unsure of some mem fields for OLE 10 Stream
 *      Still some unknowns for the IStorage: "\002OlePres000", "\001CompObj",
 *      and "\001OLE" streams.
 *     
 */
HRESULT WINAPI OleConvertIStorageToOLESTREAM (
    LPSTORAGE pstg, 
    LPOLESTREAM pOleStream)
{
    int i;
    HRESULT hRes = S_OK;
    IStream *pStream;
    OLECONVERT_OLESTREAM_DATA pOleStreamData[2];
    WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};


    memset(pOleStreamData, 0, sizeof(pOleStreamData));

    if(pstg == NULL || pOleStream == NULL)
    {
        hRes = E_INVALIDARG;
    }
    if(hRes == S_OK)
    {
        /* Get the ProgID */
        pOleStreamData[0].dwOleTypeNameLength = OLESTREAM_MAX_STR_LEN;
        hRes = OLECONVERT_GetOLE10ProgID(pstg, pOleStreamData[0].strOleTypeName, &(pOleStreamData[0].dwOleTypeNameLength));
    }
    if(hRes == S_OK)
    {
        /*Was it originaly Ole10 */
        hRes = IStorage_OpenStream(pstg, wstrStreamName, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);    
        if(hRes == S_OK)
        {
            IStream_Release(pStream);
            /*Get Presentation Data for Ole10Native */
            OLECONVERT_GetOle10PresData(pstg, pOleStreamData);
        }
        else
        {
            /*Get Presentation Data (OLE20)*/
            OLECONVERT_GetOle20PresData(pstg, pOleStreamData);
        }

        /* Save OLESTREAM */
        hRes = OLECONVERT_SaveOLE10(&(pOleStreamData[0]), pOleStream);
        if(hRes == S_OK)
        {
            hRes = OLECONVERT_SaveOLE10(&(pOleStreamData[1]), pOleStream);
        }

    }

    /* Free allocated memory */
    for(i=0; i < 2; i++)
    {
        if(pOleStreamData[i].pData != NULL)
        {
            HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
        }
    }

    return hRes;
}

/***********************************************************************
 *		GetConvertStg (OLE32.68)
 */
HRESULT WINAPI GetConvertStg(LPGUID guid) {
    UNIMPLEMENTED;
    return E_FAIL;
}
