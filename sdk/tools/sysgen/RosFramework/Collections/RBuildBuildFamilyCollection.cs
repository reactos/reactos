using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildBuildFamilyCollection : List<RBuildBuildFamily>
    {
        public RBuildBuildFamily GetByName(string name)
        {
            foreach (RBuildBuildFamily family in this)
            {
                if (family.Name == name)
                    return family;
            }

            return null;
        }
    }
}
