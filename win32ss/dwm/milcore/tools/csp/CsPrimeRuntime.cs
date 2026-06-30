// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Methods needed to implement C#'. That is, they are used by the
//              C# code that CsPrimeParser produces.
//
// Notes: Named "CsPrimeRuntime" instead of just "Runtime" to make it clear that
//        it's for the implementation of C#' itself - it's not a runtime
//        provided to the project that Csp.exe will execute.
//

namespace MS.Internal.Csp
{
    using System;
    using System.Text;

    public sealed class CsPrimeRuntime
    {
        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods
        /// <summary>
        /// Convert and apply indentation
        ///
        /// The parser generates references to this method for expressions embedded
        /// in "inline" blocks.
        ///
        /// Convert the object to a string.
        /// Remove the trailing newline, if any.
        /// If the result has multiple lines, indent all lines (except the first)
        /// with the given number of spaces.
        ///
        /// </summary>
        public static string ConvertAndIndent(object o, int iLevel)
        {
            // Convert to a string

            string sText = o.ToString();

            // Handle the empty case

            if (sText.Length == 0)
            {
                return "";
            }

            // Remove the trailing newline, if any.

            if (sText[sText.Length-1] == '\n')
            {
                sText = sText.Remove(sText.Length-1, 1);
            }

            return IndentEachLineExceptFirst(sText, iLevel);
        }
        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods
        /// <summary>
        /// Indent each line (except the first line) by the given number of spaces.
        ///
        /// e.g. IndentEachLineExceptFirst("foo\nbar\nbaz", 2)
        ///      will return "foo\n  bar\n  baz".
        /// </summary>
        private static string IndentEachLineExceptFirst(
            string s, 
            int iLevel
            )
        {
            StringBuilder result = new StringBuilder();
            bool fFirst = true;

            // For each line (and remove newline characters)
            foreach (string line in s.Split('\n'))
            {
                // Append the newline for the previous line processed
                if (!fFirst)
                {
                    result.Append("\n");
                }

                // Append indentation for the current line
                if (   (iLevel > 0)
                    && (!fFirst))
                {
                    result.Append(' ', iLevel);
                }

                // Append the rest of the current line
                result.Append(line);

                fFirst = false;
            }

            return result.ToString();
        }
        #endregion Private Methods
    }
}


