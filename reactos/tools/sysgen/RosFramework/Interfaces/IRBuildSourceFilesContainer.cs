using System;
using System.Collections.Generic;

namespace SysGen.RBuild.Framework
{
    public interface IRBuildSourceFilesContainer
    {
        RBuildSourceFileCollection SourceFiles { get; }
    }
}
