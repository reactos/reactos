using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildInstallFile : RBuildPlatformFile
    {
    }

    public class RBuildWallpaperFile : RBuildInstallFile
    {
        private string m_ID = null;

        public RBuildWallpaperFile()
        { 
        }

        public RBuildWallpaperFile(string file)
        {
            Name = file;
        }

        public string ID
        {
            get 
            {
                if (m_ID == null ||
                    m_ID == string.Empty)
                    return base.Name;

                return m_ID; 
            }
            set { m_ID = value; }
        }
    }
}