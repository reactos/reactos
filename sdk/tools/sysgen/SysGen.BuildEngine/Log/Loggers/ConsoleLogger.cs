using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.BuildEngine.Log
{
    /// <summary>
    /// The standard logger that will suffice for any command line based nant runner.
    /// </summary>
    public class ConsoleLogger : LogListener
    {
        public override void Write(string message)
        {
            Console.Write(message);
        }

        public override void WriteLine(string message)
        {
            Console.WriteLine(message);
        }
    }
}
