// module.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

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
