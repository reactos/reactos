using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("define")]
    public class DefineTask : Task
    {
        private string _name = null;
        private string _value = String.Empty;
        private string _backend = String.Empty;
        private bool _empty = false;

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("name", Required=true)]
        public string DefineName { get { return _name; } set { _name = value; } }

        /// <summary>
        /// The define value.
        /// </summary>
        [TaskAttribute("value")]
        [TaskValue]
        public string DefineValue { get { return _value; } set { _value = value; } }

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("empty")]
        [BooleanValidator]
        public bool Empty { get { return _empty; } set { _empty = value; } }

        // TODO : Remove ?
        [TaskAttribute("backend")]
        public string Backend { get { return _backend; } set { _backend = value; } }

        protected override void ExecuteTask() 
        {
            RBuildElement.Defines.Add(new RBuildDefine(DefineName, DefineValue));
        }
    }
}