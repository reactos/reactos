#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"
#include "mingw.h"

class MingwBackend;
	
extern std::string
ReplaceExtension ( const std::string& filename,
                   const std::string& newExtension );

extern std::string
PrefixFilename (
	const std::string& filename,
	const std::string& prefix );

class MingwModuleHandler
{
public:
	typedef std::vector<std::string> string_list;
	MingwModuleHandler ( ModuleType moduletype,
	                     MingwBackend* backend_ );
	virtual ~MingwModuleHandler();

	static void SetMakefile ( FILE* f );
	static MingwModuleHandler* InstanciateHandler ( const std::string& location,
	                                                ModuleType moduletype_,
	                                                MingwBackend* backend_ );
	static void SetUsePch ( bool use_pch );
	static MingwModuleHandler* LookupHandler ( const std::string& location,
	                                           ModuleType moduletype_ );
	virtual HostType DefaultHost() = 0;
	virtual void Process ( const Module& module, string_list& clean_files ) = 0;
	void GenerateCleanTarget ( const Module& module,
	                           const string_list& clean_files ) const;
protected:
	const std::string &PassThruCacheDirectory ( const std::string &f );
	const std::string GetDirectoryDependency ( const std::string& file );
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	std::string GetActualSourceFilename ( const std::string& filename ) const;
	std::string GetModuleArchiveFilename ( const Module& module ) const;
	bool IsGeneratedFile ( const File& file ) const;
	std::string GetImportLibraryDependency ( const Module& importedModule );
	std::string GetModuleDependencies ( const Module& module );
	std::string GetAllDependencies ( const Module& module ) const;
	std::string GetSourceFilenames ( const Module& module,
	                                 bool includeGeneratedFiles ) const;
	std::string GetSourceFilenames ( const Module& module ) const;
	std::string GetSourceFilenamesWithoutGeneratedFiles ( const Module& module ) const;

	std::string GetObjectFilenames ( const Module& module );
	void GenerateInvocations ( const Module& module ) const;
	
	std::string GetPreconditionDependenciesName ( const Module& module ) const;
	void GeneratePreconditionDependencies ( const Module& module );
	std::string GetCFlagsMacro ( const Module& module ) const;
	std::string GetObjectsMacro ( const Module& module ) const;
	std::string GetLinkingDependenciesMacro ( const Module& module ) const;
	std::string GetLibsMacro ( const Module& module ) const;
	std::string GetLinkerMacro ( const Module& module ) const;
	void GenerateLinkerCommand ( const Module& module,
	                             const std::string& target,
	                             const std::string& dependencies,
	                             const std::string& linker,
	                             const std::string& linkerParameters,
	                             const std::string& objectsMacro,
	                             const std::string& libsMacro,
	                             string_list& clean_files ) const;
	void GenerateMacrosAndTargets ( const Module& module,
	                                const std::string* clags,
	                                const std::string* nasmflags,
	                                string_list& clean_files );
	void GenerateImportLibraryTargetIfNeeded ( const Module& module, string_list& clean_files );
	std::string GetDefinitionDependencies ( const Module& module );
	std::string GetLinkingDependencies ( const Module& module ) const;
  bool IsCPlusPlusModule ( const Module& module );
  static FILE* fMakefile;
	static bool use_pch;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	std::string GenerateCompilerParametersFromVector ( const std::vector<CompilerFlag*>& compilerFlags ) const;
	std::string GenerateLinkerParametersFromVector ( const std::vector<LinkerFlag*>& linkerFlags ) const;
	std::string GenerateImportLibraryDependenciesFromVector ( const std::vector<Library*>& libraries );
	std::string GenerateLinkerParameters ( const Module& module ) const;
	void GenerateMacro ( const char* assignmentOperation,
	                     const std::string& macro,
	                     const IfableData& data,
	                     const std::vector<CompilerFlag*>* compilerFlags );
	void GenerateMacros (
	                      const Module& module,
	                      const char* op,
	                      const IfableData& data,
	                      const std::vector<CompilerFlag*>* compilerFlags,
	                      const std::vector<LinkerFlag*>* linkerFlags,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& windresflags_macro,
	                      const std::string& linkerflags_macro,
	                      const std::string& objs_macro,
	                      const std::string& libs_macro,
	                      const std::string& linkdeps_macro );
	void GenerateMacros ( const Module& module,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& windresflags_macro,
	                      const std::string& linkerflags_macro,
	                      const std::string& objs_macro,
	                      const std::string& libs_macro,
	                      const std::string& linkDepsMacro );
	std::string GenerateGccIncludeParameters ( const Module& module ) const;
	std::string GenerateGccParameters ( const Module& module ) const;
	std::string GenerateNasmParameters ( const Module& module ) const;
	void GenerateGccCommand ( const Module& module,
	                          const std::string& sourceFilename,
	                          const std::string& cc,
	                          const std::string& cflagsMacro );
	void GenerateGccAssemblerCommand ( const Module& module,
	                                   const std::string& sourceFilename,
	                                   const std::string& cc,
	                                   const std::string& cflagsMacro );
	void GenerateNasmCommand ( const Module& module,
	                           const std::string& sourceFilename,
	                           const std::string& nasmflagsMacro );
	void GenerateWindresCommand ( const Module& module,
	                              const std::string& sourceFilename,
	                              const std::string& windresflagsMacro );
	void GenerateWinebuildCommands ( const Module& module,
	                                 const std::string& sourceFilename,
	                                 string_list& clean_files ) const;
	void GenerateCommands ( const Module& module,
	                        const std::string& sourceFilename,
	                        const std::string& cc,
	                        const std::string& cppc,
	                        const std::string& cflagsMacro,
	                        const std::string& nasmflagsMacro,
	                        const std::string& windresflagsMacro,
	                        string_list& clean_files );
	void GenerateObjectFileTargets ( const Module& module,
	                                 const IfableData& data,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 string_list& clean_files );
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 string_list& clean_files );
	std::string GenerateArchiveTarget ( const Module& module,
	                                    const std::string& ar,
	                                    const std::string& objs_macro ) const;
	std::string GetSpecObjectDependencies ( const std::string& filename ) const;
	std::string GetDefaultDependencies ( const Module& module ) const;
  std::string GetInvocationDependencies ( const Module& module );
	MingwBackend* backend;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostTrue; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBuildToolModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateStaticLibraryModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwObjectLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwObjectLibraryModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateObjectLibraryModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModeDLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModeDriverModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDriverModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModeDriverModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateNativeDLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwNativeCUIModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeCUIModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateNativeCUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32DLLModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32DLLModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateExtractWineDLLResourcesTarget ( const Module& module, string_list& clean_files );
	void GenerateWin32DLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32CUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32CUIModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateWin32CUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32GUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32GUIModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateWin32GUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwBootLoaderModuleHandler : public MingwModuleHandler
{
public:
	MingwBootLoaderModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBootLoaderModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwBootSectorModuleHandler : public MingwModuleHandler
{
public:
	MingwBootSectorModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBootSectorModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwIsoModuleHandler ( MingwBackend* backend );
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateIsoModuleTarget ( const Module& module, string_list& clean_files );
	std::string GetBootstrapCdDirectories ( const std::string& bootcdDirectory,
	                                        const Module& module );
	std::string GetNonModuleCdDirectories ( const std::string& bootcdDirectory,
	                                        const Module& module );
	std::string GetCdDirectories ( const std::string& bootcdDirectory,
	                               const Module& module );
	void GetBootstrapCdFiles ( std::vector<std::string>& out,
	                           const Module& module ) const;
	void GetNonModuleCdFiles ( std::vector<std::string>& out,
	                           const Module& module ) const;
	void GetCdFiles ( std::vector<std::string>& out,
	                  const Module& module ) const;
	void OutputBootstrapfileCopyCommands ( const std::string& bootcdDirectory,
	                                       const Module& module );
	void OutputCdfileCopyCommands ( const std::string& bootcdDirectory,
	                                const Module& module );
};

#endif /* MINGW_MODULEHANDLER_H */
