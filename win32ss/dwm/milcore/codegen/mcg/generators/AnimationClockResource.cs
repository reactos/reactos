// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate files for AnimationClockResources
//

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class AnimationClockResource : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public AnimationClockResource(ResourceModel rm) : base(rm) {}

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        public override void Go()
        {
            // AnimationClockResources end up in src\Core\CSharp\system\windows\media\animation
            string generatedPath = Path.Combine(_resourceModel.OutputDirectory,
                                                "src\\Core\\CSharp\\system\\windows\\media\\animation\\generated");

            foreach (McgResource resource in _resourceModel.Resources)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.AnimationResource, resource))
                {
                    continue;
                }

                string fileName = resource.Name + "AnimationClockResource.cs";

                string fullPath = Path.Combine(_resourceModel.OutputDirectory, generatedPath);

                using (FileCodeSink fileCodeSink = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    string nameAsUpper = resource.Name.ToUpper(System.Globalization.CultureInfo.InvariantCulture);

                    fileCodeSink.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]

                            using System;
                            using System.Windows;
                            using System.Windows.Media;
                            using System.Windows.Media.Composition;
                            using System.Diagnostics;
                            using System.Runtime.InteropServices;
                            using System.Security;

                            namespace System.Windows.Media.Animation
                            {
                                /// <summary>
                                /// [[resource.Name]]AnimationClockResource class.
                                /// AnimationClockResource classes refer to an AnimationClock and a base
                                /// value.  They implement DUCE.IResource, and thus can be used to produce
                                /// a render-side resource which represents the current value of this
                                /// AnimationClock.
                                /// They subscribe to the Changed event on the AnimationClock and ensure
                                /// that the resource's current value is up to date.
                                /// </summary>
                                internal class [[resource.Name]]AnimationClockResource: AnimationClockResource, DUCE.IResource
                                {
                                    /// <summary>
                                    /// Constructor for public [[resource.Name]]AnimationClockResource.
                                    /// This constructor accepts the base value and AnimationClock.
                                    /// Note that since there is no current requirement that we be able to set or replace either the
                                    /// base value or the AnimationClock, this is the only way to initialize an instance of
                                    /// [[resource.Name]]AnimationClockResource.
                                    /// Also, we currently Assert that the resource is non-null, since without mutability
                                    /// such a resource isn't needed.
                                    /// We can easily extend this class if/when new requirements arise.
                                    /// </summary>
                                    /// <param name="baseValue"> [[resource.Name]] - The base value. </param>
                                    /// <param name="animationClock"> AnimationClock - cannot be null. </param>
                                    public [[resource.Name]]AnimationClockResource(
                                        [[resource.Name]] baseValue,
                                        AnimationClock animationClock
                                        ): base( animationClock )
                                    {
                                        _baseValue = baseValue;
                                    }

                                    #region Public Properties

                                    /// <summary>
                                    /// BaseValue Property - typed accessor for BaseValue.
                                    /// </summary>
                                    public [[resource.Name]] BaseValue
                                    {
                                        get
                                        {
                                            return _baseValue;
                                        }
                                    }

                                    /// <summary>
                                    /// CurrentValue Property - typed accessor for CurrentValue
                                    /// </summary>
                                    public [[resource.Name]] CurrentValue
                                    {
                                        get
                                        {
                                            if (_animationClock != null)
                                            {
                                                // No handoff for DrawingContext animations so we use the
                                                // BaseValue as the defaultOriginValue and the
                                                // defaultDestinationValue.  We call the Timeline's GetCurrentValue
                                                // directly to avoid boxing
                                                return (([[resource.Name]]AnimationBase)(_animationClock.Timeline)).GetCurrentValue(
                                                    _baseValue,  // defaultOriginValue
                                                    _baseValue,  // defaultDesinationValue
                                                    _animationClock); // clock
                                            }
                                            else
                                            {
                                                return _baseValue;
                                            }
                                        }
                                    }

                                    #endregion Public Properties

                                    #region DUCE

                                    //
                                    // Method which returns the DUCE type of this class.
                                    // The base class needs this type when calling CreateOrAddRefOnChannel.
                                    // By providing this via a virtual, we avoid a per-instance storage cost.
                                    //
                                    protected override DUCE.ResourceType ResourceType
                                    {
                                        get
                                        {
                                             return DUCE.ResourceType.TYPE_[[nameAsUpper]]RESOURCE;
                                        }
                                    }

                                    /// <summary>
                                    /// UpdateResource - This method is called to update the render-thread
                                    /// resource on a given channel.
                                    /// </summary>
                                    /// <param name="handle"> The DUCE.ResourceHandle for this resource on this channel. </param>
                                    /// <param name="channel"> The channel on which to update the render-thread resource. </param>
                                    /// <SecurityNote>
                                    ///     Critical: This code calls into an unsafe code block
                                    ///     TreatAsSafe: This code does not return any critical data.It is ok to expose
                                    ///     Channels can handle bad pointers and will not affect other appdomains or processes
                                    /// </SecurityNote>
                                    [SecurityCritical,SecurityTreatAsSafe]
                                    protected override void UpdateResource(
                                        DUCE.ResourceHandle handle,
                                        DUCE.Channel channel)
                                    {
                                        DUCE.MILCMD_[[nameAsUpper]]RESOURCE cmd = new DUCE.MILCMD_[[nameAsUpper]]RESOURCE();

                                        cmd.Type = MILCMD.MilCmd[[resource.Name]]Resource;
                                        cmd.Handle = handle;
                                        cmd.Value = CurrentValue;

                                        unsafe
                                        {
                                            channel.SendCommand(
                                                (byte*)&cmd,
                                                sizeof(DUCE.MILCMD_[[nameAsUpper]]RESOURCE));
                                        }

                                        // Validate this resource
                                        IsResourceInvalid = false;
                                    }

                                    #endregion DUCE

                                    private [[resource.Name]] _baseValue;
                                }
                            }
                        [[/inline]]
                        );
                }
            }
        }

        #endregion Public Methods
    }
}



