/* dosfsck.h  -  Interface with vfatlib.h */

/* Redefine off_t as a 'long long' */
// typedef __int64 off_t;
#define off_t __int64

#include "check/common.h"
#include "check/fsck.fat.h"
#include "check/io.h"
#include "check/boot.h"
#include "check/check.h"
#include "check/fat.h"
#include "check/file.h"
#include "check/lfn.h"
