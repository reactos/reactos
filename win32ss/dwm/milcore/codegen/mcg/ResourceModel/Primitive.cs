// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL primitive types. 
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
    //  Class: McgPrimitive
    //
    //  Either a primitive type, or an externally-defined type for which we
    //  don't have a description.
    //
    //  We will never generate code to define this type. (But this type
    //  will presumably be referenced in generated code for other types.)
    //
    //--------------------------------------------------------------------------

    public class McgPrimitive : McgType
    {
        internal McgPrimitive(
            ResourceModel resourceModel,
            string name,
            string managedType,
            string unmanagedName,
            bool isValueType,
            bool needsConvert,
            bool isAlwaysSerializableAsString,
            int size,
            string marshalAs,
            bool sameSize,
            string parseMethod
            ) : base(resourceModel, name, managedType, unmanagedName, isValueType, needsConvert, isAlwaysSerializableAsString, size, parseMethod)
        {
            Debug.Assert(unmanagedName != "");   // CreateTypeObjects ensures this.

            MarshalAs = marshalAs;
            SameSize = sameSize;
        }

        public override string MarshalUnmanagedType
        {
            get
            {
                if (MarshalAs != null)
                {
                    return MarshalAs;
                }

                return base.MarshalUnmanagedType;
            }
        }

        public readonly string MarshalAs;
        public readonly bool SameSize;
    }
}


