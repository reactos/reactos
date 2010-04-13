using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildProperty : RBuildValueKey
    {
        public RBuildProperty(string name, string value)
            : base(name, value)
        {
        }

        public RBuildProperty(string name, string value, bool readOnly)
            : base(name, value, readOnly)
        {
        }

        public RBuildProperty(string name, string value, bool readOnly, bool isInternal)
            : base(name, value, readOnly, isInternal)
        {
        }
    }

    public class RBuildBaseAdress : RBuildProperty
    {
        public RBuildBaseAdress(string name, string value)
            : base(name, value, true)
        {
        }
    }
    
    public class RBuildDefine : RBuildValueKey
    {
        public RBuildDefine(string name)
            : base(name, string.Empty, true)
        {
        }

        public RBuildDefine(string name, string value)
            : base(name, value, true)
        {
        }
    }
    
    public abstract class RBuildValueKey
    {
        protected string m_Name = null;
        protected string m_Value = null;
        protected bool m_ReadOnly = false;
        protected bool m_IsInternal = false;

        public RBuildValueKey(string name, string value)
        {
            m_Name = name;
            m_Value = value;
        }

        public RBuildValueKey(string name, string value, bool readOnly)
        {
            m_Name = name;
            m_Value = value;
            m_ReadOnly = readOnly;
        }

        public RBuildValueKey(string name, string value, bool readOnly, bool isInternal)
        {
            m_Name = name;
            m_Value = value;
            m_ReadOnly = readOnly;
            m_IsInternal = isInternal;
        }

        public bool IsEmpty
        {
            get { return string.IsNullOrEmpty(Value); }
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string Value
        {
            get { return m_Value; }
            set { m_Value = value; }
        }

        public bool ReadOnly
        {
            get { return m_ReadOnly; }
            set { m_ReadOnly = value; }
        }

        public bool Internal
        {
            get { return m_IsInternal; }
            set { m_IsInternal = value; }
        }
    }
}
