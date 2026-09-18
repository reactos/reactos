// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Test file for csp.exe, for a C#' project.
//
//------------------------------------------------------------------------------

namespace MS.Internal.Csp.Test
{
    using System;
    using System.IO;

    public class CspProjectTest
    {
        //------------------------------------------------------
        //
        //  Entry Method
        //
        //------------------------------------------------------

        #region Entry Method
        public static void Main(string[] args)
        {
            if (   (args.Length != 2)
                || (args[0] != "testparam1")
                || (args[1] != "testparam2")
               )
            {
                Console.WriteLine("CspProjectTest(0) : error: Argument-passing test failed");
                return;
            }

            string sInline = [[inline]]
                                 void *pFoo = NULL;
                             [[/inline]];

            if (sInline != "void *pFoo = NULL;\n")
            {
                Console.WriteLine("CspProjectTest(0) : error: [" + "[inline]" + "] test failed");
                // The + operators in the previous line work around a limitation in the parser.
                // It doesn't exclude strings when it searches for C#' markers.
                return;
            }

            string sValue = "42";
            string sExpression = [[inline]]
                                     i = [[sValue]];
                                 [[/inline]];

             if (sExpression != "i = 42;\n")
             {
                 Console.WriteLine("CspProjectTest(0) : error: expression test failed");
                 return;
             }

            Console.WriteLine("OK");
        }
        #endregion Entry Method
    }

    public class Dummy
    {
        public static void Main(string[] args)
        {
            Console.WriteLine("CspProjectTest(0) : error: wrong Main invoked");
        }
    }
}



