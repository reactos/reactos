///////////////////////////////////////////////////////////////////////////////
//
// fontlist.cpp
//      Explorer Font Folder extension routines
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================
#include "priv.h"
#include "globals.h"

#include "fontvect.h"
#include "fontlist.h"

#include <memory.h>     // For memcpy
#include "fontcl.h"
#include "dbutl.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

////////////////////////////////////////////////////////////////////////////

CFontList::CFontList(
   int iSize,  // Initial number of fonts
   int iVectorSize )
   :  m_pData( 0 ),
      m_iCount( 0 ),
      m_iVectorCount( 0 ),
      m_iVectorSize( iVectorSize ),
      m_iVectorBounds( 0 )
{
    if( m_iVectorSize <= 0 )
        m_iVectorSize = kDefaultVectSize;

    m_iVectorBounds = iSize / m_iVectorSize + 1;
      
}


CFontList::~CFontList( )
{
    vDeleteAll( );

    if( m_pData )
        delete [] m_pData;
}


int CFontList::bInit( )
{
    int   iRet = 0;

    ASSERT( this );

    if( m_iVectorSize && !m_iCount && m_iVectorBounds )
    {
        m_pData = new CFontVector * [ m_iVectorBounds ];

        if( m_pData )
        {
            // 
            //  Allocate one CFontVector
            //

            m_pData[ 0 ] = new CFontVector( m_iVectorSize );

            if( m_pData[ 0 ] && ( m_pData[ 0 ]->bInit( ) ) )
            {
                m_iVectorCount++;
                return 1;
            }
        }
    }

    //
    //  Error. Clean up and return.
    //

    delete this;

    return 0;
}


int CFontList::iCount( void )
{
    ASSERT( this );

    return m_iCount;
}


int CFontList::bAdd( CFontClass * t )
{
    ASSERT( this );

    if( t == ( CFontClass *) 0 )
        return 0;

    //
    //  Determine which vector to put it in.
    //

    int iVector = m_iCount / m_iVectorSize;

    //
    //  If the vector isn't valid, then make one.
    //

    if( iVector >= m_iVectorCount )
    {
        //
        //  Allocate more vector pointers if we're out.
        //

        if( m_iVectorCount >= m_iVectorBounds )
        {
            int iNewBounds = m_iVectorBounds + 5;

            CFontVector ** pNew = new CFontVector * [ iNewBounds ];

            if( !pNew )
                return 0;

            memcpy( pNew, m_pData, sizeof( CFontVector * ) * m_iVectorBounds );

            delete [] m_pData;
            m_pData = pNew;

            m_iVectorBounds = iNewBounds;
        }

        m_pData[ iVector ] = new CFontVector( m_iVectorSize );

        if( !m_pData[ iVector ] || (! m_pData[ iVector ]->bInit( ) ) )
        {
            m_pData[ iVector ] = 0;
            return 0;
        }

        m_iVectorCount++;

    }

    if(  m_pData[ iVector ]->bAdd( t ) )
    {
        t->AddRef();
        m_iCount++;
        return 1;
    }

    return 0;
}


CFontClass * CFontList::poObjectAt( int idx )
{
    ASSERT( this );

    if( idx >=0 && idx < m_iCount )
    {
        int iVector = idx / m_iVectorSize;

        int subIdx = idx % m_iVectorSize;

        return m_pData[ iVector ]->poObjectAt( subIdx );
    }

    return 0;
}


CFontClass * CFontList::poDetach( int idx )
{
    ASSERT( this );

    if( idx >=0 && idx < m_iCount )
    {
        int iVector = idx / m_iVectorSize;

        int subIdx = idx % m_iVectorSize;

        CFontClass * pID = m_pData[ iVector ]->poDetach( subIdx );

        //
        //  If this isn't the last vector, then move one out of the last
        //  and into this one.
        //

        if( iVector != ( m_iVectorCount - 1 ) )
        {
            m_pData[ iVector ]->bAdd( m_pData[ m_iVectorCount - 1 ]->poDetach( 0 ) );
        }

        //
        //  If the last vector is now empty, remove it.
        //

        if( ! m_pData[ m_iVectorCount - 1 ]->iCount( ) )
        {
            m_iVectorCount--;

            delete m_pData[ m_iVectorCount ];

#ifdef _DEBUG
            //
            //  Fill it with zero.
            //

            m_pData[ m_iVectorCount ] = 0;
#endif         
        }

        ASSERT( pID );

        if( pID )
            m_iCount--;

        return pID;
    }

    return (CFontClass *) 0;
}


void CFontList::vDetachAll( )
{
    ASSERT( this );
    
    while( m_iCount )
    {
        CFontClass *poFont = poDetach( m_iCount - 1 );
        if (NULL != poFont)
            poFont->Release();
    }
}


CFontClass * CFontList::poDetach( CFontClass * t )
{
    ASSERT( this );

    return poDetach( iFind( t ) );
}


int CFontList::bDelete( int idx )
{
    ASSERT( this );

    CFontClass * pID = poDetach( idx );

    if( pID )
    {
        pID->Release();
        return 1;
    }

    return 0;
}


int CFontList::bDelete( CFontClass * t )
{
    ASSERT( this );

    return bDelete( iFind( t ) );
}


void CFontList::vDeleteAll( )
{
    ASSERT( this );

    while( m_iCount )
        bDelete( m_iCount - 1 );
}


int CFontList::iFind( CFontClass * t )
{
    int iRet;
    
    ASSERT( this );
    
    for( int i = 0; i < m_iVectorCount; i++ )
    {
       if( ( iRet = m_pData[ i ]->iFind( t ) ) != kNotFound )
          return( i * m_iVectorSize + iRet );
    }
    
    return kNotFound;
}


//
// Call CFontClass::Release for each font contained in the list.
//
void CFontList::ReleaseAll(void)
{
    CFontClass *poFont = NULL;
    for (INT i = 0; i < m_iCount; i++)
    {
        poFont = poObjectAt(i);
        if (NULL != poFont)
            poFont->Release();
    }
}

//
// Call CFontClass::AddRef for each font contained in the list.
//
void CFontList::AddRefAll(void)
{
    CFontClass *poFont = NULL;
    for (INT i = 0; i < m_iCount; i++)
    {
        poFont = poObjectAt(i);
        if (NULL != poFont)
            poFont->AddRef();
    }
}

//
// Create a clone of the list.
// Why Clone and not a copy ctor and assignment operator?
// The font folder code in general isn't very proper C++.
// Clone() is a better match to the existing code.
//
CFontList*
CFontList::Clone(
    void
    )
{
    CFontList *pNewList = new CFontList(m_iCount, m_iVectorSize);
    if (NULL != pNewList && pNewList->bInit())
    {
        for (int i = 0; i < m_iCount; i++)
        {
            if (!pNewList->bAdd(poObjectAt(i)))
            {
                delete pNewList;
                pNewList = NULL;
                break;
            }
        }
    }
    return pNewList;
}



/**********************************************************************
 * Some things you can do with a font list.
 */

HDROP hDropFromList( CFontList * poList )
{
    HANDLE           hMem = 0;
    LPDROPFILESTRUCT lpDrop;
    DWORD            dwSize;
    int              iCount,
                     i;
    CFontClass *     poFont;
    FullPathName_t   szPath;
    LPTSTR           lpPath;
    
    //
    //  Sanity.
    //
    if( !poList )
       goto backout0;
    
    //
    //  Walk the list and find out how much space we need.
    //

    iCount = poList->iCount( );

    if( !iCount )
        goto backout0;
    
    dwSize = sizeof( DROPFILESTRUCT ) + sizeof(TCHAR);  // size + terminating extra nul

    for( i = 0; i < iCount; i++ )
    {
        poFont = poList->poObjectAt( i );

        poFont->bGetFQName( szPath, ARRAYSIZE( szPath ) );
        dwSize += ( lstrlen( szPath ) + 1 ) * sizeof( TCHAR );

        //
        // Add length of PFB file path if this font has an associated PFB.
        // Note that bGetPFB() returns FALSE if the font isn't Type1.
        //
        if (poFont->bGetPFB(szPath, ARRAYSIZE(szPath)))
            dwSize += (lstrlen(szPath) + 1) * sizeof( TCHAR );
    }
    
    //
    //  If it's bigger than the struct can hold, then bail.
    //  TODO: Return an error?
    //

    if( dwSize > 0x0000ffff )
        goto backout0;
    
    //
    //  Allocate the buffer and fill it in.
    //

    hMem = GlobalAlloc( GMEM_SHARE | GHND, dwSize );

    if( !hMem )
        goto backout0;
    
    lpDrop = (LPDROPFILESTRUCT) GlobalLock( hMem );

    lpDrop->pFiles = (DWORD)(sizeof(DROPFILESTRUCT));
    lpDrop->pt.x   = 0;
    lpDrop->pt.y   = 0;
    lpDrop->fNC    = FALSE;
#ifdef UNICODE
    lpDrop->fWide  = TRUE;
#else
    lpDrop->fWide  = FALSE;
#endif

    //
    //  Fill in the path names.
    //

    lpPath = (LPTSTR) ( (LPBYTE) lpDrop + lpDrop->pFiles );

    for( i = 0; i < iCount; i++ )
    {
        poFont = poList->poObjectAt( i );
        

        poFont->bGetFQName( lpPath, ARRAYSIZE(szPath) );
        lpPath += lstrlen( lpPath ) + 1;

        //
        // Add PFB file path if font is a type 1.
        //
        if( poFont->bGetPFB(lpPath, ARRAYSIZE(szPath)))
            lpPath += ( lstrlen( lpPath ) + 1 );
    }

    *lpPath = TEXT('\0');       // Extra Nul terminate
    
    //
    //  Unlock the buffer and return it.
    //

    GlobalUnlock( hMem );
    
backout0:
    return (HDROP) hMem;
}
