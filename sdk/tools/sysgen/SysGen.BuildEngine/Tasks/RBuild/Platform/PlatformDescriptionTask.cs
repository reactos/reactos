using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformdescription")]
    public class PlatformDescriptionTask : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            Project.Platform.Description = Value;
        }
    }
}