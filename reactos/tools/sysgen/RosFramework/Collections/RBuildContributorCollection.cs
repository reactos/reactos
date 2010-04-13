using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildContributorCollection : List<RBuildContributor>
    {
        public RBuildContributor GetByName(string alias)
        {
            foreach (RBuildContributor contributor in this)
            {
                if (contributor.Alias == alias)
                    return contributor;
            }

            return null;
        }
    }
}
