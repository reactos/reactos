#ifndef __RBUILD_H
#define __RBUILD_H

#include "pch.h"

#include "ssprintf.h"
#include "exception.h"
#include "XML.h"

#ifdef WIN32
#define EXEPOSTFIX ".exe"
#define CSEP '\\'
#define CBAD_SEP '/'
#define SSEP "\\"
#define SBAD_SEP "/"
#else
#define EXEPOSTFIX
#define CSEP '/'
#define CBAD_SEP '\\'
#define SSEP "/"
#define SBAD_SEP "\\"
#endif

class Project;
class Module;
class Include;
class Define;
class File;
class Library;

class Project
{
public:
	std::string name;
	std::string makefile;
	std::vector<Module*> modules;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	
	Project ();
	Project ( const std::string& filename );
	~Project ();
	void ProcessXML ( const XMLElement& e,
	                  const std::string& path );
	Module* LocateModule ( std::string name );
private:
	void ReadXml ();
	XMLFile xmlfile;
	XMLElement* head;
};


enum ModuleType
{
	BuildTool,
	StaticLibrary,
	KernelModeDLL
};


class Module
{
public:
	Project* project;
	const XMLElement& node;
	std::string name;
	std::string extension;
	std::string path;
	ModuleType type;
	std::vector<File*> files;
	std::vector<Library*> libraries;
	std::vector<Include*> includes;
	std::vector<Define*> defines;

	Module ( Project* project,
	         const XMLElement& moduleNode,
	         const std::string& modulePath );
	~Module ();
	ModuleType GetModuleType (const XMLAttribute& attribute );
	std::string GetPath ();
	void ProcessXML ( const XMLElement& e, const std::string& path );
private:
	std::string GetDefaultModuleExtension ();
};


class Include
{
public:
	Project* project;
	Module* module;
	const XMLElement& node;
	std::string directory;

	Include ( Project* project,
	          const XMLElement& includeNode );
	Include ( Project* project,
	          Module* module,
	          const XMLElement& includeNode );
	~Include ();
	void ProcessXML ( const XMLElement& e );
private:
	void Initialize ( const XMLElement& includeNode );
};


class Define
{
public:
	Project* project;
	Module* module;
	const XMLElement& node;
	std::string name;
	std::string value;

	Define ( Project* project,
	         const XMLElement& defineNode );
	Define ( Project* project,
	         Module* module,
	         const XMLElement& defineNode );
	~Define();
	void ProcessXML ( const XMLElement& e );
private:
	void Initialize ( const XMLElement& defineNode );
};


class File
{
public:
	std::string name;

	File ( const std::string& _name );
};


class Library
{
public:
	std::string name;

	Library ( const std::string& _name );
};

extern std::string
FixSeparator ( const std::string& s );

#endif /* __RBUILD_H */
