#include "pch.h"
#include <assert.h>

#include "rbuild.h"

Configuration::Configuration ()
{
	Verbose = false;
	CleanAsYouGo = false;
	AutomaticDependencies = true;
}

Configuration::~Configuration ()
{
}
