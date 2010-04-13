using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class MingwLiveCDTargetHandler : MingwRBuildIsoModuleHandler
    {
        public MingwLiveCDTargetHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void AddAditionalFiles()
        {
            //RBuildCDFile livecdBootIni = new RBuildCDFile();

            //livecdBootIni.Base = "boot\bootdata";
            //livecdBootIni.Name = "livecd.ini";
            //livecdBootIni.NewName = "";
        }

        protected override void WriteCabinetManager()
        {
            // Not required
        }

        protected override void WriteMakeHive()
        {
            Makefile.WriteLine("\t$(ECHO_MKHIVE)");
            Makefile.WriteLine("\t$(mkhive_TARGET) boot\bootdata " + @"$(OUTPUT)\livecd\reactos\system32\config boot\bootdata\livecd.inf boot\bootdata\hiveinst.inf");
        }

        protected override void WriteCopyCDFiles()
        {
            foreach (RBuildOutputFile platformFile in SysGen.Project.Files)
            {
                if (platformFile is RBuildPlatformFile || platformFile is RBuildCDFile)
                {
                }
                else
                {
                    RBuildCDFile cdFile = new RBuildCDFile();

                    cdFile.Root = PathRoot.LiveCD;
                    cdFile.Name = platformFile.Name;
                    cdFile.Base = platformFile.InstallBase;

                    Makefile.WriteLine("\t$(ECHO_CP)");
                    Makefile.WriteLine("\t$(cp) " + ResolveRBuildFilePath(platformFile) + " " + ResolveRBuildFilePath(cdFile) + " 1>$(NUL)");
                }
            }

            foreach (RBuildModule module in SysGen.Project.Modules)
            {
                if (module.IsInstallable)
                {
                    RBuildCDFile cdFile = new RBuildCDFile();

                    cdFile.Root = PathRoot.LiveCD;
                    cdFile.Name = module.TargetFile.Name;
                    cdFile.Base = "reactos" + "//" + module.InstallBase;

                    Makefile.WriteLine("\t$(ECHO_CP)");
                    Makefile.WriteLine("\t$(cp) " + ResolveRBuildFilePath(module.TargetFile) + " " + ResolveRBuildFilePath(cdFile) + " 1>$(NUL)");
                }
            }

            Makefile.WriteLine("\t$(ECHO_CP)");
            Makefile.WriteLine("\t${cp} " + @"boot\bootdata\livecd.ini $(OUTPUT)\livecd\freeldr.ini 1>$(NUL)");
        }

        public override RBuildFolder WorkingFolder
        {
            get { return new RBuildFolder(PathRoot.Output, "LiveCD"); }
        }

        protected override string ResolveRBuildFilePath(RBuildFile file)
        {
            if (file.Root == PathRoot.CDOutput)
                return Path.Combine(SysGen.LiveCDOutputDirectory, file.FullPath);

            return SysGen.ResolveRBuildFilePath(file);
        }
    }

    public class MingwLiveCDRegTestTargetHandler : MingwLiveCDTargetHandler
    {
        public MingwLiveCDRegTestTargetHandler(RBuildModule module)
            : base(module)
        {
        }
        public override RBuildFolder WorkingFolder
        {
            get { return new RBuildFolder(PathRoot.Output, "livecdregtest"); }
        }
    }

    public class MingwBootCDRegTestTargetHandler : MingwBootCDTargetHandler
    {
        public MingwBootCDRegTestTargetHandler(RBuildModule module)
            : base(module)
        {
        }

        public override RBuildFolder WorkingFolder
        {
            get { return new RBuildFolder(PathRoot.Output , "cdregtest"); }
        }
    }

    public class MingwBootCDTargetHandler : MingwRBuildIsoModuleHandler
    {
        public MingwBootCDTargetHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void WriteCopyCDFiles()
        {
            base.WriteCopyCDFiles();

            foreach (RBuildModule module in SysGen.Project.Platform.Modules)
            {
                if ((module.Enabled) && (module.IsBootstrap))
                {
                    RBuildBootstrapFile bootstrapFile = module.Bootstrap;

                    if (bootstrapFile != null)
                    {
                        Makefile.WriteLine("\t$(ECHO_CP)");
                        Makefile.WriteLine("\t$(cp) " + ResolveRBuildFilePath(bootstrapFile) + " " + ResolveRBuildFilePath(bootstrapFile.CDNewFile) + " 1>$(NUL)");
                    }
                }
            }

            foreach (RBuildOutputFile file in SysGen.Project.Files)
            {
                RBuildBootstrapFile bootstrapFile = file as RBuildBootstrapFile;

                if (bootstrapFile != null)
                {
                    Makefile.WriteLine("\t$(ECHO_CP)");
                    Makefile.WriteLine("\t$(cp) " + ResolveRBuildFilePath(bootstrapFile) + " " + ResolveRBuildFilePath(bootstrapFile.CDNewFile) + " 1>$(NUL)");
                }
            }
        }

        protected override void AddFolders()
        {
            //Ensure folder exists
            base.AddFolders();

            Module.Folders.Add(new RBuildFolder(WorkingFolder, "loader"));
            Module.Folders.Add(new RBuildFolder(WorkingFolder, "reactos"));
            Module.Folders.Add(new RBuildFolder(WorkingFolder, "reactos/system32"));
        }

        protected override string ResolveRBuildFilePath(RBuildFile file)
        {
            if (file.Root == PathRoot.CDOutput)
                return SysGen.NormalizePath(Path.Combine(SysGen.BootCDOutputDirectory, file.FullPath));

            return SysGen.ResolveRBuildFilePath(file);
        }

        public override RBuildFolder WorkingFolder
        {
            get { return new RBuildFolder(PathRoot.Output , "cd"); }
        }
    }

    public abstract class MingwRBuildIsoModuleHandler : MingwRBuildModuleHandler
    {
        private const string PROFILES_FOLDER = "Profiles";
        private const string ALL_USERS_FOLDER = "All Users";
        private const string DEFAULT_USER_FOLDER = "Default User";
        private const string DESKTOP_FOLDER = "Desktop";
        private const string MY_DOCUMENTS_FOLDER = "My Documents";

        protected List<string> m_BuildTools = new List<string>();

        public MingwRBuildIsoModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected List<string> BuildTools
        {
            get { return m_BuildTools; }
        }

        public override void GenerateMakeFile()
        {
            AddAditionalFiles();
            AddFolders();

            WriteFolders();
            WriteTarget();
            WriteCabinetManager();
            WriteCopyCDFiles();
            WriteMakeRegistryHives();
            WriteMakeHive();
            WriteCDMake();
            WriteCleanTarget();
        }

        protected virtual void AddAditionalFiles()
        {
        }

        protected virtual void WriteMakeHive()
        {
        }

        protected virtual void AddFolders()
        {
            Module.Folders.Add(WorkingFolder);
        }

        protected virtual void WriteTarget()
        {
            Makefile.WriteTarget(Module.MakeFileTargetMacro);
            Makefile.WriteIndentedLine("all");
            Makefile.WriteIndentedLine("$(cabman_TARGET)");
            Makefile.WriteIndentedLine("$(cdmake_TARGET)");
            Makefile.WriteIndentedLine("$(mkhive_TARGET)");
            Makefile.WriteIndentedLine(Module.MakeFileFoldersMacro);

            foreach (RBuildModule module in SysGen.Project.Platform.Modules)
            {
                if ((module.Enabled) && (module.IsBootstrap))
                {
                    Makefile.WriteIndentedLine(module.MakeFileTargetMacro);
                }
            }

            Makefile.WriteIndentedLine(BootModule.MakeFileTargetMacro);
            Makefile.WriteLine();
        }

        protected virtual void WriteMakeRegistryHives()
        {
        }

        protected virtual void WriteCabinetManager()
        {
            Makefile.WriteLine("\t$(ECHO_CABMAN)");
            Makefile.WriteLine("\t$(Q)$(cabman_TARGET) -C " + SysGen.Project.PackagesFile + @" -L $(OUTPUT)\cd\reactos -I -P $(OUTPUT)");
            Makefile.WriteLine("\t$(Q)$(cabman_TARGET) -C " + SysGen.Project.PackagesFile + @" -RC $(OUTPUT)\cd\reactos\reactos.inf -L $(OUTPUT)\cd\reactos -N -P $(OUTPUT)");
            Makefile.WriteLine("\t-@${rm} " + @"$(OUTPUT)\cd\reactos\reactos.inf" + " 2>$(NUL)");
            Makefile.WriteLine();
        }

        protected virtual void WriteCDMake()
        {
            Makefile.WriteLine("\t$(ECHO_CDMAKE)");
            Makefile.WriteLine("\t$(Q)$(cdmake_TARGET) -v -j -m -b " + ResolveRBuildFilePath(BootModule.TargetFile) + " " + ResolveRBuildFolderPath(WorkingFolder) + " " + CDLabel + " " + IsoImage);
            Makefile.WriteLine();
        }

        protected virtual void WriteCopyCDFiles()
        {
            foreach (RBuildOutputFile file in SysGen.Project.Files)
            {
                RBuildCDFile cdFile = file as RBuildCDFile;

                if (cdFile != null)
                {
                    Makefile.WriteLine("\t$(ECHO_CP)");
                    Makefile.WriteLine("\t$(cp) " + ResolveRBuildFilePath(cdFile) + " " + ResolveRBuildFilePath(cdFile.CDNewFile) + " 1>$(NUL)");
                }
            }
        }

        protected virtual RBuildModule BootModule
        {
            get { return Module.BootSector; }
        }

        public abstract RBuildFolder WorkingFolder { get; }

        public virtual string IsoImage
        {
            get { return ResolveRBuildFilePath(Module.TargetFile); }
        }

        public virtual string CDLabel
        {
            get { return Module.CDLabel; }
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return false;
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            throw new Exception("The method or operation is not implemented.");
        }
    }
}