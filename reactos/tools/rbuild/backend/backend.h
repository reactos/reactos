#ifndef __BACKEND_H
#define __BACKEND_H

#include "../rbuild.h"

class Backend
{
public:
	Backend ( Project& project );
	virtual void Process () = 0;
protected:
	Project& ProjectNode;
};

#endif /* __BACKEND_H */
