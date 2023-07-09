/*
 *  @doc    INTERNAL
 *
 *  @module ALTFONT.H -- Alternate Font Name
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      06/29/98     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__ALTFONT_H_
#define I__ALTFONT_H_
#pragma INCMSG("--- Beg 'altfont.h'")

const WCHAR * AlternateFontName( const WCHAR * pszName );
const WCHAR * AlternateFontNameIfAvailable( const WCHAR * pszName );

#pragma INCMSG("--- End 'altfont.h'")
#else
#pragma INCMSG("*** Dup 'altfont.h'")
#endif
