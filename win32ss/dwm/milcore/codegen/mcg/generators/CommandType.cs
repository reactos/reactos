// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      CommandType.cs
//
// Abstract: 
//      Generator for command type enumerations.
// 
// 
// Description:
//      This generator builds the MILCMD enumeration (wgx_command_types.h).
//
// 
// Example output fragment for wgx_command_types.h:
// 
//      enum MILCMD {
//          ...
//          MilCmdNodeInsertChildAt = 73,
//          ...
//      };
// 
// 
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Xml;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;
    using MS.Internal.MilCodeGen.Helpers;

    public class CommandType : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public CommandType(ResourceModel rm) : base(rm) 
        {
            /* do nothing */
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        public override void Go()
        {
            //
            // Opened the generated files
            //
            
            string generatedPath = 
                Path.Combine(
                    _resourceModel.OutputDirectory,
                    "src\\Graphics\\Include\\Generated"
                    );

            string extensionPath =
                Path.Combine(
                    _resourceModel.OutputDirectory,
                    "src\\Graphics\\exts"
                    );

            FileCodeSink enumFile = 
                new FileCodeSink(generatedPath, "wgx_command_types.h");

            FileCodeSink enumCsFile = 
                new FileCodeSink(generatedPath, "wgx_command_types.cs");

            FileCodeSink extsFile =
                new FileCodeSink(extensionPath, "cmdstruct.h");

            m_redirectionEnum = new StringCodeSink();
            m_redirectionTypes = new StringCodeSink();
            m_dwmEnum = new StringCodeSink();
            m_enum = new StringCodeSink();
            m_milTypes = new StringCodeSink();
            m_exts = new StringCodeSink();


            //
            // Collect the commands from the resource model and start generation.
            //

            PaddedCommandCollection paddedCommands =
                new PaddedCommandCollection(_resourceModel);


            /* ------------------------------------------------------------- */
            /* -- EMIT THE REDIRECTION COMMANDS ---------------------------- */
            /* ------------------------------------------------------------- */

            uint redirectionCommandIndex = 1;

            m_redirectionEnum.Write(
                [[inline]]
                    //--------------------------------------------------------------------------
                    //
                    //  Redirection Commands
                    //
                    //--------------------------------------------------------------------------
                
                [[/inline]]
                );

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.Command 
                    || command.Domain != "Redirection")
                {
                    continue;
                }

                EmitRedirectionCommand(command, redirectionCommandIndex++);
            }

            /* ------------------------------------------------------------- */
            /* -- EMIT REMAINING REDIRECTION TYPES ------------------------- */
            /* ------------------------------------------------------------- */

            foreach (McgEnum enumType in _resourceModel.AllEnums)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.NativeRedirection, enumType))
                {
                    continue;
                }

                EmitRedirectionEnum(enumType);
            }

            /* ------------------------------------------------------------- */
            /* -- EMIT THE DWM COMMANDS ------------------------------------ */
            /* ------------------------------------------------------------- */

            uint dwmCommandIndex = 1;

            m_dwmEnum.Write(
                [[inline]]
                    //--------------------------------------------------------------------------
                    //
                    //  Desktop Window Manager Commands
                    //
                    //--------------------------------------------------------------------------

                [[/inline]]
                );

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.Command 
                    || command.Domain != "DWM")
                {
                    continue;
                }

                EmitDwmCommand(command, dwmCommandIndex++);
            }



            /* ------------------------------------------------------------- */
            /* -- EMIT GENERAL TYPES --------------------------------------- */
            /* ------------------------------------------------------------- */

            foreach (McgEnum enumType in _resourceModel.AllEnums)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.NativeDuce, enumType))
                {
                    continue;
                }

                EmitMilEnum(enumType);
            }


            /* ------------------------------------------------------------- */
            /* -- EMIT THE GENERAL COMMANDS -------------------------------- */
            /* ------------------------------------------------------------- */

            uint commandIndex = 1;

            //
            // The goal here is to build the MILCMD enum and an XSD type
            // containing indices (or types) of the MILCE protocol commands.
            //

            m_enum.Write(
                [[inline]]
                    //--------------------------------------------------------------------------
                    //
                    //  Media Integration Layer Commands
                    //
                    //--------------------------------------------------------------------------

                [[/inline]]
                );

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.Command 
                    || command.Domain == "RenderData"
                    || command.Domain == "Redirection"
                    || command.Domain == "DWM")
                {
                    //
                    // Some render data commands are not generated via the
                    // McgRenderDataInstruction mechanism, we will emit them
                    // later in one group.
                    //

                    continue;
                }

                EmitCommand(command, commandIndex++);
            }


            /* ------------------------------------------------------------- */
            /* -- EMIT THE RENDER DATA COMMANDS ---------------------------- */
            /* ------------------------------------------------------------- */

            m_enum.Write(
                [[inline]]


                    //--------------------------------------------------------------------------
                    //
                    //  Render Data Commands
                    //
                    //--------------------------------------------------------------------------

                [[/inline]]
                );

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.Command 
                    || command.Domain != "RenderData")
                {
                    //
                    // Some render data commands are not generated via the
                    // McgRenderDataInstruction mechanism, emit them before
                    // the render data instructions.
                    //
                    
                    continue;
                }

                EmitCommand(command, commandIndex++);
            }

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.RenderDataInstruction)
                {
                    //
                    // Emit the render data instructions only.
                    //

                    continue;
                }

                EmitCommand(command, commandIndex++);
            }



            /* ------------------------------------------------------------- */
            /* -- EMIT THE RESOURCE COMMANDS ------------------------------- */
            /* ------------------------------------------------------------- */

            m_enum.Write(
                [[inline]]


                    //--------------------------------------------------------------------------
                    //
                    //  MIL resources
                    //
                    //--------------------------------------------------------------------------

                [[/inline]]
                );

            foreach (PaddedCommand command in paddedCommands.PaddedCommands)
            {
                if (command.Origin != PaddedCommandOrigin.Resource)
                {
                    //
                    // Finally, emit the resource creation commands. These commands
                    // are created implicitly from the McgResource objects...
                    //

                    continue;
                }

                EmitCommand(command, commandIndex++);
            }


            //
            // Write Avalon command types
            //

            Helpers.Style.WriteFileHeader(enumFile);

            enumFile.WriteBlock(
                [[inline]]
                    typedef enum
                    {                                   
                        /* 0x00 */ [[PadWithSpaces("MilCmdInvalid", 45)]] = 0x00,

                        [[m_enum.ToString()]]
                        
                    #if DBG
                        //
                        // This command should always remain at the end of the list. It is
                        // not actually a command - rather it is used to validate the internal
                        // structure mapping to the enum.
                        //
                        // NOTE: if you put anything after this, you have broken the debugger
                        // extension. Also, there will be a mismatch of enum IDs between
                        // debug/retail and managed/unmanaged code.
                        //
                                                                 
                        /* 0x[[commandIndex.ToString("x02")]] */ [[PadWithSpaces("MilCmdValidateStructureOrder", 45)]] = 0x[[commandIndex.ToString("x02")]]
                    #endif
                    } MILCMD;

                [[/inline]]
                );

            enumFile.WriteBlock(
                m_milTypes.ToString()
                );


            enumCsFile.WriteBlock(
                [[inline]]
                    [[Helpers.ManagedStyle.WriteFileHeader("wgx_command_types.cs")]]

                    internal enum MILCMD
                    {                                   
                        /* 0x00 */ [[PadWithSpaces("MilCmdInvalid", 45)]] = 0x00,

                        [[m_enum.ToString()]]
                        
                    #if DBG
                        //
                        // This command should always remain at the end of the list. It is
                        // not actually a command - rather it is used to validate the internal
                        // structure mapping to the enum.
                        //
                        // NOTE: if you put anything after this, you have broken the debugger
                        // extension. Also, there will be a mismatch of enum IDs between
                        // debug/retail and managed/unmanaged code.
                        //
                                                                 
                        /* 0x[[commandIndex.ToString("x02")]] */ [[PadWithSpaces("MilCmdValidateStructureOrder", 45)]] = 0x[[commandIndex.ToString("x02")]]
                    #endif
                    };

                [[/inline]]
                );

            //
            // Serialize the command descriptors for debugger extension
            //

            Helpers.Style.WriteFileHeader(extsFile);
            extsFile.WriteBlock(
                [[inline]]
                    //   Debugger extensions should not have a table of types and associations
                    //  Type information be read dynamically from the symbol files.

                    //
                    // The API definition consists of a single array containing the size, name 
                    // and pointer to specific entry array produced above. This is for code 
                    // which needs to dispatch based on the command enum and needs to discover 
                    // the type information for the given command. It can loop the size and 
                    // process everything.
                    //

                    typedef struct
                    {
                        CHAR name[100];
                        CHAR type[100];
                        bool fHasPayload;
                        bool fTypePropertiesRead;
                        ULONG64 TypeModule;
                        ULONG TypeId;
                        ULONG size;
                    } MILCOMMAND;

                    static MILCOMMAND MarshalCommands[] =
                    {
                        /* 0x00 */ { NULL }, // MilCmdInvalid
                        [[m_exts.ToString()]]
                    };

                [[/inline]]
                );


            enumFile.Dispose();
            enumCsFile.Dispose();
            extsFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private string RedirectionTypeAsHex(uint value)
        {
            return "0x" + (value | 0x80000000).ToString("x08");
        }

        private string DwmTypeAsHex(uint value)
        {
            return "0x" + (value | 0x40000000).ToString("x08");
        }

        private string MilTypeAsHex(uint value)
        {
            return "0x" + value.ToString("x02");
        }

        private string PadWithSpaces(string str, int spaces)
        {
            Debug.Assert(str.Length <= spaces);

            StringBuilder sb = new StringBuilder(spaces);

            sb.Append(str);
            sb.Append(' ', Math.Max(spaces - str.Length, 0));

            return sb.ToString();
        }

        private string UnmanagedTypeHeader(
            string strTypeType,
            McgType type,
            string strSynopsis
            )
        {
            return
                [[inline]]
                    //+--------------------------------------------------------------------
                    //
                    //  [[strTypeType]]:
                    [[Helpers.CodeGenHelpers.FormatComment(type.UnmanagedDataType, 80, String.Empty, String.Empty, false /* unmanaged */)]]
                    //
                    //  Synopsis:
                    [[Helpers.CodeGenHelpers.FormatComment(strSynopsis, 80, String.Empty, String.Empty, false /* unmanaged */)]]
                    //
                    //---------------------------------------------------------------------
                [[/inline]]
                ;
        }

        private string BeginEnum(string strName, bool flags)
        {
            return "BEGIN_MIL" + (flags ? "FLAG" : "") + "ENUM( "
                + strName.Replace(flags ? "::Flags" : "::Enum", "")
                + " )";
        }

        private string EndEnum(string strName, bool flags)
        {
            return flags ? "END_MILFLAGENUM" : "END_MILENUM";
        }


        private void EmitRedirectionCommand(PaddedCommand command, uint value)
        {
            //
            // Emit a part of the REDIRECTIONCMD enumeration...
            //

            m_redirectionEnum.Write(
                [[inline]]                                                       
                    /* [[RedirectionTypeAsHex(value)]] */ [[PadWithSpaces(command.TypeName, 45)]] = MakeRedirectionCmdId(0x[[value.ToString("x02")]]),
                [[/inline]]
                );
        }

        private void EmitRedirectionEnum(McgEnum enumType)
        {
            //
            // Emit a REDIRECTION enum...
            //

            EmitEnum(m_redirectionTypes, enumType);
        }

        private void EmitMilEnum(McgEnum enumType)
        {
            //
            // Emit a MIL enum...
            //

            EmitEnum(m_milTypes, enumType);
        }

        private void EmitEnum(StringCodeSink strSink, McgEnum enumType)
        {
            //
            // Emit an enum...
            //

            // Write enum type definition
            strSink.Write(
                [[inline]]
                    [[UnmanagedTypeHeader("Enumeration", enumType, enumType.Comment)]]
                    [[BeginEnum(enumType.UnmanagedDataType, enumType.Flags)]]

                [[/inline]]
                );

            //
            // Compute longest value name for pretty alignment
            int LongestValueName = 0;

            foreach(McgEnumValue enumValue in enumType.AllValues)
            {
                LongestValueName = Math.Max(enumValue.Name.Length, LongestValueName);
            }

            //
            // Generate enum values
            //                    

            foreach(McgEnumValue enumValue in enumType.AllValues)
            {
                if (!string.IsNullOrEmpty(enumValue.Comment))
                {
                    strSink.Write(
                        [[inline]]
                                //
                                [[Helpers.CodeGenHelpers.FormatComment(enumValue.Name + " - " + enumValue.Comment, 80, String.Empty, String.Empty, false /* unmanaged */)]]
                                //
                        [[/inline]]
                    );
                }

                strSink.WriteBlock(
                    "    " + PadWithSpaces(enumValue.Name, LongestValueName) + " = " + enumValue.Value + ",\n"
                );
            }

            // Write closing brackets for namespace
            strSink.WriteBlock(
                EndEnum(enumType.UnmanagedDataType, enumType.Flags)
                );

        }

        private void EmitDwmCommand(PaddedCommand command, uint value)
        {
            //
            // Emit a part of the DWMCMD enumeration...
            //

            m_dwmEnum.Write(
                [[inline]]
                    /* [[DwmTypeAsHex(value)]] */ [[PadWithSpaces(command.TypeName, 55)]] = MakeDwmCmdId(0x[[value.ToString("x02")]]),
                [[/inline]]
                );
        }

        private void EmitCommand(PaddedCommand command, uint value)
        {
            //
            // Emit a part of the MILCMD enumeration...
            //

            m_enum.Write(
                [[inline]]
                    /* [[MilTypeAsHex(value)]] */ [[PadWithSpaces(command.TypeName, 45)]] = 0x[[value.ToString("x02")]],
                [[/inline]]
                );

            //
            // Emit the declaration for the debugger extension...
            //

            m_exts.Write(
                [[inline]]
                    /* 0x[[value.ToString("x02")]] */ { "[[command.CommandName]]", "milcore![[command.CommandName]]", [[command.HasPayload ? "true" : "false"]], false, 0, 0, 0 }, // [[command.TypeName]]
                [[/inline]]
                );
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private StringCodeSink m_redirectionEnum;
        private StringCodeSink m_redirectionTypes;
        private StringCodeSink m_dwmEnum;
        private StringCodeSink m_enum;
        private StringCodeSink m_milTypes;
        private StringCodeSink m_exts;

        #endregion Private Fields
    }
}



