using System;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    public abstract class ValueBaseTask : Task
    {
        protected string _value = null;

        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue]
        public virtual string Value { get { return _value; } set { _value = value; } }
    }
}
