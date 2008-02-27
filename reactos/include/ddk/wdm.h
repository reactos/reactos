/*
	WDM (far from finished!)
*/

#ifndef WDM_H
#define WDM_H

typedef int CM_RESOURCE_TYPE;

#define CmResourceTypeNull		0
#define CmResourceTypePort		1
#define CmResourceTypeInterrupt		2
#define CmResourceTypeMemory		3
#define CmResourceTypeDma		4
#define CmResourceTypeDeviceSpecific	5
#define CmResourceTypeBusNumber		6
#define CmResourceTypeNonArbitrated	128
#define CmResourceTypeConfigData	128
#define CmResourceTypeDevicePrivate	129
#define CmResourceTypePcCardConfig	130
#define CmResourceTypeMfCardConfig	131

extern ULONG NtGlobalFlag;

#endif

