using System;
using System.Xml;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    public abstract class PropertyBaseTask : Task 
    {
        protected string m_Name = null;
        protected string m_Value = String.Empty;
        protected bool m_ReadOnly = false;
        protected bool m_Internal = false;

        /// <summary>the name of the property to set.</summary>        
        [TaskAttribute("name", Required=true)]
        public string PropName { get { return m_Name; } set { m_Name = value; } }

        /// <summary>the value of the property.</summary>        
        [TaskAttribute("value", Required=true)]
        public string Value { get { return m_Value; } set { m_Value = value; } }

        /// <summary>the value of the property.</summary>        
        [TaskAttribute("readonly")]
        [BooleanValidator()]
        public bool ReadOnly { get { return m_ReadOnly; } set { m_ReadOnly = value; } }

        [TaskAttribute("internal")]
        [BooleanValidator()]
        public bool Internal { get { return m_Internal; } set { m_Internal = value; } }

        protected override void OnLoad()
        {
            Project.Properties.Add(new RBuildProperty(m_Name, m_Value, m_ReadOnly, m_Internal));
        }
    }
}