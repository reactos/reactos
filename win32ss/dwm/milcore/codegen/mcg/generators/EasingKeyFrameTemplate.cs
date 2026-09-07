// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of 
//              the type-specific Easing keyframes (EasingByteKeyFrame, etc)
//              
//---------------------------------------------------------------------------

using System;
using System.IO;
using System.Xml;
using System.Collections.Generic;

using MS.Internal.MilCodeGen;
using MS.Internal.MilCodeGen.Runtime;
using MS.Internal.MilCodeGen.ResourceModel;
using MS.Internal.MilCodeGen.Helpers;

namespace MS.Internal.MilCodeGen.ResourceModel
{
    /// <summary>
    /// EasingKeyFrameTemplate: This class represents one instantiation of the EasingKeyFrame template.
    /// Due to a limitation of the build system, EasingKeyFrame classes are coalesced into one file
    /// per module.
    /// </summary>
    public class EasingKeyFrameTemplate: Template
    {
        private struct EasingKeyFrameTemplateInstance
        {
            public EasingKeyFrameTemplateInstance(
                string moduleName,
                string typeName
                )
            {
                ModuleName = moduleName;
                TypeName = typeName;
            }

            public string ModuleName;
            public string TypeName;
        }

        /// <summary>
        /// AddTemplateInstance - This is called by the code which parses the generation control.
        /// It is called on each TemplateInstance XMLNode encountered.
        /// </summary>
        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new EasingKeyFrameTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            FileCodeSink csFile = null;
            string currentModuleName = null;

            foreach (EasingKeyFrameTemplateInstance instance in Instances)
            {
                string extraInterpolateArgs = "";

                //
                // If we've hit a new module we need to close off the current file
                // and make a new one.
                //
                if (instance.ModuleName != currentModuleName)
                {
                    currentModuleName = instance.ModuleName;
                    CloseFile(ref csFile);
                    ProcessNewModule(resourceModel, currentModuleName, ref csFile);
                }


                // AnimatedTypeHelpers.Interpolate has an extra parameter
                // for the Quaternion type
                if (instance.TypeName == "Quaternion")
                {
                    extraInterpolateArgs = ", UseShortestPath";
                }


                //
                // Write the typed class for the current instance
                //

                csFile.WriteBlock(
                    [[inline]]
                        
                            /// <summary>
                            /// This class is used as part of a [[instance.TypeName]]KeyFrameCollection in
                            /// conjunction with a KeyFrame[[instance.TypeName]]Animation to animate a
                            /// [[instance.TypeName]] property value along a set of key frames.
                            ///
                            /// This [[instance.TypeName]]KeyFrame interpolates the between the [[instance.TypeName]] Value of
                            /// the previous key frame and its own Value Linearly with an EasingFunction to produce its output value.
                            /// </summary>
                            public partial class Easing[[instance.TypeName]]KeyFrame : [[instance.TypeName]]KeyFrame
                            {
                                #region Constructors
                                
                                /// <summary>
                                /// Creates a new Easing[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Easing[[instance.TypeName]]KeyFrame()
                                    : base()
                                {
                                }
                                
                                /// <summary>
                                /// Creates a new Easing[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Easing[[instance.TypeName]]KeyFrame([[instance.TypeName]] value)
                                    : this()
                                {
                                    Value = value;
                                }
                                
                                /// <summary>
                                /// Creates a new Easing[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Easing[[instance.TypeName]]KeyFrame([[instance.TypeName]] value, KeyTime keyTime)
                                    : this()
                                {
                                    Value = value;
                                    KeyTime = keyTime;
                                }

                                /// <summary>
                                /// Creates a new Easing[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Easing[[instance.TypeName]]KeyFrame([[instance.TypeName]] value, KeyTime keyTime, IEasingFunction easingFunction)
                                    : this()
                                {
                                    Value = value;
                                    KeyTime = keyTime;
                                    EasingFunction = easingFunction;
                                }
                                
                                #endregion
                                
                                #region Freezable
                                
                                /// <summary>
                                /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                                /// </summary>
                                /// <returns>The new Freezable.</returns>
                                protected override Freezable CreateInstanceCore()
                                {
                                    return new Easing[[instance.TypeName]]KeyFrame();
                                }
                                        
                                #endregion
                                
                                #region [[instance.TypeName]]KeyFrame
                                
                                /// <summary>
                                /// Implemented to Easingly interpolate between the baseValue and the
                                /// Value of this KeyFrame using the keyFrameProgress.
                                /// </summary>
                                protected override [[instance.TypeName]] InterpolateValueCore([[instance.TypeName]] baseValue, double keyFrameProgress)
                                {
                                    IEasingFunction easingFunction = EasingFunction;
                                    if (easingFunction != null)
                                    {
                                        keyFrameProgress = easingFunction.Ease(keyFrameProgress);
                                    }

                                    if (keyFrameProgress == 0.0)
                                    {
                                        return baseValue;
                                    }
                                    else if (keyFrameProgress == 1.0)
                                    {
                                        return Value;
                                    }
                                    else
                                    {
                                        return AnimatedTypeHelpers.Interpolate[[instance.TypeName]](baseValue, Value, keyFrameProgress[[extraInterpolateArgs]]);
                                    }
                                }
                                
                                #endregion

                                #region Public Properties

                                    /// <summary>
                                    /// EasingFunctionProperty
                                    /// </summary>                                 
                                    public static readonly DependencyProperty EasingFunctionProperty =
                                        DependencyProperty.Register(
                                            "EasingFunction",
                                            typeof(IEasingFunction),
                                            typeof(Easing[[instance.TypeName]]KeyFrame));

                                    /// <summary>
                                    /// EasingFunction
                                    /// </summary>
                                    public IEasingFunction EasingFunction                
                                    {
                                        get
                                        {
                                            return (IEasingFunction)GetValue(EasingFunctionProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(EasingFunctionProperty, value);
                                        }
                                    }

                                #endregion
                            }
                    [[/inline]]
                    );
                
            }

            // Done writing the last module; close the file
            CloseFile(ref csFile);
        }

        // Creates a new file for the module and writes the the beginning of the file (comment header, 
        // using statements, namespace declaration, etc
        private void ProcessNewModule(ResourceModel resourceModel, string moduleName, ref FileCodeSink csFile)
        {
            string fileName = "EasingKeyFrames.cs";
            string path = null;
            string fullPath = null;
            string moduleReference = null;

            //
            // Create a new file
            //

            path = "src\\" + moduleName + "\\System\\Windows\\Media\\Animation\\Generated";
            fullPath = Path.Combine(resourceModel.OutputDirectory, path);

            // Duplicate AnimatedTypeHelpers class across Core/Framework causes name conflicts,
            // requiring that they be split across two namespaces.
            switch (moduleName)
            {
                case @"Core\CSharp":
                    moduleReference = "using MS.Internal.PresentationCore;";
                    break;
                case "Framework":
                    moduleReference = "using MS.Internal.PresentationFramework;";
                    break;
            }

            csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */);

            //
            //  Write the file preamble
            //

            csFile.WriteBlock(
                [[inline]]
                    [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]

                    using MS.Internal;

                    using System;
                    using System.Collections;
                    using System.ComponentModel;
                    using System.Diagnostics;
                    using System.Windows.Media;
                    using System.Windows.Media.Media3D;

                    [[moduleReference]]

                    namespace System.Windows.Media.Animation
                    {
                [[/inline]]
                );

        }

        private void CloseFile(ref FileCodeSink csFile)
        {
            if (csFile != null)
            {
                // Write the closing brace of the namespace block
                csFile.WriteBlock("}");
                csFile.Dispose();
                csFile = null;
            }
        }

        private List<EasingKeyFrameTemplateInstance> Instances = new List<EasingKeyFrameTemplateInstance>();
    }
}


