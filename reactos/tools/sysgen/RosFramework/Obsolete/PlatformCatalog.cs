using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class PlatformCatalog
    {
        public List<RosPlatform> m_Platform = new List<RosPlatform>();
        public SoftwareCatalog m_SoftwareCatalog = new SoftwareCatalog();

        public PlatformCatalog()
        {
        }

        public List<RosPlatform> Platforms
        {
            get { return m_Platform; }
        }

        public SoftwareCatalog SoftwareCatalog
        {
            get { return m_SoftwareCatalog; }
            set { m_SoftwareCatalog = value; }
        }

        public RosPlatform GetPlatformByName(string name)
        {
            foreach (RosPlatform platform in Platforms)
            {
                if (platform.Name == name)
                {
                    return platform;
                }
            }

            throw new Exception("Platform not found in catalog");
        }

        public void LoadFromFile(string file)
        {
            XmlDocument doc = new XmlDocument();

            //Load the file in to memory
            doc.Load(file);

            //Load modules ...
            foreach (XmlNode node in doc.SelectNodes("/platforms/platform"))
            {
                RosPlatform platform = new RosPlatform();

                platform.Name = node.Attributes["name"].InnerText;

                if (node.Attributes["base"] != null)
                {
                    platform.Base = node.Attributes["base"].InnerText;
                }

                m_Platform.Add(platform);
            }

            foreach (XmlNode comp in doc.SelectNodes("/platforms/platform"))
            {
                // Get the component name....
                string name = comp.Attributes["name"].InnerText;

                RosPlatform platform = GetPlatformByName(name);

                if (platform.Base != null)
                {
                    platform.ParentPlatform = GetPlatformByName(platform.Base);

                    foreach (RBuildModule module in platform.ParentModules)
                    {
                        platform.Modules.Add(module);
                    }
                }

                foreach (XmlNode dep in comp.SelectNodes("modules/module"))
                {
                    // Gets the dependency name
                    name = dep.Attributes["name"].InnerText;

                    RBuildModule module = SoftwareCatalog.Modules.GetByName(name);

                    platform.Modules.Add(module);
                }
            }
        }
    }
}
