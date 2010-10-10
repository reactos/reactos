using System;
using System.Reflection;

namespace SysGen.BuildEngine.Attributes
{
    [AttributeUsage(AttributeTargets.Property, Inherited = true)]
    public class TaskValueAttribute : TaskPropertyAttribute
    {
        public TaskValueAttribute()
            : base(null)
        {
        }

        public override TaskPropertyLocation Location
        {
            get { return TaskPropertyLocation.Node; }
        }
    }
}
