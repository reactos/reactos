//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       T2EmWrap.cxx
//
//  Contents:   Wrapper functions for the OpenType Embedding DLL (t2embed.dll).
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_T2EMBAPI_H_
#define X_T2EMBAPI_H_
#include "t2embapi.h"
#endif

#ifndef X_T2EMWRAP_HXX_
#define X_T2EMWRAP_HXX_
#include "t2emwrap.hxx"
#endif


DYNLIB g_dynlibT2Embed = { NULL, NULL, "T2EMBED.DLL" };
DYNPROC g_dynprocT2EmbedLoadFont =
    { NULL, &g_dynlibT2Embed, "TTLoadEmbeddedFont" };
DYNPROC g_dynprocT2EmbedDeleteFont =
    { NULL, &g_dynlibT2Embed, "TTDeleteEmbeddedFont" };

// refcount

ULONG g_cT2EmbedFonts = 0;

void LockT2Embed()
{
    LOCK_GLOBALS;
    g_cT2EmbedFonts++;
}

void FreeT2Embed()
{
    LOCK_GLOBALS;
    g_cT2EmbedFonts--;
    if (g_cT2EmbedFonts == 0)
    {
        FreeDynlib(&g_dynlibT2Embed);
    }
}

LONG WINAPI T2LoadEmbeddedFont( HANDLE *phFontReference, ULONG ulFlags, ULONG *pulPrivStatus,
        ULONG ulPrivs, ULONG *pulStatus, READEMBEDPROC lpfnReadFromStream, LPVOID lpvReadStream,
        LPWSTR szWinFamilyName, LPSTR szMacFamilyName, TTLOADINFO *pTTLoadInfo )
{
    LONG lRet = E_EXCEPTION;
    
    LockT2Embed();
    if ( THR( LoadProcedure( &g_dynprocT2EmbedLoadFont ) ) == S_OK )
    {
        LOADEMBFONTFN lpfnTTLoadEmbeddedFont = (LOADEMBFONTFN)g_dynprocT2EmbedLoadFont.pfn;

        lRet = (lpfnTTLoadEmbeddedFont)( phFontReference, ulFlags, pulPrivStatus, ulPrivs, pulStatus,
                                                    lpfnReadFromStream, lpvReadStream, szWinFamilyName,
                                                    szMacFamilyName, pTTLoadInfo );
    }
    FreeT2Embed();
    return lRet;
}

LONG WINAPI T2DeleteEmbeddedFont ( HANDLE hFontReference, ULONG ulFlags, ULONG *pulStatus )
{
    LONG lRet = E_EXCEPTION;
    
    LockT2Embed();
    if ( THR( LoadProcedure( &g_dynprocT2EmbedDeleteFont ) ) == S_OK )
    {
        DELEMBFONTFN lpfnTTDeleteEmbeddedFont = (DELEMBFONTFN)g_dynprocT2EmbedDeleteFont.pfn;
        lRet = (lpfnTTDeleteEmbeddedFont)( hFontReference, ulFlags, pulStatus );
    }
    FreeT2Embed();
    return lRet;
}

