#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

extern std::string
ReplaceExtension ( const std::string& filename,
	               const std::string& newExtension );


class MingwModuleHandler
{
public:
	static std::map<ModuleType,MingwModuleHandler*>* handler_map;
	static int ref;

	MingwModuleHandler ( ModuleType moduletype );
	virtual ~MingwModuleHandler();

	static void SetMakefile ( FILE* f );
	static MingwModuleHandler* LookupHandler ( const std::string& location,
	                                           ModuleType moduletype_ );
	virtual void Process ( const Module& module ) = 0;
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	void GenerateDirectoryTargets () const;
	static std::string GetObjectFilename ( const std::string& sourceFilename );
protected:
	const std::string &PassThruCacheDirectory ( const std::string &f ) const;
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	std::string GetActualSourceFilename ( const std::string& filename ) const;
	std::string GetModuleArchiveFilename ( const Module& module ) const;
	bool IsGeneratedFile ( const File& file ) const;
	std::string GetImportLibraryDependency ( const Module& importedModule ) const;
	std::string GetImportLibraryDependencies ( const Module& module ) const;
	std::string GetModuleDependencies ( const Module& module ) const;
	std::string GetAllDependencies ( const Module& module ) const;
	std::string GetSourceFilenames ( const Module& module,
	                                 bool includeGeneratedFiles ) const;
	std::string GetSourceFilenames ( const Module& module ) const;
	std::string GetSourceFilenamesWithoutGeneratedFiles ( const Module& module ) const;

	std::string GetObjectFilenames ( const Module& module ) const;
	void GenerateMacrosAndTargetsHost ( const Module& module ) const;
	void GenerateMacrosAndTargetsTarget ( const Module& module ) const;
	void GenerateMacrosAndTargetsTarget ( const Module& module,
	                                      const std::string* cflags,
	                                      const std::string* nasmflags ) const;
	std::string GetInvocationDependencies ( const Module& module ) const;
	void GenerateInvocations ( const Module& module ) const;
	
	std::string GetPreconditionDependenciesName ( const Module& module ) const;
	void GeneratePreconditionDependencies ( const Module& module ) const;
	std::string GetCFlagsMacro ( const Module& module ) const;
	std::string GetObjectsMacro ( const Module& module ) const;
	std::string GetLinkerMacro ( const Module& module ) const;
	void GenerateLinkerCommand ( const Module& module,
	                             const std::string& linker,
	                             const std::string& linkerParameters,
	                             const std::string& objectFilenames ) const;
	void GenerateImportLibraryTargetIfNeeded ( const Module& module ) const;
	std::string GetDefinitionDependencies ( const Module& module ) const;
	std::string GetLinkingDependencies ( const Module& module ) const;
	bool IsCPlusPlusModule ( const Module& module ) const;
	static FILE* fMakefile;
	static std::set<std::string> directory_set;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	std::string GenerateCompilerParametersFromVector ( const std::vector<CompilerFlag*>& compilerFlags ) const;
	std::string GenerateLinkerParametersFromVector ( const std::vector<LinkerFlag*>& linkerFlags ) const;
	std::string GenerateLinkerParameters ( const Module& module ) const;
	void GenerateMacro ( const char* assignmentOperation,
	                     const std::string& macro,
	                     const std::vector<Include*>& includes,
	                     const std::vector<Define*>& defines,
	                     const std::vector<CompilerFlag*>* compilerFlags ) const;
	void GenerateMacros ( const char* op,
	                      const std::vector<File*>& files,
	                      const std::vector<Include*>& includes,
	                      const std::vector<Define*>& defines,
	                      const std::vector<CompilerFlag*>* compilerFlags,
	                      const std::vector<LinkerFlag*>* linkerFlags,
	                      const std::vector<If*>& ifs,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& windresflags_macro,
	                      const std::string& linkerflags_macro,
	                      const std::string& objs_macro) const;
	void GenerateMacros ( const Module& module,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& windresflags_macro,
	                      const std::string& linkerflags_macro,
	                      const std::string& objs_macro) const;
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
	                                 const std::string& sourceFilename ) const;
	void GenerateCommands ( const Module& module,
	                        const std::string& sourceFilename,
	                        const std::string& cc,
	                        const std::string& cppc,
	                        const std::string& cflagsMacro,
	                        const std::string& nasmflagsMacro,
	                        const std::string& windresflagsMacro ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::vector<File*>& files,
	                                 const std::vector<If*>& ifs,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc,
	                                 const std::string& cppc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro,
	                                 const std::string& windresflagsMacro ) const;
	void GetCleanTargets ( std::vector<std::string>& out,
	                       const std::vector<File*>& files,
	                       const std::vector<If*>& ifs ) const;
	std::string GenerateArchiveTarget ( const Module& module,
	                                    const std::string& ar,
	                                    const std::string& objs_macro ) const;
	void GenerateMacrosAndTargets ( const Module& module,
	                                const std::string& cc,
	                                const std::string& cppc,
	                                const std::string& ar,
	                                const std::string* clags,
	                                const std::string* nasmflags ) const;
	std::string GetSpecObjectDependencies ( const std::string& filename ) const;
	std::string GetDefaultDependencies ( const Module& module ) const;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateBuildToolModuleTarget ( const Module& module );
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModuleTarget ( const Module& module );
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateStaticLibraryModuleTarget ( const Module& module );
};


class MingwObjectLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwObjectLibraryModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateObjectLibraryModuleTarget ( const Module& module );
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModeDLLModuleTarget ( const Module& module );
};


class MingwKernelModeDriverModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDriverModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModeDriverModuleTarget ( const Module& module );
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateNativeDLLModuleTarget ( const Module& module );
};


class MingwNativeCUIModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeCUIModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateNativeCUIModuleTarget ( const Module& module );
};


class MingwWin32DLLModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32DLLModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateExtractWineDLLResourcesTarget ( const Module& module );
	void GenerateWin32DLLModuleTarget ( const Module& module );
};


class MingwWin32CUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32CUIModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateWin32CUIModuleTarget ( const Module& module );
};


class MingwWin32GUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32GUIModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateWin32GUIModuleTarget ( const Module& module );
};


class MingwBootLoaderModuleHandler : public MingwModuleHandler
{
public:
	MingwBootLoaderModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateBootLoaderModuleTarget ( const Module& module );
};


class MingwBootSectorModuleHandler : public MingwModuleHandler
{
public:
	MingwBootSectorModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateBootSectorModuleTarget ( const Module& module );
};


class MingwIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwIsoModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateIsoModuleTarget ( const Module& module );
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
