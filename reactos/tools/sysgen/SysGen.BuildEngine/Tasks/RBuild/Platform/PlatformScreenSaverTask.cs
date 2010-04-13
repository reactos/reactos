using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformscreensaver")]
    public class PlatformScreenSaverTask : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            RBuildModule module = Project.Modules.GetByName(Value);

            if (module == null)
                throw new BuildException("Unknown module '{0}' referenced by <PlatformScreenSaver>", Value);

            if (module.Type != ModuleType.Win32SCR)
                throw new BuildException("Shell can only be of type win32scr");

            if (Project.Platform.Screensaver != null)
                throw new BuildException("Only one screensaver can be set per platform");

            /* set the shell to use */
            Project.Platform.Screensaver = module;
        }
    }
}