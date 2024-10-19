/******************************Module*Header**********************************\
*
* Module Name: d3dkmdt.h
*
* Content: Windows Display Driver Model (WDDM) kernel mode
*          data type definitions
*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*
\*****************************************************************************/
#ifndef _D3DKMDT_H
#define _D3DKMDT_H

#if !defined(_D3DKMDDI_H_)      && \
    !defined(_D3DKMDDI_PEXT_H_) && \
    !defined(_DXGDMM_H_)        && \
    !defined(_VIDPRIV_H_)       && \
    !defined(_DISPMPRT_H_)      && \
    !defined(_DMM_DIAG_H_)      && \
    !defined(_D3DKMTHK_H_)
   #error This header should not be included directly!
#endif

#include "d3dukmdt.h"

#ifndef __REACTOS__

#define NTSTATUS                int32_t

/*
 * Some of the Windows return codes, which needs to be translated to Linux
 * IOCTL return codes. Positive values are success codes and need to be
 * returned from the driver IOCTLs. libdxcore.so depends on returning
 * specific return codes.
 */
#define STATUS_SUCCESS					((NTSTATUS)(0))
#define	STATUS_OBJECT_NAME_INVALID		((NTSTATUS)(0xC0000033L))
#define	STATUS_DEVICE_REMOVED			((NTSTATUS)(0xC00002B6L))
#define	STATUS_INVALID_HANDLE			((NTSTATUS)(0xC0000008L))
#define	STATUS_ILLEGAL_INSTRUCTION		((NTSTATUS)(0xC000001DL))
#define	STATUS_NOT_IMPLEMENTED			((NTSTATUS)(0xC0000002L))
#define	STATUS_PENDING					((NTSTATUS)(0x00000103L))
#define	STATUS_ACCESS_DENIED			((NTSTATUS)(0xC0000022L))
#define	STATUS_BUFFER_TOO_SMALL			((NTSTATUS)(0xC0000023L))
#define	STATUS_OBJECT_TYPE_MISMATCH		((NTSTATUS)(0xC0000024L))
#define	STATUS_GRAPHICS_ALLOCATION_BUSY	((NTSTATUS)(0xC01E0102L))
#define	STATUS_NOT_SUPPORTED			((NTSTATUS)(0xC00000BBL))
#define	STATUS_TIMEOUT					((NTSTATUS)(0x00000102L))
#define	STATUS_INVALID_PARAMETER		((NTSTATUS)(0xC000000DL))
#define	STATUS_NO_MEMORY				((NTSTATUS)(0xC0000017L))
#define	STATUS_OBJECT_NAME_COLLISION	((NTSTATUS)(0xC0000035L))
#define STATUS_OBJECT_NAME_NOT_FOUND	((NTSTATUS)(0xC0000034L))
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)(0xC0000001L))
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)(0xC000000DL))
#define NT_SUCCESS(status)              (status >= 0)

typedef enum {
    PowerActionNone = 0,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject,
    PowerActionDisplayOff
} POWER_ACTION, *PPOWER_ACTION;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified = 0,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

#else

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

#endif // !__REACTOS__

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#pragma warning(push)
#pragma warning(disable:4201) // anonymous unions warning
#pragma warning(disable:4214)   // nonstandard extension used: bit field types other than int

//
// Available only for Vista (LONGHORN) and later and for
// multiplatform tools such as debugger extensions
//
#if defined(__REACTOS__) || ((NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL))

//
// Hardcoded overlay count
//
#define D3DKMDT_MAX_OVERLAYS_BITCOUNT           2
#define D3DKMDT_MAX_OVERLAYS                    (1 << D3DKMDT_MAX_OVERLAYS_BITCOUNT)


//////////////////// VidPN management DDI handles /////////////////////////////////////////////////////////
DECLARE_HANDLE(D3DKMDT_HVIDPN);
DECLARE_HANDLE(D3DKMDT_HVIDEOPRESENTSOURCESET);
DECLARE_HANDLE(D3DKMDT_HVIDEOPRESENTTARGETSET);
DECLARE_HANDLE(D3DKMDT_HVIDPNTOPOLOGY);
DECLARE_HANDLE(D3DKMDT_HVIDPNSOURCEMODESET);
DECLARE_HANDLE(D3DKMDT_HVIDPNTARGETMODESET);
DECLARE_HANDLE(D3DKMDT_HMONITORSOURCEMODESET);
DECLARE_HANDLE(D3DKMDT_HMONITORFREQUENCYRANGESET);
DECLARE_HANDLE(D3DKMDT_HMONITORDESCRIPTORSET);

// Alias VOID* to make LDDM kernel mode interface prototypes using adapter handles self-explanatory.
typedef VOID* D3DKMDT_ADAPTER;



//////////////////// VidPN management DDI constants /////////////////////////////////////////////////////////

// Special values representing that given variable has not been initialized to a valid value intended
// to catch development time errors. A valid parameter should never have this value.
#define D3DKMDT_DIMENSION_UNINITIALIZED  (UINT)(~0)
#define D3DKMDT_FREQUENCY_UNINITIALIZED  (UINT)(~0)

// Special values representing that given parameter is not-specified.
// A parameter having this value should be ignored.
#define D3DKMDT_DIMENSION_NOTSPECIFIED  (UINT)(~1)
#define D3DKMDT_FREQUENCY_NOTSPECIFIED  (UINT)(~1)



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video mode standard descriptor type, listing standards that are explicitly supported by Windows.
//
// Remarks: This enum specifies based on which standard the video signal timing parameters should be computed.
//          Note that NTSC, PAL, and SECAM mode variants are treated as individual groups to avoid complicating
//          the DDI with the notion of "sub-standard" (because they don't differ from each other in the parameters
//          used to describe the video signal in the DDI and the parameters that they do differ in are of no
//          interest to us in DMM).
//
typedef enum _D3DKMDT_VIDEO_SIGNAL_STANDARD
{
    D3DKMDT_VSS_UNINITIALIZED =  0,

    // VESA standards
    D3DKMDT_VSS_VESA_DMT      =  1,    // See VESA Display Monitor Timings specification
    D3DKMDT_VSS_VESA_GTF      =  2,    // See VESA Generalized Timing Formula standard
    D3DKMDT_VSS_VESA_CVT      =  3,    // See VESA Coordinated Video Timings standard

    // De-facto standards
    D3DKMDT_VSS_IBM           =  4,
    D3DKMDT_VSS_APPLE         =  5,

    // Legacy STV standards                 W  x H{i|p} @ (  VR        /      HR       )
    D3DKMDT_VSS_NTSC_M        =  6,    //  720 x 525i   @ (59.94  [Hz] /  15,734.27[Hz])
    D3DKMDT_VSS_NTSC_J        =  7,    //  720 x 525i   @ (59.94  [Hz] /  15,734.27[Hz])
    D3DKMDT_VSS_NTSC_443      =  8,    //  720 x 525i   @ (59.94  [Hz] /  15,734.27[Hz])
    D3DKMDT_VSS_PAL_B         =  9,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_B1        = 10,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_G         = 11,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_H         = 12,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_I         = 13,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_D         = 14,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_N         = 15,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_NC        = 16,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_B       = 17,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_D       = 18,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_G       = 19,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_H       = 20,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_K       = 21,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_K1      = 22,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_L       = 23,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_SECAM_L1      = 24,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])

    // CEA/EIA standards
    D3DKMDT_VSS_EIA_861       = 25,
    D3DKMDT_VSS_EIA_861A      = 26,
    D3DKMDT_VSS_EIA_861B      = 27,

    // More legacy STV standards
    D3DKMDT_VSS_PAL_K         = 28,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_K1        = 29,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_L         = 30,    //  720 x 625i   @ (50     [Hz] /  15,625   [Hz])
    D3DKMDT_VSS_PAL_M         = 31,    //  720 x 525i   @ (59.94  [Hz] /  15,734   [Hz])

    D3DKMDT_VSS_OTHER         = 255
}
D3DKMDT_VIDEO_SIGNAL_STANDARD;



//////////////////// Video present sources //////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source descriptor type.
//
typedef struct _D3DKMDT_VIDEO_PRESENT_SOURCE
{
    // Unique ID used to reference the respective video present source by the miniport and the OS.
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  Id;

    // Other video present source descriptor properties go here.
    DWORD  dwReserved;
}
D3DKMDT_VIDEO_PRESENT_SOURCE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN source mode enumeration type descriptor type.
//
// Remarks: This type is used to specify whether a VidPN source mode is a graphics or a text mode
//          (see VIDEO_PRESENT_SOURCE_MODE for more details).
//
typedef enum _D3DKMDT_VIDPN_SOURCE_MODE_TYPE
{
    D3DKMDT_RMT_UNINITIALIZED                 = 0,
    D3DKMDT_RMT_GRAPHICS                      = 1,
    D3DKMDT_RMT_TEXT                          = 2,
    D3DKMDT_RMT_GRAPHICS_STEREO               = 3,
    D3DKMDT_RMT_GRAPHICS_STEREO_ADVANCED_SCAN = 4
}
D3DKMDT_VIDPN_SOURCE_MODE_TYPE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Pixel value access mode descriptor type.
//
// Remarks: Use Direct to represent VidPN source modes with colors stored directly in the primary surface.
//          Use PresetPalette to represent VidPN source modes with colors' indices stored in the primary
//          surface and actual color values stored in a palette specific to the video card, that must
//          be queried from the video miniport.
//          Use SettablePalette to represent VidPN source modes with colors' indices stored in the primary
//          surface and actual color values stored in a settable palette that can be dynamically set on
//          the video card, by specifying it to the video miniport.
//
typedef enum _D3DKMDT_PIXEL_VALUE_ACCESS_MODE
{
    D3DKMDT_PVAM_UNINITIALIZED   = 0,
    D3DKMDT_PVAM_DIRECT          = 1,
    D3DKMDT_PVAM_PRESETPALETTE   = 2,
    D3DKMDT_PVAM_SETTABLEPALETTE = 3,
}
D3DKMDT_PIXEL_VALUE_ACCESS_MODE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Descriptor type of the color basis with respect to which the pixels' colors are expanded,
//          or conversely, based on which the color values are synthesized.
//
// Remarks: The commonly used color bases in graphics industry are RGB, which has the basis (red, green, blue),
//          as well as YPbPr and YCbCr, which have scaled variants of basis:
//          (1, blue-1, red-1)*intensity(red,green,blue).
//          Tri-stimulus linear RGB is well suited for real-time rendering, since most filtering algorithms
//          use tri-stimulus values to approximate light's spectral transformations caused by its interaction
//          with the environment, primarily due to the fact that there is a linear relationship between the
//          perceived light level and the light's spectral intensity. Ideally, all processing of video content
//          (i.e. scaling, filtering, etc) should be performed in a linear RGB space.
//          Y'PbPr spaces store data using a nonlinear curve which is approximately the inverse of a gamma
//          2.2 curve (i.e. x^0.45).  This allows more precision to be stored in darker intensities where the
//          human eye is more sensitive.
//          sRGB (more accurately, sR'G'B') stores light intensities relative to a gamma curve.
//          scRGB stores linear values and requires much higher precision to represent the same perceptually
//          similar signal.
//          The light-intensity based YPbPr and YCbCr is better suited for persistence of pre-rendered content,
//          such as video streaming. This is due to the fact that a human visual system is more responsive to
//          small differences in photons' intensity rather than frequency (i.e. perceived color), and, hence,
//          a light-intensity based color expansion over a finite dynamic range, yields a better perceptual
//          image quality for the human eye than a tri-stimulus based color expansion in that same range
//          (e.g  non-linear Y8Cb8Cr8 appears slightly better than R8G8B8 and is comparable to R9G9B9).
//          To represent monochrome modes, use Intensity. Grayscale imaging is heavily used in medical imaging.
//          Note: the apostrophe notation Y'PbPr is used to remind you that you are working with non-linear data.
//
typedef enum _D3DKMDT_COLOR_BASIS
{
    D3DKMDT_CB_UNINITIALIZED = 0,
    D3DKMDT_CB_INTENSITY     = 1,
    D3DKMDT_CB_SRGB          = 2,
    D3DKMDT_CB_SCRGB         = 3,
    D3DKMDT_CB_YCBCR         = 4,
    D3DKMDT_CB_YPBPR         = 5,
}
D3DKMDT_COLOR_BASIS;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Descriptor type of the color coefficients dynamic range, whose linear combination with the
//          respective color basis produces final pixel values.
//
// Remarks: Examples include (5,6,5,0) for R5G6B5, (8,8,8,8) for R8G8B8A8, and (24, 0, 0, 0) for 24-bit
//          grayscale pixel encoding format.
//          NOTE: Currently this is only used for target modes, none of which has the 4th channel (e.g. alpha).
//                We are keeping the 4th field for extensibility purpose to avoid miniport interface revision
//                if 4-channel video interfaces became available between display adapter and monitor.
//
typedef struct _D3DKMDT_COLOR_COEFF_DYNAMIC_RANGES
{
    UINT  FirstChannel;
    UINT  SecondChannel;
    UINT  ThirdChannel;
    UINT  FourthChannel;
}
D3DKMDT_COLOR_COEFF_DYNAMIC_RANGES;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: 2D region descriptor type.
//
// Remarks: We define our own rather than reusing SIZE type to avoid dependency on SDK headers.
//
typedef struct _D3DKMDT_2DREGION
{
    UINT cx;
    UINT cy;
}
D3DKMDT_2DREGION;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: 2D offset descriptor type.
//
// Remarks: We define our own rather than reusing SIZE type to avoid dependency on SDK headers.
//
typedef D3DKMDT_2DREGION  D3DKMDT_2DOFFSET;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Graphics video present source mode descriptor type.
//
// Remarks: Graphics video present source mode is the dominantly used subtype of the video present source
//          modes (other being the text video present source mode).
//          Note that whenever video present source mode's visible size,
//          VIDEO_PRESENT_SOURCE_MODE.GRAPHICS_RENDERING_FORMAT.sizeVisible is not equal to the respective
//          video mode's visible size, VIDEO_PRESENT_TARGET_MODE.sizeVisible, h/w scaling is undertaken by
//          the video output codec.
//
//          Miniport is free to support any D3D pixel format for its graphics modes that is meaningful
//          as a primary surface pixel format.
//
typedef struct _D3DKMDT_GRAPHICS_RENDERING_FORMAT
{
    // Size of the primary surface required for this VidPN source mode.
    D3DKMDT_2DREGION  PrimSurfSize;

    // Size of the visible part of the primary surface, used for panned modes including zoom modes.
    // The visible region size is required to be the same as the primary surface
    D3DKMDT_2DREGION  VisibleRegionSize;

    // Number of bytes between the start of one scan line and the next.
    DWORD  Stride;

    // Pixel format type
    D3DDDIFORMAT  PixelFormat;

    // Color basis with respect to which rendering client encodes pixel values.
    D3DKMDT_COLOR_BASIS  ColorBasis;

    // Access mode for the pixel value information.
    D3DKMDT_PIXEL_VALUE_ACCESS_MODE  PixelValueAccessMode;
}
D3DKMDT_GRAPHICS_RENDERING_FORMAT;


typedef enum _D3DKMDT_TEXT_RENDERING_FORMAT
{
    D3DKMDT_TRF_UNINITIALIZED = 0
}
D3DKMDT_TEXT_RENDERING_FORMAT;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source mode ID type.
typedef UINT  D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN source mode descriptor type.
//
// Remarks: VidPN source (rendering) mode is the mode of operation of a given video present source and determines
//          the format of this source's primary surface to which the graphics subsystem is rendering
//          the visual image to be presented to the user, and from which the video output codec is reading
//          the visual image content to be converted into a respective video mode signal.
//
typedef struct _D3DKMDT_VIDPN_SOURCE_MODE
{
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID  Id;

    // Specifies whether the mode is a graphics or a text rendering mode.
    D3DKMDT_VIDPN_SOURCE_MODE_TYPE  Type;

    union
    {
        // Descriptor of the graphics rendering mode (valid only if Type==D3DKMDT_RMT_GRAPHICS).
        D3DKMDT_GRAPHICS_RENDERING_FORMAT  Graphics;

        // Descriptor of the text rendering mode (valid only if Type==D3DKMDT_RMT_TEXT).
        D3DKMDT_TEXT_RENDERING_FORMAT  Text;
    }
    Format;
}
D3DKMDT_VIDPN_SOURCE_MODE;



//////////////////////// Video present targets///////////////////////////////////////////////////////////////


// NOTE: Child device (e.g. video output) HPD awareness is used to represent the level of external
//       device (e.g. monitor) connectivity sensed by a display adapter. Child device can either be
//       always connected to an external device (e.g. integrated LCD in a mobile system) or have:
//         1. No HPD-awareness iff miniport is *not* aware of external device arrivals/departures
//            whether through interrupts or polling,
//         2. Polled HPD-awareness iff miniport can not asynchronously notify the OS about external
//            device arrivals/departures, but OS can sporadically poll for its presence, and of a
//            monitor, causing visual artifacts on each poll.
//         3. Interruptible HPD-awareness iff miniport can asynchronously notify the OS about
//            external device arrivals/departures.
typedef enum _DXGK_CHILD_DEVICE_HPD_AWARENESS {
    HpdAwarenessUninitialized   = 0,
    HpdAwarenessAlwaysConnected = 1,
    HpdAwarenessNone            = 2,
    HpdAwarenessPolled          = 3,
    HpdAwarenessInterruptible   = 4
} DXGK_CHILD_DEVICE_HPD_AWARENESS, *PDXGK_CHILD_DEVICE_HPD_AWARENESS;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor orientation descriptor type.
//
typedef enum _D3DKMDT_MONITOR_ORIENTATION
{
    D3DKMDT_MO_UNINITIALIZED = 0,
    D3DKMDT_MO_0DEG          = 1,
    D3DKMDT_MO_90DEG         = 2,
    D3DKMDT_MO_180DEG        = 3,
    D3DKMDT_MO_270DEG        = 4
}
D3DKMDT_MONITOR_ORIENTATION;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video output technology descriptor type.
//
// Remarks: Video output technology is used to determine the hard-coded list of video modes supported
//          by the monitor, when monitor descriptor is not available.
//
typedef enum _D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY
{
    D3DKMDT_VOT_UNINITIALIZED            = -2,
    D3DKMDT_VOT_OTHER                    = -1,
    D3DKMDT_VOT_HD15                     =  0,
    D3DKMDT_VOT_SVIDEO                   =  1,
    D3DKMDT_VOT_COMPOSITE_VIDEO          =  2,
    D3DKMDT_VOT_COMPONENT_VIDEO          =  3,
    D3DKMDT_VOT_DVI                      =  4,
    D3DKMDT_VOT_HDMI                     =  5,
    D3DKMDT_VOT_LVDS                     =  6,
    D3DKMDT_VOT_D_JPN                    =  8,
    D3DKMDT_VOT_SDI                      =  9,
    D3DKMDT_VOT_DISPLAYPORT_EXTERNAL     = 10,
    D3DKMDT_VOT_DISPLAYPORT_EMBEDDED     = 11,
    D3DKMDT_VOT_UDI_EXTERNAL             = 12,
    D3DKMDT_VOT_UDI_EMBEDDED             = 13,
    D3DKMDT_VOT_SDTVDONGLE               = 14,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_M1)
    D3DKMDT_VOT_MIRACAST                 = 15,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMDT_VOT_INDIRECT_WIRED           = 16,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_M1)
    D3DKMDT_VOT_INTERNAL                 = 0x80000000,

    // Remove when DDI is unlocked.
    D3DKMDT_VOT_SVIDEO_4PIN          = D3DKMDT_VOT_SVIDEO,
    D3DKMDT_VOT_SVIDEO_7PIN          = D3DKMDT_VOT_SVIDEO,
    D3DKMDT_VOT_RF                   = D3DKMDT_VOT_COMPOSITE_VIDEO,
    D3DKMDT_VOT_RCA_3COMPONENT       = D3DKMDT_VOT_COMPONENT_VIDEO,
    D3DKMDT_VOT_BNC                  = D3DKMDT_VOT_COMPONENT_VIDEO,
}
D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY;

typedef enum _DXGKMDT_OPM_CONNECTOR_TYPE
{
    DXGKMDT_OPM_CONNECTOR_TYPE_OTHER                                = -1,
    DXGKMDT_OPM_CONNECTOR_TYPE_HD15                                 =  0,
    DXGKMDT_OPM_CONNECTOR_TYPE_SVIDEO                               =  1,
    DXGKMDT_OPM_CONNECTOR_TYPE_COMPOSITE_VIDEO                      =  2,
    DXGKMDT_OPM_CONNECTOR_TYPE_COMPONENT_VIDEO                      =  3,
    DXGKMDT_OPM_CONNECTOR_TYPE_DVI                                  =  4,
    DXGKMDT_OPM_CONNECTOR_TYPE_HDMI                                 =  5,
    DXGKMDT_OPM_CONNECTOR_TYPE_LVDS                                 =  6,
    DXGKMDT_OPM_CONNECTOR_TYPE_D_JPN                                =  8,
    DXGKMDT_OPM_CONNECTOR_TYPE_SDI                                  =  9,
    DXGKMDT_OPM_CONNECTOR_TYPE_DISPLAYPORT_EXTERNAL                 = 10,
    DXGKMDT_OPM_CONNECTOR_TYPE_DISPLAYPORT_EMBEDDED                 = 11,
    DXGKMDT_OPM_CONNECTOR_TYPE_UDI_EXTERNAL                         = 12,
    DXGKMDT_OPM_CONNECTOR_TYPE_UDI_EMBEDDED                         = 13,
    DXGKMDT_OPM_CONNECTOR_TYPE_RESERVED                             = 14,
    DXGKMDT_OPM_CONNECTOR_TYPE_MIRACAST                             = 15,
    DXGKMDT_OPM_CONNECTOR_TYPE_TRANSPORT_AGNOSTIC_DIGITAL_MODE_A    = 16,
    DXGKMDT_OPM_CONNECTOR_TYPE_TRANSPORT_AGNOSTIC_DIGITAL_MODE_B    = 17,
    DXGKMDT_OPM_COPP_COMPATIBLE_CONNECTOR_TYPE_INTERNAL = 0x80000000
} DXGKMDT_OPM_CONNECTOR_TYPE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor orientation awareness descriptor type.
//
typedef enum _D3DKMDT_MONITOR_ORIENTATION_AWARENESS
{
    D3DKMDT_MOA_UNINITIALIZED = 0,
    D3DKMDT_MOA_NONE          = 1,
    D3DKMDT_MOA_POLLED        = 2,
    D3DKMDT_MOA_INTERRUPTIBLE = 3
}
D3DKMDT_MONITOR_ORIENTATION_AWARENESS;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target descriptor type.
//
typedef struct _D3DKMDT_VIDEO_PRESENT_TARGET
{
    // Unique ID used to reference the respective video present target by the miniport and the OS.
    D3DDDI_VIDEO_PRESENT_TARGET_ID  Id;

    // Type of the video output technology (see D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY for more details).
    D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY  VideoOutputTechnology;

    // Type of the video output's HPD awareness (see D3DKMDT_VIDEO_OUTPUT_HPD_AWARENESS for more details).
    DXGK_CHILD_DEVICE_HPD_AWARENESS  VideoOutputHpdAwareness;

    D3DKMDT_MONITOR_ORIENTATION_AWARENESS  MonitorOrientationAwareness;

    // NOTE: On monitor arrival, OS will leave monitor mode set empty for monitors connected to video outputs
    //       supporting SDTV modes, expecting miniport to populate modes it wants to expose for that monitor.
    //
    // NOTE: This predicate is also used when selecting video output for presentation on a TV via
    //       APIs that don't support explicit specification of monitors/video outputs (e.g. VIDEOPARAMETERS).
    //
    // Predicate specifying whether corresponding video output supports SDTV modes.
    BOOLEAN  SupportsSdtvModes;
}
D3DKMDT_VIDEO_PRESENT_TARGET;



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode GTF compliance descriptor type.
typedef enum _D3DKMDT_GTFCOMPLIANCE
{
    D3DKMDT_GTF_UNINITIALIZED = 0,
    D3DKMDT_GTF_COMPLIANT     = 1,
    D3DKMDT_GTF_NOTCOMPLIANT  = 2
}
D3DKMDT_GTFCOMPLIANCE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN target mode preference descriptor type.

typedef enum _D3DKMDT_MODE_PREFERENCE
{
    D3DKMDT_MP_UNINITIALIZED = 0,
    D3DKMDT_MP_PREFERRED     = 1,
    D3DKMDT_MP_NOTPREFERRED  = 2,
}
D3DKMDT_MODE_PREFERENCE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video signal info descriptor type.
//
// Remarks: Video signal info is the mode of operation of a given video output that's driving a connected
//          moitor and is driven by an internal video output codec.
//          Note that this descriptor supersedes subset of the XDDM's VIDEO_MODE_INFORMATION structure
//          related to video mode. In XDDM, both video and rendering modes were described in this struct.
//          LDDM separates these two notions, and hence their descriptors.
//          The video standard field, vidStandard, should be used for video mode comparisons, when it's
//          set to a well-defined video standard. Note that most of the standard modes do not comply with
//          the VESA GTF frequency constraints.
//
//          If color basis of the target mode does not correspond to that of the source mode, appropriate
//          color conversion is performed by the respective video output codec.
//
typedef struct _D3DKMDT_VIDEO_SIGNAL_INFO
{
    // Video mode standard this mode is defined by (if any).
    D3DKMDT_VIDEO_SIGNAL_STANDARD  VideoStandard;

    // Video signal's size in pixels (i.e. HTotal & VTotal).
    D3DKMDT_2DREGION  TotalSize;

    // Presented image's size in active pixels (i.e. HActive & VActive).
    D3DKMDT_2DREGION  ActiveSize;

    // Vertical refresh frequency (in Hz).
    D3DDDI_RATIONAL  VSyncFreq;

    // Horizontal refresh frequency (in Hz).
    D3DDDI_RATIONAL  HSyncFreq;

    // Pixel clock rate (in Hz).
    SIZE_T  PixelRate;

    union
    {
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_M1)
        struct
        {
            // Scan line ordering (e.g. progressive, interlaced).
#ifdef __REACTOS__
            UINT ScanLineOrdering : 3; // D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING
#else
            D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING ScanLineOrdering : 3;
#endif

            // Vertical refresh frequency divider
            UINT VSyncFreqDivider               : 6;

            UINT Reserved                       : 23;

        } AdditionalSignalInfo;
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3_M1

        // Scan line ordering (e.g. progressive, interlaced).
        D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING  ScanLineOrdering;
    };
}
D3DKMDT_VIDEO_SIGNAL_INFO;


#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#define D3DKMDT_BITS_PER_COMPONENT_06 0x01
#define D3DKMDT_BITS_PER_COMPONENT_08 0x02
#define D3DKMDT_BITS_PER_COMPONENT_10 0x04
#define D3DKMDT_BITS_PER_COMPONENT_12 0x08
#define D3DKMDT_BITS_PER_COMPONENT_14 0x10
#define D3DKMDT_BITS_PER_COMPONENT_16 0x20

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode ID type.
typedef UINT  D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Describe bit-fields for wire format and preference
//  In the D3DKMDT_VIDPN_TARGET_MODE structure all fields are valid and the wire format fields indicate
//  supported bit depts per color encoding format.
//  In the DXGK_SET_TIMING_PATH_INFO structure the Preference field is unused so should be ignored by the
//  driver and will be set to zero by the OS in case of future usage.  The wire format fields are used to
//  indicate which wire format has been selected so only one bit across the five fields will be set.
//

typedef union _D3DKMDT_WIRE_FORMAT_AND_PREFERENCE
{
    struct
    {
        D3DKMDT_MODE_PREFERENCE Preference  : 2;

        // Flags indicating supported formats of color components transmitted between source and target
        UINT                    Rgb         : 6;
        UINT                    YCbCr444    : 6;
        UINT                    YCbCr422    : 6;
        UINT                    YCbCr420    : 6;
        UINT                    Intensity   : 6;
    };
    UINT Value;
} D3DKMDT_WIRE_FORMAT_AND_PREFERENCE, *PD3DKMDT_WIRE_FORMAT_AND_PREFERENCE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode descriptor type.
typedef struct _D3DKMDT_VIDPN_TARGET_MODE
{
    // Identifier unique within the set this mode is part of.
    D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID  Id;

    // Video signal parameters.
    D3DKMDT_VIDEO_SIGNAL_INFO  VideoSignalInfo;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    union
    {
        D3DKMDT_WIRE_FORMAT_AND_PREFERENCE  WireFormatAndPreference;
        struct
        {
            D3DKMDT_MODE_PREFERENCE Preference  : 2;
            UINT                                :30;
        };
    };
#else
    // Predicate specifying whether this mode is preferred by the adapter given the mode pinned on
    // the source of the respective present path.
    D3DKMDT_MODE_PREFERENCE  Preference;
#endif // (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

    D3DDDI_RATIONAL MinimumVSyncFreq;

#endif // (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WDDM2_9)

}
D3DKMDT_VIDPN_TARGET_MODE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VESA Display ID detailed timing type I.

#pragma pack(push, 1)
#define DISPLAYID_DETAILED_TIMING_TYPE_I_SIZE     20

// Enum specifying monitor's aspect ratio (see DISPLAYID_DETAILED_TIMING_TYPE_I.AspectRatio)
enum _DISPLAYID_DETAILED_TIMING_TYPE_I_ASPECT_RATIO
{
    DIDDT1_AspectRatio_1x1   = 0,               // 000 = 1:1
    DIDDT1_AspectRatio_5x4   = 1,               // 001 = 5:4
    DIDDT1_AspectRatio_4x3   = 2,               // 010 = 4:3
    DIDDT1_AspectRatio_15x9  = 3,               // 011 = 15:9
    DIDDT1_AspectRatio_16x9  = 4,               // 100 = 16:9
    DIDDT1_AspectRatio_16x10 = 5,               // 101 = 16:10
};

enum _DISPLAYID_DETAILED_TIMING_TYPE_I_SCANNING_MODE
{
    DIDDT1_Progressive       = 0,               // 0 = Progressive mode
    DIDDT1_Interlaced        = 1,               // 1 = Interlaced mode
};

enum _DISPLAYID_DETAILED_TIMING_TYPE_I_STEREO_MODE
{
    DIDDT1_Monoscopic        = 0,               // 00 = Monoscopic (no stereo) mode
    DIDDT1_Stereo            = 1,               // 01 = Stereo mode
    DIDDT1_Dependent         = 2,               // 10 = Stereo mode despends on user action
};

enum _DISPLAYID_DETAILED_TIMING_TYPE_I_SYNC_POLARITY
{
    DIDDT1_Sync_Positive     = 0,
    DIDDT1_Sync_Negative     = 1,
};

typedef struct _DISPLAYID_DETAILED_TIMING_TYPE_I
{
    struct
    {
        ULONG  PixelClock       : 24;           // Pixel clock / 10000
        ULONG  AspectRatio      :  3;           // Aspect ratio
        ULONG  Reserved         :  1;
        ULONG  ScanningType     :  1;           // Frame scanning type
        ULONG  StereoMode       :  2;           // 3D stereo mode
        ULONG  PreferredTiming  :  1;           // preferred timing
    };

    USHORT  HorizontalActivePixels;             // Horizontal active image pixel number
    USHORT  HorizontalBlankPixels;              // Horizontal blank pixel number
    struct
    {
        USHORT  HorizontalFrontPorch    : 15;   // Horizontal offset (front porch) pixel number
        USHORT  HorizontalSyncPolarity  :  1;   // Horizontal sync polarity
    };
    USHORT  HorizontalSyncWidth;                // Horizontal sync pixel number

    USHORT  VerticalActiveLines;                // Number of lines of vertical active image
    USHORT  VerticalBlankLines;                 // Number of lines of vertical blank
    struct
    {
        USHORT  VerticalFrontPorch    : 15;     // Number of lines of vertical offset (front porch)
        USHORT  VerticalSyncPolarity  :  1;     // Vertical sync polarity
    };
    USHORT  VerticalSyncWidth;                  // Number of lines of vertical sync
}DISPLAYID_DETAILED_TIMING_TYPE_I;

C_ASSERT(sizeof(DISPLAYID_DETAILED_TIMING_TYPE_I) == DISPLAYID_DETAILED_TIMING_TYPE_I_SIZE);

#pragma pack(pop)

typedef struct _DXGK_TARGETMODE_DETAIL_TIMING
{
    // Video standard this detail timing comes from.
    D3DKMDT_VIDEO_SIGNAL_STANDARD       VideoStandard;

    // ID of this detail timing in VideoStandard.
    UINT                                TimingId;

    // Detail timing.
    DISPLAYID_DETAILED_TIMING_TYPE_I    DetailTiming;
}DXGK_TARGETMODE_DETAIL_TIMING;


// Structure of hardware capability
typedef struct _D3DKMDT_VIDPN_HW_CAPABILITY
{
    UINT DriverRotation             : 1;
    UINT DriverScaling              : 1;
    UINT DriverCloning              : 1;
    UINT DriverColorConvert         : 1;
    UINT DriverLinkedAdapaterOutput : 1;
    UINT DriverRemoteDisplay        : 1;
    UINT Reserved                   : 26;
}
D3DKMDT_VIDPN_HW_CAPABILITY;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode ID type.
typedef UINT  D3DKMDT_MONITOR_SOURCE_MODE_ID;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor capability origin type.
typedef enum _D3DKMDT_MONITOR_CAPABILITIES_ORIGIN
{
    D3DKMDT_MCO_UNINITIALIZED                      = 0, // mode information is coming from the:
    D3DKMDT_MCO_DEFAULTMONITORPROFILE              = 1, //  + default monitor profile.
    D3DKMDT_MCO_MONITORDESCRIPTOR                  = 2, //  + monitor's descriptor.
    D3DKMDT_MCO_MONITORDESCRIPTOR_REGISTRYOVERRIDE = 3, //  + registry override of the monitor descriptor.
    D3DKMDT_MCO_SPECIFICCAP_REGISTRYOVERRIDE       = 4, //  + registry override of a specific capability.
    D3DKMDT_MCO_DRIVER                             = 5, //  + display adapter driver.
}
D3DKMDT_MONITOR_CAPABILITIES_ORIGIN;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor timing type.
typedef enum _D3DKMDT_MONITOR_TIMING_TYPE
{
    D3DKMDT_MTT_UNINITIALIZED         = 0, // mode timing information is coming from:
    D3DKMDT_MTT_ESTABLISHED           = 1, //  + established timings block.
    D3DKMDT_MTT_STANDARD              = 2, //  + standard timings block.
    D3DKMDT_MTT_EXTRASTANDARD         = 3, //  + extra standard timings block.
    D3DKMDT_MTT_DETAILED              = 4, //  + detailed timings block.
    D3DKMDT_MTT_DEFAULTMONITORPROFILE = 5, //  + default monitor profile.
    D3DKMDT_MTT_DRIVER                = 6, //  + display adapter driver.
}
D3DKMDT_MONITOR_TIMING_TYPE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target mode descriptor type.
typedef struct _D3DKMDT_MONITOR_SOURCE_MODE
{
    // Identifier unique within the set this mode is part of.
    D3DKMDT_MONITOR_SOURCE_MODE_ID  Id;

    // Video signal parameters.
    D3DKMDT_VIDEO_SIGNAL_INFO  VideoSignalInfo;

    // Color basis with respect to which monitor is presenting the pixels sampled from the video signal.
    D3DKMDT_COLOR_BASIS  ColorBasis;

    // Supported dynamic range of each of the pixel color component coefficients by the monitor's
    // presentational technology - e.g. for a DFP LCD with 12-bit bit-depth, this will be (4, 4, 4, 0).
    // A TV supporting 10-bit per component YCbCr this will be (10, 10, 10, 0) regardless of chroma compression.
    D3DKMDT_COLOR_COEFF_DYNAMIC_RANGES  ColorCoeffDynamicRanges;

    // Origins of the monitor source mode information.
    D3DKMDT_MONITOR_CAPABILITIES_ORIGIN  Origin;

    // Predicate specifying whether this mode is preferred by the monitor connected to the respective video output.
    D3DKMDT_MODE_PREFERENCE  Preference;
}
D3DKMDT_MONITOR_SOURCE_MODE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Frequency range type.
typedef struct _D3DKMDT_FREQUENCY_RANGE
{
    // Minimum vertical refresh frequency (in Hz) supported by the monitor.
    D3DDDI_RATIONAL  MinVSyncFreq;

    // Maximum vertical refresh frequency (in Hz) supported by the monitor.
    D3DDDI_RATIONAL  MaxVSyncFreq;

    // Minimum horizontal refresh frequency (in Hz) supported by the monitor.
    D3DDDI_RATIONAL  MinHSyncFreq;

    // Maximum horizontal refresh frequency (in Hz) supported by the monitor.
    D3DDDI_RATIONAL  MaxHSyncFreq;
}
D3DKMDT_FREQUENCY_RANGE;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor frequency range constraint type.
typedef enum _D3DKMDT_MONITOR_FREQUENCY_RANGE_CONSTRAINT
{
    D3DKMDT_MFRC_UNINITIALIZED = 0,
    D3DKMDT_MFRC_ACTIVESIZE    = 1,
    D3DKMDT_MFRC_MAXPIXELRATE  = 2
}
D3DKMDT_MONITOR_FREQUENCY_RANGE_CONSTRAINT;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Monitor frequency range type.
typedef struct _D3DKMDT_MONITOR_FREQUENCY_RANGE
{
    // Origins of the monitor frequency range information.
    D3DKMDT_MONITOR_CAPABILITIES_ORIGIN  Origin;

    // Frequency range limits.
    D3DKMDT_FREQUENCY_RANGE  RangeLimits;

    // Type of the frequency range constraint.
    D3DKMDT_MONITOR_FREQUENCY_RANGE_CONSTRAINT  ConstraintType;

    // Constraint under which this frequency range is supported.
    union
    {
        // Active region size this frequency range applies to.
        D3DKMDT_2DREGION  ActiveSize;

        // Maximum pixel clock rate (in Hz) to which this pixel rate applies.
        SIZE_T  MaxPixelRate;

    } Constraint;
}
D3DKMDT_MONITOR_FREQUENCY_RANGE;




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VidPN present paths

// VidPN present path index used during enumeration of paths satisfying certain criteria
// (e.g. originating from the same source).
typedef SIZE_T  D3DKMDT_VIDPN_PRESENT_PATH_INDEX;

// Purpose: VidPN present path scaling type.
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_SCALING
{
    D3DKMDT_VPPS_UNINITIALIZED           = 0,

    // For the following transformation, spatial resolutions must be equal on both the source and the target.

    // Source content is not modified in any way.
    D3DKMDT_VPPS_IDENTITY                = 1,

    // For the following three transformations, spatial resolution of the source differs from that of the target.

    // Source content is centered on the target.
    D3DKMDT_VPPS_CENTERED                = 2,

    // Source content is scaled to fit the target, no aspect ratio preserving.
    D3DKMDT_VPPS_STRETCHED               = 3,

    // Source content is scaled to fit the target. Aspect ratio preserving.
    D3DKMDT_VPPS_ASPECTRATIOCENTEREDMAX  = 4,

    // Scaling that cannot be described by any other D3DKMDT_VPPS_XXX value
    D3DKMDT_VPPS_CUSTOM                  = 5,


    // Reserved for internal OS use
    D3DKMDT_VPPS_RESERVED1               = 253,

    // Source content scaling mode is not pinned.
    D3DKMDT_VPPS_UNPINNED                = 254,

    // OS does not specify the scaling mode, and miniport should decide based on its own settings.
    D3DKMDT_VPPS_NOTSPECIFIED            = 255
}
D3DKMDT_VIDPN_PRESENT_PATH_SCALING;


// Purpose: VidPN present path rotation type.
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_ROTATION
{
    D3DKMDT_VPPR_UNINITIALIZED = 0,

    // Source content is not modified in any way.
    D3DKMDT_VPPR_IDENTITY      = 1,

    // Source content is rotated 90 degrees.
    D3DKMDT_VPPR_ROTATE90      = 2,

    // Source content is rotated 180 degrees.
    D3DKMDT_VPPR_ROTATE180     = 3,

    // Source content is rotated 270 degrees.
    D3DKMDT_VPPR_ROTATE270     = 4,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)
    // Source content is not modified in any way. Driver should rotate this content an extra 90 degrees.
    D3DKMDT_VPPR_IDENTITY_OFFSET90      = 5,

    // Source content is rotated 90 degrees. Driver should rotate this content an extra 90 degrees.
    D3DKMDT_VPPR_ROTATE90_OFFSET90      = 6,

    // Source content is rotated 180 degrees. Driver should rotate this content an extra 90 degrees.
    D3DKMDT_VPPR_ROTATE180_OFFSET90     = 7,

    // Source content is rotated 270 degrees. Driver should rotate this content an extra 90 degrees.
    D3DKMDT_VPPR_ROTATE270_OFFSET90     = 8,

    // Source content is not modified in any way. Driver should rotate this content an extra 180 degrees.
    D3DKMDT_VPPR_IDENTITY_OFFSET180      = 9,

    // Source content is rotated 90 degrees. Driver should rotate this content an extra 180 degrees.
    D3DKMDT_VPPR_ROTATE90_OFFSET180      = 10,

    // Source content is rotated 180 degrees. Driver should rotate this content an extra 180 degrees.
    D3DKMDT_VPPR_ROTATE180_OFFSET180     = 11,

    // Source content is rotated 270 degrees. Driver should rotate this content an extra 180 degrees.
    D3DKMDT_VPPR_ROTATE270_OFFSET180     = 12,

    // Source content is not modified in any way. Driver should rotate this content an extra 270 degrees.
    D3DKMDT_VPPR_IDENTITY_OFFSET270      = 13,

    // Source content is rotated 90 degrees. Driver should rotate this content an extra 270 degrees.
    D3DKMDT_VPPR_ROTATE90_OFFSET270      = 14,

    // Source content is rotated 180 degrees. Driver should rotate this content an extra 270 degrees.
    D3DKMDT_VPPR_ROTATE180_OFFSET270     = 15,

    // Source content is rotated 270 degrees. Driver should rotate this content an extra 270 degrees.
    D3DKMDT_VPPR_ROTATE270_OFFSET270     = 16,
#endif // DXGKDDI_INTERFACE_VERSION

    // Source content rotation setting is not pinned.
    D3DKMDT_VPPR_UNPINNED      = 254,

    // OS does not specify the rotation mode, and miniport should decide based on its own settings.
    D3DKMDT_VPPR_NOTSPECIFIED  = 255
}
D3DKMDT_VIDPN_PRESENT_PATH_ROTATION;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)

__inline
D3DKMDT_VIDPN_PRESENT_PATH_ROTATION D3DKMDT_VPPR_GET_OFFSET_ROTATION(D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation)
{
    if ((Rotation >= D3DKMDT_VPPR_IDENTITY) &&
        (Rotation <= D3DKMDT_VPPR_ROTATE270_OFFSET270))
    {
        Rotation = (D3DKMDT_VIDPN_PRESENT_PATH_ROTATION)(((Rotation - 1) / 4) + 1);
    }
    return Rotation;
}

__inline
D3DKMDT_VIDPN_PRESENT_PATH_ROTATION D3DKMDT_VPPR_GET_CONTENT_ROTATION_PART(D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation)
{
    if ((Rotation >= D3DKMDT_VPPR_IDENTITY_OFFSET90) &&
        (Rotation <= D3DKMDT_VPPR_ROTATE270_OFFSET270))
    {
        Rotation = (D3DKMDT_VIDPN_PRESENT_PATH_ROTATION)(((Rotation - 1) % 4) + 1);
    }
    return Rotation;
}

__inline
D3DKMDT_VIDPN_PRESENT_PATH_ROTATION D3DKMDT_VPPR_GET_CONTENT_ROTATION(D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation)
{
    if ((Rotation >= D3DKMDT_VPPR_IDENTITY_OFFSET90) &&
        (Rotation <= D3DKMDT_VPPR_ROTATE270_OFFSET270))
    {
        D3DKMDT_VIDPN_PRESENT_PATH_ROTATION ContentPart = D3DKMDT_VPPR_GET_CONTENT_ROTATION_PART(Rotation);
        D3DKMDT_VIDPN_PRESENT_PATH_ROTATION OffsetPart = D3DKMDT_VPPR_GET_OFFSET_ROTATION(Rotation);
        Rotation = (D3DKMDT_VIDPN_PRESENT_PATH_ROTATION)((((ContentPart - 1) + (OffsetPart - 1)) % 4) + 1);
    }
    return Rotation;
}
#endif // DXGKDDI_INTERFACE_VERSION


#define D3DKMDT_SCALING_SUPPORT_MASK 0x1f;

// Purpose: Specifies what scaling modes are supported given current path configuration.
typedef struct _D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT
{
    UINT Identity                 : 1;
    UINT Centered                 : 1;
    UINT Stretched                : 1;
    UINT AspectRatioCenteredMax   : 1;
    UINT Custom                   : 1;
} D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)
#define D3DKMDT_ROTATION_SUPPORT_MASK 0xff;
#else
#define D3DKMDT_ROTATION_SUPPORT_MASK 0xf;
#endif // DXGKDDI_INTERFACE_VERSION

// Purpose: Specifies what rotation modes are supported given current path configuration.
typedef struct _D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT
{
    UINT Identity   : 1;
    UINT Rotate90   : 1;
    UINT Rotate180  : 1;
    UINT Rotate270  : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_PATH_INDEPENDENT_ROTATION)
    UINT Offset0    : 1;
    UINT Offset90   : 1;
    UINT Offset180  : 1;
    UINT Offset270  : 1;
#endif // DXGKDDI_INTERFACE_VERSION
} D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT;

// Purpose: Combines all the transformation related fields into one structure.
typedef struct _D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION
{
    // Scaling applied to the content presented on this video present path.
    D3DKMDT_VIDPN_PRESENT_PATH_SCALING  Scaling;

    // Scaling support given the currently pinned modes.
    D3DKMDT_VIDPN_PRESENT_PATH_SCALING_SUPPORT ScalingSupport;

    // Rotation applied to the content presented on this video present path.
    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION Rotation;

    // Rotation support given the currently pinned modes.
    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION_SUPPORT RotationSupport;
} D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN present path importance ordinal type.
//
// Note: Higher order importance ordinals can be used (upto 255), but only the first 10 have been explicitly named.
//
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_IMPORTANCE
{
    D3DKMDT_VPPI_UNINITIALIZED = 0,
    D3DKMDT_VPPI_PRIMARY       = 1,
    D3DKMDT_VPPI_SECONDARY     = 2,
    D3DKMDT_VPPI_TERTIARY      = 3,
    D3DKMDT_VPPI_QUATERNARY    = 4,
    D3DKMDT_VPPI_QUINARY       = 5,
    D3DKMDT_VPPI_SENARY        = 6,
    D3DKMDT_VPPI_SEPTENARY     = 7,
    D3DKMDT_VPPI_OCTONARY      = 8,
    D3DKMDT_VPPI_NONARY        = 9,
    D3DKMDT_VPPI_DENARY        = 10,
}
D3DKMDT_VIDPN_PRESENT_PATH_IMPORTANCE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN present path content ordinal type.
//
// Indicates whether the content being displayed is video or graphics.
//
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_CONTENT
{
    D3DKMDT_VPPC_UNINITIALIZED = 0,

    // Miniport should optimize presentation of the present path for graphics content.
    D3DKMDT_VPPC_GRAPHICS      = 1,

    // Miniport should optimize presentation of the present path for video content.
    D3DKMDT_VPPC_VIDEO         = 2,

    // OS does not specify the content type, and miniport should decide based on its own settings.
    D3DKMDT_VPPC_NOTSPECIFIED  = 255
}
D3DKMDT_VIDPN_PRESENT_PATH_CONTENT;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN present path macrovision information.
//
// Contains macrovision caps and controls.
//
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_TYPE
{
    D3DKMDT_VPPMT_UNINITIALIZED             = 0,

    D3DKMDT_VPPMT_NOPROTECTION              = 1,

    D3DKMDT_VPPMT_MACROVISION_APSTRIGGER    = 2,

    D3DKMDT_VPPMT_MACROVISION_FULLSUPPORT   = 3,

}
D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_TYPE;

typedef struct _D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_SUPPORT
{
    UINT NoProtection          : 1;
    UINT MacroVisionApsTrigger : 1;
    UINT MacroVisionFull       : 1;
    UINT Reserved              : 29;
}
D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_SUPPORT;

#define D3DKMDT_MACROVISION_OEMCOPYPROTECTION_SIZE  256

typedef struct _D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION
{
    // Macrovision controls.
    D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_TYPE CopyProtectionType;
    UINT                                           APSTriggerBits;
    BYTE                                           OEMCopyProtection[D3DKMDT_MACROVISION_OEMCOPYPROTECTION_SIZE];

    // Level of available copy protection support.
    D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION_SUPPORT CopyProtectionSupport;
} D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: VidPN present path gamma ramp descriptor type.
//
typedef struct _D3DKMDT_GAMMA_RAMP
{
    D3DDDI_GAMMARAMP_TYPE    Type;
    SIZE_T                   DataSize;

    // If (Type == D3DDDI_GAMMARAMP_DEFAULT), (DataSize == 0) and (Data.pRaw == NULL )
    union
    {
        D3DDDI_GAMMA_RAMP_RGB256x3x16*      pRgb256x3x16;       // Type == D3DDDI_GAMMARAMP_RGB256x3x16.
        D3DDDI_GAMMA_RAMP_DXGI_1*           pDxgi1;             // Type == D3DDDI_GAMMARAMP_DXGI_1.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
        D3DKMDT_3x4_COLORSPACE_TRANSFORM*   p3x4;               // Type == D3DDDI_GAMMARAMP_MATRIX_3x4.
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
        D3DKMDT_COLORSPACE_TRANSFORM_MATRIX_V2* pMatrixV2;     // Type == D3DDDI_GAMMARAMP_MATRIX_V2.
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
        VOID*                               pRaw;
    }
    Data;
}
D3DKMDT_GAMMA_RAMP;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present target to source mapping.
//
// Remarks: This type is used to describe a mapping from a single video present target to a single
//          video present source in a VidPN topology.
//
typedef struct _D3DKMDT_VIDPN_PRESENT_PATH
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;

    D3DDDI_VIDEO_PRESENT_TARGET_ID  VidPnTargetId;

    // Video present path importance ordinal (e.g. path presenting the primary view may have higher
    // importance set on it in order to guarantee that it gets the best source/target mode sets).
    D3DKMDT_VIDPN_PRESENT_PATH_IMPORTANCE  ImportanceOrdinal;

    // Contains all the transformation related fields.
    D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION ContentTransformation;

    // Storing deltas for visible/active pixels mapping rather than visible pixels'
    // size & offset has the added benefit of ideal/default state being zeros.

    // Monitor screen's offset of visible pixels' top-left corner from
    // video signal's active pixels top-left corner.
    // Note: Default = (0,0).
    D3DKMDT_2DOFFSET  VisibleFromActiveTLOffset;

    // Monitor screen's offset of visible pixels' bottom-right corner from
    // video signal's active pixels bottom-right corner.
    // Note: Default = (0,0).
    D3DKMDT_2DOFFSET  VisibleFromActiveBROffset;

    // Video signal color basis with respect to which video output codec encodes the pixels from the respective
    // source's primary surface (e.g. on HDMI this can be either RGB or YCbCr).
    D3DKMDT_COLOR_BASIS  VidPnTargetColorBasis;

    // Supported dynamic range of each of the color bases' coefficients by the video output codec's output
    // E.g. With a DAC scaning out A2R10G10B10 primary surface at R8G8B8 color resolution, this would be (8,8,8,0),
    D3DKMDT_COLOR_COEFF_DYNAMIC_RANGES  VidPnTargetColorCoeffDynamicRanges;

    // Indicates the content being diplayed.  The driver can use this to determine whether the flicker filter
    // and overscan should be enabled or disabled.
    D3DKMDT_VIDPN_PRESENT_PATH_CONTENT Content;

    // Contains all the copy protection related fields.
    D3DKMDT_VIDPN_PRESENT_PATH_COPYPROTECTION CopyProtection;

    // A lookup table (LUT) used to compensate intensity of presented content for the color response of the monitor
    // connected to present path's target.
    D3DKMDT_GAMMA_RAMP  GammaRamp;
}
D3DKMDT_VIDPN_PRESENT_PATH;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Specifies whether to ignore monitor connectivity checks or enforce them.
//
typedef enum _D3DKMDT_MONITOR_CONNECTIVITY_CHECKS
{
    D3DKMDT_MCC_UNINITIALIZED = 0,
    D3DKMDT_MCC_IGNORE        = 1,
    D3DKMDT_MCC_ENFORCE       = 2
}
D3DKMDT_MONITOR_CONNECTIVITY_CHECKS;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Type of the owner whose mode set does not change during an enumeration.
//
// Remarks: When OS pins/unpins a mode in one of VidPN's mode sets, the only mode set that is guaranteed to
//          stay the same is the mode set whose mode was pinned/unpinned. Enumeration pivot lets the OS
//          specify to the miniport which mode set it should not update (because this mode set could not have
//          been invalidated as a result of the change the OS made to the VidPN.
//
typedef enum _D3DKMDT_ENUMCOFUNCMODALITY_PIVOT_TYPE
{
    D3DKMDT_EPT_UNINITIALIZED,
    D3DKMDT_EPT_VIDPNSOURCE,
    D3DKMDT_EPT_VIDPNTARGET,
    D3DKMDT_EPT_SCALING,
    D3DKMDT_EPT_ROTATION,
    D3DKMDT_EPT_NOPIVOT
}
D3DKMDT_ENUMCOFUNCMODALITY_PIVOT_TYPE;

//
// Monitor descriptor related types.
//

typedef UINT  D3DKMDT_MONITOR_DESCRIPTOR_ID;

typedef enum _D3DKMDT_MONITOR_DESCRIPTOR_TYPE
{
    D3DKMDT_MDT_UNINITIALIZED          = 0,
    D3DKMDT_MDT_VESA_EDID_V1_BASEBLOCK = 1,
    D3DKMDT_MDT_VESA_EDID_V1_BLOCKMAP  = 2,
    D3DKMDT_MDT_OTHER                  = 255
}
D3DKMDT_MONITOR_DESCRIPTOR_TYPE;

typedef struct _D3DKMDT_MONITOR_DESCRIPTOR
{
    D3DKMDT_MONITOR_DESCRIPTOR_ID        Id;
    D3DKMDT_MONITOR_DESCRIPTOR_TYPE      Type;
    SIZE_T                               DataSize;
    VOID*                                pData;
    D3DKMDT_MONITOR_CAPABILITIES_ORIGIN  Origin;
}
D3DKMDT_MONITOR_DESCRIPTOR;


typedef enum _D3DKMDT_STANDARDALLOCATION_TYPE
{
    D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE  = 1,
    D3DKMDT_STANDARDALLOCATION_SHADOWSURFACE         = 2,
    D3DKMDT_STANDARDALLOCATION_STAGINGSURFACE        = 3,
    D3DKMDT_STANDARDALLOCATION_GDISURFACE            = 4,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    D3DKMDT_STANDARDALLOCATION_VGPU                  = 5,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
} D3DKMDT_STANDARDALLOCATION_TYPE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
typedef enum _D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY
{
    D3DKMDT_GRAPHICS_PREEMPTION_NONE                = 0,
    D3DKMDT_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY = 100,
    D3DKMDT_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY  = 200,
    D3DKMDT_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY   = 300,
    D3DKMDT_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY      = 400,
    D3DKMDT_GRAPHICS_PREEMPTION_SHADER_BOUNDARY     = 500,
} D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY;

typedef enum _D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY
{
    D3DKMDT_COMPUTE_PREEMPTION_NONE                     = 0,
    D3DKMDT_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY      = 100,
    D3DKMDT_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY        = 200,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY    = 300,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_BOUNDARY          = 400,
    D3DKMDT_COMPUTE_PREEMPTION_SHADER_BOUNDARY          = 500,
} D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct _D3DKMDT_PREEMPTION_CAPS
{
    D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY  ComputePreemptionGranularity;
} D3DKMDT_PREEMPTION_CAPS;

typedef struct _D3DKMT_WDDM_1_2_CAPS
{
    D3DKMDT_PREEMPTION_CAPS PreemptionCaps;
    union
    {
        struct
        {
            UINT SupportNonVGA : 1;
            UINT SupportSmoothRotation : 1;
            UINT SupportPerEngineTDR : 1;
            UINT SupportKernelModeCommandBuffer : 1;
            UINT SupportCCD : 1;
            UINT SupportSoftwareDeviceBitmaps : 1;
            UINT SupportGammaRamp : 1;
            UINT SupportHWCursor : 1;
            UINT SupportHWVSync : 1;
            UINT SupportSurpriseRemovalInHibernation : 1;
            UINT Reserved : 22;
        };
        UINT Value;
    };
} D3DKMT_WDDM_1_2_CAPS;


#endif // DXGKDDI_INTERFACE_VERSION

typedef struct _D3DKMDT_SHAREDPRIMARYSURFACEDATA
{
    UINT                            Width;
    UINT                            Height;
    D3DDDIFORMAT                    Format;
    D3DDDI_RATIONAL                 RefreshRate;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
} D3DKMDT_SHAREDPRIMARYSURFACEDATA;

typedef struct _D3DKMDT_SHADOWSURFACEDATA
{
    UINT                            Width;
    UINT                            Height;
    D3DDDIFORMAT                    Format;
    UINT                            Pitch;      //out: Driver must return Pitch as this allocation will be lockabale
} D3DKMDT_SHADOWSURFACEDATA;

//Staging Surface is a surface created potentially
//during present when a direct Blt to primary is
//not possible in cases like multimon or sprites.
//This surface is always in the format of D3DDDIFMT_X8R8G8B8

typedef struct _D3DKMDT_STAGINGSURFACEDATA
{
    UINT                            Width;      //in: Width of the staging buffer
    UINT                            Height;     //in: Height of the staging buffer
    UINT                            Pitch;      //out: Driver must return Pitch as this allocation will be lockabale
} D3DKMDT_STAGINGSURFACEDATA;

typedef struct _D3DKMDT_GDISURFACEFLAGS
{
    union
    {
        struct
        {
            UINT Reserved       : 32;
        };
        UINT Value;
    };
} D3DKMDT_GDISURFACEFLAGS;

typedef enum _D3DKMDT_GDISURFACETYPE
{
    D3DKMDT_GDISURFACE_INVALID              = 0,
    D3DKMDT_GDISURFACE_TEXTURE              = 1,
    D3DKMDT_GDISURFACE_STAGING_CPUVISIBLE   = 2,
    D3DKMDT_GDISURFACE_STAGING              = 3,
    D3DKMDT_GDISURFACE_LOOKUPTABLE          = 4,
    D3DKMDT_GDISURFACE_EXISTINGSYSMEM       = 5,
    D3DKMDT_GDISURFACE_TEXTURE_CPUVISIBLE   = 6,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3_M1)
    D3DKMDT_GDISURFACE_TEXTURE_CROSSADAPTER = 7,
    D3DKMDT_GDISURFACE_TEXTURE_CPUVISIBLE_CROSSADAPTER = 8,     // Reserved for OS
#endif // DXGKDDI_INTERFACE_VERSION
} D3DKMDT_GDISURFACETYPE;

typedef struct _D3DKMDT_GDISURFACEDATA
{
    UINT                    Width;
    UINT                    Height;
    D3DDDIFORMAT            Format;
    D3DKMDT_GDISURFACETYPE  Type;
    D3DKMDT_GDISURFACEFLAGS Flags;
    UINT                    Pitch;          // out: The driver must return pitch if allocation is CPU visible
} D3DKMDT_GDISURFACEDATA;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

typedef struct _D3DKMDT_VIRTUALGPUSURFACEDATA
{
    UINT64                  Size;
    UINT                    Alignment;
    UINT                    DriverSegmentId;
    UINT                    PrivateDriverData;
} D3DKMDT_VIRTUALGPUSURFACEDATA;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

typedef struct _D3DKMDT_PALETTEDATA
{
    BYTE    Red;
    BYTE    Green;
    BYTE    Blue;
    BYTE    Unused;
} D3DKMDT_PALETTEDATA;

//
//Red - Bits to be put in the Red portion of the color registers.
//
//Green - Bits to be put in the Green portion of the color registers.
//
//Blue - Bits to be put in the Blue portion of the color registers.
//

typedef struct _DXGKARG_SETPALETTE
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    UINT                            FirstEntry;
    UINT                            NumEntries;
    D3DKMDT_PALETTEDATA*            pLookupTable;
} DXGKARG_SETPALETTE;

//
//NumEntries - Number of entries in the LookupTable of color values.
//
//FirstEntry - Location in the device palette to which the first entry in the
//    LookupTable of colors should be copied to. The other entries in the
//    LookupTable should be copied sequentially, from this starting point into
//    the device's palette.
//
//pLookupTable - Array of color entries to copy into the device's color
//    registers/palette. The color entries can be accessed as a genric 32 bit
//    value or as Red/Green/Blue/Unused fields.
//

typedef struct _D3DKMT_MOVE_RECT
{
    POINT   SourcePoint;
    RECT    DestRect;
}D3DKMT_MOVE_RECT;

//
// Display information structure.
//

#ifndef PHYSICAL_ADDRESS
typedef LARGE_INTEGER PHYSICAL_ADDRESS;
#endif

typedef struct _DXGK_DISPLAY_INFORMATION
{
    UINT Width;
    UINT Height;
    UINT Pitch;
    D3DDDIFORMAT ColorFormat;
    PHYSICAL_ADDRESS PhysicAddress;
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    ULONG AcpiId;
} DXGK_DISPLAY_INFORMATION, *PDXGK_DISPLAY_INFORMATION;


//////////////////// I2C ////////////////////////////////////////////////////////////////////////////////////

#define DXGKMDT_I2C_NO_FLAGS                     0
#define DXGKMDT_I2C_DEVICE_TRANSMITS_DATA_LENGTH 1

//////////////////// OPM (Output Protection Manager) ////////////////////////////////////////////////////////

#pragma pack( push, 1 )

#define DXGKMDT_OPM_OMAC_SIZE                       16
#define DXGKMDT_OPM_128_BIT_RANDOM_NUMBER_SIZE      16
#define DXGKMDT_OPM_ENCRYPTED_PARAMETERS_SIZE       256
#define DXGKMDT_OPM_CONFIGURE_SETTING_DATA_SIZE     4056
#define DXGKMDT_OPM_GET_INFORMATION_PARAMETERS_SIZE 4056
#define DXGKMDT_OPM_REQUESTED_INFORMATION_SIZE      4076
#define DXGKMDT_OPM_HDCP_KEY_SELECTION_VECTOR_SIZE  5
#define DXGKMDT_OPM_PROTECTION_TYPE_SIZE            4

DEFINE_GUID(DXGKMDT_OPM_GET_CURRENT_HDCP_SRM_VERSION,              0x99c5ceff, 0x5f1d, 0x4879, 0x81, 0xc1, 0xc5, 0x24, 0x43, 0xc9, 0x48, 0x2b);
DEFINE_GUID(DXGKMDT_OPM_GET_CONNECTED_HDCP_DEVICE_INFORMATION,     0x0db59d74, 0xa992, 0x492e, 0xa0, 0xbd, 0xc2, 0x3f, 0xda, 0x56, 0x4e, 0x00);
DEFINE_GUID(DXGKMDT_OPM_GET_CONNECTOR_TYPE,                        0x81d0bfd5, 0x6afe, 0x48c2, 0x99, 0xc0, 0x95, 0xa0, 0x8f, 0x97, 0xc5, 0xda);
DEFINE_GUID(DXGKMDT_OPM_GET_SUPPORTED_PROTECTION_TYPES,            0x38f2a801, 0x9a6c, 0x48bb, 0x91, 0x07, 0xb6, 0x69, 0x6e, 0x6f, 0x17, 0x97);
DEFINE_GUID(DXGKMDT_OPM_GET_VIRTUAL_PROTECTION_LEVEL,              0xb2075857, 0x3eda, 0x4d5d, 0x88, 0xdb, 0x74, 0x8f, 0x8c, 0x1a, 0x05, 0x49);
DEFINE_GUID(DXGKMDT_OPM_GET_ACTUAL_PROTECTION_LEVEL,               0x1957210a, 0x7766, 0x452a, 0xb9, 0x9a, 0xd2, 0x7a, 0xed, 0x54, 0xf0, 0x3a);
DEFINE_GUID(DXGKMDT_OPM_GET_ACTUAL_OUTPUT_FORMAT,                  0xd7bf1ba3, 0xad13, 0x4f8e, 0xaf, 0x98, 0x0d, 0xcb, 0x3c, 0xa2, 0x04, 0xcc);
DEFINE_GUID(DXGKMDT_OPM_GET_ADAPTER_BUS_TYPE,                      0xc6f4d673, 0x6174, 0x4184, 0x8e, 0x35, 0xf6, 0xdb, 0x52, 0x0,  0xbc, 0xba);
DEFINE_GUID(DXGKMDT_OPM_GET_ACP_AND_CGMSA_SIGNALING,               0x6629a591, 0x3b79, 0x4cf3, 0x92, 0x4a, 0x11, 0xe8, 0xe7, 0x81, 0x16, 0x71);
DEFINE_GUID(DXGKMDT_OPM_GET_OUTPUT_ID,                             0x72cb6df3, 0x244f, 0x40ce, 0xb0, 0x9e, 0x20, 0x50, 0x6a, 0xf6, 0x30, 0x2f);
DEFINE_GUID(DXGKMDT_OPM_GET_DVI_CHARACTERISTICS,                   0xa470b3bb, 0x5dd7, 0x4172, 0x83, 0x9c, 0x3d, 0x37, 0x76, 0xe0, 0xeb, 0xf5);
DEFINE_GUID(DXGKMDT_OPM_GET_OUTPUT_HARDWARE_PROTECTION_SUPPORT,    0x3b129589, 0x2af8, 0x4ef0, 0x96, 0xa2, 0x70, 0x4a, 0x84, 0x5a, 0x21, 0x8e);
DEFINE_GUID(DXGKMDT_OPM_GET_CODEC_INFO,                            0x4f374491, 0x8f5f, 0x4445, 0x9d, 0xba, 0x95, 0x58, 0x8f, 0x6b, 0x58, 0xb4);
DEFINE_GUID(DXGKMDT_OPM_SET_PROTECTION_LEVEL,                      0x9bb9327c, 0x4eb5, 0x4727, 0x9f, 0x00, 0xb4, 0x2b, 0x09, 0x19, 0xc0, 0xda);
DEFINE_GUID(DXGKMDT_OPM_SET_ACP_AND_CGMSA_SIGNALING,               0x09a631a5, 0xd684, 0x4c60, 0x8e, 0x4d, 0xd3, 0xbb, 0x0f, 0x0b, 0xe3, 0xee);
DEFINE_GUID(DXGKMDT_OPM_SET_HDCP_SRM,                              0x8b5ef5d1, 0xc30d, 0x44ff, 0x84, 0xa5, 0xea, 0x71, 0xdc, 0xe7, 0x8f, 0x13);
DEFINE_GUID(DXGKMDT_OPM_SET_PROTECTION_LEVEL_ACCORDING_TO_CSS_DVD, 0x39ce333e, 0x4cc0, 0x44ae, 0xbf, 0xcc, 0xda, 0x50, 0xb5, 0xf8, 0x2e, 0x72);

typedef enum _DXGKMDT_CERTIFICATE_TYPE
{
    DXGKMDT_OPM_CERTIFICATE  = 0,
    DXGKMDT_COPP_CERTIFICATE = 1,
    DXGKMDT_UAB_CERTIFICATE  = 2,
    DXGKMDT_INDIRECT_DISPLAY_CERTIFICATE  = 3,
    DXGKMDT_FORCE_ULONG      = 0xFFFFFFFF
} DXGKMDT_CERTIFICATE_TYPE;

typedef enum _DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS
{
    DXGKMDT_OPM_VOS_COPP_SEMANTICS = 0,
    DXGKMDT_OPM_VOS_OPM_SEMANTICS  = 1,
    DXGKMDT_OPM_VOS_OPM_INDIRECT_DISPLAY = 2
} DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS;

typedef enum _DXGKMDT_DPCP_PROTECTION_LEVEL
{
    DXGKMDT_OPM_DPCP_OFF            = 0,
    DXGKMDT_OPM_DPCP_ON             = 1,
    DXGKMDT_OPM_DPCP_FORCE_ULONG    = 0x7fffffff

} DXGKMDT_OPM_DPCP_PROTECTION_LEVEL;

typedef enum _DXGKMDT_OPM_HDCP_FLAG
{
    DXGKMDT_OPM_HDCP_FLAG_NONE           = 0x00,
    DXGKMDT_OPM_HDCP_FLAG_REPEATER       = 0x01
} DXGKMDT_OPM_HDCP_FLAG;

typedef enum _DXGKMDT_OPM_STATUS
{
    DXGKMDT_OPM_STATUS_NORMAL                           = 0x00,
    DXGKMDT_OPM_STATUS_LINK_LOST                        = 0x01,
    DXGKMDT_OPM_STATUS_RENEGOTIATION_REQUIRED           = 0x02,
    DXGKMDT_OPM_STATUS_TAMPERING_DETECTED               = 0x04,
    DXGKMDT_OPM_STATUS_REVOKED_HDCP_DEVICE_ATTACHED     = 0x08
} DXGKMDT_OPM_STATUS;

// NUAE stands for Non-User Accessible Enclosure
typedef enum _DXGKMDT_OPM_BUS_TYPE_AND_IMPLEMENTATION
{
    DXGKMDT_OPM_BUS_TYPE_OTHER                                                       = 0x00000000,
    DXGKMDT_OPM_BUS_TYPE_PCI                                                         = 0x00000001,
    DXGKMDT_OPM_BUS_TYPE_PCIX                                                        = 0x00000002,
    DXGKMDT_OPM_BUS_TYPE_PCIEXPRESS                                                  = 0x00000003,
    DXGKMDT_OPM_BUS_TYPE_AGP                                                         = 0x00000004,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_INSIDE_OF_CHIPSET                        = 0x00010000,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_TRACKS_ON_MOTHER_BOARD_TO_CHIP           = 0x00020000,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_TRACKS_ON_MOTHER_BOARD_TO_SOCKET         = 0x00030000,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_DAUGHTER_BOARD_CONNECTOR                 = 0x00040000,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_DAUGHTER_BOARD_CONNECTOR_INSIDE_OF_NUAE  = 0x00050000,
    DXGKMDT_OPM_BUS_IMPLEMENTATION_MODIFIER_NON_STANDARD                             = 0x80000000,
    DXGKMDT_OPM_COPP_COMPATIBLE_BUS_TYPE_INTEGRATED                                  = 0x80000000
} DXGKMDT_OPM_BUS_TYPE_AND_IMPLEMENTATION;

typedef enum _DXGKMDT_OPM_HDCP_PROTECTION_LEVEL
{
    DXGKMDT_OPM_HDCP_OFF            = 0,
    DXGKMDT_OPM_HDCP_ON             = 1,
    DXGKMDT_OPM_HDCP_FORCE_ULONG    = 0x7fffffff
} DXGKMDT_OPM_HDCP_PROTECTION_LEVEL;

typedef enum _DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_PROTECTION_LEVEL
{
    DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_OFF                         = DXGKMDT_OPM_HDCP_OFF,
    DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_ON_WITH_NO_TYPE_RESTRICTION = DXGKMDT_OPM_HDCP_ON,       // No Type restriction on output topology nodes
    DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_ON_WITH_TYPE1_RESTRICTION   = DXGKMDT_OPM_HDCP_ON + 1,   // Restrict to Type 1 compliant output topology nodes
    DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_FORCE_ULONG                 = 0x7fffffff
} DXGKMDT_OPM_TYPE_ENFORCEMENT_HDCP_PROTECTION_LEVEL;

typedef enum _DXGKMDT_OPM_CGMSA
{
    DXGKMDT_OPM_CGMSA_OFF                                           = 0,
    DXGKMDT_OPM_CGMSA_COPY_FREELY                                   = 1,
    DXGKMDT_OPM_CGMSA_COPY_NO_MORE                                  = 2,
    DXGKMDT_OPM_CGMSA_COPY_ONE_GENERATION                           = 3,
    DXGKMDT_OPM_CGMSA_COPY_NEVER                                    = 4,
    DXGKMDT_OPM_REDISTRIBUTION_CONTROL_REQUIRED                     = 0x08,
} DXGKMDT_OPM_CGMSA;

typedef enum _DXGKMDT_OPM_ACP_PROTECTION_LEVEL
{
    DXGKMDT_OPM_ACP_OFF             = 0,
    DXGKMDT_OPM_ACP_LEVEL_ONE       = 1,
    DXGKMDT_OPM_ACP_LEVEL_TWO       = 2,
    DXGKMDT_OPM_ACP_LEVEL_THREE     = 3,
    DXGKMDT_OPM_ACP_FORCE_ULONG     = 0x7fffffff
} DXGKMDT_OPM_ACP_PROTECTION_LEVEL;

typedef enum _DXGKMDT_OPM_PROTECTION_TYPE
{
    DXGKMDT_OPM_PROTECTION_TYPE_OTHER                       = 0x80000000,
    DXGKMDT_OPM_PROTECTION_TYPE_NONE                        = 0x00000000,
    DXGKMDT_OPM_PROTECTION_TYPE_COPP_COMPATIBLE_HDCP        = 0x00000001,
    DXGKMDT_OPM_PROTECTION_TYPE_ACP                         = 0x00000002,
    DXGKMDT_OPM_PROTECTION_TYPE_CGMSA                       = 0x00000004,
    DXGKMDT_OPM_PROTECTION_TYPE_HDCP                        = 0x00000008,
    DXGKMDT_OPM_PROTECTION_TYPE_DPCP                        = 0x00000010,
    DXGKMDT_OPM_PROTECTION_TYPE_TYPE_ENFORCEMENT_HDCP       = 0x00000020,
    DXGKMDT_OPM_PROTECTION_TYPE_MASK                        = 0x8000003F
} DXGKMDT_OPM_PROTECTION_TYPE;

typedef enum _DXGKMDT_OPM_PROTECTION_STANDARD
{
    DXGKMDT_OPM_PROTECTION_STANDARD_OTHER                   = 0x80000000,
    DXGKMDT_OPM_PROTECTION_STANDARD_NONE                    = 0x00000000,
    DXGKMDT_OPM_PROTECTION_STANDARD_IEC61880_525I           = 0x00000001,
    DXGKMDT_OPM_PROTECTION_STANDARD_IEC61880_2_525I         = 0x00000002,
    DXGKMDT_OPM_PROTECTION_STANDARD_IEC62375_625P           = 0x00000004,
    DXGKMDT_OPM_PROTECTION_STANDARD_EIA608B_525             = 0x00000008,
    DXGKMDT_OPM_PROTECTION_STANDARD_EN300294_625I           = 0x00000010,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEA_525P      = 0x00000020,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEA_750P      = 0x00000040,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEA_1125I     = 0x00000080,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEB_525P      = 0x00000100,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEB_750P      = 0x00000200,
    DXGKMDT_OPM_PROTECTION_STANDARD_CEA805A_TYPEB_1125I     = 0x00000400,
    DXGKMDT_OPM_PROTECTION_STANDARD_ARIBTRB15_525I          = 0x00000800,
    DXGKMDT_OPM_PROTECTION_STANDARD_ARIBTRB15_525P          = 0x00001000,
    DXGKMDT_OPM_PROTECTION_STANDARD_ARIBTRB15_750P          = 0x00002000,
    DXGKMDT_OPM_PROTECTION_STANDARD_ARIBTRB15_1125I         = 0x00004000,
} DXGKMDT_OPM_PROTECTION_STANDARD;

typedef enum _DXGKMDT_OPM_IMAGE_ASPECT_RATIO_EN300294
{
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_FULL_FORMAT_4_BY_3                        = 0,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_BOX_14_BY_9_CENTER                        = 1,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_BOX_14_BY_9_TOP                           = 2,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_BOX_16_BY_9_CENTER                        = 3,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_BOX_16_BY_9_TOP                           = 4,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_BOX_GT_16_BY_9_CENTER                     = 5,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_FULL_FORMAT_4_BY_3_PROTECTED_CENTER       = 6,
    DXGKMDT_OPM_ASPECT_RATIO_EN300294_FULL_FORMAT_16_BY_9_ANAMORPHIC            = 7,
    DXGKMDT_OPM_ASPECT_RATIO_FORCE_ULONG                                        = 0x7FFFFFFF
} DXGKMDT_OPM_IMAGE_ASPECT_RATIO_EN300294;

typedef enum _DXGKMDT_OPM_INTERLEAVE_FORMAT
{
    DXGKMDT_OPM_INTERLEAVE_FORMAT_OTHER = 0,
    DXGKMDT_OPM_INTERLEAVE_FORMAT_PROGRESSIVE = 2,
    DXGKMDT_OPM_INTERLEAVE_FORMAT_INTERLEAVED_EVEN_FIRST = 3,
    DXGKMDT_OPM_INTERLEAVE_FORMAT_INTERLEAVED_ODD_FIRST = 4,
    DXGKMDT_OPM_INTERLEAVE_FORMAT_FORCE_ULONG = 0xFFFFFFFF

} DXGKMDT_OPM_INTERLEAVE_FORMAT;

typedef enum _DXGKDT_OPM_DVI_CHARACTERISTICS
{
    DXGKMDT_OPM_DVI_CHARACTERISTIC_1_0                      = 1,
    DXGKMDT_OPM_DVI_CHARACTERISTIC_1_1_OR_ABOVE             = 2,
    DXGKMDT_OPM_DVI_CHARACTERISTICS_FORCE_ULONG             = 0xFFFFFFFF
} DXGKDT_OPM_DVI_CHARACTERISTICS;

typedef enum _DXGKMDT_OPM_OUTPUT_HARDWARE_PROTECTION
{
    DXGKMDT_OPM_OUTPUT_HARDWARE_PROTECTION_NOT_SUPPORTED = 0x0,
    DXGKMDT_OPM_OUTPUT_HARDWARE_PROTECTION_SUPPORTED     = 0x1
} DXGKMDT_OPM_OUTPUT_HARDWARE_PROTECTION;

typedef struct _DXGKMDT_OPM_RANDOM_NUMBER
{
    BYTE abRandomNumber[DXGKMDT_OPM_128_BIT_RANDOM_NUMBER_SIZE];
} DXGKMDT_OPM_RANDOM_NUMBER, *PDXGKMDT_OPM_RANDOM_NUMBER;

typedef struct _DXGKMDT_OPM_OMAC
{
    BYTE abOMAC[DXGKMDT_OPM_OMAC_SIZE];
} DXGKMDT_OPM_OMAC, *PDXGKMDT_OPM_OMAC;

typedef struct _DXGKMDT_OPM_ENCRYPTED_PARAMETERS
{
    BYTE abEncryptedParameters[DXGKMDT_OPM_ENCRYPTED_PARAMETERS_SIZE];
} DXGKMDT_OPM_ENCRYPTED_PARAMETERS, *PDXGKMDT_OPM_ENCRYPTED_PARAMETERS;

typedef struct _DXGKMDT_OPM_GET_INFO_PARAMETERS
{
    DXGKMDT_OPM_OMAC omac;
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    GUID guidInformation;
    ULONG ulSequenceNumber;
    ULONG cbParametersSize;
    BYTE abParameters[DXGKMDT_OPM_GET_INFORMATION_PARAMETERS_SIZE];
} DXGKMDT_OPM_GET_INFO_PARAMETERS, *PDXGKMDT_OPM_GET_INFO_PARAMETERS;

typedef struct _DXGKMDT_OPM_COPP_COMPATIBLE_GET_INFO_PARAMETERS
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    GUID guidInformation;
    ULONG ulSequenceNumber;
    ULONG cbParametersSize;
    BYTE abParameters[DXGKMDT_OPM_GET_INFORMATION_PARAMETERS_SIZE];
} DXGKMDT_OPM_COPP_COMPATIBLE_GET_INFO_PARAMETERS, *PDXGKMDT_OPM_COPP_COMPATIBLE_GET_INFO_PARAMETERS;

typedef struct _DXGKMDT_OPM_HDCP_KEY_SELECTION_VECTOR
{
    BYTE abKeySelectionVector[DXGKMDT_OPM_HDCP_KEY_SELECTION_VECTOR_SIZE];
} DXGKMDT_OPM_HDCP_KEY_SELECTION_VECTOR;

typedef struct _DXGKMDT_OPM_CONNECTED_HDCP_DEVICE_INFORMATION
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    ULONG ulStatusFlags;
    ULONG ulHDCPFlags;
    DXGKMDT_OPM_HDCP_KEY_SELECTION_VECTOR ksvB;
    BYTE Reserved[11];
    BYTE Reserved2[16];
    BYTE Reserved3[16];
} DXGKMDT_OPM_CONNECTED_HDCP_DEVICE_INFORMATION;

typedef struct _DXGKMDT_OPM_REQUESTED_INFORMATION
{
    DXGKMDT_OPM_OMAC omac;
    ULONG cbRequestedInformationSize;
    BYTE abRequestedInformation[DXGKMDT_OPM_REQUESTED_INFORMATION_SIZE];
} DXGKMDT_OPM_REQUESTED_INFORMATION, *PDXGKMDT_OPM_REQUESTED_INFORMATION;

typedef struct _DXGKMDT_OPM_STANDARD_INFORMATION
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    ULONG ulStatusFlags;
    ULONG ulInformation;
    ULONG ulReserved;
    ULONG ulReserved2;
} DXGKMDT_OPM_STANDARD_INFORMATION;

typedef struct _DXGKMDT_OPM_ACTUAL_OUTPUT_FORMAT
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    ULONG ulStatusFlags;
    ULONG ulDisplayWidth;
    ULONG ulDisplayHeight;
    DXGKMDT_OPM_INTERLEAVE_FORMAT ifInterleaveFormat;
    ULONG d3dFormat;
    ULONG ulFrequencyNumerator;
    ULONG ulFrequencyDenominator;
} DXGKMDT_OPM_ACTUAL_OUTPUT_FORMAT;

typedef struct _DXGKMDT_OPM_ACP_AND_CGMSA_SIGNALING
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    ULONG ulStatusFlags;
    ULONG ulAvailableTVProtectionStandards;
    ULONG ulActiveTVProtectionStandard;
    ULONG ulReserved;
    ULONG ulAspectRatioValidMask1;
    ULONG ulAspectRatioData1;
    ULONG ulAspectRatioValidMask2;
    ULONG ulAspectRatioData2;
    ULONG ulAspectRatioValidMask3;
    ULONG ulAspectRatioData3;
    ULONG ulReserved2[4];
    ULONG ulReserved3[4];
} DXGKMDT_OPM_ACP_AND_CGMSA_SIGNALING;

typedef struct _DXGKMDT_OPM_OUTPUT_ID
{
    DXGKMDT_OPM_RANDOM_NUMBER rnRandomNumber;
    ULONG ulStatusFlags;
    UINT64 OutputId;
} DXGKMDT_OPM_OUTPUT_ID;

typedef struct _DXGKMDT_OPM_CONFIGURE_PARAMETERS
{
    DXGKMDT_OPM_OMAC omac;
    GUID guidSetting;
    ULONG ulSequenceNumber;
    ULONG cbParametersSize;
    BYTE abParameters[DXGKMDT_OPM_CONFIGURE_SETTING_DATA_SIZE];
} DXGKMDT_OPM_CONFIGURE_PARAMETERS, *PDXGKMDT_OPM_CONFIGURE_PARAMETERS;

typedef struct _DXGKMDT_OPM_SET_PROTECTION_LEVEL_PARAMETERS
{
    ULONG ulProtectionType;
    ULONG ulProtectionLevel;
    ULONG Reserved;
    ULONG Reserved2;
} DXGKMDT_OPM_SET_PROTECTION_LEVEL_PARAMETERS;

typedef struct _DXGKMDT_OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS
{
    ULONG ulNewTVProtectionStandard;
    ULONG ulAspectRatioChangeMask1;
    ULONG ulAspectRatioData1;
    ULONG ulAspectRatioChangeMask2;
    ULONG ulAspectRatioData2;
    ULONG ulAspectRatioChangeMask3;
    ULONG ulAspectRatioData3;
    ULONG ulReserved[4];
    ULONG ulReserved2[4];
    ULONG ulReserved3;
} DXGKMDT_OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS;

typedef struct _DXGKMDT_OPM_SET_HDCP_SRM_PARAMETERS
{
    ULONG ulSRMVersion;
} DXGKMDT_OPM_SET_HDCP_SRM_PARAMETERS;

typedef struct _DXGKMDT_OPM_CREATE_VIDEO_OUTPUT_FOR_TARGET_PARAMETERS
{
	LUID AdapterLuid;
	D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
	DXGKMDT_OPM_VIDEO_OUTPUT_SEMANTICS Vos;
} DXGKMDT_OPM_CREATE_VIDEO_OUTPUT_FOR_TARGET_PARAMETERS;

typedef struct _DXGK_BRIGHTNESS_CAPS
{
    union
    {
        struct
        {
            UINT SmoothBrightness   :    1;  // 0x00000001
            UINT AdaptiveBrightness :    1;  // 0x00000002
            UINT NitsBrightness     :    1;  // 0x00000004
            UINT Reserved           :    29; // 0xFFFFFFF8
        };

        UINT  Value;
    };
} DXGK_BRIGHTNESS_CAPS;

typedef struct _DXGK_BRIGHTNESS_STATE
{
    union
    {
        struct
        {
            UINT SmoothBrightness   :    1; // 0x00000001
            UINT Reserved           :   31; // 0xFFFFFFFE
        };

        UINT  Value;
    };
} DXGK_BRIGHTNESS_STATE;

typedef enum
{
    DxgkBacklightOptimizationDisable    = 0,
    DxgkBacklightOptimizationDesktop    = 1,
    DxgkBacklightOptimizationDynamic    = 2,
    DxgkBacklightOptimizationDimmed     = 3,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DxgkBacklightOptimizationEDR        = 4,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
} DXGK_BACKLIGHT_OPTIMIZATION_LEVEL;

typedef struct _DXGK_BACKLIGHT_INFO
{
    OUT USHORT                          BacklightUsersetting;
    OUT USHORT                          BacklightEffective;
    OUT D3DDDI_GAMMA_RAMP_RGB256x3x16   GammaRamp;
} DXGK_BACKLIGHT_INFO;

typedef struct _DXGK_BRIGHTNESS_SENSOR_DATA_CHROMATICITY
{
    float ChromaticityX;
    float ChromaticityY;
} DXGK_BRIGHTNESS_SENSOR_DATA_CHROMATICITY;

typedef struct _DXGK_BRIGHTNESS_SENSOR_DATA
{
    UINT32    Size;
    union
    {
        struct
        {
            UINT AlsReadingValid : 1;
            UINT ChromaticityValid : 1;
            UINT ColorTemperatureValid : 1;
            UINT Reserved : 29;
        } Flags;
        UINT ValidSensorValues;
    };
    float  AlsReading;
    DXGK_BRIGHTNESS_SENSOR_DATA_CHROMATICITY Chromaticity;
    float ColorTemperature;
} DXGK_BRIGHTNESS_SENSOR_DATA;


typedef struct _DXGK_BRIGHTNESS_SET_IN
{
    UINT32                             BrightnessMillinits;
    UINT32                             TransitionTimeMs;
    DXGK_BRIGHTNESS_SENSOR_DATA        SensorReadings;
} DXGK_BRIGHTNESS_SET_IN, *PDXGK_BRIGHTNESS_SET_IN;

typedef struct _DXGK_BRIGHTNESS_GET_OUT
{
    UINT32 CurrentBrightnessMillinits;
    UINT32 TargetBrightnessMillinits;
} DXGK_BRIGHTNESS_GET_OUT, *PDXGK_BRIGHTNESS_GET_OUT;

typedef struct _DXGK_BRIGHTNESS_NIT_RANGE
{
    UINT32 MinimumLevelMillinit;
    UINT32 MaximumLevelMillinit;
    UINT32 StepSizeMillinit;
} DXGK_BRIGHTNESS_NIT_RANGE;

#define DXGK_BRIGHTNESS_MAXIMUM_NIT_RANGE_COUNT 16

typedef struct _DXGK_BRIGHTNESS_GET_NIT_RANGES_OUT
{
    UINT32                      NormalRangeCount;
    UINT32                      RangeCount;
    UINT32                      PreferredMaximumBrightness;
    DXGK_BRIGHTNESS_NIT_RANGE   SupportedRanges[DXGK_BRIGHTNESS_MAXIMUM_NIT_RANGE_COUNT];
} DXGK_BRIGHTNESS_GET_NIT_RANGES_OUT, *PDXGK_BRIGHTNESS_GET_NIT_RANGES_OUT;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

typedef struct _D3DKMT_WDDM_1_3_CAPS
{
    union
    {
        struct
        {
            UINT SupportMiracast : 1;
            UINT IsHybridIntegratedGPU : 1;
            UINT IsHybridDiscreteGPU : 1;
            UINT SupportPowerManagementPStates : 1;
            UINT SupportVirtualModes : 1;
            UINT SupportCrossAdapterResource : 1;
            UINT Reserved : 26;
        };
        UINT Value;
    };
} D3DKMT_WDDM_1_3_CAPS;

#define DXGK_MAX_METADATA_NAME_LENGTH 32
typedef enum
{
    DXGK_ENGINE_TYPE_OTHER,
    DXGK_ENGINE_TYPE_3D,
    DXGK_ENGINE_TYPE_VIDEO_DECODE,
    DXGK_ENGINE_TYPE_VIDEO_ENCODE,
    DXGK_ENGINE_TYPE_VIDEO_PROCESSING,
    DXGK_ENGINE_TYPE_SCENE_ASSEMBLY,
    DXGK_ENGINE_TYPE_COPY,
    DXGK_ENGINE_TYPE_OVERLAY,
    DXGK_ENGINE_TYPE_CRYPTO,
    DXGK_ENGINE_TYPE_MAX
} DXGK_ENGINE_TYPE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _DXGK_NODEMETADATA_FLAGS
{
    union
    {
        struct
        {
            UINT ContextSchedulingSupported :  1;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

            UINT RingBufferFenceRelease     :  1;
            UINT SupportTrackedWorkload     :  1;
            UINT Reserved                   : 13;

            UINT MaxInFlightHwQueueBuffers  : 16;

#else

            UINT Reserved                   : 31;

#endif // !(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

        };
        UINT32 Value;
    };
} DXGK_NODEMETADATA_FLAGS;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

typedef struct _DXGK_NODEMETADATA
{
    DXGK_ENGINE_TYPE EngineType;
    WCHAR            FriendlyName[DXGK_MAX_METADATA_NAME_LENGTH];

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGK_NODEMETADATA_FLAGS Flags;
#else
    UINT32           Reserved;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    BOOLEAN          GpuMmuSupported;
    BOOLEAN          IoMmuSupported;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
} DXGK_NODEMETADATA;

typedef DXGK_NODEMETADATA DXGKARG_GETNODEMETADATA;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

typedef struct _DXGK_GPUCLOCKDATA_FLAGS
{
    union
    {
        struct
        {
            UINT ContextManagementProcessor :  1;
            UINT Reserved                   : 31;
        };
        UINT32 Value;
    };
} DXGK_GPUCLOCKDATA_FLAGS;

typedef struct _DXGK_NODE_PERFDATA
{
    ULONGLONG       Frequency;              // out: Clock frequency of the engine in hertz
    ULONGLONG       MaxFrequency;           // out: Max engine clock frequency
    ULONGLONG       MaxFrequencyOC;         // out: Max engine over clock frequency
    ULONG           Voltage;                // out: Voltage of the engine in milli volts mV
    ULONG           VoltageMax;             // out: Max voltage levels in milli volts.
    ULONG           VoltageMaxOC;           // out: Max voltage level while overclocked in milli volts.
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    ULONGLONG       MaxTransitionLatency;   // out: Max transition latency to change the frequency in 100 nanoseconds
#endif
} DXGK_NODE_PERFDATA;

typedef struct _DXGK_ADAPTER_PERFDATA
{
    ULONGLONG       MemoryFrequency;        // out: Clock frequency of the memory in hertz
    ULONGLONG       MaxMemoryFrequency;     // out: Max memory clock frequency
    ULONGLONG       MaxMemoryFrequencyOC;   // out: Clock frequency of the memory while overclocked in hertz.
    ULONGLONG       MemoryBandwidth;        // out: Amount of memory transferred in bytes
    ULONGLONG       PCIEBandwidth;          // out: Amount of memory transferred over PCI-E in bytes
    ULONG           FanRPM;                 // out: Fan rpm
    ULONG           Power;                  // out: Power draw of the adapter in tenths of a percentage
    ULONG           Temperature;            // out: Temperature in deci-Celsius 1 = 0.1C
    UCHAR           PowerStateOverride;     // out: Overrides dxgkrnls power view of linked adapters.
} DXGK_ADAPTER_PERFDATA;

typedef struct _DXGK_ADAPTER_PERFDATACAPS
{
    ULONGLONG   MaxMemoryBandwidth;     // out: Max memory bandwidth in bytes for 1 second
    ULONGLONG   MaxPCIEBandwidth;       // out: Max pcie bandwidth in bytes for 1 second
    ULONG       MaxFanRPM;              // out: Max fan rpm
    ULONG       TemperatureMax;         // out: Max temperature before damage levels
    ULONG       TemperatureWarning;     // out: The temperature level where throttling begins.
} DXGK_ADAPTER_PERFDATACAPS;

#define DXGK_MAX_GPUVERSION_NAME_LENGTH 32
typedef struct _DXGK_GPUVERSION
{
    WCHAR           BiosVersion[DXGK_MAX_GPUVERSION_NAME_LENGTH];     //out: The gpu bios version
    WCHAR           GpuArchitecture[DXGK_MAX_GPUVERSION_NAME_LENGTH]; //out: The gpu architectures name.
} DXGK_GPUVERSION;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

typedef struct _DXGK_GPUCLOCKDATA
{
    ULONGLONG               GpuFrequency;
    ULONGLONG               GpuClockCounter;
    ULONGLONG               CpuClockCounter;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    DXGK_GPUCLOCKDATA_FLAGS Flags;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
} DXGK_GPUCLOCKDATA;

typedef DXGK_GPUCLOCKDATA DXGKARG_CALIBRATEGPUCLOCK;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

typedef struct _D3DKMT_WDDM_2_0_CAPS
{
    union
    {
        struct
        {
            UINT Support64BitAtomics       : 1;
            UINT GpuMmuSupported           : 1;
            UINT IoMmuSupported            : 1;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
            UINT FlipOverwriteSupported    : 1;
            UINT SupportContextlessPresent : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
            UINT SupportSurpriseRemoval    : 1;
            UINT Reserved                  : 26;
#else
            UINT Reserved                  : 27;
#endif // !(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
#else
            UINT Reserved                  : 29;
#endif // !(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_0_CAPS;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_7)

typedef struct _D3DKMT_WDDM_2_7_CAPS
{
    union
    {
        struct
        {
            UINT HwSchSupported               :  1;    // Specifies whether the GPU supports hardware scheduling
            UINT HwSchEnabled                 :  1;    // Specifies whether the hardware scheduling is currently enabled for this GPU
            UINT HwSchEnabledByDefault        :  1;    // Set to 1 if the OS default policy is to enable hardware scheduling for this GPU
            UINT IndependentVidPnVSyncControl :  1;
            UINT Reserved                     : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_7_CAPS;

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

// DXGK_FEATURE_SUPPORT constants

// When a driver doesn't support a feature, it doesn't call into QueryFeatureSupport with that feature ID.
// This value is provided for implementation convenience of enumerating possible driver support states
// for a particular feature.
#define DXGK_FEATURE_SUPPORT_ALWAYS_OFF ((UINT)0)

// Driver support for a feature is in the experimental state
#define DXGK_FEATURE_SUPPORT_EXPERIMENTAL ((UINT)1)

// Driver support for a feature is in the stable state
#define DXGK_FEATURE_SUPPORT_STABLE ((UINT)2)

// Driver support for a feature is in the always on state,
// and it doesn't operate without this feature enabled.
#define DXGK_FEATURE_SUPPORT_ALWAYS_ON ((UINT)3)

typedef struct _D3DKMT_WDDM_2_9_CAPS
{
    union
    {
        struct
        {
            _Field_range_(DXGK_FEATURE_SUPPORT_ALWAYS_OFF, DXGK_FEATURE_SUPPORT_ALWAYS_ON)
            UINT    HwSchSupportState           :  2;   // DXGK_FEATURE_SUPPORT_* value that specifies driver support state for GPU supports hardware scheduling
            UINT    HwSchEnabled                :  1;   // Specifies whether the hardware scheduling is currently enabled for this GPU
            UINT    SelfRefreshMemorySupported  :  1;   // Specifies whether Self Refresh Memory is supported for this GPU
            UINT    Reserved                    : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_2_9_CAPS;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)

typedef struct _D3DKMT_WDDM_3_0_CAPS
{
    union
    {
        struct
        {
            _Field_range_(DXGK_FEATURE_SUPPORT_ALWAYS_OFF, DXGK_FEATURE_SUPPORT_ALWAYS_ON)
            UINT    HwFlipQueueSupportState     :  2;   // DXGK_FEATURE_SUPPORT_* value that specifies driver support state for GPU supports hardware flip queue
            UINT    HwFlipQueueEnabled          :  1;   // Specifies whether the hardware flip queue is currently enabled for this GPU
            UINT    DisplayableSupported        :  1;   // Specifies whether displayable feature is supported
            UINT    Reserved                    : 28;
        };
        UINT Value;
    };
} D3DKMT_WDDM_3_0_CAPS;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)


typedef struct _D3DKMT_TRACKEDWORKLOAD_SUPPORT
{
    _In_ UINT PhysicalAdapterIndex;
    _In_ DXGK_ENGINE_TYPE EngineType;
    _Out_ BOOL Support;
} D3DKMT_TRACKEDWORKLOAD_SUPPORT;

typedef struct _D3DKMT_NODEMETADATA
{
    _In_ UINT NodeOrdinalAndAdapterIndex;     // WDDMv2: High word is physical adapter index, low word is node ordinal
    _Out_ DXGK_NODEMETADATA NodeData;
} D3DKMT_NODEMETADATA;

typedef struct _D3DKMT_QUERYCLOCKCALIBRATION
{
    D3DKMT_HANDLE   hAdapter;               // in: The adapter to query clock information for
    UINT32          NodeOrdinal;            // in: Node ordinal of the requested engine.
    UINT32          PhysicalAdapterIndex;   // in: The physical adapter index, in an LDA chain
    DXGK_GPUCLOCKDATA ClockData;            // out: Output frequency, Gpu clock, and Cpu clock on the specified engine
} D3DKMT_QUERYCLOCKCALIBRATION;

typedef enum _DXGK_RENDER_PIPELINE_STAGE
{
    DXGK_RENDER_PIPELINE_STAGE_UNKNOWN = 0,
    DXGK_RENDER_PIPELINE_STAGE_INPUT_ASSEMBLER = 1,
    DXGK_RENDER_PIPELINE_STAGE_VERTEX_SHADER = 2,
    DXGK_RENDER_PIPELINE_STAGE_GEOMETRY_SHADER = 3,
    DXGK_RENDER_PIPELINE_STAGE_STREAM_OUTPUT = 4,
    DXGK_RENDER_PIPELINE_STAGE_RASTERIZER = 5,
    DXGK_RENDER_PIPELINE_STAGE_PIXEL_SHADER = 6,
    DXGK_RENDER_PIPELINE_STAGE_OUTPUT_MERGER = 7,
} DXGK_RENDER_PIPELINE_STAGE;

typedef enum _DXGK_PAGE_FAULT_FLAGS
{
    DXGK_PAGE_FAULT_WRITE                   = 0x1,  // when set, indicates that a write to a GPU virtual address failed
    DXGK_PAGE_FAULT_FENCE_INVALID           = 0x2,  // when set, indicates that a fence value for the packet that generated
                                                    // the page fault could not be determined
    DXGK_PAGE_FAULT_ADAPTER_RESET_REQUIRED  = 0x4,  // when set, indicates that the page fault put the GPU in a state that requires a full adapter reset
    DXGK_PAGE_FAULT_ENGINE_RESET_REQUIRED   = 0x8,  // when set, indicates that the page fault put the GPU in a state that requires GPU engine reset
    DXGK_PAGE_FAULT_FATAL_HARDWARE_ERROR    = 0x10, // when set, indicates that the system hardware, not just the GPU, cannot continue,
                                                    // and the OS should issue a bugcheck
    DXGK_PAGE_FAULT_IOMMU                   = 0x20, // when set, indicates that the faulting GPU virtual address was mapped using IoMmu,
                                                    // when not set, the faulting GPU virtual address was mapped using GPU memory management unit.

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

    DXGK_PAGE_FAULT_HW_CONTEXT_VALID        = 0x40, // when set, indicates that the faulting hardware queue was not identified, but faulting HW context was.
    DXGK_PAGE_FAULT_PROCESS_HANDLE_VALID    = 0x80, // when set, indicates that the faulting hardware queue or context was not identified,
                                                    // but process handle that submitted the faulted buffer was.

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

} DXGK_PAGE_FAULT_FLAGS;

typedef enum _DXGK_GENERAL_ERROR_CODE
{
    DXGK_GENERAL_ERROR_PAGE_FAULT = 0,
    DXGK_GENERAL_ERROR_INVALID_INSTRUCTION = 1,
} DXGK_GENERAL_ERROR_CODE;

typedef struct _DXGK_FAULT_ERROR_CODE
{
    union
    {
        struct
        {
            UINT IsDeviceSpecificCode                :  1;  // when set to 1, indicates the low 31 bits correspond to a device specific error code.
                                                            // when set to 0, indicates the low 31 bits correspond to a code defined by DXGK_GENERAL_ERROR_CODE.
            DXGK_GENERAL_ERROR_CODE GeneralErrorCode : 31;  // General error code defined by DXGK_GENERAL_ERROR_CODE.
        };
        struct
        {
            UINT IsDeviceSpecificCodeReservedBit     :  1;  // Same as IsDeviceSpecificCode member above:
                                                            // when set to 1, indicates the low 31 bits correspond to a device specific error code.
                                                            // when set to 0, indicates the low 31 bits correspond to a code defined by DXGK_GENERAL_ERROR_CODE.
            UINT DeviceSpecificCode                  : 31;  // A device specific error code.
        };
    };
} DXGK_FAULT_ERROR_CODE;

typedef struct _D3DKMT_DRIVERCAPS_EXT
{
    union
    {
        struct
        {
            UINT VirtualModeSupport     : 1;
            UINT Usb4MonitorSupport     : 1;
            UINT Reserved               : 30;
        };
        UINT Value;
    };
} D3DKMT_DRIVERCAPS_EXT;

#define DXGK_PRIMITIVE_API_SEQUENCE_NUMBER_UNKNOWN  ULONGLONG_MAX
#define DXGK_BIND_TABLE_ENTRY_UNKNOWN               UINT_MAX

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)


#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

typedef union _DXGK_MONITORLINKINFO_USAGEHINTS
{
    struct
    {
        UINT Hidden             : 1;    // 0x00000001
        UINT Reserved           :31;    // 0xFFFFFFFE
    };
    UINT Value;
} DXGK_MONITORLINKINFO_USAGEHINTS, *PDXGK_MONITORLINKINFO_USAGEHINTS;

typedef union _DXGK_MONITORLINKINFO_CAPABILITIES
{
    struct
    {
        UINT Stereo                     : 1;    // 0x00000001
        UINT WideColorSpace             : 1;    // 0x00000002
        UINT HighColorSpace             : 1;    // 0x00000004

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
        UINT DynamicColorSpace          : 1;    // 0x00000008
        UINT DynamicBitsPerColorChannel : 1;    // 0x00000010
        UINT DynamicColorEncodingFormat : 1;    // 0x00000020
        UINT DedicatedTimingGeneration  : 1;    // 0x00000040
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        UINT TargetIndependentPrimary   : 1;    // 0x00000080
        UINT SyncLockIdentical          : 1;    // 0x00000100
        UINT Hdr10Plus                  : 1;    // 0x00000200
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
        UINT DolbyVisionLowLatency      : 1;    // 0x00000400
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_5

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
        UINT VariableRefresh            : 1;    // 0x00000800
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6

#if   (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
        UINT Reserved                   :20;    // 0xFFFFF000
#elif (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
        UINT Reserved                   :21;    // 0xFFFFF800
#elif (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        UINT Reserved                   :22;    // 0xFFFFFC00
#elif (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
        UINT Reserved                   :25;    // 0xFFFFFF80
#else
        UINT Reserved                   :29;    // 0xFFFFFFF8
#endif // DXGKDDI_INTERFACE_VERSION
    };
    UINT Value;
} DXGK_MONITORLINKINFO_CAPABILITIES, *PDXGK_MONITORLINKINFO_CAPABILITIES;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)


#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if defined(__cplusplus) && !defined(SORTPP_PASS)

typedef enum _DXGK_DISPLAY_USAGE : BYTE
{
    DXGK_DU_INVALID     = 0,
    DXGK_DU_GENERIC     = 1,
    DXGK_DU_AR          = 2,
    DXGK_DU_VR          = 3,
    DXGK_DU_MEDICAL_IMAGING = 4,
    DXGK_DU_ACCESSORY   = 5,
    DXGK_DU_MAX         = 6
} DXGK_DISPLAY_USAGE, *PDXGK_DISPLAY_USAGE;

typedef enum _DXGK_DISPLAY_TECHNOLOGY : BYTE
{
    DXGK_DT_INVALID     = 0,
    DXGK_DT_OTHER       = 1,
    DXGK_DT_LCD         = 2,
    DXGK_DT_OLED        = 3,
    DXGK_DT_PROJECTOR   = 4,
    DXGK_DT_MAX         = 5
} DXGK_DISPLAY_TECHNOLOGY, *PDXGK_DISPLAY_TECHNOLOGY;

typedef enum _DXGK_DISPLAY_DESCRIPTOR_TYPE: BYTE
{
    DXGK_DDT_INVALID = 0,
    DXGK_DDT_EDID,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    DXGK_DDT_DISPLAYID,
#endif
} DXGK_DISPLAY_DESCRIPTOR_TYPE, *PDXGK_DISPLAY_DESCRIPTOR_TYPE;

#else
typedef BYTE DXGK_DISPLAY_USAGE;
typedef BYTE DXGK_DISPLAY_TECHNOLOGY;
typedef BYTE DXGK_DISPLAY_DESCRIPTOR_TYPE;
#endif // defined(__cplusplus) && !defined(SORTPP_PASS)

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Capabilities, preferences and other information reported by display only capable adapters.

typedef struct _D3DKMT_DISPLAY_CAPS
{
    union
    {
        struct
        {
            UINT64 PreferPhysicallyContiguous : 1;
            UINT64 Reserved : 63;
        };
        UINT64 Value;
    };
} D3DKMT_DISPLAY_CAPS;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)

#pragma pack( pop )

#endif // (NTDDI_VERSION >= NTDDI_LONGHORN) || defined(D3DKMDT_SPECIAL_MULTIPLATFORM_TOOL)

#pragma warning(pop)


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#endif /* _D3DKMDT_H */


