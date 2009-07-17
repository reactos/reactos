#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

/**********************************************************************
 *          CreateEnhMetaFileW   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileW(
    HDC           hdc,        /* [in] optional reference DC */
    LPCWSTR       filename,   /* [in] optional filename for disk metafiles */
    const RECT*   rect,       /* [in] optional bounding rectangle */
    LPCWSTR       description /* [in] optional description */
    )
{
    UNIMPLEMENTED;
    return 0;
}

/**********************************************************************
 *          CreateEnhMetaFileA   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileA(
    HDC hdc,           /* [in] optional reference DC */
    LPCSTR filename,   /* [in] optional filename for disk metafiles */
    const RECT *rect,  /* [in] optional bounding rectangle */
    LPCSTR description /* [in] optional description */
    )
{
    UNIMPLEMENTED;
    return 0;
}

/**********************************************************************
 *          CreateMetaFileA   (GDI32.@)
 *
 * See CreateMetaFileW.
 */
HDC WINAPI CreateMetaFileA(LPCSTR filename)
{
    UNIMPLEMENTED;
    return 0;
}

/**********************************************************************
 *	     CreateMetaFileW   (GDI32.@)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 *
 * PARAMS
 *  filename [I] Filename of disk metafile
 *
 * RETURNS
 *  A handle to the metafile DC if successful, NULL on failure.
 */
HDC WINAPI CreateMetaFileW( LPCWSTR filename )
{
    UNIMPLEMENTED;
    return 0;
}

/******************************************************************
 *             CloseEnhMetaFile (GDI32.@)
 */
HENHMETAFILE WINAPI CloseEnhMetaFile(HDC hdc) /* [in] metafile DC */
{
    UNIMPLEMENTED;
    return NULL;
}

/******************************************************************
 *	     CloseMetaFile   (GDI32.@)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 *
 * PARAMS
 *  hdc [I] Metafile DC to close 
 *
 * RETURNS
 *  Handle of newly created metafile on success, NULL on failure.
 */
HMETAFILE WINAPI CloseMetaFile(HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}
