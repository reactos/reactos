using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformshell")]
    public class PlatformShellTask : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            RBuildModule module = Project.Modules.GetByName(Value);

            if (module == null)
                throw new BuildException("Unknown module '{0}' referenced by <PlatformShell>", Value);

            if (module.Type != ModuleType.Win32CUI &&
                module.Type != ModuleType.Win32GUI)
                throw new BuildException("Shell can only be of type win32gui");

            if (Project.Platform.Shell != null)
                throw new BuildException("Only one shell can be set per platform");

            /* set the shell to use */
            Project.Platform.Shell = module;
        }
    }
}