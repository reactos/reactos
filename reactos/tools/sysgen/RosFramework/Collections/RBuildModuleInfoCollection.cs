using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildModuleInfoCollection : List<RBuildModuleInfo>
    {
        public RBuildModuleInfo GetByName(string name)
        {
            foreach (RBuildModuleInfo module in this)
            {
                if (module.Name == name)
                    return module;
            }

            return null;
        }
    }
}
