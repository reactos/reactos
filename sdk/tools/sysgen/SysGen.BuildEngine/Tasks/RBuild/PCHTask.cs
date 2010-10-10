using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    /// <summary>
    /// PreCompiled Header task
    /// </summary>
    [TaskName("pch")]
    public class PCHTask : FileTask
    {
        /// <summary>
        /// Creates a new instance of the <see cref="PCHTask"/> class.
        /// </summary>
        public PCHTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildSourceFile();
        }

        public RBuildSourceFile SourceFile
        {
            get { return m_FileSystemInfo as RBuildSourceFile; }
        }

        protected override void ExecuteTask()
        {
            if (Module.PreCompiledHeader != null)
                throw new BuildException("Only one <pch ../> is allowed per module", Location);

            //Call the base class
            base.ExecuteTask();

            //Add the folder where the PCH is present as a include folder
            Module.IncludeFolders.Add(new RBuildFolder(PathRoot.Intermediate, Base));
        }
    }
}