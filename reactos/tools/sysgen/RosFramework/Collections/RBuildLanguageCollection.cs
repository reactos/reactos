using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildLanguageCollection : List<RBuildLanguage>
    {
        public RBuildLanguage GetByName(string culture)
        {
            foreach (RBuildLanguage language in this)
            {
                if (language.Name == culture)
                    return language;
            }

            return null;
        }
    }
}
