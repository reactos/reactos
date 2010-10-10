using System;
using System.Text;
using System.IO;
using System.Collections.Generic;

namespace SysGen.RBuild.Framework
{
    public class RBuildDebugChannel
    {
        private string m_Name = null;
        private bool m_Warn = true;
        private bool m_Error = true;
        private bool m_Trace = false;
        private bool m_Fixme = true;

        public RBuildDebugChannel()
        {
        }

        public RBuildDebugChannel(string name)
        {
            m_Name = name;
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public bool Warn
        {
            get { return m_Warn; }
            set { m_Warn = value; }
        }

        public bool Error
        {
            get { return m_Error; }
            set { m_Error = value; }
        }

        public bool Trace
        {
            get { return m_Trace; }
            set { m_Trace = value; }
        }

        public bool Fixme
        {
            get { return m_Fixme; }
            set { m_Fixme = value; }
        }

        public string Text
        {
            get
            {
                StringBuilder sBuilder = new StringBuilder();

                if (Warn)
                    sBuilder.AppendFormat("warn+{0},", Name);
                if (Error)
                    sBuilder.AppendFormat("err+{0},", Name);
                if (Trace)
                    sBuilder.AppendFormat("trace+{0},", Name);
                if (Fixme)
                    sBuilder.AppendFormat("fix+{0},", Name);

                return sBuilder.ToString();
            }
        }
    }
}
