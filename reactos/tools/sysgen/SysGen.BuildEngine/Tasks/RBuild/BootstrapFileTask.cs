using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("bootstrapfile")]
    public class BootstrapFileTask : CDFileBaseTask //FileBaseTask ///PlatformFileBaseTask
    {
        public BootstrapFileTask()
        {
        }

        [TaskValue]
        public string FileName { get { return m_FileSystemInfo.Name; } set { m_FileSystemInfo.Name = value; } }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildBootstrapFile();
        }

        public RBuildBootstrapFile BootStrapFile
        {
            get { return m_FileSystemInfo as RBuildBootstrapFile; }
        }

        protected override bool TryToResolveBasePath()
        {
            Base = "i386";
            return true;
        }

        protected override void ExecuteTask() 
        {
            //Call the base class
            base.ExecuteTask();
        }
    }
}
