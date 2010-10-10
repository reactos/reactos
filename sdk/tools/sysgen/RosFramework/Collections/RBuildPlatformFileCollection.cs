using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildPlatformFileCollection : List<RBuildOutputFile>
    {
        public void Add(RBuildPlatformFileCollection files)
        {
            foreach (RBuildPlatformFile file in files)
            {
                Add(file);
            }
        }
    }
}
