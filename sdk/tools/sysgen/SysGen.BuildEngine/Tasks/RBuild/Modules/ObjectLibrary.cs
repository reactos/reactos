using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("objectlibrary")]
    public class ObjectLibrary : ModuleTask
    {
        public ObjectLibrary()
        {
            Type = ModuleType.ObjectLibrary;
        }
    }
}
