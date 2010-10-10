using System;
using System.IO;
using System.Xml;
using System.Collections;
using System.Collections.Specialized;

using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Log;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    /// <summary>
    /// Include an external build file.
    /// </summary>
    [TaskName("include", Namespace = "xi")]
    public class XIIncludeTask : FileSystemInfoBaseTask, ITaskContainer //TaskContainer
    {
        private TaskCollection m_ChildTasks = new TaskCollection();

        /// <summary>
        /// Used to check for recursived includes.
        /// </summary>
        private static Stack _includedFiles = new Stack();

        ///// <summary>
        ///// The file to be included
        ///// </summary>
        //private string _href = null;

        /// <summary>Build file to include.</summary>
        [TaskAttribute("href", Required = true)]
        public string BuildFileName 
        {
            get { return m_FileSystemInfo.Name; }
            set { m_FileSystemInfo.Name = value; }
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildFile();
        }

        public TaskCollection ChildTasks
        {
            get { return m_ChildTasks; }
        }

        public bool ExecuteChilds
        {
            get { return true; }
        }

        protected override void OnInit()
        {
            base.OnInit();

            Base = SysGenPathResolver.GetPath(this, SysGen.RootTask);
        }

        /// <summary>Verify parameters.</summary>
        /// <param name="taskNode">Xml taskNode used to define this task instance.</param>
        protected override void InitializeTask(XmlNode taskNode) 
        {
            //base.InitializeTask(taskNode);

            FailOnError = false;

            /*
            // Task can only be included as a global task.
            // This might not be a firm requirement but you could get some real 
            // funky errors if you start including targets wily-nily.
            if (Parent != null )
            {
                if ((!(Parent is RbuildTask)) || (!(Parent is ProjectTask))) 
                    throw new BuildException("Task not allowed in targets.  Must be at project level.", Location);
            }
            */

            // Check for recursive include.
            string buildFileName = Path.Combine(BaseBuildLocation, BuildFileName);
            foreach (string currentFileName in _includedFiles) {
                if (currentFileName == buildFileName) {
                    throw new BuildException("Recursive includes are not allowed.", Location);
                }
            }

            string includedFileName = Path.Combine(BaseBuildLocation , BuildFileName);
            string includeRelative = SysGen.GetRelativePath(includedFileName);

            // push ourselves onto the stack (prevents recursive includes)
            _includedFiles.Push(includedFileName);

            BuildLog.WriteLine("Including {0}", includeRelative);

            try
            {
                XmlDocument doc = new XmlDocument();
                doc.XmlResolver = null;
                doc.Load(includedFileName);

                SysGen.InitializeBuildFile(doc, this);

                SysGen.BuildFiles.Add(includedFileName);
            }
            catch (BuildException)
            {
                throw;
            }
            catch (IOException e)
            {
                try
                {
                    if (ChildTasks.Count == 0)
                        throw new BuildException("Could not include build file " + includedFileName, Location, e);

                    BuildLog.WriteLine("Including {0} Failed. Fallback present and executed", includeRelative);
                    //BuildLog.WriteLine("Include {0} not found. Fallback executed", includeRelative);
                }
                catch (Exception fallbackException)
                {
                    throw new BuildException("Could not include build file " + includedFileName + " fallback also failed", Location, fallbackException);
                }
            }
            catch (ArgumentException e)
            {
                //Puede pasar
            }
            catch (Exception e)
            {
                throw new BuildException("Could not include build file " + includedFileName + " " + e.Message, Location, e);
            }
            finally
            {
                // pop off the stack
                _includedFiles.Pop();
            }
        }

        protected override void ExecuteTask()
        {
        }

        protected override void PreExecuteTask()
        {
        }
    }
}
