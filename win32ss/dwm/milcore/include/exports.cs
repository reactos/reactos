// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// File: exports.cs
//
// Description:
//     Managed exports from MIL core.
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Threading;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Effects;
using System.Windows.Media.Media3D;
using System.Runtime.InteropServices;
using System.Windows.Media.Animation;
using MS.Internal;
using MS.Internal.PresentationCore;
using MS.Internal.Interop;
using MS.Utility;
using MS.Win32;
using System.Diagnostics;
using System.Collections.Generic;
using System.Security;
using System.Security.Permissions;
using Microsoft.Internal;
using Microsoft.Win32.SafeHandles;

using UnsafeNativeMethods=MS.Win32.PresentationCore.UnsafeNativeMethods;
using SafeNativeMethods=MS.Win32.PresentationCore.SafeNativeMethods;
using HRESULT=MS.Internal.HRESULT;
using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;
using DllImport=MS.Internal.PresentationCore.DllImport;

/*
 *
 * For the time being the flat composition api will be implemented on
 * top of milcore.h (the protocol api), this is only temporary we will
 * be adding type safe unmanaged exports to milcore.dll which will be
 * be called from the static functions.
 *
 */


namespace System.Windows.Media.Composition
{

    // enumeration of marshal types supported by the transport.
    internal enum ChannelMarshalType
    {
        ChannelMarshalTypeInvalid = 0x0,
        ChannelMarshalTypeSameThread = 0x1,
        ChannelMarshalTypeCrossThread = 0x2
    };

    /// <summary>
    /// Lock replacement till the CLR team gives us support to resolve the reentrancy issues
    /// with the CLR lock.
    /// </summary>
    /// <remark>
    /// Prefered usage pattern:
    ///
    /// <code>
    ///    using (CompositionEngineLock.Acquire())
    ///    {
    ///        ...
    ///    }
    /// </code>
    ///
    /// If you don't use this pattern remember to have a try finally block to release the lock
    /// in the event of an exception.
    /// </remark>
    internal struct CompositionEngineLock : IDisposable
    {
        /// <summary>
        /// Aquires the composition engine lock.
        /// </summary>
        /// <SecurityNote>
        /// Critical - calls unmanaged code
        /// TreatAsSafe - all inputs validated, locking is safe
        /// </SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        internal static CompositionEngineLock Acquire()
        {
            UnsafeNativeMethods.MilCoreApi.EnterCompositionEngineLock();

            return new CompositionEngineLock();
        }

        /// <summary>
        /// Releases the composition engine lock. Using Dispose enables the using syntax.
        /// </summary>
        /// <SecurityNote>
        /// Critical - calls unmanaged code
        /// TreatAsSafe - all inputs validated, unlocking is safe
        /// </SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        public void Dispose()
        {
            UnsafeNativeMethods.MilCoreApi.ExitCompositionEngineLock();
        }
    }


    /// <summary>
    /// The following class only exists to clearly separate the DUCE APIs
    /// from the legacy resource APIs.
    /// </summary>
    internal partial class DUCE
    {
        /// <summary>
        /// CopyBytes - Poor-man's mem copy.  Copies cbData from pbFrom to pbTo.
        /// pbFrom and pbTo must be DWORD aligned, and cbData must be a multiple of 4.
        /// </summary>
        /// <param name="pbTo"> byte* pointing to the "to" array.  Must be DWORD aligned. </param>
        /// <param name="pbFrom"> byte* pointing to the "from" array.  Must be DWORD aligned. </param>
        /// <param name="cbData"> int - count of bytes to copy.  Must be a multiple of 4. </param>
        /// <SecurityNote>
        ///     Critical: This code accesses an unsafe code block
        /// </SecurityNote>
        [SecurityCritical]
        internal static unsafe void CopyBytes(byte* pbTo,
                                              byte* pbFrom,
                                              int cbData)
        {
            // We'd like to only handle QWORD aligned data, but the CLR can't enforce this.
            // If there's no data to copy, it's ok if the pointers aren't aligned
            Debug.Assert((cbData == 0) || ((Int64)(IntPtr)pbFrom) % 4 == 0);
            Debug.Assert((cbData == 0) || ((Int64)(IntPtr)pbTo) % 4 == 0);
            Debug.Assert(cbData % 4 == 0);
            Debug.Assert(cbData >= 0);

            Int32* pCurFrom32 = (Int32*)pbFrom;
            Int32* pCurTo32 = (Int32*)pbTo;

            for (int i = 0; i < cbData / 4; i++)
            {
                pCurTo32[i] = pCurFrom32[i];
            }
        }

        /// <SecurityNote>
        /// Critical - pinvoke wrappers
        /// </SecurityNote>
        [SecurityCritical(SecurityCriticalScope.Everything), SuppressUnmanagedCodeSecurity]
        private static class UnsafeNativeMethods
        {
            [DllImport(DllImport.MilCore)]
            internal static extern /*HRESULT*/ int MilResource_CreateOrAddRefOnChannel(
                IntPtr pChannel,
                DUCE.ResourceType resourceType,
                ref DUCE.ResourceHandle hResource
                );

            [DllImport(DllImport.MilCore)]
            internal static extern /*HRESULT*/ int MilResource_DuplicateHandle(
                IntPtr pSourceChannel,
                DUCE.ResourceHandle original,
                IntPtr pTargetChannel,
                ref DUCE.ResourceHandle duplicate
                );

            [DllImport(DllImport.MilCore, EntryPoint = "MilConnection_CreateChannel")]//CASRemoval:
            internal static extern int MilConnection_CreateChannel(
                IntPtr pTransport,
                IntPtr hChannel,
                out IntPtr channelHandle);

            [DllImport(DllImport.MilCore, EntryPoint = "MilConnection_DestroyChannel")]//CASRemoval:
            internal static extern int MilConnection_DestroyChannel(
                IntPtr channelHandle);

            [DllImport(DllImport.MilCore, EntryPoint = "MilChannel_CloseBatch")]//CASRemoval:
            internal static extern int MilConnection_CloseBatch(
                IntPtr channelHandle);

            [DllImport(DllImport.MilCore, EntryPoint = "MilChannel_CommitChannel")]//CASRemoval:
            internal static extern int MilConnection_CommitChannel(
                IntPtr channelHandle);

            [DllImport(DllImport.MilCore)]//CASRemoval:
            internal static extern int WgxConnection_SameThreadPresent(
                IntPtr pConnection);

            [DllImport(DllImport.MilCore, EntryPoint = "MilChannel_GetMarshalType")]
            internal static extern int MilChannel_GetMarshalType(IntPtr channelHandle, out ChannelMarshalType marshalType);

            [DllImport (DllImport.MilCore, EntryPoint = "MilResource_SendCommand")]//CASRemoval:
            unsafe internal static extern int MilResource_SendCommand(
                byte *pbData,
                uint cbSize,
                bool sendInSeparateBatch,
                IntPtr pChannel);

            [DllImport (DllImport.MilCore, EntryPoint = "MilChannel_BeginCommand")]//CASRemoval:
            unsafe internal static extern int MilChannel_BeginCommand(
                IntPtr pChannel,
                byte *pbData,
                uint cbSize,
                uint cbExtra
                );

            [DllImport (DllImport.MilCore, EntryPoint = "MilChannel_AppendCommandData")]//CASRemoval:
            unsafe internal static extern int MilChannel_AppendCommandData(
                IntPtr pChannel,
                byte *pbData,
                uint cbSize
                );

            [DllImport (DllImport.MilCore, EntryPoint = "MilChannel_EndCommand")]//CASRemoval:
            unsafe internal static extern int MilChannel_EndCommand(
                IntPtr pChannel);

            [DllImport (DllImport.MilCore, EntryPoint = "MilResource_SendCommandMedia")]//CASRemoval:
            unsafe internal static extern int MilResource_SendCommandMedia(
                ResourceHandle handle,
                SafeMediaHandle pMedia,
                IntPtr pChannel,
                bool  notifyUceDirect
                );

            [DllImport (DllImport.MilCore, EntryPoint = "MilResource_SendCommandBitmapSource")]//CASRemoval:
            unsafe internal static extern int MilResource_SendCommandBitmapSource(
                ResourceHandle handle,
                BitmapSourceSafeMILHandle /* IWICBitmapSource */ pBitmapSource,
                IntPtr pChannel);

            [DllImport(DllImport.MilCore, EntryPoint = "MilResource_ReleaseOnChannel")]//CASRemoval:
            internal static extern /*HRESULT*/ int MilResource_ReleaseOnChannel(
                IntPtr pChannel,
                DUCE.ResourceHandle hResource,
                out int deleted
                );

            [DllImport(DllImport.MilCore)]
            internal static extern int MilChannel_SetNotificationWindow(
                IntPtr pChannel,
                IntPtr hwnd,
                WindowMessage message
                );

            [DllImport(DllImport.MilCore)]
            internal static extern int MilComposition_WaitForNextMessage(
                IntPtr pChannel,
                int nCount,
                IntPtr[] handles,
                int bWaitAll,
                UInt32 waitTimeout,
                out int waitReturn
                );

            [DllImport(DllImport.MilCore)]
            internal static extern int MilComposition_PeekNextMessage(
                IntPtr pChannel,
                out MilMessage.Message message,
                /* size_t */ IntPtr messageSize,
                out int messageRetrieved
                );

            [DllImport(DllImport.MilCore, EntryPoint = "MilResource_GetRefCountOnChannel")]
            internal static extern /*HRESULT*/ int MilResource_GetRefCountOnChannel(
                IntPtr pChannel,
                DUCE.ResourceHandle hResource,
                out uint refCount
                );                
        }

        /// <summary>
        /// Define the value of an infinte wait in WaitForNextMessage.
        /// </summary>
        internal const UInt32 waitInfinite = UInt32.MaxValue;

        internal static class MilMessage
        {
            /// <summary>
            /// The ID of each type of back-channel notification messages.
            /// </summary>
            internal enum Type
            {
                Invalid             = 0x00,

                SyncFlushReply      = 0x01,
                Caps                = 0x04,
                PartitionIsZombie   = 0x06,
                SyncModeStatus      = 0x09,
                Presented           = 0x0A,
                BadPixelShader      = 0x10,

                ForceDWORD          = unchecked((int)0xffffffff)
            };

            [StructLayout(LayoutKind.Explicit, Pack = 1)]
            internal struct CapsData
            {
                [FieldOffset(0)] internal Int32 CommonMinimumCaps;
                [FieldOffset(4)] internal UInt32 DisplayUniqueness;
                [FieldOffset(8)] internal MilGraphicsAccelerationCaps Caps;
            };

            [StructLayout(LayoutKind.Explicit, Pack = 1)]
            internal struct PartitionIsZombieStatus
            {
                [FieldOffset(0)] internal int HRESULTFailureCode;
            };

            [StructLayout(LayoutKind.Explicit, Pack = 1)]
            internal struct SyncModeStatus
            {
                [FieldOffset(0)] internal int Enabled;
            };

            [StructLayout(LayoutKind.Explicit, Pack = 1)]
            internal struct Presented
            {
                [FieldOffset(0)] internal MIL_PRESENTATION_RESULTS PresentationResults;
                [FieldOffset(4)] internal int RefreshRate;
                [FieldOffset(8)] internal long PresentationTime;
            };

            /// <summary>
            /// The union of all known back-channel notification messages.
            /// </summary>
            [StructLayout(LayoutKind.Explicit, Pack = 1)]
            internal struct Message
            {
                [FieldOffset(0)] internal Type Type;
                [FieldOffset(4)] internal int Reserved;
                [FieldOffset(8)] internal CapsData Caps;
                [FieldOffset(8)] internal PartitionIsZombieStatus HRESULTFailure;
                [FieldOffset(8)] internal Presented Presented;
                [FieldOffset(8)] internal SyncModeStatus SyncModeStatus;
            };
        }

        ///<summary>
        /// A channel set is a container for a matched pair of channels,
        /// one primary channel and out of band channel
        ///</summary>
        internal struct ChannelSet
        {
            internal Channel Channel;
            internal Channel OutOfBandChannel;
        }

        /// <summary>
        /// A Channel is a command pipe into a composition device.
        /// The commands send through a Channel are not executed till
        /// Channel.Commit is called. Committing a Channel is an atomic operation. In
        /// other words, all the commands are executed before the next frame is
        /// rendered.
        ///
        /// A channel is also a hard boundary for UCE resources. That means that UCE
        /// resources created on one channel can not interact with resources on a different
        /// channel.
        /// </summary>
        internal sealed partial class Channel
        #if ENFORCE_CHANNEL_THREAD_ACCESS
            : System.Windows.Threading.DispatcherObject
            // "Producer" operations - adding commands et al. - should only be done
            // on the thread that created the channel.  These operations are on the
            // hot path, so we don't add the cost of enforcement.  To detect
            // violations (which can lead to render-thread failures that
            // are very difficult to diagnose), build
            // PresentationCore with ENFORCE_CHANNEL_THREAD_ACCESS defined.
        #endif
        {
            /// <summary>
            /// Primary channel.
            /// </summary>
            /// <SecurityNote>
            /// Critical - Track usage of the channel pointer.
            /// </SecurityNote>
            [SecurityCritical]
            IntPtr _hChannel;

            private Channel _referenceChannel;
            private bool _isSynchronous;
            private bool _isOutOfBandChannel;

            IntPtr _pConnection;

            /// <summary>
            /// Creates a channel and associates it with channel group (partition).
            /// New create channel will belong to the same partition as the given referenceChannel.
            /// To create the very first channel in the group, use null argument.
            /// </summary>
            /// <SecurityNote>
            /// Critical - accesses critical resources (handles)
            /// </SecurityNote>
            [SecurityCritical]
            public Channel(Channel referenceChannel, bool isOutOfBandChannel, IntPtr pConnection, bool isSynchronous)
            {
                IntPtr referenceChannelHandle = IntPtr.Zero;

                _referenceChannel = referenceChannel;
                _pConnection = pConnection;
                _isOutOfBandChannel = isOutOfBandChannel;
                _isSynchronous = isSynchronous;

                if (referenceChannel != null)
                {
                    referenceChannelHandle = referenceChannel._hChannel;
                }

                HRESULT.Check(UnsafeNativeMethods.MilConnection_CreateChannel(
                    _pConnection,
                    referenceChannelHandle,
                    out _hChannel));

            }


            /// <summary>
            /// Commits the commands enqueued into the Channel.
            /// </summary>
            /// <SecurityNote>
            ///    Critical: This code calls into MilConnection_CommitChannel which causes an elevation
            ///    TreatAsSafe: This commits operations to the channel. Committing to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal void Commit()
            {
                if (_hChannel == IntPtr.Zero)
                {
                    //
                    // If the channel has been closed, fail silently. This could happen
                    // for the service channel if we are in disconnected state when more
                    // that one media contexts are present and not all of them have finished
                    // processing the disconnect messages.
                    //

                    return;
                }

                HRESULT.Check(UnsafeNativeMethods.MilConnection_CommitChannel(
                   _hChannel));
            }

            /// <summary>
            /// Closes the current batch on the Channel.
            /// </summary>
            /// <SecurityNote>
            ///    Critical: This code calls into MilConnection_CloseBatch which causes an elevation
            ///    TreatAsSafe: Closing a batch is safe and nothing is exposed. Batches are in the 
            ///                 render thread, and can only be written to from the UI thread while
            ///                 they're open using other SC/STAS methods on DUCE.Channel. Once closed,
            ///                 the only operation that can be done on a batch is Channel.Commit.
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal void CloseBatch()
            {
                if (_hChannel == IntPtr.Zero)
                {
                    //
                    // If the channel has been closed, fail silently. This could happen
                    // for the service channel if we are in disconnected state when more
                    // that one media contexts are present and not all of them have finished
                    // processing the disconnect messages.
                    //

                    return;
                }

                HRESULT.Check(UnsafeNativeMethods.MilConnection_CloseBatch(
                   _hChannel));
            }

            /// <summary>
            ///   Flush the currently recorded commands to the target device and prepare
            ///   to receive new commands. Block until last command was executed.
            /// </summary>
            /// <SecurityNote>
            ///   Critical - This code calls into MilComposition_SyncFlush which causes an elevation.
            ///   TreatAsSafe - The net effect is to wait until render completes.
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal void SyncFlush()
            {
                if (_hChannel == IntPtr.Zero)
                {
                    //
                    // If the channel has been closed, fail silently. This could happen
                    // for the service channel if we are in disconnected state whhen more
                    // that one media contexts are present and not all of them have finished
                    // processing the disconnect messages.
                    //

                    return;
                }

                HRESULT.Check(MilCoreApi.MilComposition_SyncFlush(_hChannel));

            }

            /// <summary>
            /// Commits the channel and then closes it.
            /// </summary>
            /// <SecurityNote>
            /// Critical - This code calls into MilConnection_CommitChannel and
            ///            MilConnection_DestroyChannel which causes an elevation.
            /// TreatAsSafe - Even if called prematurely, this will simply make all the subsequent
            ///               channel operations fail (whether silently or by raising an exception).
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal void Close()
            {
                if (_hChannel != IntPtr.Zero)
                {
                    HRESULT.Check(UnsafeNativeMethods.MilConnection_CloseBatch(_hChannel));
                    HRESULT.Check(UnsafeNativeMethods.MilConnection_CommitChannel(_hChannel));
                }

                _referenceChannel = null;

                if (_hChannel != IntPtr.Zero)
                {
                    HRESULT.Check(UnsafeNativeMethods.MilConnection_DestroyChannel(_hChannel));

                    _hChannel = IntPtr.Zero;
                }
            }

            /// <summary>
            /// Commits the commands enqueued into the Channel.
            /// </summary>
            /// <SecurityNote>
            /// Critical    -- Calls into MilChannel_Present which causes an elevation.
            /// TreatAsSafe -- This call is only relevant to synchronous channels and causes the compositor
            ///                associated with the synchronous channel to compose and present, which is
            ///                considered safe. Asynchronous channels no-op this call.
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal void Present()
            {
                HRESULT.Check(UnsafeNativeMethods.WgxConnection_SameThreadPresent(_pConnection));
            }

            /// <summary>
            /// Internal only: CreateOrAddRefOnChannel addrefs the resource corresponding to the
            /// specified handle on the channel.
            /// </summary>
            /// <return>
            /// Returns true iff the resource was created on the channel. The caller is responsible to
            /// update the resource appropriately.
            /// </return>
            /// <SecurityNote>
            /// Critical - Calls into MilResource_CreateOrAddRefOnChannel which causes an elevation.
            /// TreatAsSafe - All inputs are safe wrappers, manipulating handle on the channel
            /// will only affect resources that belong to the current process.
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal bool CreateOrAddRefOnChannel(object instance, ref DUCE.ResourceHandle handle, DUCE.ResourceType resourceType)
            {
                bool handleNeedsCreation = handle.IsNull;

                Invariant.Assert(_hChannel != IntPtr.Zero);

                HRESULT.Check(UnsafeNativeMethods.MilResource_CreateOrAddRefOnChannel(
                    _hChannel,
                    resourceType,
                    ref handle
                    ));

                if (EventTrace.IsEnabled(EventTrace.Keyword.KeywordGraphics | EventTrace.Keyword.KeywordPerf, EventTrace.Level.PERF_LOW))
                {
                    EventTrace.EventProvider.TraceEvent(EventTrace.Event.CreateOrAddResourceOnChannel, EventTrace.Keyword.KeywordGraphics | EventTrace.Keyword.KeywordPerf, EventTrace.Level.PERF_LOW, PerfService.GetPerfElementID(instance), _hChannel, (uint) handle, (uint) resourceType); 
                }

                return handleNeedsCreation;
            }

            /// <summary>
            /// DuplicateHandle attempts to duplicate a handle from one channel to another.
            /// Naturally, this can only work if both the source and target channels are
            /// within the same partition.
            /// </summary>
            /// <remarks>
            /// It is the responsibility of the caller to commit the source channel
            /// to assure that duplication took place.
            /// tables.
            /// </remarks>
            /// <return>
            /// Returns the duplicated handle (valid on the target channel) or the null
            /// handle if duplication failed.
            /// </return>
            /// <SecurityNote>
            ///     Critical - Calls security critical code.
            ///     TreatAsSafe - All inputs are safe wrappers, manipulating handle on the channel
            ///                   will only affect resources that belong to the current process.
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal DUCE.ResourceHandle DuplicateHandle(
                DUCE.ResourceHandle original,
                DUCE.Channel targetChannel
                )
            {
                DUCE.ResourceHandle duplicate = DUCE.ResourceHandle.Null;

                //Debug.WriteLine(string.Format("DuplicateHandle: Channel: {0}, Resource: {1}, Target channel: {2},  ", _hChannel, original._handle, targetChannel));

                HRESULT.Check(UnsafeNativeMethods.MilResource_DuplicateHandle(
                    _hChannel,
                    original,
                    targetChannel._hChannel,
                    ref duplicate
                    ));

                return duplicate;
            }


            /// <summary>
            /// Internal only: ReleaseOnChannel releases the resource corresponding to the specified
            /// handle on the channel.
            /// </summary>
            /// <return>
            /// Returns true iff the resource is not on this channel anymore.
            /// </return>
            /// <SecurityNote>
            /// Critical - Calls security critical code.
            /// TreatAsSafe - All inputs are safe wrappers, manipulating handle on the channel
            /// will only affect resources that belong to the current process.
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal bool ReleaseOnChannel(DUCE.ResourceHandle handle)
            {
                Invariant.Assert(_hChannel != IntPtr.Zero);
                Debug.Assert(!handle.IsNull);

                //Debug.WriteLine(string.Format("ReleaseOnChannel: Channel: {0}, Resource: {1}", _hChannel, handle._handle));

                int releasedOnChannel;

                HRESULT.Check(UnsafeNativeMethods.MilResource_ReleaseOnChannel(
                    _hChannel,
                    handle,
                    out releasedOnChannel
                    ));

                if ((releasedOnChannel != 0) && EventTrace.IsEnabled(EventTrace.Keyword.KeywordGraphics | EventTrace.Keyword.KeywordPerf, EventTrace.Level.PERF_LOW))
                {
                    EventTrace.EventProvider.TraceEvent(EventTrace.Event.ReleaseOnChannel, EventTrace.Keyword.KeywordGraphics | EventTrace.Keyword.KeywordPerf, EventTrace.Level.PERF_LOW, _hChannel, (uint) handle); 
                }

                return (releasedOnChannel != 0);
            }

            /// <summary>
            /// Internal only: GetRefCount returns the reference count of a resource 
            /// corresponding to the specified handle on the channel.
            /// </summary>
            /// <return>
            /// Returns the ref count for a resource on this channel.
            /// </return>
            /// <SecurityNote>
            /// Critical - Calls security critical code.
            /// TreatAsSafe - All inputs are safe wrappers, manipulating handle on the channel
            /// will only affect resources that belong to the current process.
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal uint GetRefCount(DUCE.ResourceHandle handle)
            {
                Invariant.Assert(_hChannel != IntPtr.Zero);
                Debug.Assert(!handle.IsNull);

                uint refCount;

                HRESULT.Check(UnsafeNativeMethods.MilResource_GetRefCountOnChannel(
                    _hChannel,
                    handle,
                    out refCount
                    ));

                return refCount;
            }


            /// <summary>
            /// IsConnected returns true if the channel is connected.
            /// </summary>
            ///<SecurityNote>
            /// Critical - this code performs an elevation.
            /// TreatAsSafe - it's safe to return whether a channel is connected or not.
            ///</SecurityNote>
            internal bool IsConnected
            {
                [SecurityCritical, SecurityTreatAsSafe]
                get
                {
                    return MediaContext.CurrentMediaContext.IsConnected;
                }
            }

            /// <summary>
            /// MarshalType returns the marshal type of the channel.
            /// </summary>
            ///<SecurityNote>
            /// Critical - this code performs an elevation.
            /// TreatAsSafe - it's safe to return a channel's marshal type.
            ///</SecurityNote>
            internal ChannelMarshalType MarshalType
            {
                [SecurityCritical, SecurityTreatAsSafe]
                get
                {
                    Invariant.Assert(_hChannel != IntPtr.Zero);

                    ChannelMarshalType marshalType;
                    HRESULT.Check(UnsafeNativeMethods.MilChannel_GetMarshalType(
                        _hChannel,
                        out marshalType
                        ));

                    return marshalType;
                }
            }

            /// <summary>
            /// Returns whether the given channel is synchronous.
            /// </summary>
            internal bool IsSynchronous
            {
                get
                {
                    return _isSynchronous;
                }
            }

            /// <summary>
            /// Returns whether the given channel is an out of band channel.
            /// </summary>
            internal bool IsOutOfBandChannel
            {
                get
                {
                    return _isOutOfBandChannel;
                }
            }

            /// <summary>
            /// SendCommand sends a command struct through the composition thread.
            /// </summary>
            /// <SecurityNote>
            /// Critical - This code accepts a raw pointer and calls other SecurityCritical code.
            /// </SecurityNote>
            [SecurityCritical]
            unsafe internal void SendCommand(
                byte *pCommandData,
                int cSize)
            {
                SendCommand(pCommandData, cSize, false);
            }

            /// <summary>
            /// SendCommand sends a command struct through the composition thread. The
            /// sendInSeparateBatch parameter determines whether the command is sent in the
            /// current open batch, or whether it will be added to a new and separate batch
            /// which is then immediately closed, leaving the current batch untouched.
            /// </summary>
            /// <SecurityNote>
            /// Critical - This code accepts a raw pointer and calls native code under SUC.
            /// </SecurityNote>
            [SecurityCritical]
            unsafe internal void SendCommand(
                byte *pCommandData,
                int cSize,
                bool sendInSeparateBatch)
            {
                #if ENFORCE_CHANNEL_THREAD_ACCESS
                VerifyAccess();
                #endif

                checked
                {
                    Invariant.Assert(pCommandData != (byte*)0 && cSize > 0);

                    int hr = HRESULT.S_OK;

                    if (_hChannel == IntPtr.Zero)
                    {
                        //
                        // If the channel has been closed, fail silently. This could happen
                        // for the service channel if we are in disconnected state when more
                        // that one media contexts are present and not all of them have finished
                        // processing the disconnect messages.
                        //

                        return;
                    }

                    hr = UnsafeNativeMethods.MilResource_SendCommand(
                        pCommandData,
                        (uint)cSize,
                        sendInSeparateBatch,
                        _hChannel);

                    HRESULT.Check(hr);
                }
            }

            /// <summary>
            /// BeginCommand opens a command on a channel
            /// </summary>
            /// <SecurityNote>
            /// Critical - This code accepts a raw pointer and calls native code under SUC.
            /// </SecurityNote>
            [SecurityCritical]
            unsafe internal void BeginCommand(
                byte *pbCommandData,
                int cbSize,
                int cbExtra)
            {
                #if ENFORCE_CHANNEL_THREAD_ACCESS
                VerifyAccess();
                #endif

                checked
                {
                    Invariant.Assert(cbSize > 0);

                    int hr = HRESULT.S_OK;

                    if (_hChannel == IntPtr.Zero)
                    {
                        //
                        // If the channel has been closed, fail silently. This could happen
                        // for the service channel if we are in disconnected state whhen more
                        // that one media contexts are present and not all of them have finished
                        // processing the disconnect messages.
                        //

                        return;
                    }

                    hr = UnsafeNativeMethods.MilChannel_BeginCommand(
                        _hChannel,
                        pbCommandData,
                        (uint)cbSize,
                        (uint)cbExtra
                        );

                    HRESULT.Check(hr);
                }
            }

            /// <summary>
            /// AppendCommandData appends data to an open command on a channel
            /// </summary>
            /// <SecurityNote>
            /// Critical - This code accepts a raw pointer and calls native code under SUC.
            /// </SecurityNote>
            [SecurityCritical]
            unsafe internal void AppendCommandData(
                byte *pbCommandData,
                int cbSize)
            {
                checked
                {
                    Invariant.Assert(pbCommandData != (byte*)0 && cbSize > 0);

                    int hr = HRESULT.S_OK;

                    if (_hChannel == IntPtr.Zero)
                    {
                        //
                        // If the channel has been closed, fail silently. This could happen
                        // for the service channel if we are in disconnected state whhen more
                        // that one media contexts are present and not all of them have finished
                        // processing the disconnect messages.
                        //

                        return;
                    }

                    hr = UnsafeNativeMethods.MilChannel_AppendCommandData(
                        _hChannel,
                        pbCommandData,
                        (uint)cbSize
                        );

                    HRESULT.Check(hr);
                }
            }

            /// <summary>
            /// EndCommand closes an open command on a channel
            /// </summary>
            ///<SecurityNote>
            /// Critical - this code performs an elevation.
            /// TreatAsSafe - it's safe to end a command in a well known channel.
            ///</SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal void EndCommand()
            {
                if (_hChannel == IntPtr.Zero)
                {
                    //
                    // If the channel has been closed, fail silently. This could happen
                    // for the service channel if we are in disconnected state whhen more
                    // that one media contexts are present and not all of them have finished
                    // processing the disconnect messages.
                    //

                    return;
                }

                HRESULT.Check(UnsafeNativeMethods.MilChannel_EndCommand(_hChannel));
            }

            /// <summary>
            /// SendCommand that creates an slave bitmap resource
            /// </summary>
            /// <SecurityNote>
            /// Critical - this code performs an elevation.
            /// </SecurityNote>
            [SecurityCritical]
            internal void SendCommandBitmapSource(
                DUCE.ResourceHandle imageHandle,
                BitmapSourceSafeMILHandle pBitmapSource
                )
            {
                Invariant.Assert(pBitmapSource != null && !pBitmapSource.IsInvalid);
                Invariant.Assert(_hChannel != IntPtr.Zero);

                HRESULT.Check(UnsafeNativeMethods.MilResource_SendCommandBitmapSource(
                    imageHandle,
                    pBitmapSource,
                    _hChannel));
            }

            /// <summary>
            /// SendCommand that creates an slave media resource
            /// </summary>
            /// <SecurityNote>
            /// Critical - this code performs an elevation.
            /// </SecurityNote>
            [SecurityCritical]
            internal void SendCommandMedia(
                DUCE.ResourceHandle mediaHandle,
                SafeMediaHandle pMedia,
                bool notifyUceDirect
                )
            {
                Invariant.Assert(pMedia != null && !pMedia.IsInvalid);

                Invariant.Assert(_hChannel != IntPtr.Zero);

                HRESULT.Check(UnsafeNativeMethods.MilResource_SendCommandMedia(
                    mediaHandle,
                    pMedia,
                    _hChannel,
                    notifyUceDirect
                    ));
            }

            /// <summary>
            /// Specifies the window and window message to be sent when messages
            /// become available in the back channel.
            /// </summary>
            /// <param name="hwnd">
            /// The target of the notification messages. If this parameter is null
            /// then the channel stop sending window messages.
            /// </param>
            /// <param name="message">
            /// The window message ID. If the hwnd parameter is null then this
            /// parameter is ignored.
            /// </param>
            /// <securitynote>
            /// Critical        - Passes a window handle to native code. This will
            ///                   cause milcore to periodically post the specified
            ///                   message to the specified window. The caller is
            ///                   safe if it owns the window and the message was
            ///                   registered with RegisterMessage.
            /// </securitynote>
            [SecurityCritical]
            internal void SetNotificationWindow(IntPtr hwnd, WindowMessage message)
            {
                Invariant.Assert(_hChannel != IntPtr.Zero);

                HRESULT.Check(UnsafeNativeMethods.MilChannel_SetNotificationWindow(
                    _hChannel,
                    hwnd,
                    message
                    ));
            }

            /// <summary>
            /// Waits until a message is available on this channel. The message
            /// can be later retrieved with the PeekNextMessage method.
            /// </summary>
            /// <remarks>
            /// The method may return with no available messages if the channel
            /// is disconnected while waiting.
            /// </remarks>
            /// <SecurityNote>
            /// Critical        - Blocks the thread until the channel receives
            ///                   a message. This is unsafe if done by any
            ///                   component other than the owner of the channel
            ///                   because the channel may not send any
            ///                   messages, in which case the function will
            ///                   never return. Only the owner of the channel
            ///                   knows whether a message can be reasonably
            ///                   expected to eventually be sent.
            /// </SecurityNote>
            [SecurityCritical]
            internal void WaitForNextMessage()
            {
                int waitReturn;

                HRESULT.Check(UnsafeNativeMethods.MilComposition_WaitForNextMessage(
                    _hChannel,
                    0,
                    null,
                    1, /* true */
                    waitInfinite,
                    out waitReturn
                    ));
            }

            /// <summary>
            /// Gets the next available message on this channel. This method
            /// does not wait if a message is not immediately available.
            /// </summary>
            /// <param name="message">
            /// Receives the message.
            /// </param>
            /// <returns>
            /// True if a message was retrieved, false otherwise.
            /// </returns>
            /// <SecurityNote>
            /// Critical        - Removes a message from the channel. This is
            ///                   unsafe if done by any component other than
            ///                   the owner of the channel because eating
            ///                   messages may result in the process becoming
            ///                   non-responsive.
            ///                   Also has an unsafe block, but that is safe
            ///                   to callers because we just need it to use
            ///                   the sizeof operator.
            /// </SecurityNote>
            [SecurityCritical]
            internal bool PeekNextMessage(out MilMessage.Message message)
            {
                Invariant.Assert(_hChannel != IntPtr.Zero);

                int messageRetrieved;

                checked
                {
                    unsafe
                    {
                        HRESULT.Check(UnsafeNativeMethods.MilComposition_PeekNextMessage(
                            _hChannel,
                            out message,
                            (IntPtr)sizeof(MilMessage.Message),
                            out messageRetrieved
                            ));
                    }
                }

                return (messageRetrieved != 0);
            }
        }

        /// <summary>
        /// The Resource structure encapsulates the functionality
        /// required to hold on to a UCE resource. A resource can be sent to a
        /// channel by calling CreateOrAddRefOnChannel. The resource can be deleted
        /// from a channel by calling ReleaseOnChannel.
        ///
        /// With resources the handle management is completely hidden from the caller.
        /// </summary>
        internal struct Resource
        {
            public static readonly Resource Null = new Resource(DUCE.ResourceHandle.Null);

            private DUCE.ResourceHandle _handle;
#if DEBUG
            private Channel _debugOnly_Channel;
#endif

            /// <summary>
            /// THIS IS A TEMPORARY API; DO NOT USE FOR ANYTHING ELSE.
            /// Creates a resource from a type and ResourceHandle.
            /// This is currently only used for some hwnd interop code in the VisualManager.
            /// </summary>
            public Resource(DUCE.ResourceHandle h)
            {
                _handle = h;
#if DEBUG
                _debugOnly_Channel = null;
#endif
            }

            /// <summary>
            /// CreatesOrAddRefs the resource on the specified channel.
            /// </summary>
            public bool CreateOrAddRefOnChannel(object instance, Channel channel, DUCE.ResourceType type)
            {
                Debug.Assert(channel != null);
#if DEBUG
                _debugOnly_Channel = channel;
#endif

                return channel.CreateOrAddRefOnChannel(instance, ref _handle, type);
            }



            /// <summary>
            /// Releases the resource from the specified channel.
            /// Returns true if the resource is not anymore on the specified channel
            /// otherwise false.
            /// </summary>
            /// <return>
            /// Returns true iff the resource is not used on the specified channel anymore.
            /// </return>
            public bool ReleaseOnChannel(Channel channel)
            {
                Debug.Assert(channel != null);
#if DEBUG
                Debug.Assert(_debugOnly_Channel == channel);
#endif
                if (channel.ReleaseOnChannel(_handle))
                {
                    _handle = DUCE.ResourceHandle.Null;
                    return true;
                }
                return false;
            }

            /// <summary>
            /// Checks if a resource was created on the specified channel.
            /// </summary>
            public bool IsOnChannel(Channel channel)
            {
#if DEBUG
                Debug.Assert(_debugOnly_Channel == channel);
#endif
                return !_handle.IsNull;
            }


            /// <summary>
            /// Returns the real UInt32 handle. 
            /// </summary>
            public DUCE.ResourceHandle Handle { get { return _handle; } }
        }

        /// <summary>
        /// ResourceHandle currently encapsulates an unmanaged resource handle.
        /// </summary>
        [StructLayout(LayoutKind.Explicit)]
        internal struct ResourceHandle
        {
            public static readonly ResourceHandle Null = new ResourceHandle(0);

            public static explicit operator uint(ResourceHandle r)
            {
                return r._handle;
            }

            [FieldOffset(0)]
            private UInt32 _handle;
            public ResourceHandle(UInt32 handle) { _handle = handle; }

            /// <summary>
            /// Checks if the handle is null.
            /// </summary>
            public bool IsNull { get { return (_handle == 0); } }
        }

        /// <summary>
        /// This is a generic map that maps a key to a value. It is heavily optimized
        /// for a single entry.
        /// </summary>
        /// <remark>
        /// We are using this map to map a resource onto multple channels. This is non-optimal
        /// solution. The map is currently used by the MultiChannelResource. Eventually, when all
        /// the UCE underlyings are in place, we will be able to remove the MultiChannelResource.
        /// </remark>
        internal struct Map<ValueType>
        {
            /// <summary>
            /// Struct for single entry.
            /// </summary>
            private struct Entry
            {
                public Entry(object k, ValueType v)
                {
                    _key = k;
                    _value = v;
                }

                public object _key;
                public ValueType _value;
            }

            private const int FOUND_IN_INLINE_STORAGE = -1;
            private const int NOT_FOUND = -2;

            // This data structure is optimized for single entry. _first is the one entry that we inline
            // into the struct for that purpose.
            private Entry _first;
            // All other entries go into the generic _others list.
            private List<Entry> _others;

            public bool IsEmpty()
            {
                if (_first._key != null)
                {
                    return false;
                }

                if (_others != null)
                {
                    return false;
                }

                return true;
            }

            /// <summary>
            /// Finds the index of the entry with the specified key. Returns FOUND_IN_INLINE_STORAGE if the
            /// key is stored in the _first inlined entry and NOT_FOUND if the key could not be found.
            /// Otherwise the method returns the index into the _others list.
            /// </summary>
            private int Find(object key)
            {
                int index = NOT_FOUND; // Not found.

                if (_first._key != null)
                {
                    if (_first._key == key)
                    {
                        index = FOUND_IN_INLINE_STORAGE; // It's stored in our inlined storage.
                    }
                    else
                    {
                        if (_others != null)
                        {
                            for (int i = 0; i < _others.Count; i++)
                            {
                                if (_others[i]._key == key)
                                {
                                    index = i;
                                    break;
                                }
                            }
                        }
                    }
                }
#if DEBUG
                else
                {
                    Debug.Assert(_others == null, "There shouldn't be anything stored in the others array.");
                }
#endif

                return index;
            }

            /// <summary>
            /// Associates a key with the specified value. If the entry already exits the old value is overriden.
            /// </summary>
            public void Set(object key, ValueType value)
            {
                int index = Find(key);

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    _first._value = value;
                }
                else
                {
                    if (index == NOT_FOUND)
                    {
                        if (_first._key == null)
                        {
                            _first = new Entry(key, value);
                        }
                        else
                        {
                            if (_others == null)
                            {
                                _others = new List<Entry>(2); // by default we have two entries in the extra storage.
                            }

                            _others.Add(new Entry(key, value));
                        }
                    }
                    else
                    {
                        _others[index] = new Entry(key, value);
                    }
                }
            }

            /// <summary>
            /// Removes an entry from the map. Returns true if the entry to the specified index existed and was removed
            /// otherwise false.
            /// </summary>
            public bool Remove(object key)
            {
                int index = Find(key);

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    if (_others != null)
                    {
                        Debug.Assert(_others.Count > 0);
                        int j = _others.Count-1;
                        _first = _others[j];
                        if (j == 0) // Only one entry in the array.
                        {
                            _others = null;
                        }
                        else
                        {
                            _others.RemoveAt(j);
                        }
                    }
                    else
                    {
                    _first = new Entry();

                    }

                    return true;
                }
                else
                {
                    if (index >= 0)
                    {
                        if (_others.Count == 1)
                        {
                            Debug.Assert(index == 0);
                            _others = null;
                        }
                        else
                        {
                            _others.RemoveAt(index);
                        }

                        return true;
                    }
                }
                return false;
            }

            /// <summary>
            /// Gets the value for the specified key. If the entry for the specified key could not
            /// be found the Get method returns false otherwise true.
            /// </summary>
            public bool Get(object key, out ValueType value)
            {
                int index = Find(key);

                value = default(ValueType);

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    value = _first._value;
                    return true;
                }
                else
                {
                    if (index >= 0)
                    {
                        value = _others[index]._value;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            /// <summary>
            /// Gets the object count in the map.
            /// </summary>
            public int Count()
            {
                if (_first._key == null)
                {
                    return 0;
                }
                else if (_others == null)
                {
                    return 1;
                }
                else
                {
                    return _others.Count + 1;
                }
            }

            /// <summary>
            /// Gets the object at a given index.
            /// </summary>
            public object Get(int index)
            {
                if (index >= Count())
                {
                    return null;
                }

                if (index == 0)
                {
                    return _first._key;
                }

                return _others[index - 1]._key;
            }

        }

        /// <summary>
        /// This is a generic map that maps a key to a value. It is heavily optimized
        /// for a single entry.
        /// </summary>
        /// <remark>
        /// We are using this map to map a resource onto multple channels. This is non-optimal
        /// solution. The map is currently used by the MultiChannelResource. Eventually, when all
        /// the UCE underlyings are in place, we will be able to remove the MultiChannelResource.
        /// </remark>
        internal struct Map
        {
            /// <summary>
            /// Struct for single entry.
            /// </summary>
            private struct Entry
            {
                public Entry(object k, DUCE.ResourceHandle v)
                {
                    _key = k;
                    _value = v;
                }

                public object _key;
                public DUCE.ResourceHandle _value;
            }

            private const int FOUND_IN_INLINE_STORAGE = -1;
            private const int NOT_FOUND = -2;

            // This data structure is optimized for single entry. _head is the one entry that we inline
            // into the struct for that purpose.  If there are more than one entries, we store a list of
            // entries in _head._key and DUCE.ResourceHandle.Null in _value.
            private Entry _head;

            public bool IsEmpty()
            {
                return _head._key == null;
            }

            /// <summary>
            /// Finds the index of the entry with the specified key. Returns FOUND_IN_INLINE_STORAGE if the
            /// key is stored in the _head inlined entry and NOT_FOUND if the key could not be found.
            /// Otherwise the method returns the index into the "others" list.
            /// </summary>
            private int Find(object key)
            {
                int index = NOT_FOUND; // Not found.

                if (_head._key != null)
                {
                    if (_head._key == key)
                    {
                        index = FOUND_IN_INLINE_STORAGE; // It's stored in our inlined storage.
                    }
                    else
                    {
                        if (_head._value.IsNull)
                        {
                            List<Entry> others = (List<Entry>)(_head._key);

                            for (int i = 0; i < others.Count; i++)
                            {
                                if (others[i]._key == key)
                                {
                                    index = i;
                                    break;
                                }
                            }
                        }
                    }
                }

                return index;
            }

            /// <summary>
            /// Associates a key with the specified value. If the entry already exits the old value is overriden.
            /// </summary>
            public void Set(object key, DUCE.ResourceHandle value)
            {
                int index = Find(key);

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    _head._value = value;
                }
                else
                {
                    if (index == NOT_FOUND)
                    {
                        // The key was not found.
                        // Is the Map empty?
                        if (_head._key == null)
                        {
                            _head = new Entry(key, value);
                        }
                        else
                        {
                            // The Map isn't empty - does it have 1 entry (!_value.IsNull) or more?
                            if (!_head._value.IsNull)
                            {
                                // There's 1 entry - allocate a list...
                                List<Entry> others = new List<Entry>(2); // by default we have two entries in the extra storage.

                                // ...move the old single entry into the List...
                                others.Add(_head);
                                // ...add the new entry...
                                others.Add(new Entry(key, value));

                                // ... and replace the single entry
                                _head._key = others;
                                _head._value = DUCE.ResourceHandle.Null;

                            }
                            else
                            {
                                // There's already a List - simply add the new entry to the list.
                                ((List<Entry>)(_head._key)).Add(new Entry(key, value));
                            }
                        }
                    }
                    else
                    {
                        ((List<Entry>)(_head._key))[index] = new Entry(key, value);
                    }
                }
            }

            /// <summary>
            /// Removes an entry from the map. Returns true if the entry to the specified index existed and was removed
            /// otherwise false.
            /// </summary>
            public bool Remove(object key)
            {
                int index = Find(key);

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    _head = new Entry();

                    return true;
                }
                else
                {
                    if (index >= 0)
                    {
                        List<Entry> others = (List<Entry>)_head._key;

                        // If the Count() is 1, index would either have been FOUND_IN_INLINE_STORAGE or
                        // NOT_FOUND, so Count() must be 2 or more.
                        // If it is exactly 2, this means that after removal there will only be one
                        // value left, which will be stored in _head.
                        if (Count() == 2)
                        {
                            Debug.Assert(index <= 1);

                            // If the index is 0, we remove 0 and store 1 in _head.
                            // If the index is 1, we remove 1 and store 0 in _head.

                            _head = others[1 - index];
                        }
                        else
                        {
                            others.RemoveAt(index);
                        }

                        return true;
                    }
                }
                return false;
            }

            /// <summary>
            /// Gets the value for the specified key. If the entry for the specified key could not
            /// be found the Get method returns false otherwise true.
            /// </summary>
            public bool Get(object key, out DUCE.ResourceHandle value)
            {
                int index = Find(key);

                value = DUCE.ResourceHandle.Null;

                if (index == FOUND_IN_INLINE_STORAGE)
                {
                    value = _head._value;
                    return true;
                }
                else
                {
                    if (index >= 0)
                    {
                        value = ((List<Entry>)_head._key)[index]._value;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            /// <summary>
            /// Gets the object count in the map.
            /// </summary>
            public int Count()
            {
                if (_head._key == null)
                {
                    return 0;
                }
                else if (!_head._value.IsNull)
                {
                    return 1;
                }
                else
                {
                    return ((List<Entry>)_head._key).Count;
                }
            }

            /// <summary>
            /// Gets the object at a given index.
            /// </summary>
            public object Get(int index)
            {
                if ((index >= Count()) || (index < 0))
                {
                    return null;
                }

                if (Count() == 1)
                {
                    Debug.Assert(index == 0);
                    return _head._key;
                }

                return ((List<Entry>)_head._key)[index]._key;
            }

        }

        /// <summary>
        /// ShareableDUCEMultiChannelResource class - this class simply wraps a MultiChannelResource,
        /// enabling it to be shared/handed off/etc via reference semantics.
        /// Note that this is ~8 bytes larger than the struct MultiChannelResource.
        /// </summary>
        internal class ShareableDUCEMultiChannelResource
        {
            public MultiChannelResource _duceResource;
        }


        /// <summary>
        /// A multi-channel resource encapsulates a resource that can be used on multiple channels at the same time.
        /// </summary>
        internal struct MultiChannelResource
        {
            private Map _map;

            /// <summary>
            /// CreatesOrAddRefs the resource on the specified channel.
            /// </summary>
            /// <remark>
            /// <return>
            /// Returns true iff the resource is not used on the specified channel anymore.
            /// </return>
            /// The method is not synchronized. If the resource is used in a multi-threaded scenario, the
            /// caller is responsible for taking a lock before calling CreateOrAddRefOnChannel or ReleaseOnChannel.
            /// </remark>
            public bool CreateOrAddRefOnChannel(object instance, Channel channel, DUCE.ResourceType type)
            {
                Debug.Assert(channel != null);
                DUCE.ResourceHandle handle;
                bool inmap = _map.Get(channel, out handle);

                bool created = channel.CreateOrAddRefOnChannel(instance, ref handle, type);
                
                if (!inmap)
                {
                    _map.Set(channel, handle);
                }

                return created;
            }

            /// <summary>
            /// Attempts to duplicate a handle to the specified target channel.
            /// </summary>
            /// <remarks>
            /// The idea here is to attempt to find a compatible channel among
            /// the channels this resource has been marshalled to.
            /// </remarks>
            /// <param name="sourceChannel">The channel to duplicate the handle from.</param>
            /// <param name="targetChannel">The channel to duplicate the handle to.</param>
            public DUCE.ResourceHandle DuplicateHandle(Channel sourceChannel, Channel targetChannel)
            {
                Debug.Assert(sourceChannel != null);
                DUCE.ResourceHandle duplicate = DUCE.ResourceHandle.Null;

                DUCE.ResourceHandle handle = DUCE.ResourceHandle.Null;
                bool found = _map.Get(sourceChannel, out handle);
                Debug.Assert(found);

                //
                // The multi channel resource should not exist on the target channel.
                // Our current implementation only keeps a map of the channel and the handle
                // so only one instance of this resource can be on one channel.
                //
                Debug.Assert(!(_map.Get(targetChannel, out duplicate)));

                duplicate = sourceChannel.DuplicateHandle(handle, targetChannel);

                if (!duplicate.IsNull)
                {
                    _map.Set(targetChannel, duplicate);
                }

                return duplicate;
            }

            /// <summary>
            /// Releases the resource from the specified channel.
            /// Returns true if the resource is not anymore on the specified channel
            /// otherwise false.
            /// </summary>
            /// <remark>
            /// The method is not synchronized. If the resource is used in a multi-threaded scenario, the
            /// caller is responsible for taking a lock before calling CreateOrAddRefOnChannel or ReleaseOnChannel.
            /// </remark>
            /// <return>
            /// Returns true iff the resource is not used on the specified channel anymore.
            /// </return>
            public bool ReleaseOnChannel(Channel channel)
            {
                Debug.Assert(channel != null);

                DUCE.ResourceHandle handle;

                bool found = _map.Get(channel, out handle);
                Debug.Assert(found);

                if (channel.ReleaseOnChannel(handle))
                {
                    //
                    // The handle isn't used on the specified channel anymore. Therefore
                    // we can reclaim the handle.
                    _map.Remove(channel);
                    return true;
                }
                return false;
            }

            /// <summary>
            /// Returns the ResourceHandle for this resource on the specified channel.
            /// </summary>
            public DUCE.ResourceHandle GetHandle(Channel channel)
            {
                DUCE.ResourceHandle h;

                if (channel != null)
                {
                    _map.Get(channel, out h);
                }
                else
                {
                    h = ResourceHandle.Null;
                }
                return h;
            }

            /// <summary>
            /// Checks if a resource was created on the specified channel.
            /// </summary>
            public bool IsOnChannel(Channel channel)
            {
                return !GetHandle(channel).IsNull;
            }

            /// <summary>
            /// Checks if a resource was created on any channel.
            /// </summary>
            public bool IsOnAnyChannel
            {
                get
                {
                    return !_map.IsEmpty();
                }
            }

            public int GetChannelCount()
            {
                return _map.Count();
            }

            public DUCE.Channel GetChannel(int index)
            {
                return _map.Get(index) as DUCE.Channel;
            }


            public uint GetRefCountOnChannel(Channel channel)
            {
                Debug.Assert(channel != null);

                DUCE.ResourceHandle handle;

                bool found = _map.Get(channel, out handle);
                Debug.Assert(found);

                return channel.GetRefCount(handle);
            }
        }

        internal static class CompositionNode
        {
            // -----------------------------------------------------------------------------------------------------------------------
            // Public imports for composition nodes.
            //
            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetTransform(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hTransform,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETTRANSFORM command;

                command.Type = MILCMD.MilCmdVisualSetTransform;
                command.Handle = hCompositionNode;
                command.hTransform = hTransform;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETTRANSFORM)
                        );
                }
            }


            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetEffect(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hEffect,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETEFFECT command;               

                command.Type = MILCMD.MilCmdVisualSetEffect;
                command.Handle = hCompositionNode;
                command.hEffect = hEffect;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETEFFECT)
                        );
                }
            }
            

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetCacheMode(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hCacheMode,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETCACHEMODE command;               

                command.Type = MILCMD.MilCmdVisualSetCacheMode;
                command.Handle = hCompositionNode;
                command.hCacheMode = hCacheMode;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETCACHEMODE)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetOffset(
                DUCE.ResourceHandle hCompositionNode,
                double offsetX,
                double offsetY,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETOFFSET command;
                command.Type = MILCMD.MilCmdVisualSetOffset;
                command.Handle = hCompositionNode;
                command.offsetX = offsetX;
                command.offsetY = offsetY;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETOFFSET)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetContent(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hContent,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETCONTENT command;

                command.Type = MILCMD.MilCmdVisualSetContent;
                command.Handle = hCompositionNode;
                command.hContent = hContent;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETCONTENT)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetAlpha(
                DUCE.ResourceHandle hCompositionNode,
                double alpha,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETALPHA command;

                command.Type = MILCMD.MilCmdVisualSetAlpha;
                command.Handle = hCompositionNode;
                command.alpha = alpha;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETALPHA)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetAlphaMask(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hAlphaMaskBrush,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETALPHAMASK command;

                command.Type = MILCMD.MilCmdVisualSetAlphaMask;
                command.Handle = hCompositionNode;
                command.hAlphaMask = hAlphaMaskBrush;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETALPHAMASK)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetScrollableAreaClip(
                DUCE.ResourceHandle hCompositionNode,
                Rect? clip,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETSCROLLABLEAREACLIP command;

                command.Type = MILCMD.MilCmdVisualSetScrollableAreaClip;
                command.Handle = hCompositionNode;
                command.IsEnabled = (uint) (clip.HasValue ? 1 : 0);

                if (clip.HasValue)
                {
                    command.Clip = clip.Value;
                }
                else
                {
                    command.Clip = Rect.Empty;
                }

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETSCROLLABLEAREACLIP)
                        );
                }
            }            

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetClip(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hClip,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETCLIP command;

                command.Type = MILCMD.MilCmdVisualSetClip;
                command.Handle = hCompositionNode;
                command.hClip = hClip;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETCLIP)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetRenderOptions(
                DUCE.ResourceHandle hCompositionNode,
                MilRenderOptions renderOptions,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_SETRENDEROPTIONS command;

                command.Type = MILCMD.MilCmdVisualSetRenderOptions;
                command.Handle = hCompositionNode;
                command.renderOptions = renderOptions;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_SETRENDEROPTIONS)
                        );
                }

            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void RemoveChild(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hChild,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_REMOVECHILD command;

                command.Type = MILCMD.MilCmdVisualRemoveChild;
                command.Handle = hCompositionNode;
                command.hChild = hChild;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_REMOVECHILD)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void RemoveAllChildren(
                DUCE.ResourceHandle hCompositionNode,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_REMOVEALLCHILDREN command;

                command.Type = MILCMD.MilCmdVisualRemoveAllChildren;
                command.Handle = hCompositionNode;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_REMOVEALLCHILDREN)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void InsertChildAt(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hChild,
                UInt32 iPosition,
                Channel channel)
            {
                DUCE.MILCMD_VISUAL_INSERTCHILDAT command;
                Debug.Assert(!hCompositionNode.IsNull);

                command.Type = MILCMD.MilCmdVisualInsertChildAt;
                command.Handle = hCompositionNode;
                command.hChild = hChild;
                command.index = iPosition;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL_INSERTCHILDAT)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetGuidelineCollection(
                DUCE.ResourceHandle hCompositionNode,
                DoubleCollection guidelinesX,
                DoubleCollection guidelinesY,
                Channel channel
                )
            {
                checked
                {
                    DUCE.MILCMD_VISUAL_SETGUIDELINECOLLECTION command;
                    Debug.Assert(!hCompositionNode.IsNull);

                    int countX = guidelinesX == null ? 0 : guidelinesX.Count;
                    int countY = guidelinesY == null ? 0 : guidelinesY.Count;
                    int countXY = countX + countY;

                    command.Type = MILCMD.MilCmdVisualSetGuidelineCollection;
                    command.Handle = hCompositionNode;
                    command.countX = (UInt16)countX;
                    command.countY = (UInt16)countY;

                    if (countX == 0 && countY == 0)
                    {
                        unsafe
                        {
                            channel.SendCommand(
                                (byte*)&command,
                                sizeof(DUCE.MILCMD_VISUAL_SETGUIDELINECOLLECTION)
                                );
                        }
                    }
                    else
                    {
                        double[] doubleArray = new double[countXY];

                        if (countX != 0)
                        {
                            guidelinesX.CopyTo(doubleArray, 0);
                            Array.Sort(doubleArray, 0, countX);
                        }

                        if (countY != 0)
                        {
                            guidelinesY.CopyTo(doubleArray, countX);
                            Array.Sort(doubleArray, countX, countY);
                        }

                        float[] floatArray = new float[countXY];
                        for (int i = 0; i < countXY; i++)
                        {
                            floatArray[i] = (float)(double)(doubleArray[i]);
                        }

                        unsafe
                        {
                            channel.BeginCommand(
                                (byte*)&command,
                                sizeof(DUCE.MILCMD_VISUAL_SETGUIDELINECOLLECTION),
                                sizeof(float) * countXY
                                );

                            fixed (float* pData = floatArray)
                            {
                                channel.AppendCommandData(
                                    (byte*)pData,
                                    sizeof(float) * countXY
                                    );
                            }

                            channel.EndCommand();
                        }
                    }
                }
            }
        }

        internal static class Viewport3DVisualNode
        {
            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetCamera(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hCamera,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VIEWPORT3DVISUAL_SETCAMERA command;

                    command.Type = MILCMD.MilCmdViewport3DVisualSetCamera;
                    command.Handle = hCompositionNode;
                    command.hCamera = hCamera;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VIEWPORT3DVISUAL_SETCAMERA)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetViewport(
                DUCE.ResourceHandle hCompositionNode,
                Rect viewport,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT command;

                    command.Type = MILCMD.MilCmdViewport3DVisualSetViewport;
                    command.Handle = hCompositionNode;
                    command.Viewport = viewport;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void Set3DChild(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hVisual3D,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VIEWPORT3DVISUAL_SET3DCHILD command;

                    command.Type = MILCMD.MilCmdViewport3DVisualSet3DChild;
                    command.Handle = hCompositionNode;
                    command.hChild = hVisual3D;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VIEWPORT3DVISUAL_SET3DCHILD)
                        );
                }
            }
        }

        internal static class Visual3DNode
        {
            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void RemoveChild(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hChild,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VISUAL3D_REMOVECHILD command;

                    command.Type = MILCMD.MilCmdVisual3DRemoveChild;
                    command.Handle = hCompositionNode;
                    command.hChild = hChild;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL3D_REMOVECHILD)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void RemoveAllChildren(
                DUCE.ResourceHandle hCompositionNode,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VISUAL3D_REMOVEALLCHILDREN command;

                    command.Type = MILCMD.MilCmdVisual3DRemoveAllChildren;
                    command.Handle = hCompositionNode;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL3D_REMOVEALLCHILDREN)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void InsertChildAt(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hChild,
                UInt32 iPosition,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VISUAL3D_INSERTCHILDAT command;
                    Debug.Assert(!hCompositionNode.IsNull);

                    command.Type = MILCMD.MilCmdVisual3DInsertChildAt;
                    command.Handle = hCompositionNode;
                    command.hChild = hChild;
                    command.index = iPosition;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL3D_INSERTCHILDAT)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetContent(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hContent,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VISUAL3D_SETCONTENT command;

                    command.Type = MILCMD.MilCmdVisual3DSetContent;
                    command.Handle = hCompositionNode;
                    command.hContent = hContent;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL3D_SETCONTENT)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical,SecurityTreatAsSafe]
            internal static void SetTransform(
                DUCE.ResourceHandle hCompositionNode,
                DUCE.ResourceHandle hTransform,
                Channel channel)
            {
                unsafe
                {
                    DUCE.MILCMD_VISUAL3D_SETTRANSFORM command;

                    command.Type = MILCMD.MilCmdVisual3DSetTransform;
                    command.Handle = hCompositionNode;
                    command.hTransform = hTransform;

                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_VISUAL3D_SETTRANSFORM)
                        );
                }
            }
        }

        internal static class CompositionTarget
        {
            // -----------------------------------------------------------------------------------------------------------------------
            // Public imports for composition targets.
            //
            /// <SecurityNote>
            ///     Critical: This code calls into unsafe code blocks and initialized hwnd for composition target
            /// </SecurityNote>
            [SecurityCritical]
            internal static void HwndInitialize(
                DUCE.ResourceHandle hCompositionTarget,
                IntPtr hWnd,
                int nWidth,
                int nHeight,
                bool softwareOnly,
                int dpiAwarenessContext,
                DpiScale dpiScale,
                Channel channel
                )
            {
                DUCE.MILCMD_HWNDTARGET_CREATE command;

                command.Type = MILCMD.MilCmdHwndTargetCreate;
                command.Handle = hCompositionTarget;

                unsafe
                {
                    //
                    // If the HWND has the highest bit set, casting it to UInt64
                    // directly will cause sign extension. Prevent this by casting
                    // through a pointer and UIntPtr first.
                    //

                    UIntPtr hWndUIntPtr = new UIntPtr(hWnd.ToPointer());
                    command.hwnd = (UInt64)hWndUIntPtr;
                }

                command.width = (UInt32)nWidth;
                command.height = (UInt32)nHeight;

                command.clearColor.b = 0.0f;
                command.clearColor.r = 0.0f;
                command.clearColor.g = 0.0f;
                command.clearColor.a = 1.0f;

                command.flags =
                    (UInt32)(MILRTInitializationFlags.MIL_RT_PRESENT_IMMEDIATELY |
                    MILRTInitializationFlags.MIL_RT_PRESENT_RETAIN_CONTENTS);

                if (softwareOnly)
                {
                    //
                    // In some scenarios, we will want to ensure that the rendered content is
                    // accessible through ntuser redirection. This is to allow graphics stream
                    // clients to selectively magnify some WPF applications through the use
                    // of the graphics stream, and some of them through legacy magnification
                    // (it could be triggered by versioning problems, rendering errors on the
                    // graphics stream client side, etc.).
                    //

                    command.flags |= (UInt32)MILRTInitializationFlags.MIL_RT_SOFTWARE_ONLY;
                }

                bool? enableMultiMonitorDisplayClipping = 
                    System.Windows.CoreCompatibilityPreferences.EnableMultiMonitorDisplayClipping;

                if (enableMultiMonitorDisplayClipping != null)
                {
                    // The flag is explicitly set by the user in application manifest
                    command.flags |= (UInt32)MILRTInitializationFlags.MIL_RT_IS_DISABLE_MULTIMON_DISPLAY_CLIPPING_VALID;

                    if (!enableMultiMonitorDisplayClipping.Value)
                    {
                        command.flags |= (UInt32) MILRTInitializationFlags.MIL_RT_DISABLE_MULTIMON_DISPLAY_CLIPPING;
                    }
                }

                command.hBitmap = DUCE.ResourceHandle.Null;
                command.stride = 0;
                command.ePixelFormat = 0;
                command.hSection = 0;
                command.masterDevice = 0;

                command.DpiAwarenessContext = dpiAwarenessContext;
                command.DpiX = dpiScale.DpiScaleX;
                command.DpiY = dpiScale.DpiScaleY;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_HWNDTARGET_CREATE),
                        false /* sendInSeparateBatch */
                        );
                }
            }

            [SecuritySafeCritical]
            internal static void ProcessDpiChanged(
                DUCE.ResourceHandle hCompositionTarget,
                DpiScale dpiScale,
                bool afterParent,
                Channel channel
                )
            {
                DUCE.MILCMD_HWNDTARGET_DPICHANGED command;

                command.Type = MILCMD.MilCmdHwndTargetDpiChanged;
                command.Handle = hCompositionTarget;
                command.DpiX = dpiScale.DpiScaleX;
                command.DpiY = dpiScale.DpiScaleY;
                command.AfterParent = afterParent ? 1U : 0U;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_HWNDTARGET_DPICHANGED),
                        sendInSeparateBatch: false);
                }
            }

            /// <SecurityNote>
            /// Critical - 1) The command being sent contains an unmanaged pointer.
            ///            2) This code accesses an unsafe code block.
            /// </SecurityNote>
            [SecurityCritical]
            internal static void PrintInitialize(
                DUCE.ResourceHandle hCompositionTarget,
                IntPtr pRenderTarget,
                int nWidth,
                int nHeight,
                Channel channel
                )
            {
                DUCE.MILCMD_GENERICTARGET_CREATE command;

                command.Type = MILCMD.MilCmdGenericTargetCreate;
                command.Handle = hCompositionTarget;
                command.hwnd = 0;
                command.pRenderTarget = (UInt64)pRenderTarget;
                command.width = (UInt32)nWidth;
                command.height = (UInt32)nHeight;
                command.dummy = 0;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_GENERICTARGET_CREATE),
                        false /* sendInSeparateBatch */
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetClearColor(
                DUCE.ResourceHandle hCompositionTarget,
                Color color,
                Channel channel
                )
            {
                DUCE.MILCMD_TARGET_SETCLEARCOLOR command;

                command.Type = MILCMD.MilCmdTargetSetClearColor;
                command.Handle = hCompositionTarget;
                command.clearColor.b = color.ScB;
                command.clearColor.r = color.ScR;
                command.clearColor.g = color.ScG;
                command.clearColor.a = color.ScA;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_TARGET_SETCLEARCOLOR)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetRenderingMode(
                DUCE.ResourceHandle hCompositionTarget,
                MILRTInitializationFlags nRenderingMode,
                Channel channel
                )
            {
                DUCE.MILCMD_TARGET_SETFLAGS command;

                command.Type = MILCMD.MilCmdTargetSetFlags;
                command.Handle = hCompositionTarget;
                command.flags = (UInt32)nRenderingMode;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_TARGET_SETFLAGS)
                        );
                }
            }


            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void SetRoot(
                DUCE.ResourceHandle hCompositionTarget,
                DUCE.ResourceHandle hRoot,
                Channel channel
                )
            {
                DUCE.MILCMD_TARGET_SETROOT command;

                command.Type = MILCMD.MilCmdTargetSetRoot;
                command.Handle = hCompositionTarget;
                command.hRoot = hRoot;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_TARGET_SETROOT)
                        );
                }
            }


            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block.
            ///               We also pass across a handle to an event, which we get via SafeWaitHandle.DangerousGetHandle.
            /// </SecurityNote>
            [SecurityCritical]
            internal static void UpdateWindowSettings(
                ResourceHandle hCompositionTarget,
                NativeMethods.RECT windowRect,
                Color colorKey,
                float constantAlpha,
                MILWindowLayerType windowLayerType,
                MILTransparencyFlags transparencyMode,
                bool isChild,
                bool isRTL,
                bool renderingEnabled,
                int disableCookie,
                Channel channel
                )
            {
                DUCE.MILCMD_TARGET_UPDATEWINDOWSETTINGS command;

                command.Type = MILCMD.MilCmdTargetUpdateWindowSettings;
                command.Handle = hCompositionTarget;

                command.renderingEnabled = (uint)(renderingEnabled ? 1 : 0);
                command.disableCookie = (uint) disableCookie;

                command.windowRect = windowRect;

                command.colorKey.b = colorKey.ScB;
                command.colorKey.r = colorKey.ScR;
                command.colorKey.g = colorKey.ScG;
                command.colorKey.a = colorKey.ScA;

                command.constantAlpha = constantAlpha;
                command.transparencyMode = transparencyMode;
                command.windowLayerType = windowLayerType;

                command.isChild = (uint)(isChild ? 1 : 0);
                command.isRTL = (uint)(isRTL ? 1 : 0);

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_TARGET_UPDATEWINDOWSETTINGS)
                        );
                }
            }

            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe: Operation is ok to call. It does not return any pointers and sending a pointer to a channel is safe
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void Invalidate(
                DUCE.ResourceHandle hCompositionTarget,
                ref NativeMethods.RECT pRect,
                Channel channel
                )
            {
                DUCE.MILCMD_TARGET_INVALIDATE command;

                command.Type = MILCMD.MilCmdTargetInvalidate;
                command.Handle = hCompositionTarget;
                command.rc = pRect;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_TARGET_INVALIDATE),
                        false /* sendInSeparateBatch */
                        );
                }

                channel.CloseBatch();
                channel.Commit();
            }
        }

        /// <summary>
        /// See <see cref="MediaContext.ShouldRenderEvenWhenNoDisplayDevicesAreAvailable"/> for 
        /// details.
        /// </summary>
        /// <SecurityNote>
        ///     Critical: This code accesses an unsafe code block
        ///     Safe:     Operation is ok to call, sending a pointer to a channel is safe, 
        ///               and this does not return any Critical data to the caller
        /// </SecurityNote>
        [SecuritySafeCritical]
        internal static void NotifyPolicyChangeForNonInteractiveMode(
            bool forceRender, 
            Channel channel
            )
        {
            var command = new DUCE.MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE
            {
                Type = MILCMD.MilCmdPartitionNotifyPolicyChangeForNonInteractiveMode,
                ShouldRenderEvenWhenNoDisplayDevicesAreAvailable = (forceRender ? 1u : 0u)
            };

            unsafe
            {
                channel.SendCommand(
                    (byte*)&command, 
                    sizeof(DUCE.MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE), 
                    sendInSeparateBatch: false
                    );
            }

            // Caller should close and commit
        }

        internal static class ETWEvent
        {
            /// <SecurityNote>
            ///     Critical: This code accesses an unsafe code block
            ///     TreatAsSafe:  It does not return any pointers and is safe to call
            /// </SecurityNote>
            [SecurityCritical, SecurityTreatAsSafe]
            internal static void RaiseEvent(
                DUCE.ResourceHandle hEtwEvent,
                UInt32 id,
                Channel channel
                )
            {
                DUCE.MILCMD_ETWEVENTRESOURCE command;

                command.Type = MILCMD.MilCmdEtwEventResource;
                command.Handle = hEtwEvent;
                command.id = id;

                unsafe
                {
                    channel.SendCommand(
                        (byte*)&command,
                        sizeof(DUCE.MILCMD_ETWEVENTRESOURCE)
                        );
                }
            }
        }


        ///<summary>
        /// DUCE.IResource
        ///</summary>
        internal interface IResource
        {
            DUCE.ResourceHandle AddRefOnChannel(Channel channel);

            int GetChannelCount();

            DUCE.Channel GetChannel(int index);

            void ReleaseOnChannel(Channel channel);

            DUCE.ResourceHandle GetHandle(Channel channel);

            /// <summary>
            /// Only Vieport3DVisual and Visual3D implement this.
            /// Vieport3DVisual has two handles. One stored in _proxy
            /// and the other one stored in _proxy3D. This function returns
            /// the handle stored in _proxy3D.
            /// </summary>
            DUCE.ResourceHandle Get3DHandle(Channel channel);

            /// <summary>
            /// Sends a command to compositor to remove the child
            /// from its parent on the channel.
            /// </summary>
            void RemoveChildFromParent(
                    IResource parent,
                    DUCE.Channel channel);
        }
    }
}



