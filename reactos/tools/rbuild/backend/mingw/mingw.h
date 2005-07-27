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
#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"

#ifdef WIN32
	#define NUL "NUL"
#else
	#define NUL "/dev/null"
#endif

class Directory;
class MingwModuleHandler;

extern std::string
v2s ( const string_list& v, int wrap_at );

typedef std::map<std::string,Directory*> directory_map;


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
	std::string ReplaceVariable ( std::string name,
	                              std::string value,
	                              std::string path );
	std::string GetEnvironmentVariable ( const std::string& name );
	void ResolveVariablesInPath ( char* buf,
	                              std::string path );
	bool CreateDirectory ( std::string path );
};


class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project,
	               Configuration& configuration );
	~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory,
	                                 Directory* directoryTree );
	const Module& GetAliasedModuleOrModule ( const Module& module ) const;
	std::string compilerPrefix;
	std::string compilerCommand;
	std::string nasmCommand;
	bool usePipe;
	Directory* intermediateDirectory;
	Directory* outputDirectory;
	Directory* installDirectory;
private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	std::string GenerateIncludesAndDefines ( IfableData& data ) const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         IfableData& data ) const;
	void GenerateProjectGccOptionsMacro ( const char* assignmentOperation,
                                              IfableData& data ) const;
	void GenerateProjectGccOptions ( const char* assignmentOperation,
	                                 IfableData& data ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectories ();
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget ( const std::vector<MingwModuleHandler*>& handlers ) const;
	std::string GetBuildToolDependencies () const;
	void GenerateInitTarget () const;
	void GenerateRegTestsRunTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	std::string GetBin2ResExecutable ();
	void UnpackWineResources ();
	void GenerateTestSupportCode ();
	std::string GetProxyMakefileTree () const;
	void GenerateProxyMakefiles ();
	void CheckAutomaticDependencies ();
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	bool TryToDetectThisCompiler ( const std::string& compiler );
	void DetectCompiler ();
	bool TryToDetectThisNetwideAssembler ( const std::string& assembler );
	void DetectNetwideAssembler ();
	void DetectPipeSupport ();
	void DetectPCHSupport ();
	void ProcessModules ();
	void CheckAutomaticDependenciesForModuleOnly ();
	void ProcessNormal ();
	std::string GetNonModuleInstallDirectories ( const std::string& installDirectory );
	std::string GetInstallDirectories ( const std::string& installDirectory );
	void GetNonModuleInstallFiles ( std::vector<std::string>& out ) const;
	void GetInstallFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleInstallTargetFiles ( std::vector<std::string>& out ) const;
	void GetModuleInstallTargetFiles ( std::vector<std::string>& out ) const;
	void GetInstallTargetFiles ( std::vector<std::string>& out ) const;
	void OutputInstallTarget ( const std::string& sourceFilename,
	                           const std::string& targetFilename,
	                           const std::string& targetDirectory );
	void OutputNonModuleInstallTargets ();
	void OutputModuleInstallTargets ();
	std::string GetRegistrySourceFiles ();
	std::string GetRegistryTargetFiles ();
	void OutputRegistryInstallTarget ();
	void GenerateInstallTarget ();
	void GetModuleTestTargets ( std::vector<std::string>& out ) const;
	void GenerateTestTarget ();
	void GenerateDirectoryTargets ();
	FILE* fMakefile;
	bool use_pch;
};


class ProxyMakefile
{
public:
	ProxyMakefile ( const Project& project );
	~ProxyMakefile ();
	void GenerateProxyMakefiles ( bool verbose,
                                      std::string outputTree );
private:
	std::string GeneratePathToParentDirectory ( int numberOfParentDirectories );
	std::string GetPathToTopDirectory ( Module& module );
	bool GenerateProxyMakefile ( Module& module );
	void GenerateProxyMakefileForModule ( Module& module,
                                              bool verbose,
                                              std::string outputTree );
	const Project& project;
};

#endif /* MINGW_H */
