#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

class MingwModuleHandler
{
public:
	MingwModuleHandler ();
	virtual bool CanHandleModule ( Module& module ) = 0;
	virtual void Process ( Module& module ) = 0;
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ();
	virtual bool CanHandleModule ( Module& module );
	virtual void Process ( Module& module );
};

#endif /* MINGW_MODULEHANDLER_H */
