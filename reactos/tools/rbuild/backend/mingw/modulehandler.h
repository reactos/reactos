#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

class MingwModuleHandler
{
public:
	MingwModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( Module& module ) = 0;
	virtual void Process ( Module& module ) = 0;
protected:
	std::string ReplaceExtension ( std::string filename,
	                               std::string newExtension );
	std::string GetModuleArchiveFilename ( Module& module );
	std::string GetModuleLibraryDependencies ( Module& module );
	std::string GetSourceFilenames ( Module& module );
	std::string GetObjectFilename ( std::string sourceFilename );
	std::string GetObjectFilenames ( Module& module );
	void GenerateObjectFileTargets ( Module& module );
	void GenerateArchiveTarget ( Module& module );
	FILE* fMakefile;
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ( FILE* fMakefile );
	virtual bool CanHandleModule ( Module& module );
	virtual void Process ( Module& module );
private:
	void GenerateKernelModuleTarget ( Module& module );
};

#endif /* MINGW_MODULEHANDLER_H */
