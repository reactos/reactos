#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

class MingwModuleHandler
{
public:
	static std::map<std::string,MingwModuleHandler*>* handler_map;

	MingwModuleHandler ( const char* moduletype_ );
	virtual ~MingwModuleHandler() {}

	static void SetMakefile ( FILE* f );
	static MingwModuleHandler* LookupHandler ( const std::string& location,
	                                           const std::string& moduletype_ );
	virtual void Process ( const Module& module ) = 0;

protected:
	std::string MingwModuleHandler::GetWorkingDirectory () const;
	std::string ReplaceExtension ( const std::string& filename,
	                               const std::string& newExtension ) const;
	std::string GetModuleArchiveFilename ( const Module& module ) const;
	std::string GetImportLibraryDependencies ( const Module& module ) const;
	std::string GetModuleDependencies ( const Module& module ) const;
	std::string GetAllDependencies ( const Module& module ) const;
	std::string GetSourceFilenames ( const Module& module ) const;

	std::string GetObjectFilename ( const std::string& sourceFilename ) const;
	std::string GetObjectFilenames ( const Module& module ) const;
	void GenerateObjectFileTargetsHost ( const Module& module ) const;
	void GenerateObjectFileTargetsTarget ( const Module& module ) const;
	void GenerateArchiveTargetHost ( const Module& module ) const;
	void GenerateArchiveTargetTarget ( const Module& module ) const;
	std::string GetInvocationDependencies ( const Module& module ) const;
	std::string GetInvocationParameters ( const Invoke& invoke ) const;
	void GenerateInvocations ( const Module& module ) const;
	void GeneratePreconditionDependencies ( const Module& module ) const;
	static FILE* fMakefile;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	void GenerateGccModuleIncludeVariable ( const Module& module ) const;
	std::string GenerateGccIncludeParameters ( const Module& module ) const;
	std::string GenerateGccParameters ( const Module& module ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc ) const;
	void GenerateArchiveTarget ( const Module& module,
	                             const std::string& ar ) const;
	std::string GetPreconditionDependenciesName ( const Module& module ) const;
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

#endif /* MINGW_MODULEHANDLER_H */
