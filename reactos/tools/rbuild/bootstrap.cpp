#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;

Bootstrap::Bootstrap ( const Project& project_,
	                   const Module* module_,
	                   const XMLElement& bootstrapNode )
	: project(project_),
	  module(module_),
	  node(bootstrapNode)
{
	Initialize();
}

Bootstrap::~Bootstrap ()
{
}

bool
Bootstrap::IsSupportedModuleType ( ModuleType type )
{
	switch ( type )
	{
		case Kernel:
		case KernelModeDLL:
		case NativeDLL:
		case NativeCUI:
		case Win32DLL:
		case Win32CUI:
		case Win32GUI:
		case KernelModeDriver:
		case BootSector:
		case BootLoader:
			return true;
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case Iso:
		case LiveIso:
		case Test:
		case RpcServer:
		case RpcClient:
			return false;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

void
Bootstrap::Initialize ()
{
	if ( !IsSupportedModuleType ( module->type ) )
	{
		throw InvalidBuildFileException (
			node.location,
			"<bootstrap> is not applicable for this module type." );
	}

	const XMLAttribute* att = node.GetAttribute ( "base", false );
	if ( att != NULL )
		base = att->value;
	else
		base = "";

	att = node.GetAttribute ( "nameoncd", false );
	if ( att != NULL )
		nameoncd = att->value;
	else
		nameoncd = module->GetTargetName ();
}

void
Bootstrap::ProcessXML()
{
}
