#ifndef __RBUILD_H
#define __RBUILD_H

#include "XML.h"

#include <string>
#include <vector>

class Project;
class Module;
class File;

class Project
{
public:
	std::string name;
	std::vector<Module*> modules;

	~Project();
	void ProcessXML ( const XMLElement& e, const std::string& path );
};

class Module
{
public:
	const XMLElement& node;
	std::string name;
	std::string path;
	std::vector<File*> files;

	Module ( const XMLElement& moduleNode,
	         const std::string& moduleName,
	         const std::string& modulePath );

	~Module();

	void ProcessXML ( const XMLElement& e, const std::string& path );
};

class File
{
public:
	std::string name;

	File ( const std::string& _name );
};

#endif /* __RBUILD_H */
