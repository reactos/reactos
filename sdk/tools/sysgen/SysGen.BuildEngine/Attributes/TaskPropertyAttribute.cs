using System;
using System.Reflection;

namespace SysGen.BuildEngine.Attributes 
{
    public enum TaskPropertyLocation
    {
        Attribute,
        Node
    }

    /// <summary>Indicates that field should be treated as a xml attribute for the task.</summary>
    /// <example>
    /// Examples of how to specify task attributes
    /// <code>
    /// // task XmlType default is string
    /// [BuildAttribute("out", Required=true)]
    /// string _out = null; // assign default value here
    ///
    /// [BuildAttribute("optimize")]
    /// [BooleanValidator()]
    /// // during ExecuteTask you can safely use Convert.ToBoolean(_optimize)
    /// string _optimize = Boolean.FalseString;
    ///
    /// [BuildAttribute("warnlevel")]
    /// [Int32Validator(0,4)] // limit values to 0-4
    /// // during ExecuteTask you can safely use Convert.ToInt32(_optimize)
    /// string _warnlevel = "0";
    ///
    /// [FileSet("sources")]
    /// FileSet _sources = new FileSet();
    /// </code>
    /// NOTE: Attribute values must be of type of string if you want
    /// to be able to have macros.  The field stores the exact value during
    /// InitializeTask.  Just before ExecuteTask is called NAnt will expand
    /// all the macros with the current values.
    /// </example>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field , Inherited=true)]
    public abstract class TaskPropertyAttribute : Attribute 
    {
        string _name;
        bool _required = false;
        bool _expandProperties = true;

        public TaskPropertyAttribute(string name) {
            _name = name;
        }

        public string Name {
            get { return _name; }
            set { _name = value; }
        }

        public bool Required {
            get { return _required; }
            set { _required = value; }
        }

        public bool ExpandProperties {
            get { return _expandProperties; }
            set { _expandProperties = value; }
        }

        public abstract TaskPropertyLocation Location { get; }
    }
}
