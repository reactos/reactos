// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Base class for the "meta-code" files. Holds commonly-used static
//              methods.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.Text;

    public abstract class GeneratorMethods
    {
        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        /// <summary>
        /// Convert the given string to all caps.
        /// </summary>
        public static string AllCaps(string stringIn)
        {
            return stringIn.ToUpper();
        }

        /// <summary>
        /// Convert the first character to a capital.
        /// </summary>
        public static string FirstCap(string stringIn)
        {
            StringBuilder sb = new StringBuilder(stringIn);
            sb[0] = Char.ToUpper(sb[0]);
            return sb.ToString();
        }

        /// <summary>
        /// Convert the given string to an initial capital, and the remainder lower case.
        /// </summary>
        public static string EnsureFirstCap(string stringIn)
        {
            return FirstCap(stringIn.ToLower());
        }

        /// <summary>
        /// Convert the first character to a lower-case.
        /// </summary>
        public static string FirstLower(string stringIn)
        {
            StringBuilder sb = new StringBuilder(stringIn);
            sb[0] = Char.ToLower(sb[0]);
            return sb.ToString();
        }

        /// <summary>
        /// Indent each line with the given number of spaces.
        ///
        /// Does not trim blank lines. (That job is up to FileCodeSink,
        /// since it's best done when the entire line is assembled).
        /// </summary>
        public static string IndentEachLine(string s, int iLevel)
        {
            StringBuilder result = new StringBuilder();
            bool fFirst = true;

            foreach (string line in s.Split('\n'))
            {
                if (!fFirst)
                {
                    result.Append("\n");
                }

                if (iLevel > 0)
                {
                    result.Append(' ', iLevel);
                }
                result.Append(line);

                fFirst = false;
            }

            return result.ToString();
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
    }
}





