/******************************Module*Header*******************************\
* Module Name: font.c
*
* Font and text handling for the OpenGL-based 3D Text screen saver.
*
* Created: 12-24-94 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commdlg.h>
#include <ptypes32.h>
#include <pwin32.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <float.h> // for FLT_MAX
#include <GL/gl.h>
#include "sscommon.h"
#include "sstext3d.h"

#ifndef UNICODE
// Support dbcs characters (this is a patch)
#define SS_DBCS_SUPPORT 1
#endif

static LISTENTRY* FindExtendedLUTEntry( TCHAR c, WglFontContext *pwfc );
static BOOL UpdateReducedFontList( TCHAR c, LISTENTRY *ple, 
                                   WglFontContext *pwfc );
#ifdef SS_DBCS_SUPPORT
static BOOL UpdateReducedFontListDBCS( DWORD c, LISTENTRY *ple, 
                                       WglFontContext *pwfc );
#endif
static void DeleteExtendedLUTTableEntries( WglFontContext *pwfc );
static void DeleteDirectLUTTableEntries( WglFontContext *pwfc );
static void DeleteLUTEntry( LISTENTRY *ple );

/******************************Public*Routine******************************\
* CreateWglFontContext
*
* Create a WglFontContext.
*
* Initializes the look-up-table memory for characters.
* The first half of this table is a direct look-up, using the char value.
* The second half is used for unicode chars that are >256, and must be
* searched sequentially.
*
\**************************************************************************/

WglFontContext *
CreateWglFontContext( HDC hdc, int type, float fExtrusion, 
                      float fChordalDeviation )
{
    WglFontContext *pwfc;

    pwfc = (WglFontContext *) LocalAlloc( LMEM_FIXED, sizeof(WglFontContext) );
    if( !pwfc ) {
        SS_ALLOC_FAILURE( "CreateWglFontContext" );
        return NULL;
    }

    pwfc->hdc = hdc;
    pwfc->type = type;
    pwfc->extrusion = fExtrusion;
    pwfc->chordalDeviation = fChordalDeviation;

    pwfc->listLUT = (LISTENTRY*) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, 
                                      SIZE_LIST_LUT * sizeof(LISTENTRY) );

    if( !pwfc->listLUT ) {
        LocalFree( pwfc );
        SS_ALLOC_FAILURE( "CreateWglFontContext" );
        return NULL;
    }

    pwfc->LUTIndex = 0;

    return pwfc;
}

/******************************Public*Routine******************************\
* DeleteWglFontContext
*
* Delete a WglFontContext.
*
\**************************************************************************/

void
DeleteWglFontContext( WglFontContext *pwfc )
{
    if( !pwfc )
        return;

    // Delete the extended lut table entries
    DeleteExtendedLUTTableEntries( pwfc );

    // Delete the direct lut table entries
    DeleteDirectLUTTableEntries( pwfc );
    
    if( pwfc->listLUT )
        LocalFree( pwfc->listLUT );

    LocalFree( pwfc );
}


/******************************Public*Routine******************************\
* DeleteExtendedLUTTablEntries
*
\**************************************************************************/

static void
DeleteExtendedLUTTableEntries( WglFontContext *pwfc )
{
    int i;
    LISTENTRY *ple = pwfc->listLUT + MAX_DIRECT_LUT;

    for( i = 0; i < pwfc->LUTIndex; i ++, ple++ )
        DeleteLUTEntry( ple );

    pwfc->LUTIndex = 0;
}

/******************************Public*Routine******************************\
* DeleteDirectLUTTablEntries
*
\**************************************************************************/

static void
DeleteDirectLUTTableEntries( WglFontContext *pwfc )
{
    int i;
    LISTENTRY *ple = pwfc->listLUT;

    for( i = 0; i < MAX_DIRECT_LUT; i ++, ple++ )
        DeleteLUTEntry( ple );
}

/******************************Public*Routine******************************\
* DeleteLUTEntry
*
\**************************************************************************/

static void
DeleteLUTEntry( LISTENTRY *ple )
{
    if( ple->listNum ) {
        glDeleteLists( ple->listNum, 1 );
        ple->listNum = 0;
    }
    if( ple->lpgmf ) {
        LocalFree( ple->lpgmf );
        ple->lpgmf = NULL;
    }
    ple->glyph = 0;
}

/**************************************************************************\
* ConvertStringToList
*
* - Translate the string into display list numbers
* - If no entry in the list table, update the table
*
\**************************************************************************/
void
ConvertStringToList( LPTSTR pszSrc, USHORT *usDst, WglFontContext *pwfc )
{
    int strLen;
    LISTENTRY *ple;
    TCHAR c;
    TUCHAR cindex; // use unsigned TCHAR for table indexing
#ifdef SS_DBCS_SUPPORT
    BOOL bIsLeadByte;
#endif

    if( !pwfc )
        return;

    /* Go thru the string, making sure every char has a display list
     * Copy the display list # into usDst
     */

    strLen = lstrlen( pszSrc );
    if( strLen > TEXT_BUF_SIZE )
        strLen = TEXT_BUF_SIZE;

    /* Check for possible overflow with the extended LUT section, and
     * invalidate the extended LUT if so
     */
    if( pwfc->LUTIndex ) {
        int roomLeft; // spaces left in extended lut table

        roomLeft = SIZE_LIST_LUT - (pwfc->LUTIndex + MAX_DIRECT_LUT +1);
        if( roomLeft < strLen ) {
            /* If none of the characters in this string have entries in the
             * table, we'll run out of room.  Assume the worst, invalidate
             * the whole table, and start over again.
             */
            DeleteExtendedLUTTableEntries( pwfc );
        }
    }

    for( ; strLen; strLen--, pszSrc++ ) {
        c = *pszSrc;
        cindex = (TUCHAR) c;

        // see if char already has cmd list entry - if not, create it
#ifdef SS_DBCS_SUPPORT
        bIsLeadByte = IsDBCSLeadByte(cindex);
        if( !bIsLeadByte && (cindex < MAX_DIRECT_LUT) ) {  // direct LUT
#else
        if( cindex < MAX_DIRECT_LUT ) {  // direct LUT entry
#endif
            ple = &pwfc->listLUT[cindex];
            if( !ple->listNum ) {
                // have to create new cmd list for this glyph
                if( ! UpdateReducedFontList( c, ple, pwfc ) ) {
                    DeleteLUTEntry( ple );
                    continue;
                }
            }
        } else {  // extended LUT entry
#ifdef SS_DBCS_SUPPORT
            DWORD dwChar;
            if( bIsLeadByte ) {
                // Iterate over the lead byte
                pszSrc++;
                strLen--;
                c = *pszSrc;
                // Setup the double byte word so that :
                //   High byte is the lead byte
                //   Low byte is the next byte in the byte stream (this byte
                //   will be used to access the extended lut table)
                dwChar = (DWORD) ( (cindex << 8) | ((TUCHAR) c)  );
            } else
                dwChar = (DWORD) (TUCHAR) c;

            if( !(ple = FindExtendedLUTEntry( c, pwfc )) ) {
                ple = &pwfc->listLUT[pwfc->LUTIndex+MAX_DIRECT_LUT];
                if( ! UpdateReducedFontListDBCS( dwChar, ple, pwfc ) ) {
                    DeleteLUTEntry( ple );
                    continue;
                }
                ple->glyph = c;
                pwfc->LUTIndex++;
            }
#else
            if( !(ple = FindExtendedLUTEntry( c, pwfc )) ) {
                ple = &pwfc->listLUT[pwfc->LUTIndex+MAX_DIRECT_LUT];
                if( ! UpdateReducedFontList( c, ple, pwfc ) ) {
                    DeleteLUTEntry( ple );
                    continue;
                }
                ple->glyph = c;
                pwfc->LUTIndex++;
            }
#endif
        }
        *usDst++ = ple->listNum;
    }
    // zero terminate
    *usDst = 0;
}

/******************************Public*Routine******************************\
* UpdateReducedFontList
*
* Calls wglUseFontOutlines() to create a command list for the supplied
* character.
*
* Allocates memory for the characters glyphmetrics as well
*
\**************************************************************************/
static BOOL 
UpdateReducedFontList( TCHAR c, LISTENTRY *ple, WglFontContext *pwfc )
{
    ple->lpgmf = (LPGLYPHMETRICSFLOAT) LocalAlloc( LMEM_FIXED,
                                                sizeof( GLYPHMETRICSFLOAT ) );

    if( !ple->lpgmf ) {
        SS_ALLOC_FAILURE( "UpdateReducedFontList" );
        return FAILURE;
    }

    // get next list # for this glyph.
    ple->listNum = (USHORT) glGenLists( 1 );
    if( !ple->listNum ) {
        SS_WARNING( "glGenLists failed\n" );
        return FAILURE;
    }

    if( !wglUseFontOutlines(pwfc->hdc, (TUCHAR) c, 1, ple->listNum, 
               pwfc->chordalDeviation, 
               pwfc->extrusion, 
               pwfc->type, ple->lpgmf) ) {
        SS_WARNING( "wglUseFontOutlines failed\n" );
        return FAILURE;
    }
    return SUCCESS;
}

#ifdef SS_DBCS_SUPPORT

// Modified version of UpdateReducedFontList that handles dbcs characters.
// This is a *patch* (as is all the SS_DBCS_SUPPORT stuff) to handle dbcs
// characters without jeopardizing SUR release stability.  Later, the functions
// can be consolidated.

static BOOL 
UpdateReducedFontListDBCS( DWORD c, LISTENTRY *ple, WglFontContext *pwfc )
{
    ple->lpgmf = (LPGLYPHMETRICSFLOAT) LocalAlloc( LMEM_FIXED,
                                                sizeof( GLYPHMETRICSFLOAT ) );

    if( !ple->lpgmf ) {
        SS_ALLOC_FAILURE( "UpdateReducedFontList" );
        return FAILURE;
    }

    // get next list # for this glyph.
    ple->listNum = (USHORT) glGenLists( 1 );
    if( !ple->listNum ) {
        SS_WARNING( "glGenLists failed\n" );
        return FAILURE;
    }

    if( !wglUseFontOutlines(pwfc->hdc, c, 1, ple->listNum, 
               pwfc->chordalDeviation, 
               pwfc->extrusion, 
               pwfc->type, ple->lpgmf) ) {
        SS_WARNING( "wglUseFontOutlines failed\n" );
        return FAILURE;
    }
    return SUCCESS;
}
#endif

/******************************Public*Routine******************************\
* FindExtendedLUTEntry
*
* Searches through the extended character LUT, and returns ptr to the
* char's info if found, otherwise NULL.
*
\**************************************************************************/

static LISTENTRY*
FindExtendedLUTEntry( TCHAR c, WglFontContext *pwfc )
{
    int i;
    LISTENTRY *ple = pwfc->listLUT + MAX_DIRECT_LUT;

    for( i = 0; i < pwfc->LUTIndex; i ++, ple++ ) {
        if( ple->glyph == c )
            return ple;
    }
    return NULL;
}

/******************************Public*Routine******************************\
* DrawString
*
* Draws string by calling cmd list for each char.
*
\**************************************************************************/

void
DrawString(  USHORT           *string,
             int              strLen,
             WglFontContext   *pwfc )
{
    if( !pwfc )
        return;

    glCallLists(strLen, GL_UNSIGNED_SHORT, (GLushort *) string);
}

/******************************Public*Routine******************************\
* GetStringExtent
*
* Calculate a string's origin and extent in world coordinates.  
* 
* The origin is determined by the first character's location, and thereafter 
* each char's location is determined by adding the previous char's cellInc 
* values.
* 
* This will work for all string orientations.
*
\**************************************************************************/

int 
GetStringExtent(  LPTSTR          szString, 
                  POINTFLOAT      *extent,
                  POINTFLOAT      *origin,
                  WglFontContext  *pwfc )
{

    int len, strLen;
    TCHAR *c;
    TUCHAR cindex;
    LPGLYPHMETRICSFLOAT lpgmf;
    POINTFLOAT cellOrigin = {0.0f, 0.0f};
    POINTFLOAT extentOrigin = {FLT_MAX, -FLT_MAX};
    POINTFLOAT extentLowerRight = {-FLT_MAX, FLT_MAX};
    POINTFLOAT boxOrigin, boxLowerRight;
    LISTENTRY *listLUT, *ple;
#ifdef SS_DBCS_SUPPORT
    BOOL bIsLeadByte;
#endif

    if( !pwfc )
        return 0;

    listLUT = pwfc->listLUT;

    extent->x = extent->y = 0.0f;
    origin->x = origin->y = 0.0f;
    len = strLen = lstrlen( szString );
    if( !len )
        return 0;  // otherwise extents will be calc'd erroneously

    c = szString;

    for( ; strLen; strLen--, c++ ) {
        cindex = (TUCHAR) *c;
#ifdef SS_DBCS_SUPPORT
        if( bIsLeadByte = IsDBCSLeadByte(cindex) ) {
            // iterate over lead byte
            c++;
            strLen--;
            cindex = (TUCHAR) *c;
        }
        if( !bIsLeadByte && (cindex < MAX_DIRECT_LUT) )
#else
        if( cindex < MAX_DIRECT_LUT )
#endif
            ple = &listLUT[cindex];
        else
            ple = FindExtendedLUTEntry( *c, pwfc );

        lpgmf = ple->lpgmf;

        if( !lpgmf )
            // Memory must be running low, but keep going
            continue;

        // calc global position of this char's BlackBox (this is 
        //  'upper left')
        boxOrigin.x = cellOrigin.x + lpgmf->gmfptGlyphOrigin.x;
        boxOrigin.y = cellOrigin.y + lpgmf->gmfptGlyphOrigin.y;

        // calc lower right position
        boxLowerRight.x = boxOrigin.x + lpgmf->gmfBlackBoxX;
        boxLowerRight.y = boxOrigin.y - lpgmf->gmfBlackBoxY;

        // compare against the current bounding box

        if( boxOrigin.x < extentOrigin.x )
            extentOrigin.x = boxOrigin.x;
        if( boxOrigin.y > extentOrigin.y )
            extentOrigin.y = boxOrigin.y;
        if( boxLowerRight.x > extentLowerRight.x )
            extentLowerRight.x = boxLowerRight.x;
        if( boxLowerRight.y < extentLowerRight.y )
            extentLowerRight.y = boxLowerRight.y;

        // set global position of next cell
        cellOrigin.x = cellOrigin.x + lpgmf->gmfCellIncX;
        cellOrigin.y = cellOrigin.y + lpgmf->gmfCellIncY;
    }

    // Check for possible total lack of glyphmetric info
    if( extentOrigin.x == FLT_MAX ) {
        // Can assume if this value is still maxed out that all glyphmetric
        // info was NULL
        origin->x = origin->y = 0.0f;
        extent->x = extent->y = 0.0f;
        return 0;
    }

    *origin = extentOrigin;
    extent->x = extentLowerRight.x - extentOrigin.x;
    extent->y = extentOrigin.y - extentLowerRight.y;
    return len;
}
