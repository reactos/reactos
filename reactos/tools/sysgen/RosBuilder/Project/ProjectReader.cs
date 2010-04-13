using System;
using System.Windows.Forms;
using System.Collections;
using System.Diagnostics;
using System.Text;
using System.Xml;
using System.Collections.Generic;

using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
	public class ProjectReader : XmlTextReader
	{
        Project project;

        public ProjectReader(Project project, string filename)
            : base(filename)
		{
            this.project = project;
			WhitespaceHandling = WhitespaceHandling.None;
		}

        protected Project Project { get { return project; } }

        public virtual Project ReadProject()
		{
			MoveToContent();

            while (Read())
                ProcessNode(Name);

			return project;
		}

        private void ReadModules()
        {
            RBuildModule module = null;

            ReadStartElement("modules");
            while (Name == "module")
            {
                try
                {
                    module = project.RBuildProject.Modules.GetByName(GetAttribute("name"));

                    if (module == null)
                        throw new Exception("Unkown module '" + Value + "'");

                    project.Modules.Add(module);

                    //project.SysGenDesinger.PlatformController.Project.Platform.Modules.Add(module);
                }
                catch (Exception e)
                {
                    MessageBox.Show(
                        e.Message,
                        "Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }

                //project.Modules.Add(GetAttribute("name"));

                // continue reading
                Read();
            }

            ReadEndElement();
        }

        private void ReadLanguages()
        {
            RBuildLanguage language = null;

            ReadStartElement("languages");
            while (Name == "language")
            {
                try
                {
                    language = project.RBuildProject.Languages.GetByName(GetAttribute("name"));

                    if (language == null)
                        throw new Exception("Unkown language '" + Value + "'");

                    project.Languages.Add(language);
                }
                catch (Exception e)
                {
                    MessageBox.Show(e.Message,
                        "Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }

                // continue reading
                Read();
            }

            ReadEndElement();
        }

        //private void ReadRSLPaths()
        //{
        //    //project.CompilerOptions.RSLPaths = ReadLibrary("rslPaths");
        //}

        //private void ReadExternalLibraryPaths()
        //{
        //    //project.CompilerOptions.ExternalLibraryPaths = ReadLibrary("externalLibraryPaths");
        //}

        //private void ReadLibrayPath()
        //{
        //    //project.CompilerOptions.LibraryPaths = ReadLibrary("libraryPaths");
        //}

        //private void ReadIncludeLibraries()
        //{
        //    //project.CompilerOptions.IncludeLibraries = ReadLibrary("includeLibraries");
        //}

        //private string[] ReadLibrary(string name)
        //{
        //    ReadStartElement(name);
        //    List<string> elements = new List<string>();
        //    while (Name == "element")
        //    {
        //        elements.Add(GetAttribute("path"));
        //        Read();
        //    }
        //    ReadEndElement();
        //    string[] result = new string[elements.Count];
        //    elements.CopyTo(result);
        //    return result;
        //}

        public void ReadApplications()
        {
            ReadStartElement("applications");
            while (Name == "option")
            {
                MoveToFirstAttribute();
                switch (Name)
                {
                    case "shell":
                        project.Shell = Project.RBuildProject.Modules.GetByName(Value);
                        break;
                    case "screensaver":
                        project.Screensaver = Project.RBuildProject.Modules.GetByName(Value);
                        break;
                    case "wallpaper":
                        //project.Wallpaper = Value;
                        break;
                }
                Read();
            }
            ReadEndElement();
        }

        //protected virtual void ProcessNode(string name)
        //{
        //    switch (name)
        //    {
        //        case "output": ReadOutputOptions(); break;
        //    //    case "classpaths": ReadClasspaths(); break;
        //    //    case "compileTargets": ReadCompileTargets(); break;
        //    //    case "hiddenPaths": ReadHiddenPaths(); break;
        //    //    case "preBuildCommand": ReadPreBuildCommand(); break;
        //    //    case "postBuildCommand": ReadPostBuildCommand(); break;
        //    //    case "options": ReadProjectOptions(); break;
        //    }
        //}

        // process AS3-specific stuff
        protected virtual void ProcessNode(string name)
        {
            if (NodeType == XmlNodeType.Element)
            {
                switch (name)
                {
                    //case "build": ReadBuildOptions(); break;
                    //case "includeLibraries": ReadIncludeLibraries(); break;
                    //case "libraryPaths": ReadLibrayPath(); break;
                    //case "externalLibraryPaths": ReadExternalLibraryPaths(); break;
                    case "modules":
                        ReadModules();
                        break;
                    case "languages":
                        ReadLanguages();
                        break;
                    case "applications":
                        ReadApplications();
                        break;
                    case "output": 
                        ReadOutputOptions(); 
                        break;
                    case "options":
                        ReadProjectOptions();
                        break;
                    //default:
                    //    base.ProcessNode(name); break;
                }
            }
        }

        public void ReadOutputOptions()
        {
            ReadStartElement("output");
            while (Name == "movie")
            {
                MoveToFirstAttribute();
                switch (Name)
                {
                    case "name": 
                        project.Name = Value; 
                        break;
                    case "desc": 
                        project.Description = Value; 
                        break;
                    //case "path": project.OutputPath = OSPath(Value); break;
                    //case "fps": project.MovieOptions.Fps = IntValue; break;
                    //case "width": project.MovieOptions.Width = IntValue; break;
                    //case "height": project.MovieOptions.Height = IntValue; break;
                    //case "version": project.MovieOptions.Version = IntValue; break;
                    //case "background": project.MovieOptions.Background = Value; break;
                }
                Read();
            }
            ReadEndElement();
        }

        //public void ReadClasspaths()
        //{
        //    ReadStartElement("classpaths");
        //    //ReadPaths("class",project.Classpaths);
        //    ReadEndElement();
        //}

        //public void ReadCompileTargets()
        //{
        //    ReadStartElement("compileTargets");
        //    //ReadPaths("compile",project.CompileTargets);
        //    ReadEndElement();
        //}

        //public void ReadHiddenPaths()
        //{
        //    ReadStartElement("hiddenPaths");
        //    //ReadPaths("hidden",project.HiddenPaths);
        //    ReadEndElement();
        //}

        //public void ReadPreBuildCommand()
        //{
        //    if (!IsEmptyElement)
        //    {
        //        ReadStartElement("preBuildCommand");
        //        project.PreBuildEvent = OSPath(ReadString().Trim());
        //        ReadEndElement();
        //    }
        //}

		public void ReadPostBuildCommand()
		{
            //project.AlwaysRunPostBuild = Convert.ToBoolean(GetAttribute("alwaysRun"));

            //if (!IsEmptyElement)
            //{
            //    ReadStartElement("postBuildCommand");
            //    project.PostBuildEvent = OSPath(ReadString().Trim());
            //    ReadEndElement();
            //}
		}

		public void ReadProjectOptions()
		{
			ReadStartElement("options");
			while (Name == "option")
			{
				MoveToFirstAttribute();
				switch (Name)
				{
					case "debug": 
                        project.Debug = BoolValue; 
                        break;
                    case "kdebug": 
                        project.KDebug = BoolValue; 
                        break;
                    case "source":
                        project.Source = Value;
                        break;
				}
				Read();
			}
			ReadEndElement();
		}

		public bool BoolValue { get { return Convert.ToBoolean(Value); } }
		public int IntValue { get { return Convert.ToInt32(Value); } }

        //public void ReadPaths(string pathNodeName, IAddPaths paths)
        //{
        //    while (Name == pathNodeName)
        //    {
        //        paths.Add(OSPath(GetAttribute("path")));
        //        Read();
        //    }
        //}

        //protected string OSPath(string path)
        //{
        //    if (path != null)
        //        return path.Replace('\\',System.IO.Path.DirectorySeparatorChar);
        //    else
        //        return null;
        //}
	}
}
