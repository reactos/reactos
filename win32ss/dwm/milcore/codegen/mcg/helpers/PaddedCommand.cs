// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      PaddedCommand.cs
//
// Abstract:
//      Generic iteration over MILCE protocol commands.
//
//
// Description:
//      The resource model can define a MILCE protocol command in three
//      ways: as a McgCommand, McgResource or McgRenderDataInstruction.
//      The PaddedCommand class is a helper for the code generators that
//      hides the resource model complexities by exposing only the
//      necessary and preprocessed data.
//
//      Command structure fields are padded and aligned as appropriate.
//      Type/Handle/Size explicit fields are added to the definitions
//      where necessary.
//
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;


    /// <summary>
    /// Describes the origin of a PaddedCommand -- whether it was created
    /// from a McgCommand, McgResource or McgRenderDataInstruction object.
    /// </summary>
    public enum PaddedCommandOrigin
    {
        Invalid = 0,
        Command,
        Resource,
        RenderDataInstruction
    }


    /// <summary>
    /// Collection of PaddedCommand objects. Extracts the commands
    /// from the resource model and provides iteration over the
    //  PaddedCommand array.
    /// </summary>
    public class PaddedCommandCollection
    {
        public PaddedCommandCollection(ResourceModel resourceModel)
        {
            m_paddedCommands = new List<PaddedCommand>();

            foreach (McgCommand command in resourceModel.Commands)
            {
                m_paddedCommands.Add(new PaddedCommand(resourceModel, command));
            }

            foreach (McgResource resource in resourceModel.Resources)
            {
                if (resourceModel.ShouldGenerate(CodeSections.NativeDuce, resource)
                    && !resource.IsAbstract)
                {
                    m_paddedCommands.Add(new PaddedCommand(resourceModel, resource));
                }
            }

            foreach (McgRenderDataInstruction instruction
                in resourceModel.RenderDataInstructionData.RenderDataInstructions)
            {
                m_paddedCommands.Add(new PaddedCommand(resourceModel, instruction, false));

                if (instruction.HasAdvancedParameters)
                {
                    m_paddedCommands.Add(new PaddedCommand(resourceModel, instruction, true));
                }
            }
        }

        public List<PaddedCommand> PaddedCommands
        {
            get
            {
                return m_paddedCommands;
            }
        }

        private readonly List<PaddedCommand> m_paddedCommands;
    }


    /// <summary>
    /// A proxy between the resource model and the code generators.
    /// Generalizes the different command models and hides unnecessary
    /// details from the generators.
    /// </summary>
    public class PaddedCommand
    {
        // -----------------------------------------------------------------
        //
        //   Public Constructors
        //
        // -----------------------------------------------------------------

        #region Public Constructors

        public PaddedCommand(ResourceModel resourceModel, McgCommand command)
        {
            m_resourceModel = resourceModel;
            m_origin = PaddedCommandOrigin.Command;
            m_domain = command.Domain;
            m_target = command.Target;
            m_name = command.Name;
            m_commandName = command.CommandName;
            m_typeName = command.CommandTypeName;


            //
            // Special case for some non-uniformly named targets
            //
            // 
            if (m_target == "Bitmap")
            {
                m_targetResourceType = "TYPE_BITMAPSOURCE";
            }
            else if (m_target == "Target")
            {
                m_targetResourceType = "TYPE_RENDERTARGET";
            }
            else if (m_target == "HwndTarget")
            {
                m_targetResourceType = "TYPE_HWNDRENDERTARGET";
            }
            else if (m_target == "IntermediateTarget")
            {
                m_targetResourceType = "TYPE_INTERMEDIATERENDERTARGET";
            }
            else if (m_target == "GenericTarget")
            {
                m_targetResourceType = "TYPE_GENERICRENDERTARGET";
            }
            else
            {
                m_targetResourceType = "TYPE_" + m_target.ToUpper();
            }

            m_targetResourceName = command.TargetResourceName;
            m_commandIsUnmanagedOnly = command.UnmanagedOnly;
            m_commandIsSecurityCritical = command.IsSecurityCritical;
            m_hasPayload = command.HasPayload;
            m_failIfCommandTransportDenied = command.FailIfCommandTransportDenied;
            m_securityComment = command.SecurityComment;

            //
            // Do not pad structures generated from McgCommand objects.
            //

            m_paddedStructData =
                new Helpers.CodeGenHelpers.PaddedStructData(command.AllUceFields);

            if (Domain != "RenderData")
            {
                //
                // Commands generated from McgCommand objects need a Type field
                // prepended.
                //
                // Need to refactor the render data command
                // generation so that we can prepend the "Type" field uniformly.
                //

                PrependTypeField();
            }

            m_paddedStructData.BuildMarshaledFieldNames();
        }

        public PaddedCommand(ResourceModel resourceModel, McgResource resource)
        {
            m_resourceModel = resourceModel;
            m_origin = PaddedCommandOrigin.Resource;
            m_domain = resource.Domain;
            m_target = resource.Target;
            m_name = "Update";
            m_commandName = resource.CommandName;
            m_targetResourceType = "TYPE_" + resource.Name.ToUpper();
            m_targetResourceName = resource.DuceClass;
            m_resourceIsCollection = resource.IsCollection;
            m_typeName = resource.CommandTypeName;

            //
            // Do not generate managed structures for unmanaged only resources.
            //

            m_commandIsUnmanagedOnly =
                !resourceModel.ShouldGenerate(CodeSections.ManagedClass, resource);

            //
            // Fixup for resources having payloads.
            //
            // 
            //

            if (   resource.Name == "LinearGradientBrush"
                || resource.Name == "RadialGradientBrush"
                || resource.Name == "TransformGroup"
                || resource.Name == "BitmapEffectGroup"
                || resource.Name == "PixelShader"
                || resource.Name == "ShaderEffect"
                || resource.Name == "MultipassShaderEffect"
                || resource.Name == "DrawingGroup"
                || resource.Name == "PathGeometry"
                || resource.Name == "GeometryGroup"
                || resource.Name == "MaterialGroup"
                || resource.Name == "Model3DGroup"
                || resource.Name == "Transform3DGroup"
                || resource.Name == "ScreenSpaceLines3D"
                || resource.Name == "MeshGeometry3D"
                || resource.Name == "DashStyle"
                || resource.Name == "GuidelineSet"
                || resource.Name == "MeshGeometry2D"
                || resource.Name == "Geometry2DGroup"
                ) 
            {            
                m_hasPayload = true;
            }

            m_paddedStructData =
                Helpers.CodeGenHelpers.SortStructForAlignment(
                    resource.AllUceFields,
                    true /* do animations */,
                    false /* don't pad entire structure */
                    );

            //
            // Prepend "Type" and "Handle" fields (in that order!)
            //

            PrependHandleField();
            PrependTypeField();

            if (m_resourceIsCollection)
            {
                //
                // Append the "Size" field for resource collections.
                //

                AppendCollectionSizeField();
            }

            m_paddedStructData.BuildMarshaledFieldNames();
        }

        public PaddedCommand(
            ResourceModel resourceModel,
            McgRenderDataInstruction instruction,
            bool animated
            )
        {
            m_resourceModel = resourceModel;
            m_origin = PaddedCommandOrigin.RenderDataInstruction;
            m_domain = "RenderDataInstruction";
            m_target = String.Empty;
            m_name = String.Empty;
            m_targetResourceType = String.Empty;
            m_targetResourceName = String.Empty;
            m_commandName = instruction.StructName + (animated ? "_ANIMATE" : "");
            m_typeName = "Mil" + instruction.Name + (animated ? "Animate" : "");

            m_paddedStructData =
                instruction.GetPaddedStructDefinition(animated);

            m_paddedStructData.BuildMarshaledFieldNames();
        }

        #endregion Public Constructors


        // -----------------------------------------------------------------
        //
        //   Public Properties
        //
        // -----------------------------------------------------------------

        #region Public Properties

        public PaddedCommandOrigin Origin
        {
            get
            {
                return m_origin;
            }
        }

        public string Domain
        {
            get
            {
                return m_domain;
            }
        }

        public string Target
        {
            get
            {
                return m_target;
            }
        }

        public string Name
        {
            get
            {
                return m_name;
            }
        }

        public string CommandName
        {
            get
            {
                return m_commandName;
            }
        }

        public string TypeName
        {
            get
            {
                return m_typeName;
            }
        }

        public bool ResourceIsCollection
        {
            get
            {
                return m_resourceIsCollection;
            }
        }

        public bool CommandIsUnmanagedOnly
        {
            get
            {
                return m_commandIsUnmanagedOnly;
            }
        }

        public bool CommandIsSecurityCritical
        {
            get
            {
                return m_commandIsSecurityCritical;
            }
        }

        public bool HasPayload
        {
            get
            {
                return m_hasPayload;
            }
        }

        public bool FailIfCommandTransportDenied
        {
            get
            {
                return m_failIfCommandTransportDenied;
            }
        }

        public string SecurityComment
        {
            get
            {
                return m_securityComment;
            }
        }

        public Helpers.CodeGenHelpers.PaddedStructData PaddedStructData
        {
            get
            {
                return m_paddedStructData;
            }
        }

        public string TargetResourceType
        {
            get
            {
                return m_targetResourceType;
            }
        }

        public string TargetResourceName
        {
            get
            {
                return m_targetResourceName;
            }
        }

        #endregion Public Properties


        // -----------------------------------------------------------------
        //
        //   Private Methods
        //
        // -----------------------------------------------------------------

        #region Private Methods

        /// <summary>
        /// Prepends a field to this padded command. Alignment is not updated.
        /// </summary>
        /// <remarks>
        /// 
        /// </remarks>
        private void PrependField(McgField field)
        {
            Helpers.CodeGenHelpers.AlignmentEntry entry =
                new Helpers.CodeGenHelpers.AlignmentEntry(field, false, false);

            List<Helpers.CodeGenHelpers.AlignmentEntry> entries =
                new List<Helpers.CodeGenHelpers.AlignmentEntry>(m_paddedStructData.AlignmentEntries);


            //
            // Update the existing field offsets
            //

            foreach (Helpers.CodeGenHelpers.AlignmentEntry existingEntry in entries)
            {
                existingEntry.Offset += field.Type.UnpaddedSize;
            }


            //
            // Prepend the new entry and write back the result
            //

            entries.Insert(0, entry);

            m_paddedStructData.AlignmentEntries =
                entries.ToArray();

            m_paddedStructData.PaddedSize += field.Type.UnpaddedSize;
        }

        /// <summary>
        /// Appends a field to this padded command. Alignment is not updated.
        /// </summary>
        /// <remarks>
        /// 
        /// </remarks>
        private void AppendField(McgField field)
        {
            Helpers.CodeGenHelpers.AlignmentEntry entry =
                new Helpers.CodeGenHelpers.AlignmentEntry(field, false, false);

            List<Helpers.CodeGenHelpers.AlignmentEntry> entries =
                new List<Helpers.CodeGenHelpers.AlignmentEntry>(m_paddedStructData.AlignmentEntries);

            entries.Add(entry);

            m_paddedStructData.AlignmentEntries =
                entries.ToArray();

            m_paddedStructData.PaddedSize += field.Type.UnpaddedSize;
        }

        private void PrependTypeField()
        {
            string kind = (Domain == "Redirection" ? "REDIRECTIONCMD"
                          : (Domain == "DWM" ? "DWMCMD" : "MILCMD"));

            McgField field = new McgField(
                Domain == "DWM" ? "type" : "Type", // special case for DWM commands
                null,
                m_resourceModel.FindType(kind),
                "Implicit command type field",
                false,  // is not animated
                null,   // has no property alias
                true,   // isReadOnly
                false,  // isManagedOnly
                false,  // isUnmanagedOnly
                false,  // isValidate
                false,  // isNew
                null,   // defaultValue
                false,  // isInternal
                false,  // isProtected
                false,  // no PropertyChanged hook
                false,  // is not commonly set
                null,   // CoerceValueCallback is null by default
                true,   // Visible to Serialization
                null,   // no TypeConverter
                false  // locally cached dp
                );

            PrependField(field);
        }

        private void PrependHandleField()
        {
            McgField field = new McgField(
                "Handle",
                null,
                m_resourceModel.FindType("ResourceHandle"),
                "Implicit resource command handle field",
                false,  // is not animated 
                null,   // has no property alias 
                true,   // isReadOnly
                false,  // isManagedOnly
                false,  // isUnmanagedOnly
                false,  // isValidate
                false,  // isNew
                null,   // defaultValue
                false,  // isInternal
                false,  // isProtected
                false,  // no PropertyChanged hook 
                false,  // is not commonly set 
                null,   // CoerceValueCallback is null by default
                true,   // Visible to Serialization
                null,   // no TypeConverter
                false   // locally cached dp
                );

            PrependField(field);
        }

        private void AppendCollectionSizeField()
        {
            McgField field = new McgField(
                "Size",
                null,
                m_resourceModel.FindType("UInt32"),
                "Implicit resource collection size field",
                false,  // is not animated 
                null,   // has no property alias 
                true,   // isReadOnly
                false,  // isManagedOnly
                false,  // isUnmanagedOnly
                false,  // isValidate
                false,  // isNew
                null,   // defaultValue
                false,  // isInternal
                false,  // isProtected
                false,  // no PropertyChanged hook 
                false,  // is not commonly set 
                null,   // CoerceValueCallback is null by default 
                true,   // Visible to Serialization 
                null,   // no TypeConverter
                false   // locally cached dp
                );

            AppendField(field);
        }

        #endregion Private Methods

        // -----------------------------------------------------------------
        //
        //   Private Fields
        //
        // -----------------------------------------------------------------

        #region Private Fields

        private readonly ResourceModel m_resourceModel;
        private readonly PaddedCommandOrigin m_origin;
        private readonly string m_domain;
        private readonly string m_target;
        private readonly string m_name;
        private readonly string m_commandName;
        private readonly bool m_resourceIsCollection;
        private readonly bool m_commandIsUnmanagedOnly;
        private readonly bool m_commandIsSecurityCritical;
        private readonly bool m_hasPayload;
        private readonly bool m_failIfCommandTransportDenied;
        private readonly string m_securityComment;
        private readonly string m_typeName;
        private readonly string m_targetResourceType;
        private readonly string m_targetResourceName;
        private readonly Helpers.CodeGenHelpers.PaddedStructData m_paddedStructData;

        #endregion Private Fields
    }
}




