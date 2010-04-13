using System;
using System.Collections.Generic;

namespace SysGen.RBuild.Framework
{
    public interface IRBuildModulesContainer
    {
        RBuildSourceFileCollection Modules { get; }
    }
}
