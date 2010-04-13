using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("linkerscript")]
    public class LinkerScriptTask : FileBaseTask
    {
        public LinkerScriptTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildFile();
        }

        public RBuildFile ScriptFile
        {
            get { return m_FileSystemInfo as RBuildFile; }
        }

        protected override void ExecuteTask() 
        {
            //Call the base class
            base.ExecuteTask();

            if (Module.LinkerScript != null)
                throw new BuildException("Only one <linkerscript ../> is allowed per module", Location);

            Module.LinkerScript = ScriptFile;
        }
    }
}
