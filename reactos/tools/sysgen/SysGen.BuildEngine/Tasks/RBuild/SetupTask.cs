using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("setup")]
    public class SetupTask : PlatformFileBaseTask
    {
        [TaskAttribute("installsection", Required = true)]
        public string InstallSection { get { return Setup.InstallSection; } set { Setup.InstallSection = value; } }

        [TaskAttribute("type")]
        public SetupType SetupType { get { return Setup.SetupType; } set { Setup.SetupType = value; } }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildSetupFile();
        }

        /// <summary>
        /// Get the underlying <see cref="RBuildFile"/>.
        /// </summary>
        public RBuildSetupFile Setup
        {
            get { return m_FileSystemInfo as RBuildSetupFile; }
        }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            if (Module.IsInstallable || Module.Type == ModuleType.Package)
            {
                if (Module.Setup != null)
                    throw new BuildException("There can be only one <setup> element for a module", Location);

                Module.Setup = Setup;
            }
            else
                throw new BuildException("<setup> is not applicable for this module type", Location);
        }
    }
}