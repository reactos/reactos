extern "C" {

#include <ntddk.h>

};
#include "stddef.h"
#include "stdarg.h"

//#include "inc\CrossNt.h"

#include "atapi.h"               // includes scsi.h
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "bsmaster.h"
#include "uniata_ver.h"
#include "id_sata.h"

#ifndef UNIATA_CORE

#include "id_queue.h"

#endif //UNIATA_CORE

#include "badblock.h"

