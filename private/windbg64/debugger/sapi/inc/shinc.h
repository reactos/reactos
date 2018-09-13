#include <stdio.h>															
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <io.h>
#include <dos.h>
#include <malloc.h>

#ifdef MTHREAD
#define _FAR_ __far
#else
#define _FAR_
#endif

#if !defined(UNALIGNED)
#if defined(_MIPS_) || defined(_ALPHA_)     
#define UNALIGNED __unaligned               
#else                                       
#define UNALIGNED                           
#endif                                      
#endif                                      

#ifdef _MBCS
#include <mbstring.h>
#else // _MBCS
#include <string.h>
#endif // _MBCS

#include <tchar.h>

#include <memory.h>
#include <stdarg.h>
#include <fcntl.h>

#include "vcbudefs.h"
// we must get PFO_DATA
#undef NOIMAGE
#include "windows.h"
#include "pdb.h"
#include "types.h"
#include "cvtypes.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "shapi.h"
#include "sapi.h"
#include "cvproto.h"
#include "shiproto.h"
#include "shassert.h"

typedef REGREL32 FAR *LPREGREL32;

