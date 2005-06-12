#include "pch.h"
#include <assert.h>

#include "rbuild.h"

Configuration::Configuration ()
{
	Verbose = false;
	CleanAsYouGo = false;
	AutomaticDependencies = true;
	CheckDependenciesForModuleOnly = false;
	MakeHandlesInstallDirectories = false;
	GenerateProxyMakefilesInSourceTree = false;
}

Configuration::~Configuration ()
{
}
