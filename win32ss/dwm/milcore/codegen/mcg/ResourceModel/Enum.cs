// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL enumerations. 
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


    //--------------------------------------------------------------------------
    //
    //  Class: McgEnum
    //
    //  An enumeration type that can be a value.  The fields represent the
    //  values of the enumeration.
    //
    //--------------------------------------------------------------------------

    public class McgEnum : McgType
    {
        internal McgEnum(
            ResourceModel resourceModel,
            string name,
            string[] namespaces,
            string managedNamespace,
            string managedType,
            string unmanagedName,
            bool isValueType,
            bool isFlags,
            bool needsConvert,
            int size,
            string guid,
            string comment,
            bool needsValidateValueCallback,
            string parseMethod,
            bool useFlatEnum
            ) : base(resourceModel,
                     name,
                     managedType,
                     unmanagedName,
                     isValueType,
                     needsConvert,
                     true /* isAlwaysSerializableAsString */,
                     size,
                     parseMethod)
        {
            _namespaces = namespaces;
            _managedNamespace = managedNamespace != null ? managedNamespace : String.Empty;

            _guid = guid;
            _comment = comment;
            _flags = isFlags;
            _needsValidateValueCallback = needsValidateValueCallback;
            _useFlatEnum = useFlatEnum;
        }

        internal void Initialize(IMcgEnumChild[] children)
        {
            _enumChildren = children;

            ArrayList alFlattenedEnumValues = new ArrayList();

            foreach (IMcgEnumChild child in children)
            {
                McgEnumValue enumValue = child as McgEnumValue;

                if (enumValue != null)
                {
                    alFlattenedEnumValues.Add(enumValue);
                }
                else
                {
                    McgBlockCommentedEnumValues block = (McgBlockCommentedEnumValues)child;

                    foreach (McgEnumValue subValue in block.Values)
                    {
                        alFlattenedEnumValues.Add(subValue);
                    }
                }
            }

            _enumValues = (McgEnumValue[])alFlattenedEnumValues.ToArray(typeof(McgEnumValue));
        }

        /// <summary>
        ///   Convert an enum name to an enum type
        /// </summary>
        private string GetType(string name)
        {
            if (UseFlatEnum)
            {
                return name;
            }
            else if (Flags)
            {
                return name + "::Flags";
            }
            else
            {
                return name + "::Enum";
            }
        }

        /// <summary>
        ///   Describes how we call this type in C++.
        /// </summary>
        /// <remarks>
        ///   UnmanagedDataType for enums differ slightly from the unmanaged
        ///   name. For an enum named Example, the UnmanagedName is Example, but
        ///   the UnmanagedDataType is (typically) Example::Enum. For non-enums
        ///   there is no difference.
        /// </remarks>
        public override string UnmanagedDataType
        {
            get
            {
                return GetType(UnmanagedName);
            }
        }

        /// <summary>
        ///   Describes how we call this type in C++.
        /// </summary>
        /// <remarks>
        ///   UnmanagedDataType for enums differ slightly from the unmanaged
        ///   name. For an enum named Example, the UnmanagedName is Example, but
        ///   the UnmanagedDataType is (typically) Example::Enum. For non-enums
        ///   there is no difference.
        /// </remarks>
        public override string BaseUnmanagedType
        {
            get
            {
                return GetType(BaseUnmanagedName);
            }
        }

        /// <summary>
        ///   Get the unmanaged name of the enum without the ::Enum or ::Flag
        /// </summary>
        public new string UnmanagedName
        {
            get
            {
                return base.UnmanagedName;
            }
        }

        public McgEnumValue[] AllValues
        {
            get
            {
                return _enumValues;
            }
        }

        public IMcgEnumChild[] AllChildren
        {
            get
            {
                return _enumChildren;
            }
        }

        public String Guid
        {
            get
            {
                return _guid;
            }
        }

        public String Comment
        {
            get
            {
                return _comment;
            }
        }

        public bool Flags
        {
            get
            {
                return _flags;
            }
        }

        public bool NeedsValidateValueCallback
        {
            get
            {
                return _needsValidateValueCallback;
            }
        }

        public String ManagedNamespace
        {
            get
            {
                return _managedNamespace;
            }
        }

        public string[] Namespaces
        {
            get
            {
                return _namespaces;
            }
        }

        public override UnmanagedTypeType ParameterType
        {
            get
            {
                return UnmanagedTypeType.Value;
            }
        }

        public string KernelAccessibleType
        {
            get
            {
                return _flags ?
                    "MILFLAGENUM(" + UnmanagedDataType.Replace("::Flags", "") + ")" :
                    "MILENUM("     + UnmanagedDataType.Replace("::Enum" , "") + ")";
            }
        }

        public bool UseFlatEnum
        {
            get
            {
                return _useFlatEnum;
            }
        }

        /// <summary>
        /// CanBeConst Property - bool
        /// This property is true if this type can be stored in a const var.
        /// Per MSDN, the only types for which this is possible are:
        /// Byte, Char, Int16, Int32, Int64, Single, Double, Decimal, Boolean, String, or enums.
        /// McgEnum overrides this method to return true, since enums can always be const.
        /// </summary>
        public override bool CanBeConst
        {
            get
            {
                return true;
            }
        }

        private String _guid;
        private String _comment;
        private bool   _flags;
        private bool   _needsValidateValueCallback;
        private bool   _useFlatEnum;

        private String[] _namespaces;
        private String _managedNamespace;

        private McgEnumValue[] _enumValues;
        private IMcgEnumChild[] _enumChildren;
    }


    public interface IMcgEnumChild {};

    //------------------------------------------------------
    //
    //  Class: McgEnumValue
    //
    //------------------------------------------------------

    public class McgEnumValue : IMcgEnumChild
    {
        internal McgEnumValue(
            string name,
            bool   unmanagedOnly,
            string value,
            string comment
            )
        {
            Name = name;
            UnmanagedOnly = unmanagedOnly;
            Value = value;
            Comment = comment;
        }

        public readonly string Name;
        public readonly bool   UnmanagedOnly;
        public readonly string Value;
        public readonly string Comment;
    }

    public class McgBlockCommentedEnumValues : IMcgEnumChild
    {
        internal McgBlockCommentedEnumValues(
            McgEnumValue[] values,
            string comment
            )
        {
            Values = values;
            Comment = comment;
        }

        public readonly McgEnumValue[] Values;
        public readonly string Comment;
    }
}



