// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains generic d3d utility routines.
//

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  PopulateSurfaceDesc
//
//  Synopsis:  Creates a D3DSURFACE_DESC from the specified parameters
//
// Return Value:
//
//  None
//
//-------------------------------------------------------------------------
void 
PopulateSurfaceDesc(
    D3DFORMAT fmtPixelFormat,
    D3DPOOL d3dPool,
    DWORD dwUsage, 
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) D3DSURFACE_DESC *pDesc 
    )
{
    //
    // Add more usages here as we need them.
    //
    Assert(   dwUsage == D3DUSAGE_RENDERTARGET
           || dwUsage == 0
           );
    Assert(uTextureWidth > 0 && uTextureHeight > 0);

    pDesc->Format = fmtPixelFormat;
    pDesc->Type = D3DRTYPE_TEXTURE;
    pDesc->Usage = dwUsage;
    pDesc->Pool = d3dPool;
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    pDesc->MultiSampleQuality = 0;
    pDesc->Width = uTextureWidth;
    pDesc->Height = uTextureHeight;
}

//+------------------------------------------------------------------------
//
//  Function:  GetMinimalTextureDesc
//
//  Synopsis:  Returns a surface description for a texture the
//              given D3D device should be capable of creating that
//              can retain all data a texture with given
//              description could hold.
//
// Return Value:
//
//   S_OK - suitable texture description was found,
//   S_FALSE - smaller texture description was found,
//            otherwise, varying D3DERR_Xxx values.
//

DeclareTagEx(tagTextureFixup, "MIL_HW", "Texture Create Description Fixup", FALSE /* fEnabled */);

HRESULT
GetMinimalTextureDesc(
    __in_ecount(1) IDirect3DDevice9 *pD3DDevice,    // The D3D device to check
                                                    // for the surface
                                                    // description

    D3DFORMAT AdapterFormat,                        // Format the adapter is
                                                    // displaying

    __in_ecount(1) D3DCAPS9 const *pcaps,           // The caps for the
                                                    // D3DDevice

    __inout_ecount(1) D3DSURFACE_DESC *pd3dsd,      // D3D surface description
                                                    // that holds the minimal
                                                    // dimension at start and
                                                    // at the end what should
                                                    // be given passed to
                                                    // CreateTexture.

    BOOL fPalUsesAlpha,                             // set to TRUE when an
                                                    // associated palette is
                                                    // not completely opaque

    DWORD dwFlags                                   // Flags specifying which
                                                    // fields need checked.
    )
{
    HRESULT hr = S_OK;

#if DBG
    D3DSURFACE_DESC d3dsdOrg;

    d3dsdOrg.Format = pd3dsd->Format;
    d3dsdOrg.Width  = pd3dsd->Width;
    d3dsdOrg.Height = pd3dsd->Height;
#endif DBG

    // Do either width or height need checked?
    //  => Are relative flags not equal to ignoring both width and height?
    if ((dwFlags & (GMTD_IGNORE_WIDTH | GMTD_IGNORE_HEIGHT))
        != (GMTD_IGNORE_WIDTH | GMTD_IGNORE_HEIGHT))
    {
        // Adjust dimensions per texture capabilites
        if ((dwFlags & GMTD_IGNORE_WIDTH) == 0)
        {
            // Adjust width as needed
            if (pd3dsd->Width > pcaps->MaxTextureWidth)
            {
                pd3dsd->Width = pcaps->MaxTextureWidth;

                Assert(!(pcaps->TextureCaps & D3DPTEXTURECAPS_POW2) ||
                       (pcaps->MaxTextureWidth ==
                        RoundToPow2(pcaps->MaxTextureWidth)));

                hr = S_FALSE;
            }
            else if (pcaps->TextureCaps & D3DPTEXTURECAPS_POW2)
            {
                if (dwFlags & GMTD_NONPOW2CONDITIONAL_OK)
                {
                    Assert(pcaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
                    // Width need not be adjusted
                }
                else
                {
                    pd3dsd->Width = RoundToPow2(pd3dsd->Width);

                    Assert(pd3dsd->Width <= pcaps->MaxTextureWidth);
                }
            }
        }

        if ((dwFlags & GMTD_IGNORE_HEIGHT) == 0)
        {
            // Adjust height as needed
            if (pd3dsd->Height > pcaps->MaxTextureHeight)
            {
                pd3dsd->Height = pcaps->MaxTextureHeight;

                Assert(!(pcaps->TextureCaps & D3DPTEXTURECAPS_POW2) ||
                       (pcaps->MaxTextureHeight ==
                        RoundToPow2(pcaps->MaxTextureHeight)));

                hr = S_FALSE;
            }
            else if (pcaps->TextureCaps & D3DPTEXTURECAPS_POW2)
            {
                if (dwFlags & GMTD_NONPOW2CONDITIONAL_OK)
                {
                    Assert(pcaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
                    // Height need not be adjusted
                }
                else
                {
                    pd3dsd->Height = RoundToPow2(pd3dsd->Height);

                    Assert(pd3dsd->Height <= pcaps->MaxTextureHeight);
                }
            }
        }
    }

    if ((dwFlags & GMTD_IGNORE_FORMAT) == 0)
    {
        IDirect3D9 *    pd3d9;
        D3DFORMAT       d3dFormat;

        // Save HR from width/height check
        HRESULT hrWH = hr;

        if (SUCCEEDED((hr = THR(pD3DDevice->GetDirect3D(&pd3d9)))))
        {
            d3dFormat = pd3dsd->Format;

            // If the format is palettized and the palette has alpha, but
            //  the device can't draw alpha from a palette then, the format
            //  needs bumped at least once.
            if ((d3dFormat == D3DFMT_P8) &&
                fPalUsesAlpha &&
                !(pcaps->TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE))
            {
                d3dFormat = GetSuperiorSurfaceFormat(d3dFormat, fPalUsesAlpha);
            }

            do
            {
                hr = pd3d9->CheckDeviceFormat(
                    pcaps->AdapterOrdinal,
                    pcaps->DeviceType,
                    AdapterFormat,
                    pd3dsd->Usage,
                    pd3dsd->Type,
                    d3dFormat
                    );

                if (SUCCEEDED(hr))
                {
                    pd3dsd->Format = d3dFormat;
                    break;
                }

                d3dFormat = GetSuperiorSurfaceFormat(d3dFormat, fPalUsesAlpha);

            } while ( d3dFormat != D3DFMT_UNKNOWN );

            pd3d9->Release();

            //
            // There are some formats to which the condition non-power of 2
            // support does not apply.  We shouldn't be using them anyway; so
            // just assert that is so.
            //
            Assert(   ((dwFlags & GMTD_NONPOW2CONDITIONAL_OK) == 0)
                   || (   (d3dFormat != D3DFMT_DXT1)
                       && (d3dFormat != D3DFMT_DXT2)
                       && (d3dFormat != D3DFMT_DXT3)
                       && (d3dFormat != D3DFMT_DXT4)
                       && (d3dFormat != D3DFMT_DXT5)
                  )   );
        }

        // If format is ok, restore HR from width/height check
        if (hr == S_OK)
        {
            hr = hrWH;
        }
    }

#if DBG
    if (IsTagEnabled(tagTextureFixup) &&
        SUCCEEDED(hr) &&
        (d3dsdOrg.Format != pd3dsd->Format ||
         d3dsdOrg.Width  != pd3dsd->Width ||
         d3dsdOrg.Height != pd3dsd->Height
       ))
    {
        TraceTag((tagTextureFixup,
                  "CD3DDeviceLevel1::GetMinimalTextureDesc modified description:\n"
                  "  In:  Fmt: %lu  %lu x %lu\n"
                  " Out:  Fmt: %lu  %lu x %lu",
                  d3dsdOrg.Format, d3dsdOrg.Width, d3dsdOrg.Height,
                  pd3dsd->Format, pd3dsd->Width, pd3dsd->Height
                  ));
    }
#endif DBG

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:  GetSuperiorSurfaceFormat
//
//  Synopsis:  Returns a surface format that is superior to the given
//         format in that it can hold more color information.
//
// Return Value:
//
//   D3DFMT_UNKNOWN - no superior format was found.
//
//-------------------------------------------------------------------------
D3DFORMAT
GetSuperiorSurfaceFormat(
    D3DFORMAT d3dFormat,    // In: base D3D surface format
    BOOL fPalUsesAlpha      // In: set to TRUE when an associated
                            //      palette is not completely opaque
    )
{
    switch (d3dFormat)
    {
    case D3DFMT_P8:
        if (fPalUsesAlpha)
        {
            return D3DFMT_A8R8G8B8;
        }
        else
        {
            return D3DFMT_R8G8B8;
        }

    case D3DFMT_X1R5G5B5:
        return D3DFMT_R5G6B5;

    case D3DFMT_R5G6B5:
        return D3DFMT_R8G8B8;

    case D3DFMT_R8G8B8:
        return D3DFMT_X8R8G8B8;

    case D3DFMT_X8R8G8B8:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A8P8:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A1R5G5B5:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A8R8G8B8:
    default:
        break;
    }

    return D3DFMT_UNKNOWN;
}





