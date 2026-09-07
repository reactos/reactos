// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       milexports.cs
//------------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Composition;
using System.Windows.Media.Imaging;
using System.Runtime.InteropServices;

using System.Security;
using System.Security.Permissions;
using MS.Internal.PresentationCore;

namespace MS.Internal
{
    #region MILRenderTargetBitmap

    internal static class MILRenderTargetBitmap
    {
        /// <SecurityNote>
        ///     Critical: Elevates to unmanagedcode permission
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint="MILRenderTargetBitmapGetBitmap")]//CASRemoval:
        internal static extern int /*HRESULT*/
            GetBitmap(
            SafeMILHandle /* IMILRenderTargetBitmap */ THIS_PTR,
            out BitmapSourceSafeMILHandle /* IWICBitmap */ ppIBitmap);

        [DllImport(DllImport.MilCore, EntryPoint = "MILRenderTargetBitmapClear")]
        internal static extern int /*HRESULT*/
            Clear(
            SafeMILHandle /* IMILRenderTargetBitmap */ THIS_PTR);
    }

    #endregion

    #region MILMedia

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [SuppressUnmanagedCodeSecurity, SecurityCritical(SecurityCriticalScope.Everything)]
    internal static class MILMedia
    {
        [DllImport(DllImport.MilCore, EntryPoint="MILMediaOpen")]
        internal static extern int /* HRESULT */ Open(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            [In, MarshalAs(UnmanagedType.BStr)] string /* LPOLESTR */ src
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaStop")]//CASRemoval:
        internal static extern int /* HRESULT */ Stop(
            SafeMediaHandle /* IMILMedia */ THIS_PTR
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaClose")]//CASRemoval:
        internal static extern int /*HRESULT */ Close(
            SafeMediaHandle /* IMILMedia */ THIS_PTR
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetPosition")]//CASRemoval:
        internal static extern int /* HRESULT */ GetPosition(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref long pllTime);

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaSetPosition")]//CASRemoval:
        internal static extern int /* HRESULT */ SetPosition(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            long llTime);

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaSetVolume")]//CASRemoval:
        internal static extern int /* HRESULT */ SetVolume(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            double dblVolume
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaSetBalance")]
        internal static extern int /* HRESULT */ SetBalance(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            double dblBalance
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaSetIsScrubbingEnabled")]
        internal static extern int /* HRESULT */ SetIsScrubbingEnabled(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            bool isScrubbingEnabled
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaIsBuffering")]//CASRemoval:
        internal static extern int /* HRESULT */ IsBuffering(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref bool pIsBuffering
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaCanPause")]//CASRemoval:
        internal static extern int /* HRESULT */ CanPause(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref bool pCanPause
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetDownloadProgress")]//CASRemoval:
        internal static extern int /* HRESULT */ GetDownloadProgress(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref double pProgress
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetBufferingProgress")]//CASRemoval:
        internal static extern int /* HRESULT */ GetBufferingProgress(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref double pProgress
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaSetRate")]//CASRemoval:
        internal static extern int /* HRESULT */ SetRate(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            double dblRate
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaHasVideo")]//CASRemoval:
        internal static extern int /* HRESULT */ HasVideo(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref bool pfHasVideo
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaHasAudio")]//CASRemoval:
        internal static extern int /* HRESULT */ HasAudio(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref bool pfHasAudio
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetNaturalHeight")]//CASRemoval:
        internal static extern int /* HRESULT */ GetNaturalHeight(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref UInt32 puiHeight
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetNaturalWidth")]//CASRemoval:
        internal static extern int /* HRESULT */ GetNaturalWidth(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref UInt32 puiWidth
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaGetMediaLength")]//CASRemoval:
        internal static extern int /* HRESULT */ GetMediaLength(
            SafeMediaHandle /* IMILMedia */ THIS_PTR,
            ref long pllLength
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaNeedUIFrameUpdate")]
        internal static extern int /* HRESULT */ NeedUIFrameUpdate(
            SafeMediaHandle /* IMILMedia */ THIS_PTR
            );

        [DllImport(DllImport.MilCore, EntryPoint="MILMediaShutdown")]//CASRemoval:
        internal static extern int /* HRESULT */ Shutdown(
            IntPtr /* IMILMedia */ THIS_PTR
            );

        [DllImport(DllImport.MilCore, EntryPoint = "MILMediaProcessExitHandler")]
        internal static extern int /*HRESULT*/ ProcessExitHandler(
            SafeMediaHandle /* IMILMedia */ THIS_PTR
            );
    }
    #endregion

    #region MILSwDoubleBufferedBitmap
    internal static class MILSwDoubleBufferedBitmap
    {
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint = "MILSwDoubleBufferedBitmapCreate")]
        internal static extern int /* HRESULT */ Create(
            uint width,
            uint height,
            double dpiX,
            double dpiY,
            ref Guid pixelFormatGuid,
            SafeMILHandle /* IWICPalette */ pPalette,
            out SafeMILHandle /* CSwDoubleBufferedBitmap */ ppSwDoubleBufferedBitmap);

        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint = "MILSwDoubleBufferedBitmapGetBackBuffer", PreserveSig = false)]
        internal static extern void GetBackBuffer(
            SafeMILHandle /* CSwDoubleBufferedBitmap */ THIS_PTR,
            out BitmapSourceSafeMILHandle /* IWICBitmap */ pBackBuffer,
            out uint pBackBufferSize
            );

        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint = "MILSwDoubleBufferedBitmapAddDirtyRect", PreserveSig = false)]
        internal static extern void AddDirtyRect(
            SafeMILHandle /* CSwDoubleBufferedBitmap */ THIS_PTR,
            ref Int32Rect dirtyRect
            );

        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint = "MILSwDoubleBufferedBitmapProtectBackBuffer")]
        internal static extern int /* HRESULT */ ProtectBackBuffer(
            SafeMILHandle /* CSwDoubleBufferedBitmap */ THIS_PTR
            );
    }
    #endregion

    #region MILUpdateSystemParametersInfo
    /// <summary>
    /// MILUpdateSystemParametersInfo
    /// </summary>
    internal static class MILUpdateSystemParametersInfo
    {
        /// <SecurityNote>
        ///  Critical: This code elevates to unmanaged Code permission
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport(DllImport.MilCore, EntryPoint="MILUpdateSystemParametersInfo")]
        internal static extern int /* HRESULT */
            Update();
    }
    #endregion

    #region WICPixelFormatGuids
    internal static class WICPixelFormatGUIDs
    {
        /* Undefined formats */
        internal static readonly Guid WICPixelFormatDontCare = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x00);

        /* Indexed formats */
        internal static readonly Guid WICPixelFormat1bppIndexed = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x01);
        internal static readonly Guid WICPixelFormat2bppIndexed = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x02);
        internal static readonly Guid WICPixelFormat4bppIndexed = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x03);
        internal static readonly Guid WICPixelFormat8bppIndexed = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x04);

        internal static readonly Guid WICPixelFormatBlackWhite = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x05);
        internal static readonly Guid WICPixelFormat2bppGray = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x06);
        internal static readonly Guid WICPixelFormat4bppGray = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x07);
        internal static readonly Guid WICPixelFormat8bppGray = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x08);

        /* sRGB formats (gamma is approx. 2.2) */
        /* For a full definition, see the sRGB spec */

        /* 16bpp formats */
        internal static readonly Guid WICPixelFormat16bppBGR555 = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x09);
        internal static readonly Guid WICPixelFormat16bppBGR565 = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0a);
        internal static readonly Guid WICPixelFormat16bppGray   = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0b);

        /* 24bpp formats */
        internal static readonly Guid WICPixelFormat24bppBGR = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0c);
        internal static readonly Guid WICPixelFormat24bppRGB = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0d);

        /* 32bpp format */
        internal static readonly Guid WICPixelFormat32bppBGR  = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0e);
        internal static readonly Guid WICPixelFormat32bppBGRA = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0f);
        internal static readonly Guid WICPixelFormat32bppPBGRA = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x10);
        internal static readonly Guid WICPixelFormat32bppGrayFloat  = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x11);

        /* scRGB formats. Gamma is 1.0 */
        /* For a full definition, see the scRGB spec */

        /* 32bpp format */
        internal static readonly Guid WICPixelFormat32bppBGR101010 = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x14);

        /* 48bpp format */
        internal static readonly Guid WICPixelFormat48bppRGB = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x15);

        /* 64bpp format */
        internal static readonly Guid WICPixelFormat64bppRGBA = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x16);
        internal static readonly Guid WICPixelFormat64bppPRGBA = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x17);

         /* Floating point scRGB formats */
        internal static readonly Guid WICPixelFormat128bppRGBAFloat = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x19);
        internal static readonly Guid WICPixelFormat128bppPRGBAFloat = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1a);
        internal static readonly Guid WICPixelFormat128bppRGBFloat = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1b);

         /* CMYK formats. */
        internal static readonly Guid WICPixelFormat32bppCMYK = new Guid(0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1c);
    }
    #endregion

    #region Guids
    internal static class MILGuidData
    {
        [SecurityCritical]
        static MILGuidData()
        {
        }

        [SecurityCritical]
        internal static readonly Guid IID_IMILRenderTargetBitmap = new Guid(0x00000201, 0xa8f2, 0x4877, 0xba, 0xa, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICPalette           = new Guid(0x00000040, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapSource      = new Guid(0x00000120, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICFormatConverter   = new Guid(0x00000301, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapScaler      = new Guid(0x00000302, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapClipper     = new Guid(0xE4FBCF03, 0x223D, 0x4e81, 0x93, 0x33, 0xD6, 0x35, 0x55, 0x6D, 0xD1, 0xB5);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapFlipRotator = new Guid(0x5009834F, 0x2D6A, 0x41ce, 0x9E, 0x1B, 0x17, 0xC5, 0xAF, 0xF7, 0xA7, 0x82);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmap            = new Guid(0x00000121, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapEncoder     = new Guid(0x00000103, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapFrameEncode = new Guid(0x00000105, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapDecoder     = new Guid(0x9EDDE9E7, 0x8DEE, 0x47ea, 0x99, 0xDF, 0xE6, 0xFA, 0xF2, 0xED, 0x44, 0xBF);

        [SecurityCritical]
        internal static readonly Guid IID_IWICBitmapFrameDecode = new Guid(0x3B16811B, 0x6A43, 0x4ec9, 0xA8, 0x13, 0x3D, 0x93, 0x0C, 0x13, 0xB9, 0x40);

        [SecurityCritical]
        internal static readonly Guid IID_IWICMetadataQueryReader = new Guid(0x30989668, 0xE1C9, 0x4597, 0xB3, 0x95, 0x45, 0x8E, 0xED, 0xB8, 0x08, 0xDF);

        [SecurityCritical]
        internal static readonly Guid IID_IWICMetadataQueryWriter = new Guid(0xA721791A, 0x0DEF, 0x4d06, 0xBD, 0x91, 0x21, 0x18, 0xBF, 0x1D, 0xB1, 0x0B);

        [SecurityCritical]
        internal static readonly Guid IID_IWICMetadataReader = new Guid(0x9204FE99, 0xD8FC, 0x4FD5, 0xA0, 0x01, 0x95, 0x36, 0xB0, 0x67, 0xA8, 0x99);

        [SecurityCritical]
        internal static readonly Guid IID_IWICMetadataWriter = new Guid(0xF7836E16, 0x3BE0, 0x470B, 0x86, 0xBB, 0x16, 0x0D, 0x0A, 0xEC, 0xD7, 0xDE);

        [SecurityCritical]
        internal static readonly Guid IID_IWICPixelFormatInfo = new Guid(0xE8EDA601, 0x3D48, 0x431a, 0xAB, 0x44, 0x69, 0x05, 0x9B, 0xE8, 0x8B, 0xBE);

        [SecurityCritical]
        internal static readonly Guid IID_IWICImagingFactory    = new Guid(0xec5ec8a9, 0xc395, 0x4314, 0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xff, 0x70);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICBmpDecoder       = new Guid(0x6b462062, 0x7cbf, 0x400d, 0x9f, 0xdb, 0x81, 0x3d, 0xd1, 0x0f, 0x27, 0x78);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICPngDecoder       = new Guid(0x389ea17b, 0x5078, 0x4cde, 0xb6, 0xef, 0x25, 0xc1, 0x51, 0x75, 0xc7, 0x51);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICIcoDecoder       = new Guid(0xc61bfcdf, 0x2e0f, 0x4aad, 0xa8, 0xd7, 0xe0, 0x6b, 0xaf, 0xeb, 0xcd, 0xfe);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICJpegDecoder      = new Guid(0x9456a480, 0xe88b, 0x43ea, 0x9e, 0x73, 0x0b, 0x2d, 0x9b, 0x71, 0xb1, 0xca);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICGifDecoder       = new Guid(0x381dda3c, 0x9ce9, 0x4834, 0xa2, 0x3e, 0x1f, 0x98, 0xf8, 0xfc, 0x52, 0xbe);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICTiffDecoder      = new Guid(0xb54e85d9, 0xfe23, 0x499f, 0x8b, 0x88, 0x6a, 0xce, 0xa7, 0x13, 0x75, 0x2b);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICWmpDecoder       = new Guid(0xa26cec36, 0x234c, 0x4950, 0xae, 0x16, 0xe3, 0x4a, 0xac, 0xe7, 0x1d, 0x0d);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICBmpEncoder       = new Guid(0x69be8bb4, 0xd66d, 0x47c8, 0x86, 0x5a, 0xed, 0x15, 0x89, 0x43, 0x37, 0x82);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICPngEncoder       = new Guid(0x27949969, 0x876a, 0x41d7, 0x94, 0x47, 0x56, 0x8f, 0x6a, 0x35, 0xa4, 0xdc);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICJpegEncoder      = new Guid(0x1a34f5c1, 0x4a5a, 0x46dc, 0xb6, 0x44, 0x1f, 0x45, 0x67, 0xe7, 0xa6, 0x76);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICGifEncoder       = new Guid(0x114f5598, 0x0b22, 0x40a0, 0x86, 0xa1, 0xc8, 0x3e, 0xa4, 0x95, 0xad, 0xbd);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICTiffEncoder      = new Guid(0x0131be10, 0x2001, 0x4c5f, 0xa9, 0xb0, 0xcc, 0x88, 0xfa, 0xb6, 0x4c, 0xe8);

        [SecurityCritical]
        internal static readonly Guid CLSID_WICWmpEncoder       = new Guid(0xac4ce3cb, 0xe1c1, 0x44cd, 0x82, 0x15, 0x5a, 0x16, 0x65, 0x50, 0x9e, 0xc2);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatBmp   = new Guid(0x0af1d87e, 0xfcfe, 0x4188, 0xbd, 0xeb, 0xa7, 0x90, 0x64, 0x71, 0xcb, 0xe3);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatIco   = new Guid(0xa3a860c4, 0x338f, 0x4c17, 0x91, 0x9a, 0xfb, 0xa4, 0xb5, 0x62, 0x8f, 0x21);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatGif   = new Guid(0x1f8a5601, 0x7d4d, 0x4cbd, 0x9c, 0x82, 0x1b, 0xc8, 0xd4, 0xee, 0xb9, 0xa5);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatJpeg  = new Guid(0x19e4a5aa, 0x5662, 0x4fc5, 0xa0, 0xc0, 0x17, 0x58, 0x02, 0x8e, 0x10, 0x57);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatPng   = new Guid(0x1b7cfaf4, 0x713f, 0x473c, 0xbb, 0xcd, 0x61, 0x37, 0x42, 0x5f, 0xae, 0xaf);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatTiff  = new Guid(0x163bcc30, 0xe2e9, 0x4f0b, 0x96, 0x1d, 0xa3, 0xe9, 0xfd, 0xb7, 0x88, 0xa3);

        [SecurityCritical]
        internal static readonly Guid GUID_ContainerFormatWmp   = new Guid(0x57a37caa, 0x367a, 0x4540, 0x91, 0x6b, 0xf1, 0x83, 0xc5, 0x09, 0x3a, 0x4b);

        [SecurityCritical]
        internal static readonly byte[] GUID_VendorMicrosoft = new byte[] {  0xca,  0x49, 0xe7, 0xf0, 0xef, 0xed, 0x89, 0x45, 0xa7, 0x3a, 0xee, 0xe, 0x62, 0x6a, 0x2a, 0x2b };
    }
    #endregion // Guids
}



