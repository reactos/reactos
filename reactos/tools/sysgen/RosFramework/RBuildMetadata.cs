using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildMetadata
    {
        private string m_Description = null;

        public string Description
        {
            get { return m_Description; }
            set { m_Description = value; }
        }
    }
}