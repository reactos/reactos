using System;
using System.IO;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    public abstract class FileBaseTask : FileSystemInfoBaseTask
    {
        public FileBaseTask()
        {
        }

        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue]
        public virtual string FileName { get { return m_FileSystemInfo.Name; } set { m_FileSystemInfo.Name = value; } }

        ///// <summary>
        ///// Get the underlying <see cref="RBuildFile"/>.
        ///// </summary>
        //public RBuildPlatformFile PlatformFile
        //{
        //    get { return m_FileSystemInfo as RBuildPlatformFile; }
        //}

        //public override string BasePath
        //{
        //    get
        //    {
        //        IElement task = this;
        //        while (task != SysGen.ProjectTask)
        //        {
        //            if (task is IDirectory)
        //                return ((IDirectory)task).BasePath;

        //            task = task.Parent;
        //        }

        //        //Is in the root
        //        return string.Empty;
        //    }
        //}
    }
}
