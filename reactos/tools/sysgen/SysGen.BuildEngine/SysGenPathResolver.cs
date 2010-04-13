using System;
using System.Collections.Generic;
using System.Text;

//using SysGen.RBuild.Framework;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Tasks;

namespace SysGen.BuildEngine
{
    class SysGenPathResolver
    {
        //public static string GetPath(Task current)
        //{
        //    return GetPath(current, SysGen.ProjectTask);
        //}

        public static string GetPath(Task current, Task root)
        {
            IElement task = current.Parent;
            while (task != root)
            {
                DirectoryTask directory = task as DirectoryTask;

                if (directory != null)
                    return directory.Folder.FullPath;

                task = task.Parent;
            }

            return string.Empty;
        }
    }
}
