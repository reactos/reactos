using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("kernel")]
    public class Kernel : ModuleTask
    {
        public Kernel()
        {
            Type = ModuleType.Kernel;
        }
    }
}
