#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

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

	static std::map<ModuleType,MingwModuleHandler*>* handler_map;
	static int ref;

	MingwModuleHandler ( ModuleType moduletype );
	virtual ~MingwModuleHandler();

	static void SetMakefile ( FILE* f );
	static void SetUsePch ( bool use_pch );
	static MingwModuleHandler* LookupHandler ( const std::string& location,
	                                           ModuleType moduletype_ );
	virtual HostType DefaultHost() = 0;
	virtual void Process ( const Module& module, string_list& clean_files ) = 0;
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	void GenerateDirectoryTargets () const;
	void GenerateCleanTarget ( const Module& module,
	                           const string_list& clean_files ) const;
protected:
	const std::string &PassThruCacheDirectory ( const std::string &f ) const;
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	std::string GetActualSourceFilename ( const std::string& filename ) const;
	std::string GetModuleArchiveFilename ( const Module& module ) const;
	bool IsGeneratedFile ( const File& file ) const;
	std::string GetImportLibraryDependency ( const Module& importedModule ) const;
	std::string GetModuleDependencies ( const Module& module ) const;
	std::string GetAllDependencies ( const Module& module ) const;
	std::string GetSourceFilenames ( const Module& module,
	                                 bool includeGeneratedFiles ) const;
	std::string GetSourceFilenames ( const Module& module ) const;
	std::string GetSourceFilenamesWithoutGeneratedFiles ( const Module& module ) const;

	std::string GetObjectFilenames ( const Module& module ) const;
	std::string GetInvocationDependencies ( const Module& module ) const;
	void GenerateInvocations ( const Module& module ) const;
	
	std::string GetPreconditionDependenciesName ( const Module& module ) const;
	void GeneratePreconditionDependencies ( const Module& module ) const;
	std::string GetCFlagsMacro ( const Module& module ) const;
	std::string GetObjectsMacro ( const Module& module ) const;
	std::string GetLinkingDependenciesMacro ( const Module& module ) const;
	std::string GetLibsMacro ( const Module& module ) const;
	std::string GetLinkerMacro ( const Module& module ) const;
	void GenerateLinkerCommand ( const Module& module,
	                             const std::string& linker,
	                             const std::string& linkerParameters,
	                             const std::string& objectsMacro,
	                             const std::string& libsMacro,
	                             string_list& clean_files ) const;
	void GenerateMacrosAndTargets ( const Module& module,
	                                const std::string* clags,
	                                const std::string* nasmflags,
	                                string_list& clean_files ) const;
	void GenerateImportLibraryTargetIfNeeded ( const Module& module, string_list& clean_files ) const;
	std::string GetDefinitionDependencies ( const Module& module ) const;
	std::string GetLinkingDependencies ( const Module& module ) const;
	bool IsCPlusPlusModule ( const Module& module ) const;
	static FILE* fMakefile;
	static bool use_pch;
	static std::set<std::string> directory_set;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	std::string GenerateCompilerParametersFromVector ( const std::vector<CompilerFlag*>& compilerFlags ) const;
	std::string GenerateLinkerParametersFromVector ( const std::vector<LinkerFlag*>& linkerFlags ) const;
	std::string GenerateImportLibraryDependenciesFromVector ( const std::vector<Library*>& libraries ) const;
	std::string GenerateLinkerParameters ( const Module& module ) const;
	void GenerateMacro ( const char* assignmentOperation,
	                     const std::string& macro,
	                     const IfableData& data,
	                     const std::vector<CompilerFlag*>* compilerFlags ) const;
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
	                      const std::string& linkdeps_macro ) const;
	void GenerateMacros ( const Module& module,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& windresflags_macro,
	                      const std::string& linkerflags_macro,
	                      const std::string& objs_macro,
	                      const std::string& libs_macro,
	                      const std::string& linkDepsMacro ) const;
	std::string GenerateGccIncludeParameters ( const Module& module ) const;
	std::string GenerateGccParameters ( const Module& module ) const;
	std::string GenerateNasmParameters ( const Module& module ) const;
	void GenerateGccCommand ( const Module& module,
	                          const std::string& sourceFilename,
	                          const std::string& cc,
	                          const std::string& cflagsMacro ) const;
	void GenerateGccAssemblerCommand ( const Module& module,
	                                   const std::string& sourceFilename,
	                                   const std::string& cc,
	                                   const std::string& cflagsMacro ) const;
	void GenerateNasmCommand ( const Module& module,
	                           const std::string& sourceFilename,
	                           const std::string& nasmflagsMacro ) const;
	void GenerateWindresCommand ( const Module& module,
	                              const std::string& sourceFilename,
	                              const std::string& windresflagsMacro ) const;
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
	                        string_list& clean_files ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const IfableData& data,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 string_list& clean_files ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro,
	                                 string_list& clean_files ) const;
	std::string GenerateArchiveTarget ( const Module& module,
	                                    const std::string& ar,
	                                    const std::string& objs_macro ) const;
	std::string GetSpecObjectDependencies ( const std::string& filename ) const;
	std::string GetDefaultDependencies ( const Module& module ) const;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ();
	virtual HostType DefaultHost() { return HostTrue; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBuildToolModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateStaticLibraryModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwObjectLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwObjectLibraryModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateObjectLibraryModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModeDLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwKernelModeDriverModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDriverModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateKernelModeDriverModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateNativeDLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwNativeCUIModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeCUIModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateNativeCUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32DLLModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32DLLModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateExtractWineDLLResourcesTarget ( const Module& module, string_list& clean_files );
	void GenerateWin32DLLModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32CUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32CUIModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateWin32CUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwWin32GUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32GUIModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateWin32GUIModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwBootLoaderModuleHandler : public MingwModuleHandler
{
public:
	MingwBootLoaderModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBootLoaderModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwBootSectorModuleHandler : public MingwModuleHandler
{
public:
	MingwBootSectorModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateBootSectorModuleTarget ( const Module& module, string_list& clean_files );
};


class MingwIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwIsoModuleHandler ();
	virtual HostType DefaultHost() { return HostFalse; }
	virtual void Process ( const Module& module, string_list& clean_files );
private:
	void GenerateIsoModuleTarget ( const Module& module, string_list& clean_files );
	std::string GetBootstrapCdDirectories ( const std::string bootcdDirectory,
	                                        const Module& module ) const;
	std::string GetNonModuleCdDirectories ( const std::string bootcdDirectory,
	                                        const Module& module ) const;
	std::string GetCdDirectories ( const std::string bootcdDirectory,
	                               const Module& module ) const;
	std::string GetBootstrapCdFiles ( const std::string bootcdDirectory,
	                                  const Module& module ) const;
	std::string GetNonModuleCdFiles ( const std::string bootcdDirectory,
	                                  const Module& module ) const;
	std::string GetCdFiles ( const std::string bootcdDirectory,
	                         const Module& module ) const;
	void OutputBootstrapfileCopyCommands ( const std::string bootcdDirectory,
	                                       const Module& module ) const;
	void OutputCdfileCopyCommands ( const std::string bootcdDirectory,
	                                const Module& module ) const;
};

#endif /* MINGW_MODULEHANDLER_H */
