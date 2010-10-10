using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildInstallFolderCollection : List<RBuildInstallFolder>
    {
        public RBuildInstallFolder GetByName(string name)
        {
            foreach (RBuildInstallFolder folder in this)
            {
                if (NormalizeFolderName(folder.Name) == NormalizeFolderName(name))
                    return folder;
            }

            return null;
        }

        private string NormalizeFolderName(string path)
        {
            return path.Replace(
                Path.AltDirectorySeparatorChar,
                Path.DirectorySeparatorChar);
        }
    }
}
