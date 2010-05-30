using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildModuleInfo
    {
        private string m_Name = string.Empty;

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }
        private string m_Base = string.Empty;

        public string Base
        {
            get { return m_Base; }
            set { m_Base = value; }
        }
        private string m_CatalogPath = string.Empty;

        public string CatalogPath
        {
            get { return m_CatalogPath; }
            set { m_CatalogPath = value; }
        }
        private ModuleType m_Type = ModuleType.BuildTool;

        public ModuleType Type
        {
            get { return m_Type; }
            set { m_Type = value; }
        }

        List<string> m_Libraries = new List<string>();
        List<string> m_Dependencies = new List<string>();
        List<string> m_Requirements = new List<string>();

        public List<string> Libraries
        {
            get { return m_Libraries; }
            set { m_Libraries = value; }
        }

        public List<string> Dependencies
        {
            get { return m_Dependencies; }
            set { m_Dependencies = value; }
        }
        
        public List<string> Requirements
        {
            get { return m_Requirements; }
            set { m_Requirements = value; }
        }
    }
}
