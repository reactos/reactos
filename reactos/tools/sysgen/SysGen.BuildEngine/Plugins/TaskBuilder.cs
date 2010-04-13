using System;
using System.Reflection;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine 
{
    public class TaskBuilder 
    {  
        private string _className;
        private string _assemblyFileName;
        private string _taskName;

        public TaskBuilder(string className) : this(className, null) {
        }

        public TaskBuilder(string className, string assemblyFileName) {
            _className = className;
            _assemblyFileName = assemblyFileName;

            // get task name from attribute
            Assembly assembly = GetAssembly();
            TaskNameAttribute taskNameAttribute = (TaskNameAttribute) 
                Attribute.GetCustomAttribute(assembly.GetType(ClassName), typeof(TaskNameAttribute));

            _taskName = taskNameAttribute.FullTaskName; // Name;
        }

        public string ClassName
        {
            get { return _className; }
        }

        public string AssemblyFileName
        {
            get { return _assemblyFileName; }
        }

        public string TaskName
        {
            get { return _taskName; }
        }

        private Assembly GetAssembly() {
            Assembly assembly = null;
            if (AssemblyFileName == null) {
                assembly = Assembly.GetExecutingAssembly();
            } else {
                //check to see if it is loaded already
                Assembly [] ass = AppDomain.CurrentDomain.GetAssemblies();
                for (int i = 0; i < ass.Length; i++){
                    try {
                        if(ass[i].Location.Equals(AssemblyFileName)) { 
                            assembly = ass[i];
                            return assembly;
                        }
                    }
                        // System.Reflection.Emit.Assembly have no location and will fail
                    catch{}
                }
                //load if not loaded
                if(assembly == null)
                    assembly = Assembly.LoadFrom(AssemblyFileName);
            }
            return assembly;
        }

        public Task CreateTask()
        {
            return (Task)GetAssembly().CreateInstance(ClassName, true);
        }
    }
}