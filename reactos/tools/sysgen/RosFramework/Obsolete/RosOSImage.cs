using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildOSImage
    {
        private RosPlatform m_Platform = null;
        private RBuildLanguage m_Language = null;
        private RosArchitecture m_Architecture = null;

        private Dictionary<string, string> m_Properties = new Dictionary<string, string>();
        private List<string> m_Defines = new List<string>();
        private List<string> m_Includes = new List<string>();

        private bool m_Debug = false;
        //private bool m_KernelDebugger = false;
        //private bool m_GDBDebugger = false;

        private bool m_MakeBootCD = false;
        private bool m_MakeLiveCD = false;

        public RBuildOSImage()
        {
            Properties.Add("SARCH", "");
            Properties.Add("OARCH", "pentium");
            Properties.Add("OPTIMIZE", "1");
            Properties.Add("MP", "0");
            Properties.Add("KDBG", "0");
            Properties.Add("DBG", "0");
            Properties.Add("GDB", "0");
            Properties.Add("NSWPAT", "0");
            Properties.Add("NTLPC", "1");
            Properties.Add("_WINKD_", "0");

            Defines.Add ("_M_IX86");
            Defines.Add ("_X86_");
            Defines.Add ("__i386__");
            Defines.Add ("_REACTOS_");

            Includes.Add(".");
            Includes.Add("include");
            Includes.Add("include/psdk");
            Includes.Add("include/dxsdk");
            Includes.Add("include/crt");
            Includes.Add("include/ddk");
            Includes.Add("include/GL");
            Includes.Add("include/ndk");
            Includes.Add("include/reactos");
            Includes.Add("include/reactos/libs");
        }

        public bool MakeLiveCD
        {
            get { return m_MakeLiveCD; }
            set { m_MakeLiveCD = value; }
        }

        public bool MakeBootCD
        {
            get { return m_MakeBootCD; }
            set { m_MakeBootCD = value; }
        }

        public RBuildLanguage Language
        {
            get { return m_Language; }
            set { m_Language = value; }
        }

        public RosPlatform Platform
        {
            get { return m_Platform; }
            set { m_Platform = value; }
        }

        public RosArchitecture Architecture
        {
            get { return m_Architecture; }
            set { m_Architecture = value; }
        }

        public Dictionary<string, string> Properties
        {
            get { return m_Properties; }
        }

        public List<string> Defines
        {
            get { return m_Defines; }
        }

        public List<string> Includes
        {
            get { return m_Includes; }
        }

        public string ReleaseType
        {
            get
            {
                if (m_Debug)
                    return "DBG";

                return "RELEASE";
            }
        }

        public string ImageType
        {
            get
            {
                if (m_MakeBootCD)
                    return "BootCD";

                return "LiveCD";
            }
        }

        public void SaveAs(string file)
        {
            // Creates an XML file is not exist 
            using (XmlTextWriter writer = new XmlTextWriter(file, Encoding.ASCII))
            {
                writer.Indentation = 4;
                writer.Formatting = Formatting.Indented;
                
                // Starts a new document 
                writer.WriteStartDocument();
                writer.WriteStartElement("project");
                writer.WriteAttributeString("name", "ReactOS");
                writer.WriteAttributeString("makefile", "makefile.auto");
                writer.WriteAttributeString("xmlns", "xi", null, "http://www.w3.org/2001/XInclude");

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
                }

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

                foreach (string include in Includes)
                {
                    writer.WriteStartElement("include");
                    writer.WriteAttributeString("root", "intermediate");
                    writer.WriteString(include);
                    writer.WriteEndElement();

                    writer.WriteStartElement("include");
                    writer.WriteString(include);
                    writer.WriteEndElement();
                }

                foreach (RBuildModule module in Platform.Modules)
                {
                    writer.WriteStartElement("xi:include");
                    writer.WriteAttributeString("href", module.RBuildFile);
                    writer.WriteEndElement();
                }

                writer.WriteEndElement(); //Project
                writer.WriteEndDocument();
            }

            string a = Name;
        }

        public string Name
        {
            get {
                return "";
                //return string.Format("{0}_{1}{2}_{3}_{4}.iso", 
                //    Platform.SafeName, 
                //    Architecture.SafeName , 
                //    Language.CultureInfo.ThreeLetterISOLanguageName, 
                //    ImageType,
                //    ReleaseType);
            }
        }
    }
}