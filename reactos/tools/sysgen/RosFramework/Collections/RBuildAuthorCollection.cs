using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildAuthorCollection : List<RBuildAuthor>
    {
        public RBuildAuthor GetByName(string alias)
        {
            foreach (RBuildAuthor author in this)
            {
                if (author.Contributor.Alias == alias)
                    return author;
            }

            return null;
        }
    }
}
