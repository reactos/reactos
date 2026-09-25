// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      CommandStructure.cs
//
// Abstract: 
//      Generator for command structure files.
// 
// 
// Description:
//      This generator builds the src\Graphics\Include\Generated\wgx_commands.h and 
//      src\Graphics\Include\Generated\wgx_commands.cs files. They contain the C++ 
//      and C# structures describing the MILCE protocol commands.
// 
// 
// Example output fragment for the src\Graphics\Include\Generated\wgx_commands.h file:
//
//      typedef struct
//      {
//          MILCMD Type;
//          HMIL_RESOURCE Handle;
//          HMIL_RESOURCE hChild;
//          UINT index;
//      } MILCMD_NODE_INSERTCHILDAT;
// 
// 
// Example output fragment for the src\Graphics\Include\Generated\wgx_commands.cs file:
// 
//      internal struct MILCMD_NODE_INSERTCHILDAT
//      {
//      [FieldOffset(0)] internal MILCMD Type;
//      [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
//      [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
//      [FieldOffset(12)] internal UInt32 index;
//      };
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

    public class CommandStructure : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public CommandStructure(ResourceModel rm) : base(rm) 
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
            string generatedPath = 
                Path.Combine(
                    _resourceModel.OutputDirectory,
                    "src\\Graphics\\Include\\Generated"
                    );

            FileCodeSink cppFile = new FileCodeSink(generatedPath, "wgx_commands.h");;
            FileCodeSink csFile = new FileCodeSink(generatedPath, "wgx_commands.cs");

            m_redirection = new StringCodeSink();
            m_dwm = new StringCodeSink();
            m_cpp = new StringCodeSink();
            m_cs = new StringCodeSink();


            //
            // Collect the commands from the resource model and start generation.
            //

            PaddedCommandCollection commands =
                new PaddedCommandCollection(_resourceModel);            

            // Contains list of Commands which contain security critical resources
            List<String> commandList = new List<String>();

            foreach (PaddedCommand command in commands.PaddedCommands)
            {
                if (command.Domain == "Redirection") 
                {
                    WriteRedirectionCommand(command);
                }
                else if (command.Domain == "DWM") 
                {
                    WriteDwmCommand(command);
                }
                else if (command.Origin != PaddedCommandOrigin.RenderDataInstruction) 
                {
                    //
                    // Skip the render data instruction, they are emitted by
                    // the RenderData.cs generator...
                    //

                    WriteCommand(command);

                    // IsSecurityCritical does not make sense if the command is only for
                    // unmanaged code since its an attribute in managed code
                    if (command.CommandIsSecurityCritical && !command.CommandIsUnmanagedOnly)
                    {
                        commandList.Add(command.TypeName);
                    }
                }
            }

            // Parses the commands (which are securityCritical) to make switch-case statement.
            StringBuilder switchCaseBlock = MakeSwitchCaseStatement(commandList);

            //
            // Serialize the C++ header and the C# files for the Avalon commands:
            //

            Helpers.Style.WriteFileHeader(cppFile);
            cppFile.WriteBlock(m_cpp.ToString());

            csFile.WriteBlock(
                Helpers.ManagedStyle.WriteFileHeader("wgx_commands.cs"));

            csFile.WriteBlock(
                [[inline]]
                    // This code is generated from mcg\generators\CommandStructure.cs

                    using System;
                    using System.Windows.Media.Composition;
                    using System.Runtime.InteropServices;
                    using System.Windows.Media.Effects;
                    using System.Security;

                    using BOOL = System.UInt32;

                    namespace System.Windows.Media.Composition
                    {
                        internal partial class DUCE
                        {
                            [[m_cs.ToString()]]
                        };
                    }
                [[/inline]]
                );

            cppFile.Dispose();
            csFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private void WriteRedirectionCommand(PaddedCommand command)
        {
            WriteRedirectionOrDwmCommand(command, m_redirection);
        }

        private void WriteDwmCommand(PaddedCommand command)
        {
            WriteRedirectionOrDwmCommand(command, m_dwm);
        }

        private void WriteRedirectionOrDwmCommand(
            PaddedCommand command, 
            StringCodeSink sink
            )
        {
            //
            // Generate the unmanaged structure description
            //

            Helpers.CodeGenHelpers.PaddedStructData paddedStruct = 
                command.PaddedStructData; // padded and aligned structure

            DelimitedList commandList = new DelimitedList("", DelimiterPosition.AfterItem);

            Helpers.CodeGenHelpers.AppendStructParameters(
                commandList, 
                paddedStruct, 
                /* initialPosition */ 0,
                /* unmanaged = */ true,
                /* kernel accessible = */ true
                );

            sink.Write(
                [[inline]]
                    typedef struct 
                    {
                        [[commandList]]
                    } [[command.CommandName]];

                [[/inline]]
                );
        }


        /// <summary>
        /// Emits the definition of an Avalon MILCE command.
        /// </summary>
        private void WriteCommand(PaddedCommand command)
        {
            string commandName = command.CommandName; // MILCMD_[DOMAIN_AND_COMMAND_NAME]       

            Helpers.CodeGenHelpers.PaddedStructData paddedStruct = 
                command.PaddedStructData; // padded and aligned structure


            //
            // Generate the unmanaged structure description
            //

            DelimitedList commandList = new DelimitedList("", DelimiterPosition.AfterItem);

            Helpers.CodeGenHelpers.AppendStructParameters(
                commandList, 
                paddedStruct, 
                /* initialPosition */ 0,
                true);


            /* ------------------------------------------------------------- */
            /* -- EMIT THE UNMANAGED STRUCTURE ----------------------------- */
            /* ------------------------------------------------------------- */

            //
            // Render data commands do not bear the type field on the managed side,
            // the easiest workaround is to remove it from the definition and manually
            // add it here. In the long term, the render data commands should be
            // generated using a different mechanism as they are not marshaled directly.
            //

            if (command.Domain == "RenderData") 
            {
                m_cpp.Write(
                    [[inline]]
                        typedef struct
                        {
                            MILCMD Type;
                            [[commandList]]
                        } [[commandName]];

                    [[/inline]]
                    );
            }
            else
            {
                m_cpp.Write(
                    [[inline]]
                        typedef struct
                        {
                            [[commandList]]
                        } [[commandName]];
    
                    [[/inline]]
                    );
            }


            /* ------------------------------------------------------------- */
            /* -- EMIT THE MANAGED STRUCTURE ------------------------------- */
            /* ------------------------------------------------------------- */

            if (!command.CommandIsUnmanagedOnly)
            {
                commandList = new DelimitedList(";", DelimiterPosition.AfterItem);

                int lastPosition = 
                    Helpers.CodeGenHelpers.AppendStructParameters(
                        commandList, 
                        paddedStruct, 
                        /* initialPosition */ 0,
                        false);

                m_cs.Write(
                    [[inline]]
                        [StructLayout(LayoutKind.Explicit, Pack=1)]
                        internal struct [[commandName]]
                        {
                        [[commandList]];
                        };
                    [[/inline]]
                );                 
            } 
        }


        /// <summary>
        /// It takes a list of commands and makes one string block out of it
        /// Eg:  Input: "MilCmdHwndTargetCreat", "MilCmdGenericTargetCreate",...
        ///      Output:
        ///            "case MILCMD.MilCmdHwndTargetCreate:
        ///             case MILCMD.MilCmdGenericTargetCreate:
        ///             ...
        ///                 return true;"
        /// </summary>
        private StringBuilder MakeSwitchCaseStatement(List<String> s)
        {
            StringBuilder toFilter = new StringBuilder("");
            foreach (String cases in s)
            {
                toFilter.Append("case MILCMD." + cases + ":\n");
            }
            toFilter.Append("\treturn true;");

            return toFilter;
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private StringCodeSink m_redirection;
        private StringCodeSink m_dwm;
        private StringCodeSink m_cpp;
        private StringCodeSink m_cs;

        #endregion Private Fields
     }
}


