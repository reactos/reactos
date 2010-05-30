using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("package")]
    public class Package : ModuleTask
    {
        public Package()
        {
            Type = ModuleType.Package;
        }
    }
}
