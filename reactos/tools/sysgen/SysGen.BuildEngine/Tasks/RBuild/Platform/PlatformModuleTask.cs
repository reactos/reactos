using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformmodule")]
    public class PlatformModuleTask : ValueBaseTask
    {
        public PlatformModuleTask()
        { 
        }

        protected override void ExecuteTask()
        {
            RBuildModule module = Project.Modules.GetByName(Value);

            if (module == null)
                throw new BuildException("Unknown module '{0}' referenced by <PlatformModule>", Value);

            Project.Platform.Modules.Add(module);
        }
    }
}