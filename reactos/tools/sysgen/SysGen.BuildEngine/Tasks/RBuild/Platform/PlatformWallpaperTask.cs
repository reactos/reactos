using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformwallpaper")]
    public class PlatformWallpaperTask : ValueBaseTask
    {
        protected override void PostExecuteTask()
        {
            if (Project.Platform.Wallpaper != null)
                throw new BuildException("Only one wallpaper can be set per platform");

            foreach (RBuildModule module in Project.Modules)
            {
                foreach (RBuildFile file in module.Files)
                {
                    RBuildWallpaperFile wallpaper = file as RBuildWallpaperFile;

                    if (wallpaper != null)
                    {
                        if (wallpaper.ID.ToLower() == Value.ToLower())
                        {
                            /* set the shell to use */
                            Project.Platform.Wallpaper = wallpaper;
                        }
                    }
                }
            }

            if (Project.Platform.Wallpaper == null)
                throw new BuildException("Unknown wallpaper '{0}' referenced by <PlatformWallpaper>", Value);
        }
    }
}