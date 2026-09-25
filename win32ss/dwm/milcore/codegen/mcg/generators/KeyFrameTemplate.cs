// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of 
//              the type-specific keyframes (ByteKeyFrame, DoubleKeyFrame)
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
    /// KeyFrameTemplate: This class represents one instantiation of the KeyFrame template.
    /// Due to a limitation of the build system, KeyFrame classes are coalesced into one file
    /// per module.
    /// </summary>
    public class KeyFrameTemplate: Template
    {
        private struct KeyFrameTemplateInstance
        {
            public KeyFrameTemplateInstance(
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
            Instances.Add(new KeyFrameTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            FileCodeSink csFile = null;
            string currentModuleName = null;

            foreach (KeyFrameTemplateInstance instance in Instances)
            {

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


                //
                // Write the typed class for the current instance
                //

                csFile.WriteBlock(
                    [[inline]]
                        
                            /// <summary>
                            /// This class is used as part of a [[instance.TypeName]]KeyFrameCollection in
                            /// conjunction with a KeyFrame[[instance.TypeName]]Animation to animate a
                            /// [[instance.TypeName]] property value along a set of key frames.
                            /// </summary>
                            public abstract class [[instance.TypeName]]KeyFrame : Freezable, IKeyFrame
                            {
                                #region Constructors
                                
                                /// <summary>
                                /// Creates a new [[instance.TypeName]]KeyFrame.
                                /// </summary>
                                protected [[instance.TypeName]]KeyFrame()
                                    : base()
                                {
                                }
                                
                                /// <summary>
                                /// Creates a new [[instance.TypeName]]KeyFrame.
                                /// </summary>
                                protected [[instance.TypeName]]KeyFrame([[instance.TypeName]] value)
                                    : this()
                                {
                                    Value = value;
                                }
                                
                                /// <summary>
                                /// Creates a new Discrete[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                protected [[instance.TypeName]]KeyFrame([[instance.TypeName]] value, KeyTime keyTime)
                                    : this()
                                {
                                    Value = value;
                                    KeyTime = keyTime;
                                }
                                        
                                #endregion

                                #region IKeyFrame

                                /// <summary>
                                /// KeyTime Property
                                /// </summary>
                                public static readonly DependencyProperty KeyTimeProperty =
                                    DependencyProperty.Register(
                                            "KeyTime",
                                            typeof(KeyTime),
                                            typeof([[instance.TypeName]]KeyFrame),
                                            new PropertyMetadata(KeyTime.Uniform));
                                
                                /// <summary>
                                /// The time at which this KeyFrame's value should be equal to the Value
                                /// property.
                                /// </summary>
                                public KeyTime KeyTime
                                {
                                    get
                                    {
                                    return (KeyTime)GetValue(KeyTimeProperty);
                                    }
                                    set
                                    {
                                    SetValueInternal(KeyTimeProperty, value);
                                    }
                                }
                                
                                /// <summary>
                                /// Value Property
                                /// </summary>
                                public static readonly DependencyProperty ValueProperty =
                                    DependencyProperty.Register(
                                            "Value",
                                            typeof([[instance.TypeName]]),
                                            typeof([[instance.TypeName]]KeyFrame),
                                            new PropertyMetadata());

                                /// <summary>
                                /// The value of this key frame at the KeyTime specified.
                                /// </summary>
                                object IKeyFrame.Value
                                {
                                    get
                                    {
                                        return Value;
                                    }
                                    set
                                    {
                                        Value = ([[instance.TypeName]])value;
                                    }
                                }
                                
                                /// <summary>
                                /// The value of this key frame at the KeyTime specified.
                                /// </summary>
                                public [[instance.TypeName]] Value
                                {
                                    get
                                    {
                                        return ([[instance.TypeName]])GetValue(ValueProperty);
                                    }
                                    set
                                    {
                                        SetValueInternal(ValueProperty, value);
                                    }
                                }
                                
                                #endregion
                                
                                #region Public Methods
                                
                                /// <summary>
                                /// Gets the interpolated value of the key frame at the progress value
                                /// provided.  The progress value should be calculated in terms of this 
                                /// specific key frame.
                                /// </summary>
                                public [[instance.TypeName]] InterpolateValue(
                                    [[instance.TypeName]] baseValue, 
                                    double keyFrameProgress)
                                {
                                    if (   keyFrameProgress < 0.0
                                        || keyFrameProgress > 1.0)
                                    {
                                        throw new ArgumentOutOfRangeException("keyFrameProgress");
                                    }
                                    
                                    return InterpolateValueCore(baseValue, keyFrameProgress);
                                }
                                
                                #endregion
                                
                                #region Protected Methods
                                
                                /// <summary>
                                /// This method should be implemented by derived classes to calculate
                                /// the value of this key frame at the progress value provided.
                                /// </summary>
                                protected abstract [[instance.TypeName]] InterpolateValueCore(
                                    [[instance.TypeName]] baseValue,
                                    double keyFrameProgress);
                                
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
            string fileName = "KeyFrames.cs";
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

        private List<KeyFrameTemplateInstance> Instances = new List<KeyFrameTemplateInstance>();
    }
}


