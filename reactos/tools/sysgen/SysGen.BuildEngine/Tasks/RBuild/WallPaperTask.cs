using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("wallpaper")]
    public class WallPaperTask : PlatformFileBaseTask
    {
        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildWallpaperFile();
        }

        protected override void ExecuteTask()
        {
            if (Module != null)
            {
                base.ExecuteTask();
            }
            else
                throw new BuildException("<wallpaper> is only applicable for modules", Location);
        }
    }
}