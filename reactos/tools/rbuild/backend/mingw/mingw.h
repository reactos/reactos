#ifndef MINGW_H
#define MINGW_H

#include "../backend.h"

class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& );
};

#endif /* MINGW_H */
