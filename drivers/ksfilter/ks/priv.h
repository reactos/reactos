#pragma once

#define _KSDDK_

#include <ntifs.h>
#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include <portcls.h>
#include <ks.h>
#include <kcom.h>
#include <pseh/pseh2.h>
#include <ntndk.h>

#include "ksfunc.h"
#include "kstypes.h"
#include "ksiface.h"


#define TAG_DEVICE_HEADER 'KSDH'

#define DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PinSet,\
    PropGeneral, PropInstances, PropIntersection)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(PropInstances),\
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(PropIntersection),\
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CONSTRAINEDDATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PROPOSEDATAFORMAT(PropGeneral)\
}
