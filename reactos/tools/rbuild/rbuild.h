#ifndef __RBUILD_H
#define __RBUILD_H

#include <string>
#include <vector>
#include "ssprintf.h"
#include "exception.h"
#include "XML.h"

class Project;
class Module;
class File;

class Project
{
public:
	std::string name;
	std::vector<Module*> modules;

	Project ();
	Project ( const std::string& filename );
	~Project ();
	void ProcessXML ( const XMLElement& e, const std::string& path );
private:
	void ReadXml ();
	XMLFile xmlfile;
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
