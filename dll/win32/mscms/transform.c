/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2005, 2006, 2008 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS

#include <config.h>
#include <wine/debug.h>

//#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wingdi.h>
#include <winuser.h>
#include <icm.h>

//#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

#ifdef HAVE_LCMS2

static DWORD from_bmformat( BMFORMAT format )
{
    static BOOL quietfixme = FALSE;
    DWORD ret;

    switch (format)
    {
    case BM_RGBTRIPLETS: ret = TYPE_RGB_8; break;
    case BM_BGRTRIPLETS: ret = TYPE_BGR_8; break;
    case BM_GRAY:        ret = TYPE_GRAY_8; break;
    case BM_xRGBQUADS:   ret = TYPE_ARGB_8; break;
    case BM_xBGRQUADS:   ret = TYPE_ABGR_8; break;
    default:
        if (!quietfixme)
        {
            FIXME( "unhandled bitmap format %08x\n", format );
            quietfixme = TRUE;
        }
        ret = TYPE_RGB_8;
        break;
    }
    TRACE( "color space: %08x -> %08x\n", format, ret );
    return ret;
}

static DWORD from_type( COLORTYPE type )
{
    DWORD ret;

    switch (type)
    {
    case COLOR_GRAY: ret = TYPE_GRAY_16; break;
    case COLOR_RGB:  ret = TYPE_RGB_16; break;
    case COLOR_XYZ:  ret = TYPE_XYZ_16; break;
    case COLOR_Yxy:  ret = TYPE_Yxy_16; break;
    case COLOR_Lab:  ret = TYPE_Lab_16; break;
    case COLOR_CMYK: ret = TYPE_CMYK_16; break;
    default:
        FIXME( "unhandled color type %08x\n", type );
        ret = TYPE_RGB_16;
        break;
    }

    TRACE( "color type: %08x -> %08x\n", type, ret );
    return ret;
}

#endif /* HAVE_LCMS2 */

/******************************************************************************
 * CreateColorTransformA            [MSCMS.@]
 *
 * See CreateColorTransformW.
 */
HTRANSFORM WINAPI CreateColorTransformA( LPLOGCOLORSPACEA space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    LOGCOLORSPACEW spaceW;
    DWORD len;

    TRACE( "( %p, %p, %p, 0x%08x )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    memcpy( &spaceW, space, FIELD_OFFSET(LOGCOLORSPACEA, lcsFilename) );
    spaceW.lcsSize = sizeof(LOGCOLORSPACEW);

    len = MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, NULL, 0 );
    MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, spaceW.lcsFilename, len );

    return CreateColorTransformW( &spaceW, dest, target, flags );
}

/******************************************************************************
 * CreateColorTransformW            [MSCMS.@]
 *
 * Create a color transform.
 *
 * PARAMS
 *  space  [I] Input color space.
 *  dest   [I] Color profile of destination device.
 *  target [I] Color profile of target device.
 *  flags  [I] Flags.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */
HTRANSFORM WINAPI CreateColorTransformW( LPLOGCOLORSPACEW space, HPROFILE dest,
    HPROFILE target, DWORD flags )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS2
    struct transform transform;
    struct profile *dst, *tgt = NULL;
    cmsHPROFILE cmsinput, cmsoutput, cmstarget = NULL;
    DWORD proofing = 0;
    int intent;

    TRACE( "( %p, %p, %p, 0x%08x )\n", space, dest, target, flags );

    if (!space || !(dst = grab_profile( dest ))) return FALSE;

    if (target && !(tgt = grab_profile( target )))
    {
        release_profile( dst );
        return FALSE;
    }
    intent = space->lcsIntent > 3 ? INTENT_PERCEPTUAL : space->lcsIntent;

    TRACE( "lcsIntent:   %x\n", space->lcsIntent );
    TRACE( "lcsCSType:   %s\n", dbgstr_tag( space->lcsCSType ) );
    TRACE( "lcsFilename: %s\n", debugstr_w( space->lcsFilename ) );

    cmsinput = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */
    if (target)
    {
        proofing = cmsFLAGS_SOFTPROOFING;
        cmstarget = tgt->cmsprofile;
    }
    cmsoutput = dst->cmsprofile;
    transform.cmstransform = cmsCreateProofingTransform(cmsinput, 0, cmsoutput, 0, cmstarget,
                                                        intent, INTENT_ABSOLUTE_COLORIMETRIC,
                                                        proofing);
    if (!transform.cmstransform)
    {
        if (tgt) release_profile( tgt );
        release_profile( dst );
        return FALSE;
    }

    ret = create_transform( &transform );

    if (tgt) release_profile( tgt );
    release_profile( dst );

#endif /* HAVE_LCMS2 */
    return ret;
}

/******************************************************************************
 * CreateMultiProfileTransform      [MSCMS.@]
 *
 * Create a color transform from an array of color profiles.
 *
 * PARAMS
 *  profiles  [I] Array of color profiles.
 *  nprofiles [I] Number of color profiles.
 *  intents   [I] Array of rendering intents.
 *  flags     [I] Flags.
 *  cmm       [I] Profile to take the CMM from.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */ 
HTRANSFORM WINAPI CreateMultiProfileTransform( PHPROFILE profiles, DWORD nprofiles,
    PDWORD intents, DWORD nintents, DWORD flags, DWORD cmm )
{
    HTRANSFORM ret = NULL;
#ifdef HAVE_LCMS2
    cmsHPROFILE *cmsprofiles;
    struct transform transform;
    struct profile *profile0, *profile1;

    TRACE( "( %p, 0x%08x, %p, 0x%08x, 0x%08x, 0x%08x )\n",
           profiles, nprofiles, intents, nintents, flags, cmm );

    if (!profiles || !nprofiles || !intents) return NULL;

    if (nprofiles > 2)
    {
        FIXME("more than 2 profiles not supported\n");
        return NULL;
    }

    profile0 = grab_profile( profiles[0] );
    if (!profile0) return NULL;
    profile1 = grab_profile( profiles[1] );
    if (!profile1)
    {
        release_profile( profile0 );
        return NULL;
    }

    if ((cmsprofiles = HeapAlloc( GetProcessHeap(), 0, (nprofiles + 1) * sizeof(cmsHPROFILE) )))
    {
        cmsprofiles[0] = profile0->cmsprofile;
        cmsprofiles[1] = profile1->cmsprofile;

        transform.cmstransform = cmsCreateMultiprofileTransform( cmsprofiles, nprofiles, 0,
                                                                 0, *intents, 0 );
        HeapFree( GetProcessHeap(), 0, cmsprofiles );
        if (!transform.cmstransform)
        {
            release_profile( profile0 );
            release_profile( profile1 );
            return FALSE;
        }
        ret = create_transform( &transform );
    }

    release_profile( profile0 );
    release_profile( profile1 );

#endif /* HAVE_LCMS2 */
    return ret;
}

/******************************************************************************
 * DeleteColorTransform             [MSCMS.@]
 *
 * Delete a color transform.
 *
 * PARAMS
 *  transform [I] Handle to a color transform.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */ 
BOOL WINAPI DeleteColorTransform( HTRANSFORM handle )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS2

    TRACE( "( %p )\n", handle );

    ret = close_transform( handle );

#endif /* HAVE_LCMS2 */
    return ret;
}

/******************************************************************************
 * TranslateBitmapBits              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  srcbits      [I] Source bitmap.
 *  input        [I] Format of the source bitmap.
 *  width        [I] Width of the source bitmap.
 *  height       [I] Height of the source bitmap.
 *  inputstride  [I] Number of bytes in one scanline.
 *  destbits     [I] Destination bitmap.
 *  output       [I] Format of the destination bitmap.
 *  outputstride [I] Number of bytes in one scanline. 
 *  callback     [I] Callback function.
 *  data         [I] Callback data. 
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateBitmapBits( HTRANSFORM handle, PVOID srcbits, BMFORMAT input,
    DWORD width, DWORD height, DWORD inputstride, PVOID destbits, BMFORMAT output,
    DWORD outputstride, PBMCALLBACKFN callback, ULONG data )
{
    BOOL ret = FALSE;
#ifdef HAVE_LCMS2
    struct transform *transform = grab_transform( handle );

    TRACE( "( %p, %p, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p, 0x%08x, 0x%08x, %p, 0x%08x )\n",
           handle, srcbits, input, width, height, inputstride, destbits, output,
           outputstride, callback, data );

    if (!transform) return FALSE;
    if (!cmsChangeBuffersFormat( transform->cmstransform, from_bmformat(input), from_bmformat(output) ))
        return FALSE;

    cmsDoTransform( transform->cmstransform, srcbits, destbits, width * height );
    release_transform( transform );
    ret = TRUE;

#endif /* HAVE_LCMS2 */
    return ret;
}

/******************************************************************************
 * TranslateColors              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  input        [I] Array of input colors.
 *  number       [I] Number of colors to translate.
 *  input_type   [I] Input color format.
 *  output       [O] Array of output colors.
 *  output_type  [I] Output color format.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateColors( HTRANSFORM handle, PCOLOR in, DWORD count,
                             COLORTYPE input_type, PCOLOR out, COLORTYPE output_type )
{
#ifdef HAVE_LCMS2
    BOOL ret = TRUE;
    struct transform *transform = grab_transform( handle );
    cmsHTRANSFORM xfrm;
    unsigned int i;

    TRACE( "( %p, %p, %d, %d, %p, %d )\n", handle, in, count, input_type, out, output_type );

    if (!transform) return FALSE;

    xfrm = transform->cmstransform;
    if (!cmsChangeBuffersFormat( xfrm, from_type(input_type), from_type(output_type) ))
        return FALSE;

    switch (input_type)
    {
    case COLOR_RGB:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].rgb, 1 ); goto done;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].Lab, 1 ); goto done;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].gray, 1 ); goto done;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].cmyk, 1 ); goto done;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].XYZ, 1 ); goto done;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            ret = FALSE;
            break;
        }
        break;
    }
    case COLOR_Lab:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].rgb, 1 ); goto done;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].Lab, 1 ); goto done;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].gray, 1 ); goto done;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].cmyk, 1 ); goto done;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].XYZ, 1 ); goto done;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            ret = FALSE;
            break;
        }
        break;
    }
    case COLOR_GRAY:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].rgb, 1 ); goto done;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].Lab, 1 ); goto done;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].gray, 1 ); goto done;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].cmyk, 1 ); goto done;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].XYZ, 1 ); goto done;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            ret = FALSE;
            break;
        }
        break;
    }
    case COLOR_CMYK:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].rgb, 1 ); goto done;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].Lab, 1 ); goto done;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].gray, 1 ); goto done;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].cmyk, 1 ); goto done;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].XYZ, 1 ); goto done;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            ret = FALSE;
            break;
        }
        break;
    }
    case COLOR_XYZ:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].rgb, 1 ); goto done;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].Lab, 1 ); goto done;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].gray, 1 ); goto done;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].cmyk, 1 ); goto done;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].XYZ, 1 ); goto done;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            ret = FALSE;
            break;
        }
        break;
    }
    default:
        FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
        ret = FALSE;
        break;
    }

done:
    release_transform( transform );
    return ret;

#else  /* HAVE_LCMS2 */
    return FALSE;
#endif /* HAVE_LCMS2 */
}
