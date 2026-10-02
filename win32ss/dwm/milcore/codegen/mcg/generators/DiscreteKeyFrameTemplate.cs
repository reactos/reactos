// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of 
//              the type-specific discrete keyframes (DiscreteByteKeyFrame, etc)
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
    /// DiscreteKeyFrameTemplate: This class represents one instantiation of the DiscreteKeyFrame template.
    /// Due to a limitation of the build system, DiscreteKeyFrame classes are coalesced into one file
    /// per module.
    /// </summary>
    public class DiscreteKeyFrameTemplate: Template
    {
        private struct DiscreteKeyFrameTemplateInstance
        {
            public DiscreteKeyFrameTemplateInstance(
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
            Instances.Add(new DiscreteKeyFrameTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            FileCodeSink csFile = null;
            string currentModuleName = null;

            foreach (DiscreteKeyFrameTemplateInstance instance in Instances)
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
                            ///
                            /// This [[instance.TypeName]]KeyFrame changes from the [[instance.TypeName]] Value of
                            /// the previous key frame to its own Value without interpolation.  The
                            /// change occurs at the KeyTime.
                            /// </summary>
                            public class Discrete[[instance.TypeName]]KeyFrame : [[instance.TypeName]]KeyFrame
                            {
                                #region Constructors
                                
                                /// <summary>
                                /// Creates a new Discrete[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Discrete[[instance.TypeName]]KeyFrame()
                                    : base()
                                {
                                }
                                
                                /// <summary>
                                /// Creates a new Discrete[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Discrete[[instance.TypeName]]KeyFrame([[instance.TypeName]] value)
                                    : base(value)
                                {
                                }
                                
                                /// <summary>
                                /// Creates a new Discrete[[instance.TypeName]]KeyFrame.
                                /// </summary>
                                public Discrete[[instance.TypeName]]KeyFrame([[instance.TypeName]] value, KeyTime keyTime)
                                    : base(value, keyTime)
                                {
                                }
                                
                                #endregion
                                
                                #region Freezable
                                
                                /// <summary>
                                /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                                /// </summary>
                                /// <returns>The new Freezable.</returns>
                                protected override Freezable CreateInstanceCore()
                                {
                                    return new Discrete[[instance.TypeName]]KeyFrame();
                                }

                                // We don't need to override CloneCore because it won't do anything
                                        
                                #endregion
                                
                                #region [[instance.TypeName]]KeyFrame
                                
                                /// <summary>
                                /// Implemented to linearly interpolate between the baseValue and the
                                /// Value of this KeyFrame using the keyFrameProgress.
                                /// </summary>
                                protected override [[instance.TypeName]] InterpolateValueCore([[instance.TypeName]] baseValue, double keyFrameProgress)
                                {
                                    if (keyFrameProgress < 1.0)
                                    {
                                        return baseValue;
                                    }
                                    else
                                    {
                                        return Value;
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
            string fileName = "DiscreteKeyFrames.cs";
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

        private List<DiscreteKeyFrameTemplateInstance> Instances = new List<DiscreteKeyFrameTemplateInstance>();
    }
}


