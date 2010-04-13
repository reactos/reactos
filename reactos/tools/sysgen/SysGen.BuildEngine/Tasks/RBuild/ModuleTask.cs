using System;
using System.IO;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("module")]
    public class ModuleTask : TaskContainer /*AutoResolvableFileSystemInfoBaseTask ,, ITaskContainer,*/ /*TaskContainer,*/ , ISysGenObject/*, IDirectory,*/, IRBuildSourceFilesContainer
    {
        protected RBuildModule m_Module = new RBuildModule();

        [TaskAttribute("name", Required = true)]
        [StringValidator(AllowEmpty = false, AllowSpaces = false)]
        public string ModuleName
        {
            get { return m_Module.Name; }
            set { m_Module.Name = value; }
        }

        [TaskAttribute("type", Required = true)]
        public ModuleType Type
        {
            get { return m_Module.Type; }
            set { m_Module.Type = value; }
        }

        [TaskAttribute("buildtype")]
        public string BuildType
        {
            get { return m_Module.BuildType; }
            set { m_Module.BuildType = value; }
        }

        [TaskAttribute("description", ExpandProperties = true)]
        public string Description
        {
            get { return m_Module.Description; }
            set { m_Module.Description = value; }
        }

        [TaskAttribute("lcid")]
        public string LCID
        {
            get { return m_Module.LCID; }
            set { m_Module.LCID = value; }
        }

        [TaskAttribute("installname")]
        public string InstallName
        {
            get { return m_Module.InstallName; }
            set { m_Module.InstallName = value; }
        }

        [TaskAttribute("installbase")]
        [UriValidatorAttribute]
        public string InstallBase
        {
            get { return m_Module.InstallBase; }
            set { m_Module.InstallBase = value; }
        }

        [TaskAttribute("output")]
        public string Output
        {
            get { return m_Module.OutputName; }
            set { m_Module.OutputName = value; }
        }

        [TaskAttribute("baseaddress")]
        public string BaseAdress
        {
            get { return m_Module.BaseAddress; }
            set { m_Module.BaseAddress = value; }
        }

        [TaskAttribute("entrypoint")]
        public string EntryPoint
        {
            get { return m_Module.EntryPoint; }
            set { m_Module.EntryPoint = value; }
        }

        [TaskAttribute("aliasof")]
        public string AliasOf
        {
            get { return m_Module.AliasOf; }
            set { m_Module.AliasOf = value; }
        }

        [TaskAttribute("extension")]
        public string Extension
        {
            get { return m_Module.Extension; }
            set { m_Module.Extension = value; }
        }

        [TaskAttribute("unicode")]
        [BooleanValidator()]
        public bool Unicode
        {
            get { return m_Module.Unicode; }
            set { m_Module.Unicode = value; }
        }

        [TaskAttribute("host")]
        [BooleanValidator()]
        public bool Host
        {
            get { return m_Module.Host; }
            set { m_Module.Host = value; }
        }

        [TaskAttribute("isstartuplib")]
        [BooleanValidator()]
        public bool IsStartupLib
        {
            get { return m_Module.IsStartupLib; }
            set { m_Module.IsStartupLib = value; }
        }

        [TaskAttribute("underscoresymbols")]
        [BooleanValidator()]
        public bool UnderscoreSymbols
        {
            get { return m_Module.UnderscoreSymbols; }
            set { m_Module.UnderscoreSymbols = value; }
        }

        [TaskAttribute("mangledsymbols")]
        [BooleanValidator()]
        public bool MangledSymbols
        {
            get { return m_Module.MangledSymbols; }
            set { m_Module.MangledSymbols = value; }
        }

        [TaskAttribute("allowwarnings")]
        [BooleanValidator()]
        public bool AllowWarnings
        {
            get { return m_Module.AllowWarnings; }
            set { m_Module.AllowWarnings = value; }
        }

        public RBuildModule Module
        {
            get { return m_Module; }
        }

        public RBuildElement RBuildElement
        {
            get { return m_Module; }
        }

        public PathRoot Root
        {
            get { return PathRoot.Default; }
        }

        public RBuildSourceFileCollection SourceFiles
        {
            get { return Module.SourceFiles; }
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            if ((Module.Type == ModuleType.BootLoader) ||
                (Module.Type == ModuleType.BootProgram) ||
                (Module.Type == ModuleType.BootSector) ||
                (Module.Type == ModuleType.EmbeddedTypeLib) ||
                (Module.Type == ModuleType.IdlHeader) ||
                (Module.Type == ModuleType.Kernel) ||
                (Module.Type == ModuleType.KernelModeDLL) ||
                (Module.Type == ModuleType.KernelModeDriver) ||
                (Module.Type == ModuleType.NativeCUI) ||
                (Module.Type == ModuleType.NativeDLL) ||
                (Module.Type == ModuleType.ObjectLibrary) ||
                (Module.Type == ModuleType.StaticLibrary) ||
                (Module.Type == ModuleType.RpcClient) ||
                (Module.Type == ModuleType.RpcServer) ||
                (Module.Type == ModuleType.RpcProxy) ||
                (Module.Type == ModuleType.Win32CUI) ||
                (Module.Type == ModuleType.Win32DLL) ||
                (Module.Type == ModuleType.Win32GUI) ||
                (Module.Type == ModuleType.Win32OCX) ||
                (Module.Type == ModuleType.Win32SCR) ||
                //(Module.Type == ModuleType.Alias) ||
                (Module.Type == ModuleType.HostStaticLibrary) ||
                (Module.Type == ModuleType.BuildTool) ||
                (Module.Type == ModuleType.Cabinet) ||
                (Module.Type == ModuleType.KeyboardLayout) ||
                (Module.Type == ModuleType.Iso) ||
                (Module.Type == ModuleType.IsoRegTest) ||
                (Module.Type == ModuleType.LiveIso) ||
                (Module.Type == ModuleType.LiveIsoRegTest) ||
                (Module.Type == ModuleType.MessageHeader) ||
                (Module.Type == ModuleType.Package) ||
                (Module.Type == ModuleType.ModuleGroup) ||
                (Module.Type == ModuleType.PlatformProfile))
            {
                Module.Folder.Base = SysGenPathResolver.GetPath(this, SysGen.RootTask);
                //Module.Base = SysGenPathResolver.GetPath(this, SysGen.ProjectTask); //BasePath;
                Module.Path = SysGen.BaseDirectory;
                Module.XmlFile = XmlFile;
                Module.RBuildFile = RBuildFile;
                Module.Enabled = false;

                //HACK:
                if (Type == ModuleType.HostStaticLibrary ||
                    Type == ModuleType.BuildTool)
                {
                    Module.Host = true;
                }

                // Add the module to the project
                Project.Modules.Add(Module);
            }
            else
            {
                Console.WriteLine("WARNING: modules of type '{0}' are being omited" , Module.Type);
            }
        }

        private void AddModuleDefines()
        {
            switch (Type)
            {
                case ModuleType.NativeCUI:
                    {
                        Module.Defines.Add("__NTAPP__");
                    }
                    break;
                case ModuleType.KernelModeDriver:
                    {
                        Module.Defines.Add("__NTDRIVER__");
                    }
                    break;
            }

            if (Unicode)
            {
                Module.Defines.Add("UNICODE");
                Module.Defines.Add("_UNICODE");
            }
        }

        private void AddModuleLinkerFlags()
        {
            switch (Type)
            {
                case ModuleType.Win32DLL:
                case ModuleType.Win32OCX:
                case ModuleType.Win32CUI:
                case ModuleType.Win32GUI:
                case ModuleType.Win32SCR:
                    {
                        Module.LinkerFlags.Add("-nostartfiles");
                        Module.LinkerFlags.Add("-lgcc");

                        if (Module.CPlusPlus)
                            Module.LinkerFlags.Add("-nostdlib");
                    }
                    break;
                case ModuleType.Kernel:
                    {
                        break;
                    }
                case ModuleType.KeyboardLayout:
                case ModuleType.KernelModeDLL:
                case ModuleType.KernelModeDriver:
                case ModuleType.NativeCUI:
                case ModuleType.NativeDLL:
                case ModuleType.Test:
                case ModuleType.BootLoader:
                case ModuleType.BootProgram:
                    {
                        Module.LinkerFlags.Add("-nostartfiles");
                        Module.LinkerFlags.Add("-nostdlib");
                    }
                    break;
            }

            Module.LinkerFlags.Add("-g");
        }

        private void AddDebugSupportLibraries()
        {
            switch (Type)
            {
                case ModuleType.Win32DLL:
                case ModuleType.Win32OCX:
                case ModuleType.Win32CUI:
                case ModuleType.Win32GUI:
                case ModuleType.Win32SCR:
                case ModuleType.NativeCUI:
                case ModuleType.NativeDLL:
                    {
                        Module.Libraries.Add(Project.Modules.GetByName("debugsup_ntdll"));
                    }
                    break;
                case ModuleType.KeyboardLayout:
                case ModuleType.KernelModeDLL:
                case ModuleType.KernelModeDriver:
                    {
                        Module.Libraries.Add(Project.Modules.GetByName("debugsup_ntoskrnl"));
                    }
                    break;
            }
        }

        private void AddModuleCompilerFlags()
        {
            if (!AllowWarnings)
            {
                Module.CompilerFlags.Add("-Werror");
            }

            // Always force disabling of sibling calls optimisation for GCC
            // (TODO: Move to version-specific once this bug is fixed in GCC)
            Module.CompilerFlags.Add("-fno-optimize-sibling-calls");
            Module.CompilerFlags.Add("-g");
            Module.CompilerFlags.Add("-pipe");
        }

        private void AddRequiredBuildTools()
        {
            switch (Module.Type)
            {
                case ModuleType.BootLoader:
                case ModuleType.BootProgram: 
                case ModuleType.BootSector:
                case ModuleType.EmbeddedTypeLib:
                case ModuleType.IdlHeader:
                case ModuleType.MessageHeader:
                case ModuleType.Kernel: 
                case ModuleType.KernelModeDLL: 
                case ModuleType.KernelModeDriver:
                case ModuleType.NativeCUI: 
                case ModuleType.NativeDLL:
                case ModuleType.ObjectLibrary:
                case ModuleType.StaticLibrary:
                case ModuleType.RpcClient:
                case ModuleType.RpcServer:
                case ModuleType.RpcProxy:
                case ModuleType.Win32CUI:
                case ModuleType.Win32DLL:
                case ModuleType.Win32GUI:
                case ModuleType.Win32OCX:
                case ModuleType.Win32SCR:
                case ModuleType.HostStaticLibrary:
                case ModuleType.KeyboardLayout:
                    Module.Requeriments.Add(Project.Modules.GetByName("wrc"));
                    Module.Requeriments.Add(Project.Modules.GetByName("wmc"));
                    Module.Requeriments.Add(Project.Modules.GetByName("widl"));
                    Module.Requeriments.Add(Project.Modules.GetByName("winebuild"));
                    Module.Requeriments.Add(Project.Modules.GetByName("winebuild"));
                    break;
                case ModuleType.Cabinet:
                    Module.Requeriments.Add(Project.Modules.GetByName("cabman"));
                    break;
                case ModuleType.Iso:
                    Module.Requeriments.Add(Project.Modules.GetByName("cdmake"));
                    Module.Requeriments.Add(Project.Modules.GetByName("cabman"));
                    break;
                case ModuleType.IsoRegTest:
                    Module.Requeriments.Add(Project.Modules.GetByName("cdmake"));
                    Module.Requeriments.Add(Project.Modules.GetByName("cabman"));
                    Module.Requeriments.Add(Project.Modules.GetByName("sysreg"));
                    break;
                case ModuleType.LiveIso:
                    Module.Requeriments.Add(Project.Modules.GetByName("cdmake"));
                    Module.Requeriments.Add(Project.Modules.GetByName("mkhive"));
                    break;
                case ModuleType.LiveIsoRegTest:
                    Module.Requeriments.Add(Project.Modules.GetByName("cdmake"));
                    Module.Requeriments.Add(Project.Modules.GetByName("mkhive"));
                    Module.Requeriments.Add(Project.Modules.GetByName("sysreg"));
                    break;
            }
        }

        private void AddDefaultDependencies()
        {
            if (Module.Type != ModuleType.BuildTool &&
                Module.Type != ModuleType.HostStaticLibrary && 
                Module.Host == false)
            {
                if (Module.Name != "psdk" &&
                    Module.Name != "dxsdk" &&
                    Module.Name != "errcodes" &&
                    Module.Name != "bugcodes" &&
                    Module.Name != "ntstatus")
                {
                    Module.Dependencies.Add(Project.Modules.GetByName("psdk"));
                    Module.Dependencies.Add(Project.Modules.GetByName("dxsdk"));
                    Module.Dependencies.Add(Project.Modules.GetByName("errcodes"));
                    Module.Dependencies.Add(Project.Modules.GetByName("bugcodes"));
                    Module.Dependencies.Add(Project.Modules.GetByName("ntstatus"));
                }
            }
        }

        //public RBuildFolder Folder
        //{
        //    get { return null; }
        //}

        private void AddModuleLibraryDependencies()
        {
            // Add mingw and msvcrt implicit libraries only if it's
            // a Win32 target.

            if ((Module.Type == ModuleType.Win32CUI) ||
                (Module.Type == ModuleType.Win32DLL) ||
                (Module.Type == ModuleType.Win32GUI) ||
                (Module.Type == ModuleType.Win32OCX) ||
                (Module.Type == ModuleType.Win32SCR))
            {
                if (!Module.IsDefaultEntryPoint)
                {
                    if (Module.NoEntryPoint)
                    {
                        if (Module.LinksToCRuntimeLibrary == false)
                        {
                            Module.Libraries.Add(0, Project.Modules.GetByName("mingw_common"));
                        }
                    }
                }
                else
                {
                    if (!Module.IsDLL)
                    {
                        if (Unicode)
                        {
                            Module.Libraries.Add(0, Project.Modules.GetByName("mingw_wmain"));
                        }
                        else
                        {
                            Module.Libraries.Add(0, Project.Modules.GetByName("mingw_main"));
                        }
                    }

                    //Is it correct ?
                    if (Module.Libraries.Count > 0)
                    {
                        Module.Libraries.Add(1, Project.Modules.GetByName("mingw_common"));
                    }
                    else
                    {
                        Module.Libraries.Add(0, Project.Modules.GetByName("mingw_common"));
                    }
                }
            }
        }

        private void AddCRuntimeLibrary()
        {
            if ((Module.Type == ModuleType.Win32CUI) ||
                (Module.Type == ModuleType.Win32DLL) ||
                (Module.Type == ModuleType.Win32GUI) ||
                (Module.Type == ModuleType.Win32OCX) ||
                (Module.Type == ModuleType.Win32SCR))
            {
                if (Module.IsDefaultEntryPoint || Module.NoEntryPoint)
                {
                    if (Module.LinksToCRuntimeLibrary == false)
                    {
                        if (Module.Name != "msvcrt")
                        {
                            // Link msvcrt to get the basic routines
                            Module.Libraries.Add(Project.Modules.GetByName("msvcrt"));
                        }
                    }
                }
            }
        }

        private void AddModuleAssemblyFlags()
        {
            if (Type == ModuleType.BootSector)
            {
                Module.AssemblyFlags.Add("-f bin");
            }
        }

        private void AddModuleIncludeFolders()
        {
            if (Module.Type == ModuleType.RpcClient ||
                Module.Type == ModuleType.RpcServer ||
                Module.Type == ModuleType.RpcProxy ||
                Module.Type == ModuleType.EmbeddedTypeLib)
            {
                Module.IncludeFolders.Add(new RBuildFolder(PathRoot.Intermediate, Module.Folder.FullPath)); //Hace falta?
                Module.IncludeFolders.Add(new RBuildFolder(PathRoot.SourceCode, Module.Folder.FullPath));
            }
        }

        private void AddModuleProperties()
        {
            Project.Properties.Add(string.Format("SysGen.Module.{0}.Enabled", Module.Name), Module.Enabled.ToString(), true, true);
            Project.Properties.Add(string.Format("SysGen.Module.{0}.BasePath", Module.Name), Module.Base, true, true);
            Project.Properties.Add(string.Format("SysGen.Module.{0}.RBuildFile", Module.Name), Module.RBuildFile, true, true);
        }

        protected override void PostExecuteTask()
        {
            if (m_Module.RBuildFile == "mstask.rbuild")
            {
                int i = 0;
            }

            AddModuleLibraryDependencies();
            AddCRuntimeLibrary();
            AddDebugSupportLibraries();
            AddDefaultDependencies();
            AddModuleProperties();

            if (m_Module.Host)
            {
                if (m_Module.CPlusPlus)
                {
                    m_Module.CompilerFlags.Add("$(HOST_CPPFLAGS)");
                }
                else
                {
                    m_Module.CompilerFlags.Add("$(HOST_CFLAGS)");
                    m_Module.CompilerFlags.Add("-Wno-strict-aliasing");
                }
            }
            else
            {
                if (m_Module.CPlusPlus)
                {
                    m_Module.CompilerFlags.Add("$(HOST_CPPFLAGS)");
                }
                else
                {
                    m_Module.CompilerFlags.Add("-nostdinc");
                }
            }

            //Sort source code files only when necessary
            if (Module.SourceFiles.ContainsASM)
                Module.SourceFiles.Sort(new SourceCodePreferenceComparer());
        }

        protected override void ExecuteTask()
        {
            Module.Enabled = true;

            //AddModuleLibraryDependencies();
            AddModuleDefines();
            AddModuleLinkerFlags();
            AddModuleCompilerFlags();
            AddModuleIncludeFolders();           
            AddModuleAssemblyFlags();
            AddRequiredBuildTools();

            //if (Type == ModuleType.Alias)
            //{
            //    if (Module.Name == "halupalias")
            //    {
            //        int i = 10;
            //    }

            //    RBuildModule alisedModule = Project.Modules.GetByName(AliasOf);

            //    if (alisedModule == null)
            //        throw new BuildException("module '" + ModuleName + "' trying to alias non-existant module '" + AliasOf + "'", Location);

            //    if (Module.Name == AliasOf)
            //        throw new BuildException("Module '" + ModuleName + "' cannot link against itself", Location);

            //    alisedModule = Module;
            //    alisedModule.Enabled = true;

            //    Module.Enabled = false;
            //}
        }
    }
}