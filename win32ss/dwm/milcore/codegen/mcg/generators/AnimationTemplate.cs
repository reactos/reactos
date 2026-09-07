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

using MS.Internal.MilCodeGen;
using MS.Internal.MilCodeGen.Runtime;
using MS.Internal.MilCodeGen.ResourceModel;
using MS.Internal.MilCodeGen.Helpers;

namespace MS.Internal.MilCodeGen.ResourceModel
{
    /// <summary>
    /// AnimationTemplate: This class represents one instantiation of the Animation template.
    /// </summary>
    public partial class AnimationTemplate: Template
    {
        private struct AnimationTemplateInstance
        {
            public AnimationTemplateInstance(
                string moduleName,
                string typeName,
                bool isValueType,
                string newDefaultValue
                )
            {
                ModuleName = moduleName;
                TypeName = typeName;
                IsValueType = isValueType;
                NewDefaultValue = newDefaultValue;
            }

            public string ModuleName;
            public string TypeName;
            public bool IsValueType;
            public string NewDefaultValue;
        }

        /// <summary>
        /// AddTemplateInstance - This is called by the code which parses the generation control.
        /// It is called on each TemplateInstance XMLNode encountered.
        /// </summary>
        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new AnimationTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName"),
                resourceModel.FindType(ResourceModel.ToString(node, "TypeName")).IsValueType,
                ResourceModel.ToString(node, "NewDefaultValue")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (AnimationTemplateInstance instance in Instances)
            {
                string fileName = instance.TypeName + "Animation" + ".cs";
                string path = "src\\" + instance.ModuleName + "\\System\\Windows\\Media\\Animation\\Generated";

                string fullPath = Path.Combine(resourceModel.OutputDirectory, path);

                string moduleReference = "";
                string extraInterpolateArgs = "";
                string checkNotNull;
                string propertyTypeName;  // the actual type for the animation (e.g. Double? for DoubleAnimation)
                string nullableAccessor;  // .Value for accessing value types, nothing for reference types


                // AnimatedTypeHelpers.Interpolate has an extra parameter
                // for the Quaternion type
                if (instance.TypeName == "Quaternion")
                {
                    extraInterpolateArgs = ", UseShortestPath";
                }

                // Duplicate AnimatedTypeHelpers class across Core/Framework causes name conflicts,
                // requiring that they be split across two namespaces.
                switch (instance.ModuleName)
                {
                    case @"Core\CSharp":
                        moduleReference = "using MS.Internal.PresentationCore;";
                        break;
                    case "Framework":
                        moduleReference = "using MS.Internal.PresentationFramework;";
                        break;
                }
               
                if (instance.IsValueType)
                {
                    checkNotNull = ".HasValue";
                    propertyTypeName = instance.TypeName + "?";
                    nullableAccessor = ".Value";
                }
                else
                {
                    checkNotNull = " != null";
                    propertyTypeName = instance.TypeName;
                    nullableAccessor = "";
                }

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]

                            using MS.Internal;
                            using MS.Internal.KnownBoxes;
                            using MS.Utility;

                            using System;
                            using System.Collections;
                            using System.ComponentModel;
                            using System.Diagnostics;
                            using System.Globalization;
                            using System.Runtime.InteropServices;
                            using System.Windows.Media;
                            using System.Windows.Media.Media3D;
                            using System.Windows.Media.Animation;                 

                            [[moduleReference]]

                            namespace System.Windows.Media.Animation
                            {       
                               
                                /// <summary>
                                /// Animates the value of a [[instance.TypeName]] property using linear interpolation
                                /// between two values.  The values are determined by the combination of
                                /// From, To, or By values that are set on the animation.
                                /// </summary>
                                public partial class [[instance.TypeName]]Animation : 
                                    [[instance.TypeName]]AnimationBase
                                {
                                    #region Data

                                    /// <summary>
                                    /// This is used if the user has specified From, To, and/or By values.
                                    /// </summary>
                                    private [[instance.TypeName]][] _keyValues;

                                    private AnimationType _animationType;        
                                    private bool _isAnimationFunctionValid;

                                    #endregion

                                    #region Constructors
                                    
                                    /// <summary>
                                    /// Static ctor for [[instance.TypeName]]Animation establishes
                                    /// dependency properties, using as much shared data as possible.
                                    /// </summary>
                                    static [[instance.TypeName]]Animation()
                                    {
                                        Type typeofProp = typeof([[propertyTypeName]]);
                                        Type typeofThis = typeof([[instance.TypeName]]Animation);
                                        PropertyChangedCallback propCallback = new PropertyChangedCallback(AnimationFunction_Changed);
                                        ValidateValueCallback validateCallback = new ValidateValueCallback(ValidateFromToOrByValue);
                                        
                                        FromProperty = DependencyProperty.Register(
                                            "From",
                                            typeofProp,
                                            typeofThis,
                                            new PropertyMetadata(([[propertyTypeName]])null, propCallback),
                                            validateCallback);
                                        
                                        ToProperty = DependencyProperty.Register(
                                            "To",
                                            typeofProp,
                                            typeofThis,
                                            new PropertyMetadata(([[propertyTypeName]])null, propCallback),
                                            validateCallback);
                                        
                                        ByProperty = DependencyProperty.Register(
                                            "By",
                                            typeofProp,
                                            typeofThis,
                                            new PropertyMetadata(([[propertyTypeName]])null, propCallback),
                                            validateCallback);

                                        EasingFunctionProperty = DependencyProperty.Register(
                                            "EasingFunction",
                                            typeof(IEasingFunction),
                                            typeofThis);
                                    }
                                    

                                    /// <summary>
                                    /// Creates a new [[instance.TypeName]]Animation with all properties set to
                                    /// their default values.
                                    /// </summary>
                                    public [[instance.TypeName]]Animation()
                                        : base()
                                    {
                                    }
                                    
                                    /// <summary>
                                    /// Creates a new [[instance.TypeName]]Animation that will animate a
                                    /// [[instance.TypeName]] property from its base value to the value specified
                                    /// by the "toValue" parameter of this constructor.
                                    /// </summary>
                                    public [[instance.TypeName]]Animation([[instance.TypeName]] toValue, Duration duration)
                                        : this()
                                    {
                                        To = toValue;
                                        Duration = duration;
                                    }
                                    
                                    /// <summary>
                                    /// Creates a new [[instance.TypeName]]Animation that will animate a
                                    /// [[instance.TypeName]] property from its base value to the value specified
                                    /// by the "toValue" parameter of this constructor.
                                    /// </summary>
                                    public [[instance.TypeName]]Animation([[instance.TypeName]] toValue, Duration duration, FillBehavior fillBehavior)
                                        : this()
                                    {
                                        To = toValue;
                                        Duration = duration;
                                        FillBehavior = fillBehavior;
                                    }
                                    
                                    /// <summary>
                                    /// Creates a new [[instance.TypeName]]Animation that will animate a
                                    /// [[instance.TypeName]] property from the "fromValue" parameter of this constructor
                                    /// to the "toValue" parameter.
                                    /// </summary>
                                    public [[instance.TypeName]]Animation([[instance.TypeName]] fromValue, [[instance.TypeName]] toValue, Duration duration)
                                        : this()
                                    {
                                        From = fromValue;
                                        To = toValue;
                                        Duration = duration;
                                    }
                                    
                                    /// <summary>
                                    /// Creates a new [[instance.TypeName]]Animation that will animate a
                                    /// [[instance.TypeName]] property from the "fromValue" parameter of this constructor
                                    /// to the "toValue" parameter.
                                    /// </summary>
                                    public [[instance.TypeName]]Animation([[instance.TypeName]] fromValue, [[instance.TypeName]] toValue, Duration duration, FillBehavior fillBehavior)
                                        : this()
                                    {
                                        From = fromValue;
                                        To = toValue;
                                        Duration = duration;
                                        FillBehavior = fillBehavior;
                                    }

                                    #endregion

                                    #region Freezable
                                    
                                    /// <summary>
                                    /// Creates a copy of this [[instance.TypeName]]Animation
                                    /// </summary>
                                    /// <returns>The copy</returns>
                                    public new [[instance.TypeName]]Animation Clone()
                                    {
                                        return ([[instance.TypeName]]Animation)base.Clone();
                                    }

                                    //
                                    // Note that we don't override the Clone virtuals (CloneCore, CloneCurrentValueCore,
                                    // GetAsFrozenCore, and GetCurrentValueAsFrozenCore) even though this class has state
                                    // not stored in a DP.
                                    // 
                                    // We don't need to clone _animationType and _keyValues because they are the the cached 
                                    // results of animation function validation, which can be recomputed.  The other remaining
                                    // field, isAnimationFunctionValid, defaults to false, which causes this recomputation to happen.
                                    //
                                   
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                                    /// </summary>
                                    /// <returns>The new Freezable.</returns>
                                    protected override Freezable CreateInstanceCore()
                                    {
                                        return new [[instance.TypeName]]Animation();
                                    }

                                    #endregion
                                    
                                    #region Methods

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
                                    protected override [[instance.TypeName]] GetCurrentValueCore([[instance.TypeName]] defaultOriginValue, [[instance.TypeName]] defaultDestinationValue, AnimationClock animationClock)
                                    {
                                        Debug.Assert(animationClock.CurrentState != ClockState.Stopped);
                                        
                                        if (!_isAnimationFunctionValid)
                                        {
                                            ValidateAnimationFunction();
                                        }
                                        
                                        double progress = animationClock.CurrentProgress.Value;

                                        IEasingFunction easingFunction = EasingFunction;
                                        if (easingFunction != null)
                                        {
                                            progress = easingFunction.Ease(progress);
                                        }

                                        [[instance.TypeName]]   from        = [[instance.NewDefaultValue]];
                                        [[instance.TypeName]]   to          = [[instance.NewDefaultValue]];
                                        [[instance.TypeName]]   accumulated = [[instance.NewDefaultValue]];
                                        [[instance.TypeName]]   foundation  = [[instance.NewDefaultValue]];

                                        // need to validate the default origin and destination values if 
                                        // the animation uses them as the from, to, or foundation values
                                        bool validateOrigin = false;
                                        bool validateDestination = false;

                                        switch(_animationType)
                                        {
                                            case AnimationType.Automatic:
                                            
                                                from    = defaultOriginValue;
                                                to      = defaultDestinationValue;

                                                validateOrigin = true;
                                                validateDestination = true;
                                                
                                                break;
                                                
                                            case AnimationType.From:
                                            
                                                from    = _keyValues[0];
                                                to      = defaultDestinationValue;

                                                validateDestination = true;

                                                break;

                                            case AnimationType.To:
                                            
                                                from = defaultOriginValue;
                                                to = _keyValues[0];

                                                validateOrigin = true;
                                                
                                                break;

                                            case AnimationType.By:
                                            
                                                // According to the SMIL specification, a By animation is
                                                // always additive.  But we don't force this so that a
                                                // user can re-use a By animation and have it replace the
                                                // animations that precede it in the list without having
                                                // to manually set the From value to the base value.
                                                
                                                to          = _keyValues[0];
                                                foundation  = defaultOriginValue;

                                                validateOrigin = true;
                                                
                                                break;

                                            case AnimationType.FromTo:
                                            
                                                from    = _keyValues[0];
                                                to      = _keyValues[1];
                                                
                                                if (IsAdditive)
                                                {
                                                    foundation = defaultOriginValue;
                                                    validateOrigin = true;
                                                }
                                                
                                                break;

                                            case AnimationType.FromBy:
                                            
                                                from    = _keyValues[0];
                                                to      = AnimatedTypeHelpers.Add[[instance.TypeName]](_keyValues[0], _keyValues[1]);
                                                
                                                if (IsAdditive)
                                                {
                                                    foundation = defaultOriginValue;
                                                    validateOrigin = true;
                                                }
                                                
                                                break;

                                            default:
                                            
                                                Debug.Fail("Unknown animation type.");
                                                
                                                break;
                                        }

                                        if (validateOrigin 
                                            && !AnimatedTypeHelpers.IsValidAnimationValue[[instance.TypeName]](defaultOriginValue))
                                        {
                                            throw new InvalidOperationException(
                                                SR.Get(
                                                    SRID.Animation_Invalid_DefaultValue,
                                                    this.GetType(),
                                                    "origin",
                                                    defaultOriginValue.ToString(CultureInfo.InvariantCulture)));
                                        }

                                        if (validateDestination 
                                            && !AnimatedTypeHelpers.IsValidAnimationValue[[instance.TypeName]](defaultDestinationValue))
                                        {
                                            throw new InvalidOperationException(
                                                SR.Get(
                                                    SRID.Animation_Invalid_DefaultValue,
                                                    this.GetType(),
                                                    "destination",
                                                    defaultDestinationValue.ToString(CultureInfo.InvariantCulture)));
                                        }


                                        if (IsCumulative)
                                        {
                                            double currentRepeat = (double)(animationClock.CurrentIteration - 1);
                                            
                                            if (currentRepeat > 0.0)
                                            {
                                                [[instance.TypeName]] accumulator = AnimatedTypeHelpers.Subtract[[instance.TypeName]](to, from);
                                              
                                                accumulated = AnimatedTypeHelpers.Scale[[instance.TypeName]](accumulator, currentRepeat);
                                            }
                                        }
                               
                                        // return foundation + accumulated + from + ((to - from) * progress)
                                        
                                        return AnimatedTypeHelpers.Add[[instance.TypeName]](
                                            foundation, 
                                            AnimatedTypeHelpers.Add[[instance.TypeName]](
                                                accumulated,
                                                AnimatedTypeHelpers.Interpolate[[instance.TypeName]](from, to, progress[[extraInterpolateArgs]])));
                                    }

                                    private void ValidateAnimationFunction()
                                    {
                                        _animationType = AnimationType.Automatic;
                                        _keyValues = null;
                                        
                                        if (From[[checkNotNull]])
                                        {
                                            if (To[[checkNotNull]])
                                            {
                                                _animationType = AnimationType.FromTo;
                                                _keyValues = new [[instance.TypeName]][2];
                                                _keyValues[0] = From[[nullableAccessor]];
                                                _keyValues[1] = To[[nullableAccessor]];
                                            }
                                            else if (By[[checkNotNull]])
                                            {
                                                _animationType = AnimationType.FromBy;
                                                _keyValues = new [[instance.TypeName]][2];
                                                _keyValues[0] = From[[nullableAccessor]];
                                                _keyValues[1] = By[[nullableAccessor]];
                                            }
                                            else
                                            {
                                                _animationType = AnimationType.From;
                                                _keyValues = new [[instance.TypeName]][1];
                                                _keyValues[0] = From[[nullableAccessor]];
                                            }
                                        }
                                        else if (To[[checkNotNull]])
                                        {
                                            _animationType = AnimationType.To;
                                            _keyValues = new [[instance.TypeName]][1];
                                            _keyValues[0] = To[[nullableAccessor]];
                                        }
                                        else if (By[[checkNotNull]])
                                        {
                                            _animationType = AnimationType.By;
                                            _keyValues = new [[instance.TypeName]][1];
                                            _keyValues[0] = By[[nullableAccessor]];
                                        }

                                        _isAnimationFunctionValid = true;
                                    }
                                    
                                    #endregion

                                    #region Properties

                                    private static void AnimationFunction_Changed(DependencyObject d, DependencyPropertyChangedEventArgs e)
                                    {
                                        [[instance.TypeName]]Animation a = ([[instance.TypeName]]Animation)d;

                                        a._isAnimationFunctionValid = false;
                                        a.PropertyChanged(e.Property);
                                    }

                                    private static bool ValidateFromToOrByValue(object value)
                                    {
                                        [[propertyTypeName]] typedValue = ([[propertyTypeName]])value;
                                        
                                        if (typedValue[[checkNotNull]])
                                        {
                                            return AnimatedTypeHelpers.IsValidAnimationValue[[instance.TypeName]](typedValue[[nullableAccessor]]);
                                        }
                                        else
                                        {
                                            return true;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// FromProperty
                                    /// </summary>                                 
                                    public static readonly DependencyProperty FromProperty;

                                    /// <summary>
                                    /// From
                                    /// </summary>
                                    public [[propertyTypeName]] From                
                                    {
                                        get
                                        {
                                            return ([[propertyTypeName]])GetValue(FromProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(FromProperty, value);
                                        }
                                    }

                                    /// <summary>
                                    /// ToProperty
                                    /// </summary>
                                    public static readonly DependencyProperty ToProperty;

                                    /// <summary>
                                    /// To
                                    /// </summary>
                                    public [[propertyTypeName]] To                
                                    {
                                        get
                                        {
                                            return ([[propertyTypeName]])GetValue(ToProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(ToProperty, value);
                                        }
                                    }

                                    /// <summary>
                                    /// ByProperty
                                    /// </summary>
                                    public static readonly DependencyProperty ByProperty;
                                    
                                    /// <summary>
                                    /// By
                                    /// </summary>
                                    public [[propertyTypeName]] By                
                                    {
                                        get
                                        {
                                            return ([[propertyTypeName]])GetValue(ByProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(ByProperty, value);
                                        }
                                    }


                                    /// <summary>
                                    /// EasingFunctionProperty
                                    /// </summary>                                 
                                    public static readonly DependencyProperty EasingFunctionProperty;

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

                                    /// <summary>
                                    /// If this property is set to true the animation will add its value to
                                    /// the base value instead of replacing it entirely.
                                    /// </summary>
                                    public bool IsAdditive         
                                    { 
                                        get
                                        {
                                            return (bool)GetValue(IsAdditiveProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(IsAdditiveProperty, BooleanBoxes.Box(value));
                                        }
                                    }

                                    /// <summary>
                                    /// It this property is set to true, the animation will accumulate its
                                    /// value over repeats.  For instance if you have a From value of 0.0 and
                                    /// a To value of 1.0, the animation return values from 1.0 to 2.0 over
                                    /// the second reteat cycle, and 2.0 to 3.0 over the third, etc.
                                    /// </summary>
                                    public bool IsCumulative      
                                    { 
                                        get
                                        {
                                            return (bool)GetValue(IsCumulativeProperty);
                                        }
                                        set
                                        {
                                            SetValueInternal(IsCumulativeProperty, BooleanBoxes.Box(value));
                                        }
                                    }

                                    #endregion
                                }
                            }
                        [[/inline]]
                        );
                }
            }
        }


        private List<AnimationTemplateInstance> Instances = new List<AnimationTemplateInstance>();
    }
}


