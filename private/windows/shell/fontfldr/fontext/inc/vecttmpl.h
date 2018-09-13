/**********************************************************************
 * VectTmpl.h
 *
 * Template for a vector class.
 *
 **********************************************************************/


#if !defined(__VECTTMPL_H__)
#define __VECTTMPL_H__

#ifndef __DBUTL_H__
#include "dbutl.h"
#endif


const int kNotFound = -1;

//
//  The CIVector class is a vector of pointers to objects (The I is for
//  Indirect).
//

template <class T>
class CIVector {
public:
    CIVector( int iSize );
    ~CIVector( );
    
    int bInit( );
    
    //
    //  The real array functions.
    //

    int   iCount( void );
    int   bAdd( T * t );
    T *   poObjectAt( int idx );
    T *   poDetach( int idx );
    int   bDelete( int idx );
    int   bDelete( T * t );
    void  vDeleteAll( );
    int   iFind( T * t );
    
private:
    T **  m_pData;
    int   m_iSize;
    int   m_iCount;

};

////////////////////////////////////////////////////////////////////////////
template <class T> 
CIVector<T>::CIVector(int iSize)
   :  m_pData(0),
      m_iSize(iSize),
      m_iCount(0)
{}

template <class T> CIVector<T>::~CIVector()
{
    ASSERT( this );
    vDeleteAll( );
    delete [] m_pData;
}

template <class T> int CIVector<T>::bInit()
{
    ASSERT( this );
    int   iRet = 0;
    
    if( !m_pData && (m_iSize > 0) )
    {
        m_pData = new T * [ m_iSize ];

        if( m_pData )
            return 1;
    }
    
    //
    //  Error. Clean up.
    //

    delete this;
    
    return 0;
}


template <class T> int CIVector<T>::iCount(void)
{
    ASSERT( this );
    return m_iCount;
}


template <class T> int CIVector<T>::bAdd(T * t)
{
    ASSERT( this );

#ifdef _DEBUG
    if( !m_pData )
    {
        OutputDebugString( TEXT( "CIVector::bAdd()  m_pData not allocated" ) );
        DebugBreak();
    }

    if( ! t )
        DebugBreak();
#endif

    if ( m_iCount < m_iSize )
    {
        m_pData[ m_iCount++ ] = t;
        return 1;
    }

    return 0;
}

template <class T> T * CIVector<T>::poObjectAt(int idx)
{
    ASSERT( this );

    if( idx >= 0 && idx < m_iCount )
    {
        ASSERT( m_pData[ idx ] );

        return m_pData[ idx ];
    }
    return 0;
}

template <class T> T * CIVector<T>::poDetach(int idx)
{
    ASSERT( this );

    if( idx >= 0 && idx < m_iCount )
    {
        ASSERT( m_pData );

        T * pID = m_pData[ idx ];

        //
        //  Move the last one down
        //

        m_pData[ idx ] = m_pData[ --m_iCount ];

#ifdef _DEBUG
        //
        //  When debugging, put a 0 in the slot.
        //

        m_pData[ m_iCount ] = (T *) 0;
#endif

        ASSERT ( pID );

        return pID;
    }

    return (T *)0;
}

template <class T> int CIVector<T>::bDelete( int idx )
{
    ASSERT( this );
    
    T * pID = poDetach( idx );
    
    if ( pID )
    {
        delete pID;
        return 1;
    }
    
    return 0;
}

template <class T> int CIVector<T>::bDelete( T * t )
{
    ASSERT( this );

    return bDelete( iFind( t ) );
}

template <class T> void CIVector<T>::vDeleteAll()
{
    ASSERT( this );

    while( m_iCount )
        bDelete( 0 );
}

template <class T> int CIVector<T>::iFind( T * t )
{
    ASSERT( this );

    for( int i = 0; i < m_iCount; i++ )
    {
        if( m_pData[ i ] == t )
            return i;
    }

    return kNotFound;
}

#endif   // __VECTTMPL_H__
