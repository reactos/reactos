using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("installwallpaperfile")]
    public class InstallWallPaperFileTask : PlatformFileBaseTask
    {
        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildInstallWallpaperFile();
        }
    }
}