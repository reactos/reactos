using System;
using System.Xml;
using System.IO;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using System.ComponentModel;

namespace SysGen.RBuild.Framework
{
    public enum ModuleType
    {
        BuildTool = 0,
        StaticLibrary = 1,
        ObjectLibrary = 2,
        Kernel = 3,
        KernelModeDLL = 4,
        KernelModeDriver = 5,
        NativeDLL = 6,
        NativeCUI = 7,
        Win32DLL = 8,
        Win32OCX = 9,
        Win32CUI = 10,
        Win32GUI = 11,
        BootLoader = 12,
        BootSector = 13,
        Iso = 14,
        LiveIso = 15,
        Test = 16,
        RpcServer = 17,
        RpcClient = 18,
        Alias = 19,
        BootProgram = 20,
        Win32SCR = 21,
        IdlHeader = 23,
        IsoRegTest = 24,
        LiveIsoRegTest = 25,
        EmbeddedTypeLib = 26,
        ElfExecutable = 27,
        RpcProxy = 28,
        HostStaticLibrary = 29,
        Cabinet = 30,
        Package = 50,
        ModuleGroup = 51,
        PlatformProfile = 52,
        KeyboardLayout,
        MessageHeader,
        IdlInterface
    }

    [DefaultPropertyAttribute("Name")]
    public class RBuildModule : RBuildElement, IRBuildSourceFilesContainer//, IRBuildInstallable
    {
        private string m_InstallBase = ".";
        private string m_InstallName = null;
        private string m_BaseAddress = null;
        private string m_EntryPoint = null;
        private string m_AliasOf = null;
        private string m_Extension = null;
        private string m_BuildType = null;
        private string m_Description = null;
        private string m_LCID = null;
        private string m_CDLabel = null;
        private string m_OutputName = null;
        private string m_CatalogPath = null;

        private ModuleType m_Type = ModuleType.Win32CUI;

        protected bool m_Enabled = true;
        protected bool m_Unicode = false;
        protected bool m_AllowWarnings = false;
        protected bool m_IsStartupLib = false;
        protected bool m_UnderscoreSymbols = false;
        protected bool m_MangledSymbols = false;
        protected bool m_HostBuild = false;

        //protected RBuildInfInstallerFile m_InfInstallComponent = null;
        protected RBuildFile m_LinkerScript = null;
        protected RBuildSourceFile m_PrecompiledHeader = null;
        protected RBuildAutoRegister m_AutoRegister = null;
        protected RBuildSetupFile m_RBuildSetup = null;
        protected RBuildImportLibrary m_ImportLibrary = null;
        protected RBuildMetadata m_Metadata = null;
        //protected RBuildInstallFolder m_InstallFolder = null;
        protected RBuildBootstrapFile m_Bootstrap = null;
        protected RBuildModule m_BootSectorModule = null;

        private RBuildFamilyCollection m_Families = new RBuildFamilyCollection();
        private RBuildAPIStatusCollection m_ApiInfo = new RBuildAPIStatusCollection();
        private RBuildModuleCollection m_Dependencies = new RBuildModuleCollection();
        private RBuildModuleCollection m_Libraries = new RBuildModuleCollection();
        private RBuildModuleCollection m_Requeriments = new RBuildModuleCollection();
        private RBuildSourceFileCollection m_SourceFiles = new RBuildSourceFileCollection();
        private RBuildLocalizationFileCollection m_LocalizationFiles = new RBuildLocalizationFileCollection();
        private RBuildExportedFunctionsCollection m_ExportedFunctions = new RBuildExportedFunctionsCollection();
        private RBuildAuthorCollection m_Authors = new RBuildAuthorCollection();
        private List<RBuildRegistryKey> m_RegistryKeys = new List<RBuildRegistryKey>();
        private List<RBuildCompilationUnitFile> m_CompilationUnits = new List<RBuildCompilationUnitFile>();

        //public void GenerateFromPath(string path)
        //{
        //    m_Base = path;

        //    m_Path = System.IO.Path.GetFileName(path);
        //    m_Name = System.IO.Path.GetFileName(path);
        //}

        public RBuildSourceFile PreCompiledHeader
        {
            get
            {
                foreach (RBuildSourceFile file in SourceFiles)
                {
                    if (file.Type == SourceType.Header)
                        return file;
                }

                return null;
            }
        }

        public RBuildFile LinkerScript
        {
            get { return m_LinkerScript; }
            set { m_LinkerScript = value; }
        }

        public RBuildBootstrapFile Bootstrap
        {
            get { return m_Bootstrap; }
            set { m_Bootstrap = value; }
        }

        public bool IsBootstrap
        {
            get { return Bootstrap != null; }
        }

        //Hack:
        public bool IsSpecialIncludedBootStrap
        {
            get { return (Name == "ntdll"); }
        }

        //Hack::
        public bool IsSpecialExcludedBootStrap
        {
            get { return (Name == "hal"); }
        }

        public RBuildModule BootSector
        {
            get { return m_BootSectorModule; }
            set { m_BootSectorModule = value; }
        }

        //public RBuildInstallFolder InstallFolder
        //{
        //    get { return m_InstallFolder; }
        //    set { m_InstallFolder = value; }
        //}

        public RBuildMetadata Metadata
        {
            get { return m_Metadata; }
            set { m_Metadata = value; }
        }

        public string Extension
        {
            get
            {
                if ((m_Extension == null) || (m_Extension == string.Empty))
                    return DefaultExtension;

                return m_Extension;
            }
            set { m_Extension = value; }
        }

        public string DefaultExtension
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.StaticLibrary:
                    case ModuleType.HostStaticLibrary:
                        return ".a";
                    case ModuleType.ObjectLibrary:
                        return ".o";
                    case ModuleType.Kernel:
                    case ModuleType.NativeCUI:
                    case ModuleType.Win32CUI:
                    case ModuleType.Win32GUI:
                        return ".exe";
                    case ModuleType.Win32SCR:
                        return ".scr";
                    case ModuleType.KeyboardLayout:
                    case ModuleType.KernelModeDLL:
                    case ModuleType.NativeDLL:
                    case ModuleType.Win32DLL:
                        return ".dll";
                    case ModuleType.Win32OCX:
                        return ".ocx";
                    case ModuleType.KernelModeDriver:
                    case ModuleType.BootLoader:
                        return ".sys";
                    case ModuleType.BootSector:
                        return ".o";
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                        return ".iso";
                    case ModuleType.Test:
                        return ".exe";
                    case ModuleType.RpcServer:
                    case ModuleType.RpcClient:
                    case ModuleType.RpcProxy:
                        return ".o";
                    case ModuleType.BuildTool:
                        return ".exe";
                    case ModuleType.Alias:
                    case ModuleType.BootProgram:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.Package:
                    case ModuleType.ModuleGroup:
                    case ModuleType.PlatformProfile:
                        return string.Empty;
                    case ModuleType.EmbeddedTypeLib:
                        return ".tlb";
                    case ModuleType.Cabinet:
                        return ".cab";
                    default:
                        throw new Exception("Unknown module type");
                }
            }
        }

        public string CatalogPath
        {
            get
            {
                if (m_CatalogPath == null)
                    return Folder.Parent.FullPath;

                return m_CatalogPath;
            }
            set { m_CatalogPath = value; }
        }

        public RBuildImportLibrary ImportLibrary
        {
            get { return m_ImportLibrary; }
            set { m_ImportLibrary = value; }
        }

        public string AliasOf
        {
            get { return m_AliasOf; }
            set { m_AliasOf = value; }
        }

        public string BuildType
        {
            get 
            {
                if (m_BuildType == null)
                    m_BuildType = "BOOTPROG";

                return m_BuildType; 
            }
            set { m_BuildType = value; }
        }

        public string BaseAddress
        {
            get
            {
                if ((m_BaseAddress == null) || (m_BaseAddress == string.Empty))
                    return DefaultBaseAdress;

                return m_BaseAddress;
            }
            set { m_BaseAddress = value; }
        }

        public string EntryPoint
        {
            get
            {
                if (string.IsNullOrEmpty(m_EntryPoint))
                    return DefaultEntrypoint;

                return m_EntryPoint;
            }
            set { m_EntryPoint = value; }
        }

        public bool NoEntryPoint
        {
            get { return (EntryPoint == "0") || (EntryPoint == "0x0"); }
        }

        public string LinkerEntryPoint
        {
            get
            {
                if (NoEntryPoint)
                    return EntryPoint;

                return string.Format("_{0}", EntryPoint);
            }
        }

        public string HtmlDocFileName
        {
            get { return string.Format("{0}.htm", Name); }
        }

        public bool IsDefaultBaseAdress
        {
            get { return (BaseAddress == DefaultBaseAdress); }
        }

        public bool IsDefaultEntryPoint
        {
            get { return (EntryPoint == DefaultEntrypoint); }
        }

        public bool PCH
        {
            get { return (PreCompiledHeader != null); }
        }

        public bool CPlusPlus
        {
            get 
            {
                foreach (RBuildSourceFile file in SourceFiles)
                {
                    if ((file.Extension == ".cpp") ||
                        (file.Extension == ".cc") ||
                        (file.Extension == ".cxx"))
                    {
                        return true;
                    }
                }

                // This module does not contain C++ code
                return false;
            }
        }

        public bool IsRPC
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.RpcClient:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcProxy:
                        return true;
                    default:
                        return false;
                }
            }
        }

        public bool IsLibrary
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.StaticLibrary:
                    case ModuleType.ObjectLibrary:
                    case ModuleType.HostStaticLibrary: //HACK
                        return true;
                    default:
                        return false;
                }
            }
        }

        public bool HasInstallBase
        {
            get { return InstallBase != null; }
        }

        public bool IsInstallable
        {
            get { return (IsDLL) || (IsApplication); }
        }

        public bool IsApplication
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.NativeCUI:
                    case ModuleType.Win32CUI:
                    case ModuleType.Win32SCR:
                    case ModuleType.Win32GUI:
                        return true;
                    case ModuleType.KeyboardLayout:
                    case ModuleType.Kernel:
                    case ModuleType.KernelModeDLL:
                    case ModuleType.KernelModeDriver:
                    case ModuleType.NativeDLL:
                    case ModuleType.Win32DLL:
                    case ModuleType.Win32OCX:
                    case ModuleType.Test:
                    case ModuleType.BuildTool:
                    case ModuleType.HostStaticLibrary:
                    case ModuleType.StaticLibrary:
                    case ModuleType.ObjectLibrary:
                    case ModuleType.BootLoader:
                    case ModuleType.BootSector:
                    case ModuleType.BootProgram:
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcClient:
                    case ModuleType.RpcProxy:
                    case ModuleType.Alias:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.EmbeddedTypeLib:
                    case ModuleType.Cabinet:
                    case ModuleType.Package:
                    case ModuleType.ModuleGroup:
                    case ModuleType.PlatformProfile:
                        return false;
                    default:
                        throw new Exception("Unknown Module Type");
                }
            }
        }

        public bool IsDLL
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.Kernel:
                    case ModuleType.KernelModeDLL:
                    case ModuleType.KernelModeDriver:
                    case ModuleType.NativeDLL:
                    case ModuleType.Win32DLL:
                    case ModuleType.Win32OCX:
                    case ModuleType.KeyboardLayout:
                        return true;
                    case ModuleType.NativeCUI:
                    case ModuleType.Win32CUI:
                    case ModuleType.Test:
                    case ModuleType.Win32SCR:
                    case ModuleType.Win32GUI:
                    case ModuleType.BuildTool:
                    case ModuleType.HostStaticLibrary:
                    case ModuleType.StaticLibrary:
                    case ModuleType.ObjectLibrary:
                    case ModuleType.BootLoader:
                    case ModuleType.BootSector:
                    case ModuleType.BootProgram:
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcClient:
                    case ModuleType.RpcProxy:
                    case ModuleType.Alias:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.EmbeddedTypeLib:
                    case ModuleType.Cabinet:
                    case ModuleType.Package:
                    case ModuleType.ModuleGroup:
                    case ModuleType.PlatformProfile:
                        return false;
                    default:
                        throw new Exception("Unknown Module Type");
                }
            }
        }

        public string DefaultBaseAdress
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.Kernel:
                        return "0x80800000";
                    case ModuleType.Win32DLL:
                    case ModuleType.Win32OCX:
                        return "0x10000000";
                    case ModuleType.NativeDLL:
                    case ModuleType.NativeCUI:
                    case ModuleType.Win32CUI:
                    case ModuleType.Test:
                        return "0x00400000";
                    case ModuleType.Win32SCR:
                    case ModuleType.Win32GUI:
                        return "0x00400000";
                    case ModuleType.KeyboardLayout:
                    case ModuleType.KernelModeDLL:
                    case ModuleType.KernelModeDriver:
                        return "0x00010000";
                    case ModuleType.BuildTool:
                    case ModuleType.HostStaticLibrary:
                    case ModuleType.StaticLibrary:
                    case ModuleType.ObjectLibrary:
                    case ModuleType.BootLoader:
                    case ModuleType.BootSector:
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcClient:
                    case ModuleType.RpcProxy:
                    case ModuleType.Alias:
                    case ModuleType.BootProgram:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.EmbeddedTypeLib:
                    case ModuleType.Cabinet:
                    case ModuleType.Package:
                    case ModuleType.ModuleGroup:
                    case ModuleType.PlatformProfile:
                        return string.Empty;
                    default:
                        throw new Exception("Unknown Module Type");
                }
            }
        }

        public string DefaultEntrypoint
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.Kernel:
                        return "KiSystemStartup";
                    case ModuleType.KeyboardLayout:
                    case ModuleType.KernelModeDLL:
                    case ModuleType.KernelModeDriver:
                        return "DriverEntry@8";
                    case ModuleType.NativeDLL:
                        return "DllMainCRTStartup@12";
                    case ModuleType.NativeCUI:
                        return "NtProcessStartup@4";
                    case ModuleType.Win32DLL:
                    case ModuleType.Win32OCX:
                        return "DllMain@12";
                    case ModuleType.Win32CUI:
                    case ModuleType.Test:
                        {
                            if (Unicode)
                                return "wmainCRTStartup";
                            return "mainCRTStartup";
                        }
                    case ModuleType.Win32SCR:
                    case ModuleType.Win32GUI:
                        {
                            if (Unicode)
                                return "wWinMainCRTStartup";
                            return "WinMainCRTStartup";
                        }
                    case ModuleType.HostStaticLibrary:
                    case ModuleType.BuildTool:
                    case ModuleType.StaticLibrary:
                    case ModuleType.ObjectLibrary:
                    case ModuleType.BootLoader:
                    case ModuleType.BootSector:
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcClient:
                    case ModuleType.RpcProxy:
                    case ModuleType.Alias:
                    case ModuleType.BootProgram:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.EmbeddedTypeLib:
                    case ModuleType.Cabinet:
                    case ModuleType.Package:
                    case ModuleType.ModuleGroup:
                    case ModuleType.PlatformProfile:
                        return string.Empty;
                    default:
                        throw new Exception("Unknown Module Type");
                }
            }
        }

        public bool IsBuildable
        {
            get { return Type != ModuleType.Package && Type != ModuleType.ModuleGroup && Type != ModuleType.PlatformProfile; }
        }

        public bool IncludeInAllTarget
        {
            get
            {
                if (Type == ModuleType.BootSector ||
                    Type == ModuleType.Iso ||
                    Type == ModuleType.LiveIso ||
                    Type == ModuleType.IsoRegTest ||
                    Type == ModuleType.LiveIsoRegTest ||
                    Type == ModuleType.Test ||
                    Type == ModuleType.Alias)
                {
                    return false;
                }

                return true;
            }
        }

        public bool LinksToCRuntimeLibrary
        {
            get
            {
                foreach (RBuildModule module in Libraries)
                {
                    if ((module.Name == "libcntpr") ||
                        (module.Name == "crt"))
                    {
                        return true;
                    }
                }

                return false;
            }
        }

        /// <summary>
        /// Default root to use when someone references 
        /// this module by using include
        /// </summary>
        public PathRoot IncludeDefaultRoot
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.RpcClient:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcProxy:
                        return PathRoot.Intermediate;
                    default:
                        return PathRoot.SourceCode;
                }
            }
        }

        /// <summary>
        /// Gets the folder to be used when another object 
        /// references this module
        /// </summary>
        public PathRoot ReferenceDefaultRoot
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.RpcClient:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcProxy:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.BootSector:
                    case ModuleType.StaticLibrary:
                    case ModuleType.HostStaticLibrary:
                        return PathRoot.Intermediate;
                    default:
                        return PathRoot.Output;
                }
            }
        }

        /// <summary>
        /// Gets the folder to be used when another objects
        /// references the target file generated by this module
        /// </summary>
        public PathRoot TargetDefaultRoot
        {
            get
            {
                switch (Type)
                {
                    case ModuleType.Iso:
                    case ModuleType.LiveIso:
                    case ModuleType.IsoRegTest:
                    case ModuleType.LiveIsoRegTest:
                        return PathRoot.Default;
                    case ModuleType.RpcClient:
                    case ModuleType.RpcServer:
                    case ModuleType.RpcProxy:
                    case ModuleType.IdlHeader:
                    case ModuleType.MessageHeader:
                    case ModuleType.BootSector:
                    case ModuleType.StaticLibrary:
                    case ModuleType.EmbeddedTypeLib:
                    case ModuleType.HostStaticLibrary:
                        return PathRoot.Intermediate;
                    default:
                        return PathRoot.Output;
                }
            }
        }

        public bool Enabled
        {
            get { return m_Enabled; }
            set { m_Enabled = value; }
        }

        public bool MangledSymbols
        {
            get { return m_MangledSymbols; }
            set { m_MangledSymbols = value; }
        }

        public bool IsStartupLib
        {
            get { return m_IsStartupLib; }
            set { m_IsStartupLib = value; }
        }

        public bool UnderscoreSymbols
        {
            get { return m_UnderscoreSymbols; }
            set { m_UnderscoreSymbols = value; }
        }

        public bool Unicode
        {
            get { return m_Unicode; }
            set { m_Unicode = value; }
        }

        public bool AllowWarnings
        {
            get { return m_AllowWarnings; }
            set { m_AllowWarnings = value; }
        }

        /*
        public RBuildFolder Folder
        {
            get { return new RBuildFolder(PathRoot.SourceCode, Base); }
        }
        */

        /// <summary>
        /// Gets the collection of <see cref="RBuildSourceFile"/>.
        /// </summary>
        public RBuildSourceFileCollection SourceFiles
        {
            get { return m_SourceFiles; }
            set { m_SourceFiles = value; }
        }

        public RBuildLocalizationFileCollection LocalizationFiles
        {
            get { return m_LocalizationFiles; }
            set { m_LocalizationFiles = value; }
        }

        public List<RBuildRegistryKey> RegistryKeys
        {
            get { return m_RegistryKeys; }
            set { m_RegistryKeys = value; }
        }

        public RBuildAuthorCollection Authors
        {
            get { return m_Authors; }
            set { m_Authors = value; }
        }

        public List<RBuildCompilationUnitFile> CompilationUnits
        {
            get { return m_CompilationUnits; }
        }

        public RBuildExportedFunctionsCollection ExportedFunctions
        {
            get { return m_ExportedFunctions; }
        }

        public RBuildAPIStatusCollection ApiInfo
        {
            get { return m_ApiInfo; }
        }

        public RBuildFamilyCollection Families
        {
            get { return m_Families; }
        }

        public string ModulePath
        {
            get { return m_Path + @"\"; }
        }

        public RBuildFolder TargetFolder
        {
            get 
            {
                RBuildFolder folder = null;

                folder = new RBuildFolder();
                folder.Root = TargetDefaultRoot;

                //Las ISO son excepciones a la regla , generan su resultado en el raiz y no en la carpeta
                //del módulo donde se encuentran
                if (Type != ModuleType.Iso &&
                    Type != ModuleType.LiveIso &&
                    Type != ModuleType.IsoRegTest &&
                    Type != ModuleType.LiveIsoRegTest)
                {
                    folder.Name = Folder.Name;
                    folder.Base = Folder.Base;
                }

                return folder;
            }
        }

        public RBuildFile TargetFile
        {
            get
            {
                RBuildFile file = null;

                file = new RBuildFile();
                file.Name = TargetName;
                file.Base = TargetFolder.FullPath;
                file.Root = TargetFolder.Root;

                return file;
            }
        }

        public RBuildFile Install
        {
            get
            {
                RBuildFile file = null;

                file = new RBuildFile();
                file.Base = InstallBase;
                file.Name = InstallName;
                file.Root = PathRoot.Install;

                return file;
            }
        }

        public RBuildFile PlatformInstall
        {
            get
            {
                RBuildFile file = null;

                file = new RBuildFile();
                file.Base = "%SystemRoot%\\" + InstallBase;
                file.Name = InstallName;
                file.Root = PathRoot.Platform;

                return file;
            }
        }

        public RBuildFile Dependency
        {
            get
            {
                RBuildFile file = null;

                file = new RBuildFile();
                file.Base = Folder.FullPath;
                file.Name = DependencyName;
                file.Root = PathRoot.Intermediate; // ReferenceDefaultRoot;

                return file;
            }
        }

        public string CDLabel
        {
            get { return "ReactOS"; }
            set { m_CDLabel = value; }
        }

        public string TargetName
        {
            get
            {
                if (OutputName != null)
                    return OutputName;

                if (InstallName != null)
                    return InstallName;

                return string.Format("{0}{1}", Name, Extension);
            }
        }

        public string DependencyName
        {
            get
            {
                if (HasImportLibrary)
                    return string.Format("lib{0}.a" , Name);

                //Get the regular name
                return string.Format("{0}.a" , Name);
            }
        }

        public string InstallBase
        {
            get { return m_InstallBase; }
            set { m_InstallBase = value; }
        }

        public string InstallName
        {
            get { return m_InstallName; }
            set { m_InstallName = value; }
        }

        public string OutputName
        {
            get { return m_OutputName; }
            set { m_OutputName = value; }
        }

        public bool Host
        {
            get { return m_HostBuild; }
            set { m_HostBuild = value; }
        }

        public bool HasImportLibrary
        {
            get { return (ImportLibrary != null) && (Type != ModuleType.StaticLibrary); }
        }

        public bool HasMessageTables
        {
            get
            {
                foreach (RBuildSourceFile source in SourceFiles)
                    if (source.Type == SourceType.MessageTable)
                        return true;

                return false;
            }
        }

        public bool HasIDLs
        {
            get
            {
                foreach (RBuildSourceFile source in SourceFiles)
                    if (source.Type == SourceType.IDL)
                        return true;

                return false;
            }
        }

        ///*
        //public string[] BaseLocation
        //{ 
        //    get { return Base.Split(new char[] { '\\' }); }
        //}

        //public string[] PathLocation
        //{
        //    get
        //    {
        //        Uri uri = new Uri(Base , UriKind.Relative);

        //        return Base.Split(new char[] { '\\' });

        //        /*
        //        DirectoryInfo info = new DirectoryInfo(Base);
        //        return info.Parent.FullName.Split(new char[] { '\\' });
        //         */
        //    }
        //}
        //*/

        public bool IsModuleInRoot
        {
            get { return Path == string.Empty; }
        }

        public string Description
        {
            get { return m_Description; }
            set { m_Description = value; }
        }

        public string LCID
        {
            get { return m_LCID; }
            set { m_LCID = value; }
        }

        public RBuildModuleCollection Dependencies
        {
            get { return m_Dependencies; }
        }

        public RBuildModuleCollection Requeriments
        {
            get { return m_Requeriments; }
        }

        public RBuildModuleCollection Libraries
        {
            get { return m_Libraries; }
        }

        public RBuildModuleCollection Needs
        {
            get 
            {
                RBuildModuleCollection modules = new RBuildModuleCollection();

                modules.Add(Dependencies);
                modules.Add(Libraries);
                modules.Add(Requeriments);

                return modules; 
            }
        }

        public ModuleType Type
        {
            get { return m_Type; }
            set { m_Type = value; }
        }

        public RBuildSetupFile Setup
        {
            get { return m_RBuildSetup; }
            set { m_RBuildSetup = value; }
        }

        public RBuildAutoRegister AutoRegister
        {
            get { return m_AutoRegister; }
            set { m_AutoRegister = value; }
        }

        public string MakeFileTargetMacro
        {
            get { return string.Format("$({0}_TARGET)", Name); }
        }

        public string MakeFileTarget
        {
            get { return string.Format("{0}_TARGET", Name); }
        }

        public string MakeFileLibs
        {
            get { return string.Format("{0}_LIBS", Name); }
        }

        public string MakeFileLinkDeps
        {
            get { return string.Format("{0}_LINKDEPS", Name); }
        }

        public string MakeFileLinkDepsMacro
        {
            get { return string.Format("$({0}_LINKDEPS)", Name); }
        }

        public string MakeFileLibsMacro
        {
            get { return string.Format("$({0}_LIBS)", Name); }
        }

        public override void SaveAs(string moduleFile)
        {
            // Creates an XML file is not exist 
            using (XmlTextWriter writer = new XmlTextWriter(moduleFile, Encoding.ASCII))
            {
                writer.Indentation = 4;
                writer.Formatting = Formatting.Indented;

                // Starts a new document 
                writer.WriteStartDocument();

                writer.WriteComment("File autogenerated by RosBuilder 0.1");
                writer.WriteStartElement("module");

                writer.WriteAttributeString("name", Name);
                writer.WriteAttributeString("type", Type.ToString());
                writer.WriteAttributeString("installbase", InstallBase);
                writer.WriteAttributeString("installname", InstallName);
                writer.WriteAttributeString("unicode", Unicode.ToString());
                writer.WriteAttributeString("allowwarnings", AllowWarnings.ToString());
                writer.WriteAttributeString("underscoresymbols", UnderscoreSymbols.ToString());
                writer.WriteAttributeString("baseadress", BaseAddress);
                writer.WriteAttributeString("entrypoint", EntryPoint);
                writer.WriteAttributeString("extension", Extension);
                writer.WriteAttributeString("isstartuplib", IsStartupLib.ToString());
                writer.WriteAttributeString("mangledsymbols", MangledSymbols.ToString());

                writer.WriteStartElement("include");
                writer.WriteAttributeString("base", Name);
                writer.WriteString(".");
                writer.WriteEndElement();

                foreach (RBuildFolder include in IncludeFolders)
                {
                    writer.WriteStartElement("include");
                    writer.WriteAttributeString("base", include.Base);
                    writer.WriteString(include.Name);
                    writer.WriteEndElement();
                }

                foreach (RBuildDefine define in Defines)
                {
                    writer.WriteStartElement("define");
                    writer.WriteAttributeString("name", define.Name);
                    
                    if (define.Name != string.Empty)
                    {
                        writer.WriteString(define.Value);
                    }

                    writer.WriteEndElement();
                }

                foreach (RBuildModule dependency in Dependencies)
                {
                    writer.WriteStartElement("dependency");
                    writer.WriteString(dependency.Name);
                    writer.WriteEndElement();
                }

                foreach (RBuildModule library in Libraries)
                {
                    writer.WriteStartElement("library");
                    writer.WriteString(library.Name);
                    writer.WriteEndElement();
                }

                foreach (RBuildSourceFile sourceFile in SourceFiles)
                {
                    if (sourceFile.IsCompilable)
                    {
                        if (sourceFile.Switches != string.Empty)
                        {
                            writer.WriteAttributeString("switches", sourceFile.Switches);
                        }

                        writer.WriteStartElement("file");
                        writer.WriteString(sourceFile.Name);
                        writer.WriteEndElement();
                    }
                }

                if (PreCompiledHeader != null)
                {
                    writer.WriteStartElement("pch");
                    writer.WriteString(PreCompiledHeader.Name);
                    writer.WriteEndElement();
                }

                writer.WriteEndDocument();
            }
        }

        public override string ToString()
        {
            return string.Format("Module : '{0}' Type : {1} Base : '{2}' Libraries : '{3}' Dependencies : '{4}' Requeriments : '{5}'",
                Name,
                Type,
                Base,
                Libraries.Count,
                Dependencies.Count,
                Requeriments.Count);
        }
    }
}
