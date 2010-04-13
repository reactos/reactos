using System;
using System.Collections.Generic;
using System.Text;

using SysGen.BuildEngine;
using SysGen.BuildEngine.Framework;

namespace SysGen.Make
{
    class Program
    {
        static void Main(string[] args)
        {
            SysGenEngine engine = new SysGenEngine(@"C:\ros\trunk\reactos\ReactOS-i386.rbuild");

            engine.ReadBuildFiles();

            /*
            Console.WriteLine("Generates project files for buildsystems\n\n");
            Console.WriteLine("  rbuild [switches] -r{rootfile.rbuild} buildsystem\n\n");
            Console.WriteLine("Switches:\n");
            Console.WriteLine("  -v            Be verbose.\n");
            Console.WriteLine("  -c            Clean as you go. Delete generated files as soon as they are not\n");
            Console.WriteLine("                needed anymore.\n");
            Console.WriteLine("  -dd           Disable automatic dependencies.\n");
            Console.WriteLine("  -dm{module}   Check only automatic dependencies for this module.\n");
            Console.WriteLine("  -ud           Disable multiple source files per compilation unit.\n");
            Console.WriteLine("  -mi           Let make handle creation of install directories. Rbuild will\n");
            Console.WriteLine("                not generate the directories.\n");
            Console.WriteLine("  -ps           Generate proxy makefiles in source tree instead of the output.\n");
            Console.WriteLine("                tree.\n");
            Console.WriteLine("  -vs{version}  Version of MS VS project files. Default is %s.\n", MS_VS_DEF_VERSION);
            Console.WriteLine("  -vo{version|configuration} Adds subdirectory path to the default Intermediate-Outputdirectory.\n");
            Console.WriteLine("  -Dvar=val     Set the value of 'var' variable to 'val'.\n");
            Console.WriteLine("\n");
            Console.WriteLine("  buildsystem   Target build system. Can be one of:\n");
            */

            Console.ReadLine();
        }
    }
}
