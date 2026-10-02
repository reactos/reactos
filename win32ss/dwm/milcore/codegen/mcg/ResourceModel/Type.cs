// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL types. 
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
    //  Class: McgType
    //
    //------------------------------------------------------
    //------------------------------------------------------
    //
    //  Class: McgType
    //
    //------------------------------------------------------

    public abstract class McgType
    {
        internal McgType(
            ResourceModel resourceModel,
            string name,
            string managedType,
            string unmanagedName,
            bool isValueType,
            bool needsConvert,
            bool isAlwaysSerializableAsString,
            int size,
            string parseMethod
            )
        {
            _ResourceModel = resourceModel;
            Name = name;
            _UnmanagedName = unmanagedName;
            ManagedName = managedType;
            IsValueType = isValueType;
            NeedsConvert = needsConvert;
            IsAlwaysSerializableAsString = isAlwaysSerializableAsString;
            _marshaledSize = size;
            ParseMethod = parseMethod;

            if (ManagedName == "")
            {
                ManagedName = Name;
            }
        }

        public readonly string Name;

        public readonly bool IsValueType;      // Is this a Value type?

        public readonly bool NeedsConvert;     // Whether we can pass this type
                                               // by value through a simple
                                               // conversion function.

        // If this flag is set to true, then codegen can omit calls to "CanSerializeToString",
        // because this type is always serializable to string.  Typically, this will be set
        // for value types and collections of value types.
        public readonly bool IsAlwaysSerializableAsString;


        private readonly string _UnmanagedName;      // The name of the
                                                     // corresponding
                                                     // unmanaged-code type.
                                                     // Note: For resources
                                                     // which hold collections,
                                                     // the UnmanagedDataType
                                                     // does not hold all of the
                                                     // resource's data.

        public readonly string ManagedName;

        public readonly string ParseMethod;

        public virtual bool IsVisual
        {
            get
            {
                if ( ManagedName == "Visual")
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        internal int _marshaledSize;

        public virtual int MarshaledSize
        {
            get
            {
                return _marshaledSize;
            }
        }

        public virtual int UnpaddedSize
        {
            get
            {
                return _marshaledSize;
            }
        }
        public virtual bool ShouldBeQuadWordAligned
        {
            get
            {
                return _marshaledSize > 4;
            }
        }

        /// <summary>
        ///   Describes how we call this type in C++.
        /// </summary>
        public virtual string UnmanagedDataType
        {
            get
            {
                return UnmanagedName;
            }
        }

        // This is currently used by types that have different data layout
        // between managed and unmanaged code.  For example, Matrix and MilMatrix3x2D.
        // and Color and MilColorF.
        public virtual string BaseUnmanagedType
        {
            get
            {
                return BaseUnmanagedName;
            }
        }

        /// <summary>
        ///   The difference between UnmanagedName and UnmanagedDataType is
        ///   that for an enum named Example, the UnmanagedName is Example, but
        ///   the UnmanagedDataType is (typically) Example::Enum. For non-enums
        ///   there is no difference.
        /// </summary>
        protected string UnmanagedName
        {
            get
            {
                if (!CodeGenHelpers.IsEmpty(_UnmanagedName))
                {
                    return _UnmanagedName;
                }
                else if (Name == "GradientStop") 
                {
                    return "MilGradientStop";
                }
                else if (Name == "PathFigure") 
                {
                    return "MilPathGeometry";
                }
                else
                {
                    return "Mil" + Name + "Data";
                }
            }
        }

        /// <summary>
        ///   The difference between UnmanagedName and UnmanagedDataType is
        ///   that for an enum named Example, the UnmanagedName is Example, but
        ///   the UnmanagedDataType is (typically) Example::Enum.
        /// </summary>
        protected string BaseUnmanagedName
        {
            get
            {
                if (!CodeGenHelpers.IsEmpty(_UnmanagedName))
                {
                    return _UnmanagedName;
                }

                return Name;
            }
        }

        /// <summary>
        /// MarshalUnmanagedType
        /// </summary>
        public virtual string MarshalUnmanagedType
        {
            get
            {
                return BaseUnmanagedType;
            }
        }

        /// <summary>
        /// Returns true if the given type derives from Freezable.
        /// </summary>
        public virtual bool IsFreezable
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        /// Returns true if the given type can introduce a cycle.
        /// </summary>
        public virtual bool CanIntroduceCycles
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Returns true if the given type derives from Animatable.
        /// </summary>
        public virtual bool IsAnimatable
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        /// Determines whether this type directly contain types which are resources.
        ///
        /// (But does not recurse into the subtypes.)
        /// 
        /// </summary>
        public virtual bool UsesHandles
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        ///     Returns true if the resource is inlined into its parent when marshaling
        /// </summary>
        public virtual bool InlinedUnmanagedResource
        {
            get
            {
                return false;
            }
        }

        public virtual string MarshalledIdentity
        {
            get
            {
                return "";
            }
        }

        public virtual bool HasMarshalledIdentity
        {
            get
            {
                return false;
            }
        }        

        public string UnmanagedType(UnmanagedTypeType tt)
        {
            switch (tt)
            {
                case UnmanagedTypeType.ConvertValue:
                case UnmanagedTypeType.Value:
                    return UnmanagedDataType;

                case UnmanagedTypeType.MarshalValue:
                    if (this.NeedsConvert == false)
                    {
                        return ManagedName;
                    }
                    else
                    {
                        return UnmanagedDataType;
                    }

                case UnmanagedTypeType.Pointer:   // Warning: In this case, the caller must supply the "*".
                    return UnmanagedDataType;

                case UnmanagedTypeType.Collection:     // If it's an array, we pass the size type...
                    return DuceHandle.UnmanagedTypeName;

                case UnmanagedTypeType.Handle:
                    return "CMilSlaveResource";
            }
            throw new ApplicationException("Internal error");
        }

        /// <summary>
        /// Returns the unmanaged type used to pass this resource as a parameter.
        /// Caller must supply the "*" in the "pointer" case.
        /// </summary>

        public string AsParameter
        {
            get
            {
                return UnmanagedType(ParameterType);
            }
        }

        /// <summary>
        /// Returns the unmanaged type used to return this type.
        /// </summary>
        public string AsReturnType
        {
            get
            {
                string ptr = "";
                if (ParameterType == UnmanagedTypeType.Pointer)
                {
                    ptr = "*";
                }

                return UnmanagedType(ParameterType) + " " + ptr;
            }

        }

        /// <summary>
        /// Returns a string defining this McgType as a parameter.
        /// </summary>
        public string ToFormalParam(
            string paramName,
            bool fArray   // If true, this is an array of items
            )
        {

            string ptr = "";
            if (   (ParameterType == UnmanagedTypeType.Pointer)
                || fArray
                || (ParameterType == UnmanagedTypeType.Handle))
            {
                ptr = "*";
            }

            return AsParameter + " " + ptr + paramName;
        }


        /// <summary>
        /// Returns the UnmanagedTypeType to use when passing this type as a parameter.
        /// </summary>
        public virtual UnmanagedTypeType ParameterType
        {
            get
            {
                // McgResource will override, so ignore that case

                if (IsValueType)
                {
                    if (NeedsConvert)
                    {
                        return UnmanagedTypeType.ConvertValue;
                    }
                    else
                    {
                        return UnmanagedTypeType.Value;
                    }
                }
                else
                {
                    return UnmanagedTypeType.Pointer;
                }
            }
        }

        /// <summary>
        /// Returns a string defining this McgType as a parameter.
        /// </summary>
        public string ToFormalParam(
            string paramName
            )
        {
            return ToFormalParam(paramName, false);
        }

        /// <summary>
        /// CanBeConst Property - bool
        /// This property is true if this type can be stored in a const var.
        /// Per MSDN, the only types for which this is possible are:
        /// Byte, Char, Int16, Int32, Int64, Single, Double, Decimal, Boolean, String, or enums.
        /// </summary>
        public virtual bool CanBeConst
        {
            get
            {
                return (0 == String.Compare("Byte", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Char", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Int16", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Int32", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Int64", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Single", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Double", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Decimal", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Boolean", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("String", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture)) ||
                       (0 == String.Compare("Int32", Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture));
            }
        }

        public string NativeDestinationDir;
        public string ManagedDestinationDir;
        public string ConverterDestinationDir;
        public string ManagedSharedDestinationDir;
        public bool IsOldStyle;

        protected readonly ResourceModel _ResourceModel;
    }

}



