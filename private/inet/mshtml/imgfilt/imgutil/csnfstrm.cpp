#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "csnfstrm.h"

CSniffStream::CSniffStream() :
   m_pbBuffer( NULL ),
   m_iOffset( 0 ),
   m_nBufferSize( 0 ),
   m_nValidBytes( 0 ),
   m_iNextFreeByte( 0 )
{
}

CSniffStream::~CSniffStream()
{
   delete m_pbBuffer;
}

STDMETHODIMP CSniffStream::Clone( IStream** ppStream )
{
   if( ppStream == NULL )
   {
      return( E_POINTER );
   }

   *ppStream = NULL;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Commit( DWORD dwFlags )
{
   (void)dwFlags;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::CopyTo( IStream* pStream, ULARGE_INTEGER nBytes,
   ULARGE_INTEGER* pnBytesRead, ULARGE_INTEGER* pnBytesWritten )
{
   (void)pStream;
   (void)nBytes;
   (void)pnBytesRead;
   (void)pnBytesWritten;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::LockRegion( ULARGE_INTEGER iOffset, 
   ULARGE_INTEGER nBytes, DWORD dwLockType )
{
   (void)iOffset;
   (void)nBytes;
   (void)dwLockType;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Read( void* pBuffer, ULONG nBytes, 
   ULONG* pnBytesRead )
{
   ULONG nBytesToRead;
   ULONG nBytesLeft;
   ULONG nBytesRead;
   BYTE* pbBuffer;
   HRESULT hResult;

   if( pnBytesRead != NULL )
   {
      *pnBytesRead = 0;
   }

   if( pBuffer == NULL )
   {
      return( E_POINTER );
   }

   if( nBytes == 0 )
   {
      return( E_INVALIDARG );
   }

   pbBuffer = LPBYTE( pBuffer );
   nBytesLeft = nBytes;
   if( m_pbBuffer != NULL )
   {
      _ASSERTE( m_nValidBytes > 0 );

      nBytesToRead = min( nBytesLeft, m_nValidBytes );
      memcpy( pbBuffer, &m_pbBuffer[m_iOffset], nBytesToRead );
      nBytesLeft -= nBytesToRead;
      if( pnBytesRead != NULL )
      {
         *pnBytesRead += nBytesToRead;
      }
      m_nValidBytes -= nBytesToRead;
      pbBuffer += nBytesToRead;
      m_iOffset += nBytesToRead;
      if( m_nValidBytes == 0 )
      {
         delete m_pbBuffer;
         m_pbBuffer = NULL;
         m_nValidBytes = 0;
         m_iNextFreeByte = 0;
         m_iOffset = 0;
         m_nBufferSize = 0;
      }
   }

   if( nBytesLeft == 0 )
   {
      return( S_OK );
   }

   _ASSERTE( m_pbBuffer == NULL );
   hResult = m_pStream->Read( pbBuffer, nBytesLeft, &nBytesRead );
   if( pnBytesRead != NULL )
   {
      *pnBytesRead += nBytesRead;
   }

   return( hResult );
}

STDMETHODIMP CSniffStream::Revert()
{
   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Seek( LARGE_INTEGER nDisplacement, DWORD dwOrigin,
   ULARGE_INTEGER* piNewPosition )
{
   (void)nDisplacement;
   (void)dwOrigin;
   (void)piNewPosition;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::SetSize( ULARGE_INTEGER nNewSize )
{
   (void)nNewSize;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Stat( STATSTG* pStatStg, DWORD dwFlags )
{
   (void)pStatStg;
   (void)dwFlags;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::UnlockRegion( ULARGE_INTEGER iOffset, 
   ULARGE_INTEGER nBytes, DWORD dwLockType )
{
   (void)iOffset;
   (void)nBytes;
   (void)dwLockType;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Write( const void* pBuffer, ULONG nBytes, 
   ULONG* pnBytesWritten )
{
   (void)pBuffer;
   (void)nBytes;
   (void)pnBytesWritten;

   return( E_NOTIMPL );
}

STDMETHODIMP CSniffStream::Init( IStream* pStream )
{
   if( pStream == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pStream = pStream;

   return( S_OK );
}

STDMETHODIMP CSniffStream::Peek( void* pBuffer, ULONG nBytes, 
   ULONG* pnBytesRead )
{
   BYTE* pbNewBuffer;
   HRESULT hResult;
   ULONG nBytesToRead;
   ULONG nBytesRead;

   if( pnBytesRead != NULL )
   {
      *pnBytesRead = 0;
   }

   if( pBuffer == NULL )
   {
      return( E_POINTER );
   }
   if( nBytes == 0 )
   {
      return( E_INVALIDARG );
   }

   hResult = S_OK;

   if( nBytes > m_nValidBytes )
   {
      // We have to read from the stream

      if( nBytes > (m_nBufferSize-m_iOffset) )
      {
         // We need more buffer space
         pbNewBuffer = new BYTE[nBytes];
         if( pbNewBuffer == NULL )
         {
            return( E_OUTOFMEMORY );
         }

         if( m_pbBuffer != NULL )
         {
            memcpy( pbNewBuffer, &m_pbBuffer[m_iOffset], m_nValidBytes );
         }
         delete m_pbBuffer;
         m_pbBuffer = pbNewBuffer;

         m_nBufferSize = nBytes;
         m_iOffset = 0;
         m_iNextFreeByte = m_nValidBytes;
      }

      hResult = m_pStream->Read( &m_pbBuffer[m_iNextFreeByte], 
         nBytes-m_nValidBytes, &nBytesRead );
      m_iNextFreeByte += nBytesRead;
      m_nValidBytes += nBytesRead;
   }

   // Copy as much as we can from our buffer
   nBytesToRead = min( nBytes, m_nValidBytes );
   if( nBytesToRead > 0 )
   {
      memcpy( pBuffer, &m_pbBuffer[m_iOffset], nBytesToRead );
      if( pnBytesRead != NULL )
      {
         *pnBytesRead += nBytesToRead;
      }
   }

   if( nBytesToRead == nBytes )
   {
      return( S_OK );
   }
   else
   {
      return( hResult );
   }
}

