// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL structure fields.
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
    using MS.Internal.MilCodeGen.Helpers;


    //------------------------------------------------------
    //
    //  Class: IMcgStructChild
    //
    //  Structs may contain more than plain fields. They may
    //  also contain unions and block commented fields.
    //  IMcgStructChild represents any of these types.
    //
    //------------------------------------------------------

    public interface IMcgStructChild {};

    //------------------------------------------------------
    //
    //  Class: McgEmptyField
    //
    //  The "empty field" signifies the property which
    //  returns the special empty singleton.
    //
    //------------------------------------------------------

    public class McgEmptyField
    {
        internal McgEmptyField(string name, bool generateClass, bool distinguished)
        {
            Name = name;
            GenerateClass = generateClass;
            Distinguished = distinguished;
        }

        public readonly string Name;
        public readonly bool GenerateClass;
        public readonly bool Distinguished;
    }

    //------------------------------------------------------
    //
    //  Class: McgField
    //
    //  A field of a structure or resource
    //
    //------------------------------------------------------

    public class McgField : IMcgStructChild
    {
        public static McgField[] StructChildArrayToFieldArray(IMcgStructChild[] structChildren)
        {
            ArrayList alFields = new ArrayList();

            foreach(IMcgStructChild child in structChildren)
            {
                if (child is McgUnion)
                {
                    CodeGenHelpers.ThrowValidationException("Cannot flatten unions.");
                }

                McgField field = child as McgField;
                McgBlockCommentedFields block = child as McgBlockCommentedFields;

                if (field != null)
                {
                    alFields.Add(field);
                }
                else if (block != null)
                {
                    //CodeGenHelpers.ValidationAssert(block != null, "Unexpected child type.");

                    alFields.AddRange(StructChildArrayToFieldArray(block.Children));
                }
            }

            return (McgField[]) alFields.ToArray(typeof(McgField));
        }

        internal McgField(string name,
                          string unmanagedName,
                          McgType type,
                          string comment,
                          bool isAnimated,
                          string propertyAlias,
                          bool isReadOnly,
                          bool isManagedOnly,
                          bool isUnmanagedOnly,
                          bool isValidate,
                          bool isNew,
                          string defaultValue,
                          bool isInternal,
                          bool isProtected,
                          bool propertyChangedHook,
                          bool isCommonlySet,
                          string coerceValueCallback,
                          bool serializationVisibility,
                          string typeConverter,
                          bool cachedLocally)
        {
            Name = name;
            UnmanagedName = (unmanagedName != null) ? unmanagedName : name;
            Type = type;
            Comment = comment;
            IsManagedOnly = isManagedOnly;
            IsUnmanagedOnly = isUnmanagedOnly;
            IsAnimated = isAnimated;
            PropertyAlias = propertyAlias;
            IsAliased = (propertyAlias != null) && (propertyAlias.Length > 0);
            IsReadOnly = isReadOnly;
            IsValidate = isValidate;
            IsNew = isNew;
            Default = defaultValue;
            IsInternal = isInternal;
            IsProtected = isProtected;
            PropertyChangedHook = propertyChangedHook;
            IsCommonlySet = isCommonlySet;
            CoerceValueCallback = coerceValueCallback;
            SerializationVisibility = serializationVisibility;
            TypeConverter = typeConverter;
            CachedLocally = cachedLocally;
            Debug.Assert(!isAnimated || !type.IsFreezable,
                name + " cannot be both freezable and a value type.");
            Debug.Assert(!isManagedOnly || !isUnmanagedOnly, 
                name + " cannot be both ManagedOnly and UnmanagedOnly");
        }

        public string PropertyName
        {
            get
            {
                return MS.Internal.MilCodeGen.Runtime.GeneratorMethods.FirstCap(Name);
            }
        }

        public string DPPropertyName
        {
            get
            {
                return PropertyName + "Property";
            }
        }

        public string ToFormalParam
        {
            get
            {
                return Type.ToFormalParam(Name);
            }
        }

        // Internal data member name of the managed classes -- only relevant for value types
        public string InternalName
        {
            get
            {
                return "_" + MS.Internal.MilCodeGen.Runtime.GeneratorMethods.FirstLower(Name);
            }
        }

        public string PropertyFlag
        {
            get
            {
                return PropertyName + "PropertyFlag";
            }
        }

        public string PropertyPackName
        {
            get
            {
                return PropertyName + "PropertyPack";
            }
        }

        public string Visibility
        {
            get
            {
                int idx = (IsInternal ? 0x01 : 0x00) | (IsProtected ? 0x02 : 0x00);
                return _visibilities[idx];
            }
        }

        public readonly string Name;
        public readonly string UnmanagedName;
        public readonly McgType Type;
        public readonly string Comment;
        public readonly bool IsManagedOnly;
        public readonly bool IsUnmanagedOnly;
        public readonly bool IsReadOnly;
        public readonly bool IsAnimated;  // If true, Type is a pass-by-value
                                          // type but the field is animatable.
                                          // In this case, the value parameter
                                          // is the base value, and we'll pass
                                          // an optional resource for the
                                          // animation info.
        public readonly string PropertyAlias; // If this property is aliased, this contains the
                                              // location of the original DP.
        public readonly bool IsAliased;   // Specified whether this property is aliased.
        public readonly bool IsValidate;  // If true, call On*Changing in the
                                          // partial class.
        public readonly bool IsNew;
        public readonly bool IsCommonlySet; // If true, this field is expected to be set on most
                                            // instances of the containing type.
                                            // This can be used to optimize UCE/transport sizes
                                            // as well as setting EffectiveValuesInitialSize.
        public readonly string Default;
        public readonly bool IsInternal;
        public readonly bool IsProtected;
        public readonly bool PropertyChangedHook;
        public readonly string CoerceValueCallback;
        public readonly bool SerializationVisibility;
        public readonly string TypeConverter;
        public readonly bool CachedLocally;


        private static string[] _visibilities = { "public", "internal", "protected", "internal protected" };
    }


    //------------------------------------------------------
    //
    //  Class: McgUnion
    //
    //  Represent a C/C++ union.
    //
    //------------------------------------------------------

    public class McgUnion : IMcgStructChild
    {
        internal McgUnion(
            IMcgStructChild[] children
            )
        {
            foreach (IMcgStructChild child in children)
            {
                if (child is McgUnion)
                {
                    CodeGenHelpers.ThrowValidationException(
                        String.Format("Nested unions are not allowed"));
                }
            }

            Children = children;
        }

        public readonly IMcgStructChild[] Children;
    }


    //------------------------------------------------------
    //
    //  Class: McgBlockCommentedFields
    //
    //  Represent a group of fields commented with a single
    //  block comment.
    //
    //------------------------------------------------------

    public class McgBlockCommentedFields : IMcgStructChild
    {
        internal McgBlockCommentedFields(
            IMcgStructChild[] children,
            string comment
            )
        {
            foreach (IMcgStructChild child in children)
            {
                if (child is McgBlockCommentedFields)
                {
                    CodeGenHelpers.ThrowValidationException(
                        String.Format("Nested block comments are not allowed: '{0}'", comment));
                }
            }

            Children = children;
            Comment = comment;
        }

        public readonly IMcgStructChild[] Children;
        public readonly string Comment;
    }


    //------------------------------------------------------
    //
    //  Class: McgConstant
    //
    //------------------------------------------------------

    public class McgConstant : IMcgStructChild
    {
        internal McgConstant(
            string name,
            string value
            )
        {
            Name = name;
            Value = value;
        }

        public readonly string Name;
        public readonly string Value;
    }
}


