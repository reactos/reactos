using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class SoftwareCatalog
    {
        public RBuildModuleCollection m_Modules = new RBuildModuleCollection();

        public SoftwareCatalog()
        {
        }

        public RBuildModuleCollection Modules
        {
            get { return m_Modules; }
        }

        public void LoadFromFile(string file)
        {
            XmlDocument doc = new XmlDocument();

            //Load the file in to memory
            doc.Load(file);

            //Load modules ...
            foreach (XmlNode node in doc.SelectNodes("/modules/module"))
            {
                RBuildModule module;
                
                module = new RBuildModule();
                module.Metadata = new RBuildMetadata();

                module.Name = node.Attributes["name"].Value;
                //module.Base = node.Attributes["base"].Value;
                module.Metadata.Description = node.Attributes["desc"].Value;
                //module.Rbuild = node.Attributes["rbuild"].Value;

                /*
                foreach (XmlNode dep in node.SelectNodes("provides/provide"))
                {
                    string value = dep.SelectSingleNode("value").InnerText;
                    string type = dep.SelectSingleNode("type").InnerText;

                    module.Provides.Add(value, type);
                }*/

                m_Modules.Add(module);
            }

            foreach (XmlNode comp in doc.SelectNodes("/modules/module"))
            {
                // Get the component name....
                string componentName = comp.Attributes["name"].Value;

                foreach (XmlNode dep in comp.SelectNodes("dependencies/dependency"))
                {
                    // Gets the dependency name
                    string dependencyName = dep.Attributes["name"].Value;

                    foreach (RBuildModule dependency in m_Modules)
                    {
                        if (dependency.Name == dependencyName)
                        {
                            foreach (RBuildModule module in m_Modules)
                            {
                                if (module.Name == componentName)
                                {
                                    module.Libraries.Add(dependency);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
