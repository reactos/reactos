// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL data types. Built from
//              an XML description.
//
//------------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.ResourceModel
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Xml;
    using MS.Internal.MilCodeGen;
    using System.Reflection;
    using System.CodeDom.Compiler;

    /// <summary>
    /// DuceHandle - this contains statics which describe DUCE handles.
    /// This allows us to change the size and type for handles without
    /// trying to find every use within the code.
    /// </summary>
    internal class DuceHandle
    {
        // Handles are currently Uint32s
        public static readonly int Size = 4;
        public static readonly string ManagedTypeName = "UInt32";
        public static readonly string UnmanagedTypeName = "HMIL_RESOURCE";
        public static readonly string ManagedNullHandle = "CompositionResourceManager.InvalidResourceHandle";
        public static readonly string UnmanagedNullHandle = "HMIL_RESOURCE_NULL";
    }

    /// <summary>
    /// A resource may have multiple unmanaged types associated with it;
    /// this enum classifies them.
    /// </summary>
    public enum UnmanagedTypeType
    {
        Value,
        ConvertValue,
        MarshalValue,
        Pointer,
        Collection,
        Handle
    }

    /// <summary>
    ///     Currently only used to hide DrawScene3D on DrawingContext, but
    ///     might be generally useful.
    /// </summary>
    [Flags]
    public enum Modifier
    {
        None = 0,
        @public = 1,
        @internal = 2
    }

    /// <summary>
    /// These "generation control sections" classify the generated code
    /// so that we can selectively control which pieces are generated.
    ///
    /// This list needs to be kept in sync with the attributes on the "Generate"
    /// element (as defined in the schema).
    /// </summary>
    public enum CodeSections
    {
        NativeIncludingKernel       = 0,
        NativeNotInKernel           = 1,
        NativeMilRenderTypes        = 2,
        NativeWincodecPrivate       = 3,
        NativeDuce                  = 4,
        NativeRetriever             = 5,
        Managed                     = 6,
        ManagedClass                = 7,
        ManagedValueMethods         = 8,
        ManagedTypeConverter        = 9,
        ManagedTypeConverterContext = 10,
        CustomFreezeCoreOverride    = 11, // The type has its own FreezeCore override

        // An animation resource is a DUCE.IResource which has a base value and an AnimationClock
        // It subscribes to the AnimationClock's changed events and updates the render-thread
        // resource on changes.
        // It is illegal to attempt to generate an AnimationResource for non-value types.
        AnimationResource           = 12,

        NativeRedirection           = 13,

        CodeSectionCount            = 14,
    };


    //------------------------------------------------------
    //
    //  Class: ResourceModel
    //
    //------------------------------------------------------

    public class ResourceModel
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ResourceModel(XmlDocument document, string outputDir)
        {
            _types = new Hashtable();
            _commandLookupTable = new Hashtable();

            _outputDir = outputDir;

            CreateTypeObjects(document);
            CreateTypeReferences(document);
            CreateGenerationList(document);
        }

        #endregion Constructors


        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        public delegate bool TypeFilter(McgType type);
        public delegate bool FieldFilter(McgField field);

        /// <summary>
        /// Return a count of the subset of the given fields, using the given filter.
        /// </summary>
        public static int Count(McgField[] fields, FieldFilter filter)
        {
            int count = 0;

            foreach (McgField field in fields)
            {
                if (filter(field))
                {
                    count++;
                }
            }

            return count;
        }

        /// <summary>
        /// Return a subset of the given fields, using the given filter.
        /// </summary>
        public static McgField[] Filter(McgField[] fields, FieldFilter filter)
        {
            ArrayList result = new ArrayList();

            foreach (McgField field in fields)
            {
                if (filter(field))
                {
                    result.Add(field);
                }
            }

            return (McgField[]) result.ToArray(typeof(McgField));
        }

        /// <summary>
        /// Return a subset of the given types, using the given filter.
        /// </summary>
        public static McgType[] Filter(McgType[] types, TypeFilter filter)
        {
            ArrayList result = new ArrayList();

            foreach (McgType type in types)
            {
                if (filter(type))
                {
                    result.Add(type);
                }
            }

            return (McgType[]) result.ToArray(typeof(McgType));
        }

        /// <summary>
        /// Return a subset of the given resource types, using the given filter.
        /// </summary>
        public static McgResource[] Filter(McgResource[] types, TypeFilter filter)
        {
            ArrayList result = new ArrayList();

            foreach (McgResource type in types)
            {
                if (filter(type))
                {
                    result.Add(type);
                }
            }

            return (McgResource[]) result.ToArray(typeof(McgResource));
        }

        /// <summary>
        /// A field filter for non-animated, non-freezable fields
        /// </summary>
        public static bool IsSimple(McgField field)
        {
            return !field.IsAnimated && !IsFreezable(field);
        }

        /// <summary>
        /// A field filter for animated fields
        /// </summary>
        public static bool IsAnimated(McgField field)
        {
            return field.IsAnimated;
        }

        /// <summary>
        /// A field filter for freezable fields
        /// </summary>
        public static bool IsFreezable(McgField field)
        {
            return IsFreezable(field.Type);
        }

        /// <summary>
        /// A field filter for Visual fields
        /// </summary>
        public static bool IsVisual(McgField field)
        {
            return IsVisual(field.Type);
        }
        
        /// <summary>
        /// A field filter for Visual fields with unmanaged resources.
        /// </summary>
        public static bool IsVisualAndNotManagedOnly(McgField field)
        {
            return IsVisual(field.Type) && !field.IsManagedOnly;
        }

        /// <summary>
        /// A field filter for fields which are of a type which has an unmanaged resource
        /// </summary>
        public static bool HasUnmanagedResource(McgField field)
        {
            return HasUnmanagedResource(field.Type);
        }

        /// <summary>
        /// A field filter for fields which are of a type which has an unmanaged resource
        /// </summary>
        public static bool HasUnmanagedResourceNoVisual(McgField field)
        {
            return HasUnmanagedResourceNoVisual(field.Type);
        }

        /// <summary>
        /// A field filter for animated fields
        /// </summary>
        public static bool IsFreezableAndNotAnimatable(McgField field)
        {
            return IsFreezableAndNotAnimatable(field.Type);
        }

        /// <summary>
        /// A field filter for animatable fields
        /// </summary>
        public static bool IsAnimatable(McgField field)
        {
            return IsAnimatable(field.Type);
        }

        /// <summary>
        /// A field filter for resources
        /// </summary>
        public static bool IsUnmanaged(McgField field)
        {
            return !field.IsManagedOnly;
        }

        /// <summary>
        /// A field filter for resources
        /// </summary>
        public static bool IsResource(McgField field)
        {
            return field.Type is McgResource;
        }

        /// <summary>
        /// A type filter for freezable types
        /// </summary>
        public static bool IsFreezable(McgType type)
        {
            return type.IsFreezable;
        }

        /// <summary>
        /// A type filter for Visual types
        /// </summary>
        public static bool IsVisual(McgType type)
        {
            return type.IsVisual;
        }

        /// <summary>
        /// A field filter for new properties
        /// </summary>
        public static bool IsNotNew(McgField field)
        {
            return !field.IsNew;
        }

        /// <summary>
        /// A field filter for fields which are marked as commonly set (IsCommonlySet==true)
        /// </summary>
        public static bool IsCommonlySet(McgField field)
        {
            return field.IsCommonlySet;
        }

        /// <summary>
        /// A type filter for types which have unmanaged resources
        /// </summary>
        public static bool HasUnmanagedResource(McgType type)
        {
            return (type is McgResource) &&
                   ((McgResource)type).HasUnmanagedResource;
        }

        /// <summary>
        /// A type filter for types which have unmanaged resources
        /// </summary>
        public static bool HasUnmanagedResourceNoVisual(McgType type)
        {
            return (type is McgResource) &&
                   ((McgResource)type).HasUnmanagedResource &&
                   !((McgResource)type).IsVisual;
        }

        /// <summary>
        /// A field filter for freezable fields
        /// </summary>
        public static bool IsFreezableAndCollection(McgField field)
        {
            McgResource resource = field.Type as McgResource;

            if (resource == null)
            {
                return false;
            }

            return field.Type.IsFreezable && resource.IsCollection;
        }

        /// <summary>
        /// A field filter for fields which are collections which do *not* have unmanaged representations.
        /// Their contents may or may not be resources.
        /// </summary>
        public static bool IsCollectionAndDoesntHaveUnmanagedResource(McgField field)
        {
            McgResource resource = field.Type as McgResource;

            // This if block tests to see if the field's type is a collection (which implies that
            // it is an McgResource), and that this type has an unmanaged resource.
            return (resource != null) &&
                   resource.IsCollection &&
                   !resource.HasUnmanagedResource;
        }

        /// <summary>
        /// A field filter for fields which are collections which do *not* have unmanaged
        /// representations but whose contents do.
        /// </summary>
        public static bool IsCollectionOfDuceAndDoesntHaveUnmanagedResource(McgField field)
        {
            McgResource resource = field.Type as McgResource;

            // This if block tests to see if the field's type is a collection (which implies that
            // it is an McgResource), and that this type has an unmanaged resource.
            if ((resource == null) ||
                !resource.IsCollection ||
                resource.HasUnmanagedResource)
            {
                 return false;
            }

            McgResource collectionType = resource.CollectionType as McgResource;

            // collectionType is the type of the contents of the collection.  If it isn't an
            // McgResource, it can't have an unmanaged resource.  If it *is*, we check to see if
            // it *does* have an unmanaged resource.  Since this is the whole point of this method,
            // we return this result.
            return (collectionType != null) && resource.IsCollectionOfHandles;
        }

        /// <summary>
        /// A field filter for freezable fields
        /// </summary>
        public static bool IsFreezableAndNotCollection(McgField field)
        {
            McgResource resource = field.Type as McgResource;

            if (resource == null)
            {
                return field.Type.IsFreezable;
            }

            return field.Type.IsFreezable && !resource.IsCollection;
        }

        /// <summary>
        /// A type filter for freezable types
        /// </summary>
        public static bool IsNotFreezable(McgType type)
        {
            return !type.IsFreezable;
        }

        /// <summary>
        /// A type filter for animatable types
        /// </summary>
        public static bool IsAnimatable(McgType type)
        {
            return type.IsAnimatable;
        }

        /// <summary>
        /// A type filter for animatable types
        /// </summary>
        public static bool IsNotAnimatable(McgType type)
        {
            return !type.IsAnimatable;
        }

        /// <summary>
        /// A field filter for animated fields
        /// </summary>
        public static bool IsFreezableAndNotAnimatable(McgType type)
        {
            return type.IsFreezable && !type.IsAnimatable;
        }

        /// <summary>
        /// A field filter for value types fields
        /// </summary>
        public static bool IsValueType(McgField field)
        {
            return field.Type.IsValueType;
        }

        /// <summary>
        /// A field filter for reference types fields
        /// </summary>
        public static bool IsNotValueType(McgField field)
        {
            return !field.Type.IsValueType;
        }

        public static bool IsReferenceTypeAndNotManagedOnly(McgField field)
        {
            return !field.IsManagedOnly &&
                   !field.Type.IsValueType;
        }

        public static bool IsBasicField(McgField field)
        {
            // Future Consideration: 
            // Use advanced parameter support
            //
            // Advanced parameters can be used to add properties to the
            // 'animate' version of the DrawingContext instruction without
            // adding them to the basic/non-animate version.  This could be
            // utilized by placing a property on McgField that specifies
            // whether or not the parameter should be hidden from
            // the 'basic' DrawingContext overload.
            return true;
        }

        public static bool IsBasicPublicField(McgField field)
        {
            // Future Consideration: 
            // Use advanced parameter support
            //
            // Advanced parameters can be used to add properties to the
            // 'animate' version of the DrawingContext instruction without
            // adding them to the basic/non-animate version.  This could be
            // utilized by placing a property on McgField that specifies
            // whether or not the parameter should be hidden from
            // the 'basic' DrawingContext overload.
            return IsBasicField(field) && IsPublicField(field);
        }

        public static bool IsPublicField(McgField field)
        {
            // Allow non-"internal" as well as "protected internal" since
            // "protected internal" fields are also "public" in the sense that
            // they're usable from public subclasses.
            return !field.IsInternal || field.IsProtected;
        }

        public static bool IsAdvancedField(McgField field)
        {
            // Future Consideration: 
            // Use advanced parameter support
            //
            // Advanced parameters can be used to add properties to the
            // 'animate' version of the DrawingContext instruction without
            // adding them to the basic/non-animate version.  This could be
            // utilized by placing a property on McgField that specifies
            // whether or not the parameter should be hidden from
            // the 'basic' DrawingContext overload.
            return false;
        }

        public static bool IsAnimatedOrAdvancedParameter(McgField field)
        {
            return IsAnimated(field) || IsAdvancedField(field);
        }

        public static bool IsTypeNamePen(McgField field)
        {
            return field.Type.Name == "Pen";
        }

        public static bool IsTypeNameBrush(McgField field)
        {
            return field.Type.Name == "Brush";
        }

        public static bool IsTypeNameGlyphRun(McgField field)
        {
            return field.Type.Name == "GlyphRun";
        }

        public static bool IsTypeNameDrawing(McgField field)
        {
            return field.Type.Name == "Drawing";
        }


        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        #region Public Properties

        public McgCommand[] Commands
        {
            get
            {
                return _commands;
            }
        }

        public McgResource[] Resources
        {
            get
            {
                return _resourceTypes;
            }
        }

        public McgEnum[] Enums
        {
            get
            {
                return _enumTypes;
            }
        }

        public RenderDataInstructionData RenderDataInstructionData
        {
            get
            {
                return _renderDataInstructionData;
            }
        }

        public McgRealization[] RealizationSet
        {
            get
            {
                if (_realizations == null)
                {
                    // We use a hashtable to filter the realizations attached to the resources
                    // down to a unique set.  (e.g., one D3DLIGHT entry)
                    //
                    Hashtable hashtable = new Hashtable();
                    foreach(McgResource resource in Resources)
                    {
                        if (resource.Realization != null && !hashtable.Contains(resource.Realization.TypeName))
                        {
                            hashtable.Add(resource.Realization.TypeName, resource.Realization);
                        }
                    }

                    // Copy back to an Array
                    //
                    _realizations = new McgRealization[hashtable.Values.Count];
                    hashtable.Values.CopyTo(_realizations, 0);
                }
                return _realizations;
            }
        }

        public McgType[] AllTypes
        {
            get
            {
                return _allTypes;
            }
        }

        public McgEnum[] AllEnums
        {
            get
            {
                ArrayList result = new ArrayList();

                foreach (McgType type in _allTypes)
                {
                    if (type is McgEnum)
                    {
                        result.Add(type);
                    }
                }

                return (McgEnum[]) result.ToArray(typeof(McgEnum));
            }
        }

        public string OutputDirectory
        {
            get
            {
                return _outputDir;
            }
        }

        #endregion Public Properties

        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
        #region Internal Methods

        /// <summary>
        /// Get the McgType for the given string. Throws an
        /// exception if not found.
        /// </summary>
        internal McgType FindType(string name)
        {
            if (_types[name] == null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Invalid type: {0}",
                    name));
            }
            return (McgType) _types[name];
        }
        #endregion Internal Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        /// <summary>
        /// Pass 1
        ///
        /// Populate the hash table, but do not set up links to other objects.
        /// (We can't look up yet because the hash table isn't fully initialized).
        /// </summary>
        private void CreateTypeObjects(XmlDocument document)
        {
            ArrayList allTypes = new ArrayList();

            //
            // Add external/primitive types
            //

            XmlNodeList xnlenumBlocks = document.SelectNodes(
                "/MilTypes/Resources");

            XmlNodeList enumsBlock = document.SelectNodes(
                "/MilTypes/Enums");

            ArrayList alEnumTypes = new ArrayList();

            foreach (XmlNode enumBlock in enumsBlock)
            {
                XmlNodeList xmlNamespaces = enumBlock.SelectNodes("Namespaces/Namespace");
                string[] namespaces = null;

                if (xmlNamespaces != null)
                {
                    namespaces = new string[xmlNamespaces.Count];

                    for (int i = 0; i < xmlNamespaces.Count; i++)
                    {
                        namespaces[i] = ToString(xmlNamespaces[i], "Name");
                    }
                }

                string managedNamespace = ToString(enumBlock, "ManagedNamespace", null);

                XmlNodeList xnlEnumsTypes = enumBlock.SelectNodes(
                    "Enum");

                foreach (XmlNode node in xnlEnumsTypes)
                {
                    McgEnum newEnum = new McgEnum(
                        this,
                        ToString(node, "Name"),
                        namespaces,
                        managedNamespace,
                        ToString(node, "ManagedType", String.Empty),
                        ToString(node, "UnmanagedName", ToString(node, "Name")),
                        true,
                        ToBoolean(node, "Flags", false),
                        false,
                        ToInt32(node, "MarshaledSize", 4),
                        ToString(node, "Guid", String.Empty),
                        ToString(node, "Comment", String.Empty),
                        ToBoolean(node, "NeedsValidateValueCallback", false),
                        ToString(node, "ParseMethod", String.Empty),
                        ToBoolean(node, "UseFlatEnum", false));

                    newEnum.Initialize(CreateEnumChildArray(node));

                    AddType(newEnum);
                    allTypes.Add(newEnum);
                    alEnumTypes.Add(newEnum);
                }
            }

            _enumTypes = (McgEnum[]) alEnumTypes.ToArray(typeof(McgEnum));

            //
            // Add external/primitive types
            //

            XmlNodeList primitives = document.SelectNodes(
                "/MilTypes/Primitives/Primitive");

            foreach (XmlNode node in primitives)
            {
                McgType newType = null;

                newType = new McgPrimitive(
                    this,
                    ToString(node, "Name"),
                    ToString(node, "ManagedType", ""),
                    ToString(node, "UnmanagedName"),    // UnmanagedDataType is
                                                               // non-optional for
                                                               // primitive types
                    ToBoolean(node, "IsValueType", true),
                    ToBoolean(node, "NeedsConvert", true),
                    ToBoolean(node, "IsAlwaysSerializableAsString", false),
                    ToInt32(node, "MarshaledSize", -1),
                    ToString(node, "MarshalAs", ""),
                    ToBoolean(node, "SameSize", false),
                    ToString(node, "ParseMethod", String.Empty));

                AddType(newType);
                allTypes.Add(newType);
            }


            //
            // Add struct types
            //

            XmlNodeList xnlStructBlocks = document.SelectNodes(
                "/MilTypes/Structs");

            foreach (XmlNode structBlock in xnlStructBlocks)
            {
                XmlNodeList xnlStructs = structBlock.SelectNodes(
                    "Struct");

                foreach (XmlNode node in xnlStructs)
                {
                    McgType newType = null;

                    XmlNodeList fields = node.SelectNodes("Field");

                    Debug.Assert(fields.Count >= 0); // Validated by the schema file.

                    newType = new McgStruct(
                        this,
                        /* domain */ "Mil",
                        /* target */ String.Empty,
                        ToString(node, "Name"),
                        ToString(node, "ManagedType", ""),
                        ToString(node, "UnmanagedName", ""),
                        ToBoolean(node, "IsValueType", false),
                        ToBoolean(node, "NeedsConvert", false),
                        ToBoolean(node, "IsAlwaysSerializableAsString", false),
                        ToInt32(node, "MarshaledSize", -1),
                        ToInt32(node, "MarshaledAlignment", -1),
                        ToString(node, "ParseMethod", String.Empty)
                        );

                    AddType(newType);
                    allTypes.Add(newType);
                }
            }


            //
            // Add commands
            //

            ArrayList alCommands = new ArrayList();

            XmlNodeList xnlCommandBlocks =
                document.SelectNodes("/MilTypes/Commands");

            foreach (XmlNode commandBlock in xnlCommandBlocks)
            {
                XmlNodeList xnlCommands =
                    commandBlock.SelectNodes("Command");

                foreach (XmlNode node in xnlCommands)
                {
                    McgCommand newCommand = new McgCommand(
                        this,
                        ToString(node, "Domain", "Mil"),
                        ToString(node, "Target", String.Empty),
                        ToString(node, "Name"),
                        ToString(node, "ManagedType", ""),
                        ToString(node, "UnmanagedName", ""),
                        ToString(node, "Comment", String.Empty),
                        ToInt32(node, "MarshaledSize", -1),
                        ToInt32(node, "MarshaledAlignment", -1),
                        ToString(node, "ParseMethod", null),
                        ToBoolean(node, "UnmanagedOnly", false),
                        ToBoolean(node, "HasPayload", false),
                        ToBoolean(node, "FailIfCommandTransportDenied", true),
                        ToString(node, "SecurityComment", String.Empty),
                        ToBoolean(node, "IsSecurityCritical", true)
                        );

                    AddCommand(newCommand);
                    alCommands.Add(newCommand);
                }
            }

            _commands = (McgCommand[]) alCommands.ToArray(typeof(McgCommand));


            //
            // Add resource types
            //

            ArrayList alResourceTypes = new ArrayList();

            XmlNodeList xnlResourceBlocks = document.SelectNodes(
                "/MilTypes/Resources");

            foreach (XmlNode resourceBlock in xnlResourceBlocks)
            {
                XmlNodeList xmlNamespaces = resourceBlock.SelectNodes("Namespaces/Namespace");
                string[] namespaces = null;

                if (xmlNamespaces != null)
                {
                    namespaces = new string[xmlNamespaces.Count];

                    for (int i = 0; i < xmlNamespaces.Count; i++)
                    {
                        namespaces[i] = ToString(xmlNamespaces[i], "Name");
                    }
                }

                string managedNamespace = ToString(resourceBlock, "ManagedNamespace", null);

                XmlNodeList xnlResourceTypes = resourceBlock.SelectNodes(
                    "Resource");

                foreach (XmlNode node in xnlResourceTypes)
                {
                    McgType newType = new McgResource(
                        this,
                        ToString(node, "Name"),
                        namespaces,
                        managedNamespace,
                        ToString(node, "ManagedType", ""),
                        ToString(node, "UnmanagedName", ""),
                        ToString(node, "UnmanagedResourceAlias", String.Empty),
                        ToString(node, "Comment", String.Empty),
                        ToBoolean(node, "IsValueType", false),
                        ToBoolean(node, "NeedsConvert", false),
                        ToInt32(node, "MarshaledSize", -1),
                        ToInt32(node, "MarshaledAlignment", -1),
                        ToBoolean(node, "SkipProperties", false),
                        ToBoolean(node, "SkipToString", false),
                        ToBoolean(node, "SkipFields", false),
                        ToBoolean(node, "IsAlwaysSerializableAsString", false),
                        ToBoolean(node, "IsFreezable", !ToBoolean(node, "IsValueType", false)),  // Default IsFreezable to true for reference types
                        ToBoolean(node, "CanIntroduceCycles", true),  // Default CanIntroduceCycles to true
                        ToBoolean(node, "IsAnimatable", !ToBoolean(node, "IsValueType", false)),  // Default IsAnimatable to true for reference types
                        CreateRealization(node),
                        ToBoolean(node, "IsAbstract", false),
                        ToBoolean(node, "CreateInstanceCoreViaActivator", false),
                        ToBoolean(node, "ToInstanceDescriptor", false),
                        ToString(node, "ParseMethod", null),
                        ToBoolean(node, "SkipUpdate", false),
                        ToBoolean(node, "UseProcessUpdateWrapper", false),
                        ToBoolean(node, "UseOnChannelCoreWrapper", false),
                        ToBoolean(node, "UseStaticInitialize", false),
                        ToBoolean(node, "HasUnmanagedResource", !ToBoolean(node, "IsValueType", false)),  // Default HasUnmanagedResource to true for non-passbyValue types),
                        ToBoolean(node, "GenerateDefaultConstructor", true),
                        ToBoolean(node, "GenerateSerializerAttribute", false),
                        ToBoolean(node, "AddCloneHooks", false),
                        ToBoolean(node, "InlinedUnmanagedResource", false),
                        ToBoolean(node, "CallProcessUpdateCore", false),
                        ToBoolean(node, "LeaveUnsealed", false),
                        ToString(node, "MarshalledIdentity", ""),
                        ToBoolean(node, "AllowsNullEntries", true),
                        ToString(node, "PragmaPack", null)
                        );
                    AddType(newType);
                    allTypes.Add(newType);
                    alResourceTypes.Add(newType);
                }
            }

            _allTypes = (McgType[]) allTypes.ToArray(typeof(McgType));
            _resourceTypes = (McgResource[]) alResourceTypes.ToArray(typeof(McgResource));

            //
            // Add RenderDataInstructions.  These are not resources, but their parameters
            // are of the types just collected above, so they still rely on that data extraction.
            //

            XmlNodeList xmlRenderDataInstructions = document.SelectNodes("/MilTypes/RenderDataInstructions");

            // We cannot have more than one of these (validated by xsd)
            Debug.Assert(xmlRenderDataInstructions.Count <= 1);

            if (xmlRenderDataInstructions.Count == 0)
            {
                _renderDataInstructionData = null;
            }
            else
            {
                XmlNodeList xmlNamespaces = xmlRenderDataInstructions[0].SelectNodes("Namespaces/Namespace");
                string[] namespaces = null;

                if (xmlNamespaces != null)
                {
                    namespaces = new string[xmlNamespaces.Count];

                    for (int i = 0; i < xmlNamespaces.Count; i++)
                    {
                        namespaces[i] = ToString(xmlNamespaces[i], "Name");
                    }
                }

                _renderDataInstructionData = new RenderDataInstructionData(
                    ToString(xmlRenderDataInstructions[0], "ManagedNamespace", null),
                    ToString(xmlRenderDataInstructions[0], "ManagedDestinationDir", null),
                    ToString(xmlRenderDataInstructions[0], "NativeDestinationDir", null),
                    ToString(xmlRenderDataInstructions[0], "ExportDestinationDir", null),
                    namespaces);

                XmlNodeList renderDataInstructions = xmlRenderDataInstructions[0].SelectNodes("RenderDataInstruction");

                ArrayList allRenderDataInstructions = new ArrayList(renderDataInstructions.Count);

                foreach (XmlNode node in renderDataInstructions)
                {
                    McgRenderDataInstruction newInstruction = null;

                    XmlNodeList fields = node.SelectNodes("Field");

                    Debug.Assert(fields.Count >= 0); // Validated by the schema file.

                    newInstruction = new McgRenderDataInstruction(
                        ToString(node, "Name"),
                        ToString(node, "UnmanagedName", String.Empty),
                        ToString(node, "Comment", String.Empty),
                        ToEnum<Modifier>(node, "Modifier", Modifier.@public),
                        ToBoolean(node, "ManagedOnly", false)
                        );

                    _renderDataInstructionData.AddRenderDataInstruction(newInstruction);

                    allRenderDataInstructions.Add(newInstruction);
                }

                _renderDataInstructionData.RenderDataInstructions = (McgRenderDataInstruction[]) allRenderDataInstructions.ToArray(typeof(McgRenderDataInstruction));
            }
        }

        /// <summary>
        /// CreateEnumValue - Create an McgEnumValue from an EnumValue node
        /// </summary>
        /// <returns>
        /// McgEnumValue - the object describing the EnumValue node
        /// </returns>
        /// <param name="node"> The node from which the McgEnumValue will be created </param>
        private McgEnumValue CreateEnumValue(XmlNode node)
        {
            Debug.Assert(node.Name == "Field");

            return new McgEnumValue(
                        ResourceModel.ToString(node, "Name"),
                        ResourceModel.ToBoolean(node, "UnmanagedOnly", false),
                        ResourceModel.ToString(node, "Value", null),
                        ResourceModel.ToString(node, "Comment", null));
        }

        /// <summary>
        /// CreateEnumChildArray - Create an array of IMcgEnumChild[] describing
        /// a node's children
        /// </summary>
        /// <returns>
        /// IMcgEnumChild[] - the children in IMcgEnumChild form
        /// </returns>
        /// <param name="node"> The node from whose children the array is created </param>
        private IMcgEnumChild[] CreateEnumChildArray(XmlNode node)
        {
            ArrayList alChildren = new ArrayList();

            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.Name == "Field")
                {
                    alChildren.Add(CreateEnumValue(child));
                }
                else if (child.Name == "BlockCommentedFields")
                {
                    ArrayList alGrandchildren = new ArrayList();

                    foreach (XmlNode grandchild in child.ChildNodes)
                    {
                        if (grandchild.Name == "Field")
                        {
                            alGrandchildren.Add(CreateEnumValue(grandchild));
                        }
                    }

                    alChildren.Add(new McgBlockCommentedEnumValues(
                        (McgEnumValue[])alGrandchildren.ToArray(typeof(McgEnumValue)),
                        ToString(child, "Comment", null)));
                }
                //
                // We ignore other names (note that XML comments are child nodes)
                //
            }

            return (IMcgEnumChild[]) alChildren.ToArray(typeof(IMcgEnumChild));
        }

        /// <summary>
        /// FindField - find a field in a list given its name, return null if not found.
        /// </summary>
        /// <returns>
        /// McgField - the field which matches the incoming string name.
        /// </returns>
        /// <param name="fields"> McgField[] - the array of fields to search. </param>
        /// <param name="name"> string - the name of the field to find. </param>
        McgField FindField(McgField[] fields, string name)
        {
            foreach (McgField field in fields )
            {
                if (String.Compare(name, field.Name, false /* do not ignore case */, System.Globalization.CultureInfo.InvariantCulture) == 0)
                {
                    return field;
                }
            }

            return null;
        }

        /// <summary>
        /// Pass 2
        ///
        /// Set up links between objects, and any dependent data.
        /// </summary>
        private void CreateTypeReferences(XmlDocument document)
        {
            //
            // Structs
            //

            XmlNodeList primitives = document.SelectNodes(
                "/MilTypes/Structs/Struct");

            foreach (XmlNode node in primitives)
            {
                McgStruct type = FindStruct(node.Attributes["Name"].Value);
                McgStruct extendsType = null;

                string sExtends = ToString(node, "Extends", "");
                if (sExtends != "")
                {
                    extendsType = FindStruct(sExtends);
                }

                type.Initialize(
                    CreateStructChildArray(node.SelectNodes("Fields")),
                    extendsType);
            }


            //
            // Commands
            //
            // 
            //   1. Link command targets to resources.
            //   2. Resolve handle types.
            //

            XmlNodeList commands =
                document.SelectNodes("/MilTypes/Commands/Command");

            foreach (XmlNode node in commands)
            {
                XmlAttribute domain = node.Attributes["Domain"];
                XmlAttribute target = node.Attributes["Target"];
                XmlAttribute name = node.Attributes["Name"];
                McgCommand type = FindCommand(
                    McgStruct.CreateFullName(
                        (domain != null) ? domain.Value : null,
                        (target != null) ? target.Value : null,
                        (name != null) ? name.Value : null
                        ));

                type.Initialize(
                    CreateStructChildArray(node.SelectNodes("Fields"))
                    );
            }

            //
            // Resources
            //

            XmlNodeList resourceTypes = document.SelectNodes(
                "/MilTypes/Resources/Resource");

            foreach (XmlNode node in resourceTypes)
            {
                McgResource type = FindResource(node.Attributes["Name"].Value);

                McgResource extendsType = null;
                McgType collectionType = null;

                string sExtends = ToString(node, "Extends", "");
                if (sExtends != "")
                {
                    extendsType = FindResource(sExtends);
                }

                string sCollectionType = ToString(node, "CollectionType", "");
                if (sCollectionType != "")
                {
                     collectionType = FindType(sCollectionType);
                }

                type.Initialize(
                    CreateStructChildArray(node.SelectNodes("Fields")),
                    CreateEmptyField(node.SelectNodes("EmptyField")),
                    extendsType,
                    collectionType);
            }

            XmlNodeList renderDataInstructions = document.SelectNodes(
                "/MilTypes/RenderDataInstructions/RenderDataInstruction");

            foreach (XmlNode node in renderDataInstructions)
            {
                IMcgStructChild[] children = CreateStructChildArray(node.SelectNodes("Fields"));

                McgRenderDataInstruction renderDataInstruction = _renderDataInstructionData.FindRenderDataInstruction(node.Attributes["Name"].Value);

                McgField[] rdFields = McgField.StructChildArrayToFieldArray(children);
                McgField[][] rdGroups = null;

                // Grab the list of groups.
                XmlNodeList noOpGroups = node.SelectNodes("NoOpGroups/NoOpGroup");

                if (noOpGroups.Count > 0)
                {
                    ArrayList rdNoOpGroups = new ArrayList(noOpGroups.Count);

                    foreach (XmlNode noOpGroupNode in noOpGroups)
                    {
                        XmlNodeList noOpGroupMembers = noOpGroupNode.SelectNodes("NoOpGroupMember");

                        // This is enforced by the schema
                        Debug.Assert(noOpGroupMembers.Count > 0);

                        ArrayList rdNoOpGroupMembers = new ArrayList(noOpGroupMembers.Count);

                        foreach (XmlNode noOpGroupMemberNode in noOpGroupMembers)
                        {
                            McgField field = FindField(rdFields, ToString(noOpGroupMemberNode, "FieldName"));

                            // The field name must match a real field
                            Debug.Assert(field != null);

                            rdNoOpGroupMembers.Add(field);
                        }

                        rdNoOpGroups.Add((McgField[]) rdNoOpGroupMembers.ToArray(typeof(McgField)));
                    }

                    rdGroups = (McgField[][])rdNoOpGroups.ToArray(typeof(McgField[]));
                }

                renderDataInstruction.Initialize(rdFields, rdGroups);
            }
        }

        /// <summary>
        /// Used when building the resource model.
        /// </summary>
        private void AddGenerationControl(int iSection, McgType type)
        {
            Debug.Assert((GenerationControl != null) && (iSection < GenerationControl.Length));

            if (GenerationControl[iSection] == null)
            {
                GenerationControl[iSection] = new List<McgType>(1);
            }

            GenerationControl[iSection].Add(type);
        }

        /// <summary>
        /// Looks up the generation control for the given "section" of generated code
        /// (i.e. something like "Native" or "ManagedClass").
        /// </summary>
        public bool ShouldGenerate(CodeSections section, McgType type)
        {
            return (GenerationControl[(int)section] != null) && (GenerationControl[(int)section].IndexOf(type) >= 0);
        }

        /// <summary>
        /// Pass 3
        ///
        /// Set up the list of work to do
        /// </summary>
        private void CreateGenerationList(XmlDocument document)
        {
            //
            // Generation control
            //

            XmlNodeList generateControl = document.SelectNodes(
                "/MilTypes/GenerationControl");

            string [] rgsCodeSections = System.Enum.GetNames(typeof(CodeSections));
            int [] rgiCodeSections = (int []) System.Enum.GetValues(typeof(CodeSections));

            foreach (XmlNode generateNode in generateControl)
            {
                bool fGenerateOldStyle = ToBoolean(generateNode, "OldStyle", false);

                string nativeDestinationDir = ToString(generateNode, "NativeDestinationDir", null);
                string managedDestinationDir = ToString(generateNode, "ManagedDestinationDir", null);
                string converterDestinationDir = ToString(generateNode, "ConverterDestinationDir", null);
                string managedSharedDestinationDir = ToString(generateNode, "ManagedSharedDestinationDir", null);

                XmlNodeList generateElements = generateNode.ChildNodes;

                foreach (XmlNode node in generateElements)
                {
                    if (node.Attributes != null)
                    {
                        string sName = node.Attributes["Name"].Value;
                        McgType type = FindType(sName);

                        // Build a hash table of booleans

                        bool fNoGeneration = true;

                        for (int i=0; i<rgsCodeSections.Length; i++)
                        {
                            if (ToBoolean(node, rgsCodeSections[i], false))
                            {
                                fNoGeneration = false;
                                AddGenerationControl(rgiCodeSections[i], type);
                            }
                        }


                        //
                        // Instead of having a GenerationControl class, the
                        // generation control information is stored directly in
                        // the types. Since there is not a 1:1 correspondence
                        // between types and GenerationControl's, we have to be
                        // careful not to overwrite previous directives. This is
                        // a deficiency and should be fixed.
                        //

                        type.IsOldStyle = type.IsOldStyle || fGenerateOldStyle;

                        if (nativeDestinationDir != null)
                        {
                            if (type.NativeDestinationDir != null)
                            {
                                ThrowValidationException(
                                    String.Format("duplicate NativeDestinationDir: '{0}'", type.Name));
                            }

                            type.NativeDestinationDir = nativeDestinationDir;
                        }

                        if (managedDestinationDir != null)
                        {
                            if (type.ManagedDestinationDir != null)
                            {
                                ThrowValidationException(
                                    String.Format("duplicate ManagedDestinationDir: '{0}'", type.Name));
                            }

                            type.ManagedDestinationDir = managedDestinationDir;
                        }

                        if (converterDestinationDir != null)
                        {
                            if (type.ConverterDestinationDir != null)
                            {
                                ThrowValidationException(
                                    String.Format("duplicate ConverterDestinationDir: '{0}'", type.Name));
                            }

                            type.ConverterDestinationDir = converterDestinationDir;
                        }


                        if (managedSharedDestinationDir != null)
                        {
                            if (type.ManagedSharedDestinationDir != null)
                            {
                                ThrowValidationException(
                                    String.Format("duplicate ManagedSharedDestinationDir: '{0}'", type.Name));
                            }

                            type.ManagedSharedDestinationDir = managedSharedDestinationDir;
                        }


                        // Validate the generation commands

                        // It is illegal to generate McgPrimitives
                        if (   (type is McgPrimitive)
                            && (!fNoGeneration))
                        {
                            ThrowValidationException(String.Format(
                                "'{0}' cannot be generated because it is a primitive.",
                                type.Name));
                        }

                        // (ValueMethods || ManagedTypeConverter) -> ManagedClass
                        if (!ShouldGenerate(CodeSections.ManagedClass, type))
                        {
                            if (ShouldGenerate(CodeSections.ManagedValueMethods, type))
                            {
                                ThrowValidationException(String.Format(
                                    "Generation control for '{0}' is inconsistent. 'ManagedClass' is missing.",
                                    type.Name));
                            }
                        }

                        // AnimationClockResource classes are only valid on value types
                        if (ShouldGenerate(CodeSections.AnimationResource, type) && !type.IsValueType)
                        {
                            ThrowValidationException(String.Format(
                                "'{0}' cannot have an associated AnimationClockResource because it is not a value type.",
                                type.Name));
                        }
                    }
                }
            }

            //
            // Template generation control
            //

            XmlNodeList templateGenerateControl = document.SelectNodes(
                "/MilTypes/TemplateGenerationControl");

            foreach (XmlNode templateGenerateNode in templateGenerateControl)
            {
                XmlNodeList templateElements = templateGenerateNode.ChildNodes;

                foreach (XmlNode node in templateElements)
                {
                    if (node.Attributes != null)
                    {
                        string sName = node.Attributes["Name"].Value;

                        System.Console.Out.WriteLine("Found template " + sName);

                        Type type = Assembly.GetExecutingAssembly().GetType(sName, true);

                        if (type.IsSubclassOf(typeof(Template)) && !type.IsAbstract)
                        {
                            //
                            // Create an instance of the class, and call its Go() method.
                            //

                            Template template = (Template) Activator.CreateInstance(
                                type,
                                null
                                );

                            TemplateGenerationControl.Add(template);

                            XmlNodeList instanceElements = node.ChildNodes;

                            foreach (XmlNode instanceNode in instanceElements)
                            {
                                template.AddTemplateInstance(this, instanceNode);
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Get the McgType for a given field. This is slightly complicated
        /// because it can either be specified as an attribute or as a Type
        /// node. The latter is the only syntax that can express arrays.
        /// </summary>
        private McgType GetFieldType(XmlNode fieldNode)
        {
            string typeAttribute = ToString(fieldNode, "Type", null);

            XmlNodeList typeNodes = fieldNode.SelectNodes("Type");

            Debug.Assert(typeNodes.Count <= 1); // schema enforces this

            if (typeAttribute != null && typeNodes.Count == 1)
            {
                ThrowValidationException(fieldNode, "Duplicate type descriptions");
                return null;
            }
            else if (typeAttribute == null && typeNodes.Count == 0)
            {
                ThrowValidationException(fieldNode, "Missing type description");
                return null;
            }
            else if (typeAttribute != null)
            {
                return FindType(typeAttribute);
            }
            else
            {
                Debug.Assert(typeNodes.Count == 1);

                XmlNode typeNode = typeNodes[0];

                McgArray array = new McgArray(this, FindType(ToString(typeNode, "Name")));

                XmlNodeList arrayDimensions = typeNode.SelectNodes("ArrayDimension");

                ArrayList alArrayDimensions = new ArrayList();

                foreach (XmlNode dim in arrayDimensions)
                {
                    alArrayDimensions.Add(new McgArrayDimension(ToString(dim, "Size", String.Empty)));
                }

                array.Initialize((McgArrayDimension[]) alArrayDimensions.ToArray(typeof(McgArrayDimension)));

                return array;
            }
        }

        /// <summary>
        /// Create an McgField from its corresponding XmlNode. This code assumes
        /// that all types have all ready been processed
        /// </summary>
        private McgField CreateField(XmlNode node)
        {
            McgType type = GetFieldType(node);
            bool isAnimate = ToBoolean(node, "Animate", false);
            string propertyAlias = ToString(node, "PropertyAlias", null);
            bool isReadOnly = ToBoolean(node, "ReadOnly", false);
            bool isManagedOnly = ToBoolean(node, "ManagedOnly", false);
            bool isUnmanagedOnly = ToBoolean(node, "UnmanagedOnly", false);
            bool isValidate = ToBoolean(node, "Validate", false);
            bool isNew = ToBoolean(node, "IsNew", false);
            string defaultValue = ToString(node, "Default", "");
            bool isInternal = ToBoolean(node, "IsInternal", false);
            bool isProtected = ToBoolean(node, "IsProtected", false);
            bool propertyChangedHook = ToBoolean(node, "PropertyChangedHook", false);
            bool isCommonlySet = ToBoolean(node, "IsCommonlySet", false);
            string coerceValueCallback = ToString(node, "CoerceValueCallback", null);
            bool serializationVisibility = ToBoolean(node, "SerializationVisibility", true);
            string typeConverter = ToString(node, "TypeConverter", null);
            bool cachedLocally = ToBoolean(node, "CachedLocally", false);

            if (isAnimate)
            {
                if (!type.IsValueType)
                {
                    Helpers.CodeGenHelpers.ThrowValidationException(
                        node,
                        "Animate=\"true\" is not valid for a resource that is passed by handle.");
                }
            }

            return new McgField(
                ToString(node, "Name"),
                ToString(node, "UnmanagedName", null),
                type,
                ToString(node, "Comment", null),
                isAnimate,
                propertyAlias,
                isReadOnly,
                isManagedOnly,
                isUnmanagedOnly,
                isValidate,
                isNew,
                defaultValue,
                isInternal,
                isProtected,
                propertyChangedHook,
                isCommonlySet,
                coerceValueCallback,
                serializationVisibility,
                typeConverter,
                cachedLocally);
        }

        /// <summary>
        /// Create an array of IMcgStructChild[] from "Fields" nodes.
        /// </summary>
        private IMcgStructChild[] CreateStructChildArray(XmlNodeList fieldsNodes)
        {
            ArrayList alChildren = new ArrayList();

            foreach (XmlNode parent in fieldsNodes)
            {
                alChildren.AddRange(CreateStructChildArray(parent));
            }

            return (IMcgStructChild[]) alChildren.ToArray(typeof(IMcgStructChild));
        }

        /// <summary>
        /// Create an array of IMcgStructChild[] from a parent node.
        /// </summary>
        private IMcgStructChild[] CreateStructChildArray(XmlNode parent)
        {
            ArrayList alChildren = new ArrayList();

            foreach (XmlNode node in parent.ChildNodes)
            {
                if (node.Name == "Field")
                {
                    alChildren.Add(CreateField(node));
                }
                else if (node.Name == "Union")
                {
                    alChildren.Add(new McgUnion(CreateStructChildArray(node)));
                }
                else if (node.Name == "BlockCommentedFields")
                {
                    alChildren.Add(
                        new McgBlockCommentedFields(
                            CreateStructChildArray(node),
                            ToString(node, "Comment")
                            ));
                }
                else if (node.Name == "Constant")
                {
                    alChildren.Add(
                        new McgConstant(
                            ToString(node, "Name"),
                            ToString(node, "Value")
                            ));
                }
            }

            return (IMcgStructChild[]) alChildren.ToArray(typeof(IMcgStructChild));
        }

        private McgEmptyField CreateEmptyField(XmlNodeList emptyFields)
        {
            Debug.Assert(emptyFields.Count <= 1,
                "Expected no more than 1 EmptyField per resource.");

            foreach (XmlNode node in emptyFields)
            {
                return new McgEmptyField(ToString(node, "Name"),
                                         ToBoolean(node, "GenerateClass", true),
                                         ToBoolean(node, "Distinguished", false));
            }

            return null;
        }

        private McgRealization CreateRealization(XmlNode resourceType)
        {
            XmlNodeList xnl = resourceType.SelectNodes("Realization");

            if (xnl.Count == 0)
            {
                return null;
            }

            XmlNode node = xnl[0];

            return new McgRealization(
                ToString(node, "RealizationType"),
                ToString(node, "RealizationTypeName"),
                ToBoolean(node, "RefCountRealization", false),
                ToBoolean(node, "CacheRealization", false),
                CreateRealizationAPI(node));
        }

        private McgRealizationAPI CreateRealizationAPI(XmlNode realization)
        {
            XmlNodeList xnl = realization.SelectNodes("API");

            if (xnl.Count == 0)
            {
                return null;
            }

            XmlNode node = xnl[0];

            string paramName = "";
            string paramType = "";
            XmlNodeList xnlParam = node.SelectNodes("Param");

            if (xnlParam.Count > 0)
            {
                paramName = ToString(xnlParam[0], "Name");
                paramType = ToString(xnlParam[0], "Type");
            }

            return new McgRealizationAPI(
                ToString(node, "Name"),
                paramName,
                paramType);
        }

        /// <summary>
        /// Get the McgCommand for the given string. Throws an
        /// exception if not found or of the wrong type.
        /// </summary
        private McgCommand FindCommand(string fullName)
        {
            object command = _commandLookupTable[fullName];

            if (command == null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(
                    String.Format(
                        "Command expected, '{0}' given",
                        fullName
                        ));
            }

            return (McgCommand)command;
        }

        /// <summary>
        /// Get the McgResource for the given string. Throws an
        /// exception if not found or of the wrong type.
        /// </summary>
        private McgResource FindResource(string name)
        {
            McgType mt = FindType(name);
            if (mt is McgResource)
            {
                return (McgResource) mt;
            }

            Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                "Resource expected, '{0}' given",
                name));

            return null;
        }

        /// <summary>
        /// Get the McgEnum for the given string. Throws an
        /// exception if not found or of the wrong type.
        /// </summary>
        private McgEnum FindEnum(string name)
        {
            McgType mt = FindType(name);
            if (mt is McgEnum)
            {
                return (McgEnum) mt;
            }

            Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                "Enum type expected, '{0}' given",
                name));

            return null;
        }


        /// <summary>
        /// Get the McgStruct for the given string. Throws an
        /// exception if not found or of the wrong type.
        /// </summary>
        private McgStruct FindStruct(string name)
        {
            McgType mt = FindType(name);
            if (mt is McgStruct)
            {
                return (McgStruct) mt;
            }

            Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                "Struct type expected, '{0}' given",
                name));

            return null;
        }

        /// <summary>
        /// Add a type to the hash table.
        /// </summary>
        private void AddType(McgType type)
        {
            if (_types[type.Name] != null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Duplicate type: {0}",
                    type.Name));
            }
            _types.Add(type.Name, type);
        }

        /// <summary>
        /// Convert the given attribute to a string. Throws if null or an empty string.
        /// </summary>
        internal static string ToString(XmlNode xn, string attr)
        {
            if (xn.Attributes[attr] != null)
            {
                string value = xn.Attributes[attr].Value;

                if (value.Trim() == "")
                {
                    Helpers.CodeGenHelpers.ThrowValidationException(xn, "Empty attribute: " + attr);
                }

                return value;
            }

            Helpers.CodeGenHelpers.ThrowValidationException(xn, "Missing attribute: " + attr);
            return null;
        }

        /// <summary>
        /// Convert the given attribute to a string. Uses the given default if null.
        /// </summary>
        internal static string ToString(XmlNode xn, string attr, string defaultValue)
        {
            if (xn.Attributes[attr] != null)
            {
                return xn.Attributes[attr].Value;
            }

            return defaultValue;
        }

        /// <summary>
        /// Convert the given attribute to a boolean. Uses the given default if null.
        /// </summary>
        internal static bool ToBoolean(XmlNode xn, string attr, bool defaultValue)
        {
            if (xn.Attributes[attr] == null)
            {
                return defaultValue;
            }

            string value = xn.Attributes[attr].Value;
            bool result = false;
            try
            {
                result = XmlConvert.ToBoolean(value);
            }
            catch (FormatException)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(
                    xn,
                    String.Format(
                        "Invalid value for boolean attribute {0}: '{1}'",
                        attr,
                        value));
            }

            return result;
        }

        /// <summary>
        /// Convert the given attribute to an Int32. Throws if not present.
        /// </summary>
        internal static int ToInt32(XmlNode xn, string attr)
        {
            if (xn.Attributes[attr] == null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(xn, "Missing attribute: " + attr);
            }

            string value = xn.Attributes[attr].Value;
            int result = 0;
            try
            {
                result = XmlConvert.ToInt32(value);
            }
            catch (FormatException)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(
                    xn,
                    String.Format(
                        "Invalid value for integer attribute {0}: '{1}'",
                        attr,
                        value));
            }

            return result;
        }

        /// <summary>
        /// Convert the given attribute to an Int32. Uses the given default if null.
        /// </summary>
        internal static int ToInt32(XmlNode xn, string attr, int defaultValue)
        {
            if (xn.Attributes[attr] == null)
            {
                return defaultValue;
            }

            return ToInt32(xn, attr);
        }

        private static T ToEnum<T>(XmlNode xn, string attr, T defaultValue)
        {
            if (xn.Attributes[attr] == null)
            {
                return defaultValue;
            }

            string value = xn.Attributes[attr].Value;
            T result = defaultValue;

            try
            {
                result = (T) Enum.Parse(typeof(T), value, /* ignoreCase = */ false);
            }
            catch (FormatException)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(
                    xn,
                    String.Format(
                        "Invalid value for enum attribute {0}: '{1}'",
                        attr,
                        value));
            }

            return result;
        }

        /// <summary>
        /// Add a command to the hash table.
        /// </summary>
        private void AddCommand(McgCommand command)
        {
            if (_commandLookupTable[command.FullName] != null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Duplicate command: {0}",
                    command.FullName
                    ));
            }

            _commandLookupTable.Add(command.FullName, command);
        }

        private static void ThrowValidationException(XmlNode node, string message)
        {
            throw new ApplicationException(String.Format(
                "Validation failed at node {0}: {1}",
                node.Name,
                message));
        }

        private static void ThrowValidationException(string message)
        {
            throw new ApplicationException(String.Format(
                "Validation failed: {0}",
                message));
        }

        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private Hashtable _types;
        private McgResource[] _resourceTypes;
        private McgEnum[] _enumTypes;
        private McgRealization[] _realizations;
        private McgType[] _allTypes;
        private string _outputDir;
        private RenderDataInstructionData _renderDataInstructionData;
        private Hashtable _commandLookupTable;
        private McgCommand[] _commands;
        private List<McgType>[] GenerationControl = new List<McgType>[(int)CodeSections.CodeSectionCount];
        internal List<Template> TemplateGenerationControl = new List<Template>();

        #endregion Private Fields
    }
}




