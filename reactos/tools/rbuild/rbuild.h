#ifndef __RBUILD_H
#define __RBUILD_H

#include "pch.h"

#ifdef WIN32
#include <direct.h>
#include <io.h>
#endif/*WIN32*/
#include <sys/stat.h>
#include <time.h>
#ifdef _MSC_VER
#include <sys/utime.h>
#else/*_MSC_VER*/
#include <utime.h>
#ifdef WIN32
#include <process.h>
#endif/*WIN32*/
#endif/*_MSC_VER*/

#include "ssprintf.h"
#include "exception.h"
#include "XML.h"

typedef std::vector<std::string> string_list;

#ifdef WIN32
#define EXEPREFIX ""
#define EXEPOSTFIX ".exe"
#define CSEP '\\'
#define CBAD_SEP '/'
#define SSEP "\\"
#define SBAD_SEP "/"
#else
#define EXEPREFIX "./"
#define EXEPOSTFIX ""
#define CSEP '/'
#define CBAD_SEP '\\'
#define SSEP "/"
#define SBAD_SEP "\\"
#endif

class Project;
class IfableData;
class Module;
class Include;
class Define;
class File;
class Library;
class Invoke;
class InvokeFile;
class Dependency;
class ImportLibrary;
class If;
class CompilerFlag;
class LinkerFlag;
class Property;
class TestSupportCode;
class WineResource;
class AutomaticDependency;
class Bootstrap;
class CDFile;
class InstallFile;
class PchFile;
class StubbedComponent;
class StubbedSymbol;

class SourceFileTest;


class Configuration
{
public:
	Configuration ();
	~Configuration ();
	bool Verbose;
	bool CleanAsYouGo;
	bool AutomaticDependencies;
	bool CheckDependenciesForModuleOnly;
	std::string CheckDependenciesForModuleOnlyModule;
	bool MakeHandlesInstallDirectories;
	bool GenerateProxyMakefilesInSourceTree;
};

class Environment
{
public:
	static std::string GetVariable ( const std::string& name );
	static std::string GetIntermediatePath ();
	static std::string GetOutputPath ();
	static std::string GetInstallPath ();
	static std::string GetEnvironmentVariablePathOrDefault ( const std::string& name,
	                                                         const std::string& defaultValue );
};

class FileSupportCode
{
public:
	static void WriteIfChanged ( char* outbuf,
	                             std::string filename );
};

class IfableData
{
public:
	std::vector<File*> files;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	std::vector<Library*> libraries;
	std::vector<Property*> properties;
	std::vector<CompilerFlag*> compilerFlags;
	std::vector<If*> ifs;

	~IfableData();
	void ProcessXML();
};

class Project
{
	std::string xmlfile;
	XMLElement *node, *head;
public:
	std::string name;
	std::string makefile;
	XMLIncludes xmlbuildfiles;
	std::vector<Module*> modules;
	std::vector<LinkerFlag*> linkerFlags;
	std::vector<CDFile*> cdfiles;
	std::vector<InstallFile*> installfiles;
	IfableData non_if_data;

	Project ( const std::string& filename );
	~Project ();
	void WriteConfigurationFile ();
	void ExecuteInvocations ();
	void ProcessXML ( const std::string& path );
	Module* LocateModule ( const std::string& name );
	const Module* LocateModule ( const std::string& name ) const;
	std::string GetProjectFilename () const;
	std::string ResolveProperties ( const std::string& s ) const;
private:
	std::string ResolveNextProperty ( std::string& s ) const;
	const Property* LookupProperty ( const std::string& name ) const;
	void SetConfigurationOption ( char* s,
	                              std::string name,
	                              std::string* alternativeName );
	void SetConfigurationOption ( char* s,
	                              std::string name );
	void ReadXml ();
	void ProcessXMLSubElement ( const XMLElement& e,
	                            const std::string& path,
	                            If* pIf = NULL );

	// disable copy semantics
	Project ( const Project& );
	Project& operator = ( const Project& );
};


enum ModuleType
{
	BuildTool = 0,
	StaticLibrary = 1,
	ObjectLibrary = 2,
	Kernel = 3,
	KernelModeDLL = 4,
	KernelModeDriver = 5,
	NativeDLL = 6,
	NativeCUI = 7,
	Win32DLL = 8,
	Win32CUI = 9,
	Win32GUI = 10,
	BootLoader = 11,
	BootSector = 12,
	Iso = 13,
	LiveIso = 14,
	Test = 15,
	RpcServer = 16,
	RpcClient = 17
};

enum HostType
{
	HostFalse,
	HostDefault,
	HostTrue
};

class Module
{
public:
	const Project& project;
	const XMLElement& node;
	std::string xmlbuildFile;
	std::string name;
	std::string extension;
	std::string entrypoint;
	std::string baseaddress;
	std::string path;
	ModuleType type;
	ImportLibrary* importLibrary;
	bool mangledSymbols;
	Bootstrap* bootstrap;
	IfableData non_if_data;
	std::vector<Invoke*> invocations;
	std::vector<Dependency*> dependencies;
	std::vector<CompilerFlag*> compilerFlags;
	std::vector<LinkerFlag*> linkerFlags;
	std::vector<StubbedComponent*> stubbedComponents;
	PchFile* pch;
	bool cplusplus;
	std::string prefix;
	HostType host;
	std::string installBase;
	std::string installName;
	bool useWRC;
	bool enableWarnings;
	bool enabled;

	Module ( const Project& project,
	         const XMLElement& moduleNode,
	         const std::string& modulePath );
	~Module ();
	ModuleType GetModuleType ( const std::string& location,
	                           const XMLAttribute& attribute );
	bool HasImportLibrary () const;
	bool IsDLL () const;
	bool GenerateInOutputTree () const;
	std::string GetTargetName () const;
	std::string GetDependencyPath () const;
	std::string GetBasePath () const;
	std::string GetPath () const;
	std::string GetPathWithPrefix ( const std::string& prefix ) const;
	void GetTargets ( string_list& ) const;
	std::string GetInvocationTarget ( const int index ) const;
	bool HasFileWithExtension ( const IfableData&, const std::string& extension ) const;
	void InvokeModule () const;
	void ProcessXML ();
	void GetSourceFilenames ( string_list& list,
                                  bool includeGeneratedFiles ) const;
private:
	std::string GetDefaultModuleExtension () const;
	std::string GetDefaultModuleEntrypoint () const;
	std::string GetDefaultModuleBaseaddress () const;
	void ProcessXMLSubElement ( const XMLElement& e,
	                            const std::string& path,
	                            If* pIf = NULL );
};


class Include
{
public:
	const Project& project;
	const Module* module;
	const XMLElement* node;
	std::string directory;
	std::string basePath;

	Include ( const Project& project,
	          const XMLElement* includeNode );
	Include ( const Project& project,
	          const Module* module,
	          const XMLElement* includeNode );
	Include ( const Project& project,
	          std::string directory,
	          std::string basePath );
	~Include ();
	void ProcessXML();
private:
};


class Define
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string name;
	std::string value;

	Define ( const Project& project,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const XMLElement& defineNode );
	~Define();
	void ProcessXML();
private:
	void Initialize();
};


class File
{
public:
	std::string name;
	bool first;
	std::string switches;
	bool isPreCompiledHeader;

	File ( const std::string& _name,
	       bool _first,
	       std::string _switches,
	       bool _isPreCompiledHeader );

	void ProcessXML();
	bool IsGeneratedFile () const;
};


class Library
{
public:
	const XMLElement& node;
	const Module& module;
	std::string name;
	const Module* importedModule;

	Library ( const XMLElement& _node,
	          const Module& _module,
	          const std::string& _name );

	void ProcessXML();
};


class Invoke
{
public:
	const XMLElement& node;
	const Module& module;
	const Module* invokeModule;
	std::vector<InvokeFile*> input;
	std::vector<InvokeFile*> output;

	Invoke ( const XMLElement& _node,
	         const Module& _module );

	void ProcessXML();
	void GetTargets ( string_list& targets ) const;
	std::string GetParameters () const;
private:
	void ProcessXMLSubElement ( const XMLElement& e );
	void ProcessXMLSubElementInput ( const XMLElement& e );
	void ProcessXMLSubElementOutput ( const XMLElement& e );
};


class InvokeFile
{
public:
	const XMLElement& node;
	std::string name;
	std::string switches;

	InvokeFile ( const XMLElement& _node,
	             const std::string& _name );

	void ProcessXML ();
};


class Dependency
{
public:
	const XMLElement& node;
	const Module& module;
	const Module* dependencyModule;

	Dependency ( const XMLElement& _node,
	             const Module& _module );

	void ProcessXML();
};


class ImportLibrary
{
public:
	const XMLElement& node;
	const Module& module;
	std::string basename;
	std::string definition;

	ImportLibrary ( const XMLElement& _node,
	                const Module& module );

	void ProcessXML ();
};


class If
{
public:
	const XMLElement& node;
	const Project& project;
	const Module* module;
	const bool negated;
	std::string property, value;
	IfableData data;

	If ( const XMLElement& node_,
	     const Project& project_,
	     const Module* module_,
	     const bool negated_ = false );
	~If();

	void ProcessXML();
};


class CompilerFlag
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string flag;

	CompilerFlag ( const Project& project,
	               const XMLElement& compilerFlagNode );
	CompilerFlag ( const Project& project,
	               const Module* module,
	               const XMLElement& compilerFlagNode );
	~CompilerFlag ();
	void ProcessXML();
private:
	void Initialize();
};


class LinkerFlag
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string flag;

	LinkerFlag ( const Project& project,
	             const XMLElement& linkerFlagNode );
	LinkerFlag ( const Project& project,
	             const Module* module,
	             const XMLElement& linkerFlagNode );
	~LinkerFlag ();
	void ProcessXML();
private:
	void Initialize();
};


class Property
{
public:
	const XMLElement& node;
	const Project& project;
	const Module* module;
	std::string name, value;

	Property ( const XMLElement& node_,
	           const Project& project_,
	           const Module* module_ );

	void ProcessXML();
};


class TestSupportCode
{
public:
	const Project& project;

	TestSupportCode ( const Project& project );
	~TestSupportCode ();
	void GenerateTestSupportCode ( bool verbose );
private:
	bool IsTestModule ( const Module& module );
	void GenerateTestSupportCodeForModule ( Module& module,
	                                        bool verbose );
	std::string GetHooksFilename ( Module& module );
	char* WriteStubbedSymbolToHooksFile ( char* buffer,
	                                      const StubbedComponent& component,
	                                      const StubbedSymbol& symbol );
	char* WriteStubbedComponentToHooksFile ( char* buffer,
	                                         const StubbedComponent& component );
	void WriteHooksFile ( Module& module );
	std::string GetStubsFilename ( Module& module );
	char* WriteStubbedSymbolToStubsFile ( char* buffer,
                                              const StubbedComponent& component,
	                                      const StubbedSymbol& symbol,
	                                      int stubIndex );
	char* WriteStubbedComponentToStubsFile ( char* buffer,
                                                 const StubbedComponent& component,
	                                         int* stubIndex );
	void WriteStubsFile ( Module& module );
	std::string GetStartupFilename ( Module& module );
	bool IsUnknownCharacter ( char ch );
	std::string GetTestDispatcherName ( std::string filename );
	bool IsTestFile ( std::string& filename ) const;
	void GetSourceFilenames ( string_list& list,
                                  Module& module ) const;
	char* WriteTestDispatcherPrototypesToStartupFile ( char* buffer,
                                                           Module& module );
	char* WriteRegisterTestsFunctionToStartupFile ( char* buffer,
	                                                Module& module );
	void WriteStartupFile ( Module& module );
};


class WineResource
{
public:
	const Project& project;
	std::string bin2res;

	WineResource ( const Project& project,
	               std::string bin2res );
	~WineResource ();
	void UnpackResources ( bool verbose );
private:
	bool IsSpecFile ( const File& file );
	bool IsWineModule ( const Module& module );
	bool IsResourceFile ( const File& file );
	std::string GetResourceFilename ( const Module& module );
	void UnpackResourcesInModule ( Module& module,
	                               bool verbose );
};


class SourceFile
{
public:
	SourceFile ( AutomaticDependency* automaticDependency,
	             const Module& module,
	             const std::string& filename,
	             SourceFile* parent,
	             bool isNonAutomaticDependency );
	SourceFile* ParseFile ( const std::string& normalizedFilename );
	void Parse ();
	std::string Location () const;
	std::vector<SourceFile*> files;
	AutomaticDependency* automaticDependency;
	const Module& module;
	std::string filename;
	std::string filenamePart;
	std::string directoryPart;
	std::vector<SourceFile*> parents; /* List of files, this file is included from */
	bool isNonAutomaticDependency;
	std::string cachedDependencies;
	time_t lastWriteTime;
	time_t youngestLastWriteTime; /* Youngest last write time of this file and all children */
	SourceFile* youngestFile;
private:
	void GetDirectoryAndFilenameParts ();
	void Close ();
	void Open ();
	void SkipWhitespace ();
	bool ReadInclude ( std::string& filename,
	                   bool& searchCurrentDirectory,
	                   bool& includeNext );
	bool IsIncludedFrom ( const std::string& normalizedFilename );
	SourceFile* GetParentSourceFile ();
	bool CanProcessFile ( const std::string& extension );
	bool IsParentOf ( const SourceFile* parent,
	                  const SourceFile* child );
	std::string buf;
	const char *p;
	const char *end;
};


class AutomaticDependency
{
	friend class SourceFileTest;
public:
	const Project& project;

	AutomaticDependency ( const Project& project );
	~AutomaticDependency ();
	std::string GetFilename ( const std::string& filename );
	bool LocateIncludedFile ( const std::string& directory,
	                          const std::string& includedFilename,
	                          std::string& resolvedFilename );
	bool LocateIncludedFile ( SourceFile* sourceFile,
	                          const Module& module,
	                          const std::string& includedFilename,
	                          bool searchCurrentDirectory,
	                          bool includeNext,
	                          std::string& resolvedFilename );
	SourceFile* RetrieveFromCacheOrParse ( const Module& module,
	                                       const std::string& filename,
	                                       SourceFile* parentSourceFile );
	SourceFile* RetrieveFromCache ( const std::string& filename );
	void CheckAutomaticDependencies ( bool verbose );
	void CheckAutomaticDependenciesForModule ( Module& module,
	                                           bool verbose );
private:
	void GetModulesToCheck ( Module& module, std::vector<const Module*>& modules );
	void CheckAutomaticDependencies ( const Module& module,
                                          bool verbose );
	void CheckAutomaticDependenciesForFile ( SourceFile* sourceFile );
	void GetIncludeDirectories ( std::vector<Include*>& includes,
	                             const Module& module,
                                     Include& currentDirectory,
                                     bool searchCurrentDirectory );
	void GetModuleFiles ( const Module& module,
                              std::vector<File*>& files ) const;
	void ParseFiles ();
	void ParseFiles ( const Module& module );
	void ParseFile ( const Module& module,
	                 const File& file );
	std::map<std::string, SourceFile*> sourcefile_map;
};


class Bootstrap
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string base;
	std::string nameoncd;

	Bootstrap ( const Project& project,
	            const Module* module,
	            const XMLElement& bootstrapNode );
	~Bootstrap ();
	void ProcessXML();
private:
	bool IsSupportedModuleType ( ModuleType type );
	void Initialize();
};


class CDFile
{
public:
	const Project& project;
	const XMLElement& node;
	std::string name;
	std::string base;
	std::string nameoncd;
	std::string path;

	CDFile ( const Project& project,
	         const XMLElement& bootstrapNode,
	         const std::string& path );
	~CDFile ();
	void ProcessXML();
	std::string GetPath () const;
};


class InstallFile
{
public:
	const Project& project;
	const XMLElement& node;
	std::string name;
	std::string base;
	std::string newname;
	std::string path;

	InstallFile ( const Project& project,
	              const XMLElement& bootstrapNode,
	              const std::string& path );
	~InstallFile ();
	void ProcessXML ();
	std::string GetPath () const;
};


class PchFile
{
public:
	const XMLElement& node;
	const Module& module;
	File file;

	PchFile (
		const XMLElement& node,
		const Module& module,
		const File file );
	void ProcessXML();
};


class StubbedComponent
{
public:
	const Module* module;
	const XMLElement& node;
	std::string name;
	std::vector<StubbedSymbol*> symbols;

	StubbedComponent ( const Module* module_,
	                   const XMLElement& stubbedComponentNode );
	~StubbedComponent ();
	void ProcessXML ();
	void ProcessXMLSubElement ( const XMLElement& e );
};


class StubbedSymbol
{
public:
	const XMLElement& node;
	std::string symbol;
	std::string newname;
	std::string strippedName;

	StubbedSymbol ( const XMLElement& stubbedSymbolNode );
	~StubbedSymbol ();
	void ProcessXML();
private:
	std::string StripSymbol ( std::string symbol );
};

extern std::string
FixSeparator ( const std::string& s );

extern std::string
ReplaceExtension (
	const std::string& filename,
	const std::string& newExtension );

extern std::string
GetSubPath (
	const std::string& location,
	const std::string& path,
	const std::string& att_value );

extern std::string
GetExtension ( const std::string& filename );

extern std::string
GetDirectory ( const std::string& filename );

extern std::string
GetFilename ( const std::string& filename );

extern std::string
NormalizeFilename ( const std::string& filename );

#endif /* __RBUILD_H */
