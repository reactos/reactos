using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildInstallFolder : RBuildFolder
    {
        private string m_ID = null;

        public RBuildInstallFolder()
        {
            Root = PathRoot.Install;
        }

        public RBuildInstallFolder(string id, string name)
        {
            ID = id;
            Name = name;
            Root = PathRoot.Install;
        }

        public string ID
        {
            get { return m_ID; }
            set { m_ID = value; }
        }
    }
}
