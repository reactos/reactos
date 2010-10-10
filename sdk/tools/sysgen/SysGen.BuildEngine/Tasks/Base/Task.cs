using System;
using System.IO;
using System.Reflection;
using System.Xml;

using SysGen.RBuild.Framework;

using SysGen.BuildEngine.Log;
using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Tasks;

namespace SysGen.BuildEngine 
{
    public enum TaskExecuteStage
    {
        PreExecute,
        Execute,
        PostExecute
    }

    /// <summary>
    /// Provides the abstract base class for tasks.
    /// </summary>
    /// <remarks>
    /// A task is a piece of code that can be executed.
    /// </remarks>
    public abstract class Task : Element, ITask
    {
        protected bool _failOnError = true;
        protected bool _verbose = false;
        protected bool _ifDefined = true;
        protected bool _ifNotDefined = false;

        TaskExecuteStage _stage = TaskExecuteStage.PreExecute;

        /// <summary>
        /// Determines if task failure stops the build, or is just reported. Default is "true".
        /// </summary>
        [TaskAttribute("failonerror")]
        [BooleanValidator()]
        public bool FailOnError {
            get { return _failOnError; }
            set { _failOnError = value; }
        }

        /// <summary>
        /// Task reports detailed build log messages.  Default is "false".
        /// </summary>
        [TaskAttribute("verbose")]
        [BooleanValidator()]
        public bool Verbose {
            get { return (_verbose || SysGen.Verbose); }
            set { _verbose = value; }
        }

        /// <summary>
        /// If true then the task will be executed; otherwise skipped. Default is "true".
        /// </summary>
        [TaskAttribute("if")]
        [BooleanValidator()]
        public bool IfDefined {
            get { return _ifDefined; }
            set { _ifDefined = value; }
        }

        /// <summary>
        /// Opposite of if. If false then the task will be executed; otherwise skipped. Default is "false".
        /// </summary>
        [TaskAttribute("ifnot")]
        [BooleanValidator()]
        public bool IfNotDefined
        {
            get { return _ifNotDefined; }
            set { _ifNotDefined = value; }
        }

        public string XmlFile
        {
            get { return new Uri(_xmlNode.OwnerDocument.BaseURI).LocalPath; }
        }

        public string RBuildFile
        {
            get { return Path.GetFileName(XmlFile); }
        }

        /// <summary>The name of the task.</summary>
        public override string Name {
            get {
                string name = null;
                TaskNameAttribute taskName = (TaskNameAttribute) Attribute.GetCustomAttribute(GetType(), typeof(TaskNameAttribute));
                if (taskName != null) {
                    name = taskName.Name;
                }
                return name;
            }
        }

        public RBuildFolder InFolder
        {
            get
            {
                IElement task = Parent;
                while (task != SysGen.RootTask)
                {
                    DirectoryTask directory = task as DirectoryTask;

                    if (directory != null)
                        return directory.Folder;

                    task = task.Parent;
                }

                return SysGen.Project.Folder;
            }
        }

        /// <summary>
        /// The prefix used when sending messages to the log.
        /// </summary>
        public string LogPrefix {
            get {
                string prefix = "[" + Name + "] ";
                return prefix.PadLeft(BuildLog.IndentSize);
            }
        }

        private TaskExecuteStage ExecutionStage
        {
            get { return _stage; }
        }

        /// <summary>
        /// Executes the task unless it is skipped. <note>Do not ovveride/new this method. Use ExecuteTask instead.</note>
        /// </summary>
        public void Execute()
        {
            RunTask(TaskExecuteStage.Execute);
        }

        public void PostExecute()
        {
            RunTask(TaskExecuteStage.PostExecute);
        }

        public void PreExecute()
        {
            RunTask(TaskExecuteStage.PreExecute);
        }

        private void RunTask (TaskExecuteStage stage)
        {
            // Save the current execution stage
            _stage = stage;

            if (IfDefined && !IfNotDefined)
            {
                try
                {
                    SysGenEngine.OnTaskStarted(this, new BuildEventArgs(Name));

                    switch (stage)
                    {
                        case TaskExecuteStage.PreExecute:
                            PreExecuteTask();
                            break;
                        case TaskExecuteStage.Execute:
                            ExecuteTask();
                            break;
                        case TaskExecuteStage.PostExecute:
                            PostExecuteTask();
                            break;
                    }

                    if (this is ITaskContainer)
                    {
                        ITaskContainer taskContainer = this as ITaskContainer;

                        if (taskContainer.ExecuteChilds)
                        {
                            foreach (Task taskChild in taskContainer.ChildTasks)
                            {
                                taskChild.RunTask(stage);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    SysGenEngine.OnTaskException(this, new BuildEventArgs(Name));

                    if (FailOnError)
                    {
                        throw;
                    }
                    else
                    {
                        BuildLog.WriteLine(e.Message);
                        if (e.InnerException != null)
                        {
                            BuildLog.WriteLine(e.InnerException.Message);
                        }
                    }
                }
                finally
                {
                    SysGenEngine.OnTaskFinished(this, new BuildEventArgs(Name));
                }
            }
        }

        protected override void InitializeElement(XmlNode elementNode) 
        {
            if (this is ITaskContainer)
            {
                ITaskContainer taskContainer = this as ITaskContainer;

                foreach (XmlNode childNode in elementNode.ChildNodes)
                {
                    if (SysGen.CanProcessNode(childNode))
                        SysGen.LoadChildTask(childNode, taskContainer);
                }
            }
        
            // Just defer for now so that everything just works
            InitializeTask(elementNode);
        }

        /// <summary>Initializes the task.</summary>
        protected virtual void InitializeTask(XmlNode taskNode) 
        {
        }

        protected virtual void PostExecuteTask()
        {
        }

        /// <summary>
        /// Executes the task.
        /// </summary>
        protected virtual void ExecuteTask()
        { 
        }

        /// <summary>
        /// Executes the task.
        /// </summary>
        protected virtual void PreExecuteTask()
        {
        }

        public override string ToString()
        {
            return Name;
        }
    }
}