// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of 
//              the type-specific animation classes (ByteAnimation, DoubleAnimation, etc)
//              
//---------------------------------------------------------------------------

using System;
using System.IO;
using System.Xml;
using System.Collections.Generic;
using System.Diagnostics;

using MS.Internal.MilCodeGen;
using MS.Internal.MilCodeGen.Runtime;
using MS.Internal.MilCodeGen.ResourceModel;
using MS.Internal.MilCodeGen.Helpers;

namespace MS.Internal.MilCodeGen.ResourceModel
{
    /// <summary>
    /// AnimationBaseTemplate: This class represents one instantiation of the Animation template.
    /// </summary>
    public class AnimationBaseTemplate: Template
    {
        private struct AnimationBaseTemplateInstance
        {
            public AnimationBaseTemplateInstance(
                string moduleName,
                string typeName,
                bool isValueType
                )
            {
                ModuleName = moduleName;
                TypeName = typeName;
                IsValueType = isValueType;
            }

            public string ModuleName;
            public string TypeName;
            public bool IsValueType;
        }

        /// <summary>
        /// AddTemplateInstance - This is called by the code which parses the generation control.
        /// It is called on each TemplateInstance XMLNode encountered.
        /// </summary>
        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new AnimationBaseTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName"),
                resourceModel.FindType(ResourceModel.ToString(node, "TypeName")).IsValueType));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (AnimationBaseTemplateInstance instance in Instances)
            {
                string fileName = instance.TypeName + "AnimationBase" + ".cs";
                string path = "src\\" + instance.ModuleName + "\\System\\Windows\\Media\\Animation\\Generated";

                string fullPath = Path.Combine(resourceModel.OutputDirectory, path);

                string moduleReference;
                string sridReference = 
                    [[inline]]
                        using SR=System.Windows.SR;
                        using SRID=System.Windows.SRID;
                    [[/inline]];

                // Duplicate AnimatedTypeHelpers class across Core/Framework causes name conflicts,
                // requiring that they be split across two namespaces.
                switch (instance.ModuleName)
                {
                    case @"Core\CSharp":
                        moduleReference = "using MS.Internal.PresentationCore;";
                        sridReference = 
                            [[inline]]
                                using SR=MS.Internal.PresentationCore.SR;
                                using SRID=MS.Internal.PresentationCore.SRID;
                            [[/inline]];
                        break;
                    case "Framework":
                        moduleReference = "using MS.Internal.PresentationFramework;";
                        break;
                    default:
                        moduleReference = "";
                        break;
                }
               

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]

                            // Allow use of presharp: #pragma warning suppress <nnnn>
                            #pragma warning disable 1634, 1691

                            using MS.Internal;

                            using System;
                            using System.Collections;
                            using System.ComponentModel;
                            using System.Diagnostics;
                            using System.Windows.Media.Animation;   
                            using System.Windows.Media.Media3D;
              
                            [[moduleReference]]

                            [[sridReference]]

                            namespace System.Windows.Media.Animation
                            {       
                                /// <summary>
                                ///
                                /// </summary>
                                public abstract class [[instance.TypeName]]AnimationBase : AnimationTimeline
                                {
                                    #region Constructors

                                    /// <Summary>
                                    /// Creates a new [[instance.TypeName]]AnimationBase.
                                    /// </Summary>
                                    protected [[instance.TypeName]]AnimationBase()
                                        : base()
                                    {
                                    }
                                    
                                    #endregion
                                    
                                    #region Freezable
                                    
                                    /// <summary>
                                    /// Creates a copy of this [[instance.TypeName]]AnimationBase
                                    /// </summary>
                                    /// <returns>The copy</returns>
                                    public new [[instance.TypeName]]AnimationBase Clone()
                                    {
                                        return ([[instance.TypeName]]AnimationBase)base.Clone();
                                    }
                                    
                                    #endregion
                                    
                                    #region IAnimation

                                    /// <summary>
                                    /// Calculates the value this animation believes should be the current value for the property.
                                    /// </summary>
                                    /// <param name="defaultOriginValue">
                                    /// This value is the suggested origin value provided to the animation
                                    /// to be used if the animation does not have its own concept of a
                                    /// start value. If this animation is the first in a composition chain
                                    /// this value will be the snapshot value if one is available or the
                                    /// base property value if it is not; otherise this value will be the 
                                    /// value returned by the previous animation in the chain with an 
                                    /// animationClock that is not Stopped.
                                    /// </param>
                                    /// <param name="defaultDestinationValue">
                                    /// This value is the suggested destination value provided to the animation
                                    /// to be used if the animation does not have its own concept of an
                                    /// end value. This value will be the base value if the animation is
                                    /// in the first composition layer of animations on a property; 
                                    /// otherwise this value will be the output value from the previous 
                                    /// composition layer of animations for the property.
                                    /// </param>
                                    /// <param name="animationClock">
                                    /// This is the animationClock which can generate the CurrentTime or
                                    /// CurrentProgress value to be used by the animation to generate its
                                    /// output value.
                                    /// </param>
                                    /// <returns>
                                    /// The value this animation believes should be the current value for the property.
                                    /// </returns>
                                    public override sealed object GetCurrentValue(object defaultOriginValue, object defaultDestinationValue, AnimationClock animationClock)
                                    {
                                        [[WriteIAnimationGetCurrentValueBody(instance)]]
                                    }
                                    
                                    /// <summary>
                                    /// Returns the type of the target property
                                    /// </summary>
                                    public override sealed Type TargetPropertyType
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return typeof([[instance.TypeName]]);
                                        }
                                    }
                                    
                                    #endregion

                                    #region Methods
                                    
                                    [[conditional(instance.TypeName != "Object")]]
                                    /// <summary>
                                    /// Calculates the value this animation believes should be the current value for the property.
                                    /// </summary>
                                    /// <param name="defaultOriginValue">
                                    /// This value is the suggested origin value provided to the animation
                                    /// to be used if the animation does not have its own concept of a
                                    /// start value. If this animation is the first in a composition chain
                                    /// this value will be the snapshot value if one is available or the
                                    /// base property value if it is not; otherise this value will be the 
                                    /// value returned by the previous animation in the chain with an 
                                    /// animationClock that is not Stopped.
                                    /// </param>
                                    /// <param name="defaultDestinationValue">
                                    /// This value is the suggested destination value provided to the animation
                                    /// to be used if the animation does not have its own concept of an
                                    /// end value. This value will be the base value if the animation is
                                    /// in the first composition layer of animations on a property; 
                                    /// otherwise this value will be the output value from the previous 
                                    /// composition layer of animations for the property.
                                    /// </param>
                                    /// <param name="animationClock">
                                    /// This is the animationClock which can generate the CurrentTime or
                                    /// CurrentProgress value to be used by the animation to generate its
                                    /// output value.
                                    /// </param>
                                    /// <returns>
                                    /// The value this animation believes should be the current value for the property.
                                    /// </returns>
                                    public [[instance.TypeName]] GetCurrentValue([[instance.TypeName]] defaultOriginValue, [[instance.TypeName]] defaultDestinationValue, AnimationClock animationClock)
                                    {
                                        [[WriteTypedGetCurrentValueBody(instance)]]
                                    }
                                    [[/conditional]]

                                    /// <summary>
                                    /// Calculates the value this animation believes should be the current value for the property.
                                    /// </summary>
                                    /// <param name="defaultOriginValue">
                                    /// This value is the suggested origin value provided to the animation
                                    /// to be used if the animation does not have its own concept of a
                                    /// start value. If this animation is the first in a composition chain
                                    /// this value will be the snapshot value if one is available or the
                                    /// base property value if it is not; otherise this value will be the 
                                    /// value returned by the previous animation in the chain with an 
                                    /// animationClock that is not Stopped.
                                    /// </param>
                                    /// <param name="defaultDestinationValue">
                                    /// This value is the suggested destination value provided to the animation
                                    /// to be used if the animation does not have its own concept of an
                                    /// end value. This value will be the base value if the animation is
                                    /// in the first composition layer of animations on a property; 
                                    /// otherwise this value will be the output value from the previous 
                                    /// composition layer of animations for the property.
                                    /// </param>
                                    /// <param name="animationClock">
                                    /// This is the animationClock which can generate the CurrentTime or
                                    /// CurrentProgress value to be used by the animation to generate its
                                    /// output value.
                                    /// </param>
                                    /// <returns>
                                    /// The value this animation believes should be the current value for the property.
                                    /// </returns>
                                    protected abstract [[instance.TypeName]] GetCurrentValueCore([[instance.TypeName]] defaultOriginValue, [[instance.TypeName]] defaultDestinationValue, AnimationClock animationClock);
                                    
                                    #endregion
                                }
                            }
                        [[/inline]]
                        );
                }
            }
        }

        //
        // Generates the body of IAnimation.GetCurrentValue(object defaultOriginValue, object defaultDestinationValue, AnimationClock animationClock)
        //
        // Object will generate the strongly-typed GetCurrentValue implementation here while
        // other types will simply forward to their strongly-typed implementation.
        private string WriteIAnimationGetCurrentValueBody(AnimationBaseTemplateInstance instance)
        {
            if (instance.TypeName.ToLower() == "object")
            {
                Debug.Assert(!instance.IsValueType);
                return WriteTypedGetCurrentValueBody(instance);
            }
            else
            {
                string nullCheck = String.Empty;

                if (instance.IsValueType)
                {
                    nullCheck = 
                        [[inline]]
                            // Verify that object arguments are non-null since we are a value type
                            if (defaultOriginValue == null)
                            {
                                throw new ArgumentNullException("defaultOriginValue");
                            }
                            if (defaultDestinationValue == null)
                            {
                                throw new ArgumentNullException("defaultDestinationValue");
                            }
                        [[/inline]];
                }

                return 
                    nullCheck +
                        [[inline]]
                            return GetCurrentValue(([[instance.TypeName]])defaultOriginValue, ([[instance.TypeName]])defaultDestinationValue, animationClock);
                        [[/inline]];
            } 
        }

        //
        // Generates the body of GetCurrentValue([type] defaultOriginValue, [type] defaultDestinationValue, AnimationClock animationClock)
        //
        private string WriteTypedGetCurrentValueBody(AnimationBaseTemplateInstance instance)
        {
            return 
                [[inline]]
                    ReadPreamble();
                    
                    if (animationClock == null)
                    {
                        throw new ArgumentNullException("animationClock");
                    }
                    
                    // We check for null above but presharp doesn't notice so we suppress the 
                    // warning here.
                    
                    #pragma warning suppress 6506
                    if (animationClock.CurrentState == ClockState.Stopped)
                    {
                        return defaultDestinationValue;
                    }
                    
                    /*
                    if (!AnimatedTypeHelpers.IsValidAnimationValue[[instance.TypeName]](defaultDestinationValue))
                    {
                        throw new ArgumentException(
                            SR.Get(
                                SRID.Animation_InvalidBaseValue,
                                defaultDestinationValue, 
                                defaultDestinationValue.GetType(), 
                                GetType()),
                                "defaultDestinationValue");
                    }
                    */
                    
                    return GetCurrentValueCore(defaultOriginValue, defaultDestinationValue, animationClock);
                [[/inline]];
        }


        private List<AnimationBaseTemplateInstance> Instances = new List<AnimationBaseTemplateInstance>();
    }
}


