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
	FILE* fMakefile;
	std::string GetModuleDependencies ( Module& module );
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
