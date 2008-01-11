
#ifndef __DMO_H__
#define __DMO_H__

#include "mediaerr.h"

#ifdef FIX_LOCK_NAME
  #define Lock DMOLock
#endif

#include "mediaobj.h"

#ifdef FIX_LOCK_NAME
  #undef Lock
#endif

#include "dmoreg.h"
#include "dmort.h"

