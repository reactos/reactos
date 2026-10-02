// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MILRender

    Created:

        10/09/2001 mbyrd

\*=========================================================================*/

/*=========================================================================*\
    Include MIL Types:
\*=========================================================================*/

#include <spec_string.h>
#include "wincodec.h"
#include "wgx_render_types.h"
#include "wgx_effect_types.h"
#include "factory.hpp"


#ifndef __MILRENDER_H__
#define __MILRENDER_H__
#pragma once

#define MILAPI WINAPI

typedef WICRect MILRect;

/*=========================================================================*\
    Interface IID's :
\*=========================================================================*/

#if defined( _WIN32 ) && !defined( _NO_COM)

//
// *** IMPORTANT ***
//
// DO NOT simply take an existing GUID and add one to it any more. This is 
// especially important now because some older GUIDS have been removed from
// below and we don't want a collision. Deprecated GUIDs are left commented
// to help prevent this problem.
//
DEFINE_GUID(IID_IMILCoreFactory,            0x00000002,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILRenderTarget,           0x00000020,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

//DEFINE_GUID(IID_IMILBitmapDescriptor,       0x00000106,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILBitmapPyramid,          0x00000122,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILBitmapDecodeOption,     0x841057ad,0x1ad4,0x4ab7,0xb8,0x22,0x8b,0x67,0xf1,0x7e,0x7,0xb9);
DEFINE_GUID(IID_IMILDynamicResource,        0x8cb53eb7,0xd409,0x4066,0x94,0x87,0xc0,0xd4,0x15,0x2f,0xe8,0x0a);

DEFINE_GUID(IID_IMILMesh,                   0x00000131,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILMesh3D,                 0x00000132,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

DEFINE_GUID(IID_IMILMedia,                  0x00000141,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILStreamCallback,         0x00000142,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILWmpFactory,             0x00000143,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

DEFINE_GUID(IID_IMILRenderTargetBitmap,     0x00000201,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILRenderTargetHWND,       0x00000202,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

//DEFINE_GUID(IID_IMILIcmColorContext,        0x00000301,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILIcmColorTransform,      0x00000302,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IColorDirectory,            0x00000303,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILEffectList,             0x00000400,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

DEFINE_GUID(IID_IMILShader,                 0x00000500,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILShaderDiffuse,          0x00000501,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILShaderSpecular,         0x00000502,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILShaderGlass,            0x00000503,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
DEFINE_GUID(IID_IMILShaderEmissive,         0x00000504,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILShaderGlass2D,          0x00000505,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);
//DEFINE_GUID(IID_IMILShaderDepth,            0x00000507,0xa8f2,0x4877,0xba,0xa,0xfd,0x2b,0x66,0x45,0xfb,0x94);

DEFINE_GUID(IID_IMILEventProxy,             0x342efd8b,0x669a,0x4d16,0xb1,0x63,0xd7,0x5f,0x5f,0xfd,0x1a,0x10);

// 0x00000500 is used by Bitmap Transforms

// {0BF5397B-B415-4ea4-892F-D8CB20273B58}
DEFINE_GUID(IID_ICompositionService,        0xbf5397b, 0xb415, 0x4ea4, 0x89, 0x2f, 0xd8, 0xcb, 0x20, 0x27, 0x3b, 0x58);

// {51A1ED10-269C-4a32-ABAB-3EF791E6C951}
DEFINE_GUID(IID_IRemoteCompositionConnection, 0x51a1ed10, 0x269c, 0x4a32, 0xab, 0xab, 0x3e, 0xf7, 0x91, 0xe6, 0xc9, 0x51);
// {B573C267-017D-4722-847E-466F99FFF86C}
DEFINE_GUID(IID_IMILSerializable, 0xb573c267, 0x17d, 0x4722, 0x84, 0x7e, 0x46, 0x6f, 0x99, 0xff, 0xf8, 0x6c);

// {DD0BF622-0650-4a1e-B20F-4B4AB6EDFCA3}
DEFINE_GUID(IID_IWGXBitmapSource, 0xdd0bf622, 0x650, 0x4a1e, 0xb2, 0xf, 0x4b, 0x4a, 0xb6, 0xed, 0xfc, 0xa3);
// {D5EC87D4-5FDC-4b77-A924-8C0EDE170A2E}
DEFINE_GUID(IID_IWGXBitmapLock, 0xd5ec87d4, 0x5fdc, 0x4b77, 0xa9, 0x24, 0x8c, 0xe, 0xde, 0x17, 0xa, 0x2e);
// {C46D6FDE-0E59-4cfd-89B1-C935906DFBD9}
DEFINE_GUID(IID_IWGXBitmap, 0xc46d6fde, 0xe59, 0x4cfd, 0x89, 0xb1, 0xc9, 0x35, 0x90, 0x6d, 0xfb, 0xd9);

#endif

/*=========================================================================*\
    Interface forward declarations :
\*=========================================================================*/
/*
    We need these so that we don't have to have say "interface IMILXXX *"
    everywhere in the prototypes. These forward defines are needed only when
    an interface needs a parameter for another interface which has not yet been
    defined in this file.

    This cross-referencing doesn't happen often, but we define them all anyway.
*/

#ifdef __cplusplus

interface IMILCoreFactory;

interface IMILRenderTarget;
interface IMILRenderTargetHWND;

interface IMILMesh;
interface IMILMesh3D;

interface IMILEffectList;

interface IMILShader;
interface IMILShaderDiffuse;
interface IMILShaderEmissive;
interface IMILShaderSpecular;

interface IMILMedia;
interface IMILWmpFactory;
interface IWMPPlayer; // forward decl needed for the definition of IMILWmpFactory

interface IMILSwDoubleBufferedBitmap;

interface ICompositionService;
interface IRemoteCompositionConnection;

interface IMILEventProxy;

interface IWGXBitmapSource;
interface IWGXBitmap;

#endif /* __cplusplus */

typedef interface IMILCoreFactory               IMILCoreFactory;
typedef interface IMILRenderTarget              IMILRenderTarget;
typedef interface IMILRenderTargetHWND          IMILRenderTargetHWND;
typedef interface IMILRenderTargetBitmap        IMILRenderTargetBitmap;
typedef interface IMILEffectList                IMILEffectList;
typedef interface IMILShader                    IMILShader;
typedef interface IMILShaderDiffuse             IMILShaderDiffuse;
typedef interface IMILShaderEmissive            IMILShaderEmissive;
typedef interface IMILShaderSpecular            IMILShaderSpecular;

typedef interface IMILMesh                      IMILMesh;
typedef interface IMILMesh3D                    IMILMesh3D;

typedef interface IMILMedia                     IMILMedia;
typedef interface IMILWmpFactory                IMILWmpFactory;

typedef interface ICompositionService           ICompositionService;
typedef interface IRemoteCompositionConnection  IRemoteCompositionConnection;

typedef interface IMILEventProxy                IMILEventProxy;

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================================*\

    DLL Function for creating a MILFactory object. This object allows
    the creation of MIL objects. Pass the value of the constant
    MIL_SDK_VERSION to this function, so that the run-time can
    validate that your application was compiled against the right headers.

    MILCreateFactory2 is a newer version of MILCreateFactory that allows specifying
    callbacks for text rendering.
    MILCreateFactory does not allow applications to perform text rendering.

\*=========================================================================*/

HRESULT WINAPI MILCreateFactory(__deref_out_ecount(1) IMILCoreFactory **ppIFactory, UINT SDKVersion);

HRESULT WINAPI MILCreateEffectList(__deref_out IMILEffectList **ppIMILEffectList);



/*=========================================================================*\
    IMILCoreFactory - Top-level MIL Factory object
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILCoreFactory

DECLARE_INTERFACE_(IMILCoreFactory, IUnknown)
{
    // Make sure factory is working with current display state information
    STDMETHOD(UpdateDisplayState)(
        THIS_
        __out_ecount(1) bool *pDisplayStateChanged,
        __out_ecount(1) int *pDisplayCount
        ) PURE;

    // Query graphics accleration capabilities
    STDMETHOD_(void, QueryCurrentGraphicsAccelerationCaps)(
        THIS_
        __in bool fReturnCommonMinimum,
        __out_ecount(1) ULONG *pulDisplayUniqueness,
        __out_ecount(1) MilGraphicsAccelerationCaps *pCaps
        );

    // Bitmap Render Target

    STDMETHOD(CreateBitmapRenderTarget)(
        THIS_
        UINT width,
        UINT height,
        MilPixelFormat::Enum format,
        FLOAT dpiX,
        FLOAT dpiY,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
        ) PURE;

    // Render Target for a client supplied Bitmap

    STDMETHOD(CreateSWRenderTargetForBitmap)(
        THIS_
        __inout_ecount(1) IWICBitmap *pIBitmap,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
        ) PURE;


    /* Audio/Video */
    STDMETHOD(CreateMediaPlayer)(
        THIS_
        __inout_ecount(1) IUnknown *pEventProxy,
        bool canOpenAnyMedia,
        __deref_out_ecount(1) IMILMedia **ppMedia
        ) PURE;

    // Print Render Target

};

/*=========================================================================*\

  IMILRenderTarget - Render Target for MIL

  Interface Description:

    This interface defines the base render target (RT) interface. All
    Render Targets are able to Clear their entire landscape.

\*=========================================================================*/

class CAliasedClip;

#undef INTERFACE
#define INTERFACE IMILRenderTarget

DECLARE_INTERFACE_(IMILRenderTarget, IUnknown)
{
    STDMETHOD_(VOID, GetBounds)(
        THIS_
        __out_ecount(1) MilRectF * const pBounds
        ) PURE;

    /* Clear the render target*/

    STDMETHOD(Clear)(
        THIS_
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip = NULL
        ) PURE;

    STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        ) PURE;

    STDMETHOD(End3D)(
        ) PURE;

};

/*=========================================================================*\
    IMILRenderTargetBitmap - MIL render target that renders to a bitmap
\*=========================================================================*/

class IRenderTargetInternal;

#undef INTERFACE
#define INTERFACE IMILRenderTargetBitmap

DECLARE_INTERFACE_(IMILRenderTargetBitmap, IMILRenderTarget)
{
    STDMETHOD(GetBitmapSource)(
        THIS_
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        ) PURE;

    STDMETHOD(GetCacheableBitmapSource)(
        THIS_
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        ) PURE;

    STDMETHOD(GetBitmap)(
        THIS_
        __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
        ) PURE;

    STDMETHOD(GetNumQueuedPresents)(
        THIS_
        UINT *puNumQueuedPresents
        );
};

/*=========================================================================*\

  IMILRenderTargetHWND

  Interface Description:

    A render target which can output to an HWND. This render target may or
    may not be hardware accelerated. This depends on being able to create
    an IDirect3DDevice9 or other similar hardware accelerated device. If
    the specific h/w acceleration device is not available, we will build
    a double-buffered system memory RT.

    All HWND render targets implement Present method.

\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILRenderTargetHWND

DECLARE_INTERFACE_(IMILRenderTargetHWND, IMILRenderTarget)
{
    STDMETHOD(SetPosition)(
        THIS_
        __in_ecount(1) MilRectF const *prc
        ) PURE;

    STDMETHOD(GetInvalidRegions)(
        THIS_
        __deref_outro_ecount(*pNumRegions) MilRectF const ** const prgRegions,
        __out_ecount(1) UINT *pNumRegions,
        __out bool *fWholeTargetInvalid
        ) PURE;

    STDMETHOD(UpdatePresentProperties)(
        THIS_
        MilTransparency::Flags transparencyFlags,
        FLOAT constantAlpha,
        __in_ecount(1) MilColorF const &colorKey
        ) PURE;

    STDMETHOD(Present)(
        THIS_
        ) PURE;

    STDMETHOD(ScrollBlt) (
        THIS_
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        ) PURE;    

    STDMETHOD(Invalidate)(
        THIS_
        __in_ecount_opt(1) MilRectF const *prc
        ) PURE;

    STDMETHOD_(VOID, GetIntersectionWithDisplay)(
        THIS_
        __in UINT iDisplay,
        __out_ecount(1) MilRectL &rcIntersection
        ) PURE;

    STDMETHOD(WaitForVBlank)(
        THIS_
        ) PURE;

    STDMETHOD_(VOID, AdvanceFrame)(
        THIS_
        UINT uFrameNumber
        ) PURE;

    STDMETHOD(GetNumQueuedPresents)(
        THIS_
        __out_ecount(1) UINT *puNumQueuedPresents
        ) PURE;

    STDMETHOD(CanAccelerateScroll)(
        THIS_
        __out_ecount(1) bool *fCanAccelerateScroll
        ) PURE;
};

/*=========================================================================*\
    IMILMesh3D - MIL Mesh 3D Primitive
\*=========================================================================*/

class CMILLightData;
class CMILMatrix;

#undef INTERFACE
#define INTERFACE IMILMesh3D

DECLARE_INTERFACE_(IMILMesh3D, IUnknown)
{
    STDMETHOD(GetBounds)(
        THIS_
        __out_ecount(1) MilPointAndSize3F *pboxBounds
        ) PURE;

    STDMETHOD(CopyPositionsFrom)(
        THIS_
        __in_bcount(cbSize) const dxlayer::vector3 *pVertexPositions,
        size_t cbSize
        );

    STDMETHOD(CopyNormalsFrom)(
        THIS_
        __in_bcount_opt(cbSize) const dxlayer::vector3 *pVertexNormals,
        size_t cbSize
        );

    STDMETHOD(CopyTextureCoordinatesFrom)(
        THIS_
        __in_bcount_opt(cbSize) const dxlayer::vector2 *pVertexTextureCoordinates,
        size_t cbSize
        );

    STDMETHOD(CopyIndicesFrom)(
        THIS_
        __in_bcount(cbSize) const UINT *rgIndices,
        size_t cbSize
        );

    STDMETHOD(CloneMesh)(
        THIS_
        __deref_out_ecount(1) IMILMesh3D **ppIMesh3D
        ) PURE;

    STDMETHOD_(UINT, GetNumVertices)(THIS_) const PURE;

    STDMETHOD_(VOID, NotifyPositionChange)(
        THIS_
        BOOL fCalculateNormals
        ) PURE;

    STDMETHOD_(VOID, NotifyIndicesChange)(
        THIS_
        BOOL fCalculateNormals
        ) PURE;

    STDMETHOD_(VOID, GetNormals)(
        __deref_outro_bcount(cbSize) const dxlayer::vector3* &buffer,
        __out_ecount(1) size_t &cbSize
        ) const PURE;

    STDMETHOD_(VOID, GetPositions)(
        __deref_outro_bcount(cbSize) const dxlayer::vector3* &buffer,
        __out_ecount(1) size_t &cbSize
        ) const PURE;

    STDMETHOD_(VOID, GetTextureCoordinates)(
        __deref_outro_bcount(cbSize) const dxlayer::vector2* &buffer,
        __out_ecount(1) size_t &cbSize
        ) const PURE;

    STDMETHOD_(VOID, GetIndices)(
        __deref_outro_bcount(cbSize) const UINT* &buffer,
        __out_ecount(1) size_t &cbSize
        ) const PURE;

    STDMETHOD(SetPosition)(
        UINT index,
        __in_ecount(1) const dxlayer::vector3 &position
        ) PURE;

};

/*=========================================================================*\
    IMILShaders
\*=========================================================================*/


#undef INTERFACE
#define INTERFACE IMILShader

class CMILShader;

DECLARE_INTERFACE_(IMILShader, IUnknown)
{
    STDMETHOD_(CMILShader *, GetClass)(
        THIS
        ) PURE;
};

/*=========================================================================*\
    IMILShaderDiffuse - MIL Shader
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILShaderDiffuse

DECLARE_INTERFACE_(IMILShaderDiffuse, IMILShader)
{

};

/*=========================================================================*\
    IMILShaderSpecular - MIL Shader
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILShaderSpecular

DECLARE_INTERFACE_(IMILShaderSpecular, IMILShader)
{

};

/*=========================================================================*\
    IMILShaderEmissive - MIL Shader
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILShaderEmissive

DECLARE_INTERFACE_(IMILShaderEmissive, IMILShader)
{

};


/*=========================================================================*\
    IMILEffectList - MIL Effect

    An Effect is a simple encapsulation around a CLSID and a parameter block.
    The CLSID is a selector which chooses the appropriate
    IMILBitmapTransform, and the parameter block is the struct or block
    of data which must be passed to the SetParams interface on the transform.
    The size parameter represents the size of the parameter block in bytes.
    IMILEffectList contains a list of these parameter blocks.

\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILEffectList

DECLARE_INTERFACE_(IMILEffectList, IUnknown)
{
    /**
     * Initialize the Effect
     *
     * @param clsid  CLSID selecting the effect
     * @param size   size of the initialization parameter block, in bytes.
     * @param pData  Initialization parameter block.
     * @return HRESULT
     */
    STDMETHOD(Add)(
        THIS_
        __in_ecount(1) REFCLSID clsid,
        UINT size,
        __in_bcount_opt(size) const EffectParams *pData
        ) PURE;

    /**
     * Initialize the Effect
     *
     * @param clsid  CLSID selecting the effect
     * @param size   size of the initialization parameter block, in bytes.
     * @param pData  Initialization parameter block.
     * @param cResources resource count
     * @param pprgIUnknown array of resources
     * @return HRESULT
     */
    STDMETHOD(AddWithResources)(
        THIS_
        __in_ecount(1) REFCLSID clsid,
        UINT size,
        __in_bcount_opt(size) const EffectParams *pData,
        UINT cResources,
        __in_pcount_opt_inout(cResources) IUnknown * const *rgpIUnknown
        ) PURE;

    /**
     * Clears the effect list
     *
     * @return void
     */
    STDMETHOD_(void, Clear)(THIS) PURE;

    /**
     * Return the count of the stored Effect parameter blocks.
     *
     * @param pCount Number of entries in the array.
     * @return HRESULT
     */
    STDMETHOD(GetCount)(
        THIS_
        __out_ecount(1) UINT *pCount
        ) const PURE;

    /**
     * Get the stored CLSID
     *
     * @param idx    Index into the array.
     * @param pClsid Pointer to CLSID to be initialized.
     * @return HRESULT
     */
    STDMETHOD(GetCLSID)(
        THIS_
        UINT idxEffect,
        __out_ecount(1) CLSID *pClsid
        ) const PURE;

    /**
     * Get the size of the stored parameter block.
     *
     * @param idx    Index into the array.
     * @param pSize  Pointer to the UINT to receive the size.
     * @return HRESULT
     */
    STDMETHOD(GetParameterSize)(
        THIS_
        UINT idxEffect,
        __out_ecount(1) UINT *pSize
        ) const PURE;

    /**
     * Get the stored parameter block.
     *
     * @param idx    Index into the array.
     * @param size   Size of the buffer pointed to by pData
     * @param pData  Pointer to a buffer to receive the parameter block.
     * @return HRESULT.
     *         Will return failure if size is too small or there is no pointer
     *         to receive the data.
     */
    STDMETHOD(GetParameters)(
        THIS_
        UINT idxEffect,
        UINT size,
        __out_bcount_full(size) EffectParams *pData
        ) const PURE;


    /**
     * Gets resource from the effect list
     *
     * @param idxEffect  index of effect containing the resources
     * @param pcResources receives number of resources this effect uses
     * @return HRESULT
     */
    STDMETHOD(GetResourceCount)(
        THIS_
        UINT idxEffect,
        __out_ecount(1) UINT *pcResources
        ) const PURE;

    /**
     * Gets resource from the effect list
     *
     * @param idxEffect  index of effect containing the resources
     * @param pprgIUnknown receives resource
     * @return HRESULT
     */
    STDMETHOD(GetResources)(
        THIS_
        UINT idxEffect,
        UINT cResources,
        __out_pcount_full_out(cResources) IUnknown **rgpResources
        ) const PURE;

    STDMETHOD_(void, GetParamRef)(
        THIS_
        UINT idx,
        __deref_out const void **ppvData
        ) const PURE;

    // Get a copy of the resources array without calling AddRef on the resources
    STDMETHOD_(void, GetResourcesNoAddRef)(
        THIS_
        UINT idxEffect,
        UINT cResources,
        __out_pcount_full_out(cResources) IUnknown **rgpResources
        ) const PURE;

    // Get total number of resources in the effect list
    STDMETHOD(GetTotalResourceCount)(
        THIS_
        __out_ecount(1) UINT *pcResources
        ) const PURE;

    // Get a specific resource from the effect list
    STDMETHOD(GetResource)(
        THIS_
        UINT idxResource,
        __deref_out_ecount(1) IUnknown **ppIUnknown
        ) const PURE;

    // Replace a specific resource in the effect list
    STDMETHOD(ReplaceResource)(
        THIS_
        UINT idxResource,
        __inout_ecount(1) IUnknown *pIUnknown
        ) PURE;

};

/*=========================================================================*\
    IMILMedia
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILMedia

DECLARE_INTERFACE_(IMILMedia, IUnknown)
{
    STDMETHOD(Open)(
        THIS_
        __in LPCWSTR pwszURL
        ) PURE;

    STDMETHOD(Stop)(
        THIS_
        ) PURE;

    STDMETHOD(Close)(
        THIS_
        ) PURE;

    STDMETHOD(GetPosition)(
        THIS_
        __out LONGLONG *pllTime
        ) PURE;

    STDMETHOD(SetPosition)(
        THIS_
        __in LONGLONG llTime
        ) PURE;

    STDMETHOD(SetRate)(
        THIS_
        __in double dblRate
        ) PURE;

    STDMETHOD(SetVolume)(
        THIS_
        __in double dblVolume
        ) PURE;

    STDMETHOD(SetBalance)(
        THIS_
        __in double dblBalance
        ) PURE;

    STDMETHOD(SetIsScrubbingEnabled)(
        THIS_
        __in bool isScrubbingEnabled
        ) PURE;

    STDMETHOD(IsBuffering)(
        THIS_
        __out_ecount(1) bool *pIsBuffering
        ) PURE;

    STDMETHOD(CanPause)(
        THIS_
        __out_ecount(1) bool *pCanPause
        ) PURE;

    STDMETHOD(GetDownloadProgress)(
        THIS_
        __out_ecount(1) double *pProgress
        ) PURE;

    STDMETHOD(GetBufferingProgress)(
        THIS_
        __out_ecount(1) double *pProgress
        ) PURE;

    STDMETHOD(HasVideo)(
        THIS_
        __out_ecount(1) bool *pfHasVideo
        ) PURE;

    STDMETHOD(HasAudio)(
        THIS_
        __out_ecount(1) bool *pfHasAudio
        ) PURE;

    STDMETHOD(GetNaturalHeight)(
        THIS_
        __out_ecount(1) UINT *puiHeight
        ) PURE;

    STDMETHOD(GetNaturalWidth)(
        THIS_
        __out_ecount(1) UINT *puiWidth
        ) PURE;

    STDMETHOD(GetMediaLength)(
        THIS_
        __out_ecount(1) LONGLONG *pllLength
        ) PURE;

    STDMETHOD(NeedUIFrameUpdate)(
        THIS_
        );

    STDMETHOD(Shutdown)(
        THIS_
        ) PURE;

    STDMETHOD(ProcessExitHandler)(
        THIS_
        ) PURE;
};

/*=========================================================================*\
    IMILWmpFactory
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILWmpFactory

DECLARE_INTERFACE_(IMILWmpFactory, IUnknown)
{
    STDMETHOD(CreateWmpOcx)(
        __deref_out_ecount(1) IWMPPlayer **ppIWMPPlayer
        );
};

/*=========================================================================*\
    IMILEventProxy
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILEventProxy

DECLARE_INTERFACE_(IMILEventProxy, IUnknown)
{
    STDMETHOD(RaiseEvent)(
        __in_bcount(cb) BYTE *pb,
        __in ULONG cb
        );
};

/*=========================================================================*\
    IMILDynamicResource
\*=========================================================================*/
#undef INTERFACE
#define INTERFACE IMILDynamicResource

DECLARE_INTERFACE_(IMILDynamicResource, IUnknown)
{
    STDMETHOD(IsDynamicResource)(
        __out_ecount(1) bool *pfIsDynamic
        ) PURE;
};

/*=========================================================================*\
    IWGXBitmapSource -  Bitmap that can provide pixels but not direct access
\*=========================================================================*/
#undef INTERFACE
#define INTERFACE IWGXBitmapSource

DECLARE_INTERFACE_(IWGXBitmapSource, IUnknown)
{
    STDMETHOD(GetSize)(
        /* [out] */ UINT *puWidth,
        /* [out] */ UINT *puHeight
        ) PURE;

    STDMETHOD(GetPixelFormat)(
        /* [out] */ MilPixelFormat::Enum *pPixelFormat
        ) PURE;

    STDMETHOD(GetResolution)(
        /* [out] */ double *pDpiX,
        /* [out] */ double *pDpiY
        ) PURE;

    STDMETHOD(CopyPalette)(
        /* [in] */ IWICPalette *pIPalette
        ) PURE;

    STDMETHOD(CopyPixels)(
        /* [in] */ const MILRect *prc,
        /* [in] */ UINT cbStride,
        /* [in] */ UINT cbBufferSize,
        /* [out] */ BYTE *pvPixels // __out_ecount(cbBufferSize)
        ) PURE;
};

/*=========================================================================*\
    IWGXBitmapLock - Lock object for bitmaps.
\*=========================================================================*/
#undef INTERFACE
#define INTERFACE IWGXBitmapLock

DECLARE_INTERFACE_(IWGXBitmapLock, IUnknown)
{
    STDMETHOD(GetSize)(
        /* [in, out] */ UINT *pWidth,
        /* [in, out] */ UINT *pHeight
        ) PURE;

    STDMETHOD(GetStride)(
        /* [in, out] */ UINT *pStride
        ) PURE;

    STDMETHOD(GetDataPointer)(
        /* [out] */ UINT *pcbBufferSize,
        /* [out] */ BYTE **ppvData
        ) PURE;

    STDMETHOD(GetPixelFormat)(
        /* [out] */ MilPixelFormat::Enum *pPixelFormat
        ) PURE;
};


/*=========================================================================*\
    IWGXBitmap - A bitmap source with direct pixel access and dirty support
\*=========================================================================*/
#undef INTERFACE
#define INTERFACE IWGXBitmap

DECLARE_INTERFACE_(IWGXBitmap, IWGXBitmapSource)
{
    STDMETHOD(Lock)(
        THIS_
        /* [in] */ const MILRect *prcLock,
        /* [in] */ DWORD flags,
        /* [out] */ IWGXBitmapLock **ppILock
        ) PURE;
    
    STDMETHOD(SetPalette)(
        /* [in] */ IWICPalette *pIPalette
        ) PURE;

    STDMETHOD(SetResolution)(
        /* [in] */ double dpiX,
        /* [in] */ double dpiY
        ) PURE;

    STDMETHOD(AddDirtyRect)(
        __in_ecount(1) const RECT *prcDirtyRect
        ) PURE;

    __success(true) STDMETHOD_(bool, GetDirtyRects)(
        __deref_out_ecount(*pcDirtyRects) MilRectU const **const prgDirtyRects,   // disallow assignment to prgDirtyRects directly
        __deref_out_range(0,5) UINT * const pcDirtyRects,
        __inout_ecount(1) UINT * const pCachedUniqueness
        ) PURE;

    // Distinguishes between bitmaps with full source, no source, or
    // a video memory only surface

    struct SourceState
    {
        enum Enum
        {
            FullSystemMemory,
                // Full source in system memory
            NoSource,
                // No source of any kind whatsoever. There is
                // nothing to copy from this bitmap.
            DeviceBitmap,     
                // Bitmap is a CDeviceBitmap. No system bits
                // but a video source that we (usually) share with DX.
        };
    };

    STDMETHOD_(SourceState::Enum, SourceState)() const PURE; 

    // This is from IMILResource but we can't derive from IMILResource here as well 
    // because it leads to ambiguous access to the IUnknown methods
    STDMETHOD_(void, GetUniquenessToken)(__out_ecount(1) UINT *puToken) const PURE;
    
    enum { c_maxBitmapDirtyListSize = 5 };
};

#ifdef __cplusplus
};
#endif

#include "wgx_error.h"

#endif /* !__MILRENDER_H__ */


