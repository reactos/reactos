using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("win32dll")]
    public class Win32DLL : ModuleTask
    {
        public Win32DLL()
        {
            Type = ModuleType.Win32DLL;
        }
    }
}
