/**************************************************************************/
/*                                                                        */
/* GENERIC.H -- global include file for pscript driver                    */
/*                                                                        */
/**************************************************************************/
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <dos.h>
#include <io.h>
#include <direct.h>
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <commctrl.h>
#include <winerror.h>

#include "icmdll.h"
#include "icc.h"
#include "icm.h"
#include "icc_i386.h"
#include "csprof.h"
#include "getcrd.h"
#include "getcsa.h"
#include "icmstr.h"
#include "profcrd.h"

#define _ICMSEG  ""
#define _ICM2SEG ""
#define  ICMSEG