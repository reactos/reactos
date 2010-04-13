using System.IO;
using System.Collections.Generic;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("directory")]
    public class DirectoryTask : FileSystemInfoBaseTask, ITaskContainer//, IDirectory
    {
        private TaskCollection m_ChildTasks = new TaskCollection();

        public DirectoryTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildFolder();
        }

        public TaskCollection ChildTasks
        {
            get { return m_ChildTasks; }
        }

        public bool ExecuteChilds
        {
            get { return true; }
        }

        /// <summary>
        /// The directory name.
        /// </summary>
        [TaskAttribute("name", Required = true)]
        public virtual string Name { get { return m_FileSystemInfo.Name; } set { m_FileSystemInfo.Name = value; } }

        public RBuildFolder Folder
        {
            get { return m_FileSystemInfo as RBuildFolder; }
        }

        protected override void OnInit()
        {
            base.OnInit();

            Base = SysGenPathResolver.GetPath(this, SysGen.RootTask);
        }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            if (RBuildElement.Folders.Contains(Folder) == false)
                RBuildElement.Folders.Add(Folder);
        }
    }
}