#ifndef PP_LPT_H
#define PP_LPT_H

#include "msports.h"

// Exported from pplpt.c
extern TCHAR m_szPortName[];

typedef struct _LPT_PROP_PARAMS
{
   HDEVINFO                     DeviceInfoSet;
   PSP_DEVINFO_DATA             DeviceInfoData;
   BOOL                         ChangesEnabled;
   GUID                         ClassGuid;          // the class GUID for this device
   DWORD                        FilterResourceMethod;
   DWORD                        ParEnableLegacyZip;
//   HKEY                         hDeviceKey;
   TCHAR                        szLptName[20];
} LPT_PROP_PARAMS, *PLPT_PROP_PARAMS;

#define RESOURCE_METHOD_DEFAULT_IDX 1 
#define FILTERMETHOD_TRYNOT     0
#define FILTERMETHOD_NEVER      1
#define FILTERMETHOD_ACCEPTANY  2

#define MAX_LPT_PORT            3   // Maximum number of LPT ports NT supports
#define MIN_LPT                 1   // Minimum new LPT port number

#define ENABLELEGACYZIPDEFAULT  0

#define IDH_NOHELP		((DWORD)-1)
#define idh_devmgr_portset_trynot	15895	// Port Settings: radio button
#define idh_devmgr_portset_never	15896	// Port Settings: radio button
#define idh_devmgr_portset_acceptany	15897	// Port Settings: radio button
#define idh_devmgr_portset_portnum	15898	// Port Settings: List box text
#define idh_devmgr_portset_LPTchoice	15899	// Port Settings: List box
#define	idh_devmgr_enable_legacy	15900

////////////////////////////////////////////////////////////////////////////////
// Port Settings Property Page Prototypes
////////////////////////////////////////////////////////////////////////////////

BOOL
APIENTRY 
ParallelPortPropPageProvider(LPVOID                 Info,
                             LPFNADDPROPSHEETPAGE   AddFunc,
                             LPARAM                 Lparam);

#endif // PP_LPT_H

