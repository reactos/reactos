
#if DBG
#define DEBUG 1
#endif

#define NT 1
#define _PNP_POWER  1
#define SECFLTR 1

#include <ntverp.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Prevent hal.h, included in ntos.h from overriding _BUS_DATA_TYPE
// enum found in ntioapi.h, included from nt.h.
//
#define _HAL_
#include <ntos.h>

#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#include "mountmgr.h"
#include "mountdev.h"
#include "mntmgr.h"
