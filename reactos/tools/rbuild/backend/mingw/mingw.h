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
	MingwBackend ( Project& project );
	virtual void Process ();
private:
	void ProcessModule ( Module& module );
	void GetModuleHandlers ( MingwModuleHandlerList& moduleHandlers );
};

#endif /* MINGW_H */
