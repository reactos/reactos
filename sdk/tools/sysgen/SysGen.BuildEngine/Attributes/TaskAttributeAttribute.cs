namespace SysGen.BuildEngine.Attributes 
{
    using System;
    using System.Reflection;

    /// <summary>Indicates that field should be treated as a xml attribute for the task.</summary>
    /// <example>
    /// Examples of how to specify task attributes
    /// <code>
    /// // task XmlType default is string
    /// [TaskAttribute("out", Required=true)]
    /// string _out = null; // assign default value here
    ///
    /// [TaskAttribute("optimize")]
    /// [BooleanValidator()]
    /// // during ExecuteTask you can safely use Convert.ToBoolean(_optimize)
    /// string _optimize = Boolean.FalseString;
    ///
    /// [TaskAttribute("warnlevel")]
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
    [AttributeUsage( AttributeTargets.Property, Inherited=true)]
    public class TaskAttributeAttribute : TaskPropertyAttribute {
       
        public TaskAttributeAttribute(string name) : base(name){          
        }

        public override TaskPropertyLocation Location
        {
            get { return TaskPropertyLocation.Attribute; }
        }
    }
}
