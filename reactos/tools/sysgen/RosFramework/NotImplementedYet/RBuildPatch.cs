using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework.NotImplementedYet
{
    public class RBuildPatch
    {
        private string m_Filename = null;
        private string m_Name = null;

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string FileName
        {
            get { return m_Filename; }
            set { m_Filename = value; }
        }
    }
}
