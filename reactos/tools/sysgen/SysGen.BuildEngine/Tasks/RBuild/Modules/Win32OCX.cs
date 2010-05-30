using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("win32ocx")]
    public class Win32OCX : ModuleTask
    {
        public Win32OCX()
        {
            Type = ModuleType.Win32OCX;
        }
    }
}
