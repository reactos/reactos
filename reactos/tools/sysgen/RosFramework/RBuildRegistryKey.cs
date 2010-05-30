using System;
using System.Collections.Generic;
using System.Text;

using Microsoft.Win32;

namespace SysGen.RBuild.Framework
{
    public class RBuildRegistryKey
    {
        private bool m_Enabled = true;
        private bool m_LiveCD = true;
        private bool m_BootCD = true;

        private string m_KeyName = null;
        private string m_KeyValue = null;

        private RegistryHive m_RegistryHive = RegistryHive.ClassesRoot;
        private RegistryValueKind m_RegistryValueKind = RegistryValueKind.Unknown;

        public RBuildRegistryKey()
        {
        }

        public bool LiveCD
        {
            get { return m_LiveCD; }
            set { m_LiveCD = value; }
        }
        
        public bool BootCD
        {
            get { return m_BootCD; }
            set { m_BootCD = value; }
        }

        public bool Enabled
        {
            get { return m_Enabled; }
            set { m_Enabled = value; }
        }

        public string KeyName
        {
            get { return m_KeyName; }
            set { m_KeyName = value; }
        }

        public string KeyValue
        {
            get { return m_KeyValue; }
            set { m_KeyValue = value; }
        }

        public RegistryHive RegistryHive
        {
            get { return m_RegistryHive; }
            set { m_RegistryHive = value; }
        }

        public RegistryValueKind RegistryValueKind
        {
            get { return m_RegistryValueKind; }
            set { m_RegistryValueKind = value; }
        }
    }
}
