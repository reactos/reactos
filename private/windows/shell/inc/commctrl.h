//
// This is a fake commctrl.h that allows Chicago code that refers to private
// (internal) commctrl.w information without including comctrlp.h. The Chicago
// header files don't split the public and private information out into
// seperate header files until much later when producing the SDK files.
// The Chicago shell source code uses the header file before the seperation
// process.
//
// BobDay
//

#ifdef _NT4SHELL_BUILD_

#include "..\nt4inc\commctrl.h"
#include "..\nt4inc\comctrlp.h"
#include "..\nt4inc\shlwapi.h"
#include "..\nt4inc\shlwapip.h"

#else

#include "..\..\..\..\public\sdk\inc\commctrl.h"
#include <comctrlp.h>
#include <shlwapi.h>
#include <shlwapip.h>

#endif
