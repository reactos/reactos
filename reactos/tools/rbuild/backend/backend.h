#ifndef __BACKEND_H
#define __BACKEND_H

#include "../rbuild.h"

class Backend;

typedef Backend* BackendFactory ( Project& project );

class Backend
{
	class Factory
	{
	public:
		std::string name;
		BackendFactory* factory;

		Factory ( const std::string& name_, BackendFactory* factory_ )
			: name(name_), factory(factory_)
		{
		}
	};

	static std::vector<Factory*> factories;

public:
	static void InitFactories();
	static Backend* Create ( const std::string& name, Project& project );

protected:
	Backend ( Project& project );

public:
	virtual void Process () = 0;

protected:
	Project& ProjectNode;
};

#endif /* __BACKEND_H */
