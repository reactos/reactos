using System;
using System.Collections;
using System.Diagnostics;
using System.Text;
using System.Xml;
using System.Collections.Generic;

using SysGen.RBuild.Framework;

namespace SysGen.Framework.Catalog
{
	public class PlatformCatalogReader
	{
        XmlDocument doc = new XmlDocument();

        RBuildProject m_Project = new RBuildProject();

        public PlatformCatalogReader(string filename)
		{
            doc.Load(filename);
		}

        public RBuildProject Project
        {
            get { return m_Project; }
        }

        public void Read()
        {

            foreach (XmlNode node in doc.SelectSingleNode("/catalog/modules").ChildNodes)
            {
                RBuildModule module = new RBuildModule();

                module.Type = (ModuleType)Enum.Parse(typeof(ModuleType), node.Attributes["type"].Value.ToString());
                module.Name = node.Attributes["name"].Value.ToString();
                module.Folder.Base = node.Attributes["base"].Value.ToString();
                module.CatalogPath = node.Attributes["path"].Value.ToString();
                module.Description = node.Attributes["desc"].Value.ToString();

                m_Project.Modules.Add(module);
            }

            foreach (XmlNode node in doc.SelectSingleNode("/catalog/modules").ChildNodes)
            {
                RBuildModule module = m_Project.Modules.GetByName(node.Attributes["name"].Value.ToString());

                foreach (XmlNode snode in node.SelectSingleNode("libraries").ChildNodes)
                {
                    module.Libraries.Add(m_Project.Modules.GetByName(snode.InnerText));
                }

                foreach (XmlNode snode in node.SelectSingleNode("dependencies").ChildNodes)
                {
                    module.Dependencies.Add(m_Project.Modules.GetByName(snode.InnerText));
                }

                foreach (XmlNode snode in node.SelectSingleNode("requeriments").ChildNodes)
                {
                    module.Requeriments.Add(m_Project.Modules.GetByName(snode.InnerText));
                }
            }

            foreach (XmlNode node in doc.SelectSingleNode("/catalog/languages").ChildNodes)
            {
                RBuildLanguage language = new RBuildLanguage();

                language.Name = node.Attributes["name"].Value.ToString();

                m_Project.Languages.Add(language);
            }

            foreach (XmlNode node in doc.SelectSingleNode("/catalog/debugchannels").ChildNodes)
            {
                RBuildDebugChannel language = new RBuildDebugChannel();

                language.Name = node.Attributes["name"].Value.ToString();

                m_Project.DebugChannels.Add(language);
            }

            /*
            RBuildModuleInfoCollection modules = new RBuildModuleInfoCollection();


            WhitespaceHandling = WhitespaceHandling.None;

            ReadStartElement("modules");
            while (Name == "module")
            {
                RBuildModuleInfo module = new RBuildModuleInfo();

                MoveToFirstAttribute();
                do
                {
                    switch (Name)
                    {
                        case "name":
                            module.Name = Value;
                            break;
                        case "type":
                            module.Type = (ModuleType)Enum.Parse(typeof(ModuleType), Value);
                            break;
                        case "base":
                            module.Base = Value;
                            break;
                        case "desc":
                            module.CatalogPath = Value;
                            break;
                        case "path":
                            module.CatalogPath = Value;
                            break;
                    }

                }
                while (MoveToNextAttribute());

                MoveToElement();

                //Read();
                //Read();

                if (Name == "libraries")
                {
                    if (!IsEmptyElement)
                    {
                        ReadStartElement("libraries");
                        while (Name == "library")
                        {
                            ReadStartElement("library");
                            module.Libraries.Add(ReadContentAsString());
                            ReadEndElement();
                        }
                        ReadEndElement();
                    }
                    else
                        ReadStartElement("libraries");
                }

                if (Name == "dependencies")
                {
                    if (!IsEmptyElement)
                    {
                        ReadStartElement("dependencies");
                        while (Name == "dependency")
                        {
                            ReadStartElement("dependency");
                            module.Dependencies.Add(ReadContentAsString());
                            ReadEndElement();
                        }
                        ReadEndElement();
                    }
                    else
                        ReadStartElement("dependencies");
                }

                if (Name == "requeriments")
                {
                    if (!IsEmptyElement)
                    {
                        ReadStartElement("requeriments");
                        while (Name == "requires")
                        {
                            ReadStartElement("requires");
                            module.Requirements.Add(ReadContentAsString());
                            ReadEndElement();
                        }
                        ReadEndElement();
                    }
                    else
                        ReadStartElement("requeriments");
                }

                modules.Add(module);

                Read();
            }

            ReadEndElement();

            foreach (RBuildModuleInfo moduleInfo in modules)
            {
                RBuildModule module = new RBuildModule();

                module.Type = moduleInfo.Type;
                module.Name = moduleInfo.Name;

                m_Modules.Add(module);
            }

            foreach (RBuildModuleInfo moduleInfo in modules)
            {
                RBuildModule module = m_Modules.GetByName(moduleInfo.Name);

                foreach (string library in moduleInfo.Libraries)
                    module.Libraries.Add(m_Modules.GetByName(library));

                foreach (string library in moduleInfo.Dependencies)
                    module.Dependencies.Add(m_Modules.GetByName(library));

                foreach (string library in moduleInfo.Requirements)
                    module.Requeriments.Add(m_Modules.GetByName(library));
            }*/
        }
	}
}
