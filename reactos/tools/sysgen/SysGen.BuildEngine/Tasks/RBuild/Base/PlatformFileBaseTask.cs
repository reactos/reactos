using System;
using System.IO;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    public abstract class PlatformFileBaseTask : FileBaseTask
    {
        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue]
        public string FileName { get { return m_FileSystemInfo.Name; } set { m_FileSystemInfo.Name = value; } }

        [TaskAttribute("installbase")]
        public string InstallBase { get { return PlatformFile.InstallBase; } set { PlatformFile.InstallBase = value; } }

        /// <summary>
        /// Get the underlying <see cref="RBuildFile"/>.
        /// </summary>
        public RBuildPlatformFile PlatformFile
        {
            get { return m_FileSystemInfo as RBuildPlatformFile; }
        }

        protected override void ExecuteTask()
        {
            //Call the base class
            base.ExecuteTask();

            //Add the file
            RBuildElement.Files.Add(PlatformFile);
        }
    }
}
