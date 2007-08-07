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

	static std::string PassThruCacheDirectory (
		const std::string &f,
		Directory* directoryTree );

	static std::string PassThruCacheDirectory (const FileLocation* fileLocation );

	static Directory* GetTargetDirectoryTree (
		const Module& module );

	static std::string GetTargetFilename (
		const Module& module,
		string_list* pclean_files );

	static std::string
	GetImportLibraryFilename (
		const Module& module,
		string_list* pclean_files );

	static std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines );
	static std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes );

	std::string GetModuleTargets ( const Module& module );
	void GetObjectsVector ( const IfableData& data,
	                        std::vector<std::string>& objectFiles ) const;
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
protected:
	virtual void GetModuleSpecificCompilationUnits ( std::vector<CompilationUnit*>& compilationUnits );
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	FileLocation* GetActualSourceFilename ( const FileLocation* fileLocation ) const;
	std::string GetExtraDependencies ( const std::string& filename ) const;
	std::string GetCompilationUnitDependencies ( const CompilationUnit& compilationUnit ) const;
	std::string GetModuleArchiveFilename () const;
	bool IsGeneratedFile ( const File& file ) const;
	std::string GetImportLibraryDependency ( const Module& importedModule );
	void GetTargets ( const Module& dependencyModule,
	                  string_list& targets );
	void GetModuleDependencies ( string_list& dependencies );
	std::string GetAllDependencies () const;
	void GetSourceFilenames ( string_list& list,
	                          bool includeGeneratedFiles ) const;
	void GetSourceFilenamesWithoutGeneratedFiles ( string_list& list ) const;
	std::string GetObjectFilename ( const FileLocation* sourceFileLocation,
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
	void GenerateBuildMapCode ( const char *mapTarget = NULL );
	void GenerateRules ();
	void GenerateImportLibraryTargetIfNeeded ();
	void GetDefinitionDependencies ( string_list& dependencies ) const;
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
	                     const IfableData& data );
	void GenerateMacros ( const char* op,
	                      const IfableData& data,
	                      const std::vector<LinkerFlag*>* linkerFlags );
	void GenerateObjectMacros ( const char* assignmentOperation,
	                            const IfableData& data,
	                            const std::vector<LinkerFlag*>* linkerFlags );
	std::string GenerateGccIncludeParameters () const;
	std::string GenerateGccParameters () const;
	std::string GenerateNasmParameters () const;
	std::string GetPrecompiledHeaderFilename () const;
	void GenerateGccCommand ( const FileLocation* sourceFileLocation,
	                          const std::string& extraDependencies,
	                          const std::string& cc,
	                          const std::string& cflagsMacro );
	void GenerateGccAssemblerCommand ( const FileLocation* sourceFileLocation,
	                                   const std::string& cc,
	                                   const std::string& cflagsMacro );
	void GenerateNasmCommand ( const FileLocation* sourceFileLocation,
	                           const std::string& nasmflagsMacro );
	void GenerateWindresCommand ( const FileLocation* sourceFileLocation,
	                              const std::string& windresflagsMacro );
	void GenerateWinebuildCommands ( const FileLocation* sourceFileLocation );
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
	std::string GenerateArchiveTarget ( const std::string& ar,
	                                    const std::string& objs_macro ) const;
	void GetSpecObjectDependencies ( string_list& dependencies,
	                                 const std::string& filename ) const;
	void GetWidlObjectDependencies ( string_list& dependencies,
	                                 const std::string& filename ) const;
	void GetDefaultDependencies ( string_list& dependencies ) const;
	void GetInvocationDependencies ( const Module& module, string_list& dependencies );
	bool IsWineModule () const;
	std::string GetDefinitionFilename () const;
	static std::string RemoveVariables ( std::string path);
	void GenerateBuildNonSymbolStrippedCode ();
	void CleanupCompilationUnitVector ( std::vector<CompilationUnit*>& compilationUnits );
	void GetRpcHeaderDependencies ( std::vector<std::string>& dependencies ) const;
	std::string GetRpcServerHeaderFilename ( std::string basename ) const;
	std::string GetRpcClientHeaderFilename ( std::string basename ) const;
	std::string GetIdlHeaderFilename ( std::string basename ) const;
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
	std::string TypeSpecificLinkerFlags() { return module.useHostStdlib ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
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
	std::string TypeSpecificLinkerFlags() { return module.useHostStdlib ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
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
	std::string TypeSpecificLinkerFlags() { return module.useHostStdlib ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
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
	std::string TypeSpecificLinkerFlags() { return module.useHostStdlib ? "-nostartfiles -lgcc" : "-nostartfiles -nostdlib -lgcc"; }
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
	std::string GetBootstrapCdDirectories ( const std::string& bootcdDirectory );
	std::string GetNonModuleCdDirectories ( const std::string& bootcdDirectory );
	std::string GetCdDirectories ( const std::string& bootcdDirectory );
	void GetBootstrapCdFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleCdFiles ( std::vector<std::string>& out ) const;
	void GetCdFiles ( std::vector<std::string>& out ) const;
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
	void OutputCopyCommand ( const std::string& sourceFilename,
	                         const std::string& targetFilename,
	                         const std::string& targetDirectory );
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

#endif /* MINGW_MODULEHANDLER_H */
