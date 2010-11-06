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

#include "config.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

#ifdef HAVE_LCMS

static DWORD from_profile( HPROFILE profile )
{
    PROFILEHEADER header;

    GetColorProfileHeader( profile, &header );
    TRACE( "color space: 0x%08x %s\n", header.phDataColorSpace, MSCMS_dbgstr_tag( header.phDataColorSpace ) );

    switch (header.phDataColorSpace)
    {
    case 0x434d594b: return TYPE_CMYK_16;  /* 'CMYK' */
    case 0x47524159: return TYPE_GRAY_16;  /* 'GRAY' */
    case 0x4c616220: return TYPE_Lab_16;   /* 'Lab ' */
    case 0x52474220: return TYPE_RGB_16;   /* 'RGB ' */
    case 0x58595a20: return TYPE_XYZ_16;   /* 'XYZ ' */
    default:
        WARN("unhandled format\n");
        return TYPE_RGB_16;
    }
}

static DWORD from_bmformat( BMFORMAT format )
{
    static int quietfixme = 0;
    TRACE( "bitmap format: 0x%08x\n", format );

    switch (format)
    {
    case BM_RGBTRIPLETS: return TYPE_RGB_8;
    case BM_BGRTRIPLETS: return TYPE_BGR_8;
    case BM_GRAY:        return TYPE_GRAY_8;
    default:
        if (quietfixme == 0)
        {
            FIXME("unhandled bitmap format 0x%x\n", format);
            quietfixme = 1;
        }
        return TYPE_RGB_8;
    }
}

static DWORD from_type( COLORTYPE type )
{
    TRACE( "color type: 0x%08x\n", type );

    switch (type)
    {
    case COLOR_GRAY:    return TYPE_GRAY_16;
    case COLOR_RGB:     return TYPE_RGB_16;
    case COLOR_XYZ:     return TYPE_XYZ_16;
    case COLOR_Yxy:     return TYPE_Yxy_16;
    case COLOR_Lab:     return TYPE_Lab_16;
    case COLOR_CMYK:    return TYPE_CMYK_16;
    default:
        FIXME("unhandled color type\n");
        return TYPE_RGB_16;
    }
}

#endif /* HAVE_LCMS */

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
#ifdef HAVE_LCMS
    struct transform transform;
    struct profile *dst, *tgt = NULL;
    cmsHPROFILE cmsinput, cmsoutput, cmstarget = NULL;
    DWORD in_format, out_format, proofing = 0;
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
    TRACE( "lcsCSType:   %s\n", MSCMS_dbgstr_tag( space->lcsCSType ) );
    TRACE( "lcsFilename: %s\n", debugstr_w( space->lcsFilename ) );

    in_format  = TYPE_RGB_16;
    out_format = from_profile( dest );

    cmsinput = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */
    if (target)
    {
        proofing = cmsFLAGS_SOFTPROOFING;
        cmstarget = tgt->cmsprofile;
    }
    cmsoutput = dst->cmsprofile;
    transform.cmstransform = cmsCreateProofingTransform(cmsinput, in_format, cmsoutput, out_format, cmstarget,
                                                        intent, INTENT_ABSOLUTE_COLORIMETRIC, proofing);

    ret = create_transform( &transform );

    if (tgt) release_profile( tgt );
    release_profile( dst );

#endif /* HAVE_LCMS */
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
#ifdef HAVE_LCMS
    cmsHPROFILE *cmsprofiles, cmsconvert = NULL;
    struct transform transform;
    struct profile *profile0, *profile1;
    DWORD in_format, out_format;

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
    in_format  = from_profile( profiles[0] );
    out_format = from_profile( profiles[nprofiles - 1] );

    if (in_format != out_format)
    {
        /* insert a conversion profile for pairings that lcms doesn't handle */
        if (out_format == TYPE_RGB_16) cmsconvert = cmsCreate_sRGBProfile();
        if (out_format == TYPE_Lab_16) cmsconvert = cmsCreateLabProfile( NULL );
    }

    cmsprofiles = HeapAlloc( GetProcessHeap(), 0, (nprofiles + 1) * sizeof(cmsHPROFILE *) );
    if (cmsprofiles)
    {
        cmsprofiles[0] = profile0->cmsprofile;
        if (cmsconvert)
        {
            cmsprofiles[1] = cmsconvert;
            cmsprofiles[2] = profile1->cmsprofile;
            nprofiles++;
        }
        else
        {
            cmsprofiles[1] = profile1->cmsprofile;
        }
        transform.cmstransform = cmsCreateMultiprofileTransform( cmsprofiles, nprofiles, in_format, out_format, *intents, 0 );

        HeapFree( GetProcessHeap(), 0, cmsprofiles );
        ret = create_transform( &transform );
    }

    release_profile( profile0 );
    release_profile( profile1 );

#endif /* HAVE_LCMS */
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
#ifdef HAVE_LCMS

    TRACE( "( %p )\n", handle );

    ret = close_transform( handle );

#endif /* HAVE_LCMS */
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
#ifdef HAVE_LCMS
    struct transform *transform = grab_transform( handle );

    TRACE( "( %p, %p, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p, 0x%08x, 0x%08x, %p, 0x%08x )\n",
           handle, srcbits, input, width, height, inputstride, destbits, output,
           outputstride, callback, data );

    if (!transform) return FALSE;
    cmsChangeBuffersFormat( transform->cmstransform, from_bmformat(input), from_bmformat(output) );

    cmsDoTransform( transform->cmstransform, srcbits, destbits, width * height );
    release_transform( transform );
    ret = TRUE;

#endif /* HAVE_LCMS */
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
    BOOL ret = FALSE;
#ifdef HAVE_LCMS
    struct transform *transform = grab_transform( handle );
    cmsHTRANSFORM xfrm;
    unsigned int i;

    TRACE( "( %p, %p, %d, %d, %p, %d )\n", handle, in, count, input_type, out, output_type );

    if (!transform) return FALSE;

    xfrm = transform->cmstransform;
    cmsChangeBuffersFormat( xfrm, from_type(input_type), from_type(output_type) );

    switch (input_type)
    {
    case COLOR_RGB:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].rgb, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_Lab:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].Lab, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_GRAY:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].gray, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_CMYK:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].cmyk, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    case COLOR_XYZ:
    {
        switch (output_type)
        {
        case COLOR_RGB:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].rgb, 1 ); return TRUE;
        case COLOR_Lab:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].Lab, 1 ); return TRUE;
        case COLOR_GRAY: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].gray, 1 ); return TRUE;
        case COLOR_CMYK: for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].cmyk, 1 ); return TRUE;
        case COLOR_XYZ:  for (i = 0; i < count; i++) cmsDoTransform( xfrm, &in[i].XYZ, &out[i].XYZ, 1 ); return TRUE;
        default:
            FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
            return FALSE;
        }
    }
    default:
        FIXME("unhandled input/output pair: %d/%d\n", input_type, output_type);
        break;
    }
    release_transform( transform );

#endif /* HAVE_LCMS */
    return ret;
}
