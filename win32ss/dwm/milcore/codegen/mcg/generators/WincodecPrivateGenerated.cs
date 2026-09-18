// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// Abstract:
//      Generator for types in wincodec_private_generated.h
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

    public class WincodecPrivateGenerated : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public WincodecPrivateGenerated(ResourceModel rm) : base(rm)
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

            FileCodeSink cppFile = new FileCodeSink(generatedPath, "wincodec_private_generated.h");;

            Helpers.Style.WriteFileHeader(cppFile);

            foreach (McgEnum e in _resourceModel.Enums)
            {
                if (_resourceModel.ShouldGenerate(CodeSections.NativeWincodecPrivate, e))
                {
                    cppFile.WriteBlock(EnumHelper.FormatNativeEnum(e));
                }
            }

            foreach (McgResource r in _resourceModel.Resources)
            {
                if (_resourceModel.ShouldGenerate(CodeSections.NativeWincodecPrivate, r))
                {
                    cppFile.WriteBlock(StructHelper.FormatNativeStructure(r));
                }
            }

            cppFile.Dispose();
        }

        #endregion Public Methods
     }
}


