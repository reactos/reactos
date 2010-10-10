using System;
using System.Globalization;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildLanguage
    {
        private string m_Name = null;
        private string m_LCID = null;

        private CultureInfo m_CultureInfo = null;

        public RBuildLanguage()
        {
        }

        public RBuildLanguage(string name)
        {
            //IsoName = name;
            Name = name;
        }

        public string LCID
        {
            get { return m_LCID; }
            set { m_LCID = value; }
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public CultureInfo CultureInfo
        {
            get { return m_CultureInfo; }
        }

        //public string IsoName
        //{
        //    get { return m_CultureInfo.Name; }
        //    set { m_CultureInfo = CultureInfo.GetCultureInfo(value); }
        //}
    }
}
