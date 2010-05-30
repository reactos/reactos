using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine
{
    public interface IDirectory
    {
        PathRoot Root { get; }
        string BasePath { get; }

        RBuildFolder Folder { get;}
    }
}
