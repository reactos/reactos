
#include "../../pch.h"

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

MingwModuleHandler::MingwModuleHandler ()
{
}


MingwKernelModuleHandler::MingwKernelModuleHandler ()
{
}

bool MingwKernelModuleHandler::CanHandleModule ( Module& module )
{
	return true;
}

void MingwKernelModuleHandler::Process ( Module& module )
{
}
