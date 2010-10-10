using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public interface IRBuildInstallable
    {
        string InstallBase { get; set; }

        RBuildInstallFolder InstallFolder { get; set; }
    }
}
