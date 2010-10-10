using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformname")]
    public class PlatformNameTask : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            Project.Platform.Name = Value;
        }
    }
}