// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: An object model which represents the MIL render data.
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
    //  Class: McgRenderDataInstruction
    //
    //------------------------------------------------------

    public class McgRenderDataInstruction
    {
        public McgRenderDataInstruction(string name,
                                        string unmanagedName,
                                        string comment,
                                        Modifier modifier,
                                        bool isManagedOnly)
        {
            Name = name;

            if ((unmanagedName == null) || unmanagedName.Length == 0)
            {
                UnmanagedName = name;
            }
            else
            {
                UnmanagedName = unmanagedName;
            }

            _structName = CreateStructName(UnmanagedName);

            Comment = comment;

            // Is this a "Push*" instruction?
            IsPush = String.Compare(name, 0, "Push", 0, 4, false /* don't ignore case */, System.Globalization.CultureInfo.InvariantCulture) == 0;

            // Is this a "Pop*" instruction?
            IsPop = !IsPush && (String.Compare(name, 0, "Pop", 0, 3, false /* don't ignore case */, System.Globalization.CultureInfo.InvariantCulture) == 0);

            Modifier = modifier;
            IsManagedOnly = isManagedOnly;

            // Assert that it's not both
            Debug.Assert(!IsPush || !IsPop);
        }

        /// <summary>
        /// CreateStructName - This method turns a pascal cased name AbcdEfghIjk into a struct name in
        /// the form ABCD_EFGH_IJK.
        /// </summary>
        private static string CreateStructName(string pascalCaseStructName)
        {
            string structName = String.Empty;

            if (pascalCaseStructName.Length <= 0)
            {
                return String.Empty;
            }

            structName += Char.ToUpper(pascalCaseStructName[0]);

            for (int i = 1; i < pascalCaseStructName.Length; i++)
            {
                if (Char.IsUpper(pascalCaseStructName[i])
                    && !(pascalCaseStructName[i - 1] == '3' && pascalCaseStructName[i] == 'D')) // special case for keeping 3D intact
                {
                    structName += "_";
                    structName += pascalCaseStructName[i];
                }
                else
                {
                    structName += char.ToUpper(pascalCaseStructName[i]);
                }
            }

            return structName;
        }

        internal void Initialize(McgField[] fields,
                                 McgField[][] noOpGroups)
        {
            _fields = fields;
            _noOpGroups = noOpGroups;
        }

        public int UnpaddedSize
        {
            get
            {
                int size = 0;

                foreach (McgField field in _fields)
                {
                    McgResource resource = (McgResource)field.Type;

                    if (resource.IsValueType)
                    {
                        size += resource.UnpaddedSize;
                    }
                    else
                    {
                        // If this field is not a value type it will be stored as a handle
                        size += DuceHandle.Size;
                    }
                }

                return size;
            }
        }

        /// <summary>
        /// GetPaddedStructDefinition - returns an array of AlignmentEntry's which describe
        /// the correct, padded/aligned layout for this struct.
        /// It walks the field list laying out the QWORD align fields before the
        /// DWORD align fields.  Each aligned field is 8-byte aligned, and the struct
        /// itself is of QWORD size.
        /// </summary>
        /// <returns>
        /// AlignmentEntry[] - an array of AlignmentEntry's which dictate the fields in the sorted struct.
        /// </returns>
        /// <param name="fields"> The fields to sort. </param>
        /// <param name="doAdvancedParameters"> Whether or not to add extra handles for animations & advanced parameters. </param>
        public Helpers.CodeGenHelpers.PaddedStructData GetPaddedStructDefinition(bool doAdvancedParameters)
        {
            if (doAdvancedParameters)
            {
                if (_advancedLayout == null)
                {
                    _advancedLayout = Helpers.CodeGenHelpers.SortStructForAlignment(
                        AllFields,
                        true /* do animations */,
                        true /* pad entire structure */
                        );
                }

                return _advancedLayout;
            }
            else
            {
                if (_basicLayout == null)
                {
                    _basicLayout = Helpers.CodeGenHelpers.SortStructForAlignment(
                        BasicFields,
                        false /* don't do animations */,
                        true /* pad entire structure */
                        );
                }

                return _basicLayout;
            }
        }

        /// <summary>
        /// GetPaddedSize - returns the padded size for this struct optionally including animations.
        /// </summary>
        /// <returns>
        /// int - the padded size for this struct with or without animations (based on "doAnimations").
        /// </returns>
        /// <param name="doAnimations"> bool - whether or not to consider animatations. </param>
        public int GetPaddedSize(bool doAnimations)
        {
            return GetPaddedStructDefinition(doAnimations).PaddedSize;
        }

        public readonly string Name;
        public readonly string UnmanagedName;
        public readonly string Comment;

        private string _structName;

        /// <summary>
        /// StructName - This property returns the MILCMD_* struct name for this renderdata instruction.
        /// </summary>
        public string StructName
        {
            get
            {
                return "MILCMD_" + _structName;
            }
        }

        /// <summary>
        /// IncStructName - This property returns the CMD_* struct name for this renderdata instruction.
        /// This is used for the .inc file generation of structs.
        /// </summary>
        public string IncStructName
        {
            get
            {
                return "CMD_" + _structName;
            }
        }

        // These are true if the instruction is a Push* or Pop instruction respectively.
        public readonly bool IsPush;
        public readonly bool IsPop;
        public readonly Modifier Modifier;
        public readonly bool IsManagedOnly;

        public bool HasAdvancedParameters
        {
            get
            {
                // Lazily allocate advanced parameters
                if (null == _advancedParameters)
                {
                    _advancedParameters = ResourceModel.Filter(AllFields, ResourceModel.IsAnimatedOrAdvancedParameter);
                }

                return _advancedParameters.Length > 0;
            }
        }

        public McgField[] BasicPublicFields
        {
            get
            {
                // Lazily allocate basic public parameters
                if (null == _basicPublicFields)
                {
                    _basicPublicFields = ResourceModel.Filter(AllFields, ResourceModel.IsBasicPublicField);
                }

                return _basicPublicFields;
            }
        }

        public McgField[] BasicFields
        {
            get
            {
                // Lazily allocate basic parameters
                if (null == _basicFields)
                {
                    _basicFields = ResourceModel.Filter(AllFields, ResourceModel.IsBasicField);
                }

                return _basicFields;
            }
        }

        public McgField[] AllPublicFields
        {
            get
            {
                // Lazily allocate all public parameters
                if (null == _allPublicFields)
                {
                    _allPublicFields = ResourceModel.Filter(AllFields, ResourceModel.IsPublicField);
                }

                return _allPublicFields;
            }
        }

        public McgField[] AllFields
        {
            get
            {
                return _fields;
            }
        }

        public McgField[][] NoOpGroups
        {
            get
            {
                return _noOpGroups;
            }
        }

        private McgField[] _basicFields;
        private McgField[] _basicPublicFields;
        private McgField[] _allPublicFields;
        private McgField[] _advancedParameters;

        private McgField[] _fields;
        private McgField[][] _noOpGroups;
        private Helpers.CodeGenHelpers.PaddedStructData _basicLayout;
        private Helpers.CodeGenHelpers.PaddedStructData _advancedLayout;
    }

    public class RenderDataInstructionData
    {
        public RenderDataInstructionData(
            string managedNamespace,
            string managedDestinationDir,
            string nativeDestinationDir,
            string exportDestinationDir,
            string[] namespaces)
        {
            ManagedNamespace = managedNamespace;
            ManagedDestinationDir = managedDestinationDir;
            NativeDestinationDir = nativeDestinationDir;
            ExportDestinationDir = exportDestinationDir;
            Namespaces = namespaces;

            RenderDataInstructionHashTable = new Hashtable();
        }

        /// <summary>
        /// Add a RenderDataInstruction to the hash table.
        /// </summary>
        public void AddRenderDataInstruction(McgRenderDataInstruction renderDataInstruction)
        {
            if (RenderDataInstructionHashTable[renderDataInstruction.Name] != null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Duplicate RenderDataInstruction: {0}",
                    renderDataInstruction.Name));
            }
            RenderDataInstructionHashTable.Add(renderDataInstruction.Name, renderDataInstruction);
        }

        /// <summary>
        /// Get the McgRenderDataInstruction for the given string.
        //  Throws an exception if not found.
        /// </summary>
        public McgRenderDataInstruction FindRenderDataInstruction(string name)
        {
            if (RenderDataInstructionHashTable[name] == null)
            {
                Helpers.CodeGenHelpers.ThrowValidationException(String.Format(
                    "Invalid instruction: {0}",
                    name));
            }

            return (McgRenderDataInstruction)RenderDataInstructionHashTable[name];
        }

        public readonly string ManagedNamespace;
        public readonly string ManagedDestinationDir;
        public readonly string NativeDestinationDir;
        public readonly string ExportDestinationDir;
        public readonly string[] Namespaces;
        public McgRenderDataInstruction[] RenderDataInstructions;
        public Hashtable RenderDataInstructionHashTable;
    }

}



