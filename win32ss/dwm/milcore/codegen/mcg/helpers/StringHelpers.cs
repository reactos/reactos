// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Helper methods for code gen
//
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class StringHelpers
    {
        internal static string FilterString(string text, char filter)
        {
            StringBuilder sb = new StringBuilder(text.Length);

            foreach(char c in text)
            {
                if (c != filter)
                    sb.Append(c);
            }

            return sb.ToString();
        }
    }
}



