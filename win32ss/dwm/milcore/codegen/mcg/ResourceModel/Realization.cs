// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL realizations. 
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
    //  Class: McgRealizationAPI
    //
    //------------------------------------------------------

    public class McgRealizationAPI
    {
        internal McgRealizationAPI(
            string name,
            string paramName,
            string paramTypeName
            )
        {
            Name = name;
            ParamName = paramName;
            ParamTypeName = paramTypeName;
        }

        public readonly string Name;
        public readonly string ParamName;
        public readonly string ParamTypeName;
    }


    //------------------------------------------------------
    //
    //  Class: McgRealization
    //
    //------------------------------------------------------

    public class McgRealization
    {
        internal McgRealization(
            string unmanagedType,
            string typeName,
            bool isRefCounted,
            bool isCached,
            McgRealizationAPI api
            )
        {
            UnmanagedType = unmanagedType;
            TypeName = typeName;
            IsRefCounted = isRefCounted;
            IsCached = isCached;
            Api = api;
        }

        public readonly string UnmanagedType;
        public readonly string TypeName;
        public readonly bool IsRefCounted;
        public readonly bool IsCached;
        public readonly McgRealizationAPI Api;
    }
}



