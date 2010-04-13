using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public enum LinkerSubSystem
    {
        Windows,
        Console,
        Native
    }

    public abstract class MingwRBuildMakefileGenerator
    {
        protected SysGenEngine m_SysGenEngine = null;
        protected MakefileWriter m_Makefile = null;

        public SysGenEngine SysGen
        {
            get { return m_SysGenEngine; }
            set { m_SysGenEngine = value; }
        }

        public MakefileWriter Makefile
        {
            get { return m_Makefile; }
            set { m_Makefile = value; }
        }

        protected virtual string ResolveRBuildFilePath(RBuildFile file)
        {
            return SysGen.ResolveRBuildFilePath(file);
        }

        protected virtual string ResolveRBuildFolderPath(RBuildFolder folder)
        {
            return SysGen.ResolveRBuildFolderPath(folder);
        }

        public virtual string ResolveRBuildFilePath(PathRoot root, RBuildFileSystemInfo file)
        {
            return SysGen.ResolveRBuildFilePath(root , file);
        }

        public abstract void GenerateMakeFile();
    }

    public abstract class MingwRBuildElementHandler : MingwRBuildMakefileGenerator
    {
        protected RBuildElement m_BuildElement = null;

        public MingwRBuildElementHandler(RBuildElement element)
        {
            m_BuildElement = element;
        }

        public override void GenerateMakeFile()
        {
            /* Do checking */
            CheckSourceFiles();

            /* Do makefile generation */
            WriteCommon();
            WriteSpecific();
            //WritePCHFiles();
            WriteFiles();
            WriteLinker();
            WriteImportLibrary();
            WriteCleanTarget();
        }

        protected virtual void WriteFolders()
        {
            Makefile.WritePropertyListStart(BuildElement.MakeFileFolders);
            WriteElementFolders(Makefile, BuildElement);
            Makefile.WritePropertyListEnd();
        }

        protected virtual void CheckSourceFiles()
        {
        }

        protected virtual void WriteCommon()
        {
            WriteFolders();
            WriteCFlags();
            WriteRCFlags();
            WriteLFlags();
            WriteWIDLFlags();
        }

        protected virtual void WriteStrip()
        {
        }

        protected virtual void WriteNonSymbolStripped()
        {
        }

        protected virtual void WriteRsym()
        { 
        }

        protected virtual void WriteImportLibrary()
        {
        }

        protected virtual void WriteLinker()
        {
        }

        protected virtual void WriteCleanTarget()
        {
        }

        protected virtual void WriteSpecific()
        {
        }

        protected virtual void WriteFiles()
        { 
        }

        protected virtual void WritePCHFiles()
        {
        }

        protected virtual void WriteCFlags()
        {
            Makefile.WritePropertyListStart(BuildElement.MakeFileCFlags);
            WriteElementIncludes(Makefile, BuildElement);
            WriteElementDefines(Makefile, BuildElement);
            Makefile.WritePropertyListEnd();
        }

        protected virtual void WriteLFlags()
        {
            Makefile.WritePropertyListStart(BuildElement.MakeFileLFlags);
            WriteLinkerFlags(Makefile, BuildElement);
            Makefile.WritePropertyListEnd();
        }

        protected virtual void WriteRCFlags()
        {
            Makefile.WriteProperty(BuildElement.MakeFileRCFlags, BuildElement.MakeFileCFlagsMacro);
        }

        protected virtual void WriteWIDLFlags()
        {
            Makefile.WriteProperty(BuildElement.MakeFileWIDLFlags, BuildElement.MakeFileCFlagsMacro);
        }

        protected void WriteElementDefines(MakefileWriter makefile, RBuildElement element)
        {
            foreach (RBuildDefine define in element.Defines)
            {
                if (!define.IsEmpty)
                {
                    makefile.WriteIndentedLine("-D" + define.Name + "=" + define.Value);
                }
                else
                    makefile.WriteIndentedLine("-D" + define.Name);
            }
        }

        protected void WriteElementDependencyFlags(MakefileWriter makefile, RBuildModule module)
        {
            foreach (RBuildModule dependency in module.Dependencies)
            {
                Makefile.WriteIndentedLine(dependency.MakeFileTargetMacro);
            }
        }

        protected void WriteElementIncludes(MakefileWriter makefile, RBuildElement element)
        {
            foreach (RBuildFolder includeFolder in element.IncludeFolders)
            {
                Makefile.WriteIndentedLine("-I" + ResolveRBuildFolderPath(includeFolder));
            }
        }

        protected void WriteElementFolders(MakefileWriter makefile, RBuildElement element)
        {
            foreach (RBuildFolder folder in element.Folders)
            {
                Makefile.WriteIndentedLine(ResolveRBuildFolderPath(folder));
            }
        }

        protected void WriteModuleAssemblyFlags(MakefileWriter makefile, RBuildModule module)
        {
            foreach (string assemblyFlag in module.AssemblyFlags)
            {
                Makefile.WriteIndentedLine(assemblyFlag);
            }
        }

        protected void WriteCompilerFlags(MakefileWriter makefile, RBuildElement element)
        {
            foreach (string compilerFlag in element.CompilerFlags)
            {
                Makefile.WriteIndentedLine(compilerFlag);
            }
        }

        protected void WriteLinkerFlags(MakefileWriter makefile, RBuildElement element)
        {
            foreach (string linkerFlag in element.LinkerFlags)
            {
                Makefile.WriteIndentedLine(linkerFlag);
            }
        }

        public RBuildElement BuildElement
        {
            get { return m_BuildElement; }
        }
    }

    public abstract class MingwRBuildModuleHandler : MingwRBuildElementHandler
    {
        private RBuildProject m_Project = null;
        private RBuildModule m_Module = null;

        private BackendBuildModule m_CompModule = null;
        private BackedBuildFolder m_Folder = null;

        public MingwRBuildModuleHandler(RBuildModule module)
            : base(module)
        {
            m_Module = module;
        }

        ////Para que sirve?
        //public bool ReferenceObjects
        //{
        //    get
        //    {
        //        switch (Module.Type)
        //        {
        //            case ModuleType.RpcServer:
        //            case ModuleType.RpcClient:
        //            case ModuleType.RpcProxy:
        //            case ModuleType.ObjectLibrary:
        //            //case ModuleType.IdlHeader:
        //            //case ModuleType.MessageHeader:
        //                return true;
        //        }

        //        return false;
        //    }
        //}

        public string ModuleTarget
        {
            get
            {
                if (Module.Type == ModuleType.IdlHeader)
                    return Module.MakeFileHeadersMacro;

                if (Module.Type == ModuleType.MessageHeader)
                    return Module.MakeFileMCHeadersMacro;

                if (Module.Type == ModuleType.RpcServer ||
                    Module.Type == ModuleType.RpcClient ||
                    Module.Type == ModuleType.RpcProxy ||
                    Module.Type == ModuleType.ObjectLibrary)
                {
                    return Module.MakeFileObjsMacro;
                }

                if (Module.TargetFile.Root == PathRoot.Intermediate ||
                    Module.TargetFile.Root == PathRoot.Output ||
                    Module.TargetFile.Root == PathRoot.Default)
                {
                    return ResolveRBuildFilePath(Module.TargetFile);
                }
                else 
                    throw new BuildException("Don't know module target");
            }
        }

        public override void GenerateMakeFile()
        {
            //m_Project = SysGen.Project;

            m_CompModule = new BackendBuildModule(Module, SysGen);
            m_Folder = new BackedBuildFolder(SysGen);
            m_Folder.BuildFolder.Base = Module.Folder.Base;
            m_Folder.BuildFolder.Name = Module.Folder.Name;
            m_Folder.BuildFolder.Element = Module;

            // Si se trata de un WinModule agregamos un include a la carpeta intermedia
            // ya que algunas dlls de wine generan ahi sus recursos incrustrados como iconos
            // o bitmaps
            if (CompilableModule.IsWineModule)
            {
                Module.IncludeFolders.Add(new RBuildFolder(PathRoot.Intermediate, Module.Base));
            }

            // Llamamos a la clase base
            base.GenerateMakeFile();
        }

        protected override void WriteFiles()
        {
            foreach (RBuildSourceFile file in Module.SourceFiles)
            {
                SourceFile sourceFile = new SourceFile(file, Module, SysGen);

                if (CanCompile(file))
                {
                    WriteFileBuildInstructions(sourceFile);
                }
                else
                    throw new Exception("Don't know how to write build instructions for '" + sourceFile.SourceCodeFile.OriginalFullPath + "' on module '" + Module.Name + "'");
            }
        }

        protected abstract void WriteFileBuildInstructions(SourceFile sourceFile);

        protected abstract bool CanCompile(RBuildSourceFile file);

        protected override void WriteSpecific()
        {
            WriteModuleCommon();
            WritePreconditions();
        }

        protected virtual void WritePreconditions()
        {
            Makefile.WritePropertyAppendListStart(Module.MakeFilePreCondition);
            WriteElementDependencyFlags(Makefile, Module);
            Makefile.WritePropertyListEnd();
            Makefile.WriteLine();

            foreach (RBuildSourceFile file in Module.SourceFiles)
            {
                if (file.IsCompilable)
                {
                    Makefile.WriteLine("{0}: {1}", ResolveRBuildFilePath(file), Module.MakeFilePreConditionMacro);
                }
            }

            Makefile.WriteLine();
        }

        protected virtual void WriteWidl()
        {
            Makefile.WritePropertyListStart(Module.MakeFileWIDLFlags);
            WriteElementIncludes(Makefile, Module);
            Makefile.WritePropertyListEnd();

            Makefile.WritePropertyAppend(Module.MakeFileWIDLFlags, Project.MakeFileWIDLFlagsMacro);
        }

        protected virtual void WriteLibs()
        {
            Makefile.WritePropertyListStart(Module.MakeFileLibs);

            foreach (RBuildModule dependency in Module.Libraries)
            {
                if (dependency.IsDLL || dependency.IsLibrary || dependency.IsRPC)
                {
                    if (dependency.Type == ModuleType.ObjectLibrary ||
                        dependency.Type == ModuleType.RpcClient ||
                        dependency.Type == ModuleType.RpcServer ||
                        dependency.Type == ModuleType.RpcProxy)
                    {
                        Makefile.WriteIndentedLine(dependency.MakeFileTargetMacro);
                    }
                    else
                    {
                        Makefile.WriteIndentedLine(ResolveRBuildFilePath(dependency.Dependency));
                    }
                }
            }

            Makefile.WritePropertyListEnd();
        }

        protected void WriteModuleCommon()
        {
            WriteLibs();
            WriteWidl();

            if (Module.Host == false)
            {
                Makefile.WritePropertyAppend(Module.MakeFileCFlags, Project.MakeFileCFlagsMacro);
                Makefile.WritePropertyAppend(Module.MakeFileRCFlags, Project.MakeFileRCFlagsMacro);
                Makefile.WritePropertyAppend(Module.MakeFileLFlags, Project.MakeFileLFlagsMacro);
            }
            else
            {
                Makefile.WritePropertyAppend(Module.MakeFileLFlags, "$(HOST_LFLAGS)");
            }

            WriteLinkDeps();

            if (Module.AssemblyFlags.Count > 0)
            {
                Makefile.WritePropertyListStart(Module.MakeFileNASMFlags);
                WriteModuleAssemblyFlags(Makefile, Module);
                Makefile.WritePropertyListEnd();
            }

            Makefile.WritePropertyAppendListStart(Module.MakeFileCFlags);
            WriteCompilerFlags(Makefile, Module);
            Makefile.WritePropertyListEnd();

            Makefile.WriteLine();
        }

        protected virtual void WriteLinkDeps()
        {
            Makefile.WritePropertyAppend(Module.MakeFileLinkDeps, Module.MakeFileLibsMacro);
        }

        public virtual string RpcHeaderDependencies
        {
            get 
            {
                string dependencies = string.Empty;

                foreach (RBuildModule module in Module.Libraries)
                {
                    if ((module.Type == ModuleType.RpcClient) ||
                        (module.Type == ModuleType.RpcServer) ||
                        (module.Type == ModuleType.IdlHeader)) /// se puede eliminar esta linea?
                    {
                        foreach (RBuildSourceFile file in module.SourceFiles)
                        {
                            SourceFile sourceFile = new SourceFile(file, module, SysGen);

                            if (file.IsWidl)
                            {
                                if (module.Type == ModuleType.RpcClient)
                                    dependencies += " " + sourceFile.SourceRpcClientHeaderFile.IntermediateFullPath;

                                if (module.Type == ModuleType.RpcServer)
                                    dependencies += " " + sourceFile.SourceRpcServerHeaderFile.IntermediateFullPath;

                                //dependencies += " " + sourceFile.SourceCodeHeaderFile.IntermediateFullPath;
                            }
                        }
                    }
                }

                return dependencies;
            }
        }

        public virtual string DefinitionDependencies
        {
            get
            {
                string dependencies = string.Empty;

                foreach (RBuildSourceFile file in Module.SourceFiles)
                {
                    SourceFile sourceFile = new SourceFile(file, Module, SysGen);

                    if (file.IsWineBuild)
                    {
                        dependencies += " " + CompilableModule.Definition.OriginalFullPath;
                        dependencies += " " + sourceFile.SourceCodeActualFile.IntermediateFullPath;
                    }
                    else if (file.IsWidl)
                    {
                        if (Module.Type == ModuleType.RpcClient ||
                            Module.Type == ModuleType.RpcServer)
                        {
                            dependencies += " " + sourceFile.SourceCodeActualFile.IntermediateFullPath;
                        }
                    }
                }

                return dependencies;
            }   
        }

        public string Linker
        {
            get
            {
                if (Module.CPlusPlus)
                    return CPPCompiler;

                return CCompiler;
            }
        }

        public string PCHCompiler
        {
            get
            {
                if (Module.CPlusPlus)
                    return CPPCompiler;

                return CCompiler;
            }
        }

        public string CCompiler
        {
            get { return (Module.Host ? "$(host_gcc)" : "$(gcc)"); }
        }

        public string CPPCompiler
        {
            get { return (Module.Host ? "$(host_gpp)" : "$(gpp)"); }
        }

        public string ArchiveCompiler
        {
            get { return (Module.Host ? "$(host_ar)" : "$(ar)"); }
        }

        protected virtual void WriteAr()
        {
            Makefile.WriteLine(m_CompModule.Target.IntermediateFullPath + ": " + Module.MakeFileObjsMacro + " | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_AR)");

            if (Module.Type == ModuleType.StaticLibrary ||
                Module.Type == ModuleType.HostStaticLibrary)
            {
                if (Module.ImportLibrary != null)
                {
                    Makefile.WriteLine("\t${dlltool} --dllname " + Module.ImportLibrary.DllName + " --def " + CompilableModule.Definition.OriginalFullPath + " --output-lib $@ " + MangledSymbols + " " + UnderscoreSymbols);
                }
            }

            Makefile.WriteLine("\t${ar} -rc $@ " + Module.MakeFileObjsMacro);
            Makefile.WriteLine();
        }

        protected virtual void WriteWindResCompiler(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " $(wrc_TARGET) " + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath);
            Makefile.WriteLine("\t$(ECHO_WRC)");
            Makefile.WriteLine("\t" + CCompiler + " -xc -E -DRC_INVOKED " + Module.MakeFileRCFlagsMacro + " " + sourceFile.SourceCodeFile.OriginalFullPath + " > " + CompilableModule.RcTempFileNameFullPath);
            Makefile.WriteLine("\t$(Q)$(wrc_TARGET) " + Module.MakeFileRCFlagsMacro + " " + CompilableModule.RcTempFileNameFullPath + " " + CompilableModule.ResTempFileNameFullPath);
            Makefile.WriteLine("\t-@${rm} " + CompilableModule.RcTempFileNameFullPath + " 2>$(NUL)");
            Makefile.WriteLine("\t${windres} " + CompilableModule.ResTempFileNameFullPath + " -o $@");
            Makefile.WriteLine("\t-@${rm} " + CompilableModule.ResTempFileNameFullPath + " 2>$(NUL)");
            Makefile.WriteLine();
        }

        protected virtual void WriteWIDLTypeLibrary(SourceFile file)
        {
            Makefile.WriteLine(file.Target.IntermediateFullPath + ": " + file.SourceCodeFile.OriginalFullPath + " $(widl_TARGET) " + " | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_WIDL)");
            Makefile.WriteLine("\t$(Q)$(widl_TARGET) " + Module.MakeFileWIDLFlagsMacro + " -t -T " + file.Target.IntermediateFullPath + " " + file.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();
        }

        protected virtual void WriteWIDLHeader(SourceFile file)
        {
            Makefile.WriteLine(file.SourceCodeObjectFile.IntermediateFullPath + ": " + file.SourceCodeFile.OriginalFullPath + " $(widl_TARGET) " + " | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_WIDL)");
            Makefile.WriteLine("\t$(Q)$(widl_TARGET) " + Module.MakeFileWIDLFlagsMacro + " -h -H " + file.SourceCodeObjectFile.IntermediateFullPath + " " + file.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();
        }

        protected virtual void WriteWIDLRpcHeader(SourceFile file)
        {
            Makefile.WriteLine(file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceCodeHeaderFile.IntermediateFullPath + ": " + file.SourceCodeFile.OriginalFullPath + " $(widl_TARGET) | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_WIDL)");

            if (Module.Type == ModuleType.RpcServer)
            {
                Makefile.WriteLine("\t$(Q)$(widl_TARGET) " + file.File.Switches + " " + Module.MakeFileWIDLFlagsMacro + " -h -H " + file.SourceCodeHeaderFile.IntermediateFullPath + " -s -S " + file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceCodeFile.OriginalFullPath);
                Makefile.WriteLine();
            }
            else if (Module.Type == ModuleType.RpcClient)
            {
                Makefile.WriteLine("\t$(Q)$(widl_TARGET) " + file.File.Switches + " " + Module.MakeFileWIDLFlagsMacro + " -h -H " + file.SourceCodeHeaderFile.IntermediateFullPath + " -c -C " + file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceCodeFile.OriginalFullPath);
                Makefile.WriteLine();
            }
            else if (Module.Type == ModuleType.RpcProxy)
            {
                Makefile.WriteLine("\t$(Q)$(widl_TARGET) " + file.File.Switches + " " + Module.MakeFileWIDLFlagsMacro + " -h -H " + file.SourceCodeHeaderFile.IntermediateFullPath + " -p -P " + file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceCodeFile.OriginalFullPath);
                Makefile.WriteLine();
            }

            if (Module.Type == ModuleType.RpcServer ||
                Module.Type == ModuleType.RpcClient)
            {
                Makefile.WriteLine(file.SourceCodeObjectFile.IntermediateFullPath + ": " + file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceRpcServerHeaderFile.IntermediateFullPath + " " + file.SourceRpcClientHeaderFile.IntermediateFullPath + " | " + ModuleFolder.IntermediateFullPath);
                Makefile.WriteLine("\t$(ECHO_CC)");
                Makefile.WriteLine("\t" + CCompiler + " -c $< -o $@ " + Module.MakeFileCFlagsMacro);
                Makefile.WriteLine();
            }
            else if (Module.Type == ModuleType.RpcProxy)
            {
                Makefile.WriteLine(file.SourceCodeObjectFile.IntermediateFullPath + ": " + file.SourceCodeActualFile.IntermediateFullPath + " " + file.SourceCodeHeaderFile.IntermediateFullPath + " | " + ModuleFolder.IntermediateFullPath);
                Makefile.WriteLine("\t$(ECHO_CC)");
                Makefile.WriteLine("\t" + CCompiler + " -c $< -o $@ " + Module.MakeFileCFlagsMacro);
                Makefile.WriteLine();
            }
        }

        protected virtual void WriteWineBuild(SourceFile sourceFile)
        {
            Makefile.WriteLine(CompilableModule.Definition.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " $(winebuild_TARGET) | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_WINEBLD)");
            Makefile.WriteLine("\t$(Q)$(winebuild_TARGET) $(WINEBUILD_FLAGS) -o " + CompilableModule.Definition.IntermediateFullPath + " --def -E " + sourceFile.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();

            Makefile.WriteLine(sourceFile.SourceCodeActualFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " $(winebuild_TARGET)");
            Makefile.WriteLine("\t$(ECHO_WINEBLD)");
            Makefile.WriteLine("\t$(Q)$(winebuild_TARGET) $(WINEBUILD_FLAGS) -o " + sourceFile.SourceCodeActualFile.IntermediateFullPath + " --pedll " + sourceFile.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();

            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeActualFile.IntermediateFullPath + " | " + ModuleFolder.IntermediateFullPath);
            Makefile.WriteLine("\t$(ECHO_CC)");
            Makefile.WriteLine("\t" + CCompiler + " -c $< -o $@ " + Module.MakeFileCFlagsMacro);
            Makefile.WriteLine();
        }

        protected virtual void WritePCH(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " " + RpcHeaderDependencies + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath);
            Makefile.WriteLine("\t$(ECHO_PCH)");
            Makefile.WriteLine("\t" + PCHCompiler + " -o " + sourceFile.SourceCodeObjectFile.IntermediateFullPath + " " + Module.MakeFileCFlagsMacro + " -g " + sourceFile.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();
        }

        protected virtual void WriteCCompiler(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " " + Module.MakeFileHeadersMacro + " " + PrecompiledHeader + " " + RpcHeaderDependencies + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath);
            Makefile.WriteLine("\t$(ECHO_CC)");
            Makefile.WriteLine("\t" + CCompiler + " -c $< -o $@ " + Module.MakeFileCFlagsMacro);
            Makefile.WriteLine();
        }

        protected virtual void WriteWMC(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.MessageTableHeaderFile.IntermediateFullPath + " " + sourceFile.MessageTableResourceFile.IntermediateFullPath + ": " + "$(wmc_TARGET)" + " " + sourceFile.SourceCodeFile.OriginalFullPath + " | " + sourceFile.MessageTableHeaderFile.IntermediateFolderFullPath + " " + sourceFile.MessageTableResourceFile.IntermediateFolderFullPath);
            Makefile.WriteLine("\t$(ECHO_WMC)");
            Makefile.WriteLine("\t$(Q)$(wmc_TARGET) -i -H " + sourceFile.MessageTableHeaderFile.IntermediateFullPath + " -o " + sourceFile.MessageTableResourceFile.IntermediateFullPath + " " + sourceFile.SourceCodeFile.OriginalFullPath);
            Makefile.WriteLine();
        }

        protected virtual void WriteCPPCompiler(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " " + Module.MakeFileHeadersMacro + " " + PrecompiledHeader + " " + RpcHeaderDependencies + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath);
            Makefile.WriteLine("\t$(ECHO_CC)");
            Makefile.WriteLine("\t" + CPPCompiler + " -c $< -o $@ " + Module.MakeFileCFlagsMacro);
            Makefile.WriteLine();
        }

        protected virtual void WriteNASMCompiler(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath /*+ ModuleFolder.IntermediateFullPath*/);
            Makefile.WriteLine("\t$(ECHO_NASM)");
            Makefile.WriteLine("\t$(Q)${nasm} -f win32 $< -o $@ " + Module.MakeFileNASMMacro);
            Makefile.WriteLine();
        }

        protected virtual void WriteASMCompiler(SourceFile sourceFile)
        {
            Makefile.WriteLine(sourceFile.SourceCodeObjectFile.IntermediateFullPath + ": " + sourceFile.SourceCodeFile.OriginalFullPath + " | " + sourceFile.SourceCodeFile.IntermediateFolderFullPath /*+ ModuleFolder.IntermediateFullPath*/);
            Makefile.WriteLine("\t$(ECHO_GAS)");
            Makefile.WriteLine("\t" + CCompiler + " -x assembler-with-cpp -c $< -o $@ -D__ASM__  " + Module.MakeFileCFlagsMacro);
            Makefile.WriteLine();
        }

        protected override void WriteRsym()
        {
            Makefile.WriteLine("\t$(ECHO_RSYM)");
            Makefile.WriteLine("\t$(Q)$(RSYM_TARGET) $@ $@");
            Makefile.WriteLine();
        }

        protected override void WriteStrip()
        {
            if (SysGen.Project.Properties["ROS_LEAN_AND_MEAN"] != null)
            {
                //No s'ha provat
                Makefile.WriteLine("\t$(ECHO_STRIP)");
                Makefile.WriteLine("\t${strip} -s -x -X $@");
                Makefile.WriteLine();
            }
        }

        protected override void WriteNonSymbolStripped()
        {
            if (SysGen.Project.Properties["ROS_BUILDNOSTRIP"] != null)
            {
                //No s'ha provat
                Makefile.WriteLine("\t$(ECHO_CP)");
                Makefile.WriteLine("\t$(cp) " + m_CompModule.TargetNoStrip.OutputFullPath + " 1>$(NUL)");
                Makefile.WriteLine();
            }
        }

        protected override void WriteLinker()
        {
            //Makefile.WriteLine(Module.MakeFileTargetMacro + ": " + CompilableModule.Definition.OriginalFullPath + " " + Module.MakeFileLinkDepsMacro + " " + Module.MakeFileObjsMacro + "  $(RSYM_TARGET) $(PEFIXUP_TARGET) | " + /*ModuleFolder.TemporaryFullPath*/  ModuleFolder.OutputFullPath);
            Makefile.WriteLine(Module.MakeFileTargetMacro + ": " + Definition + " " + Module.MakeFileLinkDepsMacro + " " + Module.MakeFileObjsMacro + "  $(RSYM_TARGET) $(PEFIXUP_TARGET) | " + ModuleFolder.OutputFullPath);
            Makefile.WriteLine("\t$(ECHO_LD)");
            
            if (Module.IsDLL)
            {
                Makefile.WriteLine("\t${dlltool} --dllname " + Module.TargetName + " --def " + CompilableModule.Definition.OriginalFullPath + " --output-exp " + m_CompModule.ExpTempFileNameFullPath + " " + MangledSymbols + " " + UnderscoreSymbols);
            }

            Makefile.WriteLine("\t" + Linker + " " + LinkerParameters);

            if (Module.IsDLL)
            {
                Makefile.WriteLine("\t$(Q)$(PEFIXUP_TARGET) " + Module.MakeFileTargetMacro + " -exports" + " " + PefixupParameters);
                Makefile.WriteLine("\t-@${rm} " + m_CompModule.ExpTempFileNameFullPath + " 2>$(NUL)");
            }

            Makefile.WriteLine();

            WriteRsym();
            WriteStrip();
            WriteNonSymbolStripped();
        }

        public string Definition
        {
            get
            {
                if (Module.ImportLibrary != null)
                    return CompilableModule.Definition.OriginalFullPath;

                return string.Empty;
            }
        }

        public virtual string PefixupParameters
        {
            get
            {
                if ((Module.Type == ModuleType.Kernel) ||
                    (Module.Type == ModuleType.KernelModeDLL) ||
                    (Module.Type == ModuleType.KernelModeDriver) ||
                    (Module.Type == ModuleType.KeyboardLayout))
                {
                    return "-sections";
                }

                return string.Empty;
            }
        }

        public string LinkerScript
        {
            get
            {
                if (Module.LinkerScript != null)
                    return string.Format("-Wl,-T,{0}", Module.LinkerScript.FullPath);

                return string.Empty;
            }
        }

        public string MangledSymbols
        {
            get
            {
                if (Module.MangledSymbols)
                    return string.Empty;

                return "--kill-at";
            }
        }

        public string UnderscoreSymbols
        {
            get
            {
                if (Module.UnderscoreSymbols)
                    return "--add-underscore";

                return string.Empty;
            }
        }


        protected string PrecompiledHeader
        {
            get
            {
                if (Module.PreCompiledHeader != null)
                    return Module.MakeFilePCHMacro;

                return string.Empty;
            }
        }

        protected override void WriteCleanTarget()
        {
            Makefile.WritePhonyTarget(Module.MakeFileCleanTarget);
            Makefile.WriteSingleLineTarget(Module.MakeFileCleanTarget);

            if (Module.Type != ModuleType.Cabinet) //Hack:
            {
                foreach (RBuildSourceFile file in Module.SourceFiles)
                {
                    SourceFile cFile = new SourceFile(file, Module, SysGen);

                    Makefile.WriteLine("\t-@$(rm) " + cFile.SourceCodeObjectFile.IntermediateFullPath + " 2>$(NUL)");
                }
            }

            Makefile.WriteLine("\t-@$(rm) " + Module.MakeFileTargetMacro + " 2>$(NUL)");
            Makefile.WriteLine();   
        }

        protected override void WriteImportLibrary ()
        {
            if (Module.HasImportLibrary)
            {
                Makefile.WriteComment("IMPORT LIBRARY RULE");
                Makefile.WriteLine(ResolveRBuildFilePath(Module.Dependency) + ": " + CompilableModule.Definition.OriginalFullPath + " " + DefinitionDependencies + " | " + ModuleFolder.IntermediateFullPath);
                Makefile.WriteLine("\t$(ECHO_DLLTOOL)");
                Makefile.WriteLine("\t$(dlltool) --dllname " + Module.TargetName + " --def " + CompilableModule.Definition.OriginalFullPath + " --output-lib " + /*CompilableModule.Dependency.IntermediateFullPath*/ ResolveRBuildFilePath(Module.Dependency) + " " + MangledSymbols + " " + UnderscoreSymbols);
                Makefile.WriteLine();
            }
        }

        protected virtual string LinkerParameters
        {
            get
            {
                return string.Format("-Wl,--subsystem," + SubSystem + " -Wl,--entry,{0} -Wl,--image-base,{1} " + AdditionalParameters2 + " -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 " + " " + NoStartFiles + " " + Shared + " " + LinkerScript + " " + AdditionalParamters + " -o {2} {3} {4} {5}",
                                                     Module.LinkerEntryPoint,
                                                     Module.BaseAddress,
                                                     Module.MakeFileTargetMacro,
                                                     Module.MakeFileObjsMacro,
                                                     Module.MakeFileLibsMacro,
                                                     Module.MakeFileLFlagsMacro);
            }
        }

        protected virtual string AdditionalParameters2
        {
            get { return string.Empty; }
        }

        //Fixme:
        protected virtual LinkerSubSystem LinkerSubsystem
        {
            get { return LinkerSubSystem.Console; }
        }

        protected virtual string SubSystem
        {
            get { return "console"; }
        }

        protected virtual string NoStartFiles
        {
            get
            {
                if (Module.Type == ModuleType.NativeCUI ||
                    Module.Type == ModuleType.NativeDLL ||
                    Module.Type == ModuleType.Kernel ||
                    Module.Type == ModuleType.KernelModeDLL ||
                    Module.Type == ModuleType.KernelModeDriver ||
                    Module.Type == ModuleType.KeyboardLayout)
                {
                    return "-nostartfiles";
                }

                return string.Empty;
            }
        }

        protected virtual string Shared
        {
            get
            {
                if (Module.IsDLL)
                    return "-shared";

                return string.Empty;
            }
        }

        protected virtual string AdditionalParamters
        {
            get 
            {
                if (Module.IsDLL)
                    return m_CompModule.ExpTempFileNameFullPath;

                return string.Empty;
            }
        }

        public RBuildModule Module
        {
            get { return m_BuildElement as RBuildModule; }
        }

        public RBuildProject Project
        {
            get { return m_Project; }
            set { m_Project = value; }
        }

        public BackendBuildModule CompilableModule
        {
            get { return m_CompModule; }
        }

        public BackedBuildFolder ModuleFolder
        {
            get { return m_Folder; }
        }
    }
}