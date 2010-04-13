using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.BuildEngine.Backends
{
    public class MakefileWriter : StreamWriter
    {
        public MakefileWriter(string path)
            : base(path)
        {
        }

        public void WriteComment(string text)
        {
            WriteLine("# {0}" , text);
        }

        public void WriteComplexComment(string text, params object[] args)
        {
            WriteComplexComment(string.Format(text, args));
        }

        public void WriteComplexComment(string text)
        {
            WriteLine();
            WriteLine("#===================================================================================");
            WriteLine("# {0}", text);
            WriteLine("#===================================================================================");
            WriteLine();
        }

        public void WriteIndentedLine(string text , params object[] args)
        {
            WriteIndentedLine(string.Format(text, args));
            WriteLine();
        }

        public void WriteSingleLineIndented(string text)
        {
            WriteLine("\t{0}", text);
        }

        public void WriteIndentedLine(string text)
        {
            WriteLine("\t{0} \\", text);
        }

        public void WriteProperty(string propertyName , string propertyValue)
        {
            WriteLine("{0} := {1}" , 
                propertyName , 
                propertyValue);
        }

        public void WritePropertyAppend(string propertyName, string propertyValue)
        {
            WriteLine("{0} += {1}",
                propertyName,
                propertyValue);
        }

        public void WritePropertyAppendListStart(string propertyName)
        {
            WriteLine("{0} += \\", propertyName);
        }

        public void WritePropertyListStart(string propertyName)
        {
            WriteLine("{0} := \\", propertyName);
        }

        public void WritePropertyListEnd()
        {
            WriteLine();
        }

        public void WritePhonyTarget(string targetName)
        {
            WriteLine(".PHONY: {0}", targetName);
        }

        public void WriteSingleLineTarget(string targetName)
        {
            WriteLine("{0}:", targetName);
        }

        public void WriteTarget(string targetName)
        {
            WriteLine("{0}: \\", targetName);
        }

        public void WriteRule(string targetName , string targetName2)
        {
            WriteLine("{0}: {1}", targetName, targetName2);
        }
    }
}
