#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"
#include "modulehandler.h"

class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project );
	virtual void Process ();
private:
	void ProcessModule ( Module& module ) const;
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateProjectCFlagsMacro ( const char* assignmentOperation,
	                                  const std::vector<Include*>& includes,
	                                  const std::vector<Define*>& defines ) const;
	void GenerateGlobalCFlagsAndProperties ( const char* op,
	                                         const std::vector<Property*>& properties,
	                                         const std::vector<Include*>& includes,
	                                         const std::vector<Define*>& defines,
	                                         const std::vector<If*>& ifs ) const;
	std::string GenerateProjectLFLAGS () const;
	void GenerateDirectoryTargets () const;
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget () const;
	FILE* fMakefile;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
