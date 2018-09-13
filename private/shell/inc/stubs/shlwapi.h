//
// This is a fake header that allows our shell code to refer to private
// (internal) header information without explicitly including the ?????p.h 
// file.  The Win95 header files don't split the public and private 
// information out into separate header files until much later when 
// producing the SDK files.  The Chicago shell source code uses the header 
// file before the separation process.
//
// BobDay
//
#ifndef UNIX
#include "..\..\..\..\public\sdk\inc\shlwapi.h"
#else
#include "/vobs/userx/userx/public/sdk/inc/shlwapi.h"
#endif /* UNIX */
#include <shlwapip.h>
