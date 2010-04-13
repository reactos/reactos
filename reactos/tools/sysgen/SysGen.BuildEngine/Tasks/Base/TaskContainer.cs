using System;
using System.Collections.Generic;

using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine
{
    /// <summary>
    /// A Generic Task Container
    /// </summary>
    public abstract class TaskContainer : Task , ITaskContainer
    {
        protected bool m_ExecuteChilds = true;
        protected TaskCollection _childTasks = new TaskCollection();

        /// <summary>
        /// Available child <see cref="Task"/> instances.
        /// </summary>
        public TaskCollection ChildTasks
        {
            get { return _childTasks; }
        }

        public bool ExecuteChilds
        {
            get { return m_ExecuteChilds; }
        }
    }
}