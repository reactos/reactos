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
private:
	bool mkdir_p ( const char* path );
	std::string ReplaceVariable ( std::string name,
	                              std::string value,
	                              std::string path );
	std::string GetIntermediatePath ();
	std::string GetOutputPath ();
	void ResolveVariablesInPath ( char* buf,
	                              std::string path );
	bool CreateDirectory ( std::string path );
};


class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project, bool verbose );
	virtual ~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory,
	                                 Directory* directoryTree );
	bool usePipe;
	Directory* intermediateDirectory;
	Directory* outputDirectory;
private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         IfableData& data ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectories ();
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget ( const std::vector<MingwModuleHandler*>& handlers ) const;
	std::string GetBuildToolDependencies () const;
	void GenerateInitTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	void CheckAutomaticDependencies ();
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	void DetectPipeSupport ();
	void DetectPCHSupport ();
	void ProcessModules ();
	std::string GetNonModuleInstallDirectories ( const std::string& installDirectory );
	std::string GetInstallDirectories ( const std::string& installDirectory );
	void GetNonModuleInstallFiles ( std::vector<std::string>& out ) const;
	void GetInstallFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleInstallTargetFiles ( std::string installDirectory,
	                                      std::vector<std::string>& out ) const;
	void GetModuleInstallTargetFiles ( std::string installDirectory,
	                                   std::vector<std::string>& out ) const;
	void GetInstallTargetFiles ( std::string installDirectory,
	                             std::vector<std::string>& out ) const;
	void OutputInstallTarget ( const std::string& installDirectory,
	                           const std::string& sourceFilename,
	                           const std::string& targetFilename,
	                           const std::string& targetDirectory );
	void OutputNonModuleInstallTargets ( const std::string& installDirectory );
	void OutputModuleInstallTargets ( const std::string& installDirectory );
	std::string GetRegistrySourceFiles ();
	std::string GetRegistryTargetFiles ( const std::string& installDirectory );
	void OutputRegistryInstallTarget ( const std::string& installDirectory );
	void GenerateInstallTarget ();
	FILE* fMakefile;
	bool use_pch;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
