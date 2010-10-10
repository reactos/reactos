using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RosPlatform
    {
        private List<RBuildModule> m_Modules = new List<RBuildModule>();
        
        private string m_Name = null;
        private string m_Base = null;

        private RosPlatform m_ParentPlatform = null;

        public RosPlatform()
        {
            m_Name = "ReactOS Core";
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string Base
        {
            get { return m_Base; }
            set { m_Base = value; }
        }

        public RosPlatform ParentPlatform
        {
            get { return m_ParentPlatform; }
            set { m_ParentPlatform = value; }
        }

        public string SafeName
        {
            get { return Utility.GetSafeString(m_Name); }
        }

        public void SaveAs(string file)
        {

        }

        public List<RBuildModule> Modules
        {
            get { return m_Modules; }
        }

        public List<RBuildModule> ParentModules
        {
            get
            {
                List<RBuildModule> modules = new List<RBuildModule>();

                if (ParentPlatform != null)
                {
                    foreach (RBuildModule module in ParentPlatform.Modules)
                    {
                        modules.Add(module);
                    }

                    foreach (RBuildModule module in ParentPlatform.ParentModules)
                    {
                        modules.Add(module);
                    }
                }

                return modules;
            }
        }
    }
}
