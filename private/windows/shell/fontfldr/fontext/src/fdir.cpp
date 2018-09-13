///////////////////////////////////////////////////////////////////////////////
//
// fdir.cpp
//      Explorer Font Folder extension routines
//      Implementation for the class: CFontDir
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
// #include <windows.h>
// #include <windowsx.h>
#include "globals.h"

#include "fdir.h"
#include "fdirvect.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

CFontDir::CFontDir( )
   :  m_iLen( 0 ),
      m_bSysDir( FALSE )
{}


CFontDir::~CFontDir( )
{}


BOOL CFontDir::bInit( LPTSTR lpPath, int iLen )
{

    if( iLen >= 0 && iLen <= ARRAYSIZE( m_oPath ) )
    {
        m_iLen = iLen;

        _tcsncpy( m_oPath, lpPath, iLen );

        m_oPath[ iLen ] = 0;

        return TRUE;
    }

    //
    // Error. Clean up and return.
    //

    delete this;
    return FALSE;
}


BOOL CFontDir::bSameDir( LPTSTR lpStr, int iLen )
{
    return( ( iLen == m_iLen ) && ( _tcsnicmp( m_oPath, lpStr, iLen ) == 0 ) );
}


LPTSTR CFontDir::lpString( )
{
    return m_oPath;
}


CFontDir *CFDirVector::fdFindDir( LPTSTR lpPath, int iLen, BOOL bAddToList )
{
    //
    // Try to find the directory in the list.
    //

    int iCnt = iCount( );
    int i;

    CFontDir *poDir;

    for( i = 0; i < iCnt; i++, poDir = 0 )
    {
        poDir = poObjectAt( i );

        if( poDir->bSameDir( lpPath, iLen ) )
            break;
    }

    //
    // If we didn't find one, create one and add it.
    //

    if( !poDir && bAddToList )
    {
        poDir = new CFontDir;

        if( poDir && poDir->bInit( lpPath, iLen ) )
        {
            if( !bAdd( poDir ) )
            {
                delete poDir;
                poDir = 0;
            }
        }
    }

    return( poDir );
}

