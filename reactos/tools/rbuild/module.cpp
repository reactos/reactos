#include "rbuild.h"

using std::string;
using std::vector;

Module::Module ( const XMLElement& moduleNode,
                 const string& moduleName,
                 const string& modulePath)
	: node(moduleNode),
	  name(moduleName),
	  path(modulePath)
{
}
