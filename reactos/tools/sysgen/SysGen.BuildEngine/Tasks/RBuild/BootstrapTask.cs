using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("bootstrap")]
    public class BootstrapTask : CDFileBaseTask
    {
        public BootstrapTask()
        {
            m_FileSystemInfo = new RBuildBootstrapFile();
        }

        private RBuildBootstrapFile BootstrapFile
        {
            get { return m_FileSystemInfo as RBuildBootstrapFile; }
        }

        protected override void ExecuteTask()
        {
            if ((Module.Type == ModuleType.Kernel) ||
                (Module.Type == ModuleType.KernelModeDLL) ||
                (Module.Type == ModuleType.KeyboardLayout) ||
                (Module.Type == ModuleType.KernelModeDriver) ||
                (Module.Type == ModuleType.NativeDLL) ||
                (Module.Type == ModuleType.NativeCUI) ||
                (Module.Type == ModuleType.Win32DLL) ||
                (Module.Type == ModuleType.Win32OCX) ||
                (Module.Type == ModuleType.Win32CUI) ||
                (Module.Type == ModuleType.Win32SCR) ||
                (Module.Type == ModuleType.Win32GUI) ||
                (Module.Type == ModuleType.BootSector) ||
                (Module.Type == ModuleType.BootLoader) ||
                (Module.Type == ModuleType.BootProgram) ||
                (Module.Type == ModuleType.Cabinet))
            {
                BootstrapFile.Element = RBuildElement;
                BootstrapFile.Name = Module.TargetName;
                BootstrapFile.Root = Module.TargetDefaultRoot;
                BootstrapFile.Base = Module.Folder.FullPath;

                Module.Bootstrap = BootstrapFile;

                //Project.Files.Add(BootstrapFile);
            }
            else
                throw new BuildException("<bootstrap> is not applicable for this module type.", Location);
        }
    }
}