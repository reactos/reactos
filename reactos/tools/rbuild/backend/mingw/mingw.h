#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"
#include "modulehandler.h"

class MingwModuleHandlerList : public std::vector<MingwModuleHandler*>
{
public:
	MingwModuleHandlerList()
	{
	}
	~MingwModuleHandlerList()
	{
		for ( size_t i = 0; i < size(); i++ )
		{
			delete (*this)[i];
		}
	}
private:
	// disable copy semantics
	MingwModuleHandlerList ( const MingwModuleHandlerList& );
	MingwModuleHandlerList& operator = ( const MingwModuleHandlerList& );
};


class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project );
	virtual void Process ();
private:
	void ProcessModule ( Module& module );
	void GetModuleHandlers ( MingwModuleHandlerList& moduleHandlers ) const;
	void CreateMakefile ();
	void CloseMakefile ();
	void GenerateHeader ();
	void GenerateGlobalVariables ();
	void GenerateAllTarget ();
	FILE* fMakefile;
};

std::string FixupTargetFilename ( const std::string& targetFilename );

#endif /* MINGW_H */
