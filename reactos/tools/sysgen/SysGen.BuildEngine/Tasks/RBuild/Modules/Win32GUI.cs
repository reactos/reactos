using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("win32gui")]
    public class Win32GUI : ModuleTask
    {
        public Win32GUI()
        {
            Type = ModuleType.Win32GUI;
        }
    }
}
