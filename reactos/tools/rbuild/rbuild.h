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
class CompilerFlag;
class LinkerFlag;
class LinkerScript;
class Property;
class TestSupportCode;
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
class Bootsector;

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

enum DirectoryLocation
{
	SourceDirectory,
	IntermediateDirectory,
	OutputDirectory,
	InstallDirectory,
	TemporaryDirectory,
};

class Directory
{
public:
	std::string name;
	directory_map subdirs;
	Directory ( const std::string& name );
	~Directory();
	void Add ( const char* subdir );
	void GenerateTree ( DirectoryLocation root,
	                    bool verbose );
	void CreateRule ( FILE* f,
	                  const std::string& parent );
private:
	bool mkdir_p ( const char* path );
	bool CreateDirectory ( const std::string& path );
	std::string EscapeSpaces ( const std::string& path );
	void GenerateTree ( const std::string& parent,
	                    bool verbose );
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
	bool PrecompiledHeadersEnabled;
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
	std::map<std::string, Property*> properties;
	std::vector<Module*> modules;
	std::vector<CompilerFlag*> compilerFlags;
	int asmFiles; // number of .asm files in compilationUnits

	IfableData();
	~IfableData();
	void ProcessXML();
	void ExtractModules( std::map<std::string, Module*> &modules );
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
	std::map<std::string, Module*> modules;
	IfableData non_if_data;

	Project ( const Configuration& configuration,
	          const std::string& filename,
	          const std::map<std::string, std::string>* properties = NULL );
	~Project ();
	void SetBackend ( Backend* backend ) { _backend = backend; }
	Backend& GetBackend() { return *_backend; }
	void ExecuteInvocations ();

	void ProcessXML ( const std::string& path );
	Module* LocateModule ( const std::string& name );
	const Module* LocateModule ( const std::string& name ) const;
	const std::string& GetProjectFilename () const;
	std::string ResolveProperties ( const std::string& s ) const;
	const Property* LookupProperty ( const std::string& name ) const;
private:
	std::string ResolveNextProperty ( const std::string& s ) const;
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
	BuildTool,
	StaticLibrary,
	ObjectLibrary,
	Kernel,
	KernelModeDLL,
	KernelModeDriver,
	NativeDLL,
	NativeCUI,
	Win32DLL,
	Win32OCX,
	Win32CUI,
	Win32GUI,
	BootLoader,
	BootSector,
	Iso,
	LiveIso,
	Test,
	RpcServer,
	RpcClient,
	Alias,
	BootProgram,
	Win32SCR,
	IdlHeader,
	IsoRegTest,
	LiveIsoRegTest,
	EmbeddedTypeLib,
	ElfExecutable,
	RpcProxy,
	HostStaticLibrary,
	Cabinet,
	KeyboardLayout,
	MessageHeader,
	TypeDontCare, // always at the end
};

enum HostType
{
	HostFalse,
	HostDefault,
	HostTrue,
	HostDontCare,
};

enum CompilerType
{
	CompilerTypeDontCare,
	CompilerTypeCC,
	CompilerTypeCPP,
};

class FileLocation
{
public:
	DirectoryLocation directory;
	std::string relative_path;
	std::string name;

	FileLocation ( const DirectoryLocation directory,
	               const std::string& relative_path,
	               const std::string& name,
	               const XMLElement *node = NULL );

	FileLocation ( const FileLocation& other );
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
	ModuleType type;
	ImportLibrary* importLibrary;
	Metadata* metadata;
	Bootsector* bootSector;
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
	std::string aliasedModuleName;
	bool allowWarnings;
	bool enabled;
	bool isStartupLib;
	FileLocation *output; // "path/foo.exe"
	FileLocation *dependency; // "path/foo.exe" or "path/libfoo.a"
	FileLocation *install;
	std::string description;
	std::string lcid;
	std::string layoutId;
	std::string layoutNameResId;

	Module ( const Project& project,
	         const XMLElement& moduleNode,
	         const std::string& modulePath );
	~Module ();
	ModuleType GetModuleType ( const std::string& location,
	                           const XMLAttribute& attribute );
	bool HasImportLibrary () const;
	bool IsDLL () const;
	std::string GetPathWithPrefix ( const std::string& prefix ) const; // "path/prefixfoo.exe"
	std::string GetPathToBaseDir() const; // "../" offset to rootdirectory
	std::string GetEntryPoint(bool leadingUnderscore) const;
	void GetTargets ( string_list& ) const;
	std::string GetInvocationTarget ( const int index ) const;
	bool HasFileWithExtension ( const IfableData&, const std::string& extension ) const;
	void InvokeModule () const;
	void ProcessXML ();
private:
	void SetImportLibrary ( ImportLibrary* importLibrary );
	DirectoryLocation GetTargetDirectoryTree () const;
	std::string GetDefaultModuleExtension () const;
	std::string GetDefaultModuleEntrypoint () const;
	std::string GetDefaultModuleBaseaddress () const;
	std::string entrypoint;
	void ProcessXMLSubElement ( const XMLElement& e,
	                            DirectoryLocation directory,
	                            const std::string& relative_path,
	                            ParseContext& parseContext );
};


class Include
{
public:
	FileLocation *directory;

	Include ( const Project& project,
	          const XMLElement* includeNode );
	Include ( const Project& project,
	          const XMLElement* includeNode,
	          const Module* module );
	Include ( const Project& project,
	          DirectoryLocation directory,
	          const std::string& relative_path );
	~Include ();
	void ProcessXML ();
private:
	const Project& project;
	const XMLElement* node;
	const Module* module;
	DirectoryLocation GetDefaultDirectoryTree ( const Module* module ) const;
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
	bool overridable;

	Define ( const Project& project,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const std::string& name_,
	         const std::string& backend_ = "" );
	~Define();
	void ProcessXML();
private:
	void Initialize();
};


class File
{
public:
	FileLocation file;
	bool first;
	std::string switches;
	bool isPreCompiledHeader;

	File ( DirectoryLocation directory,
	       const std::string& relative_path,
	       const std::string& name,
	       bool _first,
	       const std::string& _switches,
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

class Bootsector
{
public:
	const XMLElement& node;
	const Module* module;
	const Module* bootSectorModule;

	Bootsector ( const XMLElement& _node,
	             const Module* _module );

	void ProcessXML();
private:
	bool IsSupportedModuleType ( ModuleType type );
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

class ImportLibrary : public XmlNode
{
public:
	const Module* module;
	std::string dllname;
	FileLocation *source;

	ImportLibrary ( const Project& project,
	                const XMLElement& node,
	                const Module* module );
	~ImportLibrary ();
};


class CompilerFlag
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string flag;
	CompilerType compiler;

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
	const XMLElement& node;
	const Module& module;
	const FileLocation *file;

	LinkerScript ( const XMLElement& node,
	               const Module& module,
	               const FileLocation *file );
	~LinkerScript ();
	void ProcessXML();
};


class Property
{
public:
	const Project& project;
	const Module* module;
	std::string name, value;
	bool isInternal;

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


class SourceFile
{
public:
	SourceFile ( AutomaticDependency* automaticDependency,
	             const Module& module,
	             const File& file,
	             SourceFile* parent );
	void Parse ();
	std::vector<SourceFile*> files; /* List of files included in this file */
	const File& file;
	AutomaticDependency* automaticDependency;
	const Module& module;
	std::vector<SourceFile*> parents; /* List of files, this file is included from */
	time_t lastWriteTime;
	time_t youngestLastWriteTime; /* Youngest last write time of this file and all children */
	SourceFile* youngestFile;
private:
	void Close ();
	void Open ();
	void SkipWhitespace ();
	bool ReadInclude ( std::string& filename,
	                   bool& searchCurrentDirectory,
	                   bool& includeNext );
	bool IsIncludedFrom ( const File& file );
	SourceFile* ParseFile(const File& file);
	bool CanProcessFile ( const File& file );
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
	bool LocateIncludedFile ( const FileLocation& directory,
	                          const std::string& includedFilename );
	bool LocateIncludedFile ( SourceFile* sourceFile,
	                          const Module& module,
	                          const std::string& includedFilename,
	                          bool searchCurrentDirectory,
	                          bool includeNext,
	                          File& resolvedFile );
	SourceFile* RetrieveFromCacheOrParse ( const Module& module,
	                                       const File& file,
	                                       SourceFile* parentSourceFile );
	SourceFile* RetrieveFromCache ( const File& file );
	void CheckAutomaticDependencies ( bool verbose );
	void CheckAutomaticDependenciesForModule ( Module& module,
	                                           bool verbose );
private:
	void GetModulesToCheck ( Module& module, std::vector<const Module*>& modules );
	void CheckAutomaticDependencies ( const Module& module,
	                                  bool verbose );
	void CheckAutomaticDependenciesForFile ( SourceFile* sourceFile );
	void GetIncludeDirectories ( std::vector<Include*>& includes,
	                             const Module& module );
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
	static std::string ReplaceVariable ( const std::string& name,
	                                     const std::string& value,
	                                     std::string path );
};


class CDFile : public XmlNode
{
public:
	FileLocation *source;
	FileLocation *target;

	CDFile ( const Project& project,
	         const XMLElement& bootstrapNode,
	         const std::string& path );
	~CDFile ();
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
	~InstallFile ();
};


class PchFile
{
public:
	const XMLElement& node;
	const Module& module;
	const FileLocation *file;

	PchFile (
		const XMLElement& node,
		const Module& module,
		const FileLocation *file );
	~PchFile();
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
	std::string name;

	CompilationUnit ( const File* file );
	CompilationUnit ( const Project* project,
	                  const Module* module,
	                  const XMLElement* node );
	~CompilationUnit ();
	void ProcessXML();
	bool IsGeneratedFile () const;
	bool HasFileWithExtension ( const std::string& extension ) const;
	bool IsFirstFile () const;
	const FileLocation& GetFilename () const;
	const std::string& GetSwitches () const;
	void AddFile ( const File * file );
	const std::vector<const File*> GetFiles () const;
private:
	const Project* project;
	const Module* module;
	const XMLElement* node;
	std::vector<const File*> files;
	FileLocation *default_name;
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
GetExtension ( const FileLocation& file );

extern std::string
NormalizeFilename ( const std::string& filename );

extern std::string
ToLower ( std::string filename );

#endif /* __RBUILD_H */
