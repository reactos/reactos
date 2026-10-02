// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      CommandProcessMessage.cs
//
// Abstract: 
//      Generator for CComposition's message processing routine.
// 
// 
// Description:
//      This generator builds code for extracting MILCE protocol commands
//      from command batches and for validating the contents of the command
//      structures.
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

    public class CommandProcessMessage : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public CommandProcessMessage(ResourceModel rm) : base(rm) 
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
                    "src\\Graphics\\core\\uce"
                    );

            FileCodeSink processMessageFile = new FileCodeSink(generatedPath, "generated_process_message.inl");

            m_processMessage = new StringCodeSink();


            //
            // Walk the padded commands and build the message processing routine:
            //

            PaddedCommandCollection commands =
                new PaddedCommandCollection(_resourceModel);

            foreach (PaddedCommand command in commands.PaddedCommands)
            {
                if (command.Origin == PaddedCommandOrigin.RenderDataInstruction
                    || command.Domain == "DWM" 
                    || command.Domain == "Redirection" 
                    || command.Domain == "RenderData" 
                    || (command.Domain == "Transport" && command.Target == "Ts")
                    || (command.Target == String.Empty && command.Name == "GlyphBitmap")
                    || command.Name == "GradientStop")
                {
                    continue;
                }

                WriteCommandHandler(command);
            }


            //
            // Serialize the resulting routine
            //
            
            Helpers.Style.WriteFileHeader(processMessageFile);

            processMessageFile.WriteBlock(
                [[inline]]
                    switch(nCmdType)
                    {
                        [[m_processMessage.ToString()]]
    
                        default:
                            RIP("Invalid command type.");
                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                            break;
                    }
                [[/inline]]
                );

            processMessageFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        /// <summary>
        /// Builds command size inspection and unpacking code.
        /// </summary>
        private StringCodeSink BuildSizeInspector(PaddedCommand command)
        {
            StringCodeSink sizeInspector = new StringCodeSink();

            sizeInspector.WriteBlock(
                [[inline]]
                    #ifdef DEBUG
                    if (cbSize [[command.HasPayload ? "<" : "!="]] sizeof([[command.CommandName]]))
                    {
                        IFC(WGXERR_UCE_MALFORMEDPACKET);
                    }
                    #endif

                    const [[command.CommandName]]* pCmd = 
                        reinterpret_cast<const [[command.CommandName]]*>(pcvData);
                [[/inline]]
                );


            //
            // The command payloads are unstructured from the transport point
            // of view. The only thing we can do for now is to calculate the
            // payload size and pass it to specialized command processing method.
            //

            if (command.HasPayload) 
            {
                sizeInspector.WriteBlock(
                    [[inline]]
                        LPCVOID pPayload = reinterpret_cast<LPCVOID>(pCmd + 1);
                        UINT cbPayload = cbSize - sizeof([[command.CommandName]]);
                    [[/inline]]
                    );
            }

            return sizeInspector;
        }

        private StringCodeSink BuildValidator(PaddedCommand command)
        {
            StringCodeSink validator = new StringCodeSink();

            foreach (CodeGenHelpers.AlignmentEntry entry in command.PaddedStructData.AlignmentEntries) 
            {
                McgField field = entry.Field;
                McgResource resource = field.Type as McgResource;

                if (resource == null || resource.IsValueType || resource.IsCollection) 
                {
                    continue;
                }

                validator.WriteBlock(
                    [[inline]]
                        #ifdef DEBUG
                        if (pCmd->[[entry.Name]] != NULL) 
                        {
                            const [[resource.DuceClass]]* pResource = 
                                static_cast<const [[resource.DuceClass]]*>(pHandleTable->GetResource(
                                    pCmd->[[entry.Name]], 
                                    TYPE_[[resource.Name.ToUpper()]]
                                    ));

                            if (pResource == NULL) 
                            {
                                RIP("Invalid resource handle (expected a [[resource.DuceClass]]).");
                                IFC(WGXERR_UCE_MALFORMEDPACKET);
                            }
                        }
                        #endif
                    [[/inline]]
                    );
            }

            return validator;
        }


        /// <summary>
        /// Builds the code to forward a command to its receiver. We generally
        /// have two cases here: resource commands and general commands. For the
        /// resource commands, we need to infer the type of the resource we expect
        /// to be receiving and look it up accordingly. The general commands will
        /// have to have methods on CComposition to be processed.
        /// </summary>
        private StringCodeSink BuildRouter(PaddedCommand command)
        {
            StringCodeSink router = new StringCodeSink();

            //
            // Block commands not meant for cross thread 
            // and cross process transport.
            //

            string onTransportDenied;

            if (command.FailIfCommandTransportDenied) 
            {
                onTransportDenied = 
                    [[inline]]
                        #if DEBUG 
                        // Fail processing of this command and close the connection for current marshal type.
                        RIP("[[command.CommandName]] command is not meant for the current marshal type.");
                        IFC(WGXERR_UCE_COMMANDTRANSPORTDENIED);
                        #endif
                    [[/inline]];                        
            }
            else
            {
                onTransportDenied = 
                    [[inline]]
                        // Ignore this command -- it is effectively a no-op for current marshal type.
                        break;
                    [[/inline]];                        
            }

            //
            // Some commands require special handling before being routed
            // or are not targeted at resources. CComposition will provide
            // special handlers for such commands, emit appropriate calls here:
            //

            if (command.Domain == "Transport" 
                || command.Target == "Partition"
                || command.Target == "Channel"
                || command.Target == "Sprite"
                || (command.Target == "GlyphRun" && command.Name == "Create")
                || (command.Target == "Target" && command.Name == "CaptureBits")
                || (command.Target == "HwndTarget" && command.Name == "Create")
                || (command.Target == "GenericTarget" && command.Name == "Create"))
            {
                // 
                // Pretty prints special handler name.
                //
                
                string handlerName =
                    (command.Domain == "Transport" ? command.Domain : command.Target) 
                    + "_" + command.Name; // Transport_[Name] or [Target]_[Name]


                //
                // Emit call to the special handler method.
                //

                router.WriteBlock(
                    [[inline]]
                        //
                        // This command needs a special handler. We will call a special case 
                        // CComposition method instead of routing the command to a resource.
                        //
                    [[/inline]]
                    );

                if (command.HasPayload) 
                {
                    router.WriteBlock(
                        [[inline]]
                            IFC([[handlerName]](pChannel, pHandleTable, pCmd, pPayload, cbPayload));
                        [[/inline]]
                        );
                }
                else
                {
                    router.WriteBlock(
                        [[inline]]
                            IFC([[handlerName]](pChannel, pHandleTable, pCmd));
                        [[/inline]]
                        );
                }
            }
            else
            {
                //
                // For ordinary commands, create code to retrieve and type check 
                // the target resource and then emit call to appropriate command handler.
                //

                router.WriteBlock(
                    [[inline]]
                        [[command.TargetResourceName]]* pResource =
                            static_cast<[[command.TargetResourceName]]*>(pHandleTable->GetResource(
                                pCmd->Handle,
                                [[command.TargetResourceType]]
                                ));

                        if (pResource == NULL)
                        {
                            RIP("Invalid resource handle.");
                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                        }

                    [[/inline]]
                    );

                if (command.HasPayload) 
                {
                    router.WriteBlock(
                        [[inline]]
                            IFC(pResource->Process[[command.Name]](pHandleTable, pCmd, pPayload, cbPayload));
                        [[/inline]]
                        );
                }
                else
                {
                    router.WriteBlock(
                        [[inline]]
                            IFC(pResource->Process[[command.Name]](pHandleTable, pCmd));
                        [[/inline]]
                        );
                }
            }

            return router;
        }

        /// <summary>
        /// Given a MILCE protocol command, this method generates inspection
        /// code that verifies the command size and contents and returns 
        /// a typed structure.
        /// </summary>
        private void WriteCommandHandler(PaddedCommand command)
        {
            StringCodeSink sizeInspector = BuildSizeInspector(command);
            StringCodeSink router = BuildRouter(command);

            //
            // For now we only validate the resource handles. We can get
            // away with not validating them here -- exactly the same
            // validation is performed in ProcessUpdate methods generated
            // for the marshaled resources.
            //

            bool commandNeedsValidation =
                command.Origin != PaddedCommandOrigin.Resource;

            StringCodeSink validator = 
                commandNeedsValidation 
                ? BuildValidator(command)
                : new StringCodeSink();

            //
            // Write the command handler. Validator is optional.
            //

            m_processMessage.WriteBlock(
                [[inline]]
                    case [[command.TypeName]]:
                    {
                        [[sizeInspector.ToString()]]
                [[/inline]]
                );

            if (!validator.IsEmpty) 
            {
                m_processMessage.WriteBlock(
                    [[inline]]
                            [[validator.ToString()]]
                    [[/inline]]
                    );
            }
            else if (!commandNeedsValidation) 
            {
                m_processMessage.WriteBlock(
                    [[inline]]
                            /* Resource handles are validated in the ProcessUpdate method. */
                    [[/inline]]
                    );
            }

            m_processMessage.WriteBlock(
                [[inline]]
                        [[router.ToString()]]
                    }
                    break;
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

        private StringCodeSink m_processMessage;

        #endregion Private Fields
    }
}


