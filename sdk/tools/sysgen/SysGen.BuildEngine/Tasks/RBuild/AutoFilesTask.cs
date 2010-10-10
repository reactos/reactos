using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    public abstract class AutoFilesBaseTask : AutoResolvableFileSystemInfoBaseTask
    {
        private string m_Pattern = "*.*";

        public AutoFilesBaseTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildFolder();
        }

        public RBuildFolder Folder
        {
            get { return m_FileSystemInfo as RBuildFolder; }
        }

        [TaskAttribute("pattern")]
        public string Pattern { get { return m_Pattern; } set { m_Pattern = value; } }

        protected override void ExecuteTask()
        {
            base.ExecuteTask();

            foreach(string file in Directory.GetFiles (SysGen.ResolveRBuildFolderPath(Folder) , Pattern))
            {
                AddFile(file);
            }
        }

        protected abstract void AddFile (string file);
    }

    [TaskName("autoinstallfiles")]
    public class AutoInstallFiles : AutoFilesBaseTask
    {
        private string m_InstallBase = ".";

        [TaskAttribute("installbase")]
        public string InstallBase { get { return m_InstallBase; } set { m_InstallBase = value; } }

        protected override void AddFile(string file)
        {
            RBuildInstallFile autoFile = new RBuildInstallFile();

            autoFile.Root = Root;
            autoFile.Base = Folder.Base;
            autoFile.Name = Path.GetFileName(file);
            autoFile.InstallBase = InstallBase;

            RBuildElement.Files.Add(autoFile);
        }
    }
}