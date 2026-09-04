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
    //  Class: McgArray
    //
    //------------------------------------------------------

    public class McgArray : McgType
    {
        internal McgArray(
            ResourceModel resourceModel,
            McgType type
            ) : base(resourceModel,
                     type.Name,
                     type.ManagedName,
                     type.UnmanagedDataType,
                     type.IsValueType,
                     type.NeedsConvert,
                     type.IsAlwaysSerializableAsString,
                     type.MarshaledSize,
                     type.ParseMethod)
        { }

        public void Initialize(McgArrayDimension[] dimensions)
        {
            _dimensions = dimensions;
        }

        public McgArrayDimension[] Dimensions
        {
            get
            {
                return _dimensions;
            }
        }

        private McgArrayDimension[] _dimensions;
    }


    public class McgArrayDimension
    {
        internal McgArrayDimension(
            string size
            )
        {
            Size = size;
        }

        public readonly string Size;
    }
}


