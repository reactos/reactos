// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Collection of helper methods for generating managed code that
//              conforms to the WCP managed coding guidelines.
//

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Collections;

    using MS.Internal.MilCodeGen.Runtime;

    public class ManagedStyle : GeneratorMethods
    {
        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        /// <summary>
        /// WriteFileHeader - Emits the managed file header placing the filename into the text.
        /// </summary>
        /// <param name="filename"> string - the name of the generated file. </param>
        public static string WriteFileHeader(string filename)
        {
            return WriteFileHeader(filename, null);
        }

        /// <summary>
        /// WriteFileHeader - Emits the managed file header placing the filename and, 
        /// optionally, the codegen source file into the text.
        /// </summary>
        /// <param name="filename"> string - the name of the generated file. </param>
        /// <param name="source"> string - (optional) This is the name of the file in codegen
        /// a reader should start with to understand the file being generated, make updates, etc. </param>
        public static string WriteFileHeader(string filename, string source)
        {
            return
                [[inline]]
                    //---------------------------------------------------------------------------
                    //
                    // Licensed to the .NET Foundation under one or more agreements.
                    // The .NET Foundation licenses this file to you under the MIT license.
                    //
                    // This file was generated, please do not edit it directly.[[conditional((source != null) && (source.Length > 0))]]
                    // 
                    // This file was generated from the codegen template located at:
                    //     [[source]][[/conditional]]
                    //
                    //
                    //---------------------------------------------------------------------------
                [[/inline]];
        }

        public static string WriteSection(string section)
        {
            return
                [[inline]]
                    //------------------------------------------------------
                    //
                    //  [[section]]
                    //
                    //------------------------------------------------------
                [[/inline]];
        }

        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

    }
}




