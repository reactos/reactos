using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("installcomponent")]
    public class InstallComponent : FileBaseTask
    {
        public InstallComponent()
        {
        }

        [TaskAttribute("section")]
        public string InstallSection { get { return InstallComponentFile.InstallSection; } set { InstallComponentFile.InstallSection = value; } }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildInfInstallerFile();
        }

        public RBuildInfInstallerFile InstallComponentFile
        {
            get { return m_FileSystemInfo as RBuildInfInstallerFile; }
        }

        protected override void ExecuteTask()
        {
            //Call the base class
            base.ExecuteTask();

            if (Module.LinkerScript != null)
                throw new BuildException("Only one <installcomponent ../> is allowed per module", Location);

            //Module.InfInstall = InstallComponentFile;
        }
    }
}