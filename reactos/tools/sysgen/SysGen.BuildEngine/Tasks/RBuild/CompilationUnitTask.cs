using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("compilationunit")]
    public class CompilationUnitTask : FileSystemInfoBaseTask, ITaskContainer, IRBuildSourceFilesContainer
    {
        private TaskCollection m_ChildTasks = new TaskCollection();

        public CompilationUnitTask()
        {
            Root = PathRoot.Intermediate;
        }

        public TaskCollection ChildTasks
        {
            get { return m_ChildTasks; }
        }

        public bool ExecuteChilds
        {
            get { return true; }
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildCompilationUnitFile();
        }

        public RBuildCompilationUnitFile CompilationUnit
        {
            get { return m_FileSystemInfo as RBuildCompilationUnitFile; }
        }

        public RBuildSourceFileCollection SourceFiles
        {
            get { return CompilationUnit.SourceFiles; }
        }

        /// <summary>
        /// The name of the compilation unit to set.
        /// </summary>        
        [TaskAttribute("name")]
        public string FileName { get { return m_FileSystemInfo.Name; } set { m_FileSystemInfo.Name = value; } }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            // Add the compilation unit to the current module
            Module.CompilationUnits.Add(CompilationUnit);
        }

    }
}
