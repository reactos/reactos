/******************************Module*Header*******************************\
* Module Name: texture.c
*
* Texture handling functions
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <scrnsave.h>
#include <commdlg.h>
#include <GL/gl.h>
#include "tk.h"

#include "scrnsave.h"  // for hMainInstance
#include "sscommon.h"
#include "texture.h"

static int ProcessTexture( TEXTURE *pTex );
static int ProcessTkTexture( TK_RGBImageRec *image, TEXTURE *pTex );
static int VerifyTextureFile( TEXFILE *pTexFile );
static int GetTexFileType( TEXFILE *pTexFile );

static TEX_STRINGS gts = {0};
BOOL gbTextureObjects = FALSE;
static BOOL gbPalettedTexture = FALSE;
static PFNGLCOLORTABLEEXTPROC pfnColorTableEXT;
static PFNGLCOLORSUBTABLEEXTPROC pfnColorSubTableEXT;

static BOOL gbEnableErrorMsgs = FALSE;

/******************************Public*Routine******************************\
*
* ss_LoadTextureResourceStrings
*
* Load various messages and strings that are used in processing textures,
* into global TEX_STRINGS structure
*
\**************************************************************************/

BOOL
ss_LoadTextureResourceStrings()
{
    LPTSTR pszStr;

    // title for choose texture File dialog
    LoadString(hMainInstance, IDS_TEXTUREDIALOGTITLE, gts.szTextureDialogTitle, 
                GEN_STRING_SIZE);
    LoadString(hMainInstance, IDS_BMP, gts.szBmp, GEN_STRING_SIZE);
    LoadString(hMainInstance, IDS_DOTBMP, gts.szDotBmp, GEN_STRING_SIZE);

    // szTextureFilter requires a little more work.  Need to assemble the file
    // name filter string, which is composed of two strings separated by a NULL
    // and terminated with a double NULL.

    LoadString(hMainInstance, IDS_TEXTUREFILTER, gts.szTextureFilter, 
                GEN_STRING_SIZE);
    pszStr = &gts.szTextureFilter[lstrlen(gts.szTextureFilter)+1];
    LoadString(hMainInstance, IDS_STARDOTBMP, pszStr, GEN_STRING_SIZE);
    pszStr += lstrlen(pszStr);
    *pszStr++ = TEXT(';');
    LoadString(hMainInstance, IDS_STARDOTRGB, pszStr, GEN_STRING_SIZE);
    pszStr += lstrlen(pszStr);
    pszStr++;
    *pszStr = TEXT('\0');

    LoadString(hMainInstance, IDS_WARNING, gts.szWarningMsg, MAX_PATH);
    LoadString(hMainInstance, IDS_SELECT_ANOTHER_BITMAP, 
                gts.szSelectAnotherBitmapMsg, MAX_PATH );

    LoadString(hMainInstance, IDS_BITMAP_INVALID, 
                gts.szBitmapInvalidMsg, MAX_PATH );
    LoadString(hMainInstance, IDS_BITMAP_SIZE, 
                gts.szBitmapSizeMsg, MAX_PATH );

    // assumed here that all above calls loaded properly (mf: fix later)
    return TRUE;
}

/******************************Public*Routine******************************\
*
*
\**************************************************************************/

void
ss_DisableTextureErrorMsgs()
{
    gbEnableErrorMsgs = FALSE;
}

/******************************Public*Routine******************************\
*
* ss_LoadBMPTextureFile
*
* Loads a BMP file and prepares it for GL usage
*
\**************************************************************************/

int 
ss_LoadBMPTextureFile( LPCTSTR pszBmpfile, TEXTURE *pTex )
{
    TK_RGBImageRec *image = (TK_RGBImageRec *) NULL;

#ifdef UNICODE
    image = tkDIBImageLoadAW( (char *) pszBmpfile, TRUE );
#else
    image = tkDIBImageLoadAW( (char *) pszBmpfile, FALSE );
#endif

    if( !image )  {
        return 0;
    }
    return ProcessTkTexture( image, pTex );
}

/******************************Public*Routine******************************\
*
* ss_LoadTextureFile
*
* Loads a BMP file and prepares it for GL usage
*
\**************************************************************************/

int 
ss_LoadTextureFile( TEXFILE *pTexFile, TEXTURE *pTex )
{
    TK_RGBImageRec *image = (TK_RGBImageRec *) NULL;
    LPTSTR pszBmpfile = pTexFile->szPathName;
    int type;
    LPTSTR pszStr;

    // Verify file / set type
    
    if( !(type = VerifyTextureFile( pTexFile )) )
        return 0;

    if( type == TEX_BMP ) {

#ifdef UNICODE
        image = tkDIBImageLoadAW( (char *) pszBmpfile, TRUE );
#else
        image = tkDIBImageLoadAW( (char *) pszBmpfile, FALSE );
#endif
    } else {
#ifdef UNICODE
        image = tkRGBImageLoadAW( (char *) pszBmpfile, TRUE );
#else
        image = tkRGBImageLoadAW( (char *) pszBmpfile, FALSE );
#endif
    }

    if( !image )  {
        return 0;
    }
    return ProcessTkTexture( image, pTex );
}

/******************************Public*Routine******************************\
*
* ss_LoadTextureResource
*
* Loads a BMP or RGB texture resource and prepares it for GL usage
*
\**************************************************************************/

int 
ss_LoadTextureResource( TEX_RES *pTexRes, TEXTURE *pTex )
{
    HMODULE ghmodule;
    HRSRC hr;
    HGLOBAL hg;
    LPVOID pv;
    LPCTSTR lpType;
    BOOL fLoaded = FALSE;

    ghmodule = GetModuleHandle(NULL);
    switch(pTexRes->type)
    {
    case TEX_RGB:
        lpType = MAKEINTRESOURCE(RT_RGB);
        break;
    case TEX_BMP:
        lpType = MAKEINTRESOURCE(RT_MYBMP);
        break;
    case TEX_A8:
        lpType = MAKEINTRESOURCE(RT_A8);
        break;
    }

    hr = FindResource(ghmodule, MAKEINTRESOURCE(pTexRes->name), lpType);
    if (hr == NULL)
    {
        goto EH_NotFound;
    }
    hg = LoadResource(ghmodule, hr);
    if (hg == NULL)
    {
        goto EH_FreeResource;
    }
    pv = (PSZ)LockResource(hg);
    if (pv == NULL)
    {
        goto EH_FreeResource;
    }

    switch(pTexRes->type)
    {
    case TEX_RGB:
        fLoaded = ss_RGBImageLoad( pv, pTex );
        break;
    case TEX_BMP:
        fLoaded = ss_DIBImageLoad( pv, pTex );
        break;
    case TEX_A8:
        fLoaded = ss_A8ImageLoad( pv, pTex );
        break;
    }

 EH_FreeResource:
    FreeResource(hr);
 EH_NotFound:
    
    if( !fLoaded )  {
        return 0;
    }

    return ProcessTexture( pTex );
}


/******************************Public*Routine******************************\
*
* ValidateTextureSize
* 
* - Scales the texture to powers of 2
*
\**************************************************************************/

static BOOL
ValidateTextureSize( TEXTURE *pTex )
{
    double xPow2, yPow2;
    int ixPow2, iyPow2;
    int xSize2, ySize2;
    float fxFact, fyFact;
    GLint glMaxTexDim;

    if( (pTex->width <= 0) || (pTex->height <= 0) ) {
        SS_WARNING( "ValidateTextureSize : invalid texture dimensions\n" );
        return FALSE;
    }

    pTex->origAspectRatio = (float) pTex->height / (float) pTex->width;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexDim);
    if( glMaxTexDim <= 0 )
        return FALSE;

    if( pTex->format != GL_COLOR_INDEX ) {

        // We limit the max dimension here for performance reasons
        glMaxTexDim = min(256, glMaxTexDim);

        if (pTex->width <= glMaxTexDim)
            xPow2 = log((double)pTex->width) / log((double)2.0);
        else
            xPow2 = log((double)glMaxTexDim) / log((double)2.0);

        if (pTex->height <= glMaxTexDim)
            yPow2 = log((double)pTex->height) / log((double)2.0);
        else
            yPow2 = log((double)glMaxTexDim) / log((double)2.0);

        ixPow2 = (int)xPow2;
        iyPow2 = (int)yPow2;

        // Always scale to higher nearest power
        if (xPow2 != (double)ixPow2)
            ixPow2++;
        if (yPow2 != (double)iyPow2)
            iyPow2++;

        xSize2 = 1 << ixPow2;
        ySize2 = 1 << iyPow2;

        if (xSize2 != pTex->width ||
            ySize2 != pTex->height)
        {
            BYTE *pData;

            pData = (BYTE *) malloc(xSize2 * ySize2 * pTex->components * sizeof(BYTE));
            if (!pData) {
                SS_WARNING( "ValidateTextureSize : can't alloc pData\n" );
                return FALSE;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if( gluScaleImage(pTex->format, pTex->width, pTex->height,
                      GL_UNSIGNED_BYTE, pTex->data,
                      xSize2, ySize2, GL_UNSIGNED_BYTE,
                      pData) )
            {
                // glu failure
                SS_WARNING( "ValidateTextureSize : gluScaleImage failure\n" );
                return FALSE;
            }
        
            // set the new width,height,data
            pTex->width = xSize2;
            pTex->height = ySize2;
            free(pTex->data);
            pTex->data = pData;
        }
    } else {  // paletted texture case
        //mf
        // paletted texture: must be power of 2 - but might need to enforce
        // here if not done in a8 load.  Also have to check against
        // GL_MAX_TEXTURE_SIZE.  Could then clip it to power of 2 size
    }
    return TRUE;
}

/******************************Public*Routine******************************\
*
* SetDefaultTextureParams
*
\**************************************************************************/

static void
SetDefaultTextureParams()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/******************************Public*Routine******************************\
*
* ProcessTexture
*
* - Verifies texture size
* - Fills out TEXTURE structure with required data
* - Creates a texture object if extension exists
*
\**************************************************************************/

static int 
ProcessTexture( TEXTURE *pTex )
{
    // Enforce proper texture size (power of 2, etc.)

    if( !ValidateTextureSize( pTex ) )
        return 0;

    // if texturing objects available, init the object
    if( gbTextureObjects ) {
        glGenTextures( 1, &pTex->texObj );
        glBindTexture( GL_TEXTURE_2D, pTex->texObj );

        // Default attributes for texObj
        SetDefaultTextureParams();

        glTexImage2D( GL_TEXTURE_2D, 0, pTex->components,
                      pTex->width, pTex->height, 0, pTex->format,
                      GL_UNSIGNED_BYTE, pTex->data );
        
        if (gbPalettedTexture && pTex->pal != NULL)
        {
            pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pTex->pal_size,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, pTex->pal);
        }
    } else
        pTex->texObj = 0;

    return 1;
}

/******************************Public*Routine******************************\
*
* ProcessTkTexture
*
* Simple wrapper for ProcessTexture which fills out a TEXTURE
* from a TK_RGBImageRec
*
* Frees the ImageRec if ProcessTexture succeeds
*
\**************************************************************************/

static int
ProcessTkTexture( TK_RGBImageRec *image, TEXTURE *pTex )
{

    pTex->width = image->sizeX;
    pTex->height = image->sizeY;
    pTex->format = GL_RGB;
    pTex->components = 3;
    pTex->data = image->data;
    pTex->pal_size = 0;
    pTex->pal = NULL;

    if( ProcessTexture( pTex ) )
    {
        free(image);
        return 1;
    }
    else
    {
        return 0;
    }
}
    
/******************************Public*Routine******************************\
*
* ss_SetTexture
*
\**************************************************************************/

void
ss_SetTexture( TEXTURE *pTex )
{
    if( pTex == NULL )
        return;

    if( gbTextureObjects && pTex->texObj ) {
        glBindTexture( GL_TEXTURE_2D, pTex->texObj );
        return;
    }
    
    glTexImage2D( GL_TEXTURE_2D, 0, pTex->components,
                  pTex->width, pTex->height, 0, pTex->format,
                  GL_UNSIGNED_BYTE, pTex->data );
        
    if (gbPalettedTexture && pTex->pal != NULL)
    {
        pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pTex->pal_size,
                         GL_BGRA_EXT, GL_UNSIGNED_BYTE, pTex->pal);
    }
}

    
/******************************Public*Routine******************************\
*
* ss_CopyTexture
*
* Make a copy of a texture.
*
\**************************************************************************/

BOOL
ss_CopyTexture( TEXTURE *pTexDst, TEXTURE *pTexSrc )
{
    int size;

    if( (pTexDst == NULL) || (pTexSrc == NULL) )
        return FALSE;

    *pTexDst = *pTexSrc;

    if( gbTextureObjects && pTexSrc->texObj ) {
        glGenTextures( 1, &pTexDst->texObj );
    }
    
    // copy image data

    size = pTexSrc->width * pTexSrc->height;
    if( pTexSrc->components != GL_COLOR_INDEX8_EXT )
        size *= pTexSrc->components; // since data format always UNSIGNED_BYTE

    pTexDst->data = (unsigned char *) malloc( size );
    if( pTexDst->pal == NULL )
        return FALSE;
    memcpy( pTexDst->data, pTexSrc->data, size );

    // copy palette data

    if( gbPalettedTexture && pTexSrc->pal != NULL )
    {
        size = pTexSrc->pal_size*sizeof(RGBQUAD);
        pTexDst->pal = (RGBQUAD *) malloc(size);
        if( pTexDst->pal == NULL )
        {
            free(pTexDst->data);
            return FALSE;
        }
        memcpy( pTexDst->pal, pTexSrc->pal, size );
    }
    
    if( gbTextureObjects ) {
        glBindTexture( GL_TEXTURE_2D, pTexDst->texObj );

        // Default attributes for texObj
        SetDefaultTextureParams();

        glTexImage2D( GL_TEXTURE_2D, 0, pTexDst->components,
                      pTexDst->width, pTexDst->height, 0, pTexDst->format,
                      GL_UNSIGNED_BYTE, pTexDst->data );
        
        if( gbPalettedTexture && (pTexDst->pal != NULL) )
        {
            pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pTexDst->pal_size,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, pTexDst->pal);
        }
    }
    return TRUE;
}

/******************************Public*Routine******************************\
*
* ss_SetTexturePalette
*
* Set a texture's palette according to the supplied index. This index
* indicates the start of the palette, which then wraps around if necessary.
* Of course this only works on paletted textures.
*
\**************************************************************************/

void
ss_SetTexturePalette( TEXTURE *pTex, int index )
{
    if( pTex == NULL )
        return;

    if( gbTextureObjects )
        ss_SetTexture( pTex );

    if( gbPalettedTexture && pTex->pal != NULL )
    {
        int start, count;

        start = index & (pTex->pal_size - 1);
        count = pTex->pal_size - start;
        pfnColorSubTableEXT(GL_TEXTURE_2D, 0, count, GL_BGRA_EXT,
                            GL_UNSIGNED_BYTE, pTex->pal + start);
        if (start != 0)
        {
            pfnColorSubTableEXT(GL_TEXTURE_2D, count, start, GL_BGRA_EXT,
                                GL_UNSIGNED_BYTE, pTex->pal);
        }
    }
}

/******************************Public*Routine******************************\
*
* SetTextureAlpha
*
* Set a constant alpha value for the texture
* Again, don't overwrite any existing 0 alpha values, as explained in
* ss_SetTextureTransparency
*
\**************************************************************************/

static void
SetTextureAlpha( TEXTURE *pTex, float fAlpha )
{
    int i;
    unsigned char *pData = pTex->data;
    RGBA8 *pColor = (RGBA8 *) pTex->data;
    BYTE bAlpha = (BYTE) (fAlpha * 255.0f);

    if( pTex->components != 4 )
        return;

    for( i = 0; i < pTex->width*pTex->height; i ++, pColor++ ) {
        if( pColor->a != 0 ) 
            pColor->a = bAlpha;
    }
}

/******************************Public*Routine******************************\
*
* ConvertTextureToRGBA
*
* Convert RGB texture to RGBA
*
\**************************************************************************/

static void
ConvertTextureToRGBA( TEXTURE *pTex, float fAlpha )
{
    unsigned char *pNewData;
    int count = pTex->width * pTex->height;
    unsigned char *src, *dst;
    BYTE bAlpha = (BYTE) (fAlpha * 255.0f);
    int i;

    pNewData = (unsigned char *) LocalAlloc(LMEM_FIXED, count * sizeof(RGBA8));
    if( !pNewData )
        return;

    src = pTex->data;
    dst = pNewData;
    // Note: the color ordering is ABGR, where R is lsb
    for( i = 0; i < count; i ++ ) {
        *((RGB8 *)dst) = *((RGB8 *)src);
        dst += sizeof(RGB8);
        src += sizeof(RGB8);
        *dst++ = bAlpha;
    }
    LocalFree( pTex->data );
    pTex->data = pNewData;
    pTex->components = 4;
    pTex->format = GL_RGBA;
}

/******************************Public*Routine******************************\
*
* ss_SetTextureTransparency
*
* Set transparency for a texture by adding or modifying the alpha data.  
* Transparency value must be between 0.0 (opaque) and 1.0 (fully transparent)
* If the texture data previously had no alpha, add it in.
* If bSet is TRUE, make this the current texture.
*
* Note: Currently fully transparent pixels (alpha=0) will not be altered, since
* it is assumed these should be permanently transparent (could make this an
* option? - bPreserveTransparentPixels )
*
\**************************************************************************/

BOOL
ss_SetTextureTransparency( TEXTURE *pTex, float fTransp, BOOL bSet )
{
    int i;
    float fAlpha;

    if( pTex == NULL )
        return FALSE;

    SS_CLAMP_TO_RANGE2( fTransp, 0.0f, 1.0f );
    fAlpha = 1 - fTransp;

    if( pTex->format == GL_COLOR_INDEX )
    {
        // just need to modify the palette
            RGBQUAD *pPal = pTex->pal;
            BYTE bAlpha = (BYTE) (fAlpha * 255.0f);

            if( !pPal )
                return FALSE;

            for( i = 0; i < pTex->pal_size; i ++, pPal++ ) {
                if( pPal->rgbReserved != 0 )
                    pPal->rgbReserved = bAlpha;
            }
        
            // need to send down the new palette for texture objects
            if( gbTextureObjects && gbPalettedTexture )
            {
                glBindTexture( GL_TEXTURE_2D, pTex->texObj );
                pfnColorTableEXT(GL_TEXTURE_2D, GL_RGBA, pTex->pal_size,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, pTex->pal);
            }
    }
    else {
        // Need to setup new texture data
        if( pTex->components != 4 ) {
            // Make room for alpha component
            //mf: ? change to bAlpha ?
            ConvertTextureToRGBA( pTex, fAlpha );
        } else {
            // Set alpha component
            SetTextureAlpha( pTex, fAlpha );
        }
        // Send down new data if texture objects
        if( gbTextureObjects )
        {
            glBindTexture( GL_TEXTURE_2D, pTex->texObj );
            glTexImage2D( GL_TEXTURE_2D, 0, pTex->components,
                          pTex->width, pTex->height, 0, pTex->format,
                          GL_UNSIGNED_BYTE, pTex->data );
        }
    }

    if( bSet )
        ss_SetTexture( pTex );

    return TRUE;
}

/******************************Public*Routine******************************\
*
* ss_DeleteTexture
*
\**************************************************************************/

void
ss_DeleteTexture( TEXTURE *pTex )
{
    if( pTex == NULL )
        return;

    if( gbTextureObjects && pTex->texObj ) {
        glDeleteTextures( 1, &pTex->texObj );
        pTex->texObj = 0;
    }
    if (pTex->pal != NULL)
    {
        free(pTex->pal);
    }
    if( pTex->data )
        free( pTex->data );
}



/******************************Public*Routine******************************\
*
* ss_TextureObjectsEnabled
*
* Returns BOOL set by ss_QueryGLVersion (Texture Objects only supported on
* GL v.1.1 or greater)
*
\**************************************************************************/

BOOL
ss_TextureObjectsEnabled( void )
{
    return gbTextureObjects;
}

/******************************Public*Routine******************************\
*
* ss_PalettedTextureEnabled
*
* Returns result from ss_QueryPalettedTextureEXT
*
\**************************************************************************/

BOOL
ss_PalettedTextureEnabled( void )
{
    return gbPalettedTexture;
}

/******************************Public*Routine******************************\
*
* ss_QueryPalettedTextureEXT
*
* Queries the OpenGL implementation to see if paletted texture is supported
* Typically called once at app startup.
*
\**************************************************************************/

BOOL
ss_QueryPalettedTextureEXT( void )
{
    PFNGLGETCOLORTABLEPARAMETERIVEXTPROC pfnGetColorTableParameterivEXT;
    int size;

    pfnColorTableEXT = (PFNGLCOLORTABLEEXTPROC)
        wglGetProcAddress("glColorTableEXT");
    if (pfnColorTableEXT == NULL)
        return FALSE;
    pfnColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC)
        wglGetProcAddress("glColorSubTableEXT");
    if (pfnColorSubTableEXT == NULL)
        return FALSE;
        
    // Check color table size
    pfnGetColorTableParameterivEXT = (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)
        wglGetProcAddress("glGetColorTableParameterivEXT");
    if (pfnGetColorTableParameterivEXT == NULL)
        return FALSE;
    // For now, the only paletted textures supported in this lib are TEX_A8,
    // with 256 color table entries.  Make sure the device supports this.
    pfnColorTableEXT(GL_PROXY_TEXTURE_2D, GL_RGBA, 256,
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL );
    pfnGetColorTableParameterivEXT( GL_PROXY_TEXTURE_2D, 
                                    GL_COLOR_TABLE_WIDTH_EXT, &size );
    if( size != 256 )
        // The device does not support a color table size of 256, so we don't
        // enable paletted textures in general.
        return FALSE;

    return gbPalettedTexture=TRUE;
}


/******************************Public*Routine******************************\
*
* ss_VerifyTextureFile
*
* Validates texture bmp or rgb file, by checking for valid pathname and
* correct format.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
*  Jul. 25, 95 : [marcfo]
*    - Suppress warning dialog box in child preview mode, as it will
*      be continuously brought up.
*
*  Dec. 12, 95 : [marcfo]
*     - Support .rgb files as well
*
*  Dec. 14, 95 : [marcfo]
*     - Change to have it only check the file path
*
\**************************************************************************/

BOOL
ss_VerifyTextureFile( TEXFILE *ptf )
{
    // Make sure the selected texture file is OK.

    ISIZE size;
    TCHAR szFileName[MAX_PATH];
    PTSTR pszString;
    TCHAR szString[MAX_PATH];

    lstrcpy(szFileName, ptf->szPathName);

    if ( SearchPath(NULL, szFileName, NULL, MAX_PATH,
                     ptf->szPathName, &pszString)
       )
    {
        ptf->nOffset = (int)((ULONG_PTR)(pszString - ptf->szPathName));
        return TRUE;
    }
    else
    {
        lstrcpy(ptf->szPathName, szFileName);    // restore

        if( !ss_fOnWin95() && gbEnableErrorMsgs )
        {
            wsprintf(szString, gts.szSelectAnotherBitmapMsg, ptf->szPathName);
            MessageBox(NULL, szString, gts.szWarningMsg, MB_OK);
        }
        return FALSE;
    }
}


/******************************Public*Routine******************************\
*
* ss_SelectTextureFile
*
* Use the common dialog GetOpenFileName to get the name of a bitmap file
* for use as a texture.  This function will not return until the user
* either selects a valid bitmap or cancels.  If a valid bitmap is selected
* by the user, the global array szPathName will have the full path
* to the bitmap file and the global value nOffset will have the
* offset from the beginning of szPathName to the pathless file name.
*
* If the user cancels, szPathName and nOffset will remain
* unchanged.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
*    - Wrote it.
*  Apr. 28, 95 : [marcfo]
*    - Modified for common use
*  Dec. 12, 95 : [marcfo]
*    - Support .rgb files as well
*
\**************************************************************************/

BOOL
ss_SelectTextureFile( HWND hDlg, TEXFILE *ptf )
{
    OPENFILENAME ofn;
    TCHAR dirName[MAX_PATH];
    TEXFILE newTexFile;
    LPTSTR pszFileName = newTexFile.szPathName;
    TCHAR origPathName[MAX_PATH];
    PTSTR pszString;
    BOOL bTryAgain, bFileSelected;

//mf: 
    gbEnableErrorMsgs = TRUE;

    // Make a copy of the original file path name, so we can tell if
    // it changed or not
    lstrcpy( origPathName, ptf->szPathName );

    // Make dialog look nice by parsing out the initial path and
    // file name from the full pathname.  If this isn't done, then
    // dialog has a long ugly name in the file combo box and
    // directory will end up with the default current directory.

    if (ptf->nOffset) {
    // Separate the directory and file names.

        lstrcpy(dirName, ptf->szPathName);
        dirName[ptf->nOffset-1] = L'\0';
        lstrcpy(pszFileName, &ptf->szPathName[ptf->nOffset]);
    }
    else {
    // If nOffset is zero, then szPathName is not a full path.
    // Attempt to make it a full path by calling SearchPath.

        if ( SearchPath(NULL, ptf->szPathName, NULL, MAX_PATH,
                         dirName, &pszString) )
        {
        // Successful.  Go ahead a change szPathName to the full path
        // and compute the filename offset.

            lstrcpy(ptf->szPathName, dirName);
            ptf->nOffset = (int)((ULONG_PTR)(pszString - dirName));

        // Break the filename and directory paths apart.

            dirName[ptf->nOffset-1] = TEXT('\0');
            lstrcpy(pszFileName, pszString);
        }

    // Give up and use the Windows system directory.

        else
        {
            GetWindowsDirectory(dirName, MAX_PATH);
            lstrcpy(pszFileName, ptf->szPathName);
        }
    }

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = gts.szTextureFilter;
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPTSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = dirName;
    ofn.lpstrTitle = gts.szTextureDialogTitle;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = gts.szBmp;
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPTSTR) NULL;

    do {
    // Invoke the common file dialog.  If it succeeds, then validate
    // the bitmap file.  If not valid, make user try again until either
    // they pick a good one or cancel the dialog.

        bTryAgain = FALSE;

        if ( bFileSelected = GetOpenFileName(&ofn) ) {
            ISIZE size;

            newTexFile.nOffset = ofn.nFileOffset;
            if( VerifyTextureFile( &newTexFile ) ) {
                // copy in new file and offset
                *ptf = newTexFile;
            }
            else {
                bTryAgain = TRUE;
            }
        }

    // If need to try again, recompute dir and file name so dialog
    // still looks nice.

        if (bTryAgain && ofn.nFileOffset) {
            lstrcpy(dirName, pszFileName);
            dirName[ofn.nFileOffset-1] = L'\0';
            lstrcpy(pszFileName, &pszFileName[ofn.nFileOffset]);
        }

    } while (bTryAgain);

    gbEnableErrorMsgs = FALSE;

    if( bFileSelected ) {
        if( lstrcmpi( origPathName, ptf->szPathName ) )
            // a different file was selected
            return TRUE;
    }
    return FALSE;
}


/******************************Public*Routine******************************\
*
* ss_GetDefaultBmpFile
*
* Determine a default bitmap file to use for texturing, if none
* exists yet in the registry.  
*
* Put default in BmpFile parameter.   DotBmp parameter is the bitmap
* extension (usually .bmp).
*
* We have to synthesise the name from the ProductType registry entry.
* Currently, this can be WinNT, LanmanNT, or Server.  If it is
* WinNT, the bitmap is winnt.bmp.  If it is LanmanNT or Server,
* the bitmap is lanmannt.bmp.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
*  Jul. 27, 95 : [marcfo]
*    - Added support for win95
*
*  Apr. 23, 96 : [marcfo]
*    - Return NULL string for win95
*
\**************************************************************************/

void
ss_GetDefaultBmpFile( LPTSTR pszBmpFile )
{
    HKEY   hkey;
    LONG   cjDefaultBitmap = MAX_PATH;

    if( ss_fOnWin95() )
        // There is no 'nice' bmp file on standard win95 installations
        lstrcpy( pszBmpFile, TEXT("") );
    else {
        if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 (LPCTSTR) TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                 0,
                 KEY_QUERY_VALUE,
                 &hkey) == ERROR_SUCCESS )
        {

            if ( RegQueryValueEx(hkey,
                                  TEXT("ProductType"),
                                  (LPDWORD) NULL,
                                  (LPDWORD) NULL,
                                  (LPBYTE) pszBmpFile,
                                  (LPDWORD) &cjDefaultBitmap) == ERROR_SUCCESS
                 && (cjDefaultBitmap / sizeof(TCHAR) + 4) <= MAX_PATH )
                lstrcat( pszBmpFile, gts.szDotBmp );
            else
                lstrcpy( pszBmpFile, TEXT("winnt.bmp") );

            RegCloseKey(hkey);
        }
        else
            lstrcpy( pszBmpFile, TEXT("winnt.bmp") );

    // If its not winnt.bmp, then its lanmannt.bmp.  (This would be a lot
    // cleaner both in the screen savers and for usersrv desktop bitmap
    // initialization if the desktop bitmap name were stored in the
    // registry).

        if ( lstrcmpi( pszBmpFile, TEXT("winnt.bmp") ) != 0 )
            lstrcpy( pszBmpFile, TEXT("lanmannt.bmp") );
    }
}

/******************************Public*Routine******************************\
*
* VerifyTextureFile
*
* Verify that a bitmap or rgb file is valid
*
* Returns:
*   File type (RGB or BMP) if valid file; otherwise, 0.
*
* History
*  Dec. 12, 95 : [marcfo]
*    - Creation
*
\**************************************************************************/

static int
VerifyTextureFile( TEXFILE *pTexFile )
{
    int type;
    ISIZE size;
    BOOL bValid;
    TCHAR szString[2 * MAX_PATH]; // May contain a pathname

    // check for 0 offset and null strings
    if( (pTexFile->nOffset == 0) || (*pTexFile->szPathName == 0) )
        return 0;

    type = GetTexFileType( pTexFile );

    switch( type ) {
        case TEX_BMP:
            bValid = bVerifyDIB( pTexFile->szPathName, &size );
            break;
        case TEX_RGB:
            bValid = bVerifyRGB( pTexFile->szPathName, &size );
            break;
        case TEX_UNKNOWN:
        default:
            bValid = FALSE;
    }

    if( !bValid ) {
        if( gbEnableErrorMsgs ) {
            wsprintf(szString, gts.szSelectAnotherBitmapMsg, pTexFile->szPathName);
            MessageBox(NULL, szString, gts.szWarningMsg, MB_OK);
        }
        return 0;
    }

    // Check size ?

    if ( (size.width > TEX_WIDTH_MAX)     || 
         (size.height > TEX_HEIGHT_MAX) )
    {
        if( gbEnableErrorMsgs )
        {
            wsprintf(szString, gts.szBitmapSizeMsg, 
                      TEX_WIDTH_MAX, TEX_HEIGHT_MAX);
            MessageBox(NULL, szString, gts.szWarningMsg, MB_OK);
        }
        return 0;
    }

    return type;
}

/******************************Public*Routine******************************\
*
* ss_InitAutoTexture
*
* Generate texture coordinates automatically.
* If pTexRep is not NULL, use it to set the repetition of the generated
* texture.
*
\**************************************************************************/

void
ss_InitAutoTexture( TEX_POINT2D *pTexRep )
{
    GLfloat sgenparams[] = {1.0f, 0.0f, 0.0f, 0.0f};
    GLfloat tgenparams[] = {0.0f, 1.0f, 0.0f, 0.0f};

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    if( pTexRep )
        sgenparams[0] = pTexRep->s;
    glTexGenfv(GL_S, GL_OBJECT_PLANE, sgenparams );

    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    if( pTexRep )
        tgenparams[0] = pTexRep->t;
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tgenparams );

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable( GL_TEXTURE_2D );
}

/******************************Public*Routine******************************\
*
* GetTexFileType
*
* Determine if a texture file is rgb or bmp, based on extension.  This is
* good enough, as the open texture dialog only shows files with these
* extensions.
*
\**************************************************************************/

static int
GetTexFileType( TEXFILE *pTexFile )
{
    LPTSTR pszStr;

#ifdef UNICODE
    pszStr = wcsrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#else
    pszStr = strrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#endif
    if( !pszStr || (lstrlen(++pszStr) == 0) )
        return TEX_UNKNOWN;

    if( !lstrcmpi( pszStr, TEXT("bmp") ) )
        return TEX_BMP;
    else if( !lstrcmpi( pszStr, TEXT("rgb") ) )
        return TEX_RGB;
    else
        return TEX_UNKNOWN;
}
