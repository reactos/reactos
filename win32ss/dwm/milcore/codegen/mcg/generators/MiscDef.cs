// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      MiscDef.cs
//
// Abstract:
//      Generator for miscellaneous structures and enums.
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

    public class MiscStructure : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public MiscStructure(ResourceModel rm) : base(rm)
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

            FileCodeSink cppFile = new FileCodeSink(generatedPath, "wgx_misc.h");;
            FileCodeSink csFile = new FileCodeSink(generatedPath, "wgx_misc.cs");

            m_cpp = new StringCodeSink();
            m_cs = new StringCodeSink();

            //
            // Write the definitions
            //

            WriteEnums();

            WriteStructs();

            //
            // Serialize the C++ header and the C# files for the Avalon commands:
            //

            Helpers.Style.WriteFileHeader(cppFile);
            cppFile.WriteBlock(m_cpp.ToString());

            csFile.WriteBlock(Helpers.ManagedStyle.WriteFileHeader("wgx_misc.cs"));
            csFile.WriteBlock(m_cs.ToString());


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

        private void WriteEnums()
        {
            ArrayList notInKernelEnums = new ArrayList();
            ArrayList inKernelEnums = new ArrayList();

            foreach (McgEnum e in _resourceModel.Enums)
            {
                Debug.Assert(   !_resourceModel.ShouldGenerate(CodeSections.NativeNotInKernel, e)
                             || !_resourceModel.ShouldGenerate(CodeSections.NativeIncludingKernel, e),
                             String.Format("Incompatible generation directives for '{0}'",
                                           e.UnmanagedName));

                if (_resourceModel.ShouldGenerate(CodeSections.NativeNotInKernel, e))
                {
                    notInKernelEnums.Add(e);
                }

                if (_resourceModel.ShouldGenerate(CodeSections.NativeIncludingKernel, e))
                {
                    inKernelEnums.Add(e);
                }

                if (_resourceModel.ShouldGenerate(CodeSections.Managed, e))
                {
                    m_cs.WriteBlock(EnumHelper.FormatManagedEnum(e));
                }
            }

            foreach (McgEnum e in inKernelEnums)
            {
                m_cpp.WriteBlock(EnumHelper.FormatNativeEnum(e));
            }

            m_cpp.BeginBlock(2);

            m_cpp.WriteBlock(
                    [[inline]]
                        //
                        // Some enums shouldn't be available in kernel mode, so we need to
                        // protect them by ifdef'ing them out.
                        //
                        #ifndef MILCORE_KERNEL_COMPONENT
                    [[/inline]]
                    );

            foreach (McgEnum e in notInKernelEnums)
            {
                m_cpp.WriteBlock(EnumHelper.FormatNativeEnum(e));
            }

            m_cpp.WriteBlock(
                    [[inline]]
                        #endif // MILCORE_KERNEL_COMPONENT
                    [[/inline]]
                    );

            m_cpp.EndBlock(2);
        }

        private void WriteStructs()
        {
            WriteMilMatrix3x2D();

            ArrayList notInKernelStructs = new ArrayList();
            ArrayList inKernelStructs = new ArrayList();

            foreach (McgResource resource in _resourceModel.Resources)
            {
                Debug.Assert(   !_resourceModel.ShouldGenerate(CodeSections.NativeNotInKernel, resource)
                             || !_resourceModel.ShouldGenerate(CodeSections.NativeIncludingKernel, resource),
                             String.Format("Incompatible generation directives for '{0}'",
                                           resource.UnmanagedDataType));

                if (_resourceModel.ShouldGenerate(CodeSections.NativeNotInKernel, resource))
                {
                    notInKernelStructs.Add(resource);
                }

                if (_resourceModel.ShouldGenerate(CodeSections.NativeIncludingKernel, resource))
                {
                    inKernelStructs.Add(resource);
                }

                if (_resourceModel.ShouldGenerate(CodeSections.Managed, resource))
                {
                    m_cs.WriteBlock(StructHelper.FormatManagedStructure(resource));
                }
            }

            foreach (McgResource resource in inKernelStructs)
            {
                m_cpp.WriteBlock(StructHelper.FormatNativeStructure(resource));
            }

            m_cpp.BeginBlock(2);

            m_cpp.WriteBlock(
                    [[inline]]
                        //
                        // Some structs shouldn't be available in kernel mode, so we need to
                        // protect them by ifdef'ing them out.
                        //
                        #ifndef MILCORE_KERNEL_COMPONENT
                    [[/inline]]
                    );

            foreach (McgResource resource in notInKernelStructs)
            {
                m_cpp.WriteBlock(StructHelper.FormatNativeStructure(resource));
            }

            m_cpp.WriteBlock(
                    [[inline]]
                        #endif // MILCORE_KERNEL_COMPONENT
                    [[/inline]]
                    );

            m_cpp.EndBlock(2);
        }

        private void WriteMilMatrix3x2D()
        {
            McgType milMatrix3x2DType = _resourceModel.FindType("MilMatrix3x2D");
            McgResource milMatrix3x2DResource = (McgResource)milMatrix3x2DType;

            m_cpp.WriteBlock(
                    [[inline]]
                        #ifndef _MilMatrix3x2D_DEFINED

                        [[StructHelper.FormatNativeStructure(milMatrix3x2DResource)]]

                        #define _MilMatrix3x2D_DEFINED

                        #endif // _MilMatrix3x2D_DEFINED
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

        private StringCodeSink m_cpp;
        private StringCodeSink m_cs;

        #endregion Private Fields
     }
}


