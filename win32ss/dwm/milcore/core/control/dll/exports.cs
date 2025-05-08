// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//     C# Front-End for the Graphics Control Debug Lib
//
//-----------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;
using System.Diagnostics.CodeAnalysis;
using Microsoft.Win32.SafeHandles;

[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "Microsoft.Windows.Media.MediaControl._pFile")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "Microsoft.Windows.Media.MediaControl.IFT(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "Microsoft.Windows.Media.MediaControl.FantScalerDisabled", MessageId = "Fant")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "Microsoft.Windows.Media.MediaControl.FantScalerDisabled", MessageId = "Scaler")]

namespace Microsoft.Windows.Media
{
    public class MediaControl
    {
        private IntPtr _pFile;

        private struct MediaControlFile
        {
            public UInt32 ShowDirtyRegionOverlay;
            public UInt32 ClearBackBufferBeforeRendering;
            public UInt32 DisableDirtyRegionSupport;
            public UInt32 EnableTranslucentRendering;
            public UInt32 FrameRate;
            public UInt32 DirtyRectAddRate;
            public UInt32 PercentElapsedTimeForComposition;

            public UInt32 TrianglesPerFrame;
            public UInt32 TrianglesPerFrameMax;
            public UInt32 TrianglesPerFrameCumulative;

            public UInt32 PixelsFilledPerFrame;
            public UInt32 PixelsFilledPerFrameMax;
            public UInt32 PixelsFilledPerFrameCumulative;

            public UInt32 TextureUpdatesPerFrame;
            public UInt32 TextureUpdatesPerFrameMax;
            public UInt32 TextureUpdatesPerFrameCumulative;

            public UInt32 VideoMemoryUsage;
            public UInt32 VideoMemoryUsageMin;
            public UInt32 VideoMemoryUsageMax;

            public UInt32 NumSoftwareRenderTargets;
            public UInt32 NumHardwareRenderTargets;

            // Provides a per-frame count of hw IRTs
            public UInt32 NumHardwareIntermediateRenderTargets;
            public UInt32 NumHardwareIntermediateRenderTargetsMax;

            // Provides a per-frame count of sw IRTs
            public UInt32 NumSoftwareIntermediateRenderTargets;
            public UInt32 NumSoftwareIntermediateRenderTargetsMax;

            public UInt32 AlphaEffectsDisabled;
            public UInt32 PrimitiveSoftwareFallbackDisabled;
            public UInt32 PurpleSoftwareFallback;
            public UInt32 FantScalerDisabled;
            public UInt32 Draw3DDisabled;
        }

        private sealed class MediaControlHandle : SafeHandle
        {
            internal MediaControlHandle()
                : base(IntPtr.Zero, true)
            {
            }

            public override bool IsInvalid
            {
                get
                {
                    return handle == IntPtr.Zero;
                }
            }

            protected override bool ReleaseHandle()
            {
                Imports.Release(handle);
                return true;
            }
        }

        private static class Imports
        {
            [DllImport("milctrl.dll", EntryPoint = "MediaControl_CanAttach")]
            internal static extern int CanAttach(
                [MarshalAs(UnmanagedType.LPWStr)] string sectionName,
                out bool canAccess);

            [DllImport("milctrl.dll", EntryPoint = "MediaControl_Attach")]
            internal static extern int Attach(
                [MarshalAs(UnmanagedType.LPWStr)] string sectionName,
                out MediaControlHandle debugControl);

            [DllImport("milctrl.dll", EntryPoint = "MediaControl_Release")]
            internal static extern void Release(
                IntPtr pMediaControl);

            [DllImport("milctrl.dll", EntryPoint = "MediaControl_GetDataPtr")]
            internal static extern int GetDataPtr(
                MediaControlHandle debugControl,
                out IntPtr value);
        }

        private MediaControlHandle _debugControl;

        /// <summary>
        /// Disallow the creation of a MediaControl instance.
        /// </summary>
        private MediaControl() { }

        /// <summary>
        /// Creates a MediaControl object attached to the specified 
        /// unmanaged debug control.
        /// </summary>
        private MediaControl(int processId)
        {
            //
            // *** ATTENTION    ATTENTION   ATTENTION   ATTENTION   ATTENTION ***
            //
            //  This name needs to match the one in core\uce\partitionmanager.cpp
            //
            // *** ATTENTION    ATTENTION   ATTENTION   ATTENTION   ATTENTION ***
            //

            string sectionName = "wpfgfx_v0400-" + processId.ToString(System.Globalization.CultureInfo.InvariantCulture);
            IFT(Imports.Attach(sectionName, out _debugControl));
            IFT(Imports.GetDataPtr(_debugControl, out _pFile));
        }

        /// <summary>
        /// Attaches to the specified process. Returns a
        /// MediaControl instance to control the specified 
        /// process.
        /// </summary>
        public static MediaControl Attach(int processId)
        {
            return new MediaControl(processId);
        }

        /// <summary>
        /// Checks if the debug control can be accessed against
        /// the processId.
        /// </summary>
        public static bool CanAttach(int processId)
        {
            bool canAccess;
            string sectionName = "MilCore-" + processId.ToString(System.Globalization.CultureInfo.InvariantCulture);
            IFT(Imports.CanAttach(sectionName, out canAccess));
            return canAccess;
        }

        public bool ShowDirtyRegionOverlay
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->ShowDirtyRegionOverlay != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->ShowDirtyRegionOverlay = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool ClearBackBufferBeforeRendering
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->ClearBackBufferBeforeRendering != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->ClearBackBufferBeforeRendering = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool DisableDirtyRegionSupport
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->DisableDirtyRegionSupport != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->DisableDirtyRegionSupport = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool EnableTranslucentRendering
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->EnableTranslucentRendering != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->EnableTranslucentRendering = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool AlphaEffectsDisabled
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->AlphaEffectsDisabled != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->AlphaEffectsDisabled = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool PrimitiveSoftwareFallbackDisabled
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->PrimitiveSoftwareFallbackDisabled != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->PrimitiveSoftwareFallbackDisabled = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool PurpleSoftwareFallback
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->PurpleSoftwareFallback != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->PurpleSoftwareFallback = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool FantScalerDisabled
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->FantScalerDisabled != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->FantScalerDisabled = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public bool Draw3DDisabled
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return pM->Draw3DDisabled != 0;
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->Draw3DDisabled = (UInt32)(value ? 1 : 0);
                }
            }
        }

        public int FrameRate
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->FrameRate);
                }
            }
        }

        public int DirtyRectAddRate
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->DirtyRectAddRate);
                }
            }
        }

        public int PercentElapsedTimeForComposition
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->PercentElapsedTimeForComposition);
                }
            }
        }

        public int TrianglesPerFrame
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TrianglesPerFrame);
                }
            }
        }

        public int TrianglesPerFrameMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TrianglesPerFrameMax);
                }
            }
        }

        public int TrianglesPerFrameCumulative
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TrianglesPerFrameCumulative);
                }
            }
        }

        public int PixelsFilledPerFrame
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->PixelsFilledPerFrame);
                }
            }
        }

        public int PixelsFilledPerFrameMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->PixelsFilledPerFrameMax);
                }
            }
        }

        public int PixelsFilledPerFrameCumulative
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->PixelsFilledPerFrameCumulative);
                }
            }
        }

        public int TextureUpdatesPerFrame
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TextureUpdatesPerFrame);
                }
            }
        }

        public int TextureUpdatesPerFrameMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TextureUpdatesPerFrameMax);
                }
            }
        }

        public int TextureUpdatesPerFrameCumulative
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->TextureUpdatesPerFrameCumulative);
                }
            }
        }

        public int VideoMemoryUsage
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->VideoMemoryUsage);
                }
            }
        }

        public int VideoMemoryUsageMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->VideoMemoryUsageMax);
                }
            }
        }

        public int VideoMemoryUsageMin
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->VideoMemoryUsageMin);
                }
            }
        }

        public int SoftwareRenderTargets
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->NumSoftwareRenderTargets);
                }
            }
        }

        public int HardwareRenderTargets
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->NumHardwareRenderTargets);
                }
            }
        }

        public int HardwareIntermediateRenderTargetsMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->NumHardwareIntermediateRenderTargetsMax);
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->NumHardwareIntermediateRenderTargetsMax = (UInt32)(value);
                }
            }
        }

        public int SoftwareIntermediateRenderTargetsMax
        {
            get
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    return (int)(pM->NumSoftwareIntermediateRenderTargetsMax);
                }
            }
            set
            {
                unsafe
                {
                    MediaControlFile* pM = (MediaControlFile*)(_pFile);
                    pM->NumSoftwareIntermediateRenderTargetsMax = (UInt32)(value);
                }
            }
        }

        /// <summary>
        /// Helper method that converts hresults into exceptions.
        /// (If Failed Throw).
        /// </summary>
        private static void IFT(int hResult)
        {
            if (hResult >= 0)
            {
                return;
            }
            else
            {
                Marshal.ThrowExceptionForHR(hResult);
            }
        }
    }
}

