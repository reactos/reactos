using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("file")]
    public class FileTask : FileBaseTask
    {
        public FileTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildSourceFile();
        }

        [TaskAttribute("switches")]
        public string Switches { get { return SourceFile.Switches; } set { SourceFile.Switches = value; } }

        [TaskAttribute("first")]
        [BooleanValidator]
        public bool First { get { return SourceFile.First; } set { SourceFile.First = value; } }

        /// <summary>
        /// Set the root to the same as the containing folder
        /// </summary>
        protected override void SetRootFromParent()
        {
            Root = InFolder.Root;
        }

        public RBuildSourceFile SourceFile
        {
            get { return m_FileSystemInfo as RBuildSourceFile; }
        }

        protected override void ExecuteTask()
        {
            //Call the base class
            base.ExecuteTask();

            //Add the file
            Module.SourceFiles.Add(SourceFile);
        }
    }
}