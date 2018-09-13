/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mapembed.c

Abstract:

    This module contains the the prototypes of the functions that
    perform the mapping between the "embedding" section of win.ini,
    and the subkeys of HKEY_CLASSES_ROOT.

    This mapping is a hack implemented on Win3.1, that must also
    exist on NT.
    It is implemnted in the WOW layer, since only some win16 apps
    that read or write to the "embedding" section ( Excel and
    MsMail) depend on it.



Author:


    Jaime F. Sasson (jaimes) 25-Nov-1992



--*/

#if !defined( _MAP_EMBEDDING_SECTION_ )

#define _MAP_EMBEDDING_SECTION_

#define IS_EMBEDDING_SECTION(pszSection)                                     \
    ( ! (pszSection == NULL || WOW32_stricmp( pszSection, szEmbedding )) )

BOOL
IsWinIniHelper(
    IN  LPSTR   Filename
    );

//
// WARNING Filename argument to IS_WIN_INI must already be lowercase.
//

#define IS_WIN_INI(Filename) (                                               \
    (Filename)                                                               \
    ? (WOW32_strstr((Filename), szWinDotIni)                                 \
          ? IsWinIniHelper((Filename))                                       \
          : FALSE)                                                           \
     : FALSE)

VOID
UpdateEmbeddingAllKeys( VOID );

VOID
SetLastTimeUpdated( VOID );

VOID
UpdateEmbeddingKey(
    IN  LPSTR   KeyName
    );

VOID
UpdateClassesRootSubKey(
    IN  LPSTR   KeyName,
    IN  LPSTR   Value
    );

BOOL
WasSectionRecentlyUpdated( VOID );

#endif
