#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

class MingwModuleHandler
{
public:
	MingwModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( const Module& module ) const = 0;
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
	void GenerateInvocations ( const Module& module ) const;
	FILE* fMakefile;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	std::string GenerateGccIncludeParameters ( const Module& module ) const;
	std::string GenerateGccParameters ( const Module& module ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc ) const;
	void GenerateArchiveTarget ( const Module& module,
	                             const std::string& ar ) const;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( const Module& module ) const;
	virtual void Process ( const Module& module );
private:
	void GenerateBuildToolModuleTarget ( const Module& module );
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( const Module& module ) const;
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModuleTarget ( const Module& module );
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( const Module& module ) const;
	virtual void Process ( const Module& module );
private:
	void GenerateStaticLibraryModuleTarget ( const Module& module );
};

#endif /* MINGW_MODULEHANDLER_H */
