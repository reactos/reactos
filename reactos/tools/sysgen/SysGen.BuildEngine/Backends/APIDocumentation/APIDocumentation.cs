using System;
using System.Text.RegularExpressions;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine.Backends
{
    public class APIDocumentation : HtmlDocumenterBaseBacked
    {
        public APIDocumentation(SysGenEngine sysgen)
            : base(sysgen)
        {
            if (Directory.Exists(@"C:\rosLib"))
                Directory.Delete(@"C:\rosLib", true);

            Directory.CreateDirectory(@"C:\rosLib");

            File.Copy(@"c:\style.css", @"c:\rosLib\style.css");
        }

        protected override string FriendlyName
        {
            get { return "APIDocumentation Report"; }
        }

        private void WriteModuleFunctions()
        {
            foreach (RBuildModule module in Project.Modules)
            {
                if (module.IsDLL || module.IsLibrary)
                {
                    foreach (RBuildAPIInfo apiInfo in module.ApiInfo)
                    {
                        using (StreamWriter sw = new StreamWriter(@"C:\roslib\" + apiInfo.HtmlDocFileName))
                        {
                            using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                            {
                                WriteDocumentStart(writer, "Module");

                                writer.RenderBeginTag(HtmlTextWriterTag.H2);
                                writer.Write("{0} Function", apiInfo.Name);
                                writer.RenderEndTag();

                                if (apiInfo.Implemented)
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                                    writer.Write("Function '{0}' on '{1}' is currently implemented.",
                                        apiInfo.Name,
                                        apiInfo.File);
                                    writer.RenderEndTag();
                                }
                                else
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                                    writer.Write("Function '{0}' on '{1}' is currently un-implemented.",
                                        apiInfo.Name,
                                        apiInfo.File);
                                    writer.RenderEndTag();
                                }
                            }
                        }
                    }
                }
            }
        }

        private void WriteModules()
        {
            foreach (RBuildModule module in Project.Modules)
            {
                if (module.IsDLL || module.IsLibrary)
                {
                    using (StreamWriter sw = new StreamWriter(@"C:\roslib\" + module.HtmlDocFileName))
                    {
                        using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                        {
                            WriteDocumentStart(writer, "Module");

                            writer.RenderBeginTag(HtmlTextWriterTag.H2);
                            writer.Write("{0}", module.Name);
                            writer.RenderEndTag();

                            writer.RenderBeginTag(HtmlTextWriterTag.P);
                            writer.Write("Module {0} has a total of {1} functions , {2} implemented and {3} un-implemented ({4}%)",
                                module.Name,
                                module.ApiInfo.TotalFunctions,
                                module.ApiInfo.ImplementedFunctionsCount,
                                module.ApiInfo.UnImplementedFunctionsCount,
                                module.ApiInfo.Percentage);
                            writer.RenderEndTag();

                            writer.RenderBeginTag(HtmlTextWriterTag.H2);
                            writer.Write("{0} Functions", module.Name);
                            writer.RenderEndTag();

                            writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                            foreach (RBuildAPIInfo apiInfo in module.ApiInfo)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.AddAttribute("href", apiInfo.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(apiInfo.Name);
                                writer.RenderEndTag();

                                if (!apiInfo.Implemented)
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.B);
                                    writer.Write("(UnImplemented)");
                                    writer.RenderEndTag();
                                }

                                writer.RenderEndTag();
                            }
                            writer.RenderEndTag();

                            WriteDocumentEnd(writer);
                        }
                    }
                }
            }
        }

        private void WriteWelcome()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\roslib\welcome.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Warnings");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("ReactOS API Documentation");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Welcome to the ReactOS API documentation website.");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H1);
                    writer.Write("Implementation status Color Key");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Cellpadding, "5");
                    writer.AddAttribute(HtmlTextWriterAttribute.Cellspacing, "0");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    for (int i = 0; i <= 100; i = i + 5)
                    {
                        writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct" + i);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write("{0}%" , i);
                        writer.RenderEndTag();            
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Native DLLs");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Modules)
                    {
                        if (module.Type == ModuleType.NativeDLL)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute("href", module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Win32 DLLs");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Modules)
                    {
                        if (module.Type == ModuleType.Win32DLL)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute("href", module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void WriteHeader()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\roslib\header.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    writer.WriteLine("<h1>ReactOS API Documentation</h1>");
                }
            }
        }

        private void WriteFrameSet()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\roslib\default.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    writer.WriteLine("<frameset rows='75,*' frameborder='0' border='1'>");
                    writer.WriteLine("<frame src='header.htm' name='Header' id='header' scrolling='no' noresize='true' />");
                    writer.WriteLine("<frameset cols='35%,65%' frameborder='1' border='1'>");
                    writer.WriteLine("<frame src='tree.htm' name='Tree' />");
                    writer.WriteLine("<frame src='welcome.htm' name='content' />");
                    writer.WriteLine("</frameset>");
                    writer.WriteLine("</frameset>");
                }
            }
        }

        private void ReadApiStatusFile()
        {
            XmlDocument doc = new XmlDocument();

            //Load the file in to memory
            doc.Load(@"C:\Ros\clean\reactos\apistatus.xml");

            foreach (XmlNode comp in doc.SelectNodes("/components/component"))
            {
                // Get the component name....
                string modulename = comp.Attributes["name"].InnerText;

                RBuildModule module = Project.Modules.GetByName(modulename);

                if (module != null)
                {
                    foreach (XmlNode dep in comp.SelectNodes("functions/f"))
                    {
                        // Gets the dependency name
                        string name = dep.Attributes["n"].InnerText;
                        string file = dep.Attributes["f"].InnerText;
                        bool imp = Boolean.Parse(dep.Attributes["i"].InnerText);

                        RBuildAPIInfo apiInfo = new RBuildAPIInfo();

                        apiInfo.Name = name;
                        apiInfo.File = file;
                        apiInfo.Implemented = imp;

                        module.ApiInfo.Add(apiInfo);
                    }
                }
            }
        }

        protected override void Generate()
        {
            ReadApiStatusFile();
            WriteFrameSet();
            WriteHeader();
            WriteWelcome();
            WriteModules();
            WriteModuleFunctions();

            using (StreamWriter sw = new StreamWriter(@"C:\roslib\tree.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Warnings");

                    writer.AddAttribute(HtmlTextWriterAttribute.Cellpadding, "2");
                    writer.AddAttribute(HtmlTextWriterAttribute.Cellspacing, "2");
                    writer.AddAttribute(HtmlTextWriterAttribute.Border, "0");
                    //writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    foreach (RBuildModule module in Project.Modules)
                    {
                        if ((module.IsDLL) || (module.IsLibrary))
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);

                            if ((module.ApiInfo.Percentage >= 0) && (module.ApiInfo.Percentage <= 5))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct0");
                            if ((module.ApiInfo.Percentage >= 5) && (module.ApiInfo.Percentage <= 10))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct5");
                            if ((module.ApiInfo.Percentage >= 10) && (module.ApiInfo.Percentage <= 15))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct10");
                            if ((module.ApiInfo.Percentage >= 15) && (module.ApiInfo.Percentage <= 20))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct15");
                            if ((module.ApiInfo.Percentage >= 20) && (module.ApiInfo.Percentage <= 25))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct20");
                            if ((module.ApiInfo.Percentage >= 25) && (module.ApiInfo.Percentage <= 30))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct25");
                            if ((module.ApiInfo.Percentage >= 30) && (module.ApiInfo.Percentage <= 35))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct30");
                            if ((module.ApiInfo.Percentage >= 35) && (module.ApiInfo.Percentage <= 40))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct35");
                            if ((module.ApiInfo.Percentage >= 40) && (module.ApiInfo.Percentage <= 45))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct40");
                            if ((module.ApiInfo.Percentage >= 45) && (module.ApiInfo.Percentage <= 50))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct45");
                            if ((module.ApiInfo.Percentage >= 50) && (module.ApiInfo.Percentage <= 55))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct50");
                            if ((module.ApiInfo.Percentage >= 55) && (module.ApiInfo.Percentage <= 60))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct55");
                            if ((module.ApiInfo.Percentage >= 60) && (module.ApiInfo.Percentage <= 65))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct60");
                            if ((module.ApiInfo.Percentage >= 65) && (module.ApiInfo.Percentage <= 70))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct65");
                            if ((module.ApiInfo.Percentage >= 70) && (module.ApiInfo.Percentage <= 75))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct70");
                            if ((module.ApiInfo.Percentage >= 75) && (module.ApiInfo.Percentage <= 80))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct75");
                            if ((module.ApiInfo.Percentage >= 80) && (module.ApiInfo.Percentage <= 85))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct80");
                            if ((module.ApiInfo.Percentage >= 85) && (module.ApiInfo.Percentage <= 90))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct85");
                            if ((module.ApiInfo.Percentage >= 90) && (module.ApiInfo.Percentage <= 95))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct90");
                            if ((module.ApiInfo.Percentage >= 95) && (module.ApiInfo.Percentage <= 100))
                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "pct100");

                            writer.RenderBeginTag(HtmlTextWriterTag.Td);

                            if (module.ApiInfo.Count > 0)
                            {
                                writer.AddAttribute("href", module.HtmlDocFileName);
                                writer.AddAttribute("target", "content");
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(module.Name);
                                writer.RenderEndTag();

                                writer.Write(" (I:{0} U:{1} P:{2}%)",
                                    module.ApiInfo.ImplementedFunctionsCount,
                                    module.ApiInfo.UnImplementedFunctionsCount,
                                    module.ApiInfo.Percentage);
                            }
                            else
                            {
                                writer.Write(module.Name);
                            }

                            writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                            if (module.ApiInfo.Count > 0)
                            {
                                foreach (RBuildAPIInfo apiInfo in module.ApiInfo)
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                    writer.AddAttribute("href", apiInfo.HtmlDocFileName);
                                    writer.AddAttribute("target", "content");
                                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                                    writer.Write(apiInfo.Name);
                                    writer.RenderEndTag();
                                    writer.RenderEndTag();
                                }
                            }
                            else
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.Write("No documentation available yet");
                                writer.RenderEndTag();
                            }
                            writer.RenderEndTag();


                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }
    }
}
