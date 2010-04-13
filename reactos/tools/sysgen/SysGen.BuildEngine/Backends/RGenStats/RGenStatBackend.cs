using System;
using System.Text.RegularExpressions;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine.Backends
{
    public class RGenStatBackend : Backend
    {
        public RGenStatBackend(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "RGenStat Report"; }
        }

        protected override void Generate()
        {
            using (StreamWriter sw = new StreamWriter(Directory.GetCurrentDirectory() + "\\apistatus.lst"))
            {
                sw.WriteLine("; Format:");
                sw.WriteLine(";     COMPONENT_NAME PATH_TO_COMPONENT_SOURCES");
                sw.WriteLine();
                sw.WriteLine("; Where:");
                sw.WriteLine(";     COMPONENT_NAME            - Name of the module. Eg. kernel32.");
                sw.WriteLine(";     PATH_TO_COMPONENT_SOURCES - Relative path to sources (relative to where rgenstat is run from).");
                sw.WriteLine();

                foreach (RBuildModule module in Project.Modules)
                {
                    if (module.Type == ModuleType.Kernel ||
                        module.Type == ModuleType.KernelModeDLL ||
                        module.Type == ModuleType.KernelModeDriver ||
                        module.Type == ModuleType.StaticLibrary ||
                        module.Type == ModuleType.ObjectLibrary ||
                        module.Type == ModuleType.Win32DLL ||
                        module.Type == ModuleType.Win32OCX ||
                        module.Type == ModuleType.KeyboardLayout)
                    {
                        sw.WriteLine("{0} {1}", 
                            module.Name,
                            module.BaseURI.ToString().Replace("\\", "/"));
                    }
                }
            }

            //using (StreamWriter sw = new StreamWriter(Directory.GetCurrentDirectory() + "\\descriptions.rbuild"))
            //{
            //    sw.WriteLine("<!--");
            //    sw.WriteLine("This file contains human friendly module descriptions.");
            //    sw.WriteLine("");
            //    sw.WriteLine("Module descriptions will be used when generating reports and for REACTOS_STR_FILE_DESCRIPTION once the");
            //    sw.WriteLine("rbuild branch is merged. It will also allow in the future localized REACTOS_STR_FILE_DESCRIPTION versions ");
            //    sw.WriteLine("in localized builds.");
            //    sw.WriteLine("-->");
            //    sw.WriteLine("<?xml version=\"1.0\"?>");
            //    sw.WriteLine("<!DOCTYPE group SYSTEM \"tools/rbuild/project.dtd\">");
            //    sw.WriteLine("<group>");

            //    foreach (RBuildModule module in Project.Modules)
            //    {
            //        if (module.Type == ModuleType.Kernel ||
            //            module.Type == ModuleType.KernelModeDLL ||
            //            module.Type == ModuleType.KernelModeDriver ||
            //            module.Type == ModuleType.BootLoader ||
            //            module.Type == ModuleType.BootProgram ||
            //            module.Type == ModuleType.BootSector ||
            //            module.Type == ModuleType.BuildTool ||
            //            module.Type == ModuleType.Cabinet ||
            //            module.Type == ModuleType.EmbeddedTypeLib ||
            //            module.Type == ModuleType.HostStaticLibrary ||
            //            module.Type == ModuleType.IdlHeader ||
            //            module.Type == ModuleType.NativeCUI ||
            //            module.Type == ModuleType.NativeDLL ||
            //            module.Type == ModuleType.ObjectLibrary ||
            //            module.Type == ModuleType.Package ||
            //            module.Type == ModuleType.RpcClient ||
            //            module.Type == ModuleType.RpcProxy ||
            //            module.Type == ModuleType.RpcServer ||
            //            module.Type == ModuleType.StaticLibrary ||
            //            module.Type == ModuleType.Win32CUI ||
            //            module.Type == ModuleType.Win32DLL ||
            //            module.Type == ModuleType.Win32GUI ||
            //            module.Type == ModuleType.Win32OCX ||
            //            module.Type == ModuleType.Win32SCR ||
            //            module.Type == ModuleType.KeyboardLayout)

            //        {
            //            sw.WriteLine("<property name=\"DESC_{0}\" value =\"{1}\" internal=\"true\" />",
            //                module.Name.ToUpper(),
            //                module.Name);
            //        }
            //    }
                
            //    sw.WriteLine("</group>");
            //}
        }
    }
}
