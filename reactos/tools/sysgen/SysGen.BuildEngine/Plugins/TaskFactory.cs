using System;
using System.IO;
using System.Xml;
using System.Reflection;
using System.Collections;

namespace SysGen.BuildEngine 
{
    /// <summary>
    /// The TaskFactory comprises all of the loaded, and available, tasks. 
    /// Use these static methods to register, initialize and create a task.
    /// </summary>
    public class TaskFactory 
    {
        static TaskBuilderCollection _builders = new TaskBuilderCollection();
        static ArrayList _projects = new ArrayList();

        /// <summary> 
        /// Initializes the tasks in the executing assembly, and basedir of the current domain.
        /// </summary>
        static TaskFactory() 
        {
            // initialize builtin tasks
            AddTasks(Assembly.GetExecutingAssembly());
            AddTasks(Assembly.GetCallingAssembly());


            //string nantBinDir = Path.GetFullPath(AppDomain.CurrentDomain.BaseDirectory);
            //ScanDir(nantBinDir);
            //ScanDir(Path.Combine(nantBinDir, "tasks"));                      
        }

        /*
        /// <summary>Scans the path for any Tasks assemblies and adds them.</summary>
        /// <param name="path">The directory to scan in.</param>
        protected static void ScanDir(string path) {
            // Don't do anything if we don't have a valid directory path
            if(path == null || path == string.Empty) {
                return;
            }

            // intialize tasks found in assemblies that end in Tasks.dll
            DirectoryScanner scanner = new DirectoryScanner();
            scanner.BaseDirectory = path;
            scanner.Includes.Add("*Tasks.dll");
            
            //needed for testing
            scanner.Includes.Add("*Tests.dll");
            scanner.Includes.Add("*Test.dll");

            foreach(string assemblyFile in scanner.FileNames) {
                //Log.WriteLine("{0}:Add Tasks from {1}", AppDomain.CurrentDomain.FriendlyName, assemblyFile);
                
                AddTasks(Assembly.LoadFrom(assemblyFile));
                //AddTasks(AppDomain.CurrentDomain.Load(assemblyFile.Replace(AppDomain.CurrentDomain.BaseDirectory,"").Replace(".dll","")));
            }
		
        }
        */

        /*
        /// <summary> Adds any Task Assemblies in the Project.BaseDirectory.</summary>
        /// <param name="project">The project to work from.</param>
        public static void AddProject(SysGenEngine project) {
            if(project.BaseDirectory != null && !project.BaseDirectory.Equals(string.Empty)) {
                ScanDir(project.BaseDirectory);
                ScanDir(Path.Combine(project.BaseDirectory, "tasks"));
            }
            //create weakref to project. It is possible that project may go away, we don't want to hold it.
            _projects.Add(new WeakReference(project));
            foreach(TaskBuilder tb in Builders) {
                UpdateProjectWithBuilder(project, tb);
            }
        }*/

        /// <summary>Returns the list of loaded TaskBuilders</summary>
        public static TaskBuilderCollection Builders {
            get { return _builders; }
        }
        /// <summary> Scans the given assembly for any classes derived from Task and adds a new builder for them.</summary>
        /// <param name="assembly">The Assembly containing the new tasks to be loaded.</param>
        /// <returns>The count of tasks found in the assembly.</returns>
        public static int AddTasks(Assembly assembly) {
            int taskCount = 0;
            try {
                foreach(Type type in assembly.GetTypes()) {
                    if (type.IsSubclassOf(typeof(Task)) && !type.IsAbstract) {
                        TaskBuilder tb = new TaskBuilder(type.FullName, assembly.Location);
                        if (_builders.Add(tb)) {
                            foreach(WeakReference wr in _projects) {
                                if(!wr.IsAlive)
                                    continue;
                                SysGenEngine p = wr.Target as SysGenEngine;
                                if(p == null) 
                                    continue;
                                UpdateProjectWithBuilder(p, tb);
                            }
                            taskCount++;
                        }
                    }
                }
            }
                // For assemblies that don't have types
            catch{};

            return taskCount;
        }

        protected static void UpdateProjectWithBuilder(SysGenEngine sysGen, TaskBuilder taskBuilder) 
        {
            // add a true property for each task (use in build to test for task existence).
            // add a property for each task with the assembly location.
            sysGen.Properties.AddReadOnly("SysGen.Tasks." + taskBuilder.TaskName + ".Available", Boolean.TrueString);
            sysGen.Properties.AddReadOnly("SysGen.Tasks." + taskBuilder.TaskName + ".Assembly", taskBuilder.AssemblyFileName);
        }

        /// <summary> Creates a new Task instance for the given xml and project.</summary>
        /// <param name="taskNode">The XML to initialize the task with.</param>
        /// <param name="proj">The Project that the Task belongs to.</param>
        /// <returns>The Task instance.</returns>
        public static Task CreateTask(XmlNode taskNode, SysGenEngine proj) 
        {
            string taskName = taskNode.Name;

            TaskBuilder builder = _builders.FindBuilderForTask(taskName);
            if (builder == null && proj != null) {
                Location location = proj.LocationMap.GetLocation(taskNode);
                throw new BuildException(String.Format("Unknown task <{0}>", taskName), location);
            }

            Task task = builder.CreateTask();
            task.SysGen = proj;
            return task;
        }
    }
}