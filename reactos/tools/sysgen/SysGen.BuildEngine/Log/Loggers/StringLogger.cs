using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace SysGen.BuildEngine.Log
{
    /// <summary>
    /// Used for test classes to check output.
    /// </summary>
    public class StringLogger : LogListener
    {
        private StringWriter _writer = new StringWriter();

        public override void Write(string message)
        {
            _writer.Write(message);
        }

        public override void WriteLine(string message)
        {
            _writer.WriteLine(message);
        }

        /// <summary>
        /// Returns the contents of log captured.
        /// </summary>
        public override string ToString()
        {
            return _writer.ToString();
        }
    }
}