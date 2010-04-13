using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("installfile")]
    public class InstallFileTask : PlatformFileBaseTask
    {
        public InstallFileTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildInstallFile();
        }

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("newname")]
        public string NewName { get { return InstallFile.NewName; } set { InstallFile.NewName = value; } }

        private RBuildInstallFile InstallFile
        {
            get { return m_FileSystemInfo as RBuildInstallFile; }
        }
    }
}