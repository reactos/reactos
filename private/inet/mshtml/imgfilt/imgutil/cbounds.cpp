#include "stdafx.h"
#include "imgutil.h"
#include "resource.h"
#include "cbounds.h"

CBounds::CBounds() :
   m_pUnkMarshal( NULL ),
   m_nLeft( 0 ),
   m_nTop( 0 ),
   m_nRight( 0 ),
   m_nBottom( 0 ),
   m_iFirstFrame( 0 ),
   m_iLastFrame( 0 )
{
}

CBounds::~CBounds()
{
   if( m_pUnkMarshal != NULL )
   {
      m_pUnkMarshal->Release();
   }
}

HRESULT CBounds::FinalConstruct()
{
   HRESULT hResult;

   hResult = CoCreateFreeThreadedMarshaler( GetUnknown(), &m_pUnkMarshal );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDMETHODIMP CBounds::Clone( IBounds** ppResult )
{
   HRESULT hResult;

   if( ppResult == NULL )
   {
      return( E_POINTER );
   }

   hResult = CComCreator< CComObject< CBounds > >::CreateInstance( NULL,
      IID_IBounds, (void**)ppResult );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   (*ppResult)->SetRect( m_nLeft, m_nTop, m_nRight, m_nBottom );

   return( S_OK );
}

STDMETHODIMP CBounds::CopyFromBounds( IBounds* pBounds )
{
   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }

   return( E_NOTIMPL );
}

STDMETHODIMP CBounds::IsEmpty()
{
   return( E_NOTIMPL );
}

STDMETHODIMP CBounds::CompareBounds( IBounds* pBounds )
{
   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }

   return( E_NOTIMPL );
}

STDMETHODIMP CBounds::GetRect( LONG* pnLeft, LONG* pnTop, LONG* pnRight,
   LONG* pnBottom )
{
   if( (pnLeft == NULL) || (pnTop == NULL) || (pnRight == NULL) || 
      (pnBottom == NULL) )
   {
      return( E_POINTER );
   }

   *pnLeft = m_nLeft;
   *pnTop = m_nTop;
   *pnRight = m_nRight;
   *pnBottom = m_nBottom;

   return( S_OK );
}

STDMETHODIMP CBounds::SetRect( LONG nLeft, LONG nTop, LONG nRight, 
   LONG nBottom )
{
   m_nLeft = nLeft;
   m_nTop = nTop;
   m_nRight = nRight;
   m_nBottom = nBottom;

   return( S_OK );
}

STDMETHODIMP CBounds::IntersectBounds( IBounds* pBounds, IBounds** ppResult )
{
   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }
   if( ppResult == NULL )
   {
      return( E_POINTER );
   }

   return( E_NOTIMPL );
}

STDMETHODIMP CBounds::UnionBounds( IBounds* pBounds, IBounds** ppResult )
{
   if( pBounds == NULL )
   {
      return( E_INVALIDARG );
   }
   if( ppResult == NULL )
   {
      return( E_POINTER );
   }

   return( E_NOTIMPL );
}

STDMETHODIMP CBounds::GetRange( ULONG* piFirstFrame, ULONG* piLastFrame )
{
   if( (piFirstFrame == NULL) || (piLastFrame == NULL) )
   {
      return( E_POINTER );
   }

   *piFirstFrame = m_iFirstFrame;
   *piLastFrame = m_iLastFrame;

   return( S_OK );
}

STDMETHODIMP CBounds::SetRange( ULONG iFirstFrame, ULONG iLastFrame )
{
   if( iLastFrame > iFirstFrame )
   {
      return( E_INVALIDARG );
   }

   m_iFirstFrame = iFirstFrame;
   m_iLastFrame = iLastFrame;

   return( S_OK );
}
