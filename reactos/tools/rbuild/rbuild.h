/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
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

#include <infhost.h>

#include "ssprintf.h"
#include "exception.h"
#include "xml.h"

class Backend; // forward declaration

typedef std::vector<std::string> string_list;

extern std::string ExePrefix;
extern std::string ExePostfix;
extern std::string sSep;
extern std::string sBadSep;
extern char cSep;
extern char cBadSep;

#ifdef WIN32
#define DEF_EXEPREFIX ""
#define DEF_EXEPOSTFIX ".exe"
#define DEF_CSEP '\\'
#define DEF_CBAD_SEP '/'
#define DEF_SSEP "\\"
#define DEF_SBAD_SEP "/"
#else
#define DEF_EXEPREFIX "./"
#define DEF_EXEPOSTFIX ""
#define DEF_CSEP '/'
#define DEF_CBAD_SEP '\\'
#define DEF_SSEP "/"
#define DEF_SBAD_SEP "\\"
#endif

#define MS_VS_DEF_VERSION "7.10"

class XmlNode;
class Directory;
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
class LinkerScript;
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
class CompilationUnit;
class FileLocation;
class AutoRegister;

class SourceFileTest;
class Metadata;

typedef std::map<std::string,Directory*> directory_map;

class XmlNode
{
protected:
	const Project& project;
	const XMLElement& node;

	XmlNode ( const Project& project_,
	          const XMLElement& node_ );
	virtual ~XmlNode();

public:
	virtual void ProcessXML();
};

class Directory
{
public:
	std::string name;
	directory_map subdirs;
	Directory ( const std::string& name );
	void Add ( const char* subdir );
	void GenerateTree ( const std::string& parent,
	                    bool verbose );
	std::string EscapeSpaces ( std::string path );
	void CreateRule ( FILE* f,
	                  const std::string& parent );
private:
	bool mkdir_p ( const char* path );
	std::string ReplaceVariable ( const std::string& name,
	                              const std::string& value,
	                              std::string path );
	std::string GetEnvironmentVariable ( const std::string& name );
	void ResolveVariablesInPath ( char* buf,
	                              const std::string& path );
	bool CreateDirectory ( std::string path );
};


class Configuration
{
public:
	Configuration ();
	~Configuration ();
	bool Verbose;
	bool CleanAsYouGo;
	bool AutomaticDependencies;
	bool CheckDependenciesForModuleOnly;
	bool CompilationUnitsEnabled;
	std::string CheckDependenciesForModuleOnlyModule;
	std::string VSProjectVersion;
	std::string VSConfigurationType;
	bool UseVSVersionInPath;
	bool UseConfigurationInPath;
	bool MakeHandlesInstallDirectories;
	bool GenerateProxyMakefilesInSourceTree;
	bool InstallFiles;
};

class Environment
{
public:
	static std::string GetVariable ( const std::string& name );
	static std::string GetArch ();
	static std::string GetIntermediatePath ();
	static std::string GetOutputPath ();
	static std::string GetCdOutputPath ();
	static std::string GetInstallPath ();
	static std::string GetAutomakeFile ( const std::string& defaultFile );
	static std::string GetEnvironmentVariablePathOrDefault ( const std::string& name,
	                                                         const std::string& defaultValue );
};


class FileSupportCode
{
public:
	static void WriteIfChanged ( char* outbuf,
	                             const std::string& filename,
	                             bool ignoreError = false );
};


class ParseContext
{
public:
	If* ifData;
	CompilationUnit* compilationUnit;
	ParseContext ();
};


class IfableData
{
public:
	std::vector<CompilationUnit*> compilationUnits;
	std::vector<File*> files;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	std::vector<Library*> libraries;
	std::vector<Property*> properties;
	std::vector<Module*> modules;
	std::vector<CompilerFlag*> compilerFlags;
	std::vector<If*> ifs;

	~IfableData();
	void ProcessXML();
	void ExtractModules( std::vector<Module*> &modules );
};

class Project
{
	std::string xmlfile;
	XMLElement *node, *head;
	Backend* _backend;
public:
	const Configuration& configuration;
	std::string name;
	std::string makefile;
	XMLIncludes xmlbuildfiles;
	std::vector<LinkerFlag*> linkerFlags;
	std::vector<CDFile*> cdfiles;
	std::vector<InstallFile*> installfiles;
	std::vector<Module*> modules;
	IfableData non_if_data;

	Project ( const Configuration& configuration,
	          const std::string& filename,
	          const std::map<std::string, std::string>* properties = NULL );
	~Project ();
	void SetBackend ( Backend* backend ) { _backend = backend; }
	Backend& GetBackend() { return *_backend; }
	void WriteConfigurationFile ();
	void ExecuteInvocations ();

	void ProcessXML ( const std::string& path );
	Module* LocateModule ( const std::string& name );
	const Module* LocateModule ( const std::string& name ) const;
	const std::string& GetProjectFilename () const;
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
	                            ParseContext& parseContext );

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
	Win32OCX = 9,
	Win32CUI = 10,
	Win32GUI = 11,
	BootLoader = 12,
	BootSector = 13,
	Iso = 14,
	LiveIso = 15,
	Test = 16,
	RpcServer = 17,
	RpcClient = 18,
	Alias = 19,
	BootProgram = 20,
	Win32SCR = 21,
	IdlHeader = 23,
	IsoRegTest = 24,
	LiveIsoRegTest = 25,
	EmbeddedTypeLib = 26,
	ElfExecutable = 27
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
	std::string guid;
	std::string extension;
	std::string baseaddress;
	std::string payload;
	std::string buildtype;
	std::string path;
	ModuleType type;
	ImportLibrary* importLibrary;
	Metadata* metadata;
	bool mangledSymbols;
	bool underscoreSymbols;
	bool isUnicode;
	bool isDefaultEntryPoint;
	Bootstrap* bootstrap;
	AutoRegister* autoRegister; // <autoregister> node
	IfableData non_if_data;
	std::vector<Invoke*> invocations;
	std::vector<Dependency*> dependencies;
	std::vector<CompilerFlag*> compilerFlags;
	std::vector<LinkerFlag*> linkerFlags;
	std::vector<StubbedComponent*> stubbedComponents;
	LinkerScript* linkerScript;
	PchFile* pch;
	bool cplusplus;
	std::string prefix;
	HostType host;
	std::string installBase;
	std::string installName;
	std::string aliasedModuleName;
	bool useWRC;
	bool allowWarnings;
	bool enabled;
	bool useHostStdlib;
	bool isStartupLib;

	Module ( const Project& project,
	         const XMLElement& moduleNode,
	         const std::string& modulePath );
	~Module ();
	ModuleType GetModuleType ( const std::string& location,
	                           const XMLAttribute& attribute );
	bool HasImportLibrary () const;
	bool IsDLL () const;
	bool GenerateInOutputTree () const;
	std::string GetTargetName () const; // "foo.exe"
	std::string GetDependencyPath () const; // "path/foo.exe" or "path/libfoo.a"
	std::string GetDependencyTargetName () const; // "foo.exe" or "libfoo.a"
	std::string GetBasePath () const; // "path"
	std::string GetPath () const; // "path/foo.exe"
	std::string GetPathWithPrefix ( const std::string& prefix ) const; // "path/prefixfoo.exe"
	std::string GetPathToBaseDir() const; // "../" offset to rootdirectory
	std::string GetEntryPoint(bool leadingUnderscore) const;
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
	std::string entrypoint;
	void ProcessXMLSubElement ( const XMLElement& e,
	                            const std::string& path,
	                            const std::string& path_prefix,
	                            ParseContext& parseContext );
};


class Include
{
public:
	const Project& project;
	const Module* module;
	const XMLElement* node;
	const Module* baseModule;
	std::string directory;
	std::string basePath;
	std::string root;

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
	const XMLElement* node;
	std::string name;
	std::string value;
	std::string backend;

	Define ( const Project& project,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const std::string name_,
	         const std::string backend_ = "" );
	~Define();
	void ProcessXML();
private:
	void Initialize();
};


class File
{
public:
	std::string name;
	std::string path_prefix;
	bool first;
	std::string switches;
	bool isPreCompiledHeader;

	File ( const std::string& _name,
	       bool _first,
	       std::string _switches,
	       bool _isPreCompiledHeader );

	File ( const std::string& _name,
	       const std::string& _path_prefix,
	       bool _first,
	       std::string _switches,
	       bool _isPreCompiledHeader );

	void ProcessXML();
	std::string GetFullPath () const;
};


class Library
{
	const XMLElement *node;
public:
	const Module& module;
	std::string name;
	const Module* importedModule;

	Library ( const XMLElement& _node,
	          const Module& _module,
	          const std::string& _name );
	Library ( const Module& _module,
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

class Metadata
{
public:
	const XMLElement& node;
	const Module& module;
	std::string name;
	std::string description;
	std::string version;
	std::string copyright;
	std::string url;
	std::string date;
	std::string owner;

	Metadata ( const XMLElement& _node,
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
	std::string dllname;

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


class LinkerScript
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	const Module* baseModule;
	std::string directory;
	std::string basePath;

	LinkerScript ( const Project& project,
	               const Module* module,
	               const XMLElement& node );
	~LinkerScript ();
	void ProcessXML();
};


class Property
{
public:
	const Project& project;
	const Module* module;
	std::string name, value;

	Property ( const XMLElement& node_,
	           const Project& project_,
	           const Module* module_ );

	Property ( const Project& project_,
	           const Module* module_,
	           const std::string& name_,
	           const std::string& value_ );

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
	std::string ReplaceVariable ( const std::string& name,
	                              const std::string& value,
	                              std::string path );
	std::string ResolveVariablesInPath ( const std::string& path );
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
	static std::string ReplaceVariable ( const std::string& name,
	                                     const std::string& value,
	                                     std::string path );
};


enum DirectoryLocation
{
	SourceDirectory,
	IntermediateDirectory,
	OutputDirectory,
	InstallDirectory,
	TemporaryDirectory,
};


class FileLocation
{
public:
	DirectoryLocation directory;
	std::string relative_path;
	std::string name;

	FileLocation ( const DirectoryLocation directory,
	               const std::string& relative_path,
	               const std::string& name );

	FileLocation ( const FileLocation& other );
};


class CDFile : public XmlNode
{
public:
	FileLocation *source;
	FileLocation *target;

	CDFile ( const Project& project,
	         const XMLElement& bootstrapNode,
	         const std::string& path );
private:
	static std::string ReplaceVariable ( const std::string& name,
	                                     const std::string& value,
	                                     std::string path );
};


class InstallFile : public XmlNode
{
public:
	FileLocation *source;
	FileLocation *target;

	InstallFile ( const Project& project,
	              const XMLElement& bootstrapNode,
	              const std::string& path );
};


class PchFile
{
public:
	const XMLElement& node;
	const Module& module;
	FileLocation file;

	PchFile (
		const XMLElement& node,
		const Module& module,
		const FileLocation& file );
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


class CompilationUnit
{
public:
	const Project* project;
	const Module* module;
	const XMLElement* node;
	std::string name;
	std::vector<File*> files;

	CompilationUnit ( File* file );
	CompilationUnit ( const Project* project,
	                  const Module* module,
	                  const XMLElement* node );
	~CompilationUnit ();
	void ProcessXML();
	bool IsGeneratedFile () const;
	bool HasFileWithExtension ( const std::string& extension ) const;
	bool IsFirstFile () const;
	const FileLocation* GetFilename () const;
	std::string GetSwitches () const;
private:
	std::string local_name;
};


class CompilationUnitSupportCode
{
public:
	const Project& project;

	CompilationUnitSupportCode ( const Project& project );
	~CompilationUnitSupportCode ();
	void Generate ( bool verbose );
private:
	void GenerateForModule ( Module& module,
	                         bool verbose );
	std::string GetCompilationUnitFilename ( Module& module,
	                                         CompilationUnit& compilationUnit );
	void WriteCompilationUnitFile ( Module& module,
	                                CompilationUnit& compilationUnit );
};


enum AutoRegisterType
{
	DllRegisterServer,
	DllInstall,
	Both
};

class AutoRegister : public XmlNode
{
public:
	const Module* module;
	std::string infSection;
	AutoRegisterType type;
	AutoRegister ( const Project& project_,
	               const Module* module_,
	               const XMLElement& node_ );
private:
	bool IsSupportedModuleType ( ModuleType type );
	AutoRegisterType GetAutoRegisterType( const std::string& type );
	void Initialize ();
};


class SysSetupGenerator
{
public:
	const Project& project;
	SysSetupGenerator ( const Project& project );
	~SysSetupGenerator ();
	void Generate ();
private:
	std::string GetDirectoryId ( const Module& module );
	std::string GetFlags ( const Module& module );
	void Generate ( HINF inf,
	                const Module& module );
};


extern void
InitializeEnvironment ();

extern std::string
Right ( const std::string& s, size_t n );

extern std::string
Replace ( const std::string& s, const std::string& find, const std::string& with );

extern std::string
ChangeSeparator ( const std::string& s,
                  const char fromSeparator,
                  const char toSeparator );

extern std::string
FixSeparator ( const std::string& s );

extern std::string
FixSeparatorForSystemCommand ( const std::string& s );

extern std::string
DosSeparator ( const std::string& s );

extern std::string
ReplaceExtension (
	const std::string& filename,
	const std::string& newExtension );

extern std::string
GetSubPath (
	const Project& project,
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

extern std::string
ToLower ( std::string filename );

#endif /* __RBUILD_H */
