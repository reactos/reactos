using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("buildtool")]
    public class BuildTool : ModuleTask
    {
        public BuildTool()
        {
            Type = ModuleType.BuildTool;
        }
    }
}
