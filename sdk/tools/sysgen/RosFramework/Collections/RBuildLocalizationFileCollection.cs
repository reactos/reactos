using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildLocalizationFileCollection : List<RBuildLocalizationFile>
    {
        public bool ContainsLocalization(string culture)
        {
            return GetByName(culture) != null;
        }

        public RBuildLocalizationFile GetByName(string culture)
        {
            foreach (RBuildLocalizationFile file in this)
            {
                if (file.IsoName == culture)
                    return file;
            }

            return null;
        }
    }
}
