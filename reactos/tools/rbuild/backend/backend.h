#ifndef __BACKEND_H
#define __BACKEND_H

#include "../rbuild.h"

class Backend;

typedef Backend* BackendFactory ( Project& project,
                                  Configuration& configuration );

class Backend
{
public:
	class Factory
	{
		static std::map<std::string,Factory*>* factories;
		static int ref;

	protected:

		Factory ( const std::string& name_ );
		virtual ~Factory();

		virtual Backend* operator() ( Project&,
		                              Configuration& configuration ) = 0;

	public:
		static Backend* Create ( const std::string& name,
		                         Project& project,
		                         Configuration& configuration );
	};

protected:
	Backend ( Project& project,
	          Configuration& configuration );

public:
	virtual void Process () = 0;
	Project& ProjectNode;
	Configuration& configuration;
};

#endif /* __BACKEND_H */
