#include "pch.h"
#include <assert.h>

#include "rbuild.h"

Configuration::Configuration ()
{
	Verbose = false;
	CleanAsYouGo = false;
	AutomaticDependencies = true;
	MakeHandlesInstallDirectories = false;
	GenerateProxyMakefilesInSourceTree = false;
}

Configuration::~Configuration ()
{
}
