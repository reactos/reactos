using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("staticlibrary")]
    public class StaticLibrary : ModuleTask
    {
        public StaticLibrary()
        {
            Type = ModuleType.StaticLibrary;
        }
    }
}
