using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildModuleGroup
    {
        private string m_Name = null;
        private RBuildModuleCollection m_Modules = new RBuildModuleCollection();

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public RBuildModuleCollection Modules
        {
            get { return m_Modules; }
        }
    }
}
