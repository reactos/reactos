using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    public abstract class CDFileBaseTask : FileBaseTask //PlatformFileBaseTask
    {
        public CDFileBaseTask()
        {
        }

        [TaskAttribute("installbase")]
        public string InstallBase { get { return CDFile.InstallBase; } set { CDFile.InstallBase = value; } }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildCDFile();
        }

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("nameoncd")]
        public string NameOnCD { get { return CDFile.NewName; } set { CDFile.NewName = value; } }

        private RBuildCDFileBase CDFile
        {
            get { return m_FileSystemInfo as RBuildCDFileBase; }
        }

        protected override void ExecuteTask()
        {
            //Call the base class
            base.ExecuteTask();

            //Add the file
            RBuildElement.Files.Add(CDFile);
        }
    }
}