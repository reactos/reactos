#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"
#include "modulehandler.h"

class MingwModuleHandlerList : public std::vector<MingwModuleHandler*>
{
public:
	~MingwModuleHandlerList()
	{
		for ( size_t i = 0; i < size(); i++ )
		{
			delete (*this)[i];
		}
	}
};


class MingwBackend : public Backend
{
public:
	static Backend* Factory ( Project& project );
protected:
	MingwBackend ( Project& project );
public:
	virtual void Process ();
private:
	void ProcessModule ( Module& module );
	void GetModuleHandlers ( MingwModuleHandlerList& moduleHandlers );
	void CreateMakefile ();
	void CloseMakefile ();
	void GenerateHeader ();
	void GenerateAllTarget ();
	FILE* fMakefile;
};

#endif /* MINGW_H */
