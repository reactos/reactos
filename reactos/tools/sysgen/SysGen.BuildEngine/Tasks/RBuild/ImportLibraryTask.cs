using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("importlibrary")]
    public class ImportLibraryTask : AutoResolvableFileSystemInfoBaseTask //FileBaseTask
    {
        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildImportLibrary();
        }

        public RBuildImportLibrary ImportLibrary
        {
            get { return m_FileSystemInfo as RBuildImportLibrary; }
        }

        /// <summary>
        /// The directory name.
        /// </summary>
        [TaskAttribute("dllname")]
        public string DllName { get { return ImportLibrary.DllName; } set { ImportLibrary.DllName = value; } }

        /// <summary>
        /// The directory name.
        /// </summary>
        [TaskAttribute("definition", Required = true)]
        public string Definition { get { return ImportLibrary.Name; } set { ImportLibrary.Name = value; } }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            //Hack::
            if (ImportLibrary.IsSpecFile)
                ImportLibrary.Root = PathRoot.Intermediate;

            if ((DllName == null) && (Module.Type == ModuleType.StaticLibrary))
                throw new BuildException("<importlibrary ../> dllname attribute is required.", Location);

            if (Module.ImportLibrary != null)
                throw new BuildException("Only one <importlibrary ../> is allowed per module.", Location);

            Module.ImportLibrary = ImportLibrary;
        }
    }
}
