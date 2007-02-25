#ifndef _MCX_H
#define _MCX_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define DIALOPTION_BILLING 64
#define DIALOPTION_QUIET 128
#define DIALOPTION_DIALTONE 256
#define MDMVOLFLAG_LOW	1
#define MDMVOLFLAG_MEDIUM	2
#define MDMVOLFLAG_HIGH	4
#define MDMVOL_LOW	0
#define MDMVOL_MEDIUM	1
#define MDMVOL_HIGH	2
#define MDMSPKRFLAG_OFF	1
#define MDMSPKRFLAG_DIAL	2
#define MDMSPKRFLAG_ON	4
#define MDMSPKRFLAG_CALLSETUP	8
#define MDMSPKR_OFF	0
#define MDMSPKR_DIAL 1
#define MDMSPKR_ON	2
#define MDMSPKR_CALLSETUP	3
#define MDM_COMPRESSION	1
#define MDM_ERROR_CONTROL	2
#define MDM_FORCED_EC	4
#define MDM_CELLULAR	8
#define MDM_FLOWCONTROL_HARD	16
#define MDM_FLOWCONTROL_SOFT	32
#define MDM_CCITT_OVERRIDE	64
#define MDM_SPEED_ADJUST	128
#define MDM_TONE_DIAL	256
#define MDM_BLIND_DIAL	512
#define MDM_V23_OVERRIDE	1024
typedef struct _MODEMDEVCAPS {
	DWORD dwActualSize;
	DWORD dwRequiredSize;
	DWORD dwDevSpecificOffset;
	DWORD dwDevSpecificSize;
	DWORD dwModemProviderVersion;
	DWORD dwModemManufacturerOffset;
	DWORD dwModemManufacturerSize;
	DWORD dwModemModelOffset;
	DWORD dwModemModelSize;
	DWORD dwModemVersionOffset;
	DWORD dwModemVersionSize;
	DWORD dwDialOptions;
	DWORD dwCallSetupFailTimer;
	DWORD dwInactivityTimeout;
	DWORD dwSpeakerVolume;
	DWORD dwSpeakerMode;
	DWORD dwModemOptions;
	DWORD dwMaxDTERate;
	DWORD dwMaxDCERate;
	BYTE abVariablePortion[1];
} MODEMDEVCAPS,*PMODEMDEVCAPS,*LPMODEMDEVCAPS;
typedef struct _MODEMSETTINGS {
	DWORD dwActualSize;
	DWORD dwRequiredSize;
	DWORD dwDevSpecificOffset;
	DWORD dwDevSpecificSize;
	DWORD dwCallSetupFailTimer;
	DWORD dwInactivityTimeout;
	DWORD dwSpeakerVolume;
	DWORD dwSpeakerMode;
	DWORD dwPreferredModemOptions;
	DWORD dwNegotiatedModemOptions;
	DWORD dwNegotiatedDCERate;
	BYTE abVariablePortion[1];
} MODEMSETTINGS,*PMODEMSETTINGS,*LPMODEMSETTINGS;
#ifdef __cplusplus
}
#endif
#endif /* _MCX_H */
