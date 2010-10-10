using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildFolderCollection : List<RBuildFolder>
    {
        public void Add(RBuildFolderCollection folders)
        {
            foreach (RBuildFolder folder in folders)
            {
                Add(folder);
            }
        }
    }
}
