///////////////////////////////////////////////////////////////////////////////
//
// pfiles.cpp
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

#include "resource.h"
#include "cpanel.h"
#include "fontcl.h"        // Just for PANOSEBytesClass
// #include "fontinfo.h"
#include "pnewexe.h"
#include "lstrfns.h"

#include "dbutl.h"
#include "t1.h"
#include "fontfile.h"


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef WINNT

#ifdef __cplusplus
extern "C" {
#endif

//
// [stevecat]   This used to reside in "wingdip.h" (included with <winp.h>)
//  6/29/95     but I have taken it out because of C++ name-mangling problems
//              with that header file that are not going to be fixed because
//              this file is going to change significantly (according to
//              EricK) when we switch over to Kernel mode GDI/User.
//
//
//#include <stddef.h>     //  Needed for winp.h
//#include <winp.h>       //  For private GDI entry point:  GetFontResourceInfo
//
//#undef SWAPL            //  The SWAPL macro in wingdip.h clashes with mine
//

// Private Control Panel entry point to enumerate fonts by file.

#define GFRI_NUMFONTS       0L
#define GFRI_DESCRIPTION    1L
#define GFRI_LOGFONTS       2L
#define GFRI_ISTRUETYPE     3L
#define GFRI_TTFILENAME     4L
#define GFRI_ISREMOVED      5L
#define GFRI_FONTMETRICS    6L

extern BOOL WINAPI GetFontResourceInfoW( LPWSTR  lpPathname,
                                         LPDWORD lpBytes,
                                         LPVOID  lpBuffer,
                                         DWORD   iType );

#ifdef __cplusplus
}
#endif

#endif


//-------------------------------------------------------------------
// this needs to go in a header. Probably pnewexe.h
//

#pragma pack(1)
typedef struct
{
    WORD    fontOrdinal;
    WORD    dfVersion;
    DWORD   dfSize;
    char    dfCopyright[ COPYRIGHT_LEN ];
    WORD    dfType;
    WORD    dfPoints;
    WORD    dfVertRes;
    WORD    dfHorzRes;
    WORD    dfAscent;
    WORD    dfIntLeading;
    WORD    dfExtLeading;
    BYTE    dfItalic;
    BYTE    dfUnderline;
    BYTE    dfStrikeOut;
    WORD    dfWeight;
    BYTE    dfCharSet;
    WORD    dfPixWidth;
    WORD    dfPixHeight;
    BYTE    dfPitchAndFamily;
    WORD    dfAvgWidth;
    WORD    dfMaxWidth;
    BYTE    dfFirstChar;
    BYTE    dfLastChar;
    BYTE    dfDefaultChar;
    BYTE    dfBreakChar;
    WORD    dfWidthBytes;
    DWORD   dfDevice;
    DWORD   dfFace;
    DWORD   dfReserved;
    char    szDeviceName[ 1 ];

} FONTENTRY, FAR *LPFONTENTRY;
#pragma pack()

//-------------------------------------------------------------------

TCHAR c_szTrueType[] = TEXT( "TrueType" );
TCHAR c_szOpenType[] = TEXT( "OpenType" );
TCHAR c_szDotOTF[]   = TEXT( ".OTF" );

#define M_INTEGERTYPE( wType )   (wType & 0x8000)
#define M_FONTDIRTYPE( wType )   ((wType & 0x7fff) == 7)

#define SWAP2B(p) (((unsigned short) ((p)[ 0 ]) << 8) | (p)[ 1 ])
#define SWAP4B(p) (SWAP2B((p)+2) | ((SWAP2B(p) + 0L) << 16))
#define SWAPW(x)  ((WORD)SWAP2B((unsigned char FAR *)(&x)))
#define SWAPL(x)  ((unsigned long)SWAP4B((unsigned char FAR *)(&x)))

#define Mac2Ansi(a)    a    // do this right later!

//
//  Platform and language we'll be looking for in the OS2 tables
//  The NAMEID_xxx are the ids of the name records we'll be hunting for.
//

#define DEFAULTPOINTS      14

#define LANG_US_ENG        0x0904        // US (1033) (in mac order)
// #define LANG_SHIFTJIS      0x1104        // SHIFTJIS

//
//  name ids.
//

#define COPYRIGHT_ID    0x0000
#define FAMILY_ID       0x0100
#define SUBFAMILY_ID    0x0200
#define PLATFORM_MS     0x0300        // in mac order
#define FACENAME_ID     0x0400        // in mac order
#define VERSION_ID      0x0500
#define POSTSCRIPT_ID   0x0600
#define TRADEMARK_ID    0x0700

#define ENCODEID_UNICODE   0x0100
#define ENCODEID_SHIFTJIS  0x0200
#define ENCODEID_PRC       0x0300
#define ENCODEID_BIG5      0x0400
#define ENCODEID_WANGSUNG  0x0500
#define ENCODEID_JOHAB     0x0600


#define NAMEID_COPYRIGHT    0
#define NAMEID_VERSION      5
#define NAMEID_TRADEMARK    7

#define TAG_CHARTOINDEXMAP   0x70616d63      //  'cmap'
#define TAG_FONTHEADER       0x64616568      //  'head'
#define TAG_NAMINGTABLE      0x656d616e      //  'name'
#define TAG_OS2TABLE         0x322f534f      //  'os_2'
#define TAG_DSIG             0x47495344      //  'DSIG'
#define TAG_CFF              0x20464643      //  'CFF'
#define SFNT_MAGIC           0xf53C0F5f


//
// Enumeration of bit mask values enables us to identify a set of TrueType
// tables in a single DWORD.  The total set of possible tables is open-ended
// as the TrueType specification is extensible.  This enumeration is merely
// a subset that is useful in the font folder.
//
enum TrueTypeTables {
    TT_TABLE_CMAP  = 0x00000001,
    TT_TABLE_HEAD  = 0x00000002,
    TT_TABLE_NAME  = 0x00000004,
    TT_TABLE_OS2   = 0x00000008,
    TT_TABLE_DSIG  = 0x00000010,
    TT_TABLE_CFF   = 0x00000020
                    };

//
//  The TTF structure as used here:
//  The TABLERECORD's the highest level.  It contains
//     sfnt_NameTable entries which in turn consist of
//     sfnt_NameRecord entries
//

typedef struct {
        WORD    id_Specific;
        WORD    id_Platform;
        WORD    id_Language;
} IDBlock_t;

//
//  A little macro for turning an array of 4 chars into dwords.
//

#define M_MAKETAG(a,b,c,d)   ((((((((DWORD) (a) ) << 8)  \
                             | (DWORD) (b) ) << 8)  \
                             | (DWORD) (c) ) << 8)  \
                             | (DWORD) (d) )

#define TAG_TTCF     M_MAKETAG('f', 'c', 't', 't' )

//
//  True type file structures
//

typedef struct ttc_hdr_tag {
   DWORD dwTag;
   DWORD dwVersion;
   DWORD dwDirCount;
} ttc_hdr;

typedef struct tt_hdr_tag {
  DWORD dwVersion;
  WORD  uNumTables;
  WORD  uSearchRange;
  WORD  uEntrySelector;
  WORD  uRangeShift;
} tt_hdr;

typedef struct tttag_tag {
  DWORD dwTag;
  DWORD dwCheckSum;
  DWORD dwOffset;
  DWORD dwLength;
} tttag;

typedef struct tt_head_tag {
  DWORD dwVersion;
  DWORD dwRevision;
  DWORD dwChecksum;
  DWORD dwMagic;
  WORD  wFlags;
  WORD  wUnitsPerEm;
  DWORD dwCreated1;
  DWORD dwCreated2;
  DWORD dwModified1;
  DWORD dwModified2;
  WORD  wXMin;
  WORD  wYMin;
  WORD  wXMax;
  WORD  wYMax;
  WORD  wStyle;
  WORD  wMinReadableSize;
  short iDirectionHint;
  short iIndexToLocFormat;
  short iGlyphDataFormat;
} tt_head;

typedef struct {
     WORD     wPlatformID;
     WORD     wSpecificID;
     DWORD    wOffset;
} sfnt_platformEntry;

typedef struct {
    WORD    wVersion;
    WORD    wNumTables;
    // sfnt_platformEntry platform[ 1 ];   // platform[ numTables ]
} sfnt_char2IndexDir;

typedef struct {
    WORD    wPlatformID;
    WORD    wSpecificID;
    WORD    wLanguageID;
    WORD    wNameID;
    WORD    wLength;
    WORD    wOffset;
} sfnt_NameRecord, *sfnt_pNameRecord, FAR* sfnt_lpNameRecord;

typedef struct {
    WORD    wFormat;
    WORD    wCntRecords;
    WORD    wOffsetString;
/*  sfnt_NameRecord[ count ]  */
} sfnt_NameTable, *sfnt_pNameTable, FAR* sfnt_lpNameTable;


extern "C" {
    void FAR PASCAL UnicodeToAnsi( LPWORD lpwName, LPSTR szName );
}

static void NEAR PASCAL FillName( LPTSTR            szName,
                                  sfnt_lpNameRecord pNameRecord,
                                  WORD              igi,
                                  LPBYTE            pStringByte );

static BOOL  NEAR PASCAL bGetName( CFontFile&    file,
                                   tttag         *pTTTag,
                                   IDBlock_t     &ID_Block,
                                   LPTSTR         szName,
                                   LPFONTDESCINFO lpFDI = NULL );

static BOOL  NEAR PASCAL bFindNameThing( sfnt_pNameTable pNames,
                                         IDBlock_t      &ID_Block,
                                         WORD            NameID,
                                         LPTSTR          szName );

static VOID  NEAR PASCAL vReadCountedString( CFontFile& file, LPSTR lpStr, int iLen );



/***************************************************************************
 * Start of Public functions
 ***************************************************************************/

#ifdef WINNT

/////////////////////////////////////////////////////////////////////////////
//
// ValidFontFile
//
// in:
//    lpszFile       file name to validate
// out:
//    lpszDesc       on succes name of TT file or description from exehdr
//    lpiFontType    set to a value based on Font type 1 == TT, 2 == Type1
//    lpdwStatus     Set to status of validation functions.
//                   Query to determine why font is invalid.
//                   The following list contains the possible status
//                   values.  See fvscodes.h for details.
//
//                   FVS_SUCCESS
//                   FVS_INVALID_FONTFILE
//                   FVS_INVALID_ARG
//                   FVS_INSUFFICIENT_BUF
//                   FVS_FILE_IO_ERR
//                   FVS_EXCEPTION
//
// NOTE: Assumes that lpszDesc is of size DESCMAX
//
// returns:
//    TRUE success, FALSE failure
//
/////////////////////////////////////////////////////////////////////////////

BOOL bCPValidFontFile( LPTSTR    lpszFile,
                       LPTSTR    lpszDesc,
                       WORD FAR *lpwFontType,
                       BOOL      bFOTOK,
                       LPDWORD   lpdwStatus )
{
    BOOL          result;
    DWORD         dwBufSize;
    FONTDESCINFO  File;
    BOOL          bTrueType = FALSE;
    TCHAR         szDesc[ DESCMAX ];
    WORD          wType = NOT_TT_OR_T1;
    LPTSTR        lpTemp;
    DWORD         dwStatus = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);
    DWORD         dwTrueTypeTables = 0;


    //
    // Initialize status return.
    //
    if (NULL != lpdwStatus)
       *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_STATUS, FVS_FILE_UNK);

    //
    //  Set up the FDI depending on what the caller wanted.
    //

    File.dwFlags = FDI_NONE;

    if( lpszDesc )
    {
        *lpszDesc = (TCHAR) 0;
        File.dwFlags = FDI_DESC;
    }

    if( lpwFontType )
        *lpwFontType = NOT_TT_OR_T1;

    GetFullPathName( lpszFile,
                     PATHMAX,
                     File.szFile,
                     &lpTemp );

    if( bIsTrueType( &File, &dwTrueTypeTables, &dwStatus ) )
    {
        LPCTSTR pszDecoration = c_szTrueType;
        WORD    wFontType     = TRUETYPE_FONT;
        //
        // If the font has a CFF table, we append (OpenType) name decoration.
        //
        if (TT_TABLE_CFF & dwTrueTypeTables)
        {
            pszDecoration = c_szOpenType;
            wFontType     = OPENTYPE_FONT;
        }
        if( lpwFontType )
            *lpwFontType = wFontType;

        if( lpszDesc )
            wsprintf( lpszDesc, c_szDescFormat, File.szDesc, pszDecoration );

        if (NULL != lpdwStatus)
            *lpdwStatus = dwStatus;

        return TRUE;
    }
    else
    {
        //
        // Return FALSE if bIsTrueType failed for any reason other than
        // FVS_INVALID_FONTFILE.
        //
        if (FVS_STATUS(dwStatus) != FVS_INVALID_FONTFILE)
        {
            if (NULL != lpdwStatus)
                *lpdwStatus = dwStatus;

            return FALSE;
        }
    }


    if( ::IsPSFont( File.szFile, szDesc, (LPTSTR) NULL, (LPTSTR) NULL,
                    (BOOL *) NULL, &dwStatus ))
    {
        if( lpwFontType )
            *lpwFontType = TYPE1_FONT;

        if( lpszDesc )
            lstrcpy( lpszDesc, szDesc );

        if (NULL != lpdwStatus)
            *lpdwStatus = dwStatus;

        return TRUE;
    }
    else
    {
        //
        // Return FALSE if IsPSFont failed for any reason other than
        // FVS_INVALID_FONTFILE.
        //
        if (FVS_STATUS(dwStatus) != FVS_INVALID_FONTFILE)
        {
            if (NULL != lpdwStatus)
                *lpdwStatus = dwStatus;

            return FALSE;
        }
    }

    result = FALSE;

    if( AddFontResource( File.szFile ) )
    {
        //
        //  At this point it is a valid font file of some sort
        //  (like a .FON file); however, we are still looking for
        //  more validation using GetFontResourceInfoW call.
        //
        //  See if this is a TrueType font file
        //

        dwBufSize = sizeof( BOOL );

        result = GetFontResourceInfoW( File.szFile,
                                       &dwBufSize,
                                       &bTrueType,
                                       GFRI_ISTRUETYPE );

        if( bTrueType && lpwFontType )
            *lpwFontType = TRUETYPE_FONT;

        if( result )
        {
            dwBufSize = DESCMAX;

            result = GetFontResourceInfoW( File.szFile,
                                           &dwBufSize,
                                           szDesc,
                                           GFRI_DESCRIPTION );

            if (NULL != lpszDesc)
            {
                vCPStripBlanks(szDesc);
                if (result && bTrueType)
                    wsprintf( lpszDesc, c_szDescFormat, (LPTSTR) szDesc,
                                                  (LPTSTR) c_szTrueType);
                else
                    lstrcpy( lpszDesc, szDesc );
            }
        }
        RemoveFontResource( File.szFile );
    }

    //
    // At this point, "result" indicates status of the FontResource tests.
    // If we've made it this far, this function just reports SUCCESS or INVALID_FONTFILE.
    //
    if (NULL != lpdwStatus)
        *lpdwStatus = (result ? FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK) :
                                FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK));

    return( result );


    bFOTOK;
}


#else  //  WINNT

//
// NOTE for NON-NT programmers.
// The argument lpdwStatus has been added to the function definition
// so that it will build with the changes made to the prototype.
// However, the argument is ignored in this NON-NT version of
// the function body.
//

BOOL FAR PASCAL bCPValidFontFile( LPCTSTR    lpszFile,
                                  LPTSTR     lpszDesc,
                                  BOOL FAR  *lpbTrueType,
                                  BOOL       bFOTOK,
                                  LPDWORD    lpdwStatus )
{
    FONTDESCINFO  File;
    LPTSTR        lpCh;
    BOOL          bTrueType = FALSE;
    BOOL          bValid = FALSE;
    DWORD         dwTrueTypeTables = 0;

    //
    //  Set up the FDI depending on what the caller wanted.
    //

    File.dwFlags = FDI_NONE;

    if( lpszDesc )
        File.dwFlags = FDI_DESC;

    //
    //  TODO: change this when windows gets fixed.
    //

#if 0
    lstrcpy( File.szFile, lpszFile );
#else
    {
        LPTSTR   lpTemp;

        GetFullPathName( lpszFile,
                         PATHMAX,
                         File.szFile,
                         &lpTemp );
    }
#endif

    //
    //  If we identify this as a new-type EXE file, we want to find
    //  the description portion within. For a valid file, the description
    //  should start with FONTRES, then some junk, then a colon, followed
    //  by the description we're going to use.
    //

    //
    // NOTE for NON-NT programmers.
    // The argument lpdwStatus has been added to the function call so that
    // it will compile with the modified bIsTrueType prototype.
    // See similar note at top of function.
    //
    if( bIsTrueType( &File, &dwTrueTypeTables, lpdwStatus ) )
    {
        bTrueType = TRUE;
        bValid    = TRUE;
    }
    else if( bIsNewExe( &File ) )
    {
        TCHAR cSave = File.szDesc[ 7 ];

        //
        //  This does not require DBCS
        //

        File.szDesc[ 7 ] = TEXT( '\0' );

        bValid = !lstrcmp( File.szDesc, TEXT( "FONTRES" ) );

        File.szDesc[ 7 ] = cSave;
    }

    //
    //  Prepare returns (if the caller is interested)
    //

    if( lpszDesc )
    {
        *lpszDesc = (TCHAR) 0;

        if( bTrueType )
            wsprintf( lpszDesc, c_szDescFormat, (LPTSTR) File.szDesc, (LPTSTR)c_szTrueType );
        else if( bValid )
        {
            if( bFOTOK )
            {
                lpCh = StrStr( File.szDesc + 7, TEXT( ":" ) );
            }
            else
            {
                lpCh = StrStr( File.szDesc + 8, TEXT( ":" ) );
            }

            //
            //  The lstrcpy call must be inside the if check because
            //  lpCh could be 0 from the StrStr call.
            //

            if( lpCh )
            {
                vCPStripBlanks( ++lpCh );
                lstrcpy( lpszDesc, lpCh );
            }
        }

        if( *lpszDesc == 0 )
            bValid = FALSE;
    }

    if( lpbTrueType )
        *lpbTrueType = bTrueType;

    return bValid;
}


#endif  //  WINNT




/***************************************************************************
 * End of public interfaces
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Determine if a True Type font file (.TTF) was converted from a Type1 font.
//
// The string "Converter: Windows Type 1 Installer" is stored in a TrueType file
// in the the version info section of the "name" block to indicate that
// the font was converted from a Type1 font.  This function reads this version
// info string from the caller-provided name block and determines if it matches
// the Type1 converter signature.
//
// Note that in the UNICODE section of the file, the converter signature
// string is stored in Big Endian byte order.  However, since bFindNameThing
// handles byte ordering and returns a TEXT string, we can just compare strings.
//
// WARNING:  Refererence \ntgdi\fondrv\tt\ttfd\fdfon.c for the actual
//           byte string that is written by GDI to the file upon conversion.
//
///////////////////////////////////////////////////////////////////////////////
BOOL NEAR PASCAL bIsConvertedTrueType(sfnt_pNameTable pNames, IDBlock_t& ID_Block)
{
    BOOL bStatus = FALSE;
    static TCHAR szTTFConverterSignature[] = TEXT("Converter: Windows Type 1 Installer");
    static UINT cchTTFConverterSignature   = ARRAYSIZE(szTTFConverterSignature);

    if (NULL != pNames)
    {
        FontDesc_t szVersionInfo;
        if( bFindNameThing( pNames, ID_Block, VERSION_ID, szVersionInfo ) )
        {
            //
            // Got version info string from "name" block.
            // Truncate to proper length for comparison with signature and compare.
            //
            szVersionInfo[cchTTFConverterSignature - 1] = TEXT('\0');
            bStatus = lstrcmp(szVersionInfo, szTTFConverterSignature) == 0;
        }
    }
    return bStatus;
}



BOOL NEAR PASCAL bGetName( CFontFile& file,
                           tttag *pTTTag,
                           IDBlock_t& ID_Block,
                           LPTSTR lpszName,
                           LPFONTDESCINFO lpFDI )
{
    sfnt_pNameTable pNames;
    WORD            size;
    TCHAR           szSubFamily[ 64 ];

    IDBlock_t       ID_DefBlock = ID_Block;


    ID_DefBlock.id_Language = (ID_DefBlock.id_Platform == PLATFORM_MS)
                                                         ? LANG_US_ENG : 0;

    size = (WORD) SWAPL( pTTTag->dwLength );

    *lpszName = 0;

    pNames = (sfnt_pNameTable) LocalAlloc( LPTR, size );

    if( pNames )
    {
        if (ERROR_SUCCESS == file.Read(pNames, size))
        {
            //
            //  The logic for what name to find:
            //  If font file was converted from a Type1 font
            //     1) POSTCRIPT_ID in current language
            //  else
            //     1) FACENAME_ID in current language.
            //  2) FAMILY and SUBFAMILY in current language.
            //  3) FACENAME_ID in default language.
            //
            // If the TrueType font was converted from a Type1 font, we want
            // to use the "postscript" form of the font description so that
            // it matches the description returned by IsPSFont() when invoked
            // on the "parent" Type1 file.  These descriptions are used as registry
            // keys in the "Fonts" and "Type1Fonts" sections and must match.
            //
            if (bIsConvertedTrueType(pNames, ID_Block) &&
                bFindNameThing(pNames, ID_Block, POSTSCRIPT_ID, lpszName))
            {
               //
               // Replace all dashes with spaces (same as .PFM/.INF file reader code)
               //
               for (LPTSTR pc = lpszName; *pc; pc++)
                 if (*pc == TEXT('-'))
                    *pc = TEXT(' ');
            }
            else if( bFindNameThing( pNames, ID_Block, FACENAME_ID, lpszName ) )
               ;
            else if( bFindNameThing( pNames, ID_Block, SUBFAMILY_ID, szSubFamily )
                 && (bFindNameThing( pNames, ID_Block,    FAMILY_ID, lpszName )
                 ||  bFindNameThing( pNames, ID_DefBlock, FAMILY_ID, lpszName ) ) )
            {
                lstrcat( lpszName, TEXT( " " ) );
                lstrcat( lpszName, szSubFamily );
            }
            else( bFindNameThing( pNames, ID_DefBlock, FACENAME_ID, lpszName ) )
                ;

            //
            //  Get the names for the font description if requested.
            //

            if( lpFDI )
            {
                if( lpFDI->dwFlags & FDI_FAMILY )
                {
                    lpFDI->szFamily[ 0 ] = 0;

                    if( !bFindNameThing( pNames, ID_Block, FAMILY_ID,
                                         lpFDI->szFamily ) )
                        bFindNameThing( pNames, ID_DefBlock, FAMILY_ID,
                                        lpFDI->szFamily );
                }

                if( lpFDI->dwFlags & FDI_VTC )
                {
                    TCHAR  szTemp[ 256 ];

                    lpFDI->lpszVersion   = 0;
                    lpFDI->lpszTrademark = 0;
                    lpFDI->lpszCopyright = 0;


                    if( bFindNameThing( pNames, ID_Block, VERSION_ID, szTemp ) ||
                       bFindNameThing( pNames, ID_DefBlock, VERSION_ID, szTemp ) )
                    {
                        lpFDI->lpszVersion = new TCHAR[ lstrlen( szTemp ) + 1 ];

                        if( lpFDI->lpszVersion )
                            lstrcpy( lpFDI->lpszVersion, szTemp );
                    }

                    if( bFindNameThing( pNames, ID_Block, COPYRIGHT_ID, szTemp ) ||
                       bFindNameThing( pNames, ID_DefBlock, COPYRIGHT_ID, szTemp ) )
                    {
                        lpFDI->lpszCopyright = new TCHAR[ lstrlen( szTemp ) + 1 ];

                        if( lpFDI->lpszCopyright )
                            lstrcpy( lpFDI->lpszCopyright, szTemp );
                    }

                    if( bFindNameThing( pNames, ID_Block, TRADEMARK_ID, szTemp ) ||
                       bFindNameThing( pNames, ID_DefBlock, TRADEMARK_ID, szTemp ) )
                    {
                        lpFDI->lpszTrademark = new TCHAR[ lstrlen( szTemp ) + 1 ];

                        if( lpFDI->lpszTrademark )
                            lstrcpy( lpFDI->lpszTrademark, szTemp );
                    }
                }

            }
        }
        LocalFree( (HANDLE)pNames );
   }

   return *lpszName != 0;
}


void NEAR PASCAL FillName( LPTSTR            szName,
                           sfnt_lpNameRecord pNameRecord, // unsigned PlatformID,
                           WORD              igi,
                           LPBYTE            pStringByte )
{
    WORD    i;
    WORD    wName[ 64 ];
    BOOL    bUsedDefault;
    LPSTR   lpSrc;
    LPTSTR  lpDest;
    LPSTR   lpByteStr;

    WORD UNALIGNED *pStringWord;


    if( pNameRecord->wPlatformID == PLATFORM_MS )
    {
        //
        //  wName now contains the flipped bytes.
        //  Decode depending on the way the string was encoded.
        //
        //  Rules:
        //     Encodind ID=1 (Unicode)
        //        Unicode
        //
        //     Encoding ID=2 (ShiftJIS)
        //        Unicode
        //
        //     Encoding ID=3 (PRC GB2312)
        //        Uses two bytes per character and GB2312 encoding. Single byte
        //        characters need null padding for leading byte.
        //
        //     Encoding ID=4 (Big 5)
        //        Uses two bytes per character and Big 5 encoding. Single byte
        //        characters need null padding for leading byte.
        //
        //     Encoding ID=5 (Wangsung)
        //        Uses two bytes per character and Wangsung encoding. Single byte
        //        characters need null padding for leading byte.
        //
        //     Encoding ID=6 (Johab)
        //        Uses two bytes per character and Johab encoding. Single byte
        //        characters need null padding for leading byte.
        //

        switch( pNameRecord->wSpecificID )
        {
        case ENCODEID_PRC:
        case ENCODEID_BIG5:
        case ENCODEID_WANGSUNG:
        case ENCODEID_JOHAB:
            if (g_bDBCS)
            {
                lpSrc = (LPSTR)pStringByte;

                lpByteStr = (LPSTR)wName;

                for( i = 0; i < igi; i++ )
                {
                    if( IsDBCSLeadByte( *lpSrc ) )
                    {
                        *lpByteStr++ = (CHAR) *lpSrc++;
                        i++;
                    }
                    else if( !*lpSrc )
                    {
                        lpSrc++;
                        i++;
                    }

                    *lpByteStr++ = *lpSrc++;
                }

                *lpByteStr = (BYTE) 0;

#ifdef UNICODE
                MultiByteToWideChar(CP_ACP,0,(LPSTR)wName,-1,szName,64);
#else
                lstrcpy(szName,(LPSTR)wName);
#endif // UNICODE

            }
            else // !g_bDBCS
            {
                lpSrc  = (LPSTR)pStringByte;

                lpDest = szName;

                for( i = 0; i < igi; i++ )
                {
                    if( IsDBCSLeadByte( *lpSrc ) )
                    {
                        *lpDest++ = (TCHAR) *lpSrc++;
                        i++;
                    }
                    else if( !*lpSrc )
                    {
                        lpSrc++;
                        i++;
                    }

                    *lpDest++ = (TCHAR) *lpSrc++;
                }

                *lpDest = (TCHAR) 0;
            }

            break;


         default:
            if( igi >= sizeof( wName ) )
                igi = sizeof( wName ) - sizeof( wName[ 0 ] );

            igi /= sizeof( wName[ 0 ] );

            pStringWord = (PWORD) pStringByte;

            WORD wLen = igi;

#ifdef UNICODE

            wName[ igi ] = 0;

            szName[ igi ] = TEXT( '\0' );

            while( igi--)
            {
                wName[ igi ] = SWAPW( pStringWord[ igi ] );

                szName[ igi ] = wName[ igi ];
            }

#else

            while( igi--)
                wName[ igi ] = SWAPW( pStringWord[ igi ] );

            int iRet = WideCharToMultiByte( CP_ACP,
                                            0,          // WC_SEPCHARS,
                                            wName,
                                            wLen,       // -1,
                                            szName,
                                            2 * (wLen + 1),
                                            NULL,
                                            &bUsedDefault );
            szName[ iRet ] = 0;

#endif  // UNICODE

        }  // End of switch( )

    }
    else
    {
        //
        //  Mac font
        //

        szName[ igi ] = (TCHAR) 0;

        while( igi--)
            szName[ igi ] = Mac2Ansi( pStringByte[ igi ] );
    }
}



DWORD GetFontDefaultLangID( )
{
    //
    //  set it initally to illegal value
    //

    static DWORD dwLangID = 0xffffffff;


    //
    //  Only do this once
    //

    if( dwLangID == 0xffffffff )
    {
        //
        //  Default to English
        //

        DWORD dwTemp = 0x00000409;

#ifdef WINNT

        TCHAR   szModName[ PATHMAX ];
        DWORD   dwSize, dwHandle;
        LPVOID  lpvBuf;


        if( GetModuleFileName( g_hInst, szModName, PATHMAX ) )
        {
            if( dwSize = GetFileVersionInfoSize( szModName, &dwHandle ) )
            {
                if( lpvBuf = (LPVOID) LocalAlloc( LPTR, dwSize ) )
                {
                    if( GetFileVersionInfo( szModName, dwHandle, dwSize, lpvBuf ) )
                    {
                        struct
                        {
                            WORD wLang;
                            WORD wCodePage;
                        } *lpTrans;

                        UINT uSize;

                        if( VerQueryValue( lpvBuf,
                                           TEXT( "\\VarFileInfo\\Translation" ),
                                           (LPVOID *) &lpTrans,
                                           &uSize )
                            && uSize >= sizeof( *lpTrans ) )
                        {
                            dwTemp = lpTrans->wLang;
                        }
                    }
                    LocalFree( (HLOCAL) lpvBuf );
                }
            }
        }

#else

        HRSRC hrsVer = FindResource( g_hInst, (LPTSTR) VS_VERSION_INFO,
                                     RT_VERSION );

        if( hrsVer )
        {
            HGLOBAL hVer = LoadResource( g_hInst, hrsVer );

            if( hVer )
            {
                LPVOID lpVer = LockResource( hVer );

                if( lpVer )
                {
                    struct
                    {
                        WORD wLang;
                        WORD wCodePage;
                    } *lpTrans;

                    UINT uSize;

                    if( VerQueryValue( lpVer,
                                       TEXT( "\\VarFileInfo\\Translation" ),
                                       (LPVOID *) &lpTrans,
                                       &uSize )
                        && uSize >= sizeof( *lpTrans ) )
                    {
                        dwTemp = lpTrans->wLang;
                    }

                    UnlockResource( hVer );
                }

                FreeResource( hVer );
            }
        }

#endif  //  WINNT

        //
        //  Use dwTemp so this is re-entrant (if not efficient)
        //

        dwLangID = dwTemp;
    }

    return( dwLangID );
}

///////////////////////////////////////////////////////////////////////////////
//
//  bValidateTrueType
//
//  The following list contains the possible status values
//  written to lpdwStatus. See fvscodes.h for details.
//
//  FVS_SUCCESS
//  FVS_INVALID_FONTFILE
//  FVS_MEM_ALLOC_ERR
//
///////////////////////////////////////////////////////////////////////////////
BOOL bValidateTrueType( CFontFile& file,
                        DWORD dwOffset,
                        LPFONTDESCINFO lpFile,
                        DWORD *pdwTableTags,
                        LPDWORD lpdwStatus )
{
    struct cmap_thing {
        sfnt_char2IndexDir    DirCmap;
        sfnt_platformEntry    Plat[ 2 ];
    } Cmap;

    tt_hdr     TTHeader;
    tt_head    TTFontHeader;

    IDBlock_t  ID_Block;
    tttag*     pTags;

    sfnt_platformEntry FAR* lpPlat;

    short      i, nTables;
    DWORD      dwSize;
    unsigned   cTables, ncTables;
    BOOL       result = FALSE;
    //
    // Most errors from this function are "header" errors.
    // Therefore, we default to this type of error code.
    //
    DWORD      dwStatus = FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);

    //
    // Initialize return status value.
    //
    if (NULL != lpdwStatus)
        *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_STATUS, FVS_FILE_UNK);

    //
    //  Init the ID block.
    //

    ID_Block.id_Platform = (WORD) -1;

    WORD wLangID = (WORD) GetFontDefaultLangID( );

    ID_Block.id_Language = SWAPW( wLangID );   // SWAPW( info.nLanguageID );

    //
    //  Load the TTF directory header.
    //
    file.Seek(dwOffset, FILE_BEGIN);

    if (ERROR_SUCCESS != file.Read(&TTHeader, sizeof(TTHeader)))
        goto IsTrueType_closefile;

    //
    //  If number of tables is so large that LocalAlloc fails, then the font
    //  will be blown off.
    //

    if( ( nTables = SWAPW( TTHeader.uNumTables ) ) > 0x7fff / sizeof( tttag ) )
    {
        DEBUGMSG( ( DM_ERROR, TEXT( "bIsTrueType: header too large." ) ) );
        goto IsTrueType_closefile;
    }

    i = nTables * sizeof( tttag );

    if( !(pTags = (tttag *) LocalAlloc( LPTR, i ) ) )
    {
        DEBUGMSG( ( DM_ERROR, TEXT( "bIsTrueType( ): LocalAlloc failed." ) ) );
        dwStatus = FVS_MAKE_CODE(FVS_MEM_ALLOC_ERR, FVS_FILE_UNK);
        goto IsTrueType_closefile;
    }

    if (ERROR_SUCCESS != file.Read(pTags, i))
    {
        DEBUGMSG( ( DM_ERROR, TEXT( "bIsTrueType(): File READ failure" ) ) );
        goto FailAndFree;
    }

    //
    //  the tables are in sorted order, so we should find 'cmap'
    //  before 'head', then 'name'
    //

    //  first we find the cmap table so we can find out what PlatformID
    //  this font uses
    //

    for( i = 0; i < nTables; i++ )
    {
        if( pTags[ i ].dwTag == TAG_CHARTOINDEXMAP )
        {
            //
            //  get platform stuff
            //
            file.Seek(SWAPL(pTags[ i ].dwOffset), FILE_BEGIN);

            if (ERROR_SUCCESS != file.Read(&Cmap, sizeof(Cmap), &dwSize))
                break;
            else if( ( ncTables = SWAPW( Cmap.DirCmap.wNumTables ) ) == 1 )
            {
                if( dwSize < sizeof( Cmap )-sizeof( Cmap.Plat[ 1 ] ) )
                    break;
            }

            for( cTables = 0; cTables < ncTables; cTables++ )
            {
                //
                //  we read 2 platform entries at a time
                //

                if( cTables >= 2 && !(cTables & 1 ) )
                {
                    dwSize = ncTables-cTables>1 ? sizeof( Cmap.Plat )
                                                : sizeof( Cmap.Plat[ 0 ]);

                    if (ERROR_SUCCESS != file.Read(Cmap.Plat, dwSize))
                        break;
                }

                lpPlat = &Cmap.Plat[ cTables & 01 ];

                //
                //  Unicode: get this and exit
                //

                if( lpPlat->wPlatformID == PLATFORM_MS )
                {
                    DEBUGMSG( (DM_TRACE1, TEXT( "--- PlatformID is PLATFORM_MS" ) ) );

                    ID_Block.id_Platform = lpPlat->wPlatformID;
                    ID_Block.id_Specific = lpPlat->wSpecificID;
                    break;
                }

                //
                //  Mac: get it, hope the Unicode platform will come
                //

                if( lpPlat->wPlatformID == 0x100 && lpPlat->wSpecificID == 0 )
                {
                    ID_Block.id_Platform = lpPlat->wPlatformID;
                    ID_Block.id_Specific = lpPlat->wSpecificID;
                }
            }
            break; // found continue below
        }
    }

    if( ID_Block.id_Platform == (WORD)-1 )
    {
        DEBUGMSG( ( DM_ERROR, TEXT( "bIsTrueType( ): No platform id" ) ) );
        goto FailAndFree;
    }

    //
    //  we found 'cmap' with the PlatformID now look for 'head'
    //  then 'name'

    while( ++i < nTables )
    {
        if( pTags[ i ].dwTag == TAG_FONTHEADER )
        {
            file.Seek(SWAPL( pTags[ i ].dwOffset ), FILE_BEGIN);

            if (ERROR_SUCCESS != file.Read(&TTFontHeader, sizeof(TTFontHeader))
                || TTFontHeader.dwMagic != SFNT_MAGIC )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "WRONG MAGIC! : %x" ), TTFontHeader.dwMagic ) );
                goto FailAndFree;
            }
            break;
        }
    }

    //
    //  At this point, the function is successful. If the caller wants a
    //  description and can't get it, return false (see next block).
    //

    result = TRUE;

    //
    //  Retrieve the font name (description) and family name if they were
    //  requested.
    //

    if( lpFile->dwFlags & (FDI_DESC | FDI_FAMILY ) )
    {
        while( ++i < nTables )
        {
            if( pTags[ i ].dwTag == TAG_NAMINGTABLE )
            {
                file.Seek(SWAPL( pTags[ i ].dwOffset ), FILE_BEGIN);
                result = bGetName( file, &pTags[ i ], ID_Block, lpFile->szDesc,
                                   lpFile );

                break;
            }
       }
    }

    //
    //  if requested, get the style and PANOSE information.
    //

    if( lpFile->dwFlags & (FDI_STYLE | FDI_PANOSE ) )
    {
        for( i = 0; i < nTables; i++ )
        {
            if( pTags[ i ].dwTag == TAG_OS2TABLE )
            {

#define WEIGHT_OFFSET   4
#define PAN_OFFSET      32
#define SEL_OFFSET      62

                DWORD dwStart = SWAPL( pTags[ i ].dwOffset );

                if( lpFile->dwFlags & FDI_PANOSE )
                {
                    file.Seek(dwStart + PAN_OFFSET, FILE_BEGIN);
                    file.Read(lpFile->jPanose, PANOSE_LEN);
                }

                if( lpFile->dwFlags & FDI_STYLE )
                {
                    WORD  wTemp;

                    file.Seek(dwStart + WEIGHT_OFFSET, FILE_BEGIN);
                    file.Read(&wTemp, sizeof(wTemp));

                    lpFile->wWeight = SWAPW( wTemp );

                    file.Seek(dwStart + SEL_OFFSET, FILE_BEGIN);
                    file.Read(&wTemp, sizeof(wTemp));

                    wTemp = SWAPW( wTemp );

                    lpFile->dwStyle  = (wTemp & 0x0001) ? FDI_S_ITALIC
                                                        : FDI_S_REGULAR;

                    lpFile->dwStyle |= (wTemp & 0x0020) ? FDI_S_BOLD : 0;
                }
                break;
            }
        }
    }

    if (NULL != pdwTableTags)
    {
        //
        // Caller want's to know exactly what tables the font contains.
        // Would prefer to do this in one of the earlier loops but they
        // all have early exits.  This is the only reliable way to get the
        // table info.  It's merely comparing DWORDs so it's very fast.
        //
        *pdwTableTags = 0;
        for (int i = 0; i < nTables; i++)
        {
            switch(pTags[i].dwTag)
            {
                case TAG_CHARTOINDEXMAP:  *pdwTableTags |= TT_TABLE_CMAP; break;
                case TAG_FONTHEADER:      *pdwTableTags |= TT_TABLE_HEAD; break;
                case TAG_NAMINGTABLE:     *pdwTableTags |= TT_TABLE_NAME; break;
                case TAG_OS2TABLE:        *pdwTableTags |= TT_TABLE_OS2;  break;
                case TAG_DSIG:            *pdwTableTags |= TT_TABLE_DSIG; break;
                case TAG_CFF:             *pdwTableTags |= TT_TABLE_CFF;  break;
                default:
                    break;
            }
        }
    }

FailAndFree:

    LocalFree( (HANDLE) pTags );

IsTrueType_closefile:

    //
    // If successful, update verification status for success.
    // Otherwise, leave at assigned error code.
    //
    if (NULL != lpdwStatus)
        *lpdwStatus = (result ? FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK) : dwStatus);

    return result;
}


///////////////////////////////////////////////////////////////////////////////
//
//  bIsTrueType
//
//  The following list contains the possible status values
//  written to lpdwStatus. See fvscodes.h for details.
//
//  FVS_SUCCESS
//  FVS_INVALID_FONTFILE
//  FVS_MEM_ALLOC_ERR
//  FVS_FILE_OPEN_ERR
//  FVS_INSUFFICIENT_BUF
//
///////////////////////////////////////////////////////////////////////////////
BOOL NEAR PASCAL bIsTrueType( LPFONTDESCINFO lpFile, DWORD *pdwTableTags, LPDWORD lpdwStatus )
{
    ttc_hdr     TTCHeader;
    CFontFile   file;
    DWORD       i;
    BOOL        result = FALSE;
    DWORD       *pdwDirectory = 0;
    FontDesc_t  szFontDesc;
    TCHAR       szConcat[ 32 ];


    DEBUGMSG( (DM_TRACE1, TEXT( "bIsTrueType() checking file %s" ), lpFile->szFile ) );

    if (ERROR_SUCCESS != file.Open(lpFile->szFile, GENERIC_READ, FILE_SHARE_READ))
    {
        if (NULL != lpdwStatus)
            *lpdwStatus = FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_UNK);

        return( FALSE );
    }

    //
    // If any of this code causes return of FALSE,
    // we return INVALID_FONTFILE unless indicated otherwise by
    // explicitely setting the return code.
    //
    if (NULL != lpdwStatus)
        *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);

    if(ERROR_SUCCESS != file.Read(&TTCHeader, sizeof(TTCHeader)))
        goto IsTrueType_closefile;

    //
    //  Check for a TTC file.
    //

    if( TTCHeader.dwTag == TAG_TTCF )
    {
       if( !LoadString( g_hInst, IDS_TTC_CONCAT, szConcat, ARRAYSIZE( szConcat ) ) )
            lstrcpy( szConcat, TEXT( " & " ) );

       TTCHeader.dwDirCount  = SWAPL( TTCHeader.dwDirCount );

       //
       //  Load in the first directory, for now.
       //

       if( !TTCHeader.dwDirCount )
            goto IsTrueType_closefile;

       pdwDirectory = new DWORD [ TTCHeader.dwDirCount ];

       if( !pdwDirectory )
            goto IsTrueType_closefile;

       file.Seek(sizeof(TTCHeader), FILE_BEGIN);

       DWORD dwBytesToRead = sizeof( DWORD ) * TTCHeader.dwDirCount;

       if(ERROR_SUCCESS != file.Read(pdwDirectory, dwBytesToRead))
            goto IsTrueType_closefile;
    }
    else
    {
        TTCHeader.dwDirCount = 1;

        pdwDirectory = new DWORD [ 1 ];

        if( !pdwDirectory )
            goto IsTrueType_closefile;

        *pdwDirectory = 0;
    }

    //
    //  For each TrueType directory, process it.
    //

    szFontDesc[ 0 ] = 0;

    for( i = 0; i < TTCHeader.dwDirCount; i++ )
    {
        //
        //  Save of the description of the previous font.
        //

        if( i && ( lpFile->dwFlags & FDI_DESC ) )
        {
           vCPStripBlanks(lpFile->szDesc);
           if( ( lstrlen( szFontDesc ) + lstrlen( lpFile->szDesc )
                 + lstrlen( szConcat ) + 1 ) > ARRAYSIZE( szFontDesc ) )
           {
                if (NULL != lpdwStatus)
                    *lpdwStatus = FVS_MAKE_CODE(FVS_INSUFFICIENT_BUF, FVS_FILE_UNK);

                //
                // This is a coding error.  Complain about it.
                //
                ASSERT(FALSE);
                goto IsTrueType_closefile;
           }

           lstrcat( szFontDesc, lpFile->szDesc );

           lstrcat( szFontDesc, szConcat );
        }

        if( !bValidateTrueType( file, SWAPL( *(pdwDirectory + i ) ), lpFile, pdwTableTags, lpdwStatus ) )
            goto IsTrueType_closefile;
    }

    //
    //  If we did more than one font, then we have to add the name of the
    //  last one to the list.
    //

    if( TTCHeader.dwDirCount > 1 )
    {
        if( lpFile->dwFlags & FDI_DESC )
        {
            vCPStripBlanks(lpFile->szDesc);
            if( ( lstrlen( szFontDesc ) + lstrlen( lpFile->szDesc )
                  + lstrlen( szConcat ) + 1 ) > ARRAYSIZE( szFontDesc ) )
                goto IsTrueType_closefile;

          lstrcat( szFontDesc, lpFile->szDesc );

          lstrcpy( lpFile->szDesc, szFontDesc );
        }
    }

    result = TRUE;

IsTrueType_closefile:

    if( pdwDirectory )
        delete [] pdwDirectory;

    DEBUGMSG( (DM_TRACE1, TEXT( "bIsTrueType() returning %d" ), result ) );

    //
    // If successful, update verification status.
    // Otherwise, leave at assigned error code.
    //
    if ((NULL != lpdwStatus) && result)
        *lpdwStatus = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);

    return result;
}



void NEAR PASCAL vReadCountedString( CFontFile& file, LPSTR lpString, int iLen )
{
    char cBytes;

    file.Read(&cBytes, 1);

    //
    //  Limit check 6 August 1990    clarkc
    //

    cBytes = __min( cBytes, iLen-1 );

    file.Read(lpString, cBytes);

    *(lpString + cBytes) = 0;
}


BOOL bReadNewExeInfo( CFontFile& file,
                      struct new_exe * pne,
                      long             lHeaderOffset,
                      LPFONTDESCINFO   lpFile )
{
    LONG  lResTable = pne->ne_rsrctab + lHeaderOffset;
    BOOL  bRet = FALSE;
    WORD  wShiftCount;

    struct rsrc_typeinfo rt;
    struct rsrc_nameinfo ri;

    LPFONTENTRY pfe;


    //
    //  Fix up the lpFile in case we bail early.
    //

    lpFile->lpszVersion = lpFile->lpszCopyright = lpFile->lpszTrademark = 0;

    //
    //  Move to the beginning of the resource table.
    //

    file.Seek(lResTable, FILE_BEGIN);

    //
    //  Read the shift count.
    //
    if(ERROR_SUCCESS != file.Read(&wShiftCount, 2))
        goto backout;

    //
    //  Quick validity check.
    //

    if( wShiftCount > 12 )
        goto backout;

    //
    //  Read the resources until we hit the FONTDIR
    //

    while( TRUE )
    {
        memset( &rt, 0, sizeof( rt ) );

        if(ERROR_SUCCESS != file.Read(&rt, sizeof(rt)))
            goto backout;

        if( rt.rt_id == 0 )
            break;

        if( M_INTEGERTYPE( rt.rt_id ) && M_FONTDIRTYPE( rt.rt_id ) )
        {
            //
            //  Read one resinfo record. We don't need all of them. The
            //  style and name of all should be the same.
            //

            if (ERROR_SUCCESS != file.Read(&ri, sizeof(ri)))
                goto backout;


            LONG lOffset = ( (LONG) ri.rn_offset ) << wShiftCount;
            LONG lSize   = ( (LONG) ri.rn_length ) << wShiftCount;

            //
            //  Allocate memory for the resource.
            //

            LPSTR lpMem = new char [ lSize ];


            if( !lpMem )
                goto backout;

            file.Seek(lOffset, FILE_BEGIN);

            LPSTR lpTMem = lpMem;

            while( lSize )
            {
                WORD wSize;

                if( lSize >= 32767 )
                   wSize = 32767;
                else
                   wSize = (WORD) lSize;

                if (ERROR_SUCCESS != file.Read(lpTMem, wSize))
                {
                   delete lpMem;
                   goto backout;
                }

                lSize -= wSize;
                lpTMem += wSize;
            }

            //
            //  The first word is the font count and the rest is a chunk of
            //  font entries.
            //

            int nFonts = (int)*( (unsigned short *) lpMem );

            pfe = (LPFONTENTRY) (lpMem + sizeof( WORD ) );

            if( lpFile->dwFlags & FDI_STYLE )
            {
               lpFile->dwStyle = (pfe->dfItalic ) ? FDI_S_ITALIC : FDI_S_REGULAR;

               lpFile->wWeight = pfe->dfWeight;
            }

            if( lpFile->dwFlags & FDI_FAMILY )
            {
                LPSTR lpFace;
                LPSTR lpDev = pfe->szDeviceName;

                lpFace = lpDev + lstrlenA( lpDev ) + 1;

#ifdef UNICODE
                MultiByteToWideChar( CP_ACP, 0, lpFace, -1,
                                     lpFile->szFamily, PATHMAX );

#else

                strcpy( lpFile->szFamily, lpFace );

#endif  //  UNICODE
            }

            if( lpFile->dwFlags & FDI_VTC )
            {
                //
                //  No version or trademark. Get the copyright.
                //

                lpFile->lpszCopyright = new TCHAR[ COPYRIGHT_LEN ];

#ifdef UNICODE
                if( lpFile->lpszCopyright )
                    MultiByteToWideChar( CP_ACP, 0, pfe->dfCopyright, -1,
                                         lpFile->lpszCopyright, COPYRIGHT_LEN );

#else
                if( lpFile->lpszCopyright )
                    strcpy( lpFile->lpszCopyright, pfe->dfCopyright );

#endif  //  UNICODE
            }

            bRet = TRUE;
            delete lpMem;

            //
            //  We got one, get out of here.
            //

            break;
        }
    }

backout:
    return bRet;
}

BOOL NEAR PASCAL bIsNewExe( LPFONTDESCINFO lpFile )
{
    BOOL     bValid = FALSE;
    long     lNewHeader;
    CFontFile file;

    struct exe_hdr oeHeader;
    struct new_exe neHeader;


    if (ERROR_SUCCESS == file.Open(lpFile->szFile, GENERIC_READ, FILE_SHARE_READ))
    {
        file.Read(&oeHeader, sizeof(oeHeader));

        if( oeHeader.e_magic == EMAGIC && oeHeader.e_lfanew )
            lNewHeader = oeHeader.e_lfanew;
        else
            lNewHeader = 0L;

        file.Seek(lNewHeader, FILE_BEGIN);

        file.Read(&neHeader, sizeof(neHeader));

        if( neHeader.ne_magic == NEMAGIC )
        {
            //
            // seek to the description, and read it
            //
            file.Seek(neHeader.ne_nrestab, FILE_BEGIN);

#ifdef UNICODE
            char szTemp[ DESCMAX ];

            vReadCountedString( file, szTemp, DESCMAX );

            MultiByteToWideChar( CP_ACP, 0, szTemp, -1,
                                     lpFile->szDesc, DESCMAX );

#else

            vReadCountedString( fh, lpFile->szDesc, sizeof( lpFile->szDesc ) );

#endif  //  UNICODE

            bValid = TRUE;

            //
            //  If requested, get family and style information.
            //

            if( lpFile->dwFlags & (FDI_FAMILY | FDI_STYLE | FDI_VTC ) )
            {
                bValid = bReadNewExeInfo( file, &neHeader, lNewHeader, lpFile );
            }
        }
    }

    return bValid;
}


//
//  find a TT name matching the platform specific and language from
//  the name table
//
//  in:
//     pNames        name table to search
//     PlatformID    search for this
//     SpecificID    and this
//     uLanguageID    and this
//     NameID        this is the name type
//
//  out:
//     szName        name if found
//
//  returns:
//     TRUE    name found, szName contains the name
//     FALSE    name not found, szName is NULL
//

BOOL NEAR PASCAL bFindNameThing( sfnt_pNameTable pNames, IDBlock_t &ID_Block,
                                 WORD NameID, LPTSTR szName )
{
    sfnt_lpNameRecord pNameRecord;

    sfnt_lpNameRecord pFoundRecord = NULL;

    int     cNames;
    LPBYTE  pStringArea;
    WORD    wWantLang = SWAPW( ID_Block.id_Language );


    szName[ 0 ] = 0;

    //
    //  Verify that this, indeed, is a name thing. The format should be zero.
    //

    if( pNames->wFormat )
        return FALSE;

    cNames = SWAPW( pNames->wCntRecords );

    pNameRecord = (sfnt_pNameRecord)( (LPBYTE) pNames + sizeof( sfnt_NameTable ) );

    for( ; cNames--; pNameRecord++ )
    {
        if( pNameRecord->wPlatformID == ID_Block.id_Platform &&
            pNameRecord->wSpecificID == ID_Block.id_Specific &&
            pNameRecord->wNameID     == NameID )
        {
            //
            //  Check the language matches
            //

            WORD wFoundLang = SWAPW( pNameRecord->wLanguageID );

            if( PRIMARYLANGID( wFoundLang ) != PRIMARYLANGID( wWantLang ) )
            {
                continue;
            }

            pFoundRecord = pNameRecord;

            //
            //  Check the locale matches too
            //

            if( pNameRecord->wLanguageID == ID_Block.id_Language )
            {
                break;
            }
        }
    }

    if( pFoundRecord )
    {
        pNameRecord = pFoundRecord;

        pStringArea  = (LPBYTE) pNames;
        pStringArea += SWAPW( pNames->wOffsetString );
        pStringArea += SWAPW( pNameRecord->wOffset );

        FillName( szName, pNameRecord, //->wPlatformID,
                  SWAPW( pNameRecord->wLength ), pStringArea );

        return TRUE;
    }

    DEBUGMSG( (DM_ERROR, TEXT( "bFindNameThing(): ERROR!" ) ) );
    DEBUGMSG( (DM_ERROR, TEXT( "--- Platform: %x  Specific: %x   Language: %x" ),
            ID_Block.id_Platform, ID_Block.id_Specific, ID_Block.id_Language) );

    return FALSE;
}

