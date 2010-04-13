using System;
using System.IO;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("include")]
    public class IncludeTask : AutoResolvableFileSystemInfoBaseTask //FileSystemInfoBaseTask
    {
        public IncludeTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildFolder();
        }

        [TaskValue]
        public virtual string IncludePath 
        {
            get { return m_FileSystemInfo.Name; } 
            set { m_FileSystemInfo.Name = value; } 
        }

        public RBuildFolder IncludeFolder
        {
            get { return m_FileSystemInfo as RBuildFolder; }
        }

        protected override void ExecuteTask() 
        {
            //Call the base class
            base.ExecuteTask();

            //Add include folder...
            RBuildElement.IncludeFolders.Add(IncludeFolder);
        }
    }
}
