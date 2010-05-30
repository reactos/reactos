using System;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("linkerflag")]
    public class LinkerFlagTask : ValueBaseTask
    {
        protected override void ExecuteTask() 
        {
            RBuildElement.LinkerFlags.Add(Value);
        }
    }
}
