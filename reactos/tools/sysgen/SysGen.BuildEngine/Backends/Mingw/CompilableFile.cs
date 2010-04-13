using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Tasks;

namespace SysGen.BuildEngine.Backends
{
    public class BackedBuildIncludeFolder : BackedBuildFolder
    {
        public BackedBuildIncludeFolder(RBuildFolder folder, SysGenEngine sysgen)
            : base(sysgen)
        {
            m_RBuildFile = folder;
        }
    }

    public class BackendBuildFile : BackendBuildFileSystemInfo
    {
        public BackendBuildFile(SysGenEngine sysgen)
            : base(sysgen)
        {
            m_RBuildFile = new RBuildFile();
        }

        public string GetPathWithNewExtension(PathRoot root, string newExtension)
        {
            return GetPath(root, BuildFile.Name, newExtension);
        }

        public string GetPathWithNewName(PathRoot root, string newName)
        {
            return GetPath(root, newName, Path.GetExtension(BuildFile.Name));
        }

        public string GetPath(PathRoot root, string newName, string newExtension)
        {
            return Path.Combine(SysGen.GetPathRoot(root), Path.ChangeExtension(newName, newExtension));
        }

        public string IntermediateFolderFullPath
        {
            get { return Path.Combine(SysGen.GetPathRoot(PathRoot.Intermediate), BuildFile.Base) /*+ @"\."*/; }
        }

        public string BaseFolderFullPath
        {
            get { return Path.Combine(SysGen.GetPathRoot(PathRoot.SourceCode), BuildFile.Base) /*+ @"\."*/; }
        }

        public RBuildFile BuildFile
        {
            get { return m_RBuildFile as RBuildFile; }
        }
    }

    public class BackedBuildFolder : BackendBuildFileSystemInfo
    {
        public BackedBuildFolder(RBuildFolder folder, SysGenEngine sysgen)
            : base(sysgen)
        {
            m_RBuildFile = folder;
        }

        public BackedBuildFolder(SysGenEngine sysgen)
            : base(sysgen)
        {
            m_RBuildFile = new RBuildFolder();
        }

        public RBuildFolder BuildFolder
        {
            get { return m_RBuildFile as RBuildFolder; }
        }
    }

    public abstract class BackendBuildFileSystemInfo
    {
        protected RBuildFileSystemInfo m_RBuildFile = null;
        protected SysGenEngine m_SysGenEngine = null;

        public BackendBuildFileSystemInfo(SysGenEngine sysgen)
        {
            m_SysGenEngine = sysgen;
        }

        public SysGenEngine SysGen
        {
            get { return m_SysGenEngine; }
        }

        public string RelativePath
        {
            get { return SysGen.NormalizePath(m_RBuildFile.FullPath); }
        }

        public string GetPath(PathRoot root)
        {
            return Path.Combine(SysGen.GetPathRoot(root), RelativePath);
        }

        public string OriginalFullPath
        {
            get { return Path.Combine(SysGen.GetPathRoot(m_RBuildFile.Root), RelativePath); }
        }

        public string BaseFullPath
        {
            get { return Path.Combine(SysGen.BaseDirectory, RelativePath); }
        }

        public string IntermediateFullPath
        {
            get { return Path.Combine(SysGen.IntermediateDirectory, RelativePath); }
        }

        public string OutputFullPath
        {
            get { return Path.Combine(SysGen.OutputDirectory, RelativePath); }
        }

        public string BootCDOutputDirectory
        {
            get { return Path.Combine(SysGen.BootCDOutputDirectory, RelativePath); }
        }

        public string TemporaryFullPath
        {
            get { return Path.Combine(SysGen.TemporaryDirectory, RelativePath); }
        }

        public string InstallFullPath
        {
            get { return Path.Combine(SysGen.InstallDirectory, RelativePath); }
        }

        public RBuildModule Module
        {
            get { return m_RBuildFile.Element as RBuildModule; }
        }
    }

    public class SourceFile : BackendBuildModule
    {
        RBuildSourceFile m_File = null;
        BackendBuildFile m_SourceCodeFile = null;
        BackendBuildFile m_SourceCodeObjectFile = null;
        BackendBuildFile m_SourceCodeActualFile = null;
        BackendBuildFile m_SourceCodeHeaderFile = null;
        BackendBuildFile m_SourceCodePCHeaderFile = null;
        BackendBuildFile m_SourceRpcClientHeaderFile = null;
        BackendBuildFile m_SourceRpcServerHeaderFile = null;
        BackendBuildFile m_SourceMessageTableHeaderFile = null;
        BackendBuildFile m_SourceMessageTableResourceFile = null;
        BackendBuildFile m_PCH = null;
        BackendBuildFile m_PCHTemp = null;

        public SourceFile(RBuildSourceFile file, RBuildModule module, SysGenEngine sysgen)
            : base(module, sysgen)
        {
            m_File = file;

            m_SourceCodeFile = new BackendBuildFile(sysgen);
            m_SourceCodeFile.BuildFile.Name = file.Name;
            m_SourceCodeFile.BuildFile.Element = module;
            m_SourceCodeFile.BuildFile.Base = file.Base;
            m_SourceCodeFile.BuildFile.Root = file.Root;

            m_SourceCodeObjectFile = new BackendBuildFile(sysgen);
            m_SourceCodeObjectFile.BuildFile.Name = GetObjectFileName(file);
            m_SourceCodeObjectFile.BuildFile.Element = module;
            m_SourceCodeObjectFile.BuildFile.Base = file.Base;

            m_SourceCodeActualFile = new BackendBuildFile(sysgen);
            m_SourceCodeActualFile.BuildFile.Name = GetActualSourceFile(file);
            m_SourceCodeActualFile.BuildFile.Element = module;
            m_SourceCodeActualFile.BuildFile.Base = file.Base;

            m_SourceCodeHeaderFile = new BackendBuildFile(sysgen);
            m_SourceCodeHeaderFile.BuildFile.Name = GetHeaderFile(file);
            m_SourceCodeHeaderFile.BuildFile.Element = module;
            m_SourceCodeHeaderFile.BuildFile.Base = file.Base;

            m_SourceRpcClientHeaderFile = new BackendBuildFile(sysgen);
            m_SourceRpcClientHeaderFile.BuildFile.Name = GetRpcClientHeaderFile(file);
            m_SourceRpcClientHeaderFile.BuildFile.Element = module;
            m_SourceRpcClientHeaderFile.BuildFile.Base = file.Base;

            m_SourceRpcServerHeaderFile = new BackendBuildFile(sysgen);
            m_SourceRpcServerHeaderFile.BuildFile.Name = GetRpcServerHeaderFile(file);
            m_SourceRpcServerHeaderFile.BuildFile.Element = module;
            m_SourceRpcServerHeaderFile.BuildFile.Base = file.Base;

            m_SourceMessageTableHeaderFile = new BackendBuildFile(sysgen);
            m_SourceMessageTableHeaderFile.BuildFile.Name = GetMessageTableHeaderFile(file);
            m_SourceMessageTableHeaderFile.BuildFile.Element = module;
            m_SourceMessageTableHeaderFile.BuildFile.Base = /*file.Base; //*/ "include/reactos";

            m_SourceMessageTableResourceFile = new BackendBuildFile(sysgen);
            m_SourceMessageTableResourceFile.BuildFile.Name = GetMessageTableResourceFile(file);
            m_SourceMessageTableResourceFile.BuildFile.Element = module;
            m_SourceMessageTableResourceFile.BuildFile.Base = file.Base;
        }

        public string GetMessageTableResourceFile(RBuildFile file)
        {
            return Path.GetFileNameWithoutExtension(file.Name) + ".rc";
        }

        public string GetMessageTableHeaderFile(RBuildFile file)
        {
            return Path.GetFileNameWithoutExtension(file.Name) + ".h";
        }

        public string GetRpcServerHeaderFile(RBuildFile file)
        {
            return Path.GetFileNameWithoutExtension(file.Name) + "_s.h";
        }

        public string GetRpcClientHeaderFile(RBuildFile file)
        {
            return Path.GetFileNameWithoutExtension(file.Name) + "_c.h";
        }

        public string GetRpcProxyHeaderFile(RBuildFile file)
        {
            return Path.GetFileNameWithoutExtension(file.Name) + "_p.h";
        }

        private string GetHeaderFile(RBuildFile file)
        {
            switch (file.Extension)
            {
                case ".idl":
                    {
                        if (Module.Type == ModuleType.RpcServer)
                            return GetRpcServerHeaderFile(file);

                        if (Module.Type == ModuleType.RpcClient)
                            return GetRpcClientHeaderFile(file);

                        if (Module.Type == ModuleType.RpcProxy)
                            return GetRpcProxyHeaderFile(file);

                        return Path.ChangeExtension(file.Name, ".h");
                    }
                    break;
                default:
                    return file.Name;
            }
        }

        private string GetActualSourceFile(RBuildFile file)
        {
            switch (file.Extension)
            {
                case ".spec":
                    return Path.ChangeExtension(file.Name, ".stubs.c");
                    break;
                case ".idl":
                    {
                        if (Module.Type == ModuleType.RpcServer)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_s.c";

                        if (Module.Type == ModuleType.RpcClient)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_c.c";

                        if (Module.Type == ModuleType.RpcProxy)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_p.c";

                        return Path.ChangeExtension(file.Name, ".h");
                    }
                    break;
                default:
                    return file.Name;
            }
        }

        private string GetObjectFileName(RBuildFile file)
        {
            string filename = null;
            switch (file.Extension)
            {
                case ".h":
                    filename = Path.ChangeExtension(file.Name, ".h.gch");
                    break;
                case ".mc":
                    filename = Path.ChangeExtension(file.Name, ".rc");
                    break;
                case ".rc":
                    filename = Path.ChangeExtension(file.Name, ".coff");
                    break;
                case ".spec":
                    filename = Path.ChangeExtension(file.Name, ".stubs.o");
                    break;
                case ".idl":
                    {
                        if (Module.Type == ModuleType.RpcServer)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_s.o";

                        if (Module.Type == ModuleType.RpcClient)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_c.o";

                        if (Module.Type == ModuleType.RpcProxy)
                            return Path.GetFileNameWithoutExtension(file.Name) + "_p.o";

                        /*
                        if (Module.Type == ModuleType.EmbeddedTypeLib)
                            return Path.GetFileNameWithoutExtension(file.Name) + ".tlb";
                        */

                        filename = Path.ChangeExtension(file.Name, ".h");
                    }
                    break;
                //case ".c":
                //case ".cpp":
                //case ".cxx":
                //    {
                //        filename = Path.ChangeExtension(file.Name, ".o");
                //    }
                //    break;
                default:
                    filename = Path.ChangeExtension(file.Name, ".o");

                    //HACK:
                    if (Module.Type == ModuleType.BootSector)
                        return filename;

                    //filename = string.Format("{1}_{0}{2}",
                    //    Module.Name,
                    //    Path.GetFileNameWithoutExtension(filename),
                    //    Path.GetExtension(filename));
                    break;
            }

            if (file.Extension == ".mc" ||
                file.Extension == ".rc" ||
                file.Extension == ".c" ||
                file.Extension == ".cpp" ||
                file.Extension == ".cxx" ||
                file.Extension == ".asm" ||
                file.Extension == ".s")
            {
                filename = string.Format("{0}_{1}{2}",
                    Path.GetFileNameWithoutExtension(filename),
                    Module.Name,
                    Path.GetExtension(filename));
            }
            else if (file.Extension == ".h")
            {
                return filename;
            }

            return filename;
        }

        public BackendBuildFile SourceCodeFile
        {
            get { return m_SourceCodeFile; }
        }

        public BackendBuildFile SourceCodeHeaderFile
        {
            get { return m_SourceCodeHeaderFile; }
        }

        public BackendBuildFile SourceCodeObjectFile
        {
            get { return m_SourceCodeObjectFile; }
        }

        public BackendBuildFile SourceCodeActualFile
        {
            get { return m_SourceCodeActualFile; }
        }

        public BackendBuildFile SourceRpcClientHeaderFile
        {
            get { return m_SourceRpcClientHeaderFile; }
        }

        public BackendBuildFile SourceRpcServerHeaderFile
        {
            get { return m_SourceRpcServerHeaderFile; }
        }

        public BackendBuildFile SourceCodePCHeaderFile
        {
            get { return m_SourceCodePCHeaderFile; }
        }

        public RBuildSourceFile File
        {
            get { return m_File; }
        }

        public BackendBuildFile MessageTableHeaderFile
        {
            get { return m_SourceMessageTableHeaderFile; }
        }

        public BackendBuildFile MessageTableResourceFile
        {
            get { return m_SourceMessageTableResourceFile; }
        }
    }

    public class LibraryModule : BackendBuildModule
    {
        BackendBuildFile m_Dependency = null;

        public LibraryModule(RBuildModule module, SysGenEngine sysgen) : base (module , sysgen)
        {
            m_Dependency = new BackendBuildFile(sysgen);
            m_Dependency.BuildFile.Name = module.DependencyName;
            m_Dependency.BuildFile.Element = module;
            m_Dependency.BuildFile.Base = module.Base;
        }

        public BackendBuildFile Dependency
        {
            get { return m_Dependency; }
        }
    }

    public class BackendBuildModule
    {
        SysGenEngine m_SysGenEngine = null;
        RBuildModule m_Module = null;
        BackendBuildFile m_Target = null;
        BackendBuildFile m_TargetNoStrip = null;
        BackendBuildFile m_Dependency = null;
        BackendBuildFile m_Definition = null;

        public BackendBuildModule(RBuildModule module, SysGenEngine sysgen)
        {
            m_Module = module;
            m_SysGenEngine = sysgen;

            m_Target = new BackendBuildFile(sysgen);
            m_Target.BuildFile.Name = module.TargetFile.Name; //module.TargetName;
            m_Target.BuildFile.Element = module;
            m_Target.BuildFile.Base = module.TargetFile.Base; //module.Base;

            m_TargetNoStrip = new BackendBuildFile(sysgen);
            m_TargetNoStrip.BuildFile.Name = GetNoStripTargetName(module);
            m_TargetNoStrip.BuildFile.Element = module;
            m_TargetNoStrip.BuildFile.Base = module.Base;

            m_Dependency = new BackendBuildFile(sysgen);
            m_Dependency.BuildFile.Name = module.DependencyName;
            m_Dependency.BuildFile.Element = module;
            m_Dependency.BuildFile.Base = module.Base;


            m_Definition = new BackendBuildFile(sysgen);
            m_Definition.BuildFile.Element = module;

            if (module.ImportLibrary == null)
            {
                m_Definition.BuildFile.Name = "tools/rbuild/empty.def";
                m_Definition.BuildFile.Base = sysgen.BaseDirectory;
            }
            else
            {
                if (IsWineModule)
                    m_Definition.BuildFile.Root = PathRoot.Intermediate;

                m_Definition.BuildFile.Name = module.ImportLibrary.Definition;
                m_Definition.BuildFile.Base = module.ImportLibrary.Base;
            }
        }

        public bool IsWineModule
        {
            get
            {
                if (Module.ImportLibrary == null)
                    return false;

                return ((Module.ImportLibrary.Definition != null) && 
                        (Module.ImportLibrary.Definition != string.Empty) && 
                        (Module.ImportLibrary.Definition.Contains(".spec.def")));
            }
        }

        public string GetNoStripTargetName(RBuildModule module)
        {
            return string.Format("{0}.nostrip{1}", 
                Path.GetFileNameWithoutExtension(module.TargetName),
                Path.GetExtension(module.TargetName));
        }

        public string LibTempFileName
        {
            get
            {
                if (m_Module.Type == ModuleType.StaticLibrary)
                    return m_Module.TargetName;

                return Path.ChangeExtension(m_Module.TargetName, ".temp.a");
            }
        }

        public string ExpTempFileName
        {
            get
            {
                if (m_Module.Type == ModuleType.StaticLibrary)
                    return m_Module.TargetName;

                return Path.ChangeExtension(m_Module.TargetName, ".temp.exp");
            }
        }

        public string JunkTempFileName
        {
            get
            {
                if (m_Module.Type == ModuleType.StaticLibrary)
                    return m_Module.TargetName;

                return Path.ChangeExtension(m_Module.TargetName, ".junk.tmp");
            }
        }

        public string RcTempFileName
        {
            get
            {
                if (m_Module.Type == ModuleType.StaticLibrary)
                    return m_Module.TargetName;

                return Path.ChangeExtension(m_Module.TargetName, ".rci.tmp");
            }
        }

        public string ResTempFileName
        {
            get
            {
                if (m_Module.Type == ModuleType.StaticLibrary)
                    return m_Module.TargetName;

                return Path.ChangeExtension(m_Module.TargetName, ".res.tmp");
            }
        }

        public string JunkTempFileNameFullPath
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.SourceCode), JunkTempFileName); }
        }

        public string RcTempFileNameFullPath
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Temporary), RcTempFileName); }
        }

        public string ResTempFileNameFullPath
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Intermediate), ResTempFileName); }
        }

        public string ExpTempFileNameFullPath
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.SourceCode), ExpTempFileName); }
        }

        public string LibTempFileNameFullPath
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Intermediate), LibTempFileName); }
        }

        public string ModuleIntermediateLocation
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Intermediate), Module.Base); }
        }

        public string ModuleOutputLocation
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Output), Module.Base); }
        }

        public string ModuleBaseIntermediateLocation
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Intermediate), Module.Base); }
        }

        public string ModuleBaseOutputLocation
        {
            get { return Path.Combine(m_SysGenEngine.GetPathRoot(PathRoot.Output), Module.Base); }
        }

        public RBuildModule Module
        {
            get { return m_Module; }
        }

        public BackendBuildFile Target
        {
            get { return m_Target; }
        }

        public BackendBuildFile Dependency
        {
            get { return m_Dependency; }
        }

        public BackendBuildFile Definition
        {
            get { return m_Definition; }
        }

        public BackendBuildFile TargetNoStrip
        {
            get { return m_TargetNoStrip; }
        }
    }
}