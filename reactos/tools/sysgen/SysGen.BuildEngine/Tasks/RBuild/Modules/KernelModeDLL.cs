using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("kernelmodell")]
    public class KernelModeDLL : ModuleTask
    {
        public KernelModeDLL()
        {
            Type = ModuleType.KernelModeDLL;
        }
    }
}
