/*
 * Compound Storage (32 bit version)
 * Stream implementation
 *
 * This file contains the implementation of the stream interface
 * for streams contained in a compound storage.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Thuy Nguyen
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <ddk/ntddk.h>
#include <ole32/ole32.h>
#include "storage32.h"

#include <debug.h>


/*
 * Virtual function table for the StgStreamImpl class.
 */
static ICOM_VTABLE(IStream) StgStreamImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    StgStreamImpl_QueryInterface,
    StgStreamImpl_AddRef,
    StgStreamImpl_Release,
    StgStreamImpl_Read,
    StgStreamImpl_Write,
    StgStreamImpl_Seek,
    StgStreamImpl_SetSize,
    StgStreamImpl_CopyTo,
    StgStreamImpl_Commit,
    StgStreamImpl_Revert,
    StgStreamImpl_LockRegion,
    StgStreamImpl_UnlockRegion,
    StgStreamImpl_Stat,
    StgStreamImpl_Clone
};

/******************************************************************************
** StgStreamImpl implementation
*/

/***
 * This is the constructor for the StgStreamImpl class.
 *
 * Params:
 *    parentStorage - Pointer to the storage that contains the stream to open
 *    ownerProperty - Index of the property that points to this stream.
 */
StgStreamImpl* StgStreamImpl_Construct(
		StorageBaseImpl* parentStorage,
    DWORD            grfMode,
    ULONG            ownerProperty)
{
  StgStreamImpl* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(StgStreamImpl));
  
  if (newStream!=0)
  {
    /*
     * Set-up the virtual function table and reference count.
     */
    ICOM_VTBL(newStream) = &StgStreamImpl_Vtbl;
    newStream->ref       = 0;
    
    /*
     * We want to nail-down the reference to the storage in case the
     * stream out-lives the storage in the client application.
     */
    newStream->parentStorage = parentStorage;
    IStorage_AddRef((IStorage*)newStream->parentStorage);

    newStream->grfMode = grfMode;    
    newStream->ownerProperty = ownerProperty;
    
    /*
     * Start the stream at the begining.
     */
    newStream->currentPosition.u.HighPart = 0;
    newStream->currentPosition.u.LowPart = 0;
    
    /*
     * Initialize the rest of the data.
     */
    newStream->streamSize.u.HighPart = 0;
    newStream->streamSize.u.LowPart  = 0;
    newStream->bigBlockChain       = 0;
    newStream->smallBlockChain     = 0;
    
    /*
     * Read the size from the property and determine if the blocks forming
     * this stream are large or small.
     */
    StgStreamImpl_OpenBlockChain(newStream);
  }
  
  return newStream;
}

/***
 * This is the destructor of the StgStreamImpl class.
 *
 * This method will clean-up all the resources used-up by the given StgStreamImpl 
 * class. The pointer passed-in to this function will be freed and will not
 * be valid anymore.
 */
void StgStreamImpl_Destroy(StgStreamImpl* This)
{
  Print(MAX_TRACE, ("(%p)\n", This));

  /*
   * Release the reference we are holding on the parent storage.
   */
  IStorage_Release((IStorage*)This->parentStorage);
  This->parentStorage = 0;

  /*
   * Make sure we clean-up the block chain stream objects that we were using.
   */
  if (This->bigBlockChain != 0)
  {
    BlockChainStream_Destroy(This->bigBlockChain);
    This->bigBlockChain = 0;
  }

  if (This->smallBlockChain != 0)
  {
    SmallBlockChainStream_Destroy(This->smallBlockChain);
    This->smallBlockChain = 0;
  }

  /*
   * Finally, free the memory used-up by the class.
   */
  HeapFree(GetProcessHeap(), 0, This);  
}

/***
 * This implements the IUnknown method QueryInterface for this
 * class
 */
HRESULT WINAPI StgStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */          
		  void**         ppvObject)   /* [iid_is][out] */ 
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

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
    *ppvObject = (IStream*)This;
  }
  else if (memcmp(&IID_IStream, riid, sizeof(IID_IStream)) == 0) 
  {
    *ppvObject = (IStream*)This;
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
  StgStreamImpl_AddRef(iface);
  
  return S_OK;;
}

/***
 * This implements the IUnknown method AddRef for this
 * class
 */
ULONG WINAPI StgStreamImpl_AddRef(
		IStream* iface)
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  This->ref++;
  
  return This->ref;
}

/***
 * This implements the IUnknown method Release for this
 * class
 */
ULONG WINAPI StgStreamImpl_Release(
		IStream* iface)
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  ULONG newRef;
  
  This->ref--;
  
  newRef = This->ref;
  
  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (newRef==0)
  {
    StgStreamImpl_Destroy(This);
  }
  
  return newRef;
}

/***
 * This method will open the block chain pointed by the property
 * that describes the stream.
 * If the stream's size is null, no chain is opened.
 */
void StgStreamImpl_OpenBlockChain(
        StgStreamImpl* This)
{
  StgProperty    curProperty;
  BOOL         readSucessful;

  /*
   * Make sure no old object is staying behind.
   */
  if (This->smallBlockChain != 0)
  {
    SmallBlockChainStream_Destroy(This->smallBlockChain);
    This->smallBlockChain = 0;
  }

  if (This->bigBlockChain != 0)
  {
    BlockChainStream_Destroy(This->bigBlockChain);
    This->bigBlockChain = 0;
  }

  /*
   * Read the information from the property.
   */
  readSucessful = StorageImpl_ReadProperty(This->parentStorage->ancestorStorage,
					     This->ownerProperty,
					     &curProperty);
  
  if (readSucessful)
  {
    This->streamSize = curProperty.size;
    
    /*
     * This code supports only streams that are <32 bits in size.
     */
    assert(This->streamSize.u.HighPart == 0);
    
    if(curProperty.startingBlock == BLOCK_END_OF_CHAIN)
    {
      assert( (This->streamSize.u.HighPart == 0) && (This->streamSize.u.LowPart == 0) );
    }
    else
    {
      if ( (This->streamSize.u.HighPart == 0) &&
	   (This->streamSize.u.LowPart < LIMIT_TO_USE_SMALL_BLOCK) )
      {
	This->smallBlockChain = SmallBlockChainStream_Construct(
								This->parentStorage->ancestorStorage,	
								This->ownerProperty);
      }
      else
      {
	This->bigBlockChain = BlockChainStream_Construct(
							 This->parentStorage->ancestorStorage,
							 NULL,
							 This->ownerProperty);
      }
    }
  }
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * If reads a block of information from the stream at the current
 * position. It then moves the current position at the end of the
 * read block
 *
 * See the documentation of ISequentialStream for more info.
 */
HRESULT WINAPI StgStreamImpl_Read( 
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */                     
		  ULONG*         pcbRead)   /* [out] */                    
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  ULONG bytesReadBuffer;
  ULONG bytesToReadFromBuffer;

  Print(MAX_TRACE, ("(%p, %p, %ld, %p)\n",
	iface, pv, cb, pcbRead));

  /* 
   * If the caller is not interested in the nubmer of bytes read,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;
  
  /*
   * Using the known size of the stream, calculate the number of bytes
   * to read from the block chain
   */
  bytesToReadFromBuffer = min( This->streamSize.u.LowPart - This->currentPosition.u.LowPart, cb);
  
  /*
   * Depending on the type of chain that was opened when the stream was constructed,
   * we delegate the work to the method that read the block chains.
   */
  if (This->smallBlockChain!=0)
  {
    SmallBlockChainStream_ReadAt(This->smallBlockChain,
				 This->currentPosition,
				 bytesToReadFromBuffer,
				 pv,
				 pcbRead);
    
  }
  else if (This->bigBlockChain!=0)
  {
    BlockChainStream_ReadAt(This->bigBlockChain,
			    This->currentPosition,
			    bytesToReadFromBuffer,
			    pv,
			    pcbRead);
  }
  else
  {
    /*
     * Small and big block chains are both NULL. This case will happen
     * when a stream starts with BLOCK_END_OF_CHAIN and has size zero.
     */

    *pcbRead = 0;
    return S_OK;
  }

  /*
   * We should always be able to read the proper amount of data from the
   * chain.
   */
  assert(bytesToReadFromBuffer == *pcbRead);

  /*
   * Advance the pointer for the number of positions read.
   */
  This->currentPosition.u.LowPart += *pcbRead;
  
  /*
   * The function returns S_OK if the buffer was filled completely
   * it returns S_FALSE if the end of the stream is reached before the
   * buffer is filled
   */
  if(*pcbRead == cb)
    return S_OK;
  
  return S_FALSE;
}
        
/***
 * This method is part of the ISequentialStream interface.
 *
 * It writes a block of information to the stream at the current
 * position. It then moves the current position at the end of the
 * written block. If the stream is too small to fit the block,
 * the stream is grown to fit.
 *
 * See the documentation of ISequentialStream for more info.
 */
HRESULT WINAPI StgStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */ 
		  ULONG          cb,          /* [in] */          
		  ULONG*         pcbWritten)  /* [out] */         
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  ULARGE_INTEGER newSize;
  ULONG bytesWritten = 0;

  Print(MAX_TRACE, ("(%p, %p, %ld, %p)\n",
	iface, pv, cb, pcbWritten));
 
  /*
   * Do we have permission to write to this stream?
   */
  if (!(This->grfMode & (STGM_WRITE | STGM_READWRITE))) {
      return STG_E_ACCESSDENIED;
  }

  /*
   * If the caller is not interested in the number of bytes written,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;
  
  /*
   * Initialize the out parameter
   */
  *pcbWritten = 0;

  if (cb == 0)
  {
    return S_OK;
  }
  else
  {
    newSize.u.HighPart = 0;
    newSize.u.LowPart = This->currentPosition.u.LowPart + cb;
  }
  
  /*
   * Verify if we need to grow the stream
   */
  if (newSize.u.LowPart > This->streamSize.u.LowPart)
  {
    /* grow stream */
    IStream_SetSize(iface, newSize);
  }
  
  /*
   * Depending on the type of chain that was opened when the stream was constructed,
   * we delegate the work to the method that readwrites to the block chains.
   */
  if (This->smallBlockChain!=0)
  {
    SmallBlockChainStream_WriteAt(This->smallBlockChain,
				  This->currentPosition,
				  cb,
				  pv,
				  pcbWritten);
    
  }
  else if (This->bigBlockChain!=0)
  {
    BlockChainStream_WriteAt(This->bigBlockChain,
			     This->currentPosition,
			     cb,
			     pv,
			     pcbWritten);
  }
  else
    assert(FALSE);
  
  /*
   * Advance the position pointer for the number of positions written.
   */
  This->currentPosition.u.LowPart += *pcbWritten;
  
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will move the current stream pointer according to the parameters
 * given.
 *
 * See the documentation of IStream for more info.
 */        
HRESULT WINAPI StgStreamImpl_Seek( 
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */ 
		  DWORD           dwOrigin,         /* [in] */ 
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  ULARGE_INTEGER newPosition;
  LARGE_INTEGER Position;

  Print(MAX_TRACE, ("(%p, %ld, %ld, %p)\n",
	iface, dlibMove.u.LowPart, dwOrigin, plibNewPosition));

  /* 
   * The caller is allowed to pass in NULL as the new position return value.
   * If it happens, we assign it to a dynamic variable to avoid special cases
   * in the code below.
   */
  if (plibNewPosition == 0)
  {
    plibNewPosition = &newPosition;
  }

  /*
   * The file pointer is moved depending on the given "function"
   * parameter.
   */
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      plibNewPosition->u.HighPart = 0;
      plibNewPosition->u.LowPart  = 0;
      break;
    case STREAM_SEEK_CUR:
      *plibNewPosition = This->currentPosition;
      break;
    case STREAM_SEEK_END:
      *plibNewPosition = This->streamSize;
      break;
    default:
      return STG_E_INVALIDFUNCTION;
  }

  Position = RtlLargeIntegerAdd( *((PLARGE_INTEGER)plibNewPosition), dlibMove );

  plibNewPosition->QuadPart = Position.QuadPart;

  /*
   * tell the caller what we calculated
   */
  This->currentPosition = *plibNewPosition;
 
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will change the size of a stream.
 *
 * TODO: Switch from small blocks to big blocks and vice versa.
 *
 * See the documentation of IStream for more info.
 */
HRESULT WINAPI StgStreamImpl_SetSize( 
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */ 
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  StgProperty    curProperty;
  BOOL         Success;

  Print(MAX_TRACE, ("(%p, %ld)\n", iface, libNewSize.u.LowPart));

  /*
   * As documented.
   */
  if (libNewSize.u.HighPart != 0)
    return STG_E_INVALIDFUNCTION;

  /*
   * Do we have permission?
   */
  if (!(This->grfMode & (STGM_WRITE | STGM_READWRITE)))
    return STG_E_ACCESSDENIED;

  if (This->streamSize.u.LowPart == libNewSize.u.LowPart)
    return S_OK;

  /*
   * This will happen if we're creating a stream
   */
  if ((This->smallBlockChain == 0) && (This->bigBlockChain == 0))
  {
    if (libNewSize.u.LowPart < LIMIT_TO_USE_SMALL_BLOCK)
    {
      This->smallBlockChain = SmallBlockChainStream_Construct(
                                    This->parentStorage->ancestorStorage,
                                    This->ownerProperty);
    }
    else
    {
      This->bigBlockChain = BlockChainStream_Construct(
                                This->parentStorage->ancestorStorage,
                                NULL,
                                This->ownerProperty);
    }
  }

  /*
   * Read this stream's property to see if it's small blocks or big blocks
   */
  Success = StorageImpl_ReadProperty(This->parentStorage->ancestorStorage,
                                       This->ownerProperty,
                                       &curProperty); 
  /*
   * Determine if we have to switch from small to big blocks or vice versa
   */  
  if ( (This->smallBlockChain!=0) && 
       (curProperty.size.u.LowPart < LIMIT_TO_USE_SMALL_BLOCK) )
  {
    if (libNewSize.u.LowPart >= LIMIT_TO_USE_SMALL_BLOCK)
    {
      /*
       * Transform the small block chain into a big block chain
       */
      This->bigBlockChain = Storage32Impl_SmallBlocksToBigBlocks(
                                This->parentStorage->ancestorStorage,
                                &This->smallBlockChain);
    }
  }

  if (This->smallBlockChain!=0)
  {
    Success = SmallBlockChainStream_SetSize(This->smallBlockChain, libNewSize);
  }
  else
  {
    Success = BlockChainStream_SetSize(This->bigBlockChain, libNewSize);
  }

  /*
   * Write to the property the new information about this stream
   */
  Success = StorageImpl_ReadProperty(This->parentStorage->ancestorStorage,
                                       This->ownerProperty,
                                       &curProperty);

  curProperty.size.u.HighPart = libNewSize.u.HighPart;
  curProperty.size.u.LowPart = libNewSize.u.LowPart;
  
  if (Success)
  {
    StorageImpl_WriteProperty(This->parentStorage->ancestorStorage,
				This->ownerProperty,
				&curProperty);
  }
  
  This->streamSize = libNewSize;
  
  return S_OK;
}
        
/***
 * This method is part of the IStream interface.
 *
 * It will copy the 'cb' Bytes to 'pstm' IStream.
 *
 * See the documentation of IStream for more info.
 */
HRESULT WINAPI StgStreamImpl_CopyTo( 
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */ 
				    ULARGE_INTEGER  cb,           /* [in] */         
				    ULARGE_INTEGER* pcbRead,      /* [out] */        
				    ULARGE_INTEGER* pcbWritten)   /* [out] */        
{
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  Print(MAX_TRACE, ("(%p, %p, %ld, %p, %p)\n", 
	iface, pstm, cb.u.LowPart, pcbRead, pcbWritten));

  /*
   * Sanity check
   */
  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;

  totalBytesRead.u.LowPart = totalBytesRead.u.HighPart = 0;
  totalBytesWritten.u.LowPart = totalBytesWritten.u.HighPart = 0;

  /*
   * use stack to store data temporarly
   * there is surely more performant way of doing it, for now this basic
   * implementation will do the job
   */
  while ( cb.u.LowPart > 0 )
  {
    if ( cb.u.LowPart >= 128 )
      copySize = 128;
    else
      copySize = cb.u.LowPart;
    
    IStream_Read(iface, tmpBuffer, copySize, &bytesRead);

    totalBytesRead.u.LowPart += bytesRead;
    
    IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);

    totalBytesWritten.u.LowPart += bytesWritten;

    /*
     * Check that read & write operations were succesfull
     */
    if (bytesRead != bytesWritten)
    {
      hr = STG_E_MEDIUMFULL;
      break;
    }
    
    if (bytesRead!=copySize)
      cb.u.LowPart = 0;
    else
      cb.u.LowPart -= bytesRead;
  }

  /*
   * Update number of bytes read and written
   */
  if (pcbRead)
  {
    pcbRead->u.LowPart = totalBytesRead.u.LowPart;
    pcbRead->u.HighPart = totalBytesRead.u.HighPart;
  }

  if (pcbWritten)
  {
    pcbWritten->u.LowPart = totalBytesWritten.u.LowPart;
    pcbWritten->u.HighPart = totalBytesWritten.u.HighPart;
  }
  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams contained in structured storages, this method
 * does nothing. This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */        
HRESULT WINAPI StgStreamImpl_Commit( 
		  IStream*      iface,
		  DWORD           grfCommitFlags)  /* [in] */ 
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams contained in structured storages, this method
 * does nothing. This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */        
HRESULT WINAPI StgStreamImpl_Revert( 
		  IStream* iface)
{
  return S_OK;
}

HRESULT WINAPI StgStreamImpl_LockRegion( 
					IStream*     iface,
					ULARGE_INTEGER libOffset,   /* [in] */ 
					ULARGE_INTEGER cb,          /* [in] */ 
					DWORD          dwLockType)  /* [in] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

HRESULT WINAPI StgStreamImpl_UnlockRegion( 
					  IStream*     iface,
					  ULARGE_INTEGER libOffset,   /* [in] */ 
					  ULARGE_INTEGER cb,          /* [in] */ 
					  DWORD          dwLockType)  /* [in] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}

/***
 * This method is part of the IStream interface.
 *
 * This method returns information about the current
 * stream.
 *
 * See the documentation of IStream for more info.
 */        
HRESULT WINAPI StgStreamImpl_Stat( 
		  IStream*     iface,
		  STATSTG*       pstatstg,     /* [out] */
		  DWORD          grfStatFlag)  /* [in] */ 
{
  StgStreamImpl* const This=(StgStreamImpl*)iface;

  StgProperty    curProperty;
  BOOL         readSucessful;
  
  /*
   * Read the information from the property.
   */
  readSucessful = StorageImpl_ReadProperty(This->parentStorage->ancestorStorage,
					     This->ownerProperty,
					     &curProperty);
  
  if (readSucessful)
  {
    StorageUtl_CopyPropertyToSTATSTG(pstatstg, 
				     &curProperty, 
				     grfStatFlag);

    pstatstg->grfMode = This->grfMode;
    
    return S_OK;
  }
  
  return E_FAIL;
}
        
HRESULT WINAPI StgStreamImpl_Clone( 
				   IStream*     iface,
				   IStream**    ppstm) /* [out] */ 
{
  UNIMPLEMENTED;
  return E_NOTIMPL;
}
