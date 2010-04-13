using System;
using System.Xml;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("project")]
    public class ProjectTask : TaskContainer, ISysGenObject
    {
        RBuildProject _project = new RBuildProject();

        public ProjectTask()
        {
        }

        [TaskAttribute("name", Required = true)]
        public string Name
        {
            get { return _project.Name; }
            set { _project.Name = value; }
        }

        [TaskAttribute("makefile", Required = true)]
        public string MakeFile
        {
            get { return _project.MakeFile; }
            set { _project.MakeFile = value; }
        }

        public RBuildProject Project
        {
            get { return _project; }
        }

        public RBuildElement RBuildElement
        {
            get { return _project; }
        }

        protected override void OnInit()
        {
            SysGen.Project = Project;
            SysGen.RootTask = this;
            Project.RBuildFile = RBuildFile;

            
            Project.Properties.Add("ARCH", "i386");/*
            Project.Defines.Add("_M_IX86");
            Project.Defines.Add("_X86_");
            Project.Defines.Add("__i386__");*/
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            Project.Folder = new RBuildFolder(PathRoot.SourceCode , "");
        }

        protected override void PreExecuteTask()
        {
            ///Project.Base = string.Empty;
            Project.Path = SysGen.BaseDirectory;
            Project.XmlFile = XmlFile;
        }
    }
}

