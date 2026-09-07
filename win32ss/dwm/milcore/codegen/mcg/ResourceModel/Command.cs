// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL commands. 
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
    //  Class: McgCommand
    //
    //------------------------------------------------------

    public class McgCommand : McgStruct
    {
        internal McgCommand(
            ResourceModel resourceModel,
            string domain,
            string target,
            string name,
            string managedType,
            string unmanagedName,
            string comment,
            int marshaledSize,
            int marshaledAlignment,
            string parseMethod,
            bool unmanagedOnly,
            bool hasPayload,
            bool failIfCommandTransportDenied,
            string securityComment,
            bool isSecurityCritical
            )
            : base(resourceModel, domain, target, name, managedType, unmanagedName,
                   /* isValueType */ true, /* needsConvert */ false, false /* isAlwaysSerializableAsString */,
                   marshaledSize, marshaledAlignment, parseMethod)
        {
            Comment = comment;
            UnmanagedOnly = unmanagedOnly;
            IsSecurityCritical = isSecurityCritical;
            HasPayload = hasPayload;
            FailIfCommandTransportDenied = failIfCommandTransportDenied;
            SecurityComment = securityComment;
        }

        public readonly string Comment;
        public readonly bool UnmanagedOnly;
        public readonly bool HasPayload;
        public readonly bool FailIfCommandTransportDenied;
        public readonly string SecurityComment;

        // If the command contains an hwnd or a pointer
        public readonly bool IsSecurityCritical;

        public override UnmanagedTypeType ParameterType
        {
            get
            {
                return UnmanagedTypeType.Value;
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
                return base.UsesHandles;
            }
        }

        internal void Initialize(IMcgStructChild[] children)
        {
            base.Initialize(children, null);
        }
    }
}



