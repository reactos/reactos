
#include "../../pch.h"

#include "mingw.h"

using std::string;
using std::vector;

MingwBackend::MingwBackend ( Project& project )
	: Backend ( project )
{
}

void MingwBackend::Process ()
{
	for ( size_t i = 0; i < ProjectNode.modules.size (); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		ProcessModule ( module );
	}
}

void MingwBackend::ProcessModule ( Module& module )
{
	MingwModuleHandlerList moduleHandlers;
	GetModuleHandlers ( moduleHandlers );
	for (size_t i = 0; i < moduleHandlers.size(); i++)
	{
		MingwModuleHandler& moduleHandler = *moduleHandlers[i];
		if (moduleHandler.CanHandleModule ( module ) )
		{
			moduleHandler.Process ( module );
			return;
		}
	}
}

void MingwBackend::GetModuleHandlers ( MingwModuleHandlerList& moduleHandlers )
{
	moduleHandlers.push_back ( new MingwKernelModuleHandler () );
}
