// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// Abstract:
//      Generator for structures and enums in wgx_render_types_generated.h
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

    public class MilRenderTypesGenerated : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public MilRenderTypesGenerated(ResourceModel rm) : base(rm)
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

            FileCodeSink cppFile = new FileCodeSink(generatedPath, "wgx_render_types_generated.h");;

            Helpers.Style.WriteFileHeader(cppFile);

            foreach (McgEnum e in _resourceModel.Enums)
            {
                if (_resourceModel.ShouldGenerate(CodeSections.NativeMilRenderTypes, e))
                {
                    cppFile.WriteBlock(EnumHelper.FormatNativeEnum(e));
                }
            }

            foreach (McgResource r in _resourceModel.Resources)
            {
                if (_resourceModel.ShouldGenerate(CodeSections.NativeMilRenderTypes, r))
                {
                    cppFile.WriteBlock(StructHelper.FormatNativeStructure(r));
                }
            }

            //
            // MilCompoundStyle is a special case
            //
            McgEnum milCompoundStyleEnum = (McgEnum)_resourceModel.FindType("MilCompoundStyle");

            cppFile.WriteBlock(
                [[inline]]
                    #ifdef COMPOUND_PEN_IMPLEMENTED
                    [[EnumHelper.FormatNativeEnum(milCompoundStyleEnum)]]
                    #endif // COMPOUND_PEN_IMPLEMENTED
                [[/inline]]
                );

            cppFile.Dispose();
        }

        #endregion Public Methods
     }
}


