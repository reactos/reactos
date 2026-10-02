// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of AnimationResources
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
    public class AnimationResourceTemplate: Template
    {
        private struct AnimationResourceTemplateInstance
        {
            public AnimationResourceTemplateInstance(
                string mangedDestinationDir,
                McgType typeName)
            {
                ManagedDestinationDir = mangedDestinationDir;
                TypeName = typeName;
            }

            public string ManagedDestinationDir;
            public McgType TypeName;
        }

        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new AnimationResourceTemplateInstance(
                ResourceModel.ToString(node, "ManagedDestinationDir"),
                resourceModel.FindType(ResourceModel.ToString(node, "TypeName"))));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (AnimationResourceTemplateInstance instance in Instances)
            {
                string fileName = instance.TypeName.Name + "IndependentAnimationStorage.cs";

                string fullPath = Path.Combine(resourceModel.OutputDirectory, instance.ManagedDestinationDir);

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    string milTypeName = instance.TypeName.ManagedName.ToUpper();

                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName, @"wpf\src\Graphics\codegen\mcg\generators\AnimationResourceTemplate.cs")]]

                            using System;
                            using MS.Internal;
                            using System.Diagnostics;
                            using System.Runtime.InteropServices;
                            using System.Threading;

                            using System.Windows.Media;
                            using System.Windows.Media.Composition;
                            using System.Windows.Media.Media3D;
                            using System.Security;
                            using System.Security.Permissions;

                            namespace System.Windows.Media.Animation
                            {
                                internal class [[instance.TypeName.Name]]IndependentAnimationStorage : IndependentAnimationStorage
                                {
                                    //
                                    // Method which returns the DUCE type of this class.
                                    // The base class needs this type when calling CreateOrAddRefOnChannel.
                                    // By providing this via a virtual, we avoid a per-instance storage cost.
                                    //
                                    protected override DUCE.ResourceType ResourceType
                                    {
                                        get
                                        {
                                            return DUCE.ResourceType.TYPE_[[milTypeName]]RESOURCE;
                                        }
                                    }

                                    /// <SecurityNote>
                                    ///    Critical: This code is critical because it has unsafe code blocks
                                    ///    TreatAsSafe: This call is ok to expose. Channels can handle bad pointers
                                    ///  </SecurityNote>
                                    [SecurityCritical,SecurityTreatAsSafe]
                                    protected override void UpdateResourceCore(DUCE.Channel channel)
                                    {
                                        Debug.Assert(_duceResource.IsOnChannel(channel));
                                        DependencyObject dobj = ((DependencyObject) _dependencyObject.Target);

                                        // The dependency object was GCed, nothing to do here
                                        if (dobj == null)
                                        {
                                            return;
                                        }

                                        [[instance.TypeName.Name]] tempValue = ([[instance.TypeName.Name]])dobj.GetValue(_dependencyProperty);

                                        DUCE.MILCMD_[[milTypeName]]RESOURCE data;
                                        data.Type = MILCMD.MilCmd[[instance.TypeName.Name]]Resource;
                                        data.Handle = _duceResource.GetHandle(channel);
                                        data.Value = [[CodeGenHelpers.ConvertToValueType(instance.TypeName, "tempValue")]];

                                        unsafe
                                        {
                                            channel.SendCommand(
                                                (byte*)&data,
                                                sizeof(DUCE.MILCMD_[[milTypeName]]RESOURCE));
                                        }
                                    }
                                }
                            }
                        [[/inline]]
                        );
                }
            }

        }

        private List<AnimationResourceTemplateInstance> Instances = new List<AnimationResourceTemplateInstance>();
    }
}


