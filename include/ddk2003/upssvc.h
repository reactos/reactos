
#ifndef _INC_UPS_DRIVER_H_
#define _INC_UPS_DRIVER_H_

#define UPS_ONLINE                  1
#define UPS_ONBATTERY               2
#define UPS_LOWBATTERY              4
#define UPS_NOCOMM                  8
#define UPS_CRITICAL                16
#define UPS_INITUNKNOWNERROR        0
#define UPS_INITOK                  1
#define UPS_INITNOSUCHDRIVER        2
#define UPS_INITBADINTERFACE        3
#define UPS_INITREGISTRYERROR       4
#define UPS_INITCOMMOPENERROR       5
#define UPS_INITCOMMSETUPERROR      6

void 
UPSCancelWait(void);

DWORD 
UPSGetState(void);

DWORD 
UPSInit(void);

void 
UPSTurnOff(DWORD aTurnOffDelay);

void 
UPSWaitForStateChange(DWORD aCurrentState, DWORD anInterval);

#endif
