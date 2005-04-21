#ifndef __BACKEND_H
#define __BACKEND_H

#include "../rbuild.h"

class Backend;

typedef Backend* BackendFactory ( Project& project,
                                  bool verbose );

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
		                              bool verbose,
		                              bool cleanAsYouGo ) = 0;

	public:
		static Backend* Create ( const std::string& name,
		                         Project& project,
		                         bool verbose,
		                         bool cleanAsYouGo );
	};

protected:
	Backend ( Project& project,
	          bool verbose,
	          bool cleanAsYouGo );

public:
	virtual void Process () = 0;
	Project& ProjectNode;
	bool verbose;
	bool cleanAsYouGo;
};

#endif /* __BACKEND_H */
