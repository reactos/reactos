// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL structures. 
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
    //  Class: McgStruct
    //
    //  A type which contains fields. (That we know about. If we're told about a
    //  struct but not given its fields, then it is an McgPrimitive.)
    //
    //--------------------------------------------------------------------------

    public class McgStruct : McgType
    {
        internal McgStruct(
            ResourceModel resourceModel,
            string domain,
            string target,
            string name,
            string managedType,
            string unmanagedName,
            bool isValueType,
            bool needsConvert,
            bool isAlwaysSerializableAsString,
            int marshaledSize,
            int marshaledAlignment,
            string parseMethod
            ) : base(resourceModel, name, managedType, unmanagedName, isValueType, needsConvert, isAlwaysSerializableAsString, marshaledSize, parseMethod)
        {
            Domain = domain;
            Target = target;
            MarshaledAlignment = marshaledAlignment;
            IsMarshaledSizeUserSpecified = (marshaledSize!=-1);
        }

        public readonly string Domain;
        public readonly string Target;
        public readonly int MarshaledAlignment;
        public readonly bool IsMarshaledSizeUserSpecified;

        /// <summary>
        /// Command names are not unique, but the combination of the domain, 
        /// target and name is. This method combines these three to form a unique ID
        /// </summary>
        public string FullName
        {
            get
            {
                return CreateFullName(Domain, Target, Name);
            }
        }

        /// <summary>
        /// A helper method to encode the domain/target/name triplet.
        /// </summary>
        public static string CreateFullName(string domain, string target, string name)
        {
            return
                (String.IsNullOrEmpty(domain) ? "Mil::" : domain + "::") +
                (String.IsNullOrEmpty(target) ? String.Empty : target + " ::") +
                name;
        }

        /// <summary>
        /// Name of the C++/C# structure that is used to marshall the command.
        ///
        /// Example: MILCMD_VISUAL_INSERTCHILDAT
        /// </summary>
        public string CommandName
        {
            get
            {
                //
                // Create a name of the form MILCMD[_DOMAIN][_TARGET][_NAME].
                // 
                // "Mil" and "Redirection" domains are skipped for now.
                // 

                bool isTransportTsCommand =
                    Domain == "Transport" && Target == "Ts";

                string domain = 
                    (Domain == "Mil" 
                     || Domain == "Redirection" 
                     || Domain == "RenderData"
                     || Domain == "Resource"
                     || isTransportTsCommand
                     ? String.Empty 
                     : "_" + Domain.ToUpper());

                string target =
                    (String.IsNullOrEmpty(Target)
                     || isTransportTsCommand
                     ? String.Empty
                     : "_" + Target.ToUpper());

                string name =
                    Name == "Update" 
                    ? String.Empty
                    : "_" + Name.ToUpper();

                return "MILCMD" + domain + target + name;
            }
        }

        /// <summary>
        /// Name of the command type, as found in the MILCMD enumeration.
        ///
        /// Example: MilCmdVisualInsertChildAt
        /// </summary>
        public string CommandTypeName
        {
            get
            {
                //
                // Create a command type name of the form MilCmd[Domain][Target][Name]
                // 
                // Special case for render data, explicit resource update and DWM commands.
                //

                Debug.Assert(Helpers.StringHelpers.FilterString(Domain, '_') == Domain);
                Debug.Assert(Helpers.StringHelpers.FilterString(Target, '_') == Target);

                string prefix =
                    (Domain == "RenderData" ? "Mil" : (Domain == "DWM" ? "Dwm" : "MilCmd"));

                bool isTransportTsCommand =
                    Domain == "Transport" && Target == "Ts";

                string domain = 
                    (Domain == "Mil" 
                     || Domain == "RenderData" 
                     || Domain == "Redirection" 
                     || Domain == "DWM" 
                     || Domain == "Resource"
                     || isTransportTsCommand
                     ? String.Empty 
                     : Domain);

                string target =
                    (isTransportTsCommand ? String.Empty : Target);

                string name = 
                    (Name == "Update" 
                     ? String.Empty 
                     : Helpers.StringHelpers.FilterString(Name, '_'));

                return prefix + domain + target + name;
            }
        }


        /// <summary>
        /// Name of the C++ resource class that a command is targeted at.
        ///
        /// Example: CMilVisual
        /// </summary>
        /// <remarks>
        /// 
        /// </remarks>
        public string TargetResourceName
        {
            get
            {
                if (!String.IsNullOrEmpty(Target)
                    && Target != "Ts"
                    && Target != "Channel"
                    && Target != "Partition"
                    && Target != "Sprite") 
                {
                    string targetName = Target;

                    if (Target == "Bitmap") 
                    {
                        targetName = "BitmapSource";
                    }
                    else if (Target == "HwndTarget")
                    {
                        targetName = "HwndRenderTarget";
                    }
                    else if (Target == "IntermediateTarget") 
                    {
                        targetName = "IntermediateRenderTarget";
                    }
                    else if (Target == "GenericTarget") 
                    {
                        targetName = "GenericRenderTarget";
                    }
                    else if (Target == "Target") 
                    {
                        targetName = "RenderTarget";
                    }

                    McgResource resource =
                        _ResourceModel.FindType(targetName) as McgResource;

                    Debug.Assert(resource != null, String.Format("{0} is not a valid resource name.", targetName));

                    return resource.DuceClass;
                }
                else
                {
                    return String.Empty;
                }
            }
        }


        public McgField[] LocalFields
        {
            get
            {
                //
                // We calculate this on demand. Accessing LocalFields will throw
                // if any of the children are unions. This is done so that
                // callers that are not union-aware will fail if they encounter
                // a union.
                //
                if (_localFields == null)
                {
                    _localFields = McgField.StructChildArrayToFieldArray(LocalChildren);
                }

                return _localFields;
            }
        }

        public IMcgStructChild[] LocalChildren
        {
            get
            {
                return _localChildren;
            }
        }

        /// <summary>
        /// The count of fields which are commonly set (IsCommonlySet==true)
        /// </summary>
        public int CommonlySetFieldCount
        {
            get
            {
                return ResourceModel.Count(AllManagedFields, ResourceModel.IsCommonlySet);
            }
        }

        /// <summary>
        /// The fields which are commonly set (IsCommonlySet==true)
        /// </summary>
        public McgField[] CommonlySetFields
        {
            get
            {
                return ResourceModel.Filter(AllManagedFields, ResourceModel.IsCommonlySet);
            }
        }

        public McgField[] LocalNonNewFields
        {
            get
            {
                return ResourceModel.Filter(LocalFields, ResourceModel.IsNotNew);
            }
        }

        public McgField[] AllUceFields
        {
            get
            {
                return ResourceModel.Filter(AllManagedFields, ResourceModel.IsUnmanaged);
            }
        }

        public McgField[] AllManagedFields
        {
            get
            {
                if (_allFields != null)
                {
                    return _allFields;
                }

                _allFields = ComputeAllFields(new Stack());
                return _allFields;
            }
        }

        public McgStruct Extends
        {
            get
            {
                return _extends;
            }
        }

        ///<summary>
        ///Returns the top-most base class this extends.
        ///</summary>
        public McgStruct ExtendsBase
        {
            get
            {
                McgStruct extends = _extends as McgStruct;
                while (extends != null && extends.Extends != null)
                {
                    McgStruct structExtends = extends.Extends;

                    if (structExtends is McgStruct)
                    {
                        extends = structExtends as McgStruct;
                    }
                    else
                    {
                        break;
                    }
                }

                return extends;
            }
        }

        ///<summary>
        ///     Returns true if this is the final class in the inheritence chain.
        ///</summary>
        public bool IsFinal
        {
            get
            {
                foreach(McgType type in _ResourceModel.AllTypes)
                {
                    McgStruct potentialExtender = type as McgStruct;

                    if (potentialExtender != null && potentialExtender.Extends == this)
                    {
                        // Found someone who extends this class.
                        return false;
                    }
                }

                return true;
            }
        }

        private static bool UsesHandles_Helper(McgStruct structType)
        {
            foreach (McgField field in structType.AllUceFields)
            {
                if (field.IsAnimated)
                {
                    return true;
                }

                McgResource fieldResource = field.Type as McgResource;

                if (fieldResource != null && !fieldResource.IsValueType)
                {
                    return true;
                }
            }

            foreach(McgResource potentialExtender in structType._ResourceModel.Resources)
            {
                if (potentialExtender.Extends == structType && UsesHandles_Helper(potentialExtender))
                {
                    return true;
                }
            }

            return false;
        }

        public override bool UsesHandles
        {
            get
            {
                return UsesHandles_Helper(this);
            }
        }

        public override int UnpaddedSize
        {
            get
            {
                return CalculateUnpaddedSize();
            }
        }

        public override bool ShouldBeQuadWordAligned
        {
            get
            {
                return UnpaddedSize > 4;
            }
        }

        private int CalculateUnpaddedSize()
        {
            // If this cannot be passed by value, then it will be stored as a handle
            if (!IsValueType)
            {
                return DuceHandle.Size;
            }

            if (IsMarshaledSizeUserSpecified)
            {
                return MarshaledSize;
            }

            int size = 0;

            foreach (McgField field in LocalFields)
            {
                Debug.Assert(field.Type.IsValueType,
                    "Since this class can be passed by value it can only contain fields which can be passed by value.");

                Debug.Assert(field.Type.UnpaddedSize != -1, "Size must be defined");

                size += field.Type.UnpaddedSize;
            }

            return size;
        }

        internal void Initialize(IMcgStructChild[] children, McgStruct extends)
        {
            _localChildren = children;
            _extends = extends;
        }

        private McgField[] ComputeAllFields(Stack containingTypes)
        {
            ArrayList fields = new ArrayList();

            if (Extends != null)
            {
                if (containingTypes.Contains(Extends))
                {
                    ArrayList al = new ArrayList();
                    foreach (McgType type in containingTypes.ToArray())
                    {
                        al.Add(type.Name);
                    }

                    throw new ApplicationException(String.Format(
                        "Recursive type definition. Tried to push {0} onto stack: {1}",
                        Extends.Name,
                        String.Join("; ", (string[]) al.ToArray(typeof(string)))));
                }

                containingTypes.Push(this);

                fields.AddRange(Extends.ComputeAllFields(containingTypes));

                containingTypes.Pop();
            }

            fields.AddRange(LocalFields);

            return (McgField[]) fields.ToArray(typeof(McgField));
        }

        McgStruct _extends;

        IMcgStructChild[] _localChildren;
        McgField[] _localFields;   // fields declared in this class
        McgField[] _allFields;
    }
}




