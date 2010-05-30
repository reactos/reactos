using System;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class WarningReport : Backend
    {
        public WarningReport(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "Warning report"; }
        }

        protected override void Generate()
        {
            //using (StreamWriter sw = new StreamWriter(@"C:\roswarning.txt"))
            //{
            //    foreach (RBuildModule module in Project.Modules)
            //    {
            //        if (module.Unicode == false)
            //        {
            //            if ((module.Defines.ContainsKey("UNICODE")) ||
            //                (module.Defines.ContainsKey("_UNICODE")) ||
            //                (module.Defines.ContainsKey("_UNICODE_")))
            //            {
            //                sw.WriteLine("- Module '{0}' has unicode defines but 'Unicode' property set to 'False'", module.Name);
            //            }
            //        }

            //        foreach (KeyValuePair<string, string> define in Project.Defines)
            //        {
            //            if (module.Defines.ContainsKey(define.Key))
            //            {
            //                sw.WriteLine("- Module '{0}' already define '{1}' inherited from project ", module.Name, define.Key);
            //            }
            //        }

            //        foreach (string flag in Project.CompilerFlags)
            //        {
            //            if (module.CompilerFlags.Contains(flag))
            //            {
            //                sw.WriteLine("- Module '{0}' already has compiler flag '{1}' inherited from project ", module.Name, flag);
            //            }
            //        }

            //        foreach (string flag in Project.LinkerFlags)
            //        {
            //            if (module.LinkerFlags.Contains(flag))
            //            {
            //                sw.WriteLine("- Module '{0}' already has linker flag '{1}' inherited from project ", module.Name, flag);
            //            }
            //        }

            //        foreach (RBuildFolder include in module.IncludeFolders)
            //        {
            //            if (Project.IncludeFolders.Contains(include))
            //            {
            //                sw.WriteLine("- Module '{0}' already has include folder '{1}' inherited from project ", module.Name, include.RelativePath);
            //            }

            //            if (SysGen.RBuildFolderExists(include) == false)
            //            {
            //                sw.WriteLine("- Module '{0}' includes folder '{1}' which could not be found ", module.Name, include.RelativePath);
            //            }
            //        }
            //    }
            //}
        }
    }
}
