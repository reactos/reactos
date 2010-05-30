using System;
using System.Globalization;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildLocalizationFile : RBuildFile
    {
        private bool m_Dirty = false;
        private CultureInfo m_CultureInfo = null;

        public RBuildLocalizationFile()
        {
        }

        public CultureInfo CultureInfo
        {
            get { return m_CultureInfo; }
        }

        public bool Dirty
        {
            get { return m_Dirty; }
            set { m_Dirty = value; }
        }

        public string IsoName
        {
            get { return m_CultureInfo.Name; }
            set { m_CultureInfo = CultureInfo.GetCultureInfo(value); }
        }
    }
}