#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"

class Directory;
class MingwModuleHandler;

class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project );
	virtual ~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory, bool out );
private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         IfableData& data ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectoryTargets ();
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget ( const std::vector<MingwModuleHandler*>& handlers ) const;
	std::string GetBuildToolDependencies () const;
	void GenerateInitTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	void CheckAutomaticDependencies ();
	bool IncludeDirectoryTarget ( const std::string& directory ) const;
	void DetectPCHSupport();
	FILE* fMakefile;
	bool use_pch;
	Directory *int_directories, *out_directories;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
