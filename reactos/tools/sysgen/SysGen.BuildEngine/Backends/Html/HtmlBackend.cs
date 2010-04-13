using System;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Drawing;

using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class HtmlBackend : HtmlDocumenterBaseBacked
    {
        public HtmlBackend(SysGenEngine sysgen)
            : base(sysgen)
        {
            try
            {
                if (Directory.Exists(@"C:\rosDoc"))
                    Directory.Delete(@"C:\rosDoc", true);

                Directory.CreateDirectory(@"C:\rosDoc");

                File.Copy(@"c:\style.css", @"c:\rosDoc\style.css");
            }
            catch (Exception)
            {
            }
        }

        protected override string FriendlyName
        {
            get { return "HTML Report"; }
        }

        private void GenerateFrontPage()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\default.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "RBuild Auto Documentation");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Modules");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "platform.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Platform");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Quick platform overview.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "modules.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Modules");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("ReactOS is a modular operating system, made of components that collaborate with each other.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "baseaddresses.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Base Addresses");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Memory address serving as a reference point for other addresses.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "dllbaseaddresses.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("DLL Base Addresses");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Base addresses used in ReactOS dlls.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "properties.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Properties");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Properties used during the build process.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "defines.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Project Defines");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Global defines.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "depmap.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Dependency map");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Graphically represents interdependencies between modules.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "files.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Files");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Files included for current platform.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "installfolders.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Install Folders");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Install folders created during ReactOS setup.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "installfiles.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Install Files");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Install files created during ReactOS setup.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "authors.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Authors");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Individuals who have contributed time and energy to supporting the ReactOS Project.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "unicodemodules.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Unicode");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Modules supporting UNICODE builds");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "localizations.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Localizations");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Languages and cultures currently supported by ReactOS");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "codestats.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Stats");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Basic ReactOS code base statistics");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "warnings.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("RBuild Warnings");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Warnings and inconsistencies detected by rbuild.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "buildsummary.htm");
                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                    writer.Write("Build Summary");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write("Options and build settings per module.");
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateDependencyMap()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\depmap.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Module Dependency Map");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Module Dependency Map");
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.B);
                    writer.Write("Direct Dependencies :");
                    writer.RenderEndTag();
                    writer.Write("Dependencies and libraries this module is using");

                    writer.RenderBeginTag(HtmlTextWriterTag.Br);
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.B);
                    writer.Write("Full Dependencies :");
                    writer.RenderEndTag();
                    writer.Write("Dependencies and libraries this module and it's dependencies are using");

                    SysGenDependencyTracker dependencyTracker = null;

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        dependencyTracker = new SysGenDependencyTracker(Project, module);

                        writer.RenderBeginTag(HtmlTextWriterTag.H2);
                        writer.Write("{0}", module.Name);
                        writer.RenderEndTag();

                        writer.AddAttribute("name", module.Name);
                        writer.RenderBeginTag(HtmlTextWriterTag.A);
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Blockquote);
                        writer.RenderBeginTag(HtmlTextWriterTag.H2);
                        writer.Write("Direct Dependencies");
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                        foreach (RBuildModule dependency in module.Needs)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, "#" + dependency.Name);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write("{0} - ({1} Dependencies , {2} Libraries , {3} Requeriments)", 
                                dependency.Name,
                                dependency.Dependencies.Count, 
                                dependency.Libraries.Count,
                                dependency.Requeriments.Count);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.H2);
                        writer.Write("Full Dependencies");
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                        foreach (RBuildModule dependency in dependencyTracker.DependsOn)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, "#" + dependency.Name);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write("{0} - ({1} Dependencies , {2} Libraries , {3} Requeriments)",
                                dependency.Name,
                                dependency.Dependencies.Count,
                                dependency.Libraries.Count,
                                dependency.Requeriments.Count);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }
                }
            }
        }

        private void GenerateModulesBuildSummary()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\buildsummary.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Modules");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Modules");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Libraries");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Dependencies");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("CFLAGS");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("LFLAGS");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                        writer.RenderBeginTag(HtmlTextWriterTag.A);
                        writer.Write(module.Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Base);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Type);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                        
                        foreach (RBuildModule library in module.Libraries)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, library.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(library.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag(); 
                        }

                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);

                        foreach (RBuildModule dependency in module.Dependencies)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(dependency.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);

                        foreach (string flag in module.CompilerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(flag);
                            writer.RenderEndTag();
                        }

                        foreach (string flag in Project.CompilerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(flag);
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);

                        foreach (string flag in module.LinkerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(flag);
                            writer.RenderEndTag();
                        }

                        foreach (string flag in Project.LinkerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(flag);
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateModules()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\modules.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Modules");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Modules");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Unicode");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("C++");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("PCH");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Target");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Install Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Install Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("RBuild");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                        writer.RenderBeginTag(HtmlTextWriterTag.A);
                        writer.Write(module.Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Base);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Type);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Unicode);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.CPlusPlus);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.PCH);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.TargetName);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.InstallBase);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.InstallName);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.RBuildPath);
                        writer.RenderEndTag();

                        writer.RenderEndTag();
                    }
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateStats()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\codestats.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Stats");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Stats");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Modules");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.Write(Project.Modules.Count);
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateWarnings()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\warnings.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Warnings");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Warnings");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Unicode == false)
                        {
                            if ((module.Defines.IsDefined("UNICODE")) ||
                                (module.Defines.IsDefined("_UNICODE")) ||
                                (module.Defines.IsDefined("_UNICODE_")))
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.WriteLine("Module '{0}' has unicode defines but 'Unicode' property set to 'False'", module.Name);
                                writer.RenderEndTag();
                            }
                        }

                        foreach (RBuildDefine define in Project.Defines)
                        {
                            if (module.Defines.IsDefined(define.Name))
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.WriteLine("Module '{0}' defines '{1}' already inherited from project ", module.Name, define.Name);
                                writer.RenderEndTag();
                            }
                        }

                        foreach (string flag in Project.CompilerFlags)
                        {
                            if (module.CompilerFlags.Contains(flag))
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.WriteLine("Module '{0}' has compiler flag '{1}' already inherited from project ", module.Name, flag);
                                writer.RenderEndTag();
                            }
                        }

                        foreach (string flag in Project.LinkerFlags)
                        {
                            if (module.LinkerFlags.Contains(flag))
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.WriteLine("Module '{0}' has linker flag '{1}' already inherited from project ", module.Name, flag);
                                writer.RenderEndTag();
                            }
                        }

                        foreach (RBuildFolder include in module.IncludeFolders)
                        {
                            if (Project.IncludeFolders.Contains(include))
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                writer.WriteLine("Module '{0}' includes folder '({1}){2}' already inherited from project ", module.Name, include.Root, include.FullPath);
                                writer.RenderEndTag();
                            }

                            if (include.Root == PathRoot.Default ||
                                include.Root == PathRoot.SourceCode)
                            {
                                if (SysGen.RBuildFolderExists(include) == false)
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                    writer.WriteLine("Module '{0}' includes folder '({1}){2}' which could not be found ", module.Name, include.Root, include.FullPath);
                                    writer.RenderEndTag();
                                }
                            }
                        }
                    }
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateFiles()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\files.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Modules");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Files");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Path");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Root");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Install Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("New Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (RBuildOutputFile file in Project.Files)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.Base);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.FullPath);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.Root);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.InstallBase);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.NewFile.FullPath);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(file.GetType().Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateBaseAddresses()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\baseaddresses.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "BaseAdress");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Base Adress");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Value");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildProperty property in SysGen.Project.Properties)
                    {
                        if (property is RBuildBaseAdress)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(property.Name);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(property.Value);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateDllBaseAddresses()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\dllbaseaddresses.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Dll's base addresses");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Dll's base addresses");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base Adress");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Is Default");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.IsDLL)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Name);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Base);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Type);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.BaseAddress);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.IsDefaultBaseAdress);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                }
            }
        }

        private void GenerateDefines()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\defines.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Project Defines");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Project Defines");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Value");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (RBuildDefine define in Project.Defines)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(define.Name);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(define.Value);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateProperties()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\properties.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Properties");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Properties");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Value");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Read-Only");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Internal");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (RBuildProperty property in SysGen.Project.Properties)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(property.Name);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(property.Value);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(property.ReadOnly);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(property.Internal);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(property.GetType().Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateInstallFiles()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\installfiles.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Install Files");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Install Files");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("ID");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Enabled)
                        {
                            if (module.IsInstallable)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.InstallBase);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.TargetFile.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }
                        }

                        foreach (RBuildOutputFile file in module.Files)
                        {
                            RBuildPlatformFile platformFile = file as RBuildPlatformFile;

                            if (platformFile != null)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(platformFile.InstallBase);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(platformFile.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }
                        }
                    }

                    foreach (RBuildOutputFile file in Project.Files)
                    {
                        RBuildPlatformFile platformFile = file as RBuildPlatformFile;

                        if (platformFile != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(platformFile.InstallBase);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(platformFile.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateInstallFolders()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\installfolders.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Folders");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Install Folders");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("ID");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildInstallFolder folder in Project.InstallFolders)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(folder.ID);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(folder.Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateAuthors()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\authors.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Authors");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("ReactOS Authors");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Alias");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Mail");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("City");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Country");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Active");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildContributor contributor in Project.Contributors)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);

                        if (contributor.Alias != null)
                        {
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, contributor.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(contributor.Alias);
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(contributor.FullName);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(contributor.Mail);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(contributor.City);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(contributor.Country);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(contributor.Active);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateContributors()
        {
            foreach (RBuildContributor contributor in Project.Contributors)
            {
                if (contributor.Alias != null)
                {
                    using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\" + contributor.HtmlDocFileName))
                    {
                        using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                        {
                            WriteDocumentStart(writer, "Authors");

                            writer.RenderBeginTag(HtmlTextWriterTag.H2);
                            writer.Write("ReactOS Authors");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Module");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Roles");
                            writer.RenderEndTag();

                            writer.RenderEndTag();

                            foreach (RBuildModule module in Project.Platform.Modules)
                            {
                                if (module.Authors.GetByName(contributor.Alias) != null)
                                {
                                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                    writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                                    writer.RenderBeginTag(HtmlTextWriterTag.A);
                                    writer.Write(module.Name);
                                    writer.RenderEndTag();
                                    writer.RenderEndTag();

                                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                                    foreach (RBuildAuthor author in module.Authors)
                                    {
                                        if (author.Contributor == contributor)
                                        {
                                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                                            writer.Write(author.Role);
                                            writer.RenderEndTag();
                                        }
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

        private void GenerateLocalizations()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\localizations.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Translations");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Available Languages");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("ID");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Name");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("ThreeLetter ISO");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (RBuildLanguage language in Project.Languages)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(language.Name);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(language.Name);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(language.Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Localization by Module");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();

                    foreach (RBuildLanguage language in Project.Languages)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write(language.Name);
                        writer.RenderEndTag();
                    }

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                        writer.RenderBeginTag(HtmlTextWriterTag.A);
                        writer.Write(module.Name);
                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        foreach (RBuildLanguage language in Project.Languages)
                        {
                            RBuildLocalizationFile localization = module.LocalizationFiles.GetByName(language.Name);

                            if (localization != null)
                            {
                                if (localization.Dirty)
                                {
                                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "Red");
                                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                    writer.Write("Yes");
                                    writer.RenderEndTag();
                                }
                                else
                                {
                                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "Green");
                                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                    writer.Write("Yes");
                                    writer.RenderEndTag();
                                }
                            }
                            else
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write("No");
                                writer.RenderEndTag();
                            }
                        }

                        writer.RenderEndTag();
                    }
                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GenerateModulePages()
        {
            foreach (RBuildModule module in Project.Platform.Modules)
            {
                using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\" + module.HtmlDocFileName))
                {
                    // Creates an XML file is not exist 
                    using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                    {
                        WriteDocumentStart(writer, module.Name);

                        writer.RenderBeginTag(HtmlTextWriterTag.H2);
                        writer.Write("Module : {0}", module.Name);
                        writer.RenderEndTag();

                        writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                        writer.RenderBeginTag(HtmlTextWriterTag.Table);
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Base");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Base);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Entry Point");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.EntryPoint);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("C++");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.CPlusPlus);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Base Adress");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.BaseAddress);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Unicode");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Unicode);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Target Name");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.TargetName);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Type");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(module.Type.ToString());
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        if (module.Families.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Families this module belong");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildFamily family in module.Families)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, "");
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(family.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Folders.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Folders");
                            writer.RenderEndTag();
                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildFolder folder in module.Folders)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(folder.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(folder.FullPath);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.IsBuildable)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Output Files");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("File");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            if (module.TargetFile != null)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.TargetFile.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.TargetFile.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.TargetFile.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            if (module.Install != null)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.Install.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.Install.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.Install.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            if (module.PlatformInstall != null)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PlatformInstall.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PlatformInstall.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PlatformInstall.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Authors.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Authors");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Alias");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Full Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Mail");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Role");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildAuthor author in module.Authors)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, author.Contributor.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(author.Contributor.Alias);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(author.Contributor.FullName);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(author.Contributor.Mail);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(author.Role);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Metadata != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Metadata");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Description");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Metadata.Description);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        if (module.AutoRegister != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("COM Auto register");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Register Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.AutoRegister.Type.ToString());
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("SysSetup INF Section");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.AutoRegister.InfSection);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        if (module.ImportLibrary != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("DLL Import");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.ImportLibrary.Root);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.ImportLibrary.Base);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Definition");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.ImportLibrary.Definition);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Import Dll Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.ImportLibrary.DllName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        if (module.Bootstrap != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("CD Bootstrap");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Install Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Bootstrap.InstallBase);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Path");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Bootstrap.FullPath);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("New name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Bootstrap.NewName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        if (module.Dependencies.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Dependencies");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildModule dependency in module.Dependencies)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(dependency.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Libraries.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Libraries");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildModule dependency in module.Libraries)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(dependency.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Requeriments.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Requeriments");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildModule dependency in module.Requeriments)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(dependency.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(Project,module);

                        if (dependencyTracker.DependsOn.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Depdends On");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildModule dependency in dependencyTracker.DependsOn)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(dependency.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (dependencyTracker.DependencyOf.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Dependency Of");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildModule dependency in dependencyTracker.DependencyOf)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.AddAttribute(HtmlTextWriterAttribute.Href, dependency.HtmlDocFileName);
                                writer.RenderBeginTag(HtmlTextWriterTag.A);
                                writer.Write(dependency.Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(dependency.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        writer.RenderBeginTag(HtmlTextWriterTag.H3);
                        writer.Write("Include Folders");
                        writer.RenderEndTag();

                        writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                        writer.RenderBeginTag(HtmlTextWriterTag.Table);

                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Root");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Base");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Inherited");
                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        foreach (RBuildFolder folder in module.IncludeFolders)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(folder.Root);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(folder.FullPath);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.FalseString);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        foreach (RBuildFolder folder in Project.IncludeFolders)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(folder.Root);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(folder.FullPath);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.TrueString);
                            writer.RenderEndTag();

                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();

                        if (module.LocalizationFiles.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Localizations");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("ISO Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Resource");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Outdated");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildLocalizationFile localization in module.LocalizationFiles)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(localization.CultureInfo.Name);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(localization.CultureInfo.EnglishName);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(localization.Name);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(localization.Dirty);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.Defines.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Defines");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Value");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Inherited");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildDefine define in module.Defines)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(define.Name);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(define.Value);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.FalseString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            foreach (RBuildDefine define in Project.Defines)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(define.Name);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(define.Value);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(bool.TrueString);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }
                            writer.RenderEndTag();
                        }

                        writer.RenderBeginTag(HtmlTextWriterTag.H3);
                        writer.Write("Linker Flags");
                        writer.RenderEndTag();

                        writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                        writer.RenderBeginTag(HtmlTextWriterTag.Table);

                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Flag");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Inherited");
                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        foreach (string flag in module.LinkerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(flag);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.FalseString);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        foreach (string flag in Project.LinkerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(flag);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.TrueString);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();

                        if (module.LinkerScript != null)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Linker Script");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.LinkerScript.Root);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.LinkerScript.FullPath);
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            writer.RenderEndTag();
                        }

                        writer.RenderBeginTag(HtmlTextWriterTag.H3);
                        writer.Write("CompilerFlags Flags");
                        writer.RenderEndTag();

                        writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                        writer.RenderBeginTag(HtmlTextWriterTag.Table);

                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Flag");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Inherited");
                        writer.RenderEndTag();
                        writer.RenderEndTag();

                        foreach (string flag in module.CompilerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(flag);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.FalseString);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        foreach (string flag in Project.CompilerFlags)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(flag);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(bool.TrueString);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }

                        writer.RenderEndTag();

                        if (module.Files.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H2);
                            writer.Write("Files");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Path");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Install Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("New Name");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Type");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildOutputFile file in module.Files)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.Base);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.FullPath);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.InstallBase);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.NewFile.FullPath);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.GetType().Name);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();
                        }

                        if (module.SourceFiles.Count > 0)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.H3);
                            writer.Write("Source Files");
                            writer.RenderEndTag();

                            writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                            writer.RenderBeginTag(HtmlTextWriterTag.Table);

                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Root");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Source Type");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Base");
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Th);
                            writer.Write("Switches");
                            writer.RenderEndTag();
                            writer.RenderEndTag();

                            foreach (RBuildSourceFile file in module.SourceFiles)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.FullPath);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(file.Switches);
                                writer.RenderEndTag();
                                writer.RenderEndTag();
                            }

                            writer.RenderEndTag();

                            if (module.PreCompiledHeader != null)
                            {
                                writer.RenderBeginTag(HtmlTextWriterTag.H3);
                                writer.Write("Precompiled Header");
                                writer.RenderEndTag();

                                writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                                writer.RenderBeginTag(HtmlTextWriterTag.Table);

                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Th);
                                writer.Write("Root");
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Th);
                                writer.Write("Source Type");
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Th);
                                writer.Write("Base");
                                writer.RenderEndTag();
                                writer.RenderEndTag();

                                writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PreCompiledHeader.Root);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PreCompiledHeader.Type);
                                writer.RenderEndTag();
                                writer.RenderBeginTag(HtmlTextWriterTag.Td);
                                writer.Write(module.PreCompiledHeader.FullPath);
                                writer.RenderEndTag();
                                writer.RenderEndTag();

                                writer.RenderEndTag();
                            }
                        }

                        WriteDocumentEnd(writer);
                    }
                }
            }
        }

        private void GenerateUnicodeModules()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\unicodemodules.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Unicode");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Unicode Modules");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Unicode)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Base);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Type);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }

                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write("Non-Unicode Modules");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Module");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Base");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (!module.Unicode)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.Name);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Base);
                            writer.RenderEndTag();
                            writer.RenderBeginTag(HtmlTextWriterTag.Td);
                            writer.Write(module.Type);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }

                    writer.RenderEndTag();


                    WriteDocumentEnd(writer);
                }
            }
        }

        private void GeneratePlatformFrontPage()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\rosDoc\platform.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteDocumentStart(writer, "Platform");

                    writer.RenderBeginTag(HtmlTextWriterTag.H2);
                    writer.Write(Project.Platform.Name);
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.P);
                    writer.Write(Project.Platform.Description);
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Modules");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.WriteLine("{0} out of {1}", Project.Platform.Modules.Count, Project.Modules.Count);
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    if (Project.Platform.Shell != null)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Shell");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(Project.Platform.Shell.InstallName);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    if (Project.Platform.Screensaver != null)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Screen Saver");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(Project.Platform.Screensaver.InstallName);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    if (Project.Platform.Languages.Count > 0)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Languages");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                        foreach (RBuildLanguage language in Project.Platform.Languages)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(language.Name);
                            writer.RenderEndTag();
                        }
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    if (Project.Platform.DebugChannels.Count > 0)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Th);
                        writer.Write("Debug Channels");
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                        foreach (RBuildDebugChannel channel in Project.Platform.DebugChannels)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(channel.Name);
                            writer.RenderEndTag();
                        }
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Platform Profiles");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.ModuleGroup)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.Write(module.Description);
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Debug Build");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.Write(bool.TrueString);
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Kernel Debug");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.Write(bool.FalseString);
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.H3);
                    writer.Write("Files");
                    writer.RenderEndTag();

                    writer.AddAttribute(HtmlTextWriterAttribute.Class, "table");
                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("GUI Applications");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Console Applications");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Dlls");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Drivers");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Keyboard Layouts");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("KernelMode DLLs");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("OCX & TypeLibs");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.Win32GUI)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.Win32CUI)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.Win32DLL)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.KernelModeDriver)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.KeyboardLayout)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.KernelModeDLL)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderBeginTag(HtmlTextWriterTag.Td);
                    writer.RenderBeginTag(HtmlTextWriterTag.Ul);
                    foreach (RBuildModule module in Project.Platform.Modules)
                    {
                        if (module.Type == ModuleType.Win32OCX || module.Type == ModuleType.EmbeddedTypeLib)
                        {
                            writer.RenderBeginTag(HtmlTextWriterTag.Li);
                            writer.AddAttribute(HtmlTextWriterAttribute.Href, module.HtmlDocFileName);
                            writer.RenderBeginTag(HtmlTextWriterTag.A);
                            writer.Write(module.TargetName);
                            writer.RenderEndTag();
                            writer.RenderEndTag();
                        }
                    }
                    writer.RenderEndTag();
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    writer.RenderEndTag();

                    WriteDocumentEnd(writer);
                }
            }
        }

        protected override void Generate()
        {
            GenerateFrontPage();
            GeneratePlatformFrontPage();
            GenerateModules();
            GenerateProperties();
            GenerateModulesBuildSummary();
            GenerateStats();
            //GenerateDependencyMap();
            GenerateWarnings();
            GenerateFiles();
            GenerateBaseAddresses();
            GenerateDllBaseAddresses();
            GenerateDefines();
            GenerateInstallFolders();
            GenerateInstallFiles();
            GenerateAuthors();
            GenerateContributors();
            GenerateLocalizations();
            GenerateModulePages();
            GenerateUnicodeModules();
        }
    }
}
