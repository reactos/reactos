using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    class Utility
    {
        public static string GetSafeString (string str)
        {
            str = str.Replace(" ", string.Empty);
            str = str.Replace(".", string.Empty);

            return str;
        }
    }
}
