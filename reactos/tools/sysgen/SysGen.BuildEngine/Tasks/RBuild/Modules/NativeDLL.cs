using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("nativedll")]
    public class NativeDLL : ModuleTask
    {
        public NativeDLL()
        {
            Type = ModuleType.NativeDLL;
        }
    }
}
