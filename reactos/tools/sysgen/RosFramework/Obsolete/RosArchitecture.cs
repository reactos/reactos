using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RosArchitecture
    {
        private string m_Name = null;
        private string m_Sub = null;
        private string m_Optimization = null;

        public RosArchitecture ()
        {
            m_Name = "i386";
            m_Sub = string.Empty;
            m_Optimization = "pentium";
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string SubArchitecture
        {
            get { return m_Sub; }
            set { m_Sub = value; }
        }

        public string Optimization
        {
            get { return m_Optimization; }
            set { m_Optimization = value; }
        }

        public string SafeName
        {
            get { return Utility.GetSafeString(m_Name).ToUpper(); }
        }
    }
}