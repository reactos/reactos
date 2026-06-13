// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL resources.
//              Built from an XML description.
//
//------------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.ResourceModel
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Xml;
    using MS.Internal.MilCodeGen;


    //------------------------------------------------------
    //
    //  Class: McgResource
    //
    //------------------------------------------------------

    public class McgResource : McgStruct
    {
        internal McgResource(
            ResourceModel resourceModel,
            string name,
            string[] namespaces,
            string managedNamespace,
            string managedType,
            string unmanagedName,
            string unmanagedResourceAlias,
            string comment,
            bool isValueType,
            bool needsConvert,
            int marshaledSize,
            int marshaledAlignment,
            bool skipProperties,
            bool skipToString,
            bool skipFields,
            bool isAlwaysSerializableAsString,
            bool isFreezable,
            bool canIntroduceCycles,
            bool isAnimatable,
            McgRealization realization,
            bool isAbstract,
            bool createInstanceCoreViaActivator,
            bool toInstanceDescriptor,
            string parseMethod,
            bool skipUpdate,
            bool useProcessUpdateWrapper,
            bool useOnChannelCoreWrapper,
            bool useStaticInitialize,
            bool hasUnmanagedResource,
            bool generateDefaultConstructor,
            bool generateSerializerAttribute,
            bool addCloneHooks,
            bool inlinedUnmanagedResource,
            bool callProcessUpdateCore,
            bool leaveUnsealed,
            string marshalledIdentity,
            bool collectionAllowsNullEntries,
            string pragmaPack
            ) : base(resourceModel, /* domain */ "Resource", /* target */ String.Empty,
                     name, managedType, unmanagedName, isValueType, needsConvert, isAlwaysSerializableAsString,
                     marshaledSize, marshaledAlignment, parseMethod)
        {
            // A resource cannot be both a Freezable and a ValueType
            if (isFreezable && isValueType)
            {
                 Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Error in type definition for: '{0}'.  IsValueType cannot be 'true' when IsFreezable is 'true'.",
                    name));
            }

            _isFreezable = isFreezable;
            _canIntroduceCycles = canIntroduceCycles;
            _isAnimatable = isAnimatable;
            Realization = realization;
            IsAbstract = isAbstract;
            CreateInstanceCoreViaActivator = createInstanceCoreViaActivator;
            SkipToString = skipToString;
            SkipProperties = skipProperties;
            SkipFields = skipFields || !isValueType;
            ToInstanceDescriptor = toInstanceDescriptor;
            SkipUpdate = skipUpdate;
            UseProcessUpdateWrapper = useProcessUpdateWrapper;
            UseOnChannelCoreWrapper = useOnChannelCoreWrapper;
            UseStaticInitialize = useStaticInitialize;
            HasUnmanagedResource = hasUnmanagedResource;
            GenerateDefaultConstructor = generateDefaultConstructor;
            GenerateSerializerAttribute = generateSerializerAttribute;
            AddCloneHooks = addCloneHooks;
            CallProcessUpdateCore = callProcessUpdateCore;
            UnmanagedResourceAlias = unmanagedResourceAlias;
            _inlinedUnmanagedResource = inlinedUnmanagedResource;

            _namespaces = namespaces;
            ManagedNamespace = managedNamespace;
            Comment = comment;
            _leaveUnsealed = leaveUnsealed;

            _marshalledIdentity = marshalledIdentity;
            _collectionAllowsNullEntries = collectionAllowsNullEntries;
            PragmaPack = pragmaPack;
        }

        public readonly McgRealization Realization;
        public readonly bool IsAbstract;
        public readonly bool CreateInstanceCoreViaActivator;
        public readonly bool SkipToString;
        public readonly bool SkipProperties;
        public readonly bool SkipFields;
        public readonly string ManagedNamespace;
        public readonly bool ToInstanceDescriptor;
        public readonly bool SkipUpdate;
        public readonly bool UseProcessUpdateWrapper;
        public readonly bool UseOnChannelCoreWrapper;
        public readonly bool UseStaticInitialize;
        public readonly bool HasUnmanagedResource;
        public readonly bool GenerateDefaultConstructor;
        public readonly bool GenerateSerializerAttribute;
        public readonly bool AddCloneHooks;
        public readonly bool CallProcessUpdateCore;
        public readonly string UnmanagedResourceAlias;
        public readonly string PragmaPack;

        private readonly bool _leaveUnsealed;

        public McgType CollectionType
        {
            get
            {
                return _collectionType;
            }
        }
        public readonly string Comment;

        public string EmptyClassName
        {
            get { return Name + EmptyField.Name; }
        }

        public bool HasDistinguishedEmpty
        {
            get
            {
                return IsValueType &&
                    EmptyField != null &&
                    EmptyField.Distinguished;
            }
        }

        public bool IsCollection
        {
            get
            {
                return _collectionType != null;
            }
        }

        public bool IsCollectionOfHandles
        {
            get
            {
                if (IsCollection)
                {
                    return CollectionType.UsesHandles;
                }
                else
                {
                    return false;
                }
            }
        }

        /// <summary>
        /// IsCollectionWhichAllowsNullEntries Property - bool
        /// If false, this means that it is illegal to add null to the collection.
        /// Defaults to true.
        /// This flag is currently used in ProcessUpdate to validate the collection packet.
        /// </summary>
        public bool IsCollectionWhichAllowsNullEntries
        {
            get
            {
                if (IsCollection)
                {
                    return _collectionAllowsNullEntries;
                }
                else
                {
                    return false;
                }
            }
        }

        public override bool IsAnimatable
        {
            get
            {
                Debug.Assert((!_isAnimatable) || IsFreezable,
                    "McgResource '" + Name + "' should now be animatable but not freezable.");
                return _isAnimatable;
            }
        }

        public override bool IsFreezable
        {
            get
            {
                return _isFreezable;
            }
        }

        public override bool CanIntroduceCycles
        {
            get
            {
                return _canIntroduceCycles;
            }
        }

        /// <summary>
        ///     Returns true if we should emit the sealed modifier in the class declaration.
        /// </summary>
        public bool IsSealed
        {
            get
            {
                // We prevent subclassing of abstract classes by making their ctor
                // internal.  We prevent subclassing of concrete classes by sealing them.
                //
                // There is no way in the C# language to prevent 3rd party subclassing
                // of concrete base classes.  For example:
                //
                //          Light (abstract)
                //             \
                //         PointLight (concrete)  <=  Can not be sealed, ctor must be public
                //               \
                //            SpotLight (concrete)
                //
                // In most cases the right solution is to refactor so that both concrete
                // classes inherit from an abstract base class.  For example:
                //
                //                Light (abstract)
                //                  |
                //           PointLightBase (abstract)
                //            /          \
                //       PointLight   Spotlight (concrete)
                //
                Debug.Assert(_leaveUnsealed || IsAbstract || IsFinal,
                    "We can not prevent subclassing of '" + Name + "'.  Fix the class hierarchy or explicitly declare LeaveUnsealed=\"true\".");

                // If the user explicitly specified LeaveUnsealed="true" in the XML this
                // overrides our default behavior.
                if (_leaveUnsealed)
                {
                    return false;
                }

                // We want to seal all classes which are at the leaves of our inheritence
                // hierarchy (IsFinal).
                //
                // !IsValueType because structs do not support inheritence and
                // therefore can not be sealed.
                //
                return IsFinal && !IsValueType;
            }
        }

        public string DuceClass
        {
            get
            {
                if (UnmanagedResourceAlias != String.Empty)
                {
                    if (IsValueType)
                    {
                        return "CMilSlave" + UnmanagedResourceAlias;
                    }
                    else
                    {
                        if (Name.EndsWith("Resource")
                            && Name != "EtwEventResource")
                        {
                            // valueres.h
                            return "CMilSlave" + Name.Substring(0, Name.Length - "Resource".Length);
                        }
                        else
                        {
                            return UnmanagedResourceAlias;
                        }
                    }
                }
                else
                {
                    if (IsValueType)
                    {
                        return "CMilSlave" + Name;
                    }
                    else
                    {
                        return "CMil" + Name + "Duce";
                    }
                }
            }
        }

        public string ShortDataName
        {
            get
            {
                if (IsValueType)
                {
                    // NOTE: We currently use Point3Ds for Vector3Ds on the UCE side.
                    //
                    if (Name == "Vector3D")
                    {
                        return "Point3D";
                    }

                    // Value types do not get a "FooData" struct (e.g., there is no Point3DData)
                    return Name;
                }
                else
                {
                    return Name + "Data";
                }
            }
        }

        public string CreateMethod
        {
            get
            {
                return "MilResource_" + BaseUnmanagedType + "_Create";
            }
        }

        public string MilTypeEnum
        {
            get
            {
                if (Name == "Vector3D" || Name == "Double" || Name == "Rect"
                    || Name == "Point3D" || Name == "Color" || Name == "Point"
                    || Name == "Matrix" || Name == "Size" || Name == "Quaternion")
                {
                    return "TYPE_" + MS.Internal.MilCodeGen.Runtime.GeneratorMethods.AllCaps(Name) + "RESOURCE";
                }
                else
                {
                    return "TYPE_" + MS.Internal.MilCodeGen.Runtime.GeneratorMethods.AllCaps(Name);
                }
            }
        }

        public string UpdateMethod
        {
            get
            {
                return "MilResource_" + BaseUnmanagedType + "_Update";
            }
        }

        public string LastPropertyFlag
        {
            get
            {
                return "Last" + Name + "Flag";
            }
        }

        public override UnmanagedTypeType ParameterType
        {
            get
            {
                if (IsValueType)
                {
                    return UnmanagedTypeType.Value;
                }
                else if (IsCollection)
                {
                    return UnmanagedTypeType.Collection;
                }
                else
                {
                    return UnmanagedTypeType.Handle;
                }
            }
        }

        public override int MarshaledSize
        {
            get
            {
                if (_marshaledSize != -1)
                {
                    return _marshaledSize;
                }

                _marshaledSize = 0;
                foreach (McgField field in LocalFields)
                {
                    _marshaledSize += field.Type.MarshaledSize;
                }

                return _marshaledSize;
            }
        }

        public override bool UsesHandles
        {
            get
            {
                if (!HasUnmanagedResource)
                {
                    return false;
                }

                return base.UsesHandles;
            }
        }

        public string[] Namespaces
        {
            get
            {
                return _namespaces;
            }
        }

        public override bool InlinedUnmanagedResource
        {
            get
            {
                McgResource extendsAsResource = Extends as McgResource;

                return _inlinedUnmanagedResource || (extendsAsResource != null && extendsAsResource.InlinedUnmanagedResource);
            }
        }

        /// <summary>
        /// Returns true if only if this class adds any animated properties to the hierarchy.
        /// </summary>
        /// <value></value>
        public bool HasLocalAnimations
        {
            get
            {
                McgStruct baseResource = this;

                McgField[] animFields = ResourceModel.Filter(baseResource.AllManagedFields, ResourceModel.IsAnimated);

                return animFields.Length > 0;
            }
        }

        public McgEmptyField EmptyField
        {
            get { return _emptyField; }
        }

        public bool ShouldGenerateEmptyClass
        {
            get
            {
                // If we have an empty field and we are not a value type we
                // need a special singleton class to serve as a sentinel for empty.
                //
                return EmptyField != null && EmptyField.GenerateClass && !IsValueType;
            }
        }

        public bool HasUnmanagedResourceOrDerivesFromTypeWhichHasUnmanagedResource
        {
            get
            {
                McgResource extendsAsResource = this.Extends as McgResource;

                return HasUnmanagedResource ||
                    ((extendsAsResource != null) &&
                     (extendsAsResource.HasUnmanagedResourceOrDerivesFromTypeWhichHasUnmanagedResource));
            }
        }

        public bool DerivesFromTypeWhichHasUnmanagedResource
        {
            get
            {
                McgResource extendsAsResource = this.Extends as McgResource;

                return ((extendsAsResource != null) &&
                        (extendsAsResource.HasUnmanagedResourceOrDerivesFromTypeWhichHasUnmanagedResource));
            }
        }
        
        public override string MarshalledIdentity
        {
            get
            {
                return _marshalledIdentity;
            }
        }

        public override bool HasMarshalledIdentity
        {
            get
            {
                return MarshalledIdentity != "";
            }
        }

        internal void Initialize(IMcgStructChild[] children, McgEmptyField emptyField, McgStruct extends, McgType collectionType)
        {
            base.Initialize(children, extends);
            _emptyField = emptyField;
            _collectionType = collectionType;
        }

        private McgEmptyField _emptyField = null;
        private readonly bool _isFreezable;
        private readonly bool _canIntroduceCycles;
        private readonly bool _isAnimatable;
        private readonly string _emptyClassName;
        private McgType _collectionType;
        private bool _collectionAllowsNullEntries = true;
        private string[] _namespaces;
        private readonly bool _inlinedUnmanagedResource;
        public readonly string _marshalledIdentity;
    }
}



