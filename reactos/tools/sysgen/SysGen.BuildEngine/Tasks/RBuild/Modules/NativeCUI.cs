using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("nativecui")]
    public class NativeCUI : ModuleTask
    {
        public NativeCUI()
        {
            Type = ModuleType.NativeCUI;
        }
    }
}
