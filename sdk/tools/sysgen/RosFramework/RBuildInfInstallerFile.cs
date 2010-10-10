using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildInfInstallerFile : RBuildFile
    {
        private string m_InstallSection = "DefaultInstall";

        public string InstallSection
        {
            get { return m_InstallSection; }
            set { m_InstallSection = value; }
        }
    }
}