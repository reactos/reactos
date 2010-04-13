using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildFileCollection : List<RBuildFile>
    {
        public void Add(RBuildFileCollection files)
        {
            foreach (RBuildFile file in files)
            {
                Add(file);
            }
        }
    }
}
