using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.BuildEngine
{
    public interface ITaskContainer : ITask
    {
        bool ExecuteChilds { get; }
        TaskCollection ChildTasks { get; }
    }
}