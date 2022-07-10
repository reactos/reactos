#define NOEXTAPI
#include <ntifs.h>
#include <halfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arc/arc.h>
#include <windbgkd.h>
#include <kddll.h>
#include <cportlib/cportlib.h>
#include <ndk/inbvfuncs.h>
#include <drivers/bootvid/bootvid.h>

#define TAG_KDBG        'GBDK'
