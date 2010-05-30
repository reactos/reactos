using System;
using System.Collections;
using System.IO;
using System.Diagnostics;
using System.Text;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Framework;

namespace TriStateTreeViewDemo
{
	public class ProjectWriter : XmlTextWriter
	{
		Project m_Project;

		public ProjectWriter(Project project, string filename) : base(filename,Encoding.UTF8)
		{
            m_Project = project;
			Formatting = Formatting.Indented;
		}

        protected Project Project { get { return m_Project; } }

		public void WriteProject()
		{
			WriteStartDocument();
			WriteStartElement("project");
            WriteOutputOptions();
            WriteBuildOptions();
			WriteProjectOptions();
			WriteEndElement();
			WriteEndDocument();
		}

        public void WriteOutputOptions()
        {
            WriteComment(" Output SWF options ");
            WriteStartElement("output");
            WriteOption("movie", "name", m_Project.Name);
            WriteOption("movie", "desc", m_Project.Description);
            WriteEndElement();
        }

        public void WriteBuildOptions()
        {
            WriteComment(" Build options ");
            WriteStartElement("build");

            if (Project.Modules.Count > 0)
            {
                WriteStartElement("modules");
                foreach (RBuildModule module in Project.Modules)
                {
                    WriteStartElement("module");
                    WriteAttributeString("name", module.Name);
                    WriteEndElement();
                }
                WriteEndElement();
            }

            WriteEndElement();

            if (Project.Languages.Count > 0)
            {
                WriteComment(" Build options ");
                WriteStartElement("languages");
                foreach (RBuildLanguage language in Project.Languages)
                {
                    WriteStartElement("language");
                    WriteAttributeString("name", language.Name);
                    WriteEndElement();
                }
                WriteEndElement();
            }

            if (Project.DebugChannels.Count > 0)
            {
                WriteStartElement("debugchhanels");
                foreach (RBuildDebugChannel channel in Project.DebugChannels)
                {
                    WriteStartElement("debugchannel");
                    WriteAttributeString("name", channel.Name);
                    WriteEndElement();
                }
                WriteEndElement();
            }

            WriteStartElement("applications");

            if (m_Project.Shell != null)
                WriteOption("Shell", m_Project.Shell.Name);

            if (m_Project.Screensaver != null)
                WriteOption("Screensaver", m_Project.Screensaver.Name);

            if (m_Project.Wallpaper != null)
                WriteOption("Wallpaper", m_Project.Wallpaper.Name);

            WriteEndElement();
        }

		public void WriteClasspaths()
		{
			WriteComment(" Other classes to be compiled into your SWF ");
			WriteStartElement("classpaths");
			//WritePaths(project.Classpaths,"class");
			WriteEndElement();
		}

		public void WriteCompileTargets()
		{
			WriteComment(" Class files to compile (other referenced classes will automatically be included) ");
			WriteStartElement("compileTargets");
			//WritePaths(project.CompileTargets,"compile");
			WriteEndElement();			
		}

		public void WriteHiddenPaths()
		{
			WriteComment(" Paths to exclude from the Project Explorer tree ");
			WriteStartElement("hiddenPaths");
			//WritePaths(project.HiddenPaths,"hidden");
			WriteEndElement();			
		}

		public void WritePreBuildCommand()
		{
			WriteComment(" Executed before build ");
			WriteStartElement("preBuildCommand");
			if (m_Project.PreBuildEvent.Length > 0)
				WriteString(m_Project.PreBuildEvent);
			WriteEndElement();
		}

		public void WritePostBuildCommand()
		{
			WriteComment(" Executed after build ");
			WriteStartElement("postBuildCommand");
			WriteAttributeString("alwaysRun",m_Project.AlwaysRunPostBuild.ToString());
			if (m_Project.PostBuildEvent.Length > 0)
				WriteString(m_Project.PostBuildEvent);
			WriteEndElement();

		}

		public void WriteProjectOptions()
		{
			WriteComment(" Other project options ");
			WriteStartElement("options");
			WriteOption("debug",m_Project.Debug);
			WriteOption("kdebug",m_Project.KDebug);
            WriteOption("source", @"c:\ros\trunk\reactos" /*project.Source*/);
			WriteEndElement();
		}

        public void WriteOption(string optionName, object optionValue)
        {
            WriteOption("option", optionName, optionValue);
        }

        public void WriteOption(string nodeName, string optionName, object optionValue)
        {
            WriteStartElement(nodeName);
            WriteAttributeString(optionName, optionValue.ToString());
            WriteEndElement();
        }
	}
}
