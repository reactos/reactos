using System;
using System.Text.RegularExpressions;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildContributor
    {
        private string m_FirstName = null;
        private string m_LastName = null;
        private string m_Alias = null;
        private string m_City = null;
        private string m_Country = null;
        private string m_Mail = null;
        private string m_Website = null;
        private bool m_Active = true;

        public string FirstName
        {
            get { return m_FirstName; }
            set { m_FirstName = value; }
        }

        public string Website
        {
            get { return m_Website; }
            set { m_Website = value; }
        }

        public string LastName
        {
            get { return m_LastName; }
            set { m_LastName = value; }
        }

        public string Alias
        {
            get { return m_Mail; }
            set { m_Mail = value; }
        }

        public string Mail
        {
            get { return m_Alias; }
            set { m_Alias = value; }
        }

        public string City
        {
            get { return m_City; }
            set { m_City = value; }
        }

        public string Country
        {
            get { return m_Country; }
            set { m_Country = value; }
        }

        public bool Active
        {
            get { return m_Active; }
            set { m_Active = value; }
        }

        public string FullName
        {
            get { return string.Format("{0} {1}", FirstName, LastName); }
        }

        public string HtmlDocFileName
        {
            get { return string.Format("{0}.htm", Alias); }
        }

        public bool HasAlias
        {
            get { return ((Alias != null) && (Alias.Length > 0)); }
        }

        public bool HasMail
        {
            get { return ((Mail != null) && (Mail.Length > 0)); }
        }

        public bool HasLocation
        {
            get { return ((City != null) && (City.Length > 0) && (Country != null) && (Country.Length > 0)); }
        }
    }
}