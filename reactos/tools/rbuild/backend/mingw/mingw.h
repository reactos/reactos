#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"
#include "modulehandler.h"

class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project );
	virtual ~MingwBackend () { };
	virtual void Process ();
private:
	void ProcessModule ( Module& module ) const;
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  IfableData& data ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         IfableData& data ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectoryTargets () const;
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget () const;
	std::string GetBuildToolDependencies () const;
	void GenerateInitTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	void CheckAutomaticDependencies ();
	void DetectPCHSupport();

	FILE* fMakefile;
	bool use_pch;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
