using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework.NotImplementedYet
{
    public class RBuildModuleGroup
    {
        private RBuildModuleCollection m_Modules = new RBuildModuleCollection();
        private string m_Name = null;

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
