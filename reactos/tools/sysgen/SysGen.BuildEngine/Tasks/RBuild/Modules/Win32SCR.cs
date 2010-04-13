using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("win32scr")]
    public class Win32SCR : ModuleTask
    {
        public Win32SCR()
        {
            Type = ModuleType.Win32SCR;
        }
    }
}
