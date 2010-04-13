using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformautorun")]
    public class PlatformAutorunTask : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            RBuildModule module = Project.Platform.Modules.GetByName(Value);

            if (module == null)
                throw new BuildException("Unknown module '{0}' referenced by <PlatformAutorun>", Value);

            Project.Platform.AutorunModules.Add(module);
        }
    }
}