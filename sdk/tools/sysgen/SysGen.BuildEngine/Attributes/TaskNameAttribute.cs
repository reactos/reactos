namespace SysGen.BuildEngine.Attributes 
{
    using System;
    using System.Reflection;

    /// <summary>Indicates that class should be treated as a task.</summary>
    /// <remarks>
    /// Attach this attribute to a subclass of Task to have NAnt be able
    /// to recognize it.  The name should be short but must not confict
    /// with any other task already in use.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Class, Inherited=false, AllowMultiple=false)]
    public class TaskNameAttribute : Attribute 
    {
        private string m_Namespace = null;
        private string m_Name = null;

        public TaskNameAttribute(string name) 
        {
            m_Name = name;
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }
        
        public string Namespace
        {
            get { return m_Namespace; }
            set { m_Namespace = value; }
        }

        public string FullTaskName
        {
            get
            {
                if (Namespace != null)
                    return string.Format("{0}:{1}", Namespace, Name);

                return Name;
            }
        }
    }
}