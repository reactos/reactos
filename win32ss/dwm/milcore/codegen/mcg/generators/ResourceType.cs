// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      ResourceType.cs
//
// Abstract: 
//      Generator for resource type enumerations.
// 
// 
// Description:
//      This generator builds the MIL_RESOURCE_TYPE enumeration 
//      (wgx_resource_types.h and wgx_resource_types.cs).
//
// 
// Example output fragment for wgx_resource_types.h:
// 
//      enum MIL_RESOURCE_TYPE {
//          ...
//          TYPE_BITMAP = 3,
//          ...
//      };
// 
// The C# enumeration output is analogous.
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

    public class ResourceType : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ResourceType(ResourceModel rm) : base(rm) 
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

            FileCodeSink ccFile = 
                new FileCodeSink(generatedPath, "wgx_resource_types.h");

            FileCodeSink csFile =
                new FileCodeSink(generatedPath, "wgx_resource_types.cs");

            m_enum = new StringCodeSink();


            //
            // Collect the resource types...
            //

            m_resourceIndex = 1;

            foreach (McgResource resource in _resourceModel.Resources) 
            {
                if (resource.IsValueType || !resource.HasUnmanagedResource)
                {
                    continue;
                }

                EmitTypeOfResource(resource);
            }


            //
            // Serialize the C++ header and the C# files:
            //

            Helpers.Style.WriteFileHeader(ccFile);

            ccFile.WriteBlock(
                [[inline]]
                    //
                    // The MILCE resource type enumeration.
                    //
                    
                    enum MIL_RESOURCE_TYPE
                    {
                        /* 0x00 */ TYPE_NULL = 0,
                        [[m_enum.ToString()]]
                        /* 0x[[m_resourceIndex.ToString("x02")]] */ TYPE_LAST = [[m_resourceIndex]],
                        /* ---- */ TYPE_FORCE_DWORD = 0xFFFFFFFF
                    };
                [[/inline]]);

            csFile.WriteBlock(
                Helpers.ManagedStyle.WriteFileHeader("wgx_resource_types.cs")
                );

            csFile.WriteBlock(
                [[inline]]
                        internal partial class DUCE
                        {
                            //
                            // The MILCE resource type enumeration.
                            //
                            
                            internal enum ResourceType
                            {
                                /* 0x00 */ TYPE_NULL = 0,
                                [[m_enum.ToString()]]
                            };
                        }
                [[/inline]]
                ); /* extra indentation for milcoretypes.w */
            
            ccFile.Dispose();
            csFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private void EmitTypeOfResource(McgResource resource)
        {
            //
            // Emit a part of the enumeration...
            //

            m_enum.Write(
                [[inline]]
                    /* 0x[[m_resourceIndex.ToString("x02")]] */ TYPE_[[resource.Name.ToUpper()]] = [[m_resourceIndex.ToString()]],
                [[/inline]]
                );

            ++m_resourceIndex;
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private StringCodeSink m_enum;
        private int m_resourceIndex;

        #endregion Private Fields
    }
}



