using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildAPIInfo
    {
        private string m_Name;
        private string m_File;
        private bool m_Implemented;

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string File
        {
            get { return m_File; }
            set { m_File = value; }
        }

        public bool Implemented
        {
            get { return m_Implemented; }
            set { m_Implemented = value; }
        }

        public string HtmlDocFileName
        {
            get { return string.Format("{0}.htm", Name); }
        }
    }
}
