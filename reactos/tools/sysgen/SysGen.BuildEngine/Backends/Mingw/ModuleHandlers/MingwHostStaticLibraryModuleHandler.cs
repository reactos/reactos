using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwHostStaticLibraryModuleHandler : MingwStaticLibraryModuleHandler
    {
        public MingwHostStaticLibraryModuleHandler(RBuildModule module)
            : base(module)
        
        {
        }
    }
}
