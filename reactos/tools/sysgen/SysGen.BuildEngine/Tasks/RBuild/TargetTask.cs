using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("target")]
    public class TargetTask : Task
    {
        protected RBuildTarget m_Target = new RBuildTarget();

        protected override void ExecuteTask()
        {
            SysGen.Targets.Add(m_Target);
        }
    }
}