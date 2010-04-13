using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.BuildEngine
{
    class SysGenConversion
    {
        public static bool ToBolean(object value)
        {
            switch (value.ToString().ToLower())
            {
                case "yes":
                case "true":
                case "1":
                    return true;
                case "no":
                case "false":
                case "0":
                    return false;
            }

            throw new ValidationException(String.Format("Cannot resolve to '{0}' to Boolean value.", value.ToString()));
        }
    }
}
