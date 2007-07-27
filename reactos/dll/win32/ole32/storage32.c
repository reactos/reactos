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
 * Copyright 2005 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *  The compound file implementation of IStorage used for create
 *  and manage substorages and streams within a storage object
 *  residing in a compound file object.
 *
 * MSDN
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/stg/stg/istorage_compound_file_implementation.asp
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "storage32.h"
#include "ole2.h"      /* For Write/ReadClassStm */

#include "winreg.h"
#include "wine/wingdi16.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/* Used for OleConvertIStorageToOLESTREAM and OleConvertOLESTREAMToIStorage */
#define OLESTREAM_ID 0x501
#define OLESTREAM_MAX_STR_LEN 255

/*
 * These are signatures to detect the type of Document file.
 */
static const BYTE STORAGE_magic[8]    ={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};
static const BYTE STORAGE_oldmagic[8] ={0xd0,0xcf,0x11,0xe0,0x0e,0x11,0xfc,0x0d};

static const char rootPropertyName[] = "Root Entry";

/****************************************************************************
 * Storage32InternalImpl definitions.
 *
 * Definition of the implementation structure for the IStorage32 interface.
 * This one implements the IStorage32 interface for storage that are
 * inside another storage.
 */
struct StorageInternalImpl
{
  struct StorageBaseImpl base;
  /*
   * There is no specific data for this class.
   */
};
typedef struct StorageInternalImpl StorageInternalImpl;

/* Method definitions for the Storage32InternalImpl class. */
static StorageInternalImpl* StorageInternalImpl_Construct(StorageImpl* ancestorStorage,
                                                          DWORD openFlags, ULONG rootTropertyIndex);
static void StorageImpl_Destroy(StorageBaseImpl* iface);
static BOOL StorageImpl_ReadBigBlock(StorageImpl* This, ULONG blockIndex, void* buffer);
static BOOL StorageImpl_WriteBigBlock(StorageImpl* This, ULONG blockIndex, const void* buffer);
static void StorageImpl_SetNextBlockInChain(StorageImpl* This, ULONG blockIndex, ULONG nextBlock);
static HRESULT StorageImpl_LoadFileHeader(StorageImpl* This);
static void StorageImpl_SaveFileHeader(StorageImpl* This);

static void Storage32Impl_AddBlockDepot(StorageImpl* This, ULONG blockIndex);
static ULONG Storage32Impl_AddExtBlockDepot(StorageImpl* This);
static ULONG Storage32Impl_GetNextExtendedBlock(StorageImpl* This, ULONG blockIndex);
static ULONG Storage32Impl_GetExtDepotBlock(StorageImpl* This, ULONG depotIndex);
static void Storage32Impl_SetExtDepotBlock(StorageImpl* This, ULONG depotIndex, ULONG blockIndex);

static ULONG BlockChainStream_GetHeadOfChain(BlockChainStream* This);
static ULARGE_INTEGER BlockChainStream_GetSize(BlockChainStream* This);
static ULONG BlockChainStream_GetCount(BlockChainStream* This);

static ULARGE_INTEGER SmallBlockChainStream_GetSize(SmallBlockChainStream* This);
static BOOL StorageImpl_WriteDWordToBigBlock( StorageImpl* This,
    ULONG blockIndex, ULONG offset, DWORD value);
static BOOL StorageImpl_ReadDWordFromBigBlock( StorageImpl*  This,
    ULONG blockIndex, ULONG offset, DWORD* value);

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
    const OLECHAR *newProperty,
    const OLECHAR *currentProperty);


/***********************************************************************
 * Declaration of miscellaneous functions...
 */
static HRESULT validateSTGM(DWORD stgmValue);

static DWORD GetShareModeFromSTGM(DWORD stgm);
static DWORD GetAccessModeFromSTGM(DWORD stgm);
static DWORD GetCreationModeFromSTGM(DWORD stgm);

extern const IPropertySetStorageVtbl IPropertySetStorage_Vtbl;


/****************************************************************************
 * IEnumSTATSTGImpl definitions.
 *
 * Definition of the implementation structure for the IEnumSTATSTGImpl interface.
 * This class allows iterating through the content of a storage and to find
 * specific items inside it.
 */
struct IEnumSTATSTGImpl
{
  const IEnumSTATSTGVtbl *lpVtbl;    /* Needs to be the first item in the struct
				* since we want to cast this in an IEnumSTATSTG pointer */

  LONG           ref;                   /* Reference count */
  StorageImpl*   parentStorage;         /* Reference to the parent storage */
  ULONG          firstPropertyNode;     /* Index of the root of the storage to enumerate */

  /*
   * The current implementation of the IEnumSTATSTGImpl class uses a stack
   * to walk the property sets to get the content of a storage. This stack
   * is implemented by the following 3 data members
   */
  ULONG          stackSize;
  ULONG          stackMaxSize;
  ULONG*         stackToVisit;

#define ENUMSTATSGT_SIZE_INCREMENT 10
};


static IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(StorageImpl* This, ULONG firstPropertyNode);
static void IEnumSTATSTGImpl_Destroy(IEnumSTATSTGImpl* This);
static void IEnumSTATSTGImpl_PushSearchNode(IEnumSTATSTGImpl* This, ULONG nodeToPush);
static ULONG IEnumSTATSTGImpl_PopSearchNode(IEnumSTATSTGImpl* This, BOOL remove);
static ULONG IEnumSTATSTGImpl_FindProperty(IEnumSTATSTGImpl* This, const OLECHAR* lpszPropName,
                                           StgProperty* buffer);
static INT IEnumSTATSTGImpl_FindParentProperty(IEnumSTATSTGImpl *This, ULONG childProperty,
                                               StgProperty *currentProperty, ULONG *propertyId);

/************************************************************************
** Block Functions
*/

static ULONG BLOCK_GetBigBlockOffset(ULONG index)
{
    if (index == 0xffffffff)
        index = 0;
    else
        index ++;

    return index * BIG_BLOCK_SIZE;
}

/************************************************************************
** Storage32BaseImpl implementatiion
*/
static HRESULT StorageImpl_ReadAt(StorageImpl* This,
  ULARGE_INTEGER offset,
  void*          buffer,
  ULONG          size,
  ULONG*         bytesRead)
{
    return BIGBLOCKFILE_ReadAt(This->bigBlockFile,offset,buffer,size,bytesRead);
}

static HRESULT StorageImpl_WriteAt(StorageImpl* This,
  ULARGE_INTEGER offset,
  const void*    buffer,
  const ULONG    size,
  ULONG*         bytesWritten)
{
    return BIGBLOCKFILE_WriteAt(This->bigBlockFile,offset,buffer,size,bytesWritten);
}

/************************************************************************
 * Storage32BaseImpl_QueryInterface (IUnknown)
 *
 * This method implements the common QueryInterface for all IStorage32
 * implementations contained in this file.
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI StorageBaseImpl_QueryInterface(
  IStorage*        iface,
  REFIID             riid,
  void**             ppvObject)
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
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
  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_IStorage, riid))
  {
    *ppvObject = (IStorage*)This;
  }
  else if (IsEqualGUID(&IID_IPropertySetStorage, riid))
  {
    *ppvObject = (IStorage*)&This->pssVtbl;
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
  IStorage_AddRef(iface);

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
static ULONG WINAPI StorageBaseImpl_AddRef(
            IStorage* iface)
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p) AddRef to %d\n", This, ref);

  return ref;
}

/************************************************************************
 * Storage32BaseImpl_Release (IUnknown)
 *
 * This method implements the common Release for all IStorage32
 * implementations contained in this file.
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI StorageBaseImpl_Release(
      IStorage* iface)
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  /*
   * Decrease the reference count on this object.
   */
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p) ReleaseRef to %d\n", This, ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (ref == 0)
  {
    /*
     * Since we are using a system of base-classes, we want to call the
     * destructor of the appropriate derived class. To do this, we are
     * using virtual functions to implement the destructor.
     */
    This->v_destructor(This);
  }

  return ref;
}

/************************************************************************
 * Storage32BaseImpl_OpenStream (IStorage)
 *
 * This method will open the specified stream object from the current storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_OpenStream(
  IStorage*        iface,
  const OLECHAR*   pwcsName,  /* [string][in] */
  void*            reserved1, /* [unique][in] */
  DWORD            grfMode,   /* [in]  */
  DWORD            reserved2, /* [in]  */
  IStream**        ppstm)     /* [out] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  IEnumSTATSTGImpl* propertyEnumeration;
  StgStreamImpl*    newStream;
  StgProperty       currentProperty;
  ULONG             foundPropertyIndex;
  HRESULT           res = STG_E_UNKNOWN;

  TRACE("(%p, %s, %p, %x, %d, %p)\n",
	iface, debugstr_w(pwcsName), reserved1, grfMode, reserved2, ppstm);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (pwcsName==NULL) || (ppstm==0) )
  {
    res = E_INVALIDARG;
    goto end;
  }

  /*
   * Initialize the out parameter
   */
  *ppstm = NULL;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ) ||
       STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE)
  {
    res = STG_E_INVALIDFLAG;
    goto end;
  }

  /*
   * As documented.
   */
  if ( (grfMode & STGM_DELETEONRELEASE) || (grfMode & STGM_TRANSACTED) )
  {
    res = STG_E_INVALIDFUNCTION;
    goto end;
  }

  /*
   * Check that we're compatible with the parent's storage mode, but
   * only if we are not in transacted mode
   */
  if(!(This->ancestorStorage->base.openFlags & STGM_TRANSACTED)) {
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
    {
      res = STG_E_ACCESSDENIED;
      goto end;
    }
  }

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
      IStream_AddRef(*ppstm);

      res = S_OK;
      goto end;
    }

    res = E_OUTOFMEMORY;
    goto end;
  }

  res = STG_E_FILENOTFOUND;

end:
  if (res == S_OK)
    TRACE("<-- IStream %p\n", *ppstm);
  TRACE("<-- %08x\n", res);
  return res;
}

/************************************************************************
 * Storage32BaseImpl_OpenStorage (IStorage)
 *
 * This method will open a new storage object from the current storage.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_OpenStorage(
  IStorage*        iface,
  const OLECHAR*   pwcsName,      /* [string][unique][in] */
  IStorage*        pstgPriority,  /* [unique][in] */
  DWORD            grfMode,       /* [in] */
  SNB              snbExclude,    /* [unique][in] */
  DWORD            reserved,      /* [in] */
  IStorage**       ppstg)         /* [out] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  StorageInternalImpl* newStorage;
  IEnumSTATSTGImpl*      propertyEnumeration;
  StgProperty            currentProperty;
  ULONG                  foundPropertyIndex;
  HRESULT                res = STG_E_UNKNOWN;

  TRACE("(%p, %s, %p, %x, %p, %d, %p)\n",
	iface, debugstr_w(pwcsName), pstgPriority,
	grfMode, snbExclude, reserved, ppstg);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (pwcsName==NULL) || (ppstg==0) )
  {
    res = E_INVALIDARG;
    goto end;
  }

  /* as documented */
  if (snbExclude != NULL)
  {
    res = STG_E_INVALIDPARAMETER;
    goto end;
  }

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
  {
    res = STG_E_INVALIDFLAG;
    goto end;
  }

  /*
   * As documented.
   */
  if ( STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE ||
        (grfMode & STGM_DELETEONRELEASE) ||
        (grfMode & STGM_PRIORITY) )
  {
    res = STG_E_INVALIDFUNCTION;
    goto end;
  }

  /*
   * Check that we're compatible with the parent's storage mode,
   * but only if we are not transacted
   */
  if(!(This->ancestorStorage->base.openFlags & STGM_TRANSACTED)) {
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
    {
      res = STG_E_ACCESSDENIED;
      goto end;
    }
  }

  /*
   * Initialize the out parameter
   */
  *ppstg = NULL;

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
                   grfMode,
                   foundPropertyIndex);

    if (newStorage != 0)
    {
      *ppstg = (IStorage*)newStorage;

      /*
       * Since we are returning a pointer to the interface,
       * we have to nail down the reference.
       */
      StorageBaseImpl_AddRef(*ppstg);

      res = S_OK;
      goto end;
    }

    res = STG_E_INSUFFICIENTMEMORY;
    goto end;
  }

  res = STG_E_FILENOTFOUND;

end:
  TRACE("<-- %08x\n", res);
  return res;
}

/************************************************************************
 * Storage32BaseImpl_EnumElements (IStorage)
 *
 * This method will create an enumerator object that can be used to
 * retrieve informatino about all the properties in the storage object.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageBaseImpl_EnumElements(
  IStorage*       iface,
  DWORD           reserved1, /* [in] */
  void*           reserved2, /* [size_is][unique][in] */
  DWORD           reserved3, /* [in] */
  IEnumSTATSTG**  ppenum)    /* [out] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  IEnumSTATSTGImpl* newEnum;

  TRACE("(%p, %d, %p, %d, %p)\n",
	iface, reserved1, reserved2, reserved3, ppenum);

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
    IEnumSTATSTG_AddRef(*ppenum);

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
static HRESULT WINAPI StorageBaseImpl_Stat(
  IStorage*        iface,
  STATSTG*         pstatstg,     /* [out] */
  DWORD            grfStatFlag)  /* [in] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  StgProperty    curProperty;
  BOOL           readSuccessful;
  HRESULT        res = STG_E_UNKNOWN;

  TRACE("(%p, %p, %x)\n",
	iface, pstatstg, grfStatFlag);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (pstatstg==0))
  {
    res = E_INVALIDARG;
    goto end;
  }

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

    pstatstg->grfMode = This->openFlags;
    pstatstg->grfStateBits = This->stateBits;

    res = S_OK;
    goto end;
  }

  res = E_FAIL;

end:
  if (res == S_OK)
  {
    TRACE("<-- STATSTG: pwcsName: %s, type: %d, cbSize.Low/High: %d/%d, grfMode: %08x, grfLocksSupported: %d, grfStateBits: %08x\n", debugstr_w(pstatstg->pwcsName), pstatstg->type, pstatstg->cbSize.u.LowPart, pstatstg->cbSize.u.HighPart, pstatstg->grfMode, pstatstg->grfLocksSupported, pstatstg->grfStateBits);
  }
  TRACE("<-- %08x\n", res);
  return res;
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
static HRESULT WINAPI StorageBaseImpl_RenameElement(
            IStorage*        iface,
            const OLECHAR*   pwcsOldName,  /* [in] */
            const OLECHAR*   pwcsNewName)  /* [in] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  IEnumSTATSTGImpl* propertyEnumeration;
  StgProperty       currentProperty;
  ULONG             foundPropertyIndex;

  TRACE("(%p, %s, %s)\n",
	iface, debugstr_w(pwcsOldName), debugstr_w(pwcsNewName));

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

  IEnumSTATSTG_Reset((IEnumSTATSTG*)propertyEnumeration);

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

    strcpyW(renamedProperty.name, pwcsNewName);

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
     * now, before Destroying the old property we must zero its dirProperty
     * otherwise the DestroyProperty below will zap it all and we do not want
     * this to happen.
     * Also, we fake that the old property is a storage so the DestroyProperty
     * will not do a SetSize(0) on the stream data.
     *
     * This means that we need to tweak the StgProperty if it is a stream or a
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
     * the linking of its previous and next members...
     */
    IStorage_DestroyElement((IStorage*)This->ancestorStorage, pwcsOldName);

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
static HRESULT WINAPI StorageBaseImpl_CreateStream(
            IStorage*        iface,
            const OLECHAR*   pwcsName,  /* [string][in] */
            DWORD            grfMode,   /* [in] */
            DWORD            reserved1, /* [in] */
            DWORD            reserved2, /* [in] */
            IStream**        ppstm)     /* [out] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  IEnumSTATSTGImpl* propertyEnumeration;
  StgStreamImpl*    newStream;
  StgProperty       currentProperty, newStreamProperty;
  ULONG             foundPropertyIndex, newPropertyIndex;

  TRACE("(%p, %s, %x, %d, %d, %p)\n",
	iface, debugstr_w(pwcsName), grfMode,
	reserved1, reserved2, ppstm);

  /*
   * Validate parameters
   */
  if (ppstm == 0)
    return STG_E_INVALIDPOINTER;

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  if (reserved1 || reserved2)
    return STG_E_INVALIDPARAMETER;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    return STG_E_INVALIDFLAG;

  if (STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE) 
    return STG_E_INVALIDFLAG;

  /*
   * As documented.
   */
  if ((grfMode & STGM_DELETEONRELEASE) ||
      (grfMode & STGM_TRANSACTED))
    return STG_E_INVALIDFUNCTION;

  /*
   * Check that we're compatible with the parent's storage mode
   * if not in transacted mode
   */
  if(!(This->ancestorStorage->base.openFlags & STGM_TRANSACTED)) {
    if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->openFlags ) )
      return STG_E_ACCESSDENIED;
  }

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
    if (STGM_CREATE_MODE(grfMode) == STGM_CREATE)
    {
      IStorage_DestroyElement(iface, pwcsName);
    }
    else
      return STG_E_FILEALREADYEXISTS;
  }
  else if (STGM_ACCESS_MODE(This->openFlags) == STGM_READ)
  {
    WARN("read-only storage\n");
    return STG_E_ACCESSDENIED;
  }

  /*
   * memset the empty property
   */
  memset(&newStreamProperty, 0, sizeof(StgProperty));

  newStreamProperty.sizeOfNameString =
      ( lstrlenW(pwcsName)+1 ) * sizeof(WCHAR);

  if (newStreamProperty.sizeOfNameString > PROPERTY_NAME_BUFFER_LEN)
    return STG_E_INVALIDNAME;

  strcpyW(newStreamProperty.name, pwcsName);

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
    IStream_AddRef(*ppstm);
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
static HRESULT WINAPI StorageBaseImpl_SetClass(
  IStorage*        iface,
  REFCLSID         clsid) /* [in] */
{
  StorageBaseImpl *This = (StorageBaseImpl *)iface;
  HRESULT hRes = E_FAIL;
  StgProperty curProperty;
  BOOL success;

  TRACE("(%p, %p)\n", iface, clsid);

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
static HRESULT WINAPI StorageImpl_CreateStorage(
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

  TRACE("(%p, %s, %x, %d, %d, %p)\n",
	iface, debugstr_w(pwcsName), grfMode,
	reserved1, reserved2, ppstg);

  /*
   * Validate parameters
   */
  if (ppstg == 0)
    return STG_E_INVALIDPOINTER;

  if (pwcsName == 0)
    return STG_E_INVALIDNAME;

  /*
   * Initialize the out parameter
   */
  *ppstg = NULL;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ) ||
       (grfMode & STGM_DELETEONRELEASE) )
  {
    WARN("bad grfMode: 0x%x\n", grfMode);
    return STG_E_INVALIDFLAG;
  }

  /*
   * Check that we're compatible with the parent's storage mode
   */
  if ( STGM_ACCESS_MODE( grfMode ) > STGM_ACCESS_MODE( This->base.openFlags ) )
  {
    WARN("access denied\n");
    return STG_E_ACCESSDENIED;
  }

  /*
   * Create a property enumeration and search the properties
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct( This->base.ancestorStorage,
                                                    This->base.rootPropertySetIndex);

  foundPropertyIndex = IEnumSTATSTGImpl_FindProperty(propertyEnumeration,
                                                     pwcsName,
                                                     &currentProperty);
  IEnumSTATSTGImpl_Destroy(propertyEnumeration);

  if (foundPropertyIndex != PROPERTY_NULL)
  {
    /*
     * An element with this name already exists
     */
    if (STGM_CREATE_MODE(grfMode) == STGM_CREATE)
      IStorage_DestroyElement(iface, pwcsName);
    else
    {
      WARN("file already exists\n");
      return STG_E_FILEALREADYEXISTS;
    }
  }
  else if (STGM_ACCESS_MODE(This->base.openFlags) == STGM_READ)
  {
    WARN("read-only storage\n");
    return STG_E_ACCESSDENIED;
  }

  /*
   * memset the empty property
   */
  memset(&newProperty, 0, sizeof(StgProperty));

  newProperty.sizeOfNameString = (lstrlenW(pwcsName)+1)*sizeof(WCHAR);

  if (newProperty.sizeOfNameString > PROPERTY_NAME_BUFFER_LEN)
  {
    FIXME("name too long\n");
    return STG_E_INVALIDNAME;
  }

  strcpyW(newProperty.name, pwcsName);

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
  newPropertyIndex = getFreeProperty(This->base.ancestorStorage);

  /*
   * Save the new property into the new property spot
   */
  StorageImpl_WriteProperty(
    This->base.ancestorStorage,
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
         (const OLECHAR*)pwcsName,
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
    readSuccessful = StorageImpl_ReadProperty(storage->base.ancestorStorage,
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
                   storage->base.ancestorStorage->rootBlockChain)+1;

    /*
     * initialize the size used by the property stream
     */
    newSize.u.HighPart = 0;
    newSize.u.LowPart  = storage->bigBlockSize * blockCount;

    /*
     * add a property block to the property chain
     */
    BlockChainStream_SetSize(storage->base.ancestorStorage->rootBlockChain, newSize);

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
        storage->base.ancestorStorage,
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
    const OLECHAR *newProperty,
    const OLECHAR *currentProperty)
{
  LONG diff      = lstrlenW(newProperty) - lstrlenW(currentProperty);

  if (diff == 0)
  {
    /*
     * We compare the string themselves only when they are of the same length
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
  StorageImpl_ReadProperty(storage->base.ancestorStorage,
                             storage->base.rootPropertySetIndex,
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
    StorageImpl_ReadProperty(storage->base.ancestorStorage,
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
          StorageImpl_ReadProperty(storage->base.ancestorStorage,
                                     previous,
                                     &currentProperty);
          current = previous;
        }
        else
        {
          currentProperty.previousProperty = newPropertyIndex;
          StorageImpl_WriteProperty(storage->base.ancestorStorage,
                                      current,
                                      &currentProperty);
          found = 1;
        }
      }
      else if (diff > 0)
      {
        if (next != PROPERTY_NULL)
        {
          StorageImpl_ReadProperty(storage->base.ancestorStorage,
                                     next,
                                     &currentProperty);
          current = next;
        }
        else
        {
          currentProperty.nextProperty = newPropertyIndex;
          StorageImpl_WriteProperty(storage->base.ancestorStorage,
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
     * The root storage is empty, link the new property to its dir property
     */
    currentProperty.dirProperty = newPropertyIndex;
    StorageImpl_WriteProperty(storage->base.ancestorStorage,
                                storage->base.rootPropertySetIndex,
                                &currentProperty);
  }
}


/*************************************************************************
 * CopyTo (IStorage)
 */
static HRESULT WINAPI StorageImpl_CopyTo(
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
    FIXME("Exclude option not implemented\n");

  TRACE("(%p, %d, %p, %p, %p)\n",
	iface, ciidExclude, rgiidExclude,
	snbExclude, pstgDest);

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
      WARN("unknown element type: %d\n", curElement.type);
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
static HRESULT WINAPI StorageImpl_MoveElementTo(
  IStorage*     iface,
  const OLECHAR *pwcsName,   /* [string][in] */
  IStorage      *pstgDest,   /* [unique][in] */
  const OLECHAR *pwcsNewName,/* [string][in] */
  DWORD           grfFlags)    /* [in] */
{
  FIXME("(%p %s %p %s %u): stub\n", iface,
         debugstr_w(pwcsName), pstgDest,
         debugstr_w(pwcsNewName), grfFlags);
  return E_NOTIMPL;
}

/*************************************************************************
 * Commit (IStorage)
 *
 * Ensures that any changes made to a storage object open in transacted mode
 * are reflected in the parent storage
 *
 * NOTES
 *  Wine doesn't implement transacted mode, which seems to be a basic
 *  optimization, so we can ignore this stub for now.
 */
static HRESULT WINAPI StorageImpl_Commit(
  IStorage*   iface,
  DWORD         grfCommitFlags)/* [in] */
{
  FIXME("(%p %d): stub\n", iface, grfCommitFlags);
  return S_OK;
}

/*************************************************************************
 * Revert (IStorage)
 *
 * Discard all changes that have been made since the last commit operation
 */
static HRESULT WINAPI StorageImpl_Revert(
  IStorage* iface)
{
  FIXME("(%p): stub\n", iface);
  return E_NOTIMPL;
}

/*************************************************************************
 * DestroyElement (IStorage)
 *
 * Strategy: This implementation is built this way for simplicity not for speed.
 *          I always delete the topmost element of the enumeration and adjust
 *          the deleted element pointer all the time.  This takes longer to
 *          do but allow to reinvoke DestroyElement whenever we encounter a
 *          storage object.  The optimisation resides in the usage of another
 *          enumeration strategy that would give all the leaves of a storage
 *          first. (postfix order)
 */
static HRESULT WINAPI StorageImpl_DestroyElement(
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
  ULONG             parentPropertyId = 0;

  TRACE("(%p, %s)\n",
	iface, debugstr_w(pwcsName));

  /*
   * Perform a sanity check on the parameters.
   */
  if (pwcsName==NULL)
    return STG_E_INVALIDPOINTER;

  /*
   * Create a property enumeration to search the property with the given name
   */
  propertyEnumeration = IEnumSTATSTGImpl_Construct(
    This->base.ancestorStorage,
    This->base.rootPropertySetIndex);

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
   * the parent is This. Otherwise, the parent is one of its sibling...
   */

  /*
   * First, read This's StgProperty..
   */
  res = StorageImpl_ReadProperty(
          This->base.ancestorStorage,
          This->base.rootPropertySetIndex,
          &parentProperty);

  assert(res);

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
    parentPropertyId = This->base.rootPropertySetIndex;
  }
  else
  {
    /*
     * Create a property enumeration to search the parent properties, and
     * delete it once done.
     */
    IEnumSTATSTGImpl* propertyEnumeration2;

    propertyEnumeration2 = IEnumSTATSTGImpl_Construct(
      This->base.ancestorStorage,
      This->base.rootPropertySetIndex);

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


/************************************************************************
 * StorageImpl_Stat (IStorage)
 *
 * This method will retrieve information about this storage object.
 *
 * See Windows documentation for more details on IStorage methods.
 */
static HRESULT WINAPI StorageImpl_Stat( IStorage* iface,
                                 STATSTG*  pstatstg,     /* [out] */
                                 DWORD     grfStatFlag)  /* [in] */
{
  StorageImpl* const This = (StorageImpl*)iface;
  HRESULT result = StorageBaseImpl_Stat( iface, pstatstg, grfStatFlag );

  if ( !FAILED(result) && ((grfStatFlag & STATFLAG_NONAME) == 0) && This->pwcsName )
  {
      CoTaskMemFree(pstatstg->pwcsName);
      pstatstg->pwcsName = CoTaskMemAlloc((lstrlenW(This->pwcsName)+1)*sizeof(WCHAR));
      strcpyW(pstatstg->pwcsName, This->pwcsName);
  }

  return result;
}

/******************************************************************************
 * Internal stream list handlers
 */

void StorageBaseImpl_AddStream(StorageBaseImpl * stg, StgStreamImpl * strm)
{
  TRACE("Stream added (stg=%p strm=%p)\n", stg, strm);
  list_add_tail(&stg->strmHead,&strm->StrmListEntry);
}

void StorageBaseImpl_RemoveStream(StorageBaseImpl * stg, StgStreamImpl * strm)
{
  TRACE("Stream removed (stg=%p strm=%p)\n", stg,strm);
  list_remove(&(strm->StrmListEntry));
}

static void StorageBaseImpl_DeleteAll(StorageBaseImpl * stg)
{
  struct list *cur, *cur2;
  StgStreamImpl *strm=NULL;

  LIST_FOR_EACH_SAFE(cur, cur2, &stg->strmHead) {
    strm = LIST_ENTRY(cur,StgStreamImpl,StrmListEntry);
    TRACE("Streams deleted (stg=%p strm=%p next=%p prev=%p)\n", stg,strm,cur->next,cur->prev);
    strm->parentStorage = NULL;
    list_remove(cur);
  }
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
        STGM_WRITE | STGM_SHARE_EXCLUSIVE,
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
   * Invalidate the property by zeroing its name member.
   */
  propertyToDelete.sizeOfNameString = 0;

  StorageImpl_WriteProperty(parentStorage->base.ancestorStorage,
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
   * Invalidate the property by zeroing its name member.
   */
  propertyToDelete.sizeOfNameString = 0;

  /*
   * Here we should re-read the property so we get the updated pointer
   * but since we are here to zap it, I don't do it...
   */
  StorageImpl_WriteProperty(
    parentStorage->base.ancestorStorage,
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
          storage->base.ancestorStorage,
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
         storage->base.ancestorStorage,
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
          This->base.ancestorStorage,
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
static HRESULT WINAPI StorageImpl_SetElementTimes(
  IStorage*     iface,
  const OLECHAR *pwcsName,/* [string][in] */
  const FILETIME  *pctime,  /* [in] */
  const FILETIME  *patime,  /* [in] */
  const FILETIME  *pmtime)  /* [in] */
{
  FIXME("(%s,...), stub!\n",debugstr_w(pwcsName));
  return S_OK;
}

/******************************************************************************
 * SetStateBits (IStorage)
 */
static HRESULT WINAPI StorageImpl_SetStateBits(
  IStorage*   iface,
  DWORD         grfStateBits,/* [in] */
  DWORD         grfMask)     /* [in] */
{
  StorageImpl* const This = (StorageImpl*)iface;
  This->base.stateBits = (This->base.stateBits & ~grfMask) | (grfStateBits & grfMask);
  return S_OK;
}

/*
 * Virtual function table for the IStorage32Impl class.
 */
static const IStorageVtbl Storage32Impl_Vtbl =
{
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
    StorageImpl_Stat
};

static HRESULT StorageImpl_Construct(
  StorageImpl* This,
  HANDLE       hFile,
  LPCOLESTR    pwcsName,
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
   * Initialize stream list
   */

  list_init(&This->base.strmHead);

  /*
   * Initialize the virtual function table.
   */
  This->base.lpVtbl = &Storage32Impl_Vtbl;
  This->base.pssVtbl = &IPropertySetStorage_Vtbl;
  This->base.v_destructor = &StorageImpl_Destroy;
  This->base.openFlags = (openFlags & ~STGM_CREATE);

  /*
   * This is the top-level storage so initialize the ancestor pointer
   * to this.
   */
  This->base.ancestorStorage = This;

  /*
   * Initialize the physical support of the storage.
   */
  This->hFile = hFile;

  /*
   * Store copy of file path.
   */
  if(pwcsName) {
      This->pwcsName = HeapAlloc(GetProcessHeap(), 0,
                                (lstrlenW(pwcsName)+1)*sizeof(WCHAR));
      if (!This->pwcsName)
         return STG_E_INSUFFICIENTMEMORY;
      strcpyW(This->pwcsName, pwcsName);
  }

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
    BYTE bigBlockBuffer[BIG_BLOCK_SIZE];

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
    memset(bigBlockBuffer, BLOCK_UNUSED, This->bigBlockSize);
    StorageUtl_WriteDWord(bigBlockBuffer, 0, BLOCK_SPECIAL);
    StorageUtl_WriteDWord(bigBlockBuffer, sizeof(ULONG), BLOCK_END_OF_CHAIN);
    StorageImpl_WriteBigBlock(This, 0, bigBlockBuffer);
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
  if(!(This->rootBlockChain =
       BlockChainStream_Construct(This, &This->rootStartBlock, PROPERTY_NULL)))
    return STG_E_READFAULT;

  if(!(This->smallBlockDepotChain =
       BlockChainStream_Construct(This, &This->smallBlockDepotStart,
				  PROPERTY_NULL)))
    return STG_E_READFAULT;

  /*
   * Write the root property (memory only)
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
    rootProp.sizeOfNameString = (strlenW(rootProp.name)+1) * sizeof(WCHAR);
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
   * Find the ID of the root in the property sets.
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
        This->base.rootPropertySetIndex = currentPropertyIndex;
      }
    }

    currentPropertyIndex++;

  } while (readSuccessful && (This->base.rootPropertySetIndex == PROPERTY_NULL) );

  if (!readSuccessful)
  {
    /* TODO CLEANUP */
    return STG_E_READFAULT;
  }

  /*
   * Create the block chain abstraction for the small block root chain.
   */
  if(!(This->smallBlockRootChain =
       BlockChainStream_Construct(This, NULL, This->base.rootPropertySetIndex)))
    return STG_E_READFAULT;

  return hr;
}

static void StorageImpl_Destroy(StorageBaseImpl* iface)
{
  StorageImpl *This = (StorageImpl*) iface;
  TRACE("(%p)\n", This);

  StorageBaseImpl_DeleteAll(&This->base);

  HeapFree(GetProcessHeap(), 0, This->pwcsName);

  BlockChainStream_Destroy(This->smallBlockRootChain);
  BlockChainStream_Destroy(This->rootBlockChain);
  BlockChainStream_Destroy(This->smallBlockDepotChain);

  BIGBLOCKFILE_Destructor(This->bigBlockFile);
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      Storage32Impl_GetNextFreeBigBlock
 *
 * Returns the index of the next free big block.
 * If the big block depot is filled, this method will enlarge it.
 *
 */
static ULONG StorageImpl_GetNextFreeBigBlock(
  StorageImpl* This)
{
  ULONG depotBlockIndexPos;
  BYTE depotBuffer[BIG_BLOCK_SIZE];
  BOOL success;
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

    success = StorageImpl_ReadBigBlock(This, depotBlockIndexPos, depotBuffer);

    if (success)
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
    }

    depotIndex++;
    depotBlockOffset = 0;
  }

  /*
   * make sure that the block physically exists before using it
   */
  BIGBLOCKFILE_EnsureExists(This->bigBlockFile, freeBlock);

  This->prevFreeBlock = freeBlock;

  return freeBlock;
}

/******************************************************************************
 *      Storage32Impl_AddBlockDepot
 *
 * This will create a depot block, essentially it is a block initialized
 * to BLOCK_UNUSEDs.
 */
static void Storage32Impl_AddBlockDepot(StorageImpl* This, ULONG blockIndex)
{
  BYTE blockBuffer[BIG_BLOCK_SIZE];

  /*
   * Initialize blocks as free
   */
  memset(blockBuffer, BLOCK_UNUSED, This->bigBlockSize);
  StorageImpl_WriteBigBlock(This, blockIndex, blockBuffer);
}

/******************************************************************************
 *      Storage32Impl_GetExtDepotBlock
 *
 * Returns the index of the block that corresponds to the specified depot
 * index. This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
static ULONG Storage32Impl_GetExtDepotBlock(StorageImpl* This, ULONG depotIndex)
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
    StorageImpl_ReadDWordFromBigBlock(This, extBlockIndex,
                        extBlockOffset * sizeof(ULONG), &blockIndex);

  return blockIndex;
}

/******************************************************************************
 *      Storage32Impl_SetExtDepotBlock
 *
 * Associates the specified block index to the specified depot index.
 * This method is only for depot indexes equal or greater than
 * COUNT_BBDEPOTINHEADER.
 */
static void Storage32Impl_SetExtDepotBlock(StorageImpl* This, ULONG depotIndex, ULONG blockIndex)
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
    StorageImpl_WriteDWordToBigBlock(This, extBlockIndex,
                        extBlockOffset * sizeof(ULONG),
                        blockIndex);
  }
}

/******************************************************************************
 *      Storage32Impl_AddExtBlockDepot
 *
 * Creates an extended depot block.
 */
static ULONG Storage32Impl_AddExtBlockDepot(StorageImpl* This)
{
  ULONG numExtBlocks           = This->extBigBlockDepotCount;
  ULONG nextExtBlock           = This->extBigBlockDepotStart;
  BYTE  depotBuffer[BIG_BLOCK_SIZE];
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
    unsigned int i;
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
    StorageImpl_WriteDWordToBigBlock(This, nextExtBlock, nextBlockOffset,
                                     index);
  }

  /*
   * Initialize this block.
   */
  memset(depotBuffer, BLOCK_UNUSED, This->bigBlockSize);
  StorageImpl_WriteBigBlock(This, index, depotBuffer);

  return index;
}

/******************************************************************************
 *      Storage32Impl_FreeBigBlock
 *
 * This method will flag the specified block as free in the big block depot.
 */
static void StorageImpl_FreeBigBlock(
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
 *          nextBlockIndex - receives the return value.
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
static HRESULT StorageImpl_GetNextBlockInChain(
  StorageImpl* This,
  ULONG        blockIndex,
  ULONG*       nextBlockIndex)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  BYTE depotBuffer[BIG_BLOCK_SIZE];
  BOOL success;
  ULONG depotBlockIndexPos;
  int index;

  *nextBlockIndex   = BLOCK_SPECIAL;

  if(depotBlockCount >= This->bigBlockDepotCount)
  {
    WARN("depotBlockCount %d, bigBlockDepotCount %d\n", depotBlockCount,
	 This->bigBlockDepotCount);
    return STG_E_READFAULT;
  }

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

    success = StorageImpl_ReadBigBlock(This, depotBlockIndexPos, depotBuffer);

    if (!success)
      return STG_E_READFAULT;

    for (index = 0; index < NUM_BLOCKS_PER_DEPOT_BLOCK; index++)
    {
      StorageUtl_ReadDWord(depotBuffer, index*sizeof(ULONG), nextBlockIndex);
      This->blockDepotCached[index] = *nextBlockIndex;
    }
  }

  *nextBlockIndex = This->blockDepotCached[depotBlockOffset/sizeof(ULONG)];

  return S_OK;
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
static ULONG Storage32Impl_GetNextExtendedBlock(StorageImpl* This, ULONG blockIndex)
{
  ULONG nextBlockIndex   = BLOCK_SPECIAL;
  ULONG depotBlockOffset = This->bigBlockSize - sizeof(ULONG);

  StorageImpl_ReadDWordFromBigBlock(This, blockIndex, depotBlockOffset,
                        &nextBlockIndex);

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
static void StorageImpl_SetNextBlockInChain(
          StorageImpl* This,
          ULONG          blockIndex,
          ULONG          nextBlock)
{
  ULONG offsetInDepot    = blockIndex * sizeof (ULONG);
  ULONG depotBlockCount  = offsetInDepot / This->bigBlockSize;
  ULONG depotBlockOffset = offsetInDepot % This->bigBlockSize;
  ULONG depotBlockIndexPos;

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

  StorageImpl_WriteDWordToBigBlock(This, depotBlockIndexPos, depotBlockOffset,
                        nextBlock);
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
static HRESULT StorageImpl_LoadFileHeader(
          StorageImpl* This)
{
  HRESULT hr = STG_E_FILENOTFOUND;
  BYTE    headerBigBlock[BIG_BLOCK_SIZE];
  BOOL    success;
  int     index;

  TRACE("\n");
  /*
   * Get a pointer to the big block of data containing the header.
   */
  success = StorageImpl_ReadBigBlock(This, -1, headerBigBlock);

  /*
   * Extract the information from the header.
   */
  if (success)
  {
    /*
     * Check for the "magic number" signature and return an error if it is not
     * found.
     */
    if (memcmp(headerBigBlock, STORAGE_oldmagic, sizeof(STORAGE_oldmagic))==0)
    {
      return STG_E_OLDFORMAT;
    }

    if (memcmp(headerBigBlock, STORAGE_magic, sizeof(STORAGE_magic))!=0)
    {
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
    if (This->bigBlockSize != DEF_BIG_BLOCK_SIZE ||
	This->smallBlockSize != DEF_SMALL_BLOCK_SIZE)
    {
	WARN("Broken OLE storage file\n");
	hr = STG_E_INVALIDHEADER;
    }
    else
	hr = S_OK;
  }

  return hr;
}

/******************************************************************************
 *      Storage32Impl_SaveFileHeader
 *
 * This method will save to the file the header, i.e. big block -1.
 */
static void StorageImpl_SaveFileHeader(
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
  }

  /*
   * Write the information to the header.
   */
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
    OFFSET_SBDEPOTCOUNT,
    This->smallBlockDepotChain ?
     BlockChainStream_GetCount(This->smallBlockDepotChain) : 0);

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
  HRESULT        readRes;
  ULONG          bytesRead;

  offsetInPropSet.u.HighPart = 0;
  offsetInPropSet.u.LowPart  = index * PROPSET_BLOCK_SIZE;

  readRes = BlockChainStream_ReadAt(
                    This->rootBlockChain,
                    offsetInPropSet,
                    PROPSET_BLOCK_SIZE,
                    currentProperty,
                    &bytesRead);

  if (SUCCEEDED(readRes))
  {
    /* replace the name of root entry (often "Root Entry") by the file name */
    WCHAR *propName = (index == This->base.rootPropertySetIndex) ?
	    		This->filename : (WCHAR *)currentProperty+OFFSET_PS_NAME;

    memset(buffer->name, 0, sizeof(buffer->name));
    memcpy(
      buffer->name,
      propName,
      PROPERTY_NAME_BUFFER_LEN );
    TRACE("storage name: %s\n", debugstr_w(buffer->name));

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

  return SUCCEEDED(readRes) ? TRUE : FALSE;
}

/*********************************************************************
 * Write the specified property into the property chain
 */
BOOL StorageImpl_WriteProperty(
  StorageImpl*          This,
  ULONG                 index,
  const StgProperty*    buffer)
{
  BYTE           currentProperty[PROPSET_BLOCK_SIZE];
  ULARGE_INTEGER offsetInPropSet;
  HRESULT        writeRes;
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

  writeRes = BlockChainStream_WriteAt(This->rootBlockChain,
                                      offsetInPropSet,
                                      PROPSET_BLOCK_SIZE,
                                      currentProperty,
                                      &bytesWritten);
  return SUCCEEDED(writeRes) ? TRUE : FALSE;
}

static BOOL StorageImpl_ReadBigBlock(
  StorageImpl* This,
  ULONG          blockIndex,
  void*          buffer)
{
  ULARGE_INTEGER ulOffset;
  DWORD  read;

  ulOffset.u.HighPart = 0;
  ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex);

  StorageImpl_ReadAt(This, ulOffset, buffer, This->bigBlockSize, &read);
  return (read == This->bigBlockSize);
}

static BOOL StorageImpl_ReadDWordFromBigBlock(
  StorageImpl*  This,
  ULONG         blockIndex,
  ULONG         offset,
  DWORD*        value)
{
  ULARGE_INTEGER ulOffset;
  DWORD  read;
  DWORD  tmp;

  ulOffset.u.HighPart = 0;
  ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex);
  ulOffset.u.LowPart += offset;

  StorageImpl_ReadAt(This, ulOffset, &tmp, sizeof(DWORD), &read);
  *value = le32toh(tmp);
  return (read == sizeof(DWORD));
}

static BOOL StorageImpl_WriteBigBlock(
  StorageImpl*  This,
  ULONG         blockIndex,
  const void*   buffer)
{
  ULARGE_INTEGER ulOffset;
  DWORD  wrote;

  ulOffset.u.HighPart = 0;
  ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex);

  StorageImpl_WriteAt(This, ulOffset, buffer, This->bigBlockSize, &wrote);
  return (wrote == This->bigBlockSize);
}

static BOOL StorageImpl_WriteDWordToBigBlock(
  StorageImpl* This,
  ULONG         blockIndex,
  ULONG         offset,
  DWORD         value)
{
  ULARGE_INTEGER ulOffset;
  DWORD  wrote;

  ulOffset.u.HighPart = 0;
  ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex);
  ulOffset.u.LowPart += offset;

  value = htole32(value);
  StorageImpl_WriteAt(This, ulOffset, &value, sizeof(DWORD), &wrote);
  return (wrote == sizeof(DWORD));
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
  ULONG cbRead, cbWritten;
  ULARGE_INTEGER cbTotalRead;
  ULONG propertyIndex;
  HRESULT resWrite = S_OK;
  HRESULT resRead;
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
  if(!bbTempChain) return NULL;
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
  cbTotalRead.QuadPart = 0;

  buffer = HeapAlloc(GetProcessHeap(),0,DEF_SMALL_BLOCK_SIZE);
  do
  {
    resRead = SmallBlockChainStream_ReadAt(*ppsbChain,
                                           offset,
                                           This->smallBlockSize,
                                           buffer,
                                           &cbRead);
    if (FAILED(resRead))
        break;

    if (cbRead > 0)
    {
        cbTotalRead.QuadPart += cbRead;

        resWrite = BlockChainStream_WriteAt(bbTempChain,
                                            offset,
                                            cbRead,
                                            buffer,
                                            &cbWritten);

        if (FAILED(resWrite))
            break;

        offset.u.LowPart += This->smallBlockSize;
    }
  } while (cbTotalRead.QuadPart < size.QuadPart);
  HeapFree(GetProcessHeap(),0,buffer);

  if (FAILED(resRead) || FAILED(resWrite))
  {
    ERR("conversion failed: resRead = 0x%08x, resWrite = 0x%08x\n", resRead, resWrite);
    BlockChainStream_Destroy(bbTempChain);
    return NULL;
  }

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

static void StorageInternalImpl_Destroy( StorageBaseImpl *iface)
{
  StorageInternalImpl* This = (StorageInternalImpl*) iface;

  StorageBaseImpl_Release((IStorage*)This->base.ancestorStorage);
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
**
** Storage32InternalImpl_Commit
**
** The non-root storages cannot be opened in transacted mode thus this function
** does nothing.
*/
static HRESULT WINAPI StorageInternalImpl_Commit(
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
static HRESULT WINAPI StorageInternalImpl_Revert(
  IStorage*            iface)
{
  return S_OK;
}

static void IEnumSTATSTGImpl_Destroy(IEnumSTATSTGImpl* This)
{
  IStorage_Release((IStorage*)This->parentStorage);
  HeapFree(GetProcessHeap(), 0, This->stackToVisit);
  HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IEnumSTATSTGImpl_QueryInterface(
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
  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_IEnumSTATSTG, riid))
  {
    *ppvObject = (IEnumSTATSTG*)This;
    IEnumSTATSTG_AddRef((IEnumSTATSTG*)This);
    return S_OK;
  }

  return E_NOINTERFACE;
}

static ULONG   WINAPI IEnumSTATSTGImpl_AddRef(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;
  return InterlockedIncrement(&This->ref);
}

static ULONG   WINAPI IEnumSTATSTGImpl_Release(
  IEnumSTATSTG* iface)
{
  IEnumSTATSTGImpl* const This=(IEnumSTATSTGImpl*)iface;

  ULONG newRef;

  newRef = InterlockedDecrement(&This->ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (newRef==0)
  {
    IEnumSTATSTGImpl_Destroy(This);
  }

  return newRef;
}

static HRESULT WINAPI IEnumSTATSTGImpl_Next(
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


static HRESULT WINAPI IEnumSTATSTGImpl_Skip(
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

static HRESULT WINAPI IEnumSTATSTGImpl_Reset(
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

static HRESULT WINAPI IEnumSTATSTGImpl_Clone(
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

static INT IEnumSTATSTGImpl_FindParentProperty(
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

static ULONG IEnumSTATSTGImpl_FindProperty(
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
          (const OLECHAR*)currentProperty->name,
          (const OLECHAR*)lpszPropName) == 0)
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

static void IEnumSTATSTGImpl_PushSearchNode(
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

static ULONG IEnumSTATSTGImpl_PopSearchNode(
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

/*
 * Virtual function table for the IEnumSTATSTGImpl class.
 */
static const IEnumSTATSTGVtbl IEnumSTATSTGImpl_Vtbl =
{
    IEnumSTATSTGImpl_QueryInterface,
    IEnumSTATSTGImpl_AddRef,
    IEnumSTATSTGImpl_Release,
    IEnumSTATSTGImpl_Next,
    IEnumSTATSTGImpl_Skip,
    IEnumSTATSTGImpl_Reset,
    IEnumSTATSTGImpl_Clone
};

/******************************************************************************
** IEnumSTATSTGImpl implementation
*/

static IEnumSTATSTGImpl* IEnumSTATSTGImpl_Construct(
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
    newEnumeration->lpVtbl    = &IEnumSTATSTGImpl_Vtbl;
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

/*
 * Virtual function table for the Storage32InternalImpl class.
 */
static const IStorageVtbl Storage32InternalImpl_Vtbl =
{
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

/******************************************************************************
** Storage32InternalImpl implementation
*/

static StorageInternalImpl* StorageInternalImpl_Construct(
  StorageImpl* ancestorStorage,
  DWORD        openFlags,
  ULONG        rootPropertyIndex)
{
  StorageInternalImpl* newStorage;

  /*
   * Allocate space for the new storage object
   */
  newStorage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StorageInternalImpl));

  if (newStorage!=0)
  {
    /*
     * Initialize the stream list
     */
    list_init(&newStorage->base.strmHead);

    /*
     * Initialize the virtual function table.
     */
    newStorage->base.lpVtbl = &Storage32InternalImpl_Vtbl;
    newStorage->base.v_destructor = &StorageInternalImpl_Destroy;
    newStorage->base.openFlags = (openFlags & ~STGM_CREATE);

    /*
     * Keep the ancestor storage pointer and nail a reference to it.
     */
    newStorage->base.ancestorStorage = ancestorStorage;
    StorageBaseImpl_AddRef((IStorage*)(newStorage->base.ancestorStorage));

    /*
     * Keep the index of the root property set for this storage,
     */
    newStorage->base.rootPropertySetIndex = rootPropertyIndex;

    return newStorage;
  }

  return 0;
}

/******************************************************************************
** StorageUtl implementation
*/

void StorageUtl_ReadWord(const BYTE* buffer, ULONG offset, WORD* value)
{
  WORD tmp;

  memcpy(&tmp, buffer+offset, sizeof(WORD));
  *value = le16toh(tmp);
}

void StorageUtl_WriteWord(BYTE* buffer, ULONG offset, WORD value)
{
  value = htole16(value);
  memcpy(buffer+offset, &value, sizeof(WORD));
}

void StorageUtl_ReadDWord(const BYTE* buffer, ULONG offset, DWORD* value)
{
  DWORD tmp;

  memcpy(&tmp, buffer+offset, sizeof(DWORD));
  *value = le32toh(tmp);
}

void StorageUtl_WriteDWord(BYTE* buffer, ULONG offset, DWORD value)
{
  value = htole32(value);
  memcpy(buffer+offset, &value, sizeof(DWORD));
}

void StorageUtl_ReadULargeInteger(const BYTE* buffer, ULONG offset,
 ULARGE_INTEGER* value)
{
#ifdef WORDS_BIGENDIAN
    ULARGE_INTEGER tmp;

    memcpy(&tmp, buffer + offset, sizeof(ULARGE_INTEGER));
    value->u.LowPart = htole32(tmp.u.HighPart);
    value->u.HighPart = htole32(tmp.u.LowPart);
#else
    memcpy(value, buffer + offset, sizeof(ULARGE_INTEGER));
#endif
}

void StorageUtl_WriteULargeInteger(BYTE* buffer, ULONG offset,
 const ULARGE_INTEGER *value)
{
#ifdef WORDS_BIGENDIAN
    ULARGE_INTEGER tmp;

    tmp.u.LowPart = htole32(value->u.HighPart);
    tmp.u.HighPart = htole32(value->u.LowPart);
    memcpy(buffer + offset, &tmp, sizeof(ULARGE_INTEGER));
#else
    memcpy(buffer + offset, value, sizeof(ULARGE_INTEGER));
#endif
}

void StorageUtl_ReadGUID(const BYTE* buffer, ULONG offset, GUID* value)
{
  StorageUtl_ReadDWord(buffer, offset,   &(value->Data1));
  StorageUtl_ReadWord(buffer,  offset+4, &(value->Data2));
  StorageUtl_ReadWord(buffer,  offset+6, &(value->Data3));

  memcpy(value->Data4, buffer+offset+8, sizeof(value->Data4));
}

void StorageUtl_WriteGUID(BYTE* buffer, ULONG offset, const GUID* value)
{
  StorageUtl_WriteDWord(buffer, offset,   value->Data1);
  StorageUtl_WriteWord(buffer,  offset+4, value->Data2);
  StorageUtl_WriteWord(buffer,  offset+6, value->Data3);

  memcpy(buffer+offset+8, value->Data4, sizeof(value->Data4));
}

void StorageUtl_CopyPropertyToSTATSTG(
  STATSTG*              destination,
  const StgProperty*    source,
  int                   statFlags)
{
  /*
   * The copy of the string occurs only when the flag is not set
   */
  if( ((statFlags & STATFLAG_NONAME) != 0) || 
       (source->name == NULL) || 
       (source->name[0] == 0) )
  {
    destination->pwcsName = 0;
  }
  else
  {
    destination->pwcsName =
      CoTaskMemAlloc((lstrlenW(source->name)+1)*sizeof(WCHAR));

    strcpyW((LPWSTR)destination->pwcsName, source->name);
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

    if(FAILED(StorageImpl_GetNextBlockInChain(
	      parentStorage,
	      blockIndex,
	      &blockIndex)))
    {
      HeapFree(GetProcessHeap(), 0, newStream);
      return NULL;
    }
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
static ULONG BlockChainStream_GetHeadOfChain(BlockChainStream* This)
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
static ULONG BlockChainStream_GetCount(BlockChainStream* This)
{
  ULONG blockIndex;
  ULONG count = 0;

  blockIndex = BlockChainStream_GetHeadOfChain(This);

  while (blockIndex != BLOCK_END_OF_CHAIN)
  {
    count++;

    if(FAILED(StorageImpl_GetNextBlockInChain(
                   This->parentStorage,
                   blockIndex,
		   &blockIndex)))
      return 0;
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
HRESULT BlockChainStream_ReadAt(BlockChainStream* This,
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

  TRACE("(%p)-> %i %p %i %p\n",This, offset.u.LowPart, buffer, size, bytesRead);

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
    if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex, &blockIndex)))
      return STG_E_DOCFILECORRUPT;
    blockNoInSequence--;
  }

  if ((blockNoInSequence > 0) && (blockIndex == BLOCK_END_OF_CHAIN))
      return STG_E_DOCFILECORRUPT; /* We failed to find the starting block */

  This->lastBlockNoInSequenceIndex = blockIndex;

  /*
   * Start reading the buffer.
   */
  *bytesRead   = 0;
  bufferWalker = buffer;

  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    ULARGE_INTEGER ulOffset;
    DWORD bytesReadAt;
    /*
     * Calculate how many bytes we can copy from this big block.
     */
    bytesToReadInBuffer =
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);

     TRACE("block %i\n",blockIndex);
     ulOffset.u.HighPart = 0;
     ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex) +
                             offsetInBlock;

     StorageImpl_ReadAt(This->parentStorage,
         ulOffset,
         bufferWalker,
         bytesToReadInBuffer,
         &bytesReadAt);
    /*
     * Step to the next big block.
     */
    if( size > bytesReadAt && FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex, &blockIndex)))
      return STG_E_DOCFILECORRUPT;

    bufferWalker += bytesReadAt;
    size         -= bytesReadAt;
    *bytesRead   += bytesReadAt;
    offsetInBlock = 0;  /* There is no offset on the next block */

    if (bytesToReadInBuffer != bytesReadAt)
        break;
  }

  return (size == 0) ? S_OK : STG_E_READFAULT;
}

/******************************************************************************
 *      BlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * bytesWritten may be NULL.
 * Will fail if not all specified number of bytes have been written.
 */
HRESULT BlockChainStream_WriteAt(BlockChainStream* This,
  ULARGE_INTEGER    offset,
  ULONG             size,
  const void*       buffer,
  ULONG*            bytesWritten)
{
  ULONG blockNoInSequence = offset.u.LowPart / This->parentStorage->bigBlockSize;
  ULONG offsetInBlock     = offset.u.LowPart % This->parentStorage->bigBlockSize;
  ULONG bytesToWrite;
  ULONG blockIndex;
  const BYTE* bufferWalker;

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
    if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex,
					      &blockIndex)))
      return STG_E_DOCFILECORRUPT;
    blockNoInSequence--;
  }

  This->lastBlockNoInSequenceIndex = blockIndex;

  /* BlockChainStream_SetSize should have already been called to ensure we have
   * enough blocks in the chain to write into */
  if (blockIndex == BLOCK_END_OF_CHAIN)
  {
    ERR("not enough blocks in chain to write data\n");
    return STG_E_DOCFILECORRUPT;
  }

  *bytesWritten   = 0;
  bufferWalker = (const BYTE*)buffer;

  while ( (size > 0) && (blockIndex != BLOCK_END_OF_CHAIN) )
  {
    ULARGE_INTEGER ulOffset;
    DWORD bytesWrittenAt;
    /*
     * Calculate how many bytes we can copy from this big block.
     */
    bytesToWrite =
      min(This->parentStorage->bigBlockSize - offsetInBlock, size);

    TRACE("block %i\n",blockIndex);
    ulOffset.u.HighPart = 0;
    ulOffset.u.LowPart = BLOCK_GetBigBlockOffset(blockIndex) +
                             offsetInBlock;

    StorageImpl_WriteAt(This->parentStorage,
         ulOffset,
         bufferWalker,
         bytesToWrite,
         &bytesWrittenAt);

    /*
     * Step to the next big block.
     */
    if(size > bytesWrittenAt && FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex,
					      &blockIndex)))
      return STG_E_DOCFILECORRUPT;

    bufferWalker  += bytesWrittenAt;
    size          -= bytesWrittenAt;
    *bytesWritten += bytesWrittenAt;
    offsetInBlock  = 0;      /* There is no offset on the next block */

    if (bytesWrittenAt != bytesToWrite)
      break;
  }

  return (size == 0) ? S_OK : STG_E_WRITEFAULT;
}

/******************************************************************************
 *      BlockChainStream_Shrink
 *
 * Shrinks this chain in the big block depot.
 */
static BOOL BlockChainStream_Shrink(BlockChainStream* This,
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
    if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex,
					      &blockIndex)))
      return FALSE;
    count++;
  }

  /* Get the next block before marking the new end */
  if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, blockIndex,
					    &extraBlock)))
    return FALSE;

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
    if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, extraBlock,
					      &blockIndex)))
      return FALSE;
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
static BOOL BlockChainStream_Enlarge(BlockChainStream* This,
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

      if(FAILED(StorageImpl_GetNextBlockInChain(This->parentStorage, currentBlock,
						&blockIndex)))
	return FALSE;
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
static ULARGE_INTEGER BlockChainStream_GetSize(BlockChainStream* This)
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
static ULONG SmallBlockChainStream_GetHeadOfChain(
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
static HRESULT SmallBlockChainStream_GetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex,
  ULONG*                 nextBlockInChain)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  bytesRead;
  HRESULT res;

  *nextBlockInChain = BLOCK_END_OF_CHAIN;

  offsetOfBlockInDepot.u.HighPart = 0;
  offsetOfBlockInDepot.u.LowPart  = blockIndex * sizeof(ULONG);

  /*
   * Read those bytes in the buffer from the small block file.
   */
  res = BlockChainStream_ReadAt(
              This->parentStorage->smallBlockDepotChain,
              offsetOfBlockInDepot,
              sizeof(DWORD),
              &buffer,
              &bytesRead);

  if (SUCCEEDED(res))
  {
    StorageUtl_ReadDWord((BYTE *)&buffer, 0, nextBlockInChain);
    return S_OK;
  }

  return res;
}

/******************************************************************************
 *       SmallBlockChainStream_SetNextBlockInChain
 *
 * Writes the index of the next block of the specified block in the small
 * block depot.
 * To set the end of chain use BLOCK_END_OF_CHAIN as nextBlock.
 * To flag a block as free use BLOCK_UNUSED as nextBlock.
 */
static void SmallBlockChainStream_SetNextBlockInChain(
  SmallBlockChainStream* This,
  ULONG                  blockIndex,
  ULONG                  nextBlock)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD  buffer;
  ULONG  bytesWritten;

  offsetOfBlockInDepot.u.HighPart = 0;
  offsetOfBlockInDepot.u.LowPart  = blockIndex * sizeof(ULONG);

  StorageUtl_WriteDWord((BYTE *)&buffer, 0, nextBlock);

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
static void SmallBlockChainStream_FreeBlock(
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
static ULONG SmallBlockChainStream_GetNextFreeBlock(
  SmallBlockChainStream* This)
{
  ULARGE_INTEGER offsetOfBlockInDepot;
  DWORD buffer;
  ULONG bytesRead;
  ULONG blockIndex = 0;
  ULONG nextBlockIndex = BLOCK_END_OF_CHAIN;
  HRESULT res = S_OK;
  ULONG smallBlocksPerBigBlock;

  offsetOfBlockInDepot.u.HighPart = 0;

  /*
   * Scan the small block depot for a free block
   */
  while (nextBlockIndex != BLOCK_UNUSED)
  {
    offsetOfBlockInDepot.u.LowPart = blockIndex * sizeof(ULONG);

    res = BlockChainStream_ReadAt(
                This->parentStorage->smallBlockDepotChain,
                offsetOfBlockInDepot,
                sizeof(DWORD),
                &buffer,
                &bytesRead);

    /*
     * If we run out of space for the small block depot, enlarge it
     */
    if (SUCCEEDED(res))
    {
      StorageUtl_ReadDWord((BYTE *)&buffer, 0, &nextBlockIndex);

      if (nextBlockIndex != BLOCK_UNUSED)
        blockIndex++;
    }
    else
    {
      ULONG count =
        BlockChainStream_GetCount(This->parentStorage->smallBlockDepotChain);

      ULONG sbdIndex = This->parentStorage->smallBlockDepotStart;
      ULONG nextBlock, newsbdIndex;
      BYTE smallBlockDepot[BIG_BLOCK_SIZE];

      nextBlock = sbdIndex;
      while (nextBlock != BLOCK_END_OF_CHAIN)
      {
        sbdIndex = nextBlock;
	StorageImpl_GetNextBlockInChain(This->parentStorage, sbdIndex, &nextBlock);
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
      memset(smallBlockDepot, BLOCK_UNUSED, This->parentStorage->bigBlockSize);
      StorageImpl_WriteBigBlock(This->parentStorage, newsbdIndex, smallBlockDepot);

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
          This->parentStorage->base.rootPropertySetIndex,
          &rootProp);

        rootProp.startingBlock = sbStartIndex;
        rootProp.size.u.HighPart = 0;
        rootProp.size.u.LowPart  = This->parentStorage->bigBlockSize;

        StorageImpl_WriteProperty(
          This->parentStorage,
          This->parentStorage->base.rootPropertySetIndex,
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
      This->parentStorage->base.rootPropertySetIndex,
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
        This->parentStorage->base.rootPropertySetIndex,
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
HRESULT SmallBlockChainStream_ReadAt(
  SmallBlockChainStream* This,
  ULARGE_INTEGER         offset,
  ULONG                  size,
  void*                  buffer,
  ULONG*                 bytesRead)
{
  HRESULT rc = S_OK;
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
    rc = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex);
    if(FAILED(rc))
      return rc;
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
     * The small block has already been identified so it shouldn't fail
     * unless the file is corrupt.
     */
    rc = BlockChainStream_ReadAt(This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToReadInBuffer,
      bufferWalker,
      &bytesReadFromBigBlockFile);

    if (FAILED(rc))
      return rc;

    /*
     * Step to the next big block.
     */
    rc = SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex);
    if(FAILED(rc))
      return STG_E_DOCFILECORRUPT;

    bufferWalker += bytesReadFromBigBlockFile;
    size         -= bytesReadFromBigBlockFile;
    *bytesRead   += bytesReadFromBigBlockFile;
    offsetInBlock = (offsetInBlock + bytesReadFromBigBlockFile) % This->parentStorage->smallBlockSize;
  }

  return (size == 0) ? S_OK : STG_E_READFAULT;
}

/******************************************************************************
 *       SmallBlockChainStream_WriteAt
 *
 * Writes the specified number of bytes to this chain at the specified offset.
 * bytesWritten may be NULL.
 * Will fail if not all specified number of bytes have been written.
 */
HRESULT SmallBlockChainStream_WriteAt(
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
  ULONG bytesWrittenToBigBlockFile;
  const BYTE* bufferWalker;
  HRESULT res;

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
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex, &blockIndex)))
      return STG_E_DOCFILECORRUPT;
    blockNoInSequence--;
  }

  /*
   * Start writing the buffer.
   *
   * Here, I'm casting away the constness on the buffer variable
   * This is OK since we don't intend to modify that buffer.
   */
  *bytesWritten   = 0;
  bufferWalker = (const BYTE*)buffer;
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
    res = BlockChainStream_WriteAt(
      This->parentStorage->smallBlockRootChain,
      offsetInBigBlockFile,
      bytesToWriteInBuffer,
      bufferWalker,
      &bytesWrittenToBigBlockFile);
    if (FAILED(res))
      return res;

    /*
     * Step to the next big block.
     */
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex,
							&blockIndex)))
      return FALSE;
    bufferWalker  += bytesWrittenToBigBlockFile;
    size          -= bytesWrittenToBigBlockFile;
    *bytesWritten += bytesWrittenToBigBlockFile;
    offsetInBlock  = (offsetInBlock + bytesWrittenToBigBlockFile) % This->parentStorage->smallBlockSize;
  }

  return (size == 0) ? S_OK : STG_E_WRITEFAULT;
}

/******************************************************************************
 *       SmallBlockChainStream_Shrink
 *
 * Shrinks this chain in the small block depot.
 */
static BOOL SmallBlockChainStream_Shrink(
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
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex,
							&blockIndex)))
      return FALSE;
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
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, blockIndex,
							&extraBlock)))
      return FALSE;

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
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, extraBlock,
							&blockIndex)))
      return FALSE;
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
static BOOL SmallBlockChainStream_Enlarge(
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
    if(FAILED(SmallBlockChainStream_GetNextBlockInChain(This, currentBlock, &blockIndex)))
      return FALSE;
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
static ULARGE_INTEGER SmallBlockChainStream_GetSize(SmallBlockChainStream* This)
{
  StgProperty chainProperty;

  StorageImpl_ReadProperty(
    This->parentStorage,
    This->ownerPropertyIndex,
    &chainProperty);

  return chainProperty.size;
}

/******************************************************************************
 *    StgCreateDocfile  [OLE32.@]
 * Creates a new compound file storage object
 *
 * PARAMS
 *  pwcsName  [ I] Unicode string with filename (can be relative or NULL)
 *  grfMode   [ I] Access mode for opening the new storage object (see STGM_ constants)
 *  reserved  [ ?] unused?, usually 0
 *  ppstgOpen [IO] A pointer to IStorage pointer to the new onject
 *
 * RETURNS
 *  S_OK if the file was successfully created
 *  some STG_E_ value if error
 * NOTES
 *  if pwcsName is NULL, create file with new unique name
 *  the function can returns
 *  STG_S_CONVERTED if the specified file was successfully converted to storage format
 *  (unrealized now)
 */
HRESULT WINAPI StgCreateDocfile(
  LPCOLESTR pwcsName,
  DWORD       grfMode,
  DWORD       reserved,
  IStorage  **ppstgOpen)
{
  StorageImpl* newStorage = 0;
  HANDLE       hFile      = INVALID_HANDLE_VALUE;
  HRESULT        hr         = STG_E_INVALIDFLAG;
  DWORD          shareMode;
  DWORD          accessMode;
  DWORD          creationMode;
  DWORD          fileAttributes;
  WCHAR          tempFileName[MAX_PATH];

  TRACE("(%s, %x, %d, %p)\n",
	debugstr_w(pwcsName), grfMode,
	reserved, ppstgOpen);

  /*
   * Validate the parameters
   */
  if (ppstgOpen == 0)
    return STG_E_INVALIDPOINTER;
  if (reserved != 0)
    return STG_E_INVALIDPARAMETER;

  /* if no share mode given then DENY_NONE is the default */
  if (STGM_SHARE_MODE(grfMode) == 0)
      grfMode |= STGM_SHARE_DENY_NONE;

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ))
    goto end;

  /* StgCreateDocFile seems to refuse readonly access, despite MSDN */
  switch(STGM_ACCESS_MODE(grfMode))
  {
  case STGM_WRITE:
  case STGM_READWRITE:
    break;
  default:
    goto end;
  }

  /* in direct mode, can only use SHARE_EXCLUSIVE */
  if (!(grfMode & STGM_TRANSACTED) && (STGM_SHARE_MODE(grfMode) != STGM_SHARE_EXCLUSIVE))
    goto end;

  /* but in transacted mode, any share mode is valid */

  /*
   * Generate a unique name.
   */
  if (pwcsName == 0)
  {
    WCHAR tempPath[MAX_PATH];
    static const WCHAR prefix[] = { 'S', 'T', 'O', 0 };

    memset(tempPath, 0, sizeof(tempPath));
    memset(tempFileName, 0, sizeof(tempFileName));

    if ((GetTempPathW(MAX_PATH, tempPath)) == 0 )
      tempPath[0] = '.';

    if (GetTempFileNameW(tempPath, prefix, 0, tempFileName) != 0)
      pwcsName = tempFileName;
    else
    {
      hr = STG_E_INSUFFICIENTMEMORY;
      goto end;
    }

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
    FIXME("Transacted mode not implemented.\n");

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
    if(GetLastError() == ERROR_FILE_EXISTS)
      hr = STG_E_FILEALREADYEXISTS;
    else
      hr = E_FAIL;
    goto end;
  }

  /*
   * Allocate and initialize the new IStorage32object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));

  if (newStorage == 0)
  {
    hr = STG_E_INSUFFICIENTMEMORY;
    goto end;
  }

  hr = StorageImpl_Construct(
         newStorage,
         hFile,
        pwcsName,
         NULL,
         grfMode,
         TRUE,
         TRUE);

  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    goto end;
  }

  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);
end:
  TRACE("<-- %p  r = %08x\n", *ppstgOpen, hr);

  return hr;
}

/******************************************************************************
 *              StgCreateStorageEx        [OLE32.@]
 */
HRESULT WINAPI StgCreateStorageEx(const WCHAR* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, STGOPTIONS* pStgOptions, void* reserved, REFIID riid, void** ppObjectOpen)
{
    TRACE("(%s, %x, %x, %x, %p, %p, %p, %p)\n", debugstr_w(pwcsName),
          grfMode, stgfmt, grfAttrs, pStgOptions, reserved, riid, ppObjectOpen);

    if (stgfmt != STGFMT_FILE && grfAttrs != 0)
    {
        ERR("grfAttrs must be 0 if stgfmt != STGFMT_FILE\n");
        return STG_E_INVALIDPARAMETER;  
    }

    if (stgfmt == STGFMT_FILE && grfAttrs != 0 && grfAttrs != FILE_FLAG_NO_BUFFERING)
    {
        ERR("grfAttrs must be 0 or FILE_FLAG_NO_BUFFERING if stgfmt == STGFMT_FILE\n");
        return STG_E_INVALIDPARAMETER;  
    }

    if (stgfmt == STGFMT_FILE)
    {
        ERR("Cannot use STGFMT_FILE - this is NTFS only\n");  
        return STG_E_INVALIDPARAMETER;
    }

    if (stgfmt == STGFMT_STORAGE || stgfmt == STGFMT_DOCFILE)
    {
        FIXME("Stub: calling StgCreateDocfile, but ignoring pStgOptions and grfAttrs\n");
        return StgCreateDocfile(pwcsName, grfMode, 0, (IStorage **)ppObjectOpen); 
    }

    ERR("Invalid stgfmt argument\n");
    return STG_E_INVALIDPARAMETER;
}

/******************************************************************************
 *              StgCreatePropSetStg       [OLE32.@]
 */
HRESULT WINAPI StgCreatePropSetStg(IStorage *pstg, DWORD reserved,
 IPropertySetStorage **ppPropSetStg)
{
    HRESULT hr;

    TRACE("(%p, 0x%x, %p)\n", pstg, reserved, ppPropSetStg);
    if (reserved)
        hr = STG_E_INVALIDPARAMETER;
    else
        hr = StorageBaseImpl_QueryInterface(pstg, &IID_IPropertySetStorage,
         (void**)ppPropSetStg);
    return hr;
}

/******************************************************************************
 *              StgOpenStorageEx      [OLE32.@]
 */
HRESULT WINAPI StgOpenStorageEx(const WCHAR* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, STGOPTIONS* pStgOptions, void* reserved, REFIID riid, void** ppObjectOpen)
{
    TRACE("(%s, %x, %x, %x, %p, %p, %p, %p)\n", debugstr_w(pwcsName),
          grfMode, stgfmt, grfAttrs, pStgOptions, reserved, riid, ppObjectOpen);

    if (stgfmt != STGFMT_DOCFILE && grfAttrs != 0)
    {
        ERR("grfAttrs must be 0 if stgfmt != STGFMT_DOCFILE\n");
        return STG_E_INVALIDPARAMETER;  
    }

    switch (stgfmt)
    {
    case STGFMT_FILE:
        ERR("Cannot use STGFMT_FILE - this is NTFS only\n");  
        return STG_E_INVALIDPARAMETER;
        
    case STGFMT_STORAGE:
        break;

    case STGFMT_DOCFILE:
        if (grfAttrs && grfAttrs != FILE_FLAG_NO_BUFFERING)
        {
            ERR("grfAttrs must be 0 or FILE_FLAG_NO_BUFFERING if stgfmt == STGFMT_DOCFILE\n");
            return STG_E_INVALIDPARAMETER;  
        }
        FIXME("Stub: calling StgOpenStorage, but ignoring pStgOptions and grfAttrs\n");
        break;

    case STGFMT_ANY:
        WARN("STGFMT_ANY assuming storage\n");
        break;

    default:
        return STG_E_INVALIDPARAMETER;
    }

    return StgOpenStorage(pwcsName, NULL, grfMode, (SNB)NULL, 0, (IStorage **)ppObjectOpen); 
}


/******************************************************************************
 *              StgOpenStorage        [OLE32.@]
 */
HRESULT WINAPI StgOpenStorage(
  const OLECHAR *pwcsName,
  IStorage      *pstgPriority,
  DWORD          grfMode,
  SNB            snbExclude,
  DWORD          reserved,
  IStorage     **ppstgOpen)
{
  StorageImpl*   newStorage = 0;
  HRESULT        hr = S_OK;
  HANDLE         hFile = 0;
  DWORD          shareMode;
  DWORD          accessMode;
  WCHAR          fullname[MAX_PATH];

  TRACE("(%s, %p, %x, %p, %d, %p)\n",
	debugstr_w(pwcsName), pstgPriority, grfMode,
	snbExclude, reserved, ppstgOpen);

  /*
   * Perform sanity checks
   */
  if (pwcsName == 0)
  {
    hr = STG_E_INVALIDNAME;
    goto end;
  }

  if (ppstgOpen == 0)
  {
    hr = STG_E_INVALIDPOINTER;
    goto end;
  }

  if (reserved)
  {
    hr = STG_E_INVALIDPARAMETER;
    goto end;
  }

  if (grfMode & STGM_PRIORITY)
  {
    if (grfMode & (STGM_TRANSACTED|STGM_SIMPLE|STGM_NOSCRATCH|STGM_NOSNAPSHOT))
      return STG_E_INVALIDFLAG;
    if (grfMode & STGM_DELETEONRELEASE)
      return STG_E_INVALIDFUNCTION;
    if(STGM_ACCESS_MODE(grfMode) != STGM_READ)
      return STG_E_INVALIDFLAG;
    grfMode &= ~0xf0; /* remove the existing sharing mode */
    grfMode |= STGM_SHARE_DENY_NONE;

    /* STGM_PRIORITY stops other IStorage objects on the same file from
     * committing until the STGM_PRIORITY IStorage is closed. it also
     * stops non-transacted mode StgOpenStorage calls with write access from
     * succeeding. obviously, both of these cannot be achieved through just
     * file share flags */
    FIXME("STGM_PRIORITY mode not implemented correctly\n");
  }

  /*
   * Validate the sharing mode
   */
  if (!(grfMode & (STGM_TRANSACTED|STGM_PRIORITY)))
    switch(STGM_SHARE_MODE(grfMode))
    {
      case STGM_SHARE_EXCLUSIVE:
      case STGM_SHARE_DENY_WRITE:
        break;
      default:
        hr = STG_E_INVALIDFLAG;
        goto end;
    }

  /*
   * Validate the STGM flags
   */
  if ( FAILED( validateSTGM(grfMode) ) ||
       (grfMode&STGM_CREATE))
  {
    hr = STG_E_INVALIDFLAG;
    goto end;
  }

  /* shared reading requires transacted mode */
  if( STGM_SHARE_MODE(grfMode) == STGM_SHARE_DENY_WRITE &&
      STGM_ACCESS_MODE(grfMode) == STGM_READWRITE &&
     !(grfMode&STGM_TRANSACTED) )
  {
    hr = STG_E_INVALIDFLAG;
    goto end;
  }

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
    DWORD last_error = GetLastError();

    hr = E_FAIL;

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

    goto end;
  }

  /*
   * Refuse to open the file if it's too small to be a structured storage file
   * FIXME: verify the file when reading instead of here
   */
  if (GetFileSize(hFile, NULL) < 0x100)
  {
    CloseHandle(hFile);
    hr = STG_E_FILEALREADYEXISTS;
    goto end;
  }

  /*
   * Allocate and initialize the new IStorage32object.
   */
  newStorage = HeapAlloc(GetProcessHeap(), 0, sizeof(StorageImpl));

  if (newStorage == 0)
  {
    hr = STG_E_INSUFFICIENTMEMORY;
    goto end;
  }

  /* Initialize the storage */
  hr = StorageImpl_Construct(
         newStorage,
         hFile,
         pwcsName,
         NULL,
         grfMode,
         TRUE,
         FALSE );

  if (FAILED(hr))
  {
    HeapFree(GetProcessHeap(), 0, newStorage);
    /*
     * According to the docs if the file is not a storage, return STG_E_FILEALREADYEXISTS
     */
    if(hr == STG_E_INVALIDHEADER)
	hr = STG_E_FILEALREADYEXISTS;
    goto end;
  }

  /* prepare the file name string given in lieu of the root property name */
  GetFullPathNameW(pwcsName, MAX_PATH, fullname, NULL);
  memcpy(newStorage->filename, fullname, PROPERTY_NAME_BUFFER_LEN);
  newStorage->filename[PROPERTY_NAME_BUFFER_LEN-1] = '\0';

  /*
   * Get an "out" pointer for the caller.
   */
  hr = StorageBaseImpl_QueryInterface(
         (IStorage*)newStorage,
         (REFIID)&IID_IStorage,
         (void**)ppstgOpen);

end:
  TRACE("<-- %08x, IStorage %p\n", hr, ppstgOpen ? *ppstgOpen : NULL);
  return hr;
}

/******************************************************************************
 *    StgCreateDocfileOnILockBytes    [OLE32.@]
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
 *    StgOpenStorageOnILockBytes    [OLE32.@]
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
 *              StgSetTimes [ole32.@]
 *              StgSetTimes [OLE32.@]
 *
 *
 */
HRESULT WINAPI StgSetTimes(OLECHAR const *str, FILETIME const *pctime,
                           FILETIME const *patime, FILETIME const *pmtime)
{
  IStorage *stg = NULL;
  HRESULT r;
 
  TRACE("%s %p %p %p\n", debugstr_w(str), pctime, patime, pmtime);

  r = StgOpenStorage(str, NULL, STGM_READWRITE | STGM_SHARE_DENY_WRITE,
                     0, 0, &stg);
  if( SUCCEEDED(r) )
  {
    r = IStorage_SetElementTimes(stg, NULL, pctime, patime, pmtime);
    IStorage_Release(stg);
  }

  return r;
}

/******************************************************************************
 *              StgIsStorageILockBytes        [OLE32.@]
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
 *              WriteClassStg        [OLE32.@]
 *
 * This method will store the specified CLSID in the specified storage object
 */
HRESULT WINAPI WriteClassStg(IStorage* pStg, REFCLSID rclsid)
{
  HRESULT hRes;

  if(!pStg)
    return E_INVALIDARG;

  hRes = IStorage_SetClass(pStg, rclsid);

  return hRes;
}

/***********************************************************************
 *    ReadClassStg (OLE32.@)
 *
 * This method reads the CLSID previously written to a storage object with
 * the WriteClassStg.
 *
 * PARAMS
 *  pstg    [I] IStorage pointer
 *  pclsid  [O] Pointer to where the CLSID is written
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid){

    STATSTG pstatstg;
    HRESULT hRes;

    TRACE("(%p, %p)\n", pstg, pclsid);

    if(!pstg || !pclsid)
        return E_INVALIDARG;

   /*
    * read a STATSTG structure (contains the clsid) from the storage
    */
    hRes=IStorage_Stat(pstg,&pstatstg,STATFLAG_DEFAULT);

    if(SUCCEEDED(hRes))
        *pclsid=pstatstg.clsid;

    return hRes;
}

/***********************************************************************
 *    OleLoadFromStream (OLE32.@)
 *
 * This function loads an object from stream
 */
HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj)
{
    CLSID	clsid;
    HRESULT	res;
    LPPERSISTSTREAM	xstm;

    TRACE("(%p,%s,%p)\n",pStm,debugstr_guid(iidInterface),ppvObj);

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
 *    OleSaveToStream (OLE32.@)
 *
 * This function saves an object with the IPersistStream interface on it
 * to the specified stream.
 */
HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm)
{

    CLSID clsid;
    HRESULT res;

    TRACE("(%p,%p)\n",pPStm,pStm);

    res=IPersistStream_GetClassID(pPStm,&clsid);

    if (SUCCEEDED(res)){

        res=WriteClassStm(pStm,&clsid);

        if (SUCCEEDED(res))

            res=IPersistStream_Save(pPStm,pStm,TRUE);
    }

    TRACE("Finished Save\n");
    return res;
}

/****************************************************************************
 * This method validate a STGM parameter that can contain the values below
 *
 * The stgm modes in 0x0000ffff are not bit masks, but distinct 4 bit values.
 * The stgm values contained in 0xffff0000 are bitmasks.
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
  DWORD access = STGM_ACCESS_MODE(stgm);
  DWORD share  = STGM_SHARE_MODE(stgm);
  DWORD create = STGM_CREATE_MODE(stgm);

  if (stgm&~STGM_KNOWN_FLAGS)
  {
    ERR("unknown flags %08x\n", stgm);
    return E_FAIL;
  }

  switch (access)
  {
  case STGM_READ:
  case STGM_WRITE:
  case STGM_READWRITE:
    break;
  default:
    return E_FAIL;
  }

  switch (share)
  {
  case STGM_SHARE_DENY_NONE:
  case STGM_SHARE_DENY_READ:
  case STGM_SHARE_DENY_WRITE:
  case STGM_SHARE_EXCLUSIVE:
    break;
  default:
    return E_FAIL;
  }

  switch (create)
  {
  case STGM_CREATE:
  case STGM_FAILIFTHERE:
    break;
  default:
    return E_FAIL;
  }

  /*
   * STGM_DIRECT | STGM_TRANSACTED | STGM_SIMPLE
   */
  if ( (stgm & STGM_TRANSACTED) && (stgm & STGM_SIMPLE) )
      return E_FAIL;

  /*
   * STGM_CREATE | STGM_CONVERT
   * if both are false, STGM_FAILIFTHERE is set to TRUE
   */
  if ( create == STGM_CREATE && (stgm & STGM_CONVERT) )
    return E_FAIL;

  /*
   * STGM_NOSCRATCH requires STGM_TRANSACTED
   */
  if ( (stgm & STGM_NOSCRATCH) && !(stgm & STGM_TRANSACTED) )
    return E_FAIL;

  /*
   * STGM_NOSNAPSHOT requires STGM_TRANSACTED and
   * not STGM_SHARE_EXCLUSIVE or STGM_SHARE_DENY_WRITE`
   */
  if ( (stgm & STGM_NOSNAPSHOT) &&
        (!(stgm & STGM_TRANSACTED) ||
         share == STGM_SHARE_EXCLUSIVE ||
         share == STGM_SHARE_DENY_WRITE) )
    return E_FAIL;

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
  switch (STGM_SHARE_MODE(stgm))
  {
  case STGM_SHARE_DENY_NONE:
    return FILE_SHARE_READ | FILE_SHARE_WRITE;
  case STGM_SHARE_DENY_READ:
    return FILE_SHARE_WRITE;
  case STGM_SHARE_DENY_WRITE:
    return FILE_SHARE_READ;
  case STGM_SHARE_EXCLUSIVE:
    return 0;
  }
  ERR("Invalid share mode!\n");
  assert(0);
  return 0;
}

/****************************************************************************
 *      GetAccessModeFromSTGM
 *
 * This method will return an access mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetAccessModeFromSTGM(DWORD stgm)
{
  switch (STGM_ACCESS_MODE(stgm))
  {
  case STGM_READ:
    return GENERIC_READ;
  case STGM_WRITE:
  case STGM_READWRITE:
    return GENERIC_READ | GENERIC_WRITE;
  }
  ERR("Invalid access mode!\n");
  assert(0);
  return 0;
}

/****************************************************************************
 *      GetCreationModeFromSTGM
 *
 * This method will return a creation mode flag from a STGM value.
 * The STGM value is assumed valid.
 */
static DWORD GetCreationModeFromSTGM(DWORD stgm)
{
  switch(STGM_CREATE_MODE(stgm))
  {
  case STGM_CREATE:
    return CREATE_ALWAYS;
  case STGM_CONVERT:
    FIXME("STGM_CONVERT not implemented!\n");
    return CREATE_NEW;
  case STGM_FAILIFTHERE:
    return CREATE_NEW;
  }
  ERR("Invalid create mode!\n");
  assert(0);
  return 0;
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
static HRESULT OLECONVERT_LoadOLE10(LPOLESTREAM pOleStream, OLECONVERT_OLESTREAM_DATA *pData, BOOL bStrem1)
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
			/* Get the length of the OleTypeName */
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
					pData->pstrOleObjFileName = HeapAlloc(GetProcessHeap(), 0, pData->dwOleObjFileNameLength);
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
				/* Get the Length of the Data */
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
					pData->pData = HeapAlloc(GetProcessHeap(),0,pData->dwDataLength);

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
static HRESULT OLECONVERT_SaveOLE10(OLECONVERT_OLESTREAM_DATA *pData, LPOLESTREAM pOleStream)
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
        /* Set the Length of the OleTypeName */
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
            /* Set the length of the Data */
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
static void OLECONVERT_GetOLE20FromOLE10(LPSTORAGE pDestStorage, const BYTE *pBuffer, DWORD nBufferLength)
{
    HRESULT hRes;
    HANDLE hFile;
    IStorage *pTempStorage;
    DWORD dwNumOfBytesWritten;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];
    static const WCHAR wstrPrefix[] = {'s', 'i', 's', 0};

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
static DWORD OLECONVERT_WriteOLE20ToBuffer(LPSTORAGE pStorage, BYTE **pData)
{
    HANDLE hFile;
    HRESULT hRes;
    DWORD nDataLength = 0;
    IStorage *pTempStorage;
    WCHAR wstrTempDir[MAX_PATH], wstrTempFile[MAX_PATH];
    static const WCHAR wstrPrefix[] = {'s', 'i', 's', 0};

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
            *pData = HeapAlloc(GetProcessHeap(),0,nDataLength);
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
 * Creates the "\001OLE" stream in the IStorage if necessary.
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
    static const WCHAR wstrStreamName[] = {1,'O', 'l', 'e', 0};
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

/* write a string to a stream, preceded by its length */
static HRESULT STREAM_WriteString( IStream *stm, LPCWSTR string )
{
    HRESULT r;
    LPSTR str;
    DWORD len = 0;

    if( string )
        len = WideCharToMultiByte( CP_ACP, 0, string, -1, NULL, 0, NULL, NULL);
    r = IStream_Write( stm, &len, sizeof(len), NULL);
    if( FAILED( r ) )
        return r;
    if(len == 0)
        return r;
    str = CoTaskMemAlloc( len );
    WideCharToMultiByte( CP_ACP, 0, string, -1, str, len, NULL, NULL);
    r = IStream_Write( stm, str, len, NULL);
    CoTaskMemFree( str );
    return r;
}

/* read a string preceded by its length from a stream */
static HRESULT STREAM_ReadString( IStream *stm, LPWSTR *string )
{
    HRESULT r;
    DWORD len, count = 0;
    LPSTR str;
    LPWSTR wstr;

    r = IStream_Read( stm, &len, sizeof(len), &count );
    if( FAILED( r ) )
        return r;
    if( count != sizeof(len) )
        return E_OUTOFMEMORY;

    TRACE("%d bytes\n",len);
    
    str = CoTaskMemAlloc( len );
    if( !str )
        return E_OUTOFMEMORY;
    count = 0;
    r = IStream_Read( stm, str, len, &count );
    if( FAILED( r ) )
        return r;
    if( count != len )
    {
        CoTaskMemFree( str );
        return E_OUTOFMEMORY;
    }

    TRACE("Read string %s\n",debugstr_an(str,len));

    len = MultiByteToWideChar( CP_ACP, 0, str, count, NULL, 0 );
    wstr = CoTaskMemAlloc( (len + 1)*sizeof (WCHAR) );
    if( wstr )
         MultiByteToWideChar( CP_ACP, 0, str, count, wstr, len );
    CoTaskMemFree( str );

    *string = wstr;

    return r;
}


static HRESULT STORAGE_WriteCompObj( LPSTORAGE pstg, CLSID *clsid,
    LPCWSTR lpszUserType, LPCWSTR szClipName, LPCWSTR szProgIDName )
{
    IStream *pstm;
    HRESULT r = S_OK;
    static const WCHAR szwStreamName[] = {1, 'C', 'o', 'm', 'p', 'O', 'b', 'j', 0};

    static const BYTE unknown1[12] =
       { 0x01, 0x00, 0xFE, 0xFF, 0x03, 0x0A, 0x00, 0x00,
         0xFF, 0xFF, 0xFF, 0xFF};
    static const BYTE unknown2[16] =
       { 0xF4, 0x39, 0xB2, 0x71, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    TRACE("%p %s %s %s %s\n", pstg, debugstr_guid(clsid),
           debugstr_w(lpszUserType), debugstr_w(szClipName),
           debugstr_w(szProgIDName));

    /*  Create a CompObj stream if it doesn't exist */
    r = IStorage_CreateStream(pstg, szwStreamName,
        STGM_WRITE  | STGM_SHARE_EXCLUSIVE, 0, 0, &pstm );
    if( FAILED (r) )
        return r;

    /* Write CompObj Structure to stream */
    r = IStream_Write(pstm, unknown1, sizeof(unknown1), NULL);

    if( SUCCEEDED( r ) )
        r = WriteClassStm( pstm, clsid );

    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, lpszUserType );
    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, szClipName );
    if( SUCCEEDED( r ) )
        r = STREAM_WriteString( pstm, szProgIDName );
    if( SUCCEEDED( r ) )
        r = IStream_Write(pstm, unknown2, sizeof(unknown2), NULL);

    IStream_Release( pstm );

    return r;
}

/***********************************************************************
 *               WriteFmtUserTypeStg (OLE32.@)
 */
HRESULT WINAPI WriteFmtUserTypeStg(
	  LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType)
{
    HRESULT r;
    WCHAR szwClipName[0x40];
    CLSID clsid = CLSID_NULL;
    LPWSTR wstrProgID = NULL;
    DWORD n;

    TRACE("(%p,%x,%s)\n",pstg,cf,debugstr_w(lpszUserType));

    /* get the clipboard format name */
    n = GetClipboardFormatNameW( cf, szwClipName, sizeof(szwClipName) );
    szwClipName[n]=0;

    TRACE("Clipboard name is %s\n", debugstr_w(szwClipName));

    /* FIXME: There's room to save a CLSID and its ProgID, but
       the CLSID is not looked up in the registry and in all the
       tests I wrote it was CLSID_NULL.  Where does it come from?
    */

    /* get the real program ID.  This may fail, but that's fine */
    ProgIDFromCLSID(&clsid, &wstrProgID);

    TRACE("progid is %s\n",debugstr_w(wstrProgID));

    r = STORAGE_WriteCompObj( pstg, &clsid, 
                              lpszUserType, szwClipName, wstrProgID );

    CoTaskMemFree(wstrProgID);

    return r;
}


/******************************************************************************
 *              ReadFmtUserTypeStg        [OLE32.@]
 */
HRESULT WINAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT* pcf, LPOLESTR* lplpszUserType)
{
    HRESULT r;
    IStream *stm = 0;
    static const WCHAR szCompObj[] = { 1, 'C','o','m','p','O','b','j', 0 };
    unsigned char unknown1[12];
    unsigned char unknown2[16];
    DWORD count;
    LPWSTR szProgIDName = NULL, szCLSIDName = NULL, szOleTypeName = NULL;
    CLSID clsid;

    TRACE("(%p,%p,%p)\n", pstg, pcf, lplpszUserType);

    r = IStorage_OpenStream( pstg, szCompObj, NULL, 
                    STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm );
    if( FAILED ( r ) )
    {
        WARN("Failed to open stream r = %08x\n", r);
        return r;
    }

    /* read the various parts of the structure */
    r = IStream_Read( stm, unknown1, sizeof(unknown1), &count );
    if( FAILED( r ) || ( count != sizeof(unknown1) ) )
        goto end;
    r = ReadClassStm( stm, &clsid );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szCLSIDName );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szOleTypeName );
    if( FAILED( r ) )
        goto end;

    r = STREAM_ReadString( stm, &szProgIDName );
    if( FAILED( r ) )
        goto end;

    r = IStream_Read( stm, unknown2, sizeof(unknown2), &count );
    if( FAILED( r ) || ( count != sizeof(unknown2) ) )
        goto end;

    /* ok, success... now we just need to store what we found */
    if( pcf )
        *pcf = RegisterClipboardFormatW( szOleTypeName );
    CoTaskMemFree( szOleTypeName );

    if( lplpszUserType )
        *lplpszUserType = szCLSIDName;
    CoTaskMemFree( szProgIDName );

end:
    IStream_Release( stm );

    return r;
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
    static const WCHAR wstrStreamName[] = {1,'C', 'o', 'm', 'p', 'O', 'b', 'j', 0};
    WCHAR bufferW[OLESTREAM_MAX_STR_LEN];

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
        MultiByteToWideChar( CP_ACP, 0, IStorageCompObj.strProgIDName, -1,
                             bufferW, OLESTREAM_MAX_STR_LEN );
        hRes = CLSIDFromProgID(bufferW, &(IStorageCompObj.clsid));

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
                hErr = RegQueryValueA(hKey, NULL, strTemp, (LONG*) &(IStorageCompObj.dwCLSIDNameLength));
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
static void OLECONVERT_CreateOlePresStream(LPSTORAGE pStorage, DWORD dwExtentX, DWORD dwExtentY , BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;
    static const WCHAR wstrStreamName[] = {2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};
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

        /* Set Data and Length */
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
static void OLECONVERT_CreateOle10NativeStream(LPSTORAGE pStorage, const BYTE *pData, DWORD dwDataLength)
{
    HRESULT hRes;
    IStream *pStream;
    static const WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};

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
static HRESULT OLECONVERT_GetOLE10ProgID(LPSTORAGE pStorage, char *strProgID, DWORD *dwSize)
{
    HRESULT hRes;
    IStream *pStream;
    LARGE_INTEGER iSeekPos;
    OLECONVERT_ISTORAGE_COMPOBJ CompObj;
    static const WCHAR wstrStreamName[] = {1,'C', 'o', 'm', 'p', 'O', 'b', 'j', 0};

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
static void OLECONVERT_GetOle10PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{

    HRESULT hRes;
    IStream *pStream;
    static const WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};

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
            pOleStreamData->pData = HeapAlloc(GetProcessHeap(),0,pOleStreamData->dwDataLength);
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
static void OLECONVERT_GetOle20PresData(LPSTORAGE pStorage, OLECONVERT_OLESTREAM_DATA *pOleStreamData)
{
    HRESULT hRes;
    IStream *pStream;
    OLECONVERT_ISTORAGE_OLEPRES olePress;
    static const WCHAR wstrStreamName[] = {2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};

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
        static const char strMetafilePictName[] = "METAFILEPICT";

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
            pOleStreamData[1].pData = HeapAlloc(GetProcessHeap(),0,pOleStreamData[1].dwDataLength);
            memcpy(pOleStreamData[1].pData, &MetaFilePict, sizeof(MetaFilePict));
            IStream_Read(pStream, &(pOleStreamData[1].pData[sizeof(MetaFilePict)]), pOleStreamData[1].dwDataLength-sizeof(METAFILEPICT16), NULL);
        }
        IStream_Release(pStream);
    }
}

/*************************************************************************
 * OleConvertOLESTREAMToIStorage [OLE32.@]
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

    TRACE("%p %p %p\n", pOleStream, pstg, ptd);

    memset(pOleStreamData, 0, sizeof(pOleStreamData));

    if(ptd != NULL)
    {
        FIXME("DVTARGETDEVICE is not NULL, unhandled parameter\n");
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
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pstrOleObjFileName);
        pOleStreamData[i].pstrOleObjFileName = NULL;
    }
    return hRes;
}

/*************************************************************************
 * OleConvertIStorageToOLESTREAM [OLE32.@]
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
    static const WCHAR wstrStreamName[] = {1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0};

    TRACE("%p %p\n", pstg, pOleStream);

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
        /* Was it originally Ole10 */
        hRes = IStorage_OpenStream(pstg, wstrStreamName, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
        if(hRes == S_OK)
        {
            IStream_Release(pStream);
            /* Get Presentation Data for Ole10Native */
            OLECONVERT_GetOle10PresData(pstg, pOleStreamData);
        }
        else
        {
            /* Get Presentation Data (OLE20) */
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
        HeapFree(GetProcessHeap(),0,pOleStreamData[i].pData);
    }

    return hRes;
}

/***********************************************************************
 *		GetConvertStg (OLE32.@)
 */
HRESULT WINAPI GetConvertStg(IStorage *stg) {
    FIXME("unimplemented stub!\n");
    return E_FAIL;
}

/******************************************************************************
 * StgIsStorageFile [OLE32.@]
 * Verify if the file contains a storage object
 *
 * PARAMS
 *  fn      [ I] Filename
 *
 * RETURNS
 *  S_OK    if file has magic bytes as a storage object
 *  S_FALSE if file is not storage
 */
HRESULT WINAPI
StgIsStorageFile(LPCOLESTR fn)
{
	HANDLE		hf;
	BYTE		magic[8];
	DWORD		bytes_read;

	TRACE("%s\n", debugstr_w(fn));
	hf = CreateFileW(fn, GENERIC_READ,
	                 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
	                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hf == INVALID_HANDLE_VALUE)
		return STG_E_FILENOTFOUND;

	if (!ReadFile(hf, magic, 8, &bytes_read, NULL))
	{
		WARN(" unable to read file\n");
		CloseHandle(hf);
		return S_FALSE;
	}

	CloseHandle(hf);

	if (bytes_read != 8) {
		WARN(" too short\n");
		return S_FALSE;
	}

	if (!memcmp(magic,STORAGE_magic,8)) {
		WARN(" -> YES\n");
		return S_OK;
	}

	WARN(" -> Invalid header.\n");
	return S_FALSE;
}

/***********************************************************************
 *		WriteClassStm (OLE32.@)
 *
 * Writes a CLSID to a stream.
 *
 * PARAMS
 *  pStm   [I] Stream to write to.
 *  rclsid [I] CLSID to write.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid)
{
    TRACE("(%p,%p)\n",pStm,rclsid);

    if (!pStm || !rclsid)
        return E_INVALIDARG;

    return IStream_Write(pStm,rclsid,sizeof(CLSID),NULL);
}

/***********************************************************************
 *		ReadClassStm (OLE32.@)
 *
 * Reads a CLSID from a stream.
 *
 * PARAMS
 *  pStm   [I] Stream to read from.
 *  rclsid [O] CLSID to read.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI ReadClassStm(IStream *pStm,CLSID *pclsid)
{
    ULONG nbByte;
    HRESULT res;

    TRACE("(%p,%p)\n",pStm,pclsid);

    if (!pStm || !pclsid)
        return E_INVALIDARG;

    /* clear the output args */
    memcpy(pclsid, &CLSID_NULL, sizeof(*pclsid));

    res = IStream_Read(pStm,(void*)pclsid,sizeof(CLSID),&nbByte);

    if (FAILED(res))
        return res;

    if (nbByte != sizeof(CLSID))
        return STG_E_READFAULT;
    else
        return S_OK;
}
