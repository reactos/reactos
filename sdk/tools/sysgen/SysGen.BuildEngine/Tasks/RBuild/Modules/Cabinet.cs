using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("cabinet")]
    public class Cabinet : ModuleTask
    {
        public Cabinet()
        {
            Type = ModuleType.Cabinet;
        }
    }
}
