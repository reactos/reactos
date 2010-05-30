using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("win32cui")]
    public class Win32CUI : ModuleTask
    {
        public Win32CUI()
        {
            Type = ModuleType.Win32CUI;
        }
    }
}
