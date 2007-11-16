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
v2s ( const Backend* backend, const std::vector<FileLocation>& files, int wrap_at );
extern std::string
v2s ( const string_list& v, int wrap_at );


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
	std::string binutilsPrefix;
	std::string binutilsCommand;
	bool usePipe, manualBinutilsSetting;
	Directory* intermediateDirectory;
	Directory* outputDirectory;
	Directory* installDirectory;

	std::string GetFullName ( const FileLocation& file ) const;
	std::string GetFullPath ( const FileLocation& file ) const;

private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  const IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         const IfableData& data ) const;
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
    void GenerateFamiliesTargetsInfo() const;
    void GenerateInitTarget () const;
	void GenerateRegTestsRunTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	std::string GetBin2ResExecutable ();
	void UnpackWineResources ();
	void GenerateTestSupportCode ();
	void GenerateCompilationUnitSupportCode ();
	void GenerateSysSetup ();
	void GenerateModulesResources();
	void GenerateModulesManifests();
    void GenerateCreditsFile ();
	std::string GetProxyMakefileTree () const;
	void GenerateProxyMakefiles ();
	void CheckAutomaticDependencies ();
	bool TryToDetectThisCompiler ( const std::string& compiler );
	void DetectCompiler ();
	std::string GetCompilerVersion ( const std::string& compilerCommand );
	bool IsSupportedCompilerVersion ( const std::string& compilerVersion );
	bool TryToDetectThisNetwideAssembler ( const std::string& assembler );
	bool TryToDetectThisBinutils ( const std::string& binutils );
	std::string GetBinutilsVersion ( const std::string& binutilsCommand );
	std::string GetBinutilsVersionDate ( const std::string& binutilsCommand );
	bool IsSupportedBinutilsVersion ( const std::string& binutilsVersion );
	std::string GetVersionString ( const std::string& versionCommand );
	std::string GetNetwideAssemblerVersion ( const std::string& nasmCommand );
	void DetectBinutils ();
	void DetectNetwideAssembler ();
	void DetectPipeSupport ();
	void DetectPCHSupport ();
	bool CanEnablePreCompiledHeaderSupportForModule ( const Module& module );
	void ProcessModules ();
	void CheckAutomaticDependenciesForModuleOnly ();
	void ProcessNormal ();
	std::string GetNonModuleInstallDirectories ( const std::string& installDirectory );
	std::string GetInstallDirectories ( const std::string& installDirectory );
	void GetNonModuleInstallFiles ( std::vector<std::string>& out ) const;
	void GetInstallFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void GetModuleInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void GetInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void OutputInstallTarget ( const FileLocation& source, const FileLocation& target );
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
