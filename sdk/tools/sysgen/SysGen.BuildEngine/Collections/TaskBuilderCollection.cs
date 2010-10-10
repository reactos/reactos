using System;
using System.Collections;
using System.Collections.Generic;

namespace SysGen.BuildEngine 
{
    public class TaskBuilderCollection : List<TaskBuilder>
    {
        public bool Add(TaskBuilder builder)
        {
            // prevent adding duplicate builders with the same task name
            if (FindBuilderForTask(builder.TaskName) == null)
            {
                base.Add(builder);
                return true;
            }

            return false;
        }

        public TaskBuilder FindBuilderForTask(string taskName)
        {
            foreach (TaskBuilder builder in this)
            {
                if (builder.TaskName == taskName)
                {
                    return builder;
                }
            }
            return null;
        }
    }
}
