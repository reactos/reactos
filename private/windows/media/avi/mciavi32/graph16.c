/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   graphic.c - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/

#include <win32.h>
#include <mmddk.h>
#include <vfw.h>
#include "common.h"
#include "digitalv.h"

#include "mciavi.h"

HANDLE ghModule;
typedef DWORD NPMCIGRAPHIC;

#define MCIAVI_PRODUCTNAME       2
#define MCIAVI_VERSION           3

BOOL FAR PASCAL  GraphicInit (void);
BOOL NEAR PASCAL  GraphicWindowInit (void);

void  PASCAL  GraphicFree (void);
DWORD PASCAL  GraphicDrvOpen (LPMCI_OPEN_DRIVER_PARMS lpParms);
void  FAR PASCAL  GraphicDelayedNotify (NPMCIGRAPHIC npMCI, UINT wStatus);
void FAR PASCAL GraphicImmediateNotify (UINT wDevID,
    LPMCI_GENERIC_PARMS lpParms,
    DWORD dwFlags, DWORD dwErr);
DWORD PASCAL  GraphicClose(NPMCIGRAPHIC npMCI);
DWORD PASCAL GraphicOpen (NPMCIGRAPHIC FAR * lpnpMCI, DWORD dwFlags,
    LPMCI_DGV_OPEN_PARMS lpOpen, UINT wDeviceID);
DWORD FAR PASCAL GraphicInfo(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_INFO_PARMS lpInfo);
DWORD FAR PASCAL GraphicPlay (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_PLAY_PARMS lpPlay );
DWORD FAR PASCAL GraphicCue(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_CUE_PARMS lpCue);
DWORD FAR PASCAL GraphicStep (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_STEP_PARMS lpStep);
DWORD FAR PASCAL GraphicStop (NPMCIGRAPHIC npMCI, DWORD dwFlags,
					LPMCI_GENERIC_PARMS lpParms);
DWORD FAR PASCAL GraphicSeek (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_SEEK_PARMS lpSeek);
DWORD FAR PASCAL GraphicPause(NPMCIGRAPHIC npMCI, DWORD dwFlags,
					LPMCI_GENERIC_PARMS lpParms);
DWORD FAR PASCAL GraphicStatus (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_STATUS_PARMS lpStatus);
DWORD FAR PASCAL GraphicSet (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SET_PARMS lpSet);
DWORD FAR PASCAL GraphicResume (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);
DWORD FAR PASCAL GraphicRealize(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD FAR PASCAL GraphicUpdate(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_UPDATE_PARMS lpParms);
DWORD FAR PASCAL GraphicWindow (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_WINDOW_PARMS lpWindow);
DWORD FAR PASCAL GraphicConfig(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD FAR PASCAL GraphicSetAudio (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SETAUDIO_PARMS lpSet);
DWORD FAR PASCAL GraphicSetVideo (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SETVIDEO_PARMS lpSet);
DWORD FAR PASCAL GraphicSignal(NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SIGNAL_PARMS lpSignal);

DWORD FAR PASCAL GraphicWhere(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_RECT_PARMS lpParms);
DWORD FAR PASCAL GraphicPut ( NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms);
BOOL FAR PASCAL ConfigDialog(HWND hwnd, NPMCIGRAPHIC npMCI);



DWORD PASCAL mciDriverEntry(UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

void  CheckWindowMove(NPMCIGRAPHIC npMCI, BOOL fForce);


/* statics */
static INT              swCommandTable = -1;

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicDrvOpen | This function is called when the DriverProc
 *      gets a DRV_OPEN message. This happens each time that a new movie
 *      is opened thru MCI.
 *
 * @parm LPMCI_OPEN_DRIVER_PARMS | lpOpen | Far pointer to the standard
 *      MCI open parameters
 *
 * @rdesc Returns the mci device id. The installable driver interface will
 *      pass this ID to the DriverProc in the dwDriverID parameter on all
 *      subsequent messages. To fail the open, return 0L.
 *
 ***************************************************************************/

DWORD PASCAL GraphicDrvOpen(LPMCI_OPEN_DRIVER_PARMS lpOpen)
{
    /* Specify the custom command table and the device type  */

    lpOpen->wCustomCommandTable = swCommandTable;
    lpOpen->wType = MCI_DEVTYPE_DIGITAL_VIDEO;

    /* Set the device ID to the MCI Device ID */

    return (DWORD) (UINT)lpOpen->wDeviceID;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | GraphicFree | This function is called when the DriverProc
 *      gets a DRV_FREE message. This happens when the drivers open count
 *      reaches 0.
 *
 ***************************************************************************/

void PASCAL GraphicFree16(void)
{
    if (swCommandTable != -1) {
	mciFreeCommandResource(swCommandTable);
	swCommandTable = -1;
    }

    GraphicFree();
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicInfo | This function returns alphanumeric information.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the info. message.
 *
 * @parm LPMCI_INFO_PARMS | lpPlay | Parameters for the info message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicInfo16(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_INFO_PARMS lpInfo)
{
    DWORD	dwRet = 0L;
    TCHAR	ch = TEXT('\0');
    BOOL	fTest = FALSE;

    if (!lpInfo->lpstrReturn)
    	return MCIERR_PARAM_OVERFLOW;

    if (dwFlags & MCI_TEST)
	fTest = TRUE;

    dwFlags &= ~(MCI_WAIT | MCI_NOTIFY | MCI_TEST);

    switch (dwFlags) {
    case 0L:
	return MCIERR_MISSING_PARAMETER;
	
        /* !!! Not returning PARAM_OVERFLOW here but I am above - lazy eh */
        LoadString(ghModule, MCIAVI_PRODUCTNAME, lpInfo->lpstrReturn,
                (UINT)lpInfo->dwRetSize);
	break;

    case MCI_INFO_VERSION:
	/* !!! Not returning PARAM_OVERFLOW here but I am above - lazy eh */
	LoadString(ghModule, MCIAVI_VERSION, lpInfo->lpstrReturn,
		(UINT)lpInfo->dwRetSize);
	break;

	case MCI_DGV_INFO_USAGE:
	    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
	    break;

    case MCI_DGV_INFO_ITEM:
	switch (lpInfo->dwItem) {	
	case MCI_DGV_INFO_AUDIO_QUALITY:
	case MCI_DGV_INFO_VIDEO_QUALITY:
	case MCI_DGV_INFO_STILL_QUALITY:
	case MCI_DGV_INFO_AUDIO_ALG:
	case MCI_DGV_INFO_VIDEO_ALG:
	case MCI_DGV_INFO_STILL_ALG:
	default:
	    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
	    break;
	}
	break;

    default:
    	dwRet = MCIERR_FLAGS_NOT_COMPATIBLE;
	break;
    }

    if (fTest && (LOWORD(dwRet) == 0)) {
	/* There is no error, but the test flag is on.  Return as little
	** as possible.
	*/
	dwRet = 0;
	if (lpInfo->dwRetSize)
	    lpInfo->lpstrReturn[0] = '\0';
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicList | This function supports the MCI_LIST command.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the List message.
 *
 * @parm LPMCI_DGV_LIST_PARMS | lpList | Parameters for the list message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicList(NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_LIST_PARMS lpList)
{
    return MCIERR_UNSUPPORTED_FUNCTION;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicGetDevCaps | This function returns  device
 *      capabilities
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the GetDevCaps message.
 *
 * @parm LPMCI_GETDEVCAPS_PARMS | lpCaps | Parameters for the GetDevCaps
 *      message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicGetDevCaps (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpCaps )
{

    DWORD dwRet = 0L;


    if (dwFlags & MCI_GETDEVCAPS_ITEM)
        {

        switch (lpCaps->dwItem)
            {
            case MCI_GETDEVCAPS_CAN_RECORD:
            case MCI_GETDEVCAPS_CAN_EJECT:
            case MCI_GETDEVCAPS_CAN_SAVE:
            case MCI_DGV_GETDEVCAPS_CAN_LOCK:
            case MCI_DGV_GETDEVCAPS_CAN_STR_IN:
            case MCI_DGV_GETDEVCAPS_CAN_FREEZE:
            case MCI_DGV_GETDEVCAPS_HAS_STILL:
		
                lpCaps->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

            case MCI_DGV_GETDEVCAPS_CAN_REVERSE:
            case MCI_GETDEVCAPS_CAN_PLAY:
            case MCI_GETDEVCAPS_HAS_AUDIO:
            case MCI_GETDEVCAPS_HAS_VIDEO:
            case MCI_GETDEVCAPS_USES_FILES:
            case MCI_GETDEVCAPS_COMPOUND_DEVICE:
            case MCI_DGV_GETDEVCAPS_PALETTES:
            case MCI_DGV_GETDEVCAPS_CAN_STRETCH:
            case MCI_DGV_GETDEVCAPS_CAN_TEST:
                lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

            case MCI_GETDEVCAPS_DEVICE_TYPE:

                lpCaps->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_DIGITAL_VIDEO,
					    MCI_DEVTYPE_DIGITAL_VIDEO);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

	    case MCI_DGV_GETDEVCAPS_MAX_WINDOWS:
	    case MCI_DGV_GETDEVCAPS_MAXIMUM_RATE:
	    case MCI_DGV_GETDEVCAPS_MINIMUM_RATE:
            default:

                dwRet = MCIERR_UNSUPPORTED_FUNCTION;
                break;
            }
        }
    else
        dwRet = MCIERR_MISSING_PARAMETER;

    if ((dwFlags & MCI_TEST) && (LOWORD(dwRet) == 0)) {
	/* There is no error, but the test flag is on.  Return as little
	** as possible.
	*/
	dwRet = 0;
	lpCaps->dwReturn = 0;
    }

    return (dwRet);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | mciSpecial | This function handles all the MCI
 *      commands that don't require instance data such as open.
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @parm DWORD | lpParms | Parameters for this message.
 *
 * @rdesc Error Constant. 0L on success
 *
 ***************************************************************************/

DWORD NEAR PASCAL mciSpecial (UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    NPMCIGRAPHIC npMCI = 0L;
    DWORD dwRet;

    /* since there in no instance block, there is no saved notification */
    /* to abort. */

    switch (wMessage) {
	case MCI_OPEN_DRIVER:
            if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
                dwRet = GraphicOpen (&npMCI, dwFlags,
			    (LPMCI_DGV_OPEN_PARMS) lpParms, wDeviceID);
            else
                dwRet = 0L;

            mciSetDriverData (wDeviceID, (UINT)npMCI);
            break;

        case MCI_GETDEVCAPS:
            dwRet = GraphicGetDevCaps(NULL, dwFlags,
			    (LPMCI_GETDEVCAPS_PARMS)lpParms);
            break;

        case MCI_CONFIGURE:

            if (!(dwFlags & MCI_TEST))
                ConfigDialog(NULL, NULL);
	    dwRet = 0L;
	    break;

        case MCI_INFO:
            dwRet = GraphicInfo16(NULL, dwFlags, (LPMCI_DGV_INFO_PARMS)lpParms);
            break;

        case MCI_CLOSE_DRIVER:
            dwRet = 0L;
            break;

        default:
            dwRet = MCIERR_UNSUPPORTED_FUNCTION;
            break;
    }

    GraphicImmediateNotify (wDeviceID, lpParms, dwFlags, dwRet);
    return (dwRet);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | mciDriverEntry | This function is the MCI handler
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @parm DWORD | lpParms | Parameters for this message.
 *
 * @rdesc Error Constant. 0L on success
 *
 ***************************************************************************/

DWORD PASCAL mciDriverEntry (UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    NPMCIGRAPHIC npMCI = 0L;
    DWORD dwRet = MCIERR_UNRECOGNIZED_COMMAND;
    BOOL fDelayed = FALSE;
    BOOL fNested = FALSE;

    /* All current commands require a parameter block. */

    if (!lpParms && (dwFlags & MCI_NOTIFY))
        return (MCIERR_MISSING_PARAMETER);

    npMCI = (NPMCIGRAPHIC) (UINT)mciGetDriverData(wDeviceID);

    if (!npMCI)
        return mciSpecial(wDeviceID, wMessage, dwFlags, lpParms);

#if 0
#ifdef DEBUG
    else
        Assert(npMCI->mciid == MCIID);
#endif

    if (npMCI->wMessageCurrent) {
	fNested = TRUE;
	
	if (wMessage != MCI_STATUS && wMessage != MCI_GETDEVCAPS &&
		    wMessage != MCI_INFO) {
	    //DPF(("Warning!!!!!\n"));
	    //DPF(("Warning!!!!!     MCIAVI reentered: received %x while processing %x\n", wMessage, npMCI->wMessageCurrent));
	    //DPF(("Warning!!!!!\n"));
//	    Assert(0);
//	    return MCIERR_DEVICE_NOT_READY;
	}
    } else	
	npMCI->wMessageCurrent = wMessage;
#endif
    switch (wMessage) {

	case MCI_CLOSE_DRIVER:


            // Question:  Should we set the driver data to NULL
            // before closing the device?  It would seem the right order.
            // So... we have moved this line before the call to GraphicClose
            mciSetDriverData(wDeviceID, 0L);

	    // note that GraphicClose will release and delete the critsec
 	    dwRet = GraphicClose(npMCI);
	
	    npMCI = NULL;
	    break;

    	case MCI_PLAY:
	
            dwRet = GraphicPlay(npMCI, dwFlags, (LPMCI_PLAY_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

    	case MCI_CUE:
	
            dwRet = GraphicCue(npMCI, dwFlags, (LPMCI_DGV_CUE_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

	case MCI_STEP:

            dwRet = GraphicStep(npMCI, dwFlags, (LPMCI_DGV_STEP_PARMS)lpParms);
	    fDelayed = TRUE;
	    break;
	
	case MCI_STOP:

            dwRet = GraphicStop(npMCI, dwFlags, lpParms);
            break;

	case MCI_SEEK:

            dwRet = GraphicSeek (npMCI, dwFlags, (LPMCI_SEEK_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

	case MCI_PAUSE:

            dwRet = GraphicPause(npMCI, dwFlags, lpParms);
	    fDelayed = TRUE;
            break;

        case MCI_RESUME:

            dwRet = GraphicResume(npMCI, dwFlags, lpParms);
	    fDelayed = TRUE;
            break;

        case MCI_SET:

            dwRet = GraphicSet(npMCI, dwFlags,
				(LPMCI_DGV_SET_PARMS)lpParms);
	    break;

	case MCI_STATUS:

            dwRet = GraphicStatus(npMCI, dwFlags,
				(LPMCI_DGV_STATUS_PARMS)lpParms);
	    break;

	case MCI_INFO:

 	    dwRet = GraphicInfo (npMCI, dwFlags, (LPMCI_DGV_INFO_PARMS)lpParms);
	    break;

        case MCI_GETDEVCAPS:

            dwRet = GraphicGetDevCaps(npMCI, dwFlags, (LPMCI_GETDEVCAPS_PARMS)lpParms);
	    break;

        case MCI_REALIZE:

            dwRet = GraphicRealize(npMCI, dwFlags);
            break;

        case MCI_UPDATE:

            dwRet = GraphicUpdate(npMCI, dwFlags, (LPMCI_DGV_UPDATE_PARMS)lpParms);
            break;

	case MCI_WINDOW:
 	
            dwRet = GraphicWindow(npMCI, dwFlags, (LPMCI_DGV_WINDOW_PARMS)lpParms);
	    break;

        case MCI_PUT:

 	    dwRet = GraphicPut(npMCI, dwFlags, (LPMCI_DGV_RECT_PARMS)lpParms);
            break;
	
        case MCI_WHERE:

            dwRet = GraphicWhere(npMCI, dwFlags, (LPMCI_DGV_RECT_PARMS)lpParms);
            break;
	
	case MCI_CONFIGURE:
	    dwRet = GraphicConfig(npMCI, dwFlags);
	    break;

	case MCI_SETAUDIO:
	    dwRet = GraphicSetAudio(npMCI, dwFlags,
			(LPMCI_DGV_SETAUDIO_PARMS) lpParms);
	    break;

	case MCI_SETVIDEO:
	    dwRet = GraphicSetVideo(npMCI, dwFlags,
			(LPMCI_DGV_SETVIDEO_PARMS) lpParms);
	    break;

	case MCI_SIGNAL:
	    dwRet = GraphicSignal(npMCI, dwFlags,
			(LPMCI_DGV_SIGNAL_PARMS) lpParms);
	    break;
	
	case MCI_LIST:
	    dwRet = GraphicList(npMCI, dwFlags,
			(LPMCI_DGV_LIST_PARMS) lpParms);
	    break;

#if 0
        case MCI_LOAD:
	    dwRet = GraphicLoad(npMCI, dwFlags,
				  (LPMCI_DGV_LOAD_PARMS) lpParms);
	    break;
#endif
	
        case MCI_RECORD:
        case MCI_SAVE:
	
        case MCI_CUT:
        case MCI_COPY:
        case MCI_PASTE:
        case MCI_UNDO:
	
	case MCI_DELETE:
	case MCI_CAPTURE:
	case MCI_QUALITY:
	case MCI_MONITOR:
	case MCI_RESERVE:
	case MCI_FREEZE:
	case MCI_UNFREEZE:
            dwRet = MCIERR_UNSUPPORTED_FUNCTION;
            break;
	
	    /* Do we need this case? */
	default:
            dwRet = MCIERR_UNRECOGNIZED_COMMAND;
            break;
    }

    if (!fDelayed || (dwFlags & MCI_TEST)) {
	/* We haven't processed the notify yet. */
        if (npMCI && (dwFlags & MCI_NOTIFY) && (!LOWORD(dwRet)))
	    /* Throw away the old notify */
            GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUPERSEDED);

	/* And send the new one out immediately. */
        GraphicImmediateNotify(wDeviceID, lpParms, dwFlags, dwRet);
    }

    if (npMCI) {
        /* Everything from here on relies on npMCI still being around */

#if 0
        /* If there's an error, don't save the callback.... */
        if (fDelayed && dwRet != 0 && (dwFlags & MCI_NOTIFY)) {

	    // this might be too late, of course, but shouldn't do
	    // any harm
    	    npMCI->hCallback = 0;
	}

        //
        //  see if we need to tell the DRAW device about moving.
        //  MPlayer is sending the status and position command alot
        //  so this is a "timer"
        //
        //  !!!do we need to do it this often?
        //
        if (npMCI->dwFlags & MCIAVI_WANTMOVE)
    	    CheckWindowMove(npMCI, FALSE);

        if (!fNested)
	    npMCI->wMessageCurrent = 0;
#endif
    }

    return dwRet;
}

#define CONFIG_ID   10000L  // Use the hiword of dwDriverID to identify
extern HWND ghwndConfig;

/* Link to DefDriverProc in MMSystem explicitly, so we don't get the
** one in USER by mistake.
*/
#ifndef _WIN32
extern DWORD FAR PASCAL mmDefDriverProc(DWORD, HANDLE, UINT, DWORD, DWORD);
#else
#define mmDefDriverProc DefDriverProc
#endif

#ifndef _WIN32
BOOL FAR PASCAL LibMain (HANDLE hModule, int cbHeap, LPSTR lpchCmdLine)
{
    ghModule = hModule;
    return TRUE;
}
#else
#if 0
// Get the module handle on DRV_LOAD
BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    if (Reason == DLL_PROCESS_ATTACH) {
        ghModule = hModule;  // All we need to save is our module handle...
    } else {
        if (Reason == DLL_PROCESS_DETACH) {
        }
    }
    return TRUE;
}

#endif
#endif // WIN16

/***************************************************************************
 *
 * @doc     INTERNAL
 *
 * @api     DWORD | DriverProc | The entry point for an installable driver.
 *
 * @parm    DWORD | dwDriverId | For most messages, dwDriverId is the DWORD
 *          value that the driver returns in response to a DRV_OPEN message.
 *          Each time that the driver is opened, through the DrvOpen API,
 *          the driver receives a DRV_OPEN message and can return an
 *          arbitrary, non-zero, value. The installable driver interface
 *          saves this value and returns a unique driver handle to the
 *          application. Whenever the application sends a message to the
 *          driver using the driver handle, the interface routes the message
 *          to this entry point and passes the corresponding dwDriverId.
 *
 *          This mechanism allows the driver to use the same or different
 *          identifiers for multiple opens but ensures that driver handles
 *          are unique at the application interface layer.
 *
 *          The following messages are not related to a particular open
 *          instance of the driver. For these messages, the dwDriverId
 *          will always be  ZERO.
 *
 *              DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm    UINT | wMessage | The requested action to be performed. Message
 *          values below DRV_RESERVED are used for globally defined messages.
 *          Message values from DRV_RESERVED to DRV_USER are used for
 *          defined driver portocols. Messages above DRV_USER are used
 *          for driver specific messages.
 *
 * @parm    DWORD | dwParam1 | Data for this message.  Defined separately for
 *          each message
 *
 * @parm    DWORD | dwParam2 | Data for this message.  Defined separately for
 *          each message
 *
 * @rdesc Defined separately for each message.
 *
 ***************************************************************************/

DWORD FAR PASCAL _LOADDS DriverProc (DWORD dwDriverID, HANDLE hDriver, UINT wMessage,
    DWORD dwParam1, DWORD dwParam2)
{
    DWORD dwRes = 0L;


    /*
     * critical sections are now per-device. This means they
     * cannot be held around the whole driver-proc, since until we open
     * the device, we don't have a critical section to hold.
     * The critical section is allocated in mciSpecial on opening. It is
     * also held in mciDriverEntry, in GraphicWndProc, and around
     * all worker thread draw functions.
     */


    switch (wMessage)
        {

        // Standard, globally used messages.

        case DRV_LOAD:

#ifdef _WIN32
            if (ghModule) {
                Assert(!"Did not expect ghModule to be non-NULL");
            }
            ghModule = GetDriverModuleHandle(hDriver);  // Remember

            #define GET_MAPPING_MODULE_NAME         TEXT("wow32.dll")
            runningInWow = (GetModuleHandle(GET_MAPPING_MODULE_NAME) != NULL);
#endif
            if (GraphicInit())       // Initialize graphic mgmt.
                dwRes = 1L;
            else
                dwRes = 0L;

            break;

        case DRV_FREE:

            GraphicFree16();
            dwRes = 1L;
            //DPF(("Returning from DRV_FREE\n"));
#if 0
            Assert(npMCIList == NULL);
#endif
            ghModule = NULL;
            break;

        case DRV_OPEN:

            if (!dwParam2)
                dwRes = CONFIG_ID;
            else
                dwRes = GraphicDrvOpen((LPMCI_OPEN_DRIVER_PARMS)dwParam2);

            break;

        case DRV_CLOSE:
	    /* If we have a configure dialog up, fail the close.
	    ** Otherwise, we'll be unloaded while we still have the
	    ** configuration window up.
	    */
#if 0
	    if (ghwndConfig)
		dwRes = 0L;
	    else
#endif
		dwRes = 1L;
            break;

        case DRV_ENABLE:

            dwRes = 1L;
            break;

        case DRV_DISABLE:

            dwRes = 1L;
            break;

        case DRV_QUERYCONFIGURE:

            dwRes = 1L;	/* Yes, we can be configured */
            break;

        case DRV_CONFIGURE:
            ConfigDialog((HWND)(UINT)dwParam1, NULL);
            dwRes = 1L;
            break;

        default:

            if (!HIWORD(dwDriverID) &&
                wMessage >= DRV_MCI_FIRST &&
                wMessage <= DRV_MCI_LAST)

                dwRes = mciDriverEntry ((UINT)dwDriverID,
                                        wMessage,
                                        dwParam1,
                                        (LPMCI_GENERIC_PARMS)dwParam2);
            else
                dwRes = mmDefDriverProc(dwDriverID,
                                      hDriver,
                                      wMessage,
                                      dwParam1,
                                      dwParam2);
            break;
        }

    return dwRes;
}

