using System;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("compilerflag")]
    public class CompilerFlagTask : ValueBaseTask
    {
        protected override void ExecuteTask() 
        {
            RBuildElement.CompilerFlags.Add(Value);
        }
    }
}
