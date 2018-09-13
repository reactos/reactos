// The  Telephony  API  is jointly copyrighted by Intel and Microsoft.  You are
// granted  a royalty free worldwide, unlimited license to make copies, and use
// the   API/SPI  for  making  applications/drivers  that  interface  with  the
// specification provided that this paragraph and the Intel/Microsoft copyright
// statement is maintained as is in the text and source code files.
//
// Copyright 1994 Microsoft, all rights reserved.
// Portions copyright 1992, 1993 Intel/Microsoft, all rights reserved.

#ifndef TSPI_H
#define TSPI_H


#include <windows.h>

#include "tapi.h"

// tspi.h  is  only  of  use  in  conjunction  with tapi.h.  Very few types are
// defined  in  tspi.h.   Most  types of procedure formal parameters are simply
// passed through from corresponding procedures in tapi.h.  A working knowledge
// of the TAPI interface is required for an understanding of this interface.

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef DECLARE_OPAQUE32
#define DECLARE_OPAQUE32(name)  struct name##__ { int unused; }; \
				typedef const struct name##__ FAR* name
#endif  // DECLARE_OPAQUE32

#ifndef TSPIAPI
#define TSPIAPI __export __far __pascal
#endif

DECLARE_OPAQUE32(HDRVCALL);
DECLARE_OPAQUE32(HDRVLINE);
DECLARE_OPAQUE32(HDRVPHONE);

typedef HDRVCALL FAR * LPHDRVCALL;
typedef HDRVLINE FAR * LPHDRVLINE;
typedef HDRVPHONE FAR * LPHDRVPHONE;

DECLARE_OPAQUE32(HTAPICALL);
DECLARE_OPAQUE32(HTAPILINE);
DECLARE_OPAQUE32(HTAPIPHONE);

typedef HTAPICALL FAR * LPHTAPICALL;
typedef HTAPILINE FAR * LPHTAPILINE;
typedef HTAPIPHONE FAR * LPHTAPIPHONE;



typedef void (CALLBACK * LINEEVENT) (
    HTAPILINE   htLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3);

typedef void (CALLBACK * PHONEEVENT) (
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    DWORD       dwParam1,
    DWORD       dwParam2,
    DWORD       dwParam3);


#define TSPI_MESSAGE_BASE 500
    // The lowest-numbered TSPI-specific message ID number

#define LINE_NEWCALL                                            ((long) TSPI_MESSAGE_BASE + 0)
#define LINE_CALLDEVSPECIFIC                    ((long) TSPI_MESSAGE_BASE + 1)
#define LINE_CALLDEVSPECIFICFEATURE     ((long) TSPI_MESSAGE_BASE + 2)

#define INITIALIZE_NEGOTIATION 0xFFFFFFFFL


typedef DWORD DRV_REQUESTID;

typedef void (CALLBACK * ASYNC_COMPLETION) (
    DRV_REQUESTID  dwRequestID,
    LONG           lResult);



// TSPIAPI TSPI_line functions
// ----------------------------------------------------------------------------
    
LONG TSPIAPI TSPI_lineAccept(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineAddToConference(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdConfCall,
    HDRVCALL       hdConsultCall);

LONG TSPIAPI TSPI_lineAnswer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineBlindTransfer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineClose(
    HDRVLINE  hdLine);

LONG TSPIAPI TSPI_lineCloseCall(
    HDRVCALL  hdCall);

LONG TSPIAPI TSPI_lineCompleteCall(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPDWORD        lpdwCompletionID,
    DWORD          dwCompletionMode,
    DWORD          dwMessageID);

LONG TSPIAPI TSPI_lineCompleteTransfer(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    HDRVCALL       hdConsultCall,
    HTAPICALL      htConfCall,
    LPHDRVCALL     lphdConfCall,
    DWORD          dwTransferMode);

LONG TSPIAPI TSPI_lineConditionalMediaDetection(
    HDRVLINE          hdLine,
    DWORD             dwMediaModes,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineConfigDialog(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass);

LONG TSPIAPI TSPI_lineDevSpecific(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    LPVOID         lpParams,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineDevSpecificFeature(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwFeature,
    LPVOID         lpParams,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineDial(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineDrop(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineDropOnClose(
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineDropNoOwner(
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineForward(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    DWORD             bAllAddresses,
    DWORD             dwAddressID,
    LPLINEFORWARDLIST const lpForwardList,
    DWORD             dwNumRingsNoAnswer,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineGatherDigits(
    HDRVCALL       hdCall,
    DWORD          dwEndToEndID,
    DWORD          dwDigitModes,
    LPSTR          lpsDigits,
    DWORD          dwNumDigits,
    LPCSTR         lpszTerminationDigits,
    DWORD          dwFirstDigitTimeout,
    DWORD          dwInterDigitTimeout);

LONG TSPIAPI TSPI_lineGenerateDigits(
    HDRVCALL       hdCall,
    DWORD          dwEndToEndID,
    DWORD          dwDigitMode,
    LPCSTR         lpszDigits,
    DWORD          dwDuration);

LONG TSPIAPI TSPI_lineGenerateTone(
    HDRVCALL            hdCall,
    DWORD               dwEndToEndID,
    DWORD               dwToneMode,
    DWORD               dwDuration,
    DWORD               dwNumTones,
    LPLINEGENERATETONE  const lpTones);

LONG TSPIAPI TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps);

LONG TSPIAPI TSPI_lineGetAddressID(
    HDRVLINE       hdLine,
    LPDWORD        lpdwAddressID,
    DWORD          dwAddressMode,
    LPCSTR         lpsAddress,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineGetAddressStatus(
    HDRVLINE             hdLine,
    DWORD                dwAddressID,
    LPLINEADDRESSSTATUS  lpAddressStatus);

LONG TSPIAPI TSPI_lineGetCallAddressID(
    HDRVCALL  hdCall,
    LPDWORD   lpdwAddressID);

LONG TSPIAPI TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpCallInfo);

LONG TSPIAPI TSPI_lineGetCallStatus(
    HDRVCALL          hdCall,
    LPLINECALLSTATUS  lpCallStatus);

LONG TSPIAPI TSPI_lineGetDevCaps(
    DWORD          dwDeviceID,
    DWORD          dwTSPIVersion,
    DWORD          dwExtVersion,
    LPLINEDEVCAPS  lpLineDevCaps);

LONG TSPIAPI TSPI_lineGetDevConfig(
	DWORD dwDeviceID,
	LPVARSTRING lpDeviceConfig,
	LPCSTR lpszDeviceClass);

LONG TSPIAPI TSPI_lineGetExtensionID(
    DWORD              dwDeviceID,
    DWORD              dwTSPIVersion,
    LPLINEEXTENSIONID  lpExtensionID);

LONG TSPIAPI TSPI_lineGetIcon(
    DWORD    dwDeviceID,
    LPCSTR   lpszDeviceClass,
    LPHICON  lphIcon);

LONG TSPIAPI TSPI_lineGetID(
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    DWORD          dwSelect,
    LPVARSTRING    lpDeviceID,
    LPCSTR         lpszDeviceClass);

LONG TSPIAPI TSPI_lineGetLineDevStatus(
    HDRVLINE         hdLine,
    LPLINEDEVSTATUS  lpLineDevStatus);

LONG TSPIAPI TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs);

LONG TSPIAPI TSPI_lineHold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineMakeCall(
    DRV_REQUESTID     dwRequestID,
    HDRVLINE          hdLine,
    HTAPICALL         htCall,
    LPHDRVCALL        lphdCall,
    LPCSTR            lpszDestAddress,
    DWORD             dwCountryCode,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineMonitorDigits(
    HDRVCALL       hdCall,
    DWORD          dwDigitModes);

LONG TSPIAPI TSPI_lineMonitorMedia(
    HDRVCALL       hdCall,
    DWORD          dwMediaModes);

LONG TSPIAPI TSPI_lineMonitorTones(
    HDRVCALL           hdCall,
    DWORD              dwToneListID,
    LPLINEMONITORTONE  const lpToneList,
    DWORD              dwNumEntries);

LONG TSPIAPI TSPI_lineNegotiateExtVersion(
    DWORD    dwDeviceID,
    DWORD    dwTSPIVersion,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwExtVersion);

LONG TSPIAPI TSPI_lineNegotiateTSPIVersion(
    DWORD    dwDeviceID,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwTSPIVersion);

LONG TSPIAPI TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   htLine,
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc);

LONG TSPIAPI TSPI_linePark(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    DWORD          dwParkMode,
    LPCSTR         lpszDirAddress,
    LPVARSTRING    lpNonDirAddress);

LONG TSPIAPI TSPI_linePickup(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HTAPICALL      htCall,
    LPHDRVCALL     lphdCall,
    LPCSTR         lpszDestAddress,
    LPCSTR         lpszGroupID);

LONG TSPIAPI TSPI_linePrepareAddToConference(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdConfCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineRedirect(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpszDestAddress,
    DWORD          dwCountryCode);

LONG TSPIAPI TSPI_lineRemoveFromConference(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineSecureCall(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineSelectExtVersion(
    HDRVLINE  hdLine,
    DWORD     dwExtVersion);

LONG TSPIAPI TSPI_lineSendUserUserInfo(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall,
    LPCSTR         lpsUserUserInfo,
    DWORD          dwSize);

LONG TSPIAPI TSPI_lineSetAppSpecific(
    HDRVCALL       hdCall,
    DWORD          dwAppSpecific);

LONG TSPIAPI TSPI_lineSetCallParams(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    DWORD             dwBearerMode,
    DWORD             dwMinRate,
    DWORD             dwMaxRate,
    LPLINEDIALPARAMS  const lpDialParams);

LONG TSPIAPI TSPI_lineSetDefaultMediaDetection(
    HDRVLINE       hdLine,
    DWORD          dwMediaModes);

LONG TSPIAPI TSPI_lineSetDevConfig(
    DWORD                       dwDeviceID,
    LPVOID                      const lpDeviceConfig,
    DWORD                       dwSize,
    LPCSTR                      lpszDeviceClass);

LONG TSPIAPI TSPI_lineSetMediaControl(
    HDRVLINE                     hdLine,
    DWORD                        dwAddressID,
    HDRVCALL                     hdCall,
    DWORD                        dwSelect,
    LPLINEMEDIACONTROLDIGIT      const lpDigitList,
    DWORD                        dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA      const lpMediaList,
    DWORD                        dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE       const lpToneList,
    DWORD                        dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE  const lpCallStateList,
    DWORD                        dwCallStateNumEntries);

LONG TSPIAPI TSPI_lineSetMediaMode(
    HDRVCALL       hdCall,
    DWORD          dwMediaMode);

LONG TSPIAPI TSPI_lineSetStatusMessages(
    HDRVLINE       hdLine,
    DWORD          dwLineStates,
    DWORD          dwAddressStates);

LONG TSPIAPI TSPI_lineSetTerminal(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HDRVCALL       hdCall,
    DWORD          dwSelect,
    DWORD          dwTerminalModes,
    DWORD          dwTerminalID,
    DWORD           bEnable);

LONG TSPIAPI TSPI_lineSetupConference(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    HDRVLINE          hdLine,
    HTAPICALL         htConfCall,
    LPHDRVCALL        lphdConfCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    DWORD             dwNumParties,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineSetupTransfer(
    DRV_REQUESTID     dwRequestID,
    HDRVCALL          hdCall,
    HTAPICALL         htConsultCall,
    LPHDRVCALL        lphdConsultCall,
    LPLINECALLPARAMS  const lpCallParams);

LONG TSPIAPI TSPI_lineSwapHold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdActiveCall,
    HDRVCALL       hdHeldCall);

LONG TSPIAPI TSPI_lineUncompleteCall(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwCompletionID);

LONG TSPIAPI TSPI_lineUnhold(
    DRV_REQUESTID  dwRequestID,
    HDRVCALL       hdCall);

LONG TSPIAPI TSPI_lineUnpark(
    DRV_REQUESTID  dwRequestID,
    HDRVLINE       hdLine,
    DWORD          dwAddressID,
    HTAPICALL      htCall,
    LPHDRVCALL     lphdCall,
    LPCSTR         lpszDestAddress);



// TSPIAPI TSPI_phone functions
// ----------------------------------------------------------------------------

LONG TSPIAPI TSPI_phoneClose(
    HDRVPHONE  hdPhone);

LONG TSPIAPI TSPI_phoneConfigDialog(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCSTR  lpszDeviceClass);

LONG TSPIAPI TSPI_phoneDevSpecific(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    LPVOID         lpParams,
    DWORD          dwSize);

LONG TSPIAPI TSPI_phoneGetButtonInfo(
    HDRVPHONE          hdPhone,
    DWORD              dwButtonLampID,
    LPPHONEBUTTONINFO  lpButtonInfo);

LONG TSPIAPI TSPI_phoneGetData(
    HDRVPHONE      hdPhone,
    DWORD          dwDataID,
    LPVOID         lpData,
    DWORD          dwSize);

LONG TSPIAPI TSPI_phoneGetDevCaps(
    DWORD          dwDeviceID,
    DWORD          dwTSPIVersion,
    DWORD          dwExtVersion,
    LPPHONECAPS    lpPhoneCaps);

LONG TSPIAPI TSPI_phoneGetDisplay(
    HDRVPHONE      hdPhone,
    LPVARSTRING    lpDisplay);

LONG TSPIAPI TSPI_phoneGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID);

LONG TSPIAPI TSPI_phoneGetGain(
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    LPDWORD        lpdwGain);

LONG TSPIAPI TSPI_phoneGetHookSwitch(
    HDRVPHONE      hdPhone,
    LPDWORD        lpdwHookSwitchDevs);

LONG TSPIAPI TSPI_phoneGetIcon(
    DWORD    dwDeviceID,
    LPCSTR   lpszDeviceClass,
    LPHICON  lphIcon);

LONG TSPIAPI TSPI_phoneGetID(
    HDRVPHONE      hdPhone,
    LPVARSTRING    lpDeviceID,
    LPCSTR         lpszDeviceClass);

LONG TSPIAPI TSPI_phoneGetLamp(
    HDRVPHONE      hdPhone,
    DWORD          dwButtonLampID,
    LPDWORD        lpdwLampMode);

LONG TSPIAPI TSPI_phoneGetRing(
    HDRVPHONE      hdPhone,
    LPDWORD        lpdwRingMode,
    LPDWORD        lpdwVolume);

LONG TSPIAPI TSPI_phoneGetStatus(
    HDRVPHONE      hdPhone,
    LPPHONESTATUS  lpPhoneStatus);

LONG TSPIAPI TSPI_phoneGetVolume(
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    LPDWORD        lpdwVolume);

LONG TSPIAPI TSPI_phoneNegotiateExtVersion(
    DWORD    dwDeviceID,
    DWORD    dwTSPIVersion,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwExtVersion);

LONG TSPIAPI TSPI_phoneNegotiateTSPIVersion(
    DWORD    dwDeviceID,
    DWORD    dwLowVersion,
    DWORD    dwHighVersion,
    LPDWORD  lpdwTSPIVersion);

LONG TSPIAPI TSPI_phoneOpen(
    DWORD        dwDeviceID,
    HTAPIPHONE   htPhone,
    LPHDRVPHONE  lphdPhone,
    DWORD        dwTSPIVersion,
    PHONEEVENT   lpfnEventProc);

LONG TSPIAPI TSPI_phoneSelectExtVersion(
    HDRVPHONE  hdPhone,
    DWORD      dwExtVersion);

LONG TSPIAPI TSPI_phoneSetButtonInfo(
    DRV_REQUESTID            dwRequestID,
    HDRVPHONE                hdPhone,
    DWORD                    dwButtonLampID,
    LPPHONEBUTTONINFO  const lpButtonInfo);

LONG TSPIAPI TSPI_phoneSetData(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwDataID,
    LPVOID         const lpData,
    DWORD          dwSize);

LONG TSPIAPI TSPI_phoneSetDisplay(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwRow,
    DWORD          dwColumn,
    LPCSTR         lpsDisplay,
    DWORD          dwSize);

LONG TSPIAPI TSPI_phoneSetGain(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    DWORD          dwGain);

LONG TSPIAPI TSPI_phoneSetHookSwitch(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDevs,
    DWORD          dwHookSwitchMode);

LONG TSPIAPI TSPI_phoneSetLamp(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwButtonLampID,
    DWORD          dwLampMode);

LONG TSPIAPI TSPI_phoneSetRing(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwRingMode,
    DWORD          dwVolume);

LONG TSPIAPI TSPI_phoneSetStatusMessages(
    HDRVPHONE      hdPhone,
    DWORD          dwPhoneStates,
    DWORD          dwButtonModes,
    DWORD          dwButtonStates);

LONG TSPIAPI TSPI_phoneSetVolume(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    DWORD          dwVolume);



// TSPIAPI TSPI_provider functions
// ----------------------------------------------------------------------------

LONG TSPIAPI TSPI_providerConfig(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID);

LONG TSPIAPI TSPI_providerInit(
    DWORD             dwTSPIVersion,
    DWORD             dwPermanentProviderID,
    DWORD             dwLineDeviceIDBase,
    DWORD             dwPhoneDeviceIDBase,
    DWORD             dwNumLines,
    DWORD             dwNumPhones,
    ASYNC_COMPLETION  lpfnCompletionProc);

LONG TSPIAPI TSPI_providerInstall(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID);

LONG TSPIAPI TSPI_providerRemove(
    HWND   hwndOwner,
    DWORD  dwPermanentProviderID);

LONG TSPIAPI TSPI_providerShutdown(
    DWORD    dwTSPIVersion);

LONG TSPIAPI TSPI_providerEnumDevices(
    DWORD    dwPermanentProviderID,
    LPDWORD  lpdwNumLines,
    LPDWORD  lpdwNumPhones);


// The following macros are the ordinal numbers of the exported tspi functions

#define TSPI_PROC_BASE            500

#define TSPI_LINEACCEPT                    (TSPI_PROC_BASE + 0)
#define TSPI_LINEADDTOCONFERENCE           (TSPI_PROC_BASE + 1)
#define TSPI_LINEANSWER                    (TSPI_PROC_BASE + 2)
#define TSPI_LINEBLINDTRANSFER             (TSPI_PROC_BASE + 3)
#define TSPI_LINECLOSE                     (TSPI_PROC_BASE + 4)
#define TSPI_LINECLOSECALL                 (TSPI_PROC_BASE + 5)
#define TSPI_LINECOMPLETECALL              (TSPI_PROC_BASE + 6)
#define TSPI_LINECOMPLETETRANSFER          (TSPI_PROC_BASE + 7)
#define TSPI_LINECONDITIONALMEDIADETECTION (TSPI_PROC_BASE + 8)
#define TSPI_LINECONFIGDIALOG              (TSPI_PROC_BASE + 9)
#define TSPI_LINEDEVSPECIFIC               (TSPI_PROC_BASE + 10)
#define TSPI_LINEDEVSPECIFICFEATURE        (TSPI_PROC_BASE + 11)
#define TSPI_LINEDIAL                      (TSPI_PROC_BASE + 12)
#define TSPI_LINEDROP                      (TSPI_PROC_BASE + 13)
#define TSPI_LINEFORWARD                   (TSPI_PROC_BASE + 14)
#define TSPI_LINEGATHERDIGITS              (TSPI_PROC_BASE + 15)
#define TSPI_LINEGENERATEDIGITS            (TSPI_PROC_BASE + 16)
#define TSPI_LINEGENERATETONE              (TSPI_PROC_BASE + 17)
#define TSPI_LINEGETADDRESSCAPS            (TSPI_PROC_BASE + 18)
#define TSPI_LINEGETADDRESSID              (TSPI_PROC_BASE + 19)
#define TSPI_LINEGETADDRESSSTATUS          (TSPI_PROC_BASE + 20)
#define TSPI_LINEGETCALLADDRESSID          (TSPI_PROC_BASE + 21)
#define TSPI_LINEGETCALLINFO               (TSPI_PROC_BASE + 22)
#define TSPI_LINEGETCALLSTATUS             (TSPI_PROC_BASE + 23)
#define TSPI_LINEGETDEVCAPS                (TSPI_PROC_BASE + 24)
#define TSPI_LINEGETDEVCONFIG              (TSPI_PROC_BASE + 25)
#define TSPI_LINEGETEXTENSIONID            (TSPI_PROC_BASE + 26)
#define TSPI_LINEGETICON                   (TSPI_PROC_BASE + 27)
#define TSPI_LINEGETID                     (TSPI_PROC_BASE + 28)
#define TSPI_LINEGETLINEDEVSTATUS          (TSPI_PROC_BASE + 29)
#define TSPI_LINEGETNUMADDRESSIDS          (TSPI_PROC_BASE + 30)
#define TSPI_LINEHOLD                      (TSPI_PROC_BASE + 31)
#define TSPI_LINEMAKECALL                  (TSPI_PROC_BASE + 32)
#define TSPI_LINEMONITORDIGITS             (TSPI_PROC_BASE + 33)
#define TSPI_LINEMONITORMEDIA              (TSPI_PROC_BASE + 34)
#define TSPI_LINEMONITORTONES              (TSPI_PROC_BASE + 35)
#define TSPI_LINENEGOTIATEEXTVERSION       (TSPI_PROC_BASE + 36)
#define TSPI_LINENEGOTIATETSPIVERSION      (TSPI_PROC_BASE + 37)
#define TSPI_LINEOPEN                      (TSPI_PROC_BASE + 38)
#define TSPI_LINEPARK                      (TSPI_PROC_BASE + 39)
#define TSPI_LINEPICKUP                    (TSPI_PROC_BASE + 40)
#define TSPI_LINEPREPAREADDTOCONFERENCE    (TSPI_PROC_BASE + 41)
#define TSPI_LINEREDIRECT                  (TSPI_PROC_BASE + 42)
#define TSPI_LINEREMOVEFROMCONFERENCE      (TSPI_PROC_BASE + 43)
#define TSPI_LINESECURECALL                (TSPI_PROC_BASE + 44)
#define TSPI_LINESELECTEXTVERSION          (TSPI_PROC_BASE + 45)
#define TSPI_LINESENDUSERUSERINFO          (TSPI_PROC_BASE + 46)
#define TSPI_LINESETAPPSPECIFIC            (TSPI_PROC_BASE + 47)
#define TSPI_LINESETCALLPARAMS             (TSPI_PROC_BASE + 48)
#define TSPI_LINESETDEFAULTMEDIADETECTION  (TSPI_PROC_BASE + 49)
#define TSPI_LINESETDEVCONFIG              (TSPI_PROC_BASE + 50)
#define TSPI_LINESETMEDIACONTROL           (TSPI_PROC_BASE + 51)
#define TSPI_LINESETMEDIAMODE              (TSPI_PROC_BASE + 52)
#define TSPI_LINESETSTATUSMESSAGES         (TSPI_PROC_BASE + 53)
#define TSPI_LINESETTERMINAL               (TSPI_PROC_BASE + 54)
#define TSPI_LINESETUPCONFERENCE           (TSPI_PROC_BASE + 55)
#define TSPI_LINESETUPTRANSFER             (TSPI_PROC_BASE + 56)
#define TSPI_LINESWAPHOLD                  (TSPI_PROC_BASE + 57)
#define TSPI_LINEUNCOMPLETECALL            (TSPI_PROC_BASE + 58)
#define TSPI_LINEUNHOLD                    (TSPI_PROC_BASE + 59)
#define TSPI_LINEUNPARK                    (TSPI_PROC_BASE + 60)
#define TSPI_PHONECLOSE                    (TSPI_PROC_BASE + 61)
#define TSPI_PHONECONFIGDIALOG             (TSPI_PROC_BASE + 62)
#define TSPI_PHONEDEVSPECIFIC              (TSPI_PROC_BASE + 63)
#define TSPI_PHONEGETBUTTONINFO            (TSPI_PROC_BASE + 64)
#define TSPI_PHONEGETDATA                  (TSPI_PROC_BASE + 65)
#define TSPI_PHONEGETDEVCAPS               (TSPI_PROC_BASE + 66)
#define TSPI_PHONEGETDISPLAY               (TSPI_PROC_BASE + 67)
#define TSPI_PHONEGETEXTENSIONID           (TSPI_PROC_BASE + 68)
#define TSPI_PHONEGETGAIN                  (TSPI_PROC_BASE + 69)
#define TSPI_PHONEGETHOOKSWITCH            (TSPI_PROC_BASE + 70)
#define TSPI_PHONEGETICON                  (TSPI_PROC_BASE + 71)
#define TSPI_PHONEGETID                    (TSPI_PROC_BASE + 72)
#define TSPI_PHONEGETLAMP                  (TSPI_PROC_BASE + 73)
#define TSPI_PHONEGETRING                  (TSPI_PROC_BASE + 74)
#define TSPI_PHONEGETSTATUS                (TSPI_PROC_BASE + 75)
#define TSPI_PHONEGETVOLUME                (TSPI_PROC_BASE + 76)
#define TSPI_PHONENEGOTIATEEXTVERSION      (TSPI_PROC_BASE + 77)
#define TSPI_PHONENEGOTIATETSPIVERSION     (TSPI_PROC_BASE + 78)
#define TSPI_PHONEOPEN                     (TSPI_PROC_BASE + 79)
#define TSPI_PHONESELECTEXTVERSION         (TSPI_PROC_BASE + 80)
#define TSPI_PHONESETBUTTONINFO            (TSPI_PROC_BASE + 81)
#define TSPI_PHONESETDATA                  (TSPI_PROC_BASE + 82)
#define TSPI_PHONESETDISPLAY               (TSPI_PROC_BASE + 83)
#define TSPI_PHONESETGAIN                  (TSPI_PROC_BASE + 84)
#define TSPI_PHONESETHOOKSWITCH            (TSPI_PROC_BASE + 85)
#define TSPI_PHONESETLAMP                  (TSPI_PROC_BASE + 86)
#define TSPI_PHONESETRING                  (TSPI_PROC_BASE + 87)
#define TSPI_PHONESETSTATUSMESSAGES        (TSPI_PROC_BASE + 88)
#define TSPI_PHONESETVOLUME                (TSPI_PROC_BASE + 89)
#define TSPI_PROVIDERCONFIG                (TSPI_PROC_BASE + 90)
#define TSPI_PROVIDERINIT                  (TSPI_PROC_BASE + 91)
#define TSPI_PROVIDERINSTALL               (TSPI_PROC_BASE + 92)
#define TSPI_PROVIDERREMOVE                (TSPI_PROC_BASE + 93)
#define TSPI_PROVIDERSHUTDOWN              (TSPI_PROC_BASE + 94)
#define TSPI_PROVIDERENUMDEVICES           (TSPI_PROC_BASE + 95)
#define TSPI_LINEDROPONCLOSE               (TSPI_PROC_BASE + 96)
#define TSPI_LINEDROPNOOWNER               (TSPI_PROC_BASE + 97)


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  // TSPI_H
