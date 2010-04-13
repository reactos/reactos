using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildFamily
    {
        private string m_FamilyName = null;

        public string Name
        {
            get { return m_FamilyName; }
            set { m_FamilyName = value; }
        }
    }
}
