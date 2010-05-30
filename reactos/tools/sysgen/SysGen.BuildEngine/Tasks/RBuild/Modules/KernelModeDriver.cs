using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("kernelmodedriver")]
    public class KernelModeDriver : ModuleTask
    {
        public KernelModeDriver()
        {
            Type = ModuleType.KernelModeDriver;
        }
    }
}
