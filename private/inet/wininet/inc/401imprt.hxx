#ifndef _401IMPRT_H_
#define _401IMPRT_H_


namespace ie401
{

//  Import the visited container and history containers from IE4
BOOL Import401History();

//  Imports the content container from IE4 (stealing the cached content)
BOOL Import401Content();

}

#endif
