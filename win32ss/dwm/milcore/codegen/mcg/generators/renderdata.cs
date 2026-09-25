// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate files for render data.
//

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class RenderData : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public RenderData(ResourceModel rm) : base(rm) {}

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        public override void Go()
        {
            RenderDataInstructionData rdid = _resourceModel.RenderDataInstructionData;

            if (rdid == null)
            {
                return;
            }

            string fullManagedPath = Path.Combine(_resourceModel.OutputDirectory, rdid.ManagedDestinationDir);

            // The base class for DrawingContext - contains abstract methods for each
            // renderdata instruction.
            string drawingContextFileName = "DrawingContext.cs";

            // The derived DrawingContext class which produces renderdata.
            string renderDataDrawingContextFileName = "RenderDataDrawingContext.cs";

            string drawingContextWalkerFileName = "DrawingContextWalker.cs";

            string drawingContextDrawingContextWalkerFileName = "DrawingContextDrawingContextWalker.cs";

            // The class which encapsulates the instruction stream and dependents.
            string renderDataManagedFileName = "RenderData.cs";

            string fullExportPath = Path.Combine(_resourceModel.OutputDirectory, rdid.ExportDestinationDir);
            string incFileName = "wgx_renderdata_commands.h";

            // This file is used for unmanaged renderdata resource code.
            string unmanagedRenderDataFileName = "renderdata_generated.cpp";

            string fullUnmanagedPath = Path.Combine(_resourceModel.OutputDirectory, rdid.NativeDestinationDir);

            // This method emits the abstract base class "DrawingContext"
            DoDrawingContext(rdid, fullManagedPath, drawingContextFileName);

            // This method emits the managed render data drawing context, which emits
            // the code to build the render data from managed code
            DoRenderDataDrawingContext(rdid, fullManagedPath, renderDataDrawingContextFileName);

            DoDrawingContextWalker(rdid, fullManagedPath, drawingContextWalkerFileName);

            DoDrawingContextDrawingContextWalker(rdid, fullManagedPath, drawingContextDrawingContextWalkerFileName);

            // Emit the managed class which contains a blob for the actual inline
            // renderdata instructions
            DoRenderDataManaged(rdid, fullManagedPath, renderDataManagedFileName);

            DoIncFile(rdid,
                      fullExportPath,
                      incFileName);

            DoRenderDataUnmanaged(
                rdid,
                fullUnmanagedPath,
                unmanagedRenderDataFileName);
        }

        private void DoDrawingContext(
            RenderDataInstructionData rdid,
            string fullManagedPath,
            string drawingContextFileName)
        {
            // We will create both the abstract base class and the RenderDataDrawingContext class.
            using (FileCodeSink dcFile = BeginDrawingContextFile(rdid, fullManagedPath, drawingContextFileName))
            {
                dcFile.WriteBlock(
                    [[inline]]

                        namespace [[rdid.ManagedNamespace]]
                        {
                            /// <summary>
                            ///     DrawingContext
                            /// </summary>
                            public abstract partial class DrawingContext : DispatcherObject, IDisposable
                            {
                                [[WriteDrawingMethods(rdid, new WriteDrawingBodyDelegate(WriteDrawingContextBody), "abstract", null)]]
                            }
                        }
                    [[/inline]]
                    );
            }
       }

        /// <summary>
        /// DoDrawingContextWalker - this method generates the contents of the DrawingContextWalker class
        /// </summary>
        /// <param name="rdid"> RenderDataInstructionData - the collection of renderdata instructions. </param>
        /// <param name="fullManagedPath"> The path for the file to emit. </param>
        /// <param name="renderDataDrawingContextFileName"> The filename to emit. </param>
        private void DoDrawingContextWalker(
              RenderDataInstructionData rdid,
              string fullManagedPath,
              string drawingContextWalkerFileName)
        {
            // Create the file code sink for the file.
            using (FileCodeSink dcwFile = BeginDrawingContextFile(rdid, fullManagedPath, drawingContextWalkerFileName))
            {
                dcwFile.WriteBlock(
                    [[inline]]

                        namespace [[rdid.ManagedNamespace]]
                        {
                            /// <summary>
                            /// DrawingContextWalker : The base class for DrawingContext iterators.
                            /// This is *not* thread safe
                            /// </summary>
                            internal abstract partial class DrawingContextWalker : DrawingContext
                            {
                                [[WriteDrawingMethods(rdid, new WriteDrawingBodyDelegate(WriteDrawingContextWalkerBody), "override", null)]]
                            }
                        }
                    [[/inline]]
                    );
            }
        }

        /// <summary>
        /// DoDrawingContextDrawingContextWalker - this method generates the contents of the
        /// DrawingContextDrawingContextWalker class
        /// </summary>
        /// <param name="rdid"> RenderDataInstructionData - the collection of renderdata instructions. </param>
        /// <param name="fullManagedPath"> The path for the file to emit. </param>
        /// <param name="renderDataDrawingContextFileName"> The filename to emit. </param>
        private void DoDrawingContextDrawingContextWalker(
              RenderDataInstructionData rdid,
              string fullManagedPath,
              string drawingContextWalkerFileName)
        {
            // Create the file code sink for the file.
            using (FileCodeSink dcwFile = BeginDrawingContextFile(rdid, fullManagedPath, drawingContextWalkerFileName))
            {
                dcwFile.WriteBlock(
                    [[inline]]

                        namespace [[rdid.ManagedNamespace]]
                        {
                            /// <summary>
                            /// DrawingContextDrawingContextWalker is a DrawingContextWalker
                            /// that forwards all of it's calls to a DrawingContext.
                            /// </summary>
                            internal partial class DrawingContextDrawingContextWalker: DrawingContextWalker
                            {
                                [[WriteDrawingMethods(rdid, new WriteDrawingBodyDelegate(WriteDrawingContextDrawingContextBody), "override", null)]]
                            }
                        }
                    [[/inline]]
                    );
            }
        }

        /// <summary>
        /// DoRenderDataDrawingContext - this method generates the contents of
        /// the renderdata DrawingContext.
        /// </summary>
        /// <param name="rdid"> RenderDataInstructionData - the collection of renderdata instructions. </param>
        /// <param name="fullManagedPath"> The path for the file to emit. </param>
        /// <param name="renderDataDrawingContextFileName"> The filename to emit. </param>
        private void DoRenderDataDrawingContext(
              RenderDataInstructionData rdid,
              string fullManagedPath,
              string renderDataDrawingContextFileName)
        {
            // Create the file code sink for the file.
            using (FileCodeSink rdFile = BeginDrawingContextFile(rdid, fullManagedPath, renderDataDrawingContextFileName))
            {
                StringCodeSink securityCriticalCS = new StringCodeSink();

                securityCriticalCS.Write(
                    [[inline]]
                        /// <SecurityNote>
                        ///    Critical:This code calls into unsafe code
                        ///    TreatAsSafe: This code is ok to expose. Writing a record is a safe operation as long as the size and pointer are valid.
                        /// </SecurityNote>
                        [SecurityCritical,SecurityTreatAsSafe][[/inline]]
                    );

                rdFile.WriteBlock(
                    [[inline]]

                        namespace [[rdid.ManagedNamespace]]
                        {
                            /// <summary>
                            ///     RenderDataDrawingContext - A DrawingContext which produces a Drawing.
                            /// </summary>
                            internal partial class RenderDataDrawingContext : DrawingContext
                            {
                                [[WriteDrawingMethods(rdid, new WriteDrawingBodyDelegate(WriteRenderDataDrawingContextBody), "override", securityCriticalCS.ToString())]]
                                [[WriteUseAnimationMethods()]]
                            }
                        }
                    [[/inline]]
                    );
            }
        }

        /// <summary>
        /// BeginDrawingContextFile - Consolidates common DrawingContext file creation
        /// logic of creating a FileCodeSink & generating the file header and namespaces
        /// </summary>
        private FileCodeSink BeginDrawingContextFile(
              RenderDataInstructionData rdid,
              string fullManagedPath,
              string drawingContextFileName)
        {
            // Create the codesink & write the file header
            FileCodeSink fileCS = new FileCodeSink(
                fullManagedPath,
                drawingContextFileName,
                true /* Create dir if necessary */
                );

            fileCS.WriteBlock(
                [[inline]]
                    [[Helpers.ManagedStyle.WriteFileHeader(drawingContextFileName)]]
                [[/inline]]
                );

            // Emit the using statements
            foreach (string s in rdid.Namespaces)
            {
                fileCS.Write(
                    [[inline]]
                        using [[s]];
                    [[/inline]]
                    );
            }

            return fileCS;
        }

        /// <summary>
        /// WriteDrawingMethods -- Contains the logic common to generating
        /// all DrawingContext derivatives.  This includes iterating through
        /// each instruction, writing out the instructions prototype, and
        /// writing the advanced version of the method when neccessary.  It
        /// calls to the WriteDrawingBody delegate to write the body of each
        /// Drawing method.
        /// </summary>
        private string WriteDrawingMethods(RenderDataInstructionData rdid, WriteDrawingBodyDelegate writeDrawingBodyDelegate, string virtualModifier, string attributes)
        {
            StringCodeSink cs = new StringCodeSink();

            //
            // For each instruction, output the drawing methods
            //
            foreach (McgRenderDataInstruction renderdataInstruction in rdid.RenderDataInstructions)
            {
                DelimitedList modifierList = new DelimitedList(" ", DelimiterPosition.AfterItem, /* insertNewline = */ false);
                Helpers.CodeGenHelpers.AddFlagsToList<Modifier>(modifierList, renderdataInstruction.Modifier);
                string modifier = modifierList.ToString() + " " + virtualModifier;

                //
                //
                // Output the basic Drawing methods which do not contain AnimationClock or
                // non-basic parameters
                //
                //

                cs.Write(WriteDrawingPrototype(renderdataInstruction, modifier, attributes, true /* skip advanced params */));
                cs.Write(writeDrawingBodyDelegate(renderdataInstruction, true /* skip advanced params */));

                //
                //
                // Output the advanced Drawing methods which contain both AnimationClock and
                // advanced parameters
                //
                //

                if (renderdataInstruction.HasAdvancedParameters)
                {

                    cs.Write(WriteDrawingPrototype(renderdataInstruction, modifier, attributes, false /* don't skip advanced parms */));
                    cs.Write(writeDrawingBodyDelegate(renderdataInstruction, false /* skip advanced params */));
                }

            }

            return cs.ToString();
        }

        /// <summary>
        /// WriteDrawingMethods -- Generates the prototype of a DrawingContext
        /// method in the form:
        ///
        /// <comments>
        /// <optional attributes>
        /// <modifiers> void <instructionName> (<parameters>)
        ///
        /// </summary>
        private string WriteDrawingPrototype(McgRenderDataInstruction rdi, string modifiers, string attributes, bool skipAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            Helpers.CodeGenHelpers.ParameterType paramType = Helpers.CodeGenHelpers.ParameterType.ManagedParamList;

            McgField[] parameters;
            if (skipAdvancedParameters)
            {
                parameters = rdi.BasicPublicFields;
                paramType |= Helpers.CodeGenHelpers.ParameterType.SkipAnimations;
            }
            else
            {
                parameters = rdi.AllPublicFields;
            }

            ParameterList paramList = new ParameterList();
            Helpers.CodeGenHelpers.AppendParameters(
                paramList,
                parameters,
                paramType
                );

            // Write comment first

            cs.Write(
                [[inline]]
                    [[WriteComment(rdi, skipAdvancedParameters, false)]]
                [[/inline]]
                );

            // Next, write any method attributes, if they exist
            if (null != attributes)
            {
                cs.WriteBlock(attributes);
            }

            // We have deprecated PushEffect, so place the warning on the method declaration.
            if (rdi.Name == "PushEffect")
            {
                cs.Write("[Obsolete(MS.Internal.Media.VisualTreeUtils.BitmapEffectObsoleteMessage)]\n");
            }

            // Finally, write the prototype
            cs.Write(
                [[inline]]
                    [[modifiers]] void [[rdi.Name]](
                        [[paramList]])[[/inline]]
                );

            return cs.ToString();
        }

        /// <summary>
        /// WriteDrawingBodyDelegate -- Definition of a delegate whose implementations
        /// write the body of a DrawingContext method.
        /// </summary>
        delegate string WriteDrawingBodyDelegate(McgRenderDataInstruction renderdataInstruction, bool skipAdvancedParameters);

        private static string WriteDrawingContextBody(McgRenderDataInstruction renderdataInstruction, bool skipAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            // The abstract DrawingContext methdods have no body
            cs.Write(
                [[inline]]
                    ;

                [[/inline]]
                    );

            return cs.ToString();
        }

        private static string WriteDrawingContextWalkerBody(McgRenderDataInstruction renderdataInstruction, bool skipAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            if (renderdataInstruction.Name == "DrawDrawing")
            {
                // Special case DrawDrawing to traverse the graph
                cs.Write(
                    [[inline]]

                        {
                            if (drawing != null)
                            {
                                drawing.WalkCurrentValue(this);
                            }
                        }

                    [[/inline]]
                    );
            }
            else
            {
                // The DrawingContextWalker methods provides a default implementation
                // that Assert's in DBG builds.
                cs.Write(
                    [[inline]]

                        {
                            Debug.Assert(false);
                        }

                    [[/inline]]
                    );
            }

            return cs.ToString();
        }

        private static string WriteRenderDataDrawingContextBody(McgRenderDataInstruction renderdataInstruction, bool skipAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            if (skipAdvancedParameters)
            {

                //
                //  Output the basic DrawingContext method
                //

                ParameterList callingList = new ParameterList();

                Helpers.CodeGenHelpers.AppendParameters(
                    callingList,
                    renderdataInstruction.BasicPublicFields,
                    Helpers.CodeGenHelpers.ParameterType.SkipAnimations |
                    Helpers.CodeGenHelpers.ParameterType.RenderDataCallParamList);

                // In the call to WriteDataRecord below, we generate the size for this packet
                // at codegen time for two reasons:
                // One, the size *must* be exactly what we calculate at codegen time
                // because it's a wire protocol and cannot vary on platforms/machines/etc
                // Two, because sizeof(struct) will return 1, not 0, if the struct is empty
                // even though it has no data.

                string sizeAssert = String.Empty;

                if (renderdataInstruction.GetPaddedSize(false /* no animations */) == 0)
                {
                    sizeAssert =
                        [[inline]]
                            // Note that since sizeof(emptyStruct) returns 1, we compare against 1 for empty structs.
                            Debug.Assert(sizeof([[renderdataInstruction.StructName]]) == 1);
                        [[/inline]];
                }
                else
                {
                    sizeAssert =
                        [[inline]]
                            Debug.Assert(sizeof([[renderdataInstruction.StructName]]) == [[renderdataInstruction.GetPaddedSize(false /* no animations */)]]);
                        [[/inline]];

                }

                if (renderdataInstruction.Name == "PushGuidelineSet")
                {
                cs.Write(
                    [[inline]]

                        {
                            VerifyApiNonstructuralChange();

                            [[WriteInitialChecks(renderdataInstruction, true /* managed */, false /* don't indent */)]]

                        #if DEBUG
                            MediaTrace.DrawingContextOp.Trace("[[renderdataInstruction.Name]](const)");
                        #endif

                            unsafe
                            {
                                EnsureRenderData();

                                if (guidelines != null && guidelines.IsFrozen && guidelines.IsDynamic)
                                {
                                    DoubleCollection guidelinesX = guidelines.GuidelinesX;
                                    DoubleCollection guidelinesY = guidelines.GuidelinesY;
                                    int countX = guidelinesX == null ? 0 : guidelinesX.Count;
                                    int countY = guidelinesY == null ? 0 : guidelinesY.Count;

                                    if (countX == 0 && (countY == 1 || countY == 2)
                                        )
                                    {
                                        if (countY == 1)
                                        {
                                            MILCMD_PUSH_GUIDELINE_Y1 record =
                                                new MILCMD_PUSH_GUIDELINE_Y1(
                                                    guidelinesY[0]
                                                    );

                                            _renderData.WriteDataRecord(
                                                MILCMD.MilPushGuidelineY1,
                                                (byte*)&record,
                                                sizeof(MILCMD_PUSH_GUIDELINE_Y1)
                                                );
                                        }
                                        else
                                        {
                                            MILCMD_PUSH_GUIDELINE_Y2 record =
                                                new MILCMD_PUSH_GUIDELINE_Y2(
                                                    guidelinesY[0],
                                                    guidelinesY[1] - guidelinesY[0]
                                                    );

                                            _renderData.WriteDataRecord(
                                                MILCMD.MilPushGuidelineY2,
                                                (byte*)&record,
                                                sizeof(MILCMD_PUSH_GUIDELINE_Y2)
                                                );
                                        }
                                    }
                                }
                                else
                                {
                                    [[WriteRealizationChecks(renderdataInstruction, renderdataInstruction.BasicPublicFields)]]

                                    [[renderdataInstruction.StructName]] record =
                                        new [[renderdataInstruction.StructName]] (
                                            [[callingList]]
                                            );

                                    // Assert that the calculated packet size is the same as the size returned by sizeof().
                                    [[sizeAssert]]

                                    _renderData.WriteDataRecord(MILCMD.Mil[[renderdataInstruction.Name]],
                                                                (byte*)&record,
                                                                [[renderdataInstruction.GetPaddedSize(false /* no animations */)]] /* sizeof([[renderdataInstruction.StructName]]) */);
                                }
                            }
                           
                            [[WriteStackOperation(renderdataInstruction, true)]]
                        }

                    [[/inline]]
                        );
                }
                else
                {
                cs.Write(
                    [[inline]]

                        {
                            VerifyApiNonstructuralChange();

                            [[WriteInitialChecks(renderdataInstruction, true /* managed */, false /* don't indent */)]]

                        #if DEBUG
                            MediaTrace.DrawingContextOp.Trace("[[renderdataInstruction.Name]](const)");
                        #endif

                            unsafe
                            {
                                EnsureRenderData();

                                [[WriteRealizationChecks(renderdataInstruction, renderdataInstruction.BasicPublicFields)]]

                                [[renderdataInstruction.StructName]] record =
                                    new [[renderdataInstruction.StructName]] (
                                        [[callingList]]
                                        );

                                // Assert that the calculated packet size is the same as the size returned by sizeof().
                                [[sizeAssert]]

                                _renderData.WriteDataRecord(MILCMD.Mil[[renderdataInstruction.Name]],
                                                            (byte*)&record,
                                                            [[renderdataInstruction.GetPaddedSize(false /* no animations */)]] /* sizeof([[renderdataInstruction.StructName]]) */);
                            }                           
                            
                            [[WriteStackOperation(renderdataInstruction, true)]]                            
                            [[WriteEffectStackOperation(renderdataInstruction)]]                                                                                  
                        }

                    [[/inline]]
                        );
                }
            }
            else
            {
                //
                // Output the advanced DrawingContext methods
                //
                McgField[] animatedFields = ResourceModel.Filter(renderdataInstruction.AllPublicFields, ResourceModel.IsAnimated);

                ParameterList callingListAnimate = new ParameterList();

                Helpers.CodeGenHelpers.AppendParameters(
                    callingListAnimate,
                    renderdataInstruction.AllPublicFields,
                    Helpers.CodeGenHelpers.ParameterType.RenderDataCallParamList);

                // In the call to WriteDataRecord below, we generate the size for this packet
                // at codegen time for two reasons:
                // One, the size *must* be exactly what we calculate at codegen time
                // because it's a wire protocol and cannot vary on platforms/machines/etc
                // Two, because sizeof(struct) will return 1, not 0, if the struct is empty
                // even though it has no data.

                string sizeAssert = String.Empty;

                if (renderdataInstruction.GetPaddedSize(true /* include animations */) == 0)
                {
                    sizeAssert =
                        [[inline]]
                            // Note that since sizeof(emptyStruct) returns 1, we compare against 1 for empty structs.
                            Debug.Assert(sizeof([[renderdataInstruction.StructName]]_ANIMATE) == 1);
                        [[/inline]];
                }
                else
                {
                    sizeAssert =
                        [[inline]]
                            Debug.Assert(sizeof([[renderdataInstruction.StructName]]_ANIMATE) == [[renderdataInstruction.GetPaddedSize(true /* include animations */)]]);
                        [[/inline]];

                }

                cs.Write(
                    [[inline]]

                        {
                            VerifyApiNonstructuralChange();

                            [[WriteInitialChecks(renderdataInstruction, true /* managed */, false /* dont' indent */)]]

                        #if DEBUG
                            MediaTrace.DrawingContextOp.Trace("[[renderdataInstruction.Name]](animate)");
                        #endif

                            unsafe
                            {
                                EnsureRenderData();

                                [[WriteRealizationChecks(renderdataInstruction, renderdataInstruction.AllPublicFields)]]

                                [[Helpers.CodeGenHelpers.WriteFieldStatements(animatedFields,
                                    DuceHandle.ManagedTypeName + " h{propertyName}Animations = " + DuceHandle.ManagedNullHandle + ";")]]
                                [[Helpers.CodeGenHelpers.WriteFieldStatements(animatedFields,
                                    "h{propertyName}Animations = UseAnimations({localName}, {localName}Animations);")]]

                                [[renderdataInstruction.StructName]]_ANIMATE record =
                                    new [[renderdataInstruction.StructName]]_ANIMATE (
                                        [[callingListAnimate]]
                                        );

                                // Assert that the calculated packet size is the same as the size returned by sizeof().
                                [[sizeAssert]]

                                _renderData.WriteDataRecord(MILCMD.Mil[[renderdataInstruction.Name]]Animate,
                                                            (byte*)&record,
                                                            [[renderdataInstruction.GetPaddedSize(true /* include animations */)]] /* sizeof([[renderdataInstruction.StructName]]_ANIMATE) */);
                            }                            
                            
                            [[WriteStackOperation(renderdataInstruction, true)]]
                            [[WriteEffectStackOperation(renderdataInstruction)]]                                                        
                        }
                    [[/inline]]
                    );
            }
            return cs.ToString();
        }


        private static string WriteDrawingContextDrawingContextBody(McgRenderDataInstruction renderdataInstruction, bool skipAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            if (skipAdvancedParameters)
            {
                //
                //  Output the basic DrawingContext method
                //

                ParameterList callingList = new ParameterList();

                Helpers.CodeGenHelpers.AppendParameters(
                    callingList,
                    renderdataInstruction.BasicPublicFields,
                    Helpers.CodeGenHelpers.ParameterType.SkipAnimations |
                    Helpers.CodeGenHelpers.ParameterType.ManagedCallParamList
                    );

                cs.Write(
                    [[inline]]

                        {
                            _drawingContext.[[renderdataInstruction.Name]](
                                [[callingList]]
                                );
                        }

                    [[/inline]]
                        );
            }
            else
            {
                ParameterList callingListAnimate = new ParameterList();

                Helpers.CodeGenHelpers.AppendParameters(
                    callingListAnimate,
                    renderdataInstruction.AllPublicFields,
                    Helpers.CodeGenHelpers.ParameterType.ManagedCallParamList
                    );

                cs.Write(
                    [[inline]]

                        {
                            _drawingContext.[[renderdataInstruction.Name]](
                                [[callingListAnimate]]
                                );
                        }
                    [[/inline]]
                        );

            }

            return cs.ToString();
        }

        private static string WriteRealizationChecks(
            McgRenderDataInstruction rdi,
            McgField[] parameters)
        {
            StringCodeSink realizationCheckCS = new StringCodeSink();

            McgField[] glyphRunFields = ResourceModel.Filter(
                parameters,
                ResourceModel.IsTypeNameGlyphRun
                );

            McgField[] drawingFields = ResourceModel.Filter(
                parameters,
                ResourceModel.IsTypeNameDrawing
                );

            // Check for glyph run.  If they exist, we always
            // set HasStaticContentRequiringRealizations to true, so we won't need to generate
            // any other realization checks.

            //
            // Generate checks for DrawingBrush & VisualBrush parameters that
            // cause realization updates to be needed
            //
            McgField[] penTypeFields = ResourceModel.Filter(
                parameters,
                ResourceModel.IsTypeNamePen
                );

            McgField[] brushTypeFields = ResourceModel.Filter(
                parameters,
                ResourceModel.IsTypeNameBrush
                );

            // First, write comment if there are in checks
            if (penTypeFields.Length > 0 || brushTypeFields.Length > 0)
            {
                realizationCheckCS.Write(
                    [[inline]]
                        // Always assume visual and drawing brushes need realization updates
                    [[/inline]]
                    );
            }

            return realizationCheckCS.ToString();
        }

        /// <summary>
        /// WriteUseAnimationMethods - this emits the UseAnimations methods.
        /// </summary>
        private string WriteUseAnimationMethods()
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (McgResource resource in _resourceModel.Resources)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.AnimationResource, resource))
                {
                    continue;
                }

                cs.WriteBlock(
                    [[inline]]
                        private UInt32 UseAnimations(
                            [[resource.Name]] baseValue,
                            AnimationClock animations)
                        {
                            if (animations == null)
                            {
                                return 0;
                            }
                            else
                            {
                                return _renderData.AddDependentResource(
                                    new [[resource.Name]]AnimationClockResource(
                                        baseValue,
                                        animations));
                            }
                        }
                    [[/inline]]
                    );
            }

            return cs.ToString();
        }

        /// <summary>
        /// WriteDUCEMarshalInstructions - write the case statements for a switch block which
        /// marshals to the DUCE
        /// </summary>
        /// <returns>
        /// string - the contents of the switch block
        /// </returns>
        /// <param name="rdid"> RenderDataInstructionData - the collection of instructions to emit. </param>
        public string WriteDUCEMarshalInstructions(RenderDataInstructionData rdid)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (McgRenderDataInstruction instruction in rdid.RenderDataInstructions)
            {
                cs.Write(
                    [[inline]]
                        case MILCMD.Mil[[instruction.Name]]:
                        {
                    [[/inline]]
                        );


                // keep track of push and pop operations
                if (instruction.Name == "PushEffect")
                {
                    cs.Write(
                    [[inline]]
                            pushStack.Push(PushType.BitmapEffect);
                            pushedEffects++;
                    [[/inline]]
                        );
                }
                else if (instruction.Name.StartsWith("Push"))
                {
                    cs.Write(
                    [[inline]]
                            pushStack.Push(PushType.Other);
                    [[/inline]]
                        );

                }

                if (instruction.Name == "Pop")
                {
                    cs.Write(
                    [[inline]]
                            if (pushStack.Pop() == PushType.BitmapEffect)
                            {
                                pushedEffects -= 1;
                            }
                    [[/inline]]
                        );
                }

                // Do not emit AppendCommandData for zero-sized instructions (currently applies to MilPop only)
                if (instruction.GetPaddedSize(false /* don't do animations */) > 0)
                {
                    cs.Write(
                        [[inline]]
                                [[instruction.StructName]] data = *([[instruction.StructName]]*)(pCur + sizeof(RecordHeader));
                        [[/inline]]
                        );

                    if (NeedToMarshallHandles(instruction.BasicPublicFields, /* animated */ false))
                    {
                        cs.Write(
                            [[inline]]

                                    // Marshal the Handles for the dependents
                            [[/inline]]
                            );
                    }

                    foreach(McgField field in instruction.BasicPublicFields)
                    {
                        // Field is a resource that can not be passed by value.
                        if(!field.Type.IsValueType)
                        {
                            string handleName = "data.h" + field.PropertyName;

                            cs.Write(
                                [[inline]]

                                        if ( [[handleName]] != 0 )
                                        {
                                            [[handleName]] = (uint)(((DUCE.IResource)_dependentResources[ (int)( [[handleName]] - 1)]).GetHandle(channel));
                                        }
                                [[/inline]]
                                );
                        }
                    }

                    // Note that we use the calculated size of the struct vs. "sizeof" because "sizeof"
                    // returns a size of at least 1, even for empty structs, which we do not want.
                    cs.Write(
                        [[inline]]

                                channel.AppendCommandData(
                                    (byte*)&data,
                                    [[instruction.GetPaddedSize(false /* don't do animations */)]] /* codegen'ed size of this instruction struct */
                                    );
                        [[/inline]]
                            );
                }
                else
                {
                    cs.Write(
                        [[inline]]
                                /* instruction size is zero, do nothing */
                        [[/inline]]
                            );
                }

                cs.Write(
                    [[inline]]
                        }
                        break;
                    [[/inline]]
                    );

                if (instruction.HasAdvancedParameters)
                {
                    cs.Write(
                        [[inline]]
                            case MILCMD.Mil[[instruction.Name]]Animate:
                            {
                        [[/inline]]
                        );

                    // keep track of push and pop operations
                    if (instruction.Name.StartsWith("Push"))
                    {
                        cs.Write(
                        [[inline]]
                                pushStack.Push(PushType.Other);
                        [[/inline]]
                            );
                    }

                    // Do not emit AppendCommandData for zero-sized instructions (MilPop, etc.)
                    if (instruction.GetPaddedSize(true /* do animations */) > 0)
                    {
                        cs.Write(
                            [[inline]]
                                    [[instruction.StructName]]_ANIMATE data = *([[instruction.StructName]]_ANIMATE*)(pCur + sizeof(RecordHeader));
                            [[/inline]]
                                );

                        if (NeedToMarshallHandles(instruction.AllPublicFields, /* animated */ true))
                        {
                            cs.Write(
                                [[inline]]

                                        // Marshal the Handles for the dependents
                                [[/inline]]
                                    );
                        }

                        foreach(McgField field in instruction.AllPublicFields)
                        {
                            // Field is a resource that can not be passed by value or an animation
                            if(!field.Type.IsValueType || field.IsAnimated)
                            {
                                string handleName = "data.h" + field.PropertyName;

                                if (field.IsAnimated)
                                {
                                    handleName += "Animations";
                                }

                                cs.Write(
                                    [[inline]]

                                            if ( [[handleName]] != 0)
                                            {
                                                [[handleName]] = (uint)(((DUCE.IResource)_dependentResources[ (int)( [[handleName]] - 1)]).GetHandle(channel));
                                            }
                                    [[/inline]]
                                    );
                            }
                        }

                        // Note that we use the calculated size of the struct vs. "sizeof" because "sizeof"
                        // returns a size of at least 1, even for empty structs, which we do not want.
                        cs.Write(
                            [[inline]]

                                    channel.AppendCommandData(
                                        (byte*)&data,
                                        [[instruction.GetPaddedSize(true /* do animations */)]] /* codegen'ed size of this instruction struct */
                                        );
                            [[/inline]]
                                );
                    }
                    else
                    {
                        cs.Write(
                            [[inline]]
                                    /* instruction size is zero, do nothing */
                            [[/inline]]
                                );
                    }

                    cs.Write(
                        [[inline]]
                            }
                            break;
                        [[/inline]]
                    );
                }
            }

            // This is where we have to manually add support for non-generated instructions
            cs.WriteBlock(
                [[inline]]

                    default:
                    {
                        Debug.Assert(false);
                    }
                    break;
                [[/inline]]
                );

            return cs.ToString();
        }

        /// <summary>
        /// WriteDrawingContextWalkInstructions - write the case statements for a switch block which
        /// calls back to a DrawingContextWalker
        /// </summary>
        /// <returns>
        /// string - the contents of the switch block
        /// </returns>
        /// <param name="rdid"> RenderDataInstructionData - the collection of instructions to iterate over. </param>
        /// <param name="useCurrentValue">
        ///   bool - if true, we retrieve the current value of all animations (and animate
        ///   resources) and only call into the non-animate APIs of a DrawingContextWalker.
        /// </param>
        public string WriteDrawingContextWalkInstructions(RenderDataInstructionData rdid,
                                                          bool useCurrentValue)
        {
            StringCodeSink cs = new StringCodeSink();

            string param = null;

            foreach (McgRenderDataInstruction instruction in rdid.RenderDataInstructions)
            {

                //
                // Generate case for basic instruction
                //

                ParameterList paramList = new ParameterList();

                foreach(McgField field in instruction.BasicPublicFields)
                {
                    // Field is a resource that can not be passed by value.
                    if(!field.Type.IsValueType)
                    {
                        param = "(" + field.Type.Name + ")DependentLookup(data->h" + field.PropertyName + ")";
                    }
                    else
                    {
                        param = "data->" + field.Name;
                    }

                    paramList.Append(param);
                }

                if (instruction.Name == "PushEffect")
                {
                    cs.Write(
                        [[inline]]
                            // Disable warning about obsolete method.  This code must remain active 
                            // until we can remove the public BitmapEffect APIs.
                            #pragma warning disable 0618
                        [[/inline]]
                        );
                }
                
                cs.Write(
                    [[inline]]
                        case MILCMD.Mil[[instruction.Name]]:
                        {
                            [[instruction.StructName]]* data = ([[instruction.StructName]]*)(pCur + sizeof(RecordHeader));

                            // Retrieve the resources for the dependents and call the context.
                            ctx.[[instruction.Name]](
                                [[paramList]]
                                );
                        }
                        break;
                    [[/inline]]
                    );

                if (instruction.Name == "PushEffect")
                {
                    cs.Write(
                        [[inline]]
                            #pragma warning restore 0618
                        [[/inline]]
                        );
                }
                
                //
                // Generate case for advanced instruction
                //

                if (instruction.HasAdvancedParameters)
                {
                    ParameterList animatedParamList = new ParameterList();

                    foreach(McgField field in instruction.AllPublicFields)
                    {
                        // Field is a resource that can not be passed by value.
                        if(!field.Type.IsValueType)
                        {
                            param = "(" + field.Type.Name + ")DependentLookup(data->h" + field.PropertyName + ")";

                            // If we're supposed to use the current value,
                            if (useCurrentValue && field.Type.IsAnimatable)
                            {
                                animatedParamList.Append("(data->h" + field.PropertyName + " == 0) ? null : " + param);
                            }
                            else
                            {
                                animatedParamList.Append(param);
                            }
                        }
                        else
                        {
                            param = "data->" + field.Name;

                            // If we're supposed to use the current value,
                            if (useCurrentValue && field.IsAnimated)
                            {
                                animatedParamList.Append(
                                    "(data->h" + field.PropertyName + "Animations == 0) ? " +
                                    param + " : " +
                                    "((" + field.Type.Name + "AnimationClockResource)DependentLookup(data->h" +
                                    field.PropertyName + "Animations)).CurrentValue");

                                    // If this instruction has advanced fields, we must call the advanced
                                    // version of the function by passing 'null' to the AnimationClock parameter
                                    // of the animated properties.
                                    //
                                    bool hasAdvancedFields = ResourceModel.Filter(instruction.AllPublicFields, ResourceModel.IsAdvancedField).Length > 0;
                                    if(field.IsAnimated && hasAdvancedFields)
                                    {
                                        animatedParamList.Append("null");
                                    }
                            }
                            else
                            {
                                animatedParamList.Append(param);

                                if (field.IsAnimated)
                                {
                                    animatedParamList.Append("((" + field.Type.Name + "AnimationClockResource)DependentLookup(data->h" + field.PropertyName + "Animations" + ")).AnimationClock");
                                }
                            }
                        }
                    }

                    cs.Write(
                        [[inline]]
                            case MILCMD.Mil[[instruction.Name]]Animate:
                            {
                                [[instruction.StructName]]_ANIMATE* data = ([[instruction.StructName]]_ANIMATE*)(pCur + sizeof(RecordHeader));

                                // Retrieve the resources for the dependents and call the context.
                                ctx.[[instruction.Name]](
                                    [[animatedParamList]]
                                    );
                            }
                            break;
                        [[/inline]]
                        );
                }
            }

            // Write the default statement
            cs.WriteBlock(
                [[inline]]
                    default:
                    {
                        Debug.Assert(false);
                    }
                    break;
                [[/inline]]
                );

            return cs.ToString();
        }

        /// <summary>
        /// WriteMarshalToDUCE - returns the implementation of MarshalToDUCE for a given
        /// set of RenderData instructions.
        /// </summary>
        /// <returns>
        /// string - the string which contains the implementation of MarshalToDUCE for a given
        /// set of RenderData instructions.
        /// </returns>
        /// <param name="rdid">
        ///   RenderDataInstructionData - the instructions for which we generate the
        ///   MarshalToDUCE implemenation.
        /// </param>
        public string WriteMarshalToDUCE(RenderDataInstructionData rdid)
        {
            StringCodeSink cs = new StringCodeSink();

            cs.WriteBlock(
                [[inline]]
                    /// <summary>
                    /// MarshalToDUCE - Marshalling code to the DUCE
                    /// </summary>
                    /// <SecurityNote>
                    ///    Critical:Calls into unsafe code
                    ///    TreatAsSafe: This code is ok to expose. Channels are safe to call with bad data.
                    ///    They do not affect windows cross process or cross app domain
                    /// </SecurityNote>
                    [SecurityCritical,SecurityTreatAsSafe]
                    private void MarshalToDUCE(DUCE.Channel channel)
                    {
                        Debug.Assert(_duceResource.IsOnChannel(channel));

                        DUCE.MILCMD_RENDERDATA renderdataCmd;
                        renderdataCmd.Type = MILCMD.MilCmdRenderData;
                        renderdataCmd.Handle = _duceResource.GetHandle(channel);
                        renderdataCmd.cbData = (uint)DataSize;

                        // This is the total extra size required
                        uint cbExtraData = renderdataCmd.cbData;

                        // This cast is to ensure that cbExtraData can be cast to an int without
                        // wrapping over, since in managed code indices are int, not uint.
                        Debug.Assert(cbExtraData <= (uint)Int32.MaxValue);

                        unsafe
                        {
                            channel.BeginCommand(
                                (byte*)&renderdataCmd,
                                sizeof(DUCE.MILCMD_RENDERDATA),
                                (int)cbExtraData
                                );

                            // We shouldn't have any dependent resources if _curOffset is 0
                            // (_curOffset == 0) -> (renderData._dependentResources.Count == 0)
                            Debug.Assert((_curOffset > 0) || (_dependentResources.Count == 0));

                            // The buffer being null implies that _curOffset must be 0.
                            // (_buffer == null) -> (_curOffset == 0)
                            Debug.Assert((_buffer != null) || (_curOffset == 0));

                            // The _curOffset must be less than the length, if there is a buffer.
                            Debug.Assert((_buffer == null) || (_curOffset <= _buffer.Length));

                            Stack<PushType> pushStack = new Stack<PushType>();
                            int pushedEffects = 0;

                            if (_curOffset > 0)
                            {
                                fixed (byte* pByte = this._buffer)
                                {
                                    // This pointer points to the current read point in the
                                    // instruction stream.
                                    byte* pCur = pByte;

                                    // This points to the first byte past the end of the
                                    // instruction stream (i.e. when to stop)
                                    byte* pEndOfInstructions = pByte + _curOffset;

                                    while (pCur < pEndOfInstructions)
                                    {
                                        RecordHeader *pCurRecord = (RecordHeader*)pCur;

                                        channel.AppendCommandData(
                                        (byte*)pCurRecord,
                                            sizeof(RecordHeader)
                                            );

                                         switch (pCurRecord->Id)
                                         {
                                            [[WriteDUCEMarshalInstructions(rdid)]]
                                         }
                                        pCur += pCurRecord->Size;
                                    }
                                }
                            }
                            channel.EndCommand();
                        }
                    }
                [[/inline]]
                    );

            return cs.ToString();
        }


        /// <summary>
        /// WriteDrawingContextWalk - returns the implementation of DrawingContextWalk for a given
        /// set of RenderData instructions.
        /// </summary>
        /// <returns>
        /// string - the string which contains the implementation of DrawingContextWalk for a given
        /// set of RenderData instructions.
        /// </returns>
        /// <param name="rdid">
        ///   RenderDataInstructionData - the instructions for which we generate the
        ///   DrawingContextWalk implemenation.
        /// </param>
        public string WriteDrawingContextWalkBody(RenderDataInstructionData rdid, bool useCurrentValue)
        {
            StringCodeSink cs = new StringCodeSink();

            cs.WriteBlock(
                [[inline]]
                    {
                        // We shouldn't have any dependent resources if _curOffset is 0
                        // (_curOffset == 0) -> (renderData._dependentResources.Count == 0)
                        Debug.Assert((_curOffset > 0) || (_dependentResources.Count == 0));

                        // The buffer being null implies that _curOffset must be 0.
                        // (_buffer == null) -> (_curOffset == 0)
                        Debug.Assert((_buffer != null) || (_curOffset == 0));

                        // The _curOffset must be less than the length, if there is a buffer.
                        Debug.Assert((_buffer == null) || (_curOffset <= _buffer.Length));

                        if (_curOffset > 0)
                        {
                            unsafe
                            {
                                fixed (byte* pByte = this._buffer)
                                {
                                    // This pointer points to the current read point in the
                                    // instruction stream.
                                    byte* pCur = pByte;

                                    // This points to the first byte past the end of the
                                    // instruction stream (i.e. when to stop)
                                    byte* pEndOfInstructions = pByte + _curOffset;

                                    // Iterate across the entire list of instructions, stopping at the
                                    // end or when the DrawingContextWalker has signalled a stop.
                                    while ((pCur < pEndOfInstructions) && !ctx.ShouldStopWalking)
                                    {
                                        RecordHeader* pCurRecord = (RecordHeader*)pCur;

                                        switch (pCurRecord->Id)
                                        {
                                            [[WriteDrawingContextWalkInstructions(rdid, useCurrentValue )]]
                                        }

                                        pCur += pCurRecord->Size;
                                    }
                                }
                            }
                        }
                    }
                [[/inline]]
                    );

            return cs.ToString();
        }

        /// <summary>
        /// DoRenderDataManaged - Emit the managed class which contains a blob for the actual inline
        /// renderdata instructions.
        /// </summary>
        /// <param name="rdid"> RenderDataInstructionData - the instructions to emit. </param>
        /// <param name="fullManagedPath"> The path for the filename. </param>
        /// <param name="renderDataManagedFileName"> The filename. </param>
        public void DoRenderDataManaged(RenderDataInstructionData rdid,
                                        string fullManagedPath,
                                        string renderDataManagedFileName)
        {
            using (FileCodeSink rdFile = new FileCodeSink(fullManagedPath, renderDataManagedFileName, true /* Create dir if necessary */))
            {
                rdFile.WriteBlock(
                    [[inline]]
                        [[Helpers.ManagedStyle.WriteFileHeader(renderDataManagedFileName)]]
                    [[/inline]]
                    );

                foreach (string s in rdid.Namespaces)
                {
                    rdFile.Write(
                        [[inline]]
                            using [[s]];
                        [[/inline]]
                        );
                }

                rdFile.WriteBlock(
                    [[inline]]

                        namespace [[rdid.ManagedNamespace]]
                        {
                            [[WriteRecordDefinitions(rdid)]]

                            /// <summary>
                            ///     RenderDataDrawingContext - A DrawingContext which produces a Drawing.
                            /// </summary>
                            internal partial class RenderData: DUCE.IResource
                            {
                                [[WriteMarshalToDUCE(rdid)]]

                                /// <summary>
                                /// DrawingContextWalk - Iterates this renderdata and call out to methods on the
                                /// provided DrawingContext, passing the current values to their parameters.
                                /// </summary>
                                /// <SecurityNote>
                                ///     Critical:This code calls into unsafe code
                                ///     TreatAsSafe: This code is ok to expose. Writing to a channel is a safe operation.
                                ///     Channels can deal with bad pointers.
                                /// </SecurityNote>
                                [SecurityCritical,SecurityTreatAsSafe]
                                public void DrawingContextWalk(DrawingContextWalker ctx)
                                [[WriteDrawingContextWalkBody(rdid, true /* Use current value of animations */)]]

                                /// <summary>
                                /// BaseValueDrawingContextWalk - Iterates this renderdata and call out to methods on the
                                /// provided DrawingContext, passing base values and animations to their parameters.
                                /// </summary>
                                /// <SecurityNote>
                                ///     Critical:This code calls into unsafe code
                                ///     TreatAsSafe: This code is ok to expose. Writing to a channel is a safe operation.
                                ///     Channels can deal with bad pointers.
                                /// </SecurityNote>
                                [SecurityCritical, SecurityTreatAsSafe]
                                public void BaseValueDrawingContextWalk(DrawingContextWalker ctx)
                                [[WriteDrawingContextWalkBody(rdid, false /* Use the base value & animations */)]]
                            }
                        }
                    [[/inline]]
                        );
            }
        }

        private void DoRenderDataUnmanaged(
              RenderDataInstructionData rdid,
              string fullUnmanagedPath,
              string renderDataFileName)
        {
            using (FileCodeSink rdFile = new FileCodeSink(fullUnmanagedPath, renderDataFileName, true /* Create dir if necessary */))
            {
                Helpers.Style.WriteFileHeader(rdFile);

                StringCodeSink cs = new StringCodeSink();

                foreach (McgRenderDataInstruction renderdataInstruction in rdid.RenderDataInstructions)
                {
                    cs.WriteBlock(WriteUnmanagedGetHandle(renderdataInstruction, false /* don't do animation */));

                    if (renderdataInstruction.HasAdvancedParameters)
                    {
                        cs.WriteBlock(WriteUnmanagedGetHandle(renderdataInstruction, true /* do animation */));
                    }
                }

                rdFile.WriteBlock(
                    [[inline]]

                        #include "precomp.hpp"

                        HRESULT
                        CMilSlaveRenderData::GetHandles(CMilSlaveHandleTable *pHandleTable)
                        {
                            HRESULT hr = S_OK;
                            UINT nItemID;
                            PVOID pItemData;
                            UINT nItemDataSize;
                            int stackDepth;

                            //
                            // Set up the command enumeration.
                            //

                            CMilDataBlockReader cmdReader(m_instructions.FlushData());

                            IFC(m_rgpResources.Add(NULL));

                            stackDepth = 0;

                            //
                            // Now get the first item and start executing the render buffer.
                            //

                            IFC(cmdReader.GetFirstItemSafe(&nItemID, &pItemData, &nItemDataSize));

                            while (hr == S_OK)
                            {

                                //
                                // Dispatch the current command to the appropriate handler routine.
                                //

                                if (SUCCEEDED(hr))
                                {
                                    switch (nItemID)
                                    {
                                    default:
                                        MIL_THR( WGXERR_UCE_MALFORMEDPACKET );
                                        break;

                                    [[cs]]
                                    }
                                }

                                IFC(cmdReader.GetNextItemSafe(
                                    &nItemID,
                                    &pItemData,
                                    &nItemDataSize
                                    ));
                            }

                            //
                            // S_FALSE means that we reached the end of the stream. Hence we executed the stream
                            // correctly and therefore we should return S_OK.
                            //

                            if (hr == S_FALSE)
                            {
                                hr = S_OK;
                            }

                            //
                            // Ensure that we have a matched number of Push/
                            // Pop calls, otherwise the render data is invalid
                            //
                            if (stackDepth != 0)
                            {
                                IFC(WGXERR_UCE_MALFORMEDPACKET);
                            }

                        Cleanup:

                            Assert(SUCCEEDED(hr));
                            RRETURN(hr);
                        }

                    [[/inline]]
                    );
            }
        }

        /// <summary>
        /// WriteUnmanagedGetHandle
        /// Emits the unmanaged switch block which, in GetHandle, retrieves the handles for a given
        /// instruction.
        /// </summary>
        /// <returns>
        /// string - the text of the methods
        /// </returns>
        /// <param name="renderdataInstruction"> The RenderDataInstruction to emit. </param>
        /// <param name="doAdvancedParameters">
        ///    Whether or not this call should emit the advanced version of this api.
        ///    If true, it will emit only the advanced version, if false, it will emit only the
        ///    basic version.
        /// </param>
        private string WriteUnmanagedGetHandle(McgRenderDataInstruction renderdataInstruction, bool doAdvancedParameters)
        {
            StringCodeSink cs = new StringCodeSink();

            string idSuffix = String.Empty;
            string structSuffix = String.Empty;

            // The name changes if there's animation
            if (doAdvancedParameters)
            {
                idSuffix = "Animate";
                structSuffix = "_ANIMATE";
            }

            // Use this code sink to accumulate the per-field info
            StringCodeSink csFields = new StringCodeSink();

            // Use this to determine if we have any fields worth processing
            bool haveResourceFields = false;

            McgField[] parameters = (doAdvancedParameters) ? renderdataInstruction.AllPublicFields : renderdataInstruction.BasicPublicFields;

            foreach (McgField field in parameters)
            {
                // Is it a resource?
                if (!field.Type.IsValueType && !field.IsManagedOnly)
                {
                    // We've found a field worth processing
                    McgResource resource = field.Type as McgResource;
                    Debug.Assert(resource != null);

                    haveResourceFields = true;

                    csFields.WriteBlock("IFC(AddHandleToArrayAndReplace(&(pData->h" + field.PropertyName + "), " + resource.MilTypeEnum + ", &m_rgpResources, pHandleTable));");
                }
                // Handle animations, if appropriate
                else if (doAdvancedParameters && field.IsAnimated)
                {
                    // We've found a field worth processing
                    McgResource resource = field.Type as McgResource;
                    Debug.Assert(resource != null);

                    haveResourceFields = true;

                    csFields.WriteBlock("IFC(AddHandleToArrayAndReplace(&(pData->h" + field.PropertyName + "Animations), " + resource.MilTypeEnum + ", &m_rgpResources, pHandleTable));");
                }
            }

            cs.Write(
                [[inline]]
                    case Mil[[renderdataInstruction.UnmanagedName]][[idSuffix]]:
                [[/inline]]
                );

            if (renderdataInstruction.UnmanagedName == "PushGuidelineY1")
            {
                cs.WriteBlock(
                    [[inline]]
                            {
                                if (nItemDataSize < sizeof([[renderdataInstruction.StructName]][[structSuffix]]))
                                {
                                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                                }

                                MILCMD_PUSH_GUIDELINE_Y1 *pData = static_cast<MILCMD_PUSH_GUIDELINE_Y1*>(pItemData);

                                CGuidelineCollection* pGuidelineCollection = NULL;
                                float coordinates[2] = {
                                    static_cast<float>(pData->coordinate),
                                    0
                                    };

                                MIL_THR(CDynamicGuidelineCollection::Create(
                                    0, //uCountX
                                    2, //uCountY
                                    coordinates,
                                    &pGuidelineCollection
                                    ));
                                if (FAILED(hr))
                                {
                                    Assert(pGuidelineCollection == NULL);
                                    if (hr != WGXERR_MALFORMED_GUIDELINE_DATA)
                                        goto Cleanup;
                                    // WGXERR_MALFORMED_GUIDELINE_DATA handling:
                                    // allow NULL in m_rgpGuidelineKits that will
                                    // cause pushing empty guideline frame
                                }

                                MIL_THR(m_rgpGuidelineKits.Add(pGuidelineCollection));
                                if (FAILED(hr))
                                {
                                    delete pGuidelineCollection;
                                    goto Cleanup;
                                }

                                *(reinterpret_cast<UINT*>(&pData->coordinate)) = m_rgpGuidelineKits.GetCount() - 1;
                            }              
                    [[/inline]]
                    );
            }
            else if (renderdataInstruction.UnmanagedName == "PushGuidelineY2")
            {
                cs.WriteBlock(
                    [[inline]]
                            {
                                if (nItemDataSize < sizeof([[renderdataInstruction.StructName]][[structSuffix]]))
                                {
                                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                                }

                                MILCMD_PUSH_GUIDELINE_Y2 *pData = static_cast<MILCMD_PUSH_GUIDELINE_Y2*>(pItemData);

                                float coordinates[2] =
                                {
                                    static_cast<float>(pData->leadingCoordinate),
                                    static_cast<float>(pData->offsetToDrivenCoordinate)
                                };

                                CGuidelineCollection* pGuidelineCollection = NULL;

                                MIL_THR(CDynamicGuidelineCollection::Create(
                                    0, //uCountX
                                    2, //uCountY
                                    coordinates,
                                    &pGuidelineCollection
                                    ));
                                if (FAILED(hr))
                                {
                                    Assert(pGuidelineCollection == NULL);
                                    if (hr != WGXERR_MALFORMED_GUIDELINE_DATA)
                                        goto Cleanup;
                                    // WGXERR_MALFORMED_GUIDELINE_DATA handling:
                                    // allow NULL in m_rgpGuidelineKits that will
                                    // cause pushing empty guideline frame
                                }

                                MIL_THR(m_rgpGuidelineKits.Add(pGuidelineCollection));
                                if (FAILED(hr))
                                {
                                    delete pGuidelineCollection;
                                    goto Cleanup;
                                }

                                *(reinterpret_cast<UINT*>(&pData->leadingCoordinate)) = m_rgpGuidelineKits.GetCount() - 1;
                            }
                    [[/inline]]
                    );
            }
            // If there are no fields, just emit "break"
            else if (haveResourceFields)
            {
                cs.WriteBlock(
                    [[inline]]
                            {
                                if (nItemDataSize < sizeof([[renderdataInstruction.StructName]][[structSuffix]]))
                                {
                                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                                }

                                [[renderdataInstruction.StructName]][[structSuffix]] *pData = static_cast<[[renderdataInstruction.StructName]][[structSuffix]]*>(pItemData);
                                [[csFields]]
                            }
                    [[/inline]]
                    );
            }
                        
            cs.WriteBlock(
                [[inline]]
                        [[WriteStackOperation(renderdataInstruction, false)]]
                        break;
                [[/inline]]
                    );
    
            return cs.ToString();
        }

        private string WriteComment(McgRenderDataInstruction rdi,
                                    bool skipAdvancedParameters,
                                    bool managedImport)
        {
            StringCodeSink cs = new StringCodeSink();

            cs.Write(
                [[inline]]
                    /// <summary>
                    [[Helpers.CodeGenHelpers.FormatComment(rdi.Comment, 84,  "///     " + rdi.Name + " - \n", "/// " + rdi.Name, true /* managed */)]]
                    /// </summary>
                [[/inline]]
                );

            McgField[] parameters = skipAdvancedParameters ? rdi.BasicPublicFields : rdi.AllPublicFields;

            foreach (McgField field in parameters)
            {
                // If this field is not a value type, it's being passed by handle
                if (!field.Type.IsValueType && managedImport)
                {
                    cs.WriteBlock(Helpers.CodeGenHelpers.FormatParam("h" + field.PropertyName,
                                                             field.Comment,
                                                             field.Type.Name,
                                                             80));
                }
                else
                {
                    cs.WriteBlock(Helpers.CodeGenHelpers.FormatParam(field.Name,
                                                             field.Comment,
                                                             field.Type.Name,
                                                             80));
                }

                if (!skipAdvancedParameters && field.Type.IsValueType && field.IsAnimated)
                {
                    if (managedImport)
                    {
                        cs.WriteBlock("/// <param name=\"h" + GeneratorMethods.FirstCap(field.Name) + "Animations\"> Optional " + field.Type.Name + " animation resource for " + field.Name + ". </param>");
                    }
                    else
                    {
                        cs.WriteBlock("/// <param name=\"" + field.Name + "Animations\"> Optional AnimationClock for " + field.Name + ". </param>");
                    }
                }
            }

            return cs.ToString();
        }

        private string WriteUnmanagedComment(McgRenderDataInstruction rdi,
                                             string className,
                                             bool skipAnimations)
        {
            StringCodeSink cs = new StringCodeSink();

            string memberName = String.Empty;

            if ((className != null) && (className.Length > 0))
            {
                memberName = className + "::";
            }

            memberName += rdi.Name;

            if (!skipAnimations)
            {
                memberName += "_Animate";
            }

            cs.Write(
                [[inline]]
                    //+---------------------------------------------------------------------------
                    //
                    //  Member:     [[memberName]]
                    //
                    [[Helpers.CodeGenHelpers.FormatComment(rdi.Comment, 84,  "//  Synopsis:   \n", String.Empty, false /* unmanaged */)]]
                    //
                    //  Returns:    HRESULT.
                    //
                    //  Params:
                [[/inline]]
                );

            foreach (McgField field in rdi.AllPublicFields)
            {
                // If this field is not a value type, it's being passed by handle
                if (!field.Type.IsValueType)
                {
                    cs.WriteBlock(Helpers.CodeGenHelpers.FormatParamUnmanaged("h" + field.Name,
                                                                      field.Comment,
                                                                      field.Type.Name,
                                                                      80));
                }

                if (!skipAnimations && field.Type.IsValueType && field.IsAnimated)
                {
                    cs.WriteBlock("//              h" + GeneratorMethods.FirstCap(field.Name) + "Animations: Optional " + field.Type.Name + " animation resource for " + field.Name + ".");
                }
            }

            cs.Write("//----------------------------------------------------------------------------\n");

            return cs.ToString();
        }

        /// <summary>
        /// WriteInitialChecks - Writes any checks that are needed before the instruction
        /// is recorded to the instruction stream.
        /// </summary>
        /// <returns>
        /// string - the code to add to the drawing call's body
        /// </returns>
        /// <param name="instruction"> The McgRenderDataInstruction to which this logic is applied. </param>
        /// <param name="isManaged"> bool - is this managed or unmanaged? </param>
        /// <param name="indent"> bool - should we indent the function body? </param>
        private static string WriteInitialChecks(McgRenderDataInstruction instruction, bool isManaged, bool indent)
        {
            // Initialize return string to empty in case no checks are needed before
            // the instruction is recorded to the instruction stream.
            string returnString = String.Empty;

            if (instruction != null)
            {
                // If this is a pop instruction, write a check that validates the stack depth
                if (instruction.IsPop)
                {
                    // Support isn't written for Pop instructions with NoOp groups
                    Debug.Assert(
                        (instruction.NoOpGroups == null)  ||
                        (instruction.NoOpGroups.Length == 0)
                        );

                    returnString = WritePopCheck();
                }
                // If in-parameters can cause the operation to become a no-op, write a check
                // that no-ops the operation when the no-op conditions are met.
                else if(instruction.NoOpGroups != null &&
                        instruction.NoOpGroups.Length > 0)
                {
                    returnString = WriteNoOpCheck(instruction, isManaged, indent);
                }
            }

            return returnString;
        }

        /// <summary>
        /// WritePopCheck - Writes a check that validates the stack depth of a Pop instruction
        /// </summary>
        /// <returns>
        /// string - the code to add to the drawing call's body
        /// </returns>
        private static string WritePopCheck()
        {
            StringCodeSink cs = new StringCodeSink();

            cs.Write(
                [[inline]]
                    if (_stackDepth <= 0)
                    {
                        throw new InvalidOperationException(SR.Get(SRID.DrawingContext_TooManyPops));
                    }
                [[/inline]]
                );

            return cs.ToString();
        }

        /// <summary>
        /// WriteNoOpCheck - Emits the code which allows potential "early out" logic for a given instruction.
        /// </summary>
        /// <returns>
        /// string - the code to add to the drawing call's body
        /// </returns>
        /// <param name="instruction"> The McgRenderDataInstruction to which this logic is applied. </param>
        /// <param name="isManaged"> bool - is this managed or unmanaged? </param>
        /// <param name="indent"> bool - should we indent the function body? </param>
        private static string WriteNoOpCheck(McgRenderDataInstruction instruction, bool isManaged, bool indent)
        {
            // This method assumes it is only called when the following conditions are true
            Debug.Assert(
                (instruction != null) &&
                (instruction.NoOpGroups != null) &&
                (instruction.NoOpGroups.Length > 0)
                );

            StringCodeSink cs = new StringCodeSink();

            DelimitedList listOfGroups = new DelimitedList(" || ", DelimiterPosition.AfterItem, false /* no newline */);

            cs.Write("if ");

            if (instruction.NoOpGroups.Length > 1)
            {
                cs.Write("(");
            }

            foreach (McgField[] noOpGroup in instruction.NoOpGroups)
            {
                // This is ensured by the schema
                Debug.Assert(noOpGroup.Length > 0);

                StringCodeSink innerCs = new StringCodeSink();

                if (noOpGroup.Length > 1)
                {
                    innerCs.Write("(");
                }

                DelimitedList list = new DelimitedList(" && ", DelimiterPosition.AfterItem, false /* no newline */);

                foreach (McgField field in noOpGroup)
                {
                    if (isManaged)
                    {
                        list.Append("(" + field.Name + " == null)");
                    }
                    else
                    {
                        list.Append("(" + DuceHandle.UnmanagedNullHandle + " == h" + field.PropertyName + ")");
                    }
                }

                innerCs.Write(list.ToString());

                if (noOpGroup.Length > 1)
                {
                    innerCs.Write(")");
                }

                listOfGroups.Append(innerCs.ToString());
            }

            cs.Write(listOfGroups.ToString());

            if (instruction.NoOpGroups.Length > 1)
            {
                cs.Write(")");
            }

            if (indent)
            {
                cs.Indent(4);
            }

            if (isManaged)
            {
                cs.Write(
                    [[inline]]

                        {
                            return;
                        }

                    [[/inline]]
                );
            }
            else
            {
                cs.Write(
                    [[inline]]

                        {
                            hr = S_OK;
                            goto Cleanup;
                        }

                    [[/inline]]
                );
            }

            if (indent)
            {
                cs.Unindent();
            }

            return cs.ToString();
        }

        /// <summary>
        /// WriteStackOperation
        /// Emits stack depth increment code for Push instructions and decrement
        /// code for Pop instruction.
        /// </summary>
        /// <returns>
        /// string - the text of the methods
        /// </returns>
        /// <param name="instruction"> The McgRenderDataInstruction for current the instruction. </param>
        /// <param name="managedCode"> Is output code managed? This affects the naming convention </param>
        private static string WriteStackOperation(McgRenderDataInstruction instruction, bool managedCode)
        {
            if (instruction.IsPush)
            {
                return managedCode ? "_stackDepth++;" : "stackDepth++;";
            }
            else if (instruction.IsPop)
            {
                return managedCode ? "_stackDepth--;" : "stackDepth--;";
            }
            else
            {
                return "";
            }
        }

        /// <summary>
        /// WriteEffectStackOperation
        /// Emits stack depth increment code for Push instructions and decrement
        /// code for Pop instruction.
        /// </summary>
        /// <returns>
        /// string - the text of the methods
        /// </returns>
        /// <param name="instruction"> The McgRenderDataInstruction for current the instruction. </param>
        private static string WriteEffectStackOperation(McgRenderDataInstruction instruction)
        {
            StringCodeSink cs = new StringCodeSink();
            if (instruction.Name == "PushEffect")
            {
                cs.Write(
                    [[inline]]
                        if (_renderData.BitmapEffectStackDepth == 0)
                        {
                            _renderData.BeginTopLevelBitmapEffect(_stackDepth);
                        }
                    [[/inline]]
                );
            }
            else if (instruction.IsPop)
            {
                cs.Write(
                    [[inline]]
                        // end the top level effect, if we are popping the top
                        // level push effect instruction
                        if (_renderData.BitmapEffectStackDepth == (_stackDepth + 1))
                        {
                            _renderData.EndTopLevelBitmapEffect();
                        }

                    [[/inline]]
                );
            }

            return cs.ToString();
        }
        private void DoIncEntry(
            CodeSink cs,
            McgRenderDataInstruction renderdataInstruction,
            bool doAdvancedParameters
            )
        {
            string structName = renderdataInstruction.IncStructName;

            // This is added to differentiate the different pad fields (if necessary)
            int padSuffix = 0;

            string suffix = String.Empty;

            if (doAdvancedParameters)
            {
                suffix = "_ANIMATE";
            }

            // Sort the struct for alignment
            Helpers.CodeGenHelpers.PaddedStructData psd = renderdataInstruction.GetPaddedStructDefinition(doAdvancedParameters);
            Helpers.CodeGenHelpers.AlignmentEntry[] alignmentEntries = psd.AlignmentEntries;

            StringCodeSink fields = new StringCodeSink();

            Helpers.CodeGenHelpers.AlignedFieldOffsetHelper alignedFieldHelper = new Helpers.CodeGenHelpers.AlignedFieldOffsetHelper(4, 4);
            
            for (int i = 0; i < alignmentEntries.Length; i++)
            {
                alignedFieldHelper.MoveToNextEntry(alignmentEntries[i].Offset, alignmentEntries[i].Size);

                // Insert enough padding to ensure the field is properly aligned
                foreach(string alignmentField in alignedFieldHelper.AlignmentFields)
                {
                    fields.WriteBlock(alignmentField);
                }
                
                if (alignmentEntries[i].IsPad)
                {
                    fields.WriteBlock(
                        [[inline]]UINT32 QuadWordPad[[padSuffix++]];[[/inline]]
                        );
                }
                else
                {
                    McgField field = alignmentEntries[i].Field;
                    McgResource resource = (McgResource)field.Type;

                    if (!field.IsManagedOnly)
                    {
                        // If it's not pass by resource, we declare a handle
                        if (!resource.IsValueType)
                        {
                            fields.WriteBlock(
                                [[inline]]HMIL_RESOURCE h[[field.PropertyName]];[[/inline]]
                                );
                        }
                        else
                        {
                            if (!alignmentEntries[i].IsAnimation)
                            {
                                fields.WriteBlock(
                                    [[inline]][[resource.UnmanagedDataType]] [[field.Name]];[[/inline]]
                                    );
                            }
                            else // Animate
                            {
                                fields.WriteBlock(
                                [[inline]]HMIL_RESOURCE h[[field.PropertyName]]Animations;[[/inline]]
                                );
                            }
                        }
                    }
                }
            }
            
            // Insert enough padding to ensure the struct's size falls on a packing boundary
            foreach(string packingField in alignedFieldHelper.NativePackingFields)
            {
                fields.WriteBlock(packingField);
            }

            cs.WriteBlock(
                [[inline]]
                    struct [[renderdataInstruction.StructName + suffix]]
                    {
                        MILCMD type;
                        [[fields.ToString()]]
                    };

                [[/inline]]
                );
        }

        private void DoIncFile(
            RenderDataInstructionData rdid,
            string fullPath,
            string fileName)
        {
            using (FileCodeSink rdFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
            {
                Helpers.Style.WriteFileHeader(rdFile);

                foreach (McgRenderDataInstruction renderdataInstruction in rdid.RenderDataInstructions)
                {
                    DoIncEntry(rdFile, renderdataInstruction, false /* don't do advanced parameters*/);

                    if (renderdataInstruction.HasAdvancedParameters)
                    {
                        DoIncEntry(rdFile, renderdataInstruction, true);
                    }
                }
            }
        }

        private void DoManagedRecordEntry(
            CodeSink cs,
            McgRenderDataInstruction renderdataInstruction,
            bool doAdvancedRecord
            )
        {
            // This is added to differentiate the different pad fields (if necessary)
            int padSuffix = 0;

            // Sort the struct for alignment
            Helpers.CodeGenHelpers.PaddedStructData psd = renderdataInstruction.GetPaddedStructDefinition(doAdvancedRecord);
            Helpers.CodeGenHelpers.AlignmentEntry[] alignmentEntries = psd.AlignmentEntries;

            string structName = renderdataInstruction.StructName;

            if (doAdvancedRecord)
            {
                structName += "_ANIMATE";
            }

            cs.Write("[StructLayout(LayoutKind.Explicit)]\n");
            cs.Write("internal struct " + structName + "\n{");

            cs.Indent(3);

            //
            //
            // Generate the constructor for this struct
            //
            //

            if (renderdataInstruction.AllPublicFields.Length > 0)
            {
                McgField[] publicParameters = doAdvancedRecord ? renderdataInstruction.AllPublicFields : renderdataInstruction.BasicPublicFields;
                Helpers.CodeGenHelpers.ParameterType paramType = Helpers.CodeGenHelpers.ParameterType.ManagedImportsParamList;

                //
                // Generate the parameters to the managed struct's constructor
                //

                // Skip animations & non-basic parameter if we're not supposed to handle
                // animations in this call.
                if (!doAdvancedRecord)
                {
                    paramType |= Helpers.CodeGenHelpers.ParameterType.SkipAnimations;
                }

                ParameterList paramList = new ParameterList();
                Helpers.CodeGenHelpers.AppendParameters(
                    paramList,
                    publicParameters,
                    paramType);

                //
                // Generate assignment of the managed struct constructor's parameters
                // to it's member variables.
                //

                // ... and the field assignments
                paramType = Helpers.CodeGenHelpers.ParameterType.UnmanagedCallParamList |
                            Helpers.CodeGenHelpers.ParameterType.AssignToMemberVariables;

                // Skip animations if we're not supposed to handle animations in this call.
                if (!doAdvancedRecord)
                {
                    paramType |= Helpers.CodeGenHelpers.ParameterType.SkipAnimations;
                }

                DelimitedList assignList = new DelimitedList(";", DelimiterPosition.AfterItem);
                Helpers.CodeGenHelpers.AppendParameters(
                    assignList,
                    publicParameters,
                    paramType);

                //
                // Generate assignment of 0 to any padding fields and internal fields in this struct
                //

                // Since this is a value type, we need to assign 0's to the pad members (if present)
                for (int i = 0; i < alignmentEntries.Length; i++)
                {
                    if (alignmentEntries[i].IsPad)
                    {
                        assignList.Append("this.QuadWordPad" + (padSuffix++) + " = 0");
                    }
                    else if (alignmentEntries[i].Field.IsInternal)
                    {
                        assignList.Append("this." + alignmentEntries[i].Field.Name + " = default(" + alignmentEntries[i].Field.Type.ManagedName + ")");
                    }
                }


                //
                // Generate the constructor by pasting the parameter list & assignment
                // list into the method's body
                //

                // reset padSuffix to 0
                padSuffix = 0;

                cs.Write(
                    [[inline]]

                        public [[structName]] (
                            [[paramList]]
                            )
                        {
                            [[assignList]];
                        }
                    [[/inline]]
                    );
            }


            //
            //
            // Generate the field definition for this struct
            //
            //
            Helpers.CodeGenHelpers.AlignedFieldOffsetHelper alignedFieldHelper = new Helpers.CodeGenHelpers.AlignedFieldOffsetHelper(4, 4);
            
            for (int i = 0; i < alignmentEntries.Length; i++)
            {
                alignedFieldHelper.MoveToNextEntry(alignmentEntries[i].Offset, alignmentEntries[i].Size);
                int alignedFieldOffset = alignedFieldHelper.AlignedFieldOffset;
                
                if (alignmentEntries[i].IsPad)
                {
                    cs.Write("\n[FieldOffset(" + alignedFieldOffset + ")] private UInt32 QuadWordPad" + (padSuffix++) + ";");
                }
                else
                {
                    McgField field = alignmentEntries[i].Field;
                    McgResource resource = (McgResource)field.Type;

                    // If it's not pass by resource, we declare a handle
                    if (!resource.IsValueType)
                    {
                        cs.Write("\n[FieldOffset(" + alignedFieldOffset + ")] public " + DuceHandle.ManagedTypeName + " h");
                        cs.Write(field.PropertyName);
                        cs.Write(";");
                    }
                    else
                    {
                        if (!alignmentEntries[i].IsAnimation)
                        {
                            cs.Write("\n[FieldOffset(" + alignedFieldOffset + ")] public ");
                            cs.Write(resource.ManagedName);
                            cs.Write(" ");
                            cs.Write(field.Name);
                            cs.Write(";");
                        }
                        else // Animate
                        {
                            cs.Write("\n[FieldOffset(" + alignedFieldOffset + ")] public " + DuceHandle.ManagedTypeName + " h");
                            cs.Write(field.PropertyName);
                            cs.Write("Animations;");
                        }
                    }
                }
            }
            
            // Insert a field to ensure the struct's size falls on a packing boundary
            foreach(string packingField in alignedFieldHelper.ManagedPackingFields)
            {
                cs.Write(packingField);
            }

            cs.Unindent();
            cs.Write("\n}\n\n");
        }

        private string WriteRecordDefinitions(RenderDataInstructionData rdid)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (McgRenderDataInstruction renderdataInstruction in rdid.RenderDataInstructions)
            {
                DoManagedRecordEntry(cs, renderdataInstruction, false /* don't do non-basic parameters */);

                if (renderdataInstruction.HasAdvancedParameters)
                {
                    DoManagedRecordEntry(cs, renderdataInstruction, true);
                }
            }

            return cs.ToString();
        }

        private bool NeedToMarshallHandles(McgField[] fields, bool animated)
        {
            foreach(McgField field in fields)
            {
                // Field is a resource that can not be passed by value or an animation
                if(!field.Type.IsValueType || (animated && field.IsAnimated))
                {
                    return true;
                }
            }

            return false;
        }
        #endregion Public Methods
    }
}




