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

class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project, bool verbose );
	virtual ~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory, bool out );
	bool usePipe;
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
	FILE* fMakefile;
	bool use_pch;
	Directory *int_directories, *out_directories;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
