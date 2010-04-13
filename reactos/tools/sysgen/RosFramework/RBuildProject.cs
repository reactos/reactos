using System;
using System.Text;
using System.Xml;
using System.Collections.Generic;

namespace SysGen.RBuild.Framework
{
    /// <summary>
    /// There can be one project per top-level XML build file. 
    /// A project can only be defined in a top-level xml build file.
    /// </summary>
    public class RBuildProject : RBuildElement
    {
        private RBuildContributorCollection m_Contributors = new RBuildContributorCollection();
        private RBuildModuleCollection m_Modules = new RBuildModuleCollection();
        private RBuildLanguageCollection m_Languages = new RBuildLanguageCollection();
        private RBuildInstallFolderCollection m_InstallFolders = new RBuildInstallFolderCollection();
        private RBuildBuildFamilyCollection m_BuildFamilies = new RBuildBuildFamilyCollection();
        private RBuildPlatform m_Platform = new RBuildPlatform();
        private RBuildDebugChannelCollection m_DebugChannels = new RBuildDebugChannelCollection();

        private string m_PackagesFile = "obj-i386/reactos.dff";
        private string m_MakeFile = "makefile.auto";

        public RBuildProject()
        {
            Folder = new RBuildFolder(PathRoot.SourceCode);
        }

        /// <summary>
        /// Filename of the GNU makefile that is to be created.
        /// </summary>
        public string MakeFile
        {
            get { return m_MakeFile; }
            set { m_MakeFile = value; }
        }

        public string PackagesFile
        {
            get { return m_PackagesFile; }
            set { m_PackagesFile = value; }
        }

        public RBuildDebugChannelCollection DebugChannels
        {
            get { return m_DebugChannels; }
        }

        public RBuildPlatform Platform
        {
            get { return m_Platform; }
            set { m_Platform = value; }
        }

        public RBuildBuildFamilyCollection BuildFamilies
        {
            get { return m_BuildFamilies; }
            set { m_BuildFamilies = value; }
        }

        public RBuildInstallFolderCollection InstallFolders
        {
            get { return m_InstallFolders; }
            set { m_InstallFolders = value; }
        }

        public RBuildContributorCollection Contributors
        {
            get { return m_Contributors; }
            set { m_Contributors = value; }
        }

        public RBuildModuleCollection Modules
        {
            get { return m_Modules; }
        }

        public RBuildLanguageCollection Languages
        {
            get { return m_Languages; }
        }

        public string MakeFileGCCOptions
        {
            get { return string.Format("{0}_GCCOPTIONS", Name); }
        }

        public string MakeFileGCCOptionsMacro
        {
            get { return string.Format("$({0}_GCCOPTIONS)", Name); }
        }

        public override void SaveAs(string projectFile)
        {
            // Creates an XML file is not exist 
            using (XmlTextWriter writer = new XmlTextWriter(projectFile, Encoding.ASCII))
            {
                writer.Indentation = 4;
                writer.Formatting = Formatting.Indented;

                // Starts a new document 
                writer.WriteStartDocument();
                writer.WriteStartElement("project");
                writer.WriteAttributeString("name", Name);
                writer.WriteAttributeString("makefile", MakeFile);
                writer.WriteAttributeString("xmlns", "xi", null, "http://www.w3.org/2001/XInclude");

                /*
                writer.WriteComment("Generic Properties");
                foreach (KeyValuePair<string, string> property in Properties)
                {
                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", property.Key);
                    writer.WriteAttributeString("value", property.Value);
                    writer.WriteEndElement();
                }

                foreach (string define in Defines)
                {
                    writer.WriteStartElement("define");
                    writer.WriteAttributeString("name", define);
                    writer.WriteEndElement();
                }*/

                writer.WriteStartElement("xi:include");
                writer.WriteAttributeString("href", "baseaddress.rbuild");
                writer.WriteEndElement();

                writer.WriteStartElement("xi:include");
                writer.WriteAttributeString("href", "boot/bootdata/bootdata.rbuild");
                writer.WriteEndElement();

                writer.WriteElementString("compilerflag", "-Os");
                writer.WriteElementString("compilerflag", "-ftracer");
                writer.WriteElementString("compilerflag", "-momit-leaf-frame-pointer");
                writer.WriteElementString("compilerflag", "-mpreferred-stack-boundary=2");

                writer.WriteElementString("compilerflag", "-Wno-strict-aliasing");
                writer.WriteElementString("compilerflag", "-Wpointer-arith");
                writer.WriteElementString("linkerflag", "-enable-stdcall-fixup");

                foreach (RBuildFolder folder in IncludeFolders)
                {
                    writer.WriteStartElement("include");
                    writer.WriteAttributeString("root", folder.Root.ToString());
                    writer.WriteString(folder.Name);
                    writer.WriteEndElement();
                }

                foreach (RBuildModule module in Modules)
                {
                    writer.WriteStartElement("xi:include");
                    writer.WriteAttributeString("href", module.RBuildFile);
                    writer.WriteEndElement();
                }

                writer.WriteEndElement(); //Project
                writer.WriteEndDocument();
            }
        }
    }
}
