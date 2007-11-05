/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"
#include "mingw.h"

class MingwBackend;

extern std::string
GetTargetMacro ( const Module&, bool with_dollar = true );

extern std::string
PrefixFilename (
	const std::string& filename,
	const std::string& prefix );

class MingwModuleHandler
{
public:
	MingwModuleHandler ( const Module& module_ );
	virtual ~MingwModuleHandler();

	static void SetBackend ( MingwBackend* backend_ );
	static void SetMakefile ( FILE* f );
	void EnablePreCompiledHeaderSupport ();

	static const FileLocation* PassThruCacheDirectory (const FileLocation* fileLocation );

	static const FileLocation* GetTargetFilename (
		const Module& module,
		string_list* pclean_files );

	static const FileLocation* GetImportLibraryFilename (
		const Module& module,
		string_list* pclean_files );

	static std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines, std::set<std::string> &used_defs );
	static std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes );

	std::string GetModuleTargets ( const Module& module );
	void GetObjectsVector ( const IfableData& data,
	                        std::vector<FileLocation>& objectFiles ) const;
	void GenerateSourceMacro();
	void GenerateObjectMacro();
	void GenerateTargetMacro();
	void GenerateOtherMacros();

	static MingwModuleHandler* InstanciateHandler ( const Module& module_,
	                                                MingwBackend* backend_ );
	virtual HostType DefaultHost() = 0;
	void GeneratePreconditionDependencies ();
	virtual void Process () = 0;
	virtual std::string TypeSpecificCFlags() { return ""; }
	virtual std::string TypeSpecificNasmFlags() { return ""; }
	virtual std::string TypeSpecificLinkerFlags() { return ""; }
	void GenerateInvocations () const;
	void GenerateCleanTarget () const;
	void GenerateInstallTarget () const;
	void GenerateDependsTarget () const;
	static bool ReferenceObjects ( const Module& module );
	virtual void AddImplicitLibraries ( Module& module ) { return; }

	void OutputCopyCommand ( const FileLocation& source,
	                         const FileLocation& destination );
protected:
	virtual void GetModuleSpecificCompilationUnits ( std::vector<CompilationUnit*>& compilationUnits );
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	const FileLocation* GetActualSourceFilename ( const FileLocation* file ) const;
	std::string GetExtraDependencies ( const FileLocation *file ) const;
	std::string GetCompilationUnitDependencies ( const CompilationUnit& compilationUnit ) const;
	const FileLocation* GetModuleArchiveFilename () const;
	bool IsGeneratedFile ( const File& file ) const;
	std::string GetImportLibraryDependency ( const Module& importedModule );
	void GetTargets ( const Module& dependencyModule,
	                  string_list& targets );
	void GetModuleDependencies ( string_list& dependencies );
	std::string GetAllDependencies () const;
	void GetSourceFilenames ( std::vector<FileLocation>& list,
	                          bool includeGeneratedFiles ) const;
	void GetSourceFilenamesWithoutGeneratedFiles ( std::vector<FileLocation>& list ) const;
	const FileLocation* GetObjectFilename ( const FileLocation* sourceFile,
	                                        const Module& module,
	                                        string_list* pclean_files ) const;

	std::string GetObjectFilenames ();

	std::string GetPreconditionDependenciesName () const;
	std::string GetCFlagsMacro () const;
	static std::string GetObjectsMacro ( const Module& );
	std::string GetLinkingDependenciesMacro () const;
	std::string GetLibsMacro () const;
	std::string GetLinkerMacro () const;
	void GenerateCleanObjectsAsYouGoCode () const;
	void GenerateRunRsymCode () const;
	void GenerateRunStripCode () const;
	void GenerateLinkerCommand ( const std::string& dependencies,
	                             const std::string& linker,
	                             const std::string& linkerParameters,
	                             const std::string& objectsMacro,
	                             const std::string& libsMacro,
	                             const std::string& pefixupParameters );
	void GeneratePhonyTarget() const;
	void GenerateBuildMapCode ( const FileLocation *mapTarget = NULL );
	void GenerateRules ();
	void GenerateImportLibraryTargetIfNeeded ();
	void GetDefinitionDependencies ( std::vector<FileLocation>& dependencies ) const;

	std::string GetLinkingDependencies () const;
	static MingwBackend* backend;
	static FILE* fMakefile;
	bool use_pch;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParameters () const;
	std::string GenerateCompilerParametersFromVector ( const std::vector<CompilerFlag*>& compilerFlags ) const;
	std::string GenerateLinkerParametersFromVector ( const std::vector<LinkerFlag*>& linkerFlags ) const;
	std::string GenerateImportLibraryDependenciesFromVector ( const std::vector<Library*>& libraries );
	std::string GenerateLinkerParameters () const;
	void GenerateMacro ( const char* assignmentOperation,
	                     const std::string& macro,
	                     const IfableData& data,
	                     std::set<const Define *>* used_defs,
	                     bool generatingCompilerMacro );
	void GenerateMacros ( const char* op,
	                      const IfableData& data,
	                      const std::vector<LinkerFlag*>* linkerFlags,
	                      std::set<const Define *>& used_defs );
	void GenerateSourceMacros ( const char* assignmentOperation,
	                            const IfableData& data );
	void GenerateObjectMacros ( const char* assignmentOperation,
	                            const IfableData& data );
	std::string GenerateGccIncludeParameters () const;
	std::string GenerateGccParameters () const;
	std::string GenerateNasmParameters () const;
	const FileLocation* GetPrecompiledHeaderFilename () const;
	void GenerateGccCommand ( const FileLocation* sourceFile,
	                          const std::string& extraDependencies,
	                          const std::string& cc,
	                          const std::string& cflagsMacro );
	void GenerateGccAssemblerCommand ( const FileLocation* sourceFile,
	                                   const std::string& cc,
	                                   const std::string& cflagsMacro );
	void GenerateNasmCommand ( const FileLocation* sourceFile,
	                           const std::string& nasmflagsMacro );
	void GenerateWindresCommand ( const FileLocation* sourceFile,
	                              const std::string& windresflagsMacro );
	void GenerateWinebuildCommands ( const FileLocation* sourceFile );
	std::string GetWidlFlags ( const CompilationUnit& compilationUnit );
	void GenerateWidlCommandsServer (
		const CompilationUnit& compilationUnit,
		const std::string& widlflagsMacro );
	void GenerateWidlCommandsClient (
		const CompilationUnit& compilationUnit,
		const std::string& widlflagsMacro );
	void GenerateWidlCommandsIdlHeader (
		const CompilationUnit& compilationUnit,
		const std::string& widlflagsMacro );
	void GenerateWidlCommandsEmbeddedTypeLib (
		const CompilationUnit& compilationUnit,
		const std::string& widlflagsMacro );
	void GenerateWidlCommands ( const CompilationUnit& compilationUnit,
	                            const std::string& widlflagsMacro );
	void GenerateCommands ( const CompilationUnit& compilationUnit,
	                        const std::string& extraDependencies,
	                        const std::string& cc,
	                        const std::string& cppc,
	                        const std::string& cflagsMacro,
	                        const std::string& nasmflagsMacro,
	                        const std::string& windresflagsMacro,
	                        const std::string& widlflagsMacro );
	void GenerateObjectFileTargets ( const IfableData& data,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 const std::string& widlflagsMacro );
	void GenerateObjectFileTargets ( const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 const std::string& widlflagsMacro );
	const FileLocation* GenerateArchiveTarget ( const std::string& ar,
	                                            const std::string& objs_macro ) const;
	void GetSpecObjectDependencies ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetWidlObjectDependencies ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetDefaultDependencies ( string_list& dependencies ) const;
	void GetInvocationDependencies ( const Module& module, string_list& dependencies );
	bool IsWineModule () const;
	const FileLocation* GetDefinitionFilename () const;
	void GenerateBuildNonSymbolStrippedCode ();
	void CleanupCompilationUnitVector ( std::vector<CompilationUnit*>& compilationUnits );
	void GetRpcHeaderDependencies ( std::vector<FileLocation>& dependencies ) const;
	static std::string GetPropertyValue ( const Module& module, const std::string& name );
	const FileLocation* GetRpcServerHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetRpcClientHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetIdlHeaderFilename ( const FileLocation *base ) const;
	std::string GetModuleCleanTarget ( const Module& module ) const;
	void GetReferencedObjectLibraryModuleCleanTargets ( std::vector<std::string>& moduleNames ) const;
public:
	const Module& module;
	string_list clean_files;
	std::string cflagsMacro;
	std::string nasmflagsMacro;
	std::string windresflagsMacro;
	std::string widlflagsMacro;
	std::string linkerflagsMacro;
	std::string sourcesMacro;
	std::string objectsMacro;
	std::string libsMacro;
	std::string linkDepsMacro;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostTrue; }
	virtual void Process ();
private:
	void GenerateBuildToolModuleTarget ();
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
private:
	void GenerateKernelModuleTarget ();
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
private:
	void GenerateStaticLibraryModuleTarget ();
};


class MingwObjectLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwObjectLibraryModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
private:
	void GenerateObjectLibraryModuleTarget ();
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateKernelModeDLLModuleTarget ();
};


class MingwKernelModeDriverModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDriverModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificCFlags() { return "-D__NTDRIVER__"; }
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateKernelModeDriverModuleTarget ();
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateNativeDLLModuleTarget ();
};


class MingwNativeCUIModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeCUIModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificCFlags() { return "-D__NTAPP__"; }
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateNativeCUIModuleTarget ();
};


class MingwWin32DLLModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32DLLModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return module.cplusplus ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32DLLModuleTarget ();
};


class MingwWin32OCXModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32OCXModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return module.cplusplus ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32OCXModuleTarget ();
};


class MingwWin32CUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32CUIModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return module.cplusplus ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32CUIModuleTarget ();
};


class MingwWin32GUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32GUIModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return module.cplusplus ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32GUIModuleTarget ();
};


class MingwBootLoaderModuleHandler : public MingwModuleHandler
{
public:
	MingwBootLoaderModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
private:
	void GenerateBootLoaderModuleTarget ();
};


class MingwBootSectorModuleHandler : public MingwModuleHandler
{
public:
	MingwBootSectorModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificNasmFlags() { return "-f bin"; }
private:
	void GenerateBootSectorModuleTarget ();
};


class MingwBootProgramModuleHandler : public MingwModuleHandler
{
public:
	MingwBootProgramModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string GetProgTextAddrMacro ();
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
private:
	void GenerateBootProgramModuleTarget ();
};


class MingwIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwIsoModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
private:
	void GenerateIsoModuleTarget ();
	void GetBootstrapCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetNonModuleCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetBootstrapCdFiles ( std::vector<FileLocation>& out ) const;
	void GetNonModuleCdFiles ( std::vector<FileLocation>& out ) const;
	void GetCdFiles ( std::vector<FileLocation>& out ) const;
	void OutputBootstrapfileCopyCommands ( const std::string& bootcdDirectory );
	void OutputCdfileCopyCommands ( const std::string& bootcdDirectory );
};


class MingwLiveIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwLiveIsoModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
private:
	void GenerateLiveIsoModuleTarget ();
	void CreateDirectory ( const std::string& directory );
	void OutputModuleCopyCommands ( std::string& livecdDirectory,
	                                std::string& livecdReactos );
	void OutputNonModuleCopyCommands ( std::string& livecdDirectory,
	                                   std::string& livecdReactos );
	void OutputProfilesDirectoryCommands ( std::string& livecdDirectory );
	void OutputLoaderCommands ( std::string& livecdDirectory );
	void OutputRegistryCommands ( std::string& livecdDirectory );
};


class MingwTestModuleHandler : public MingwModuleHandler
{
public:
	MingwTestModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
	std::string TypeSpecificLinkerFlags() { return "-nostartfiles -nostdlib"; }
protected:
	virtual void GetModuleSpecificCompilationUnits ( std::vector<CompilationUnit*>& compilationUnits );
private:
	void GenerateTestModuleTarget ();
};


class MingwRpcServerModuleHandler : public MingwModuleHandler
{
public:
	MingwRpcServerModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};


class MingwRpcClientModuleHandler : public MingwModuleHandler
{
public:
	MingwRpcClientModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};

class MingwAliasModuleHandler : public MingwModuleHandler
{
public:
	MingwAliasModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};

class MingwIdlHeaderModuleHandler : public MingwModuleHandler
{
public:
	MingwIdlHeaderModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};

class MingwEmbeddedTypeLibModuleHandler : public MingwModuleHandler
{
public:
	MingwEmbeddedTypeLibModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};

class MingwElfExecutableModuleHandler : public MingwModuleHandler
{
public:
	MingwElfExecutableModuleHandler ( const Module& module );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ();
};

#endif /* MINGW_MODULEHANDLER_H */
