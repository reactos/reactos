using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.BuildEngine.Framework;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public abstract class CompilerBaseBacked : Backend
    {
        public CompilerBaseBacked(SysGenEngine sysgen)
            : base(sysgen)
        {
            //Initialize();
        }

        protected override void Generate()
        {
            CheckCompiler();
            GenerateRosCfg();
            GenerateFolders();
            //GenerateBuildNumber();
            //GenerateCompilationUnits();
            GenerateTxtSetupCustomHive();
            GenerateSysSetup();
            GenerateTxtSetup();
            GenerateDffSetup();
        }

        protected virtual void GenerateCompilationUnits()
        {
            foreach (RBuildModule module in SysGen.Project.Modules)
            {
                foreach (RBuildCompilationUnitFile unit in module.CompilationUnits)
                {
                    using (CompilationUnitFileWriter writer = new CompilationUnitFileWriter(module , unit , SysGen.ResolveRBuildFilePath(unit)))
                    {
                        writer.WriteFile();
                    }
                }
            }
        }

        protected virtual void GenerateRosCfg()
        {
            Directory.CreateDirectory (Project.Path + "\\obj-i386\\include\\reactos");

            using (HeaderRosCfgFileWriter writer = new HeaderRosCfgFileWriter(SysGen.Project, Project.Path + "\\obj-i386\\include\\reactos\\roscfg.h"))
            {
                writer.WriteFile();
            }
        }

        protected virtual void GenerateDffSetup()
        {
            using (DffFileWriter writer = new DffFileWriter(Project, Project.Path + "\\obj-i386\\reactos.dff"))
            {
                writer.WriteFile();
            }
        }

        protected virtual void GenerateBuildNumber()
        {
            using (DffFileWriter writer = new DffFileWriter(SysGen.Project, "c:\\buildno.h"))
            {
                writer.WriteFile();
            }
        }

        protected virtual void GenerateTxtSetup()
        {
            using (TxtSetupFileWriter writer = new TxtSetupFileWriter(SysGen.Project, "c:\\txtsetup.sif"))
            {
                writer.WriteFile();
            }
        }

        protected virtual void GenerateTxtSetupCustomHive()
        {
            //using (DesktopComponentSetupFileWriter writer = new DesktopComponentSetupFileWriter(SysGen.Project, Project.Path + "\\output-i386\\wallpaper.inf"))
            //{
            //    writer.WriteFile();

            //    RBuildSetup s = new RBuildSetup();

            //    s.InstallBase = "inf";
            //    s.Name = "wallpaper.inf";
            //    s.Root = PathRoot.Output;

            //    Project.Files.Add(s);
            //}

            //using (ShellComponentSetupFileWriter writer = new ShellComponentSetupFileWriter(SysGen.Project, Project.Path + "\\output-i386\\shell.inf"))
            //{
            //    writer.WriteFile();

            //    RBuildSetup s1 = new RBuildSetup();

            //    s1.InstallBase = "inf";
            //    s1.Name = "shell.inf";
            //    s1.Root = PathRoot.Output;

            //    Project.Files.Add(s1);
            //}

            using (TxtSetupHiveFileWriter writer = new TxtSetupHiveFileWriter(SysGen.Project, Project.Path + "\\output-i386\\hivecst.inf"))
            {
                writer.WriteFile();
            }

            RBuildBootstrapFile bs = new RBuildBootstrapFile();

            bs.InstallBase = "reactos";
            bs.Name = "hivecst.inf";
            bs.Root = PathRoot.Output;

            Project.Files.Add(bs);
        }

        protected virtual void GenerateSysSetup()
        {
            Directory.CreateDirectory(Project.Path + "\\output-i386\\media\\inf");

            using (SysSetupFileWriter writer = new SysSetupFileWriter(SysGen.Project, Project.Path + "\\output-i386\\media\\inf\\syssetup.inf"))
            {
                writer.WriteFile();
            }
        }

        private void GenerateFolders()
        {
            //foreach (RBuildModule module in Project.Modules)
            //{
            //    foreach (RBuildFolder folder in module.Folders)
            //    {
            //        if (folders.Contains(folder) == false)
            //            folders.Add(folder);
            //    }
            //}

            //foreach (RBuildFolder folder in Project.Folders)
            //{
            //    if (folders.Contains(folder) == false)
            //        folders.Add(folder);
            //}

            //foreach (RBuildFolder folder in folders)
            //{
            //    GenerateFolder(makefile, folder);
            //}
        }

        protected virtual void CheckCompiler()
        {
        }
    }
}
