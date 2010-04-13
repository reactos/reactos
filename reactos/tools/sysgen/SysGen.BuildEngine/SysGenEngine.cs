using System;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.XPath;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Log;
using SysGen.BuildEngine.Tasks;
using SysGen.BuildEngine.Handlers;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine 
{
    public class SysGenEngine
    {
        private Task m_RootTask = null;

        private RBuildProject m_Project = null;
        private RBuildPlatform m_Platform = null;

        //private TargetCollection m_Targets = new TargetCollection();
        private FileHandlerCollection m_FileHandlers = new FileHandlerCollection();
        private BackendCollection m_Backends = new BackendCollection();
        private RBuildPropertyCollection m_Properties = new RBuildPropertyCollection();
        private StringCollection m_XmlBuildFiles = new StringCollection();
        
        //xml element and attribute names that are not defined in metadata
        protected const string PROJECT_XMLROOT = "project";
        protected const string PROJECT_NAME_ATTRIBUTE = "name";
        protected const string PROJECT_DEFAULT_ATTRIBUTE = "default";
        protected const string PROJECT_BASEDIR_ATTRIBUTE = "basedir";

        public const string SYSGEN_PROPERTY_FILENAME = "SysGen.Filename";
        public const string SYSGEN_PROPERTY_VERSION = "SysGen.Version";
        public const string SYSGEN_PROPERTY_LOCATION = "SysGen.Location";
        public const string SYSGEN_PROPERTY_PROJECT_NAME = "SysGen.Project.Name";
        public const string SYSGEN_PROPERTY_PROJECT_BUILDFILE = "SysGen.Project.File";
        public const string SYSGEN_PROPERTY_PROJECT_BASEDIR = "SysGen.Project.BaseDir";

        private string m_BaseDir = null;
        private bool m_Verbose = false;
        private bool m_ExecuteBackends = true;
        private bool m_SetDefaults = true;

        private LocationMap _locationMap = new LocationMap();
        private XmlDocument _doc = null; // set in ctorHelper

        public static event BuildEventHandler BuildStarted;
        public static event BuildEventHandler BuildFinished;
        public static event BuildEventHandler TargetStarted;
        public static event BuildEventHandler TargetFinished;
        public static event BuildEventHandler TaskStarted;
        public static event BuildEventHandler TaskFinished;
        public static event BuildEventHandler TaskException;
        public static event BuildEventHandler BuildFileLoaded;

        public static void OnBuildFileLoaded(object o, BuildEventArgs e)
        {
            if (BuildFileLoaded != null)
                BuildFileLoaded(o, e);
        }

        public static void OnBuildStarted(object o, BuildEventArgs e)
        {
            if (BuildStarted != null)
                BuildStarted(o, e);
        }

        public static void OnBuildFinished(object o, BuildEventArgs e)
        {
            if (BuildFinished != null)
                BuildFinished(o, e);
        }

        public static void OnTargetStarted(object o, BuildEventArgs e)
        {
            if (TargetStarted != null)
                TargetStarted(o, e);
        }

        public static void OnTargetFinished(object o, BuildEventArgs e)
        {
            if (TargetFinished != null)
                TargetFinished(o, e);
        }

        public static void OnTaskStarted(object o, BuildEventArgs e)
        {
            if (TaskStarted != null)
                TaskStarted(o, e);
        }

        public static void OnTaskFinished(object o, BuildEventArgs e)
        {
            if (TaskFinished != null)
                TaskFinished(o, e);
        }

        public static void OnTaskException(object o, BuildEventArgs e)
        {
            if (TaskException != null)
                TaskException(o, e);
        }

        public RBuildProject Project
        {
            get { return m_Project; }
            set
            {
                if (m_Project != null)
                    throw new BuildException("Only one ProjectTask is allowed per project");

                m_Project = value;

                InitializeEnvironment();
            }
        }

        public Task RootTask
        {
            get { return m_RootTask; }
            set { m_RootTask = value; }
        }

        public StringCollection BuildFiles
        {
            get { return m_XmlBuildFiles; }
        }

        public bool SetDefaults
        {
            get { return m_SetDefaults; }
            set { m_SetDefaults = value; }
        }

        public SysGenEngine(string path, string project) : this (Path.Combine (path , project))
        {
        }

        /// <summary>
        /// Constructs a new Project with the given source.
        /// </summary>
        /// <param name="source">
        /// <para> The Source should be the full path to the build file.</para>
        /// <para> This can be of any form that XmlDocument.Load(string url) accepts.</para>
        /// </param>
        /// <remarks><para>If the source is a uri of form 'file:///path' then use the path part.</para></remarks>
        public SysGenEngine(string source)
        {
            string path = source;
            //if the source is not a valid uri, pass it thru.
            //if the source is a file uri, pass the localpath of it thru.
            try
            {
                Uri testURI = new Uri(source);
                if (testURI.IsFile)
                {
                    path = testURI.LocalPath;
                }
            }
            catch (Exception e)
            {
                //do nothing.
                e.ToString();
            }
            finally
            {
                if (path == null)
                    path = source;
            }

            ctorHelper(LoadBuildFile(path));
        }

        /// <summary>
        /// Inits stuff:
        ///     <para>TaskFactory: Calls Initialize and AddProject </para>
        ///     <para>Log.IndentSize set to 12</para>
        ///     <para>Project properties are initialized ("nant.* stuff set")</para>
        ///     <list type="nant.items">
        ///         <listheader>NAnt Props:</listheader>
        ///         <item>nant.filename</item>
        ///         <item>nant.version</item>
        ///         <item>nant.location</item>
        ///         <item>nant.project.name</item>
        ///         <item>nant.project.buildfile (if doc has baseuri)</item>
        ///         <item>nant.project.basedir</item>
        ///         <item>nant.project.default = defaultTarget</item>
        ///         <item>nant.tasks.[name] = true</item>
        ///         <item>nant.tasks.[name].location = AssemblyFileName</item>
        ///     </list>
        /// </summary>
        /// <param name="doc">The Project Document.</param>
        protected virtual void ctorHelper(XmlDocument doc) 
        {
            //TaskFactory.AddProject(this);
            BuildLog.IndentSize = 12;
            _doc = doc;

            string newBaseDir = null;

            //check to make sure that the root element in named correctly
            if(!doc.DocumentElement.Name.Equals(PROJECT_XMLROOT))
                throw new ApplicationException("Root Element must be named " + PROJECT_XMLROOT + " in " + doc.BaseURI);

            /*
            // get project attributes
            if(doc.DocumentElement.HasAttribute(PROJECT_NAME_ATTRIBUTE))   
                _projectName = doc.DocumentElement.GetAttribute(PROJECT_NAME_ATTRIBUTE);

            if(doc.DocumentElement.HasAttribute(PROJECT_BASEDIR_ATTRIBUTE)) 
                newBaseDir = doc.DocumentElement.GetAttribute(PROJECT_BASEDIR_ATTRIBUTE);

            if(doc.DocumentElement.HasAttribute(PROJECT_DEFAULT_ATTRIBUTE)) 
                _defaultTargetName  = doc.DocumentElement.GetAttribute(PROJECT_DEFAULT_ATTRIBUTE);
            */

            // give the project a meaningful base directory
            if (newBaseDir == null) {
                if (BuildFileLocalName != null) {
                    newBaseDir = Path.GetDirectoryName(BuildFileLocalName);
                }
                else {
                    newBaseDir = Environment.CurrentDirectory;
                }
            }

            newBaseDir = Path.GetFullPath(newBaseDir);
            //BaseDirectory must be rooted.
            BaseDirectory = newBaseDir;

        }

        internal void InitializeBuildFile(XmlDocument doc, ITaskContainer parent)
        {
            // load line and column number information into position map
            LocationMap.Add(doc);

            // initialize targets and global tasks
            foreach (XmlNode childNode in doc.ChildNodes) 
            {
                if (CanProcessNode(childNode))
                {
                    LoadChildTask(childNode, parent);
                }
            }
        }

        internal bool CanProcessNode(XmlNode childNode)
        {
            if ((childNode.NodeType == XmlNodeType.Element) &&
                (childNode.Name.StartsWith("#") == false) &&
                (childNode.Name.StartsWith("xml") == false) &&
                (childNode.Name.StartsWith("!") == false) &&
                (childNode.NamespaceURI.Equals(string.Empty) || 
                (childNode.NamespaceURI.Equals("http://www.w3.org/2001/XInclude"))))
            {
                return true;
            }

            return false;
        }

        internal Task LoadChildTask(XmlNode taskNode, ITaskContainer parent)
        {
            try
            {
                Task task = TaskFactory.CreateTask(taskNode, this);

                task.Parent = parent;
                task.Project = m_Project;
                task.SysGen = this;
                task.Initialize(taskNode);

                if (task != RootTask)
                    parent.ChildTasks.Add(task);

                return task;
            }
            catch (BuildException be)
            {
                BuildLog.WriteLine("{0} Failed to created Task for '{1}' xml element for reason: \n {2}", parent.LogPrefix, taskNode.Name, be.Message);
            }

            return null;
        }

        /// <summary>
        /// Creates a new XmlDocument based on the project definition.
        /// </summary>
        /// <param name="source">The source of the document. <para>Any form that is valid for XmlDocument.Load(string url) can be used here.</para></param>
        /// <returns>The project document.</returns>
        private XmlDocument LoadBuildFile(string source) 
        {
            XmlDocument doc = new XmlDocument();

            try 
            {
                OnBuildFileLoaded(this, new BuildEventArgs(source));

                doc.XmlResolver = null;
                doc.Load(source);

                //Add the build file to the collection of readed xml build files
                m_XmlBuildFiles.Add(source);
            } 
            catch (XmlException e) 
            {
                string message = "Error loading buildfile";
                Location location = new Location(source, e.LineNumber, e.LinePosition);
                throw new BuildException(message, location, e);
            } 
            catch (Exception e) 
            {
                string message = "Error loading buildfile";
                Location location = new Location(source);
                throw new BuildException(message, location, e);
            }
            return doc;
        }

        public virtual bool RBuildFolderExists(RBuildFolder folder)
        {
            return Directory.Exists(ResolveRBuildFilePath(folder));
        }

        public virtual bool RBuildFileExists(RBuildFile file)
        {
            return File.Exists(ResolveRBuildFilePath(file));
        }

        public virtual string ResolveRBuildFilePath(RBuildFileSystemInfo file)
        {
            return NormalizePath(Path.Combine(GetPathRoot(file.Root), file.FullPath));
        }

        public virtual string ResolveRBuildFolderPath(RBuildFolder folder)
        {
            return NormalizePath(Path.Combine(GetPathRoot(folder.Root), folder.FullPath));
        }

        public virtual string ResolveRBuildFilePath(PathRoot root, RBuildFileSystemInfo file)
        {
            return NormalizePath(Path.Combine(GetPathRoot(root), file.FullPath));
        }

        public string NormalizePath(string path)
        {
            return path.Replace(
                Path.AltDirectorySeparatorChar, 
                Path.DirectorySeparatorChar);
        }

        public string IntermediateDirectory
        {
            get { return Path.Combine(BaseDirectory, "obj-i386"); }
        }

        public string DocumentationDirectory
        {
            get { return Path.Combine(BaseDirectory, "doc-i386"); }
        }

        public string ISODirectory
        {
            get { return Path.Combine(BaseDirectory, "iso-i386"); }
        }

        public string OutputDirectory
        {
            get { return Path.Combine(BaseDirectory, "output-i386"); }
        }

        public string BootCDOutputDirectory
        {
            get { return Path.Combine(OutputDirectory, "cd"); }
        }

        public string LiveCDOutputDirectory
        {
            get { return Path.Combine(OutputDirectory, "livecd"); }
        }

        public string TemporaryDirectory
        {
            get { return Path.Combine(BaseDirectory, "obj-i386"); }
        }

        public string InstallDirectory
        {
            get { return Path.Combine(BaseDirectory, "reactos"); }
        }

        public string GetPathRoot(PathRoot root)
        {
            switch (root)
            {
                case PathRoot.Default:
                case PathRoot.SourceCode:
                    return BaseDirectory;
                    break;
                case PathRoot.LiveCD:
                    return LiveCDOutputDirectory;
                    break;
                case PathRoot.BootCD:
                    return BootCDOutputDirectory;
                    break;
                case PathRoot.Intermediate:
                    return IntermediateDirectory;
                    break;
                case PathRoot.Install:
                    return InstallDirectory;
                    break;
                case PathRoot.Output:
                    return OutputDirectory;
                    break;
                case PathRoot.Temporary:
                    return TemporaryDirectory;
                    break;
                case PathRoot.Platform:
                    return "%SystemRoot%";
                    break;
                default:
                    throw new Exception("Unknown PathRoot");
            }
        }

        /// <summary>
        /// The Base Directory used for relative references.
        /// </summary>
        /// <remarks>
        ///     <para>The directory must be rooted. (must start with drive letter, unc, etc.)</para>
        ///     <para>The BaseDirectory sets and gets the special property named 'nant.project.basedir'.</para>
        /// </remarks>
        public string BaseDirectory
        {
            get
            {
                //string basedir = null; // = Properties[NANT_PROPERTY_PROJECT_BASEDIR];

                if (m_BaseDir == null)
                    return null;

                if (!Path.IsPathRooted(m_BaseDir))
                    throw new BuildException("BaseDirectory must be rooted! " + m_BaseDir);

                return m_BaseDir;
            }
            set
            {
                if (!Path.IsPathRooted(value))
                    throw new BuildException("BaseDirectory must be rooted! " + value);

                m_BaseDir = value;

                //Properties[NANT_PROPERTY_PROJECT_BASEDIR] = value;
            }
        }

        /// <summary>
        /// The URI form of the current Document
        /// </summary>
        public Uri BuildFileURI {
            get {
                //TODO: Need to remove this.
                if(Doc == null || Doc.BaseURI == "") {
                    return null;//new Uri("http://localhost");
                }
                else {
                    return new Uri(Doc.BaseURI);
                }
            }
        }

        /// <summary>
        /// If the build document is not file backed then null will be returned.
        /// </summary>
        public string BuildFileLocalName {
            get {
                if (BuildFileURI != null && BuildFileURI.IsFile) {
                    return BuildFileURI.LocalPath;
                }
                else {
                    return null;
                }
            }
        }

        /// <summary>Returns the active build file</summary>
        public virtual XmlDocument Doc {
            get { return _doc; }
        }

        /// <summary>
        /// When true tasks should output more build log messages.
        /// </summary>
        public bool Verbose 
        {
            get { return m_Verbose; }
            set { m_Verbose = value; }
        }

        public bool RunBackends
        {
            get { return m_ExecuteBackends; }
            set { m_ExecuteBackends = value; }
        }

        public RBuildPlatform Platform
        {
            get { return m_Platform; }
        }

        public RBuildPropertyCollection Properties
        {
            get { return m_Properties; }
        }

        internal LocationMap LocationMap {
            get { return _locationMap; }
        }

        ///// <summary>
        ///// The targets defined in the this project.
        ///// </summary>
        //public TargetCollection Targets 
        //{
        //    get { return m_Targets; }
        //}

        /// <summary>Executes the default target.</summary>
        /// <remarks>
        ///     <para>No top level error handling is done. Any BuildExceptions will make it out of this method.</para>
        /// </remarks>
        public virtual void Execute()
        {
            //InitializeEnvironment();

            //will initialize the list of Targets, and execute any global tasks.
            InitializeBuildFile(Doc, null);

            RegisterBackends();

            if (Project.InstallFolders.Count == 0)
            {
                Project.InstallFolders.Add(new RBuildInstallFolder("1", @"."));
                Project.InstallFolders.Add(new RBuildInstallFolder("2", @"system32"));
                Project.InstallFolders.Add(new RBuildInstallFolder("3", @"system32\config"));
                Project.InstallFolders.Add(new RBuildInstallFolder("4", @"system32\drivers"));
                Project.InstallFolders.Add(new RBuildInstallFolder("5", @"system"));
                Project.InstallFolders.Add(new RBuildInstallFolder("17", @"system32\drivers\etc"));
                Project.InstallFolders.Add(new RBuildInstallFolder("20", @"inf"));
                Project.InstallFolders.Add(new RBuildInstallFolder("22", @"fonts"));
                Project.InstallFolders.Add(new RBuildInstallFolder("201", @"system32\bin"));
                Project.InstallFolders.Add(new RBuildInstallFolder("202", @"media\fonts"));
                Project.InstallFolders.Add(new RBuildInstallFolder("203", @"bin"));
                Project.InstallFolders.Add(new RBuildInstallFolder("204", @"media"));
            }

            if (Project.DebugChannels.Count == 0)
            {
                Project.DebugChannels.Add(new RBuildDebugChannel("ole"));
                Project.DebugChannels.Add(new RBuildDebugChannel("rpc"));
                Project.DebugChannels.Add(new RBuildDebugChannel("gdi"));
                Project.DebugChannels.Add(new RBuildDebugChannel("crtdll"));
                Project.DebugChannels.Add(new RBuildDebugChannel("mshtml"));
                Project.DebugChannels.Add(new RBuildDebugChannel("setupapi"));
                Project.DebugChannels.Add(new RBuildDebugChannel("typelib"));
                Project.DebugChannels.Add(new RBuildDebugChannel("shdocvw"));
                Project.DebugChannels.Add(new RBuildDebugChannel("combo"));
                Project.DebugChannels.Add(new RBuildDebugChannel("listview"));
                Project.DebugChannels.Add(new RBuildDebugChannel("ntdll"));
                Project.DebugChannels.Add(new RBuildDebugChannel("richedit"));
                Project.DebugChannels.Add(new RBuildDebugChannel("statusbar"));
                Project.DebugChannels.Add(new RBuildDebugChannel("text"));
                Project.DebugChannels.Add(new RBuildDebugChannel("toolbar"));
            }

            if (Project.Languages.Count == 0)
            {
                Project.Languages.Add(new RBuildLanguage("en-us"));
                Project.Languages.Add(new RBuildLanguage("es-es"));
            }

            m_RootTask.PreExecute();
            m_RootTask.Execute();
            m_RootTask.PostExecute();

            SetPlatformDefaults();

            //m_FileHandlers.Add(new TxtSetupFileHandler(this));
            //m_FileHandlers.Add(new SysSetupFileHandler(this));
            //m_FileHandlers.Add(new UnattendFileHandler(this));
            //m_FileHandlers.Add(new DownloaderFileHandler(this));
            //m_FileHandlers.Add(new AutorunFileHandler(this));
            //m_FileHandlers.Add(new HiveAutoGeneratedFileHandler(this));
            m_FileHandlers.ProcessFiles(Project.Files);

            if (RunBackends)
                Backends.Generate();
        }

        private void SetPlatformDefaults()
        {
            if (SetDefaults)
            {
                if (Project.Platform.Modules.Count == 0)
                {
                    foreach (RBuildModule module in Project.Modules)
                    {
                        Project.Platform.Modules.Add(module);
                    }
                }

                if (Project.Languages.Count == 0)
                {
                    foreach (RBuildLanguage language in Project.Languages)
                    {
                        Project.Platform.Languages.Add(language);
                    }
                }

                if (Project.DebugChannels.Count == 0)
                {
                    foreach (RBuildDebugChannel channel in Project.DebugChannels)
                    {
                        Project.Platform.DebugChannels.Add(channel);
                    }
                }
            }
        }

        private void RegisterBackends()
        {
            //m_Backends.Add(new CatalogBackend(this));
            //m_Backends.Add(new MingwBackend(this));
            m_Backends.Add(new HtmlBackend(this));
            //m_Backends.Add(new RGenStatBackend(this));
            //m_Backends.Add(new BaseAddressReportBackend(this));
            //m_Backends.Add(new BuildLogReport(this));
            //m_Backends.Add(new ProjectTreeReport(this));
            m_Backends.Add(new RBuildDBBackend(this));
            //m_Backends.Add(new APIDocumentation(this));
        }

        public void CleanCustomConfigs()
        {
            Directory.SetCurrentDirectory (BaseDirectory);

            if (File.Exists("config.rbuild"))
                File.Delete("config.rbuild");

            if (File.Exists("config-arm.rbuild"))
                File.Delete("config-arm.rbuild");

            if (File.Exists("config-ppc.rbuild"))
                File.Delete("config-ppc.rbuild");
        }

        public BackendCollection Backends
        {
            get { return m_Backends; }
        }

        private void InitializeEnvironment()
        {
            Assembly ass = Assembly.GetExecutingAssembly();

            Properties.AddReadOnly(SYSGEN_PROPERTY_FILENAME, ass.CodeBase);
            Properties.AddReadOnly(SYSGEN_PROPERTY_VERSION, ass.GetName().Version.ToString());
            Properties.AddReadOnly(SYSGEN_PROPERTY_LOCATION, AppDomain.CurrentDomain.BaseDirectory);

            Project.Properties.AddReadOnly("CDOUTPUT", BootCDOutputDirectory + @"\reactos");
            Project.Properties.AddReadOnly("INTERMEDIATE", IntermediateDirectory);
            Project.Properties.AddReadOnly("SOURCECODE", BaseDirectory);
            Project.Properties.AddReadOnly("OUTPUT", OutputDirectory);
            Project.Properties.AddReadOnly("INSTALL", InstallDirectory);
            Project.Properties.AddReadOnly("TEMP", TemporaryDirectory);
            Project.Properties.AddReadOnly("DOC", DocumentationDirectory);
        }

        /// <summary>
        /// Does Execute() and wraps in error handling and time stamping.
        /// </summary>
        /// <returns>Indication of success</returns>
        public bool ReadBuildFiles() 
        {
            //SysGenEngine.OnBuildStarted(this, new BuildEventArgs(_projectName));

            bool success = true;
            try
            {
                // Remember when the build was started
                DateTime startTime = DateTime.Now;

                BuildLog.WriteLine();
                BuildLog.WriteLine("SysGen 0.2");
                BuildLog.WriteLine();
                BuildLog.WriteLine("Running on {0}", Environment.OSVersion.VersionString);
                BuildLog.WriteLine();
                BuildLog.WriteLine("Buildfile:       {0}", BuildFileURI.AbsolutePath);
                BuildLog.WriteLine("Base Directory:  {0}", BaseDirectory);
                BuildLog.WriteLine();
                BuildLog.WriteLine("Reading rbuild files :");
                BuildLog.WriteLine();

                Execute();

                TimeSpan buildTime = DateTime.Now - startTime;

                BuildLog.WriteLine();
                BuildLog.WriteLine("{0} SysGen Module(s) detected.", Project.Modules.Count);
                BuildLog.WriteLine("{0} Platform Module(s) detected.", Project.Platform.Modules.Count);
                BuildLog.WriteLine();
                BuildLog.WriteLine("SysGen COMPLETED in {0} second(s)", (int)buildTime.TotalSeconds);
                BuildLog.WriteLine();

                //SysGenEngine.OnBuildFinished(this, new BuildEventArgs(_projectName));

                success = true;
                return true;

            }
            catch (BuildException e)
            {
                BuildLog.WriteMessage("Build Failed" , "error");
                BuildLog.WriteLine();
                BuildLog.WriteLine(e.Message);

                if (e.InnerException != null)
                {
                    BuildLog.WriteLine( e.InnerException.Message);
                }

                success = false;
                return false;

            }
            catch (Exception e)
            {
                //throw;
                // all other exceptions should have been caught
                string message = "\nINTERNAL ERROR\n" + e.ToString() + "\nPlease send bug report to nant-developers@lists.sourceforge.net";
                BuildLog.WriteMessage(message, "error");
                success = false;
                return false;
            }
            finally
            {
                //SysGenEngine.OnBuildFinished(this, new BuildEventArgs(_projectName));
            }
        }

        /// <summary>Combine with project's <see cref="BaseDirectory"/> to form a full path to file or directory.</summary>
        /// <remarks>
        ///   <para>If it is possible for the <c>path</c> to contain property macros the <c>path</c> call <see cref="ExpandProperties"/> first.</para>
        /// </remarks>
        /// <returns>
        ///   <para>A rooted path.</para>
        /// </returns>
        /// <param name="path">The relative or absolute path.</param>
        public string GetFullPath(string path) {
            if (path == null) {
                return BaseDirectory;
            }

            //Docs above read we should do this. But it should be done before it gets here.
            //path = this.ExpandProperties(path);

            if (!Path.IsPathRooted(path)) {
                path = Path.Combine(BaseDirectory, path);
            }
            return path;
        }

        public string GetRelativePath(string path)
        {
            return path.Replace(BaseDirectory +"\\" , string.Empty);
        }

        /// <summary>
        /// Expands a string from known properties
        /// </summary>
        /// <param name="input">The string with replacement tokens</param>
        /// <returns>The expanded and replaced string</returns>
        public string ExpandProperties(string input)
        {
            string output = input;
            if (input != null)
            {
                //matches ${abc} and $(abc) style properties
                const string pattern = @"\$\{(?<name>([^\}]*))\}|\$\(((?<name>[^\}]*))\)";
                foreach (Match m in Regex.Matches(input, pattern))
                {
                    if (m.Length > 0)
                    {
                        try
                        {
                            string token = m.ToString();
                            string propertyName = m.Groups["name"].Value;

                            if (Project.Properties[propertyName] != null)
                            {
                                output = output.Replace(token, Project.Properties[propertyName].Value);
                            }
                            else
                                throw new BuildException(String.Format("Property '{0}' has not been set!", propertyName));
                        }
                        catch (ArgumentException ae)
                        {
                            throw new BuildException(String.Format("Bad formed property"));
                        }
                    }
                }
            }
            return output;
        }   
    }
}