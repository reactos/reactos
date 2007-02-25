/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/user.c
 * PURPOSE:             WDM Audio Support - User Mode Interface
 * PROGRAMMER:			Andrew Greenwood
 * UPDATE HISTORY:
 *						Nov 18, 2005: Created
 */

/*
 *  The GETDEVCAPS message parameters have different meaning in our case.
 *  The second parameter usually indicates the struct size. But this has
 *  been replaced by a pointer to the device path.
 */



#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "wdmaud.h"


APIENTRY LRESULT DriverProc(
    DWORD DriverID,
    HDRVR DriverHandle,
    UINT Message,
    LONG Param1,
    LONG Param2)
{
	/*
		Only DRV_ENABLE and DRV_DISABLE need special handling - everything else
		is just implemented to aid in debugging.
	*/

	DPRINT("DriverProc %d %d %d %d %d\n", (INT) DriverID, (int) DriverHandle, (int) Message, (int) Param1, (int) Param2);

    switch(Message)
    {
		/*
			DRV_LOAD is the first message we receive, to say we've been loaded.
			DriverHandle is documented as being unused, but appears to be the
			number 3 (on my system, at least.)
		*/
        case DRV_LOAD :
            DPRINT("DRV_LOAD\n");
            /* We should initialize the device list */
            return TRUE; // dont need to do any more

        case DRV_FREE :
            /* We should stop all wave and MIDI playback */
            DPRINT("DRV_FREE\n");
            return TRUE;

		/*
			DRV_OPEN is sent when WINMM wishes to open the driver. Param1
			can specify configuration information, but we don't need any.
		*/
        case DRV_OPEN :
            DPRINT("DRV_OPEN\n");
            return TRUE;

        case DRV_CLOSE :
            DPRINT("DRV_CLOSE\n");
            return TRUE;

		/*
            Enabling this driver causes the kernel-mode portion of WDMAUD to
            be opened. We send a message to the kernel-mode driver to say that
            we want to make use of it.

            And, of course, when we are being disabled, we tell the kernel-mode
            portion that we don't require its services any more, and close
            the handle to it.
		*/

        case DRV_ENABLE :
        {
            DPRINT("DRV_ENABLE\n");
            return EnableKernelInterface();
        }

        case DRV_DISABLE :
            DPRINT("DRV_DISABLE\n");
            DisableKernelInterface();
            return TRUE;

        /*
            We don't actually support configuration or installation, so these
            could probably be safely pruned.
        */

        case DRV_QUERYCONFIGURE :
            DPRINT("DRV_QUERYCONFIGURE\n");
            return FALSE;

        case DRV_CONFIGURE :
            DPRINT("DRV_CONFIGURE\n");
            return FALSE;

        case DRV_INSTALL :
            DPRINT("DRV_INSTALL\n");
            return TRUE;   /* ok? */

		case DRV_REMOVE :
            DPRINT("DRV_REMOVE\n");
            return TRUE;

        default :
            DPRINT("?\n");
            return DefDriverProc(DriverID, DriverHandle, Message, Param1, Param2);
    };
}

void NotifyClient(
    PWDMAUD_DEVICE_INFO device,
    DWORD message,
    DWORD p1,
    DWORD p2
)
{
    DPRINT("Calling client\n");

    DriverCallback(device->client_callback,
                   HIWORD(device->flags),
                   (HDRVR) device->handle,
                   message,
                   device->client_instance,
                   0,
                   0);
}

APIENTRY DWORD widMessage(
    DWORD id,
    DWORD message,
    DWORD user,
    DWORD p1,
    DWORD p2
)
{
	DPRINT("widMessage %d %d %d %d %d\n", (int)id, (int)message, (int)user, (int)p1, (int)p2);

	switch(message)
	{
		case DRVM_INIT :
			DPRINT("WIDM_INIT\n");
            return AddWaveInDevice((WCHAR*) p2);

		case DRVM_EXIT :
            DPRINT("WIDM_EXIT\n");
            return RemoveWaveInDevice((WCHAR*) p2); /* FIXME */

		case WIDM_GETNUMDEVS :
			DPRINT("WIDM_GETNUMDEVS\n");
            return GetWaveInCount((WCHAR*) p1);
	
        case WIDM_GETDEVCAPS :
            DPRINT("WIDM_GETDEVCAPS\n");
            return GetWaveInCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);
   };

	return MMSYSERR_NOERROR;
	return MMSYSERR_NOTSUPPORTED;
}

APIENTRY DWORD wodMessage(
    DWORD id,
    DWORD message,
    DWORD user,
    DWORD p1,
    DWORD p2)
{
	DPRINT("wodMessage %d %d %d %d %d\n",
           (int)id, (int)message, (int)user, (int)p1, (int)p2);

	switch(message)
	{
        /*
         *  DRVM_INIT
         *    Parameter 1 : Not used
         *    Parameter 2 : Topology path
         */
		case DRVM_INIT :
			DPRINT("DRVM_INIT\n");
			return AddWaveOutDevice((WCHAR*) p2);

		case DRVM_EXIT :
            DPRINT("DRVM_EXIT\n");
            return RemoveWaveOutDevice((WCHAR*) p2); /* FIXME? */

        /*
         *  WODM_GETNUMDEVS
         *    Parameter 1 : Topology device path
         *    Parameter 2 : Not used
         */
		case WODM_GETNUMDEVS :
			DPRINT("WODM_GETNUMDEVS\n");
			return GetWaveOutCount((WCHAR*) p1);
		
        /*
         *  WODM_GETDEVCAPS
         *    Parameter 1 : Pointer to a MDEVICECAPS struct
         *    Parameter 2 : Device path
         */
        case WODM_GETDEVCAPS :
			DPRINT("WODM_GETDEVCAPS\n");
			return GetWaveOutCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);

        /*
         *  WODM_OPEN
         *    Parameter 1 : Pointer to a WAVEOPENDESC struct (the dnDevNode
         *                  member holds a device path.)
         *    Parameter 2 : Flags
         */
		case WODM_OPEN :
			DPRINT("WODM_OPEN\n");
            return OpenWaveOutDevice(id,
                                     (LPWAVEOPENDESC) p1,
                                     p2,
                                     (PWDMAUD_DEVICE_INFO*) user);
		
        case WODM_CLOSE :
			DPRINT("WODM_CLOSE\n");
			return CloseWaveDevice((PWDMAUD_DEVICE_INFO) user);

        case WODM_PREPARE :
            DPRINT("WODM_PREPARE\n");
            return PrepareWaveHeader((PWDMAUD_DEVICE_INFO) user,
                                     (PWAVEHDR) p1);

        case WODM_UNPREPARE :
            DPRINT("WODM_UNPREPARE\n");
            return UnprepareWaveHeader((PWAVEHDR) p1);

        case WODM_WRITE :
            DPRINT("WODM_WRITE\n");
            return WriteWaveData((PWDMAUD_DEVICE_INFO) user,
                                 (PWAVEHDR) p1);
	}

	DPRINT("* NOT IMPLEMENTED *\n");
	return MMSYSERR_NOTSUPPORTED;
}

APIENTRY DWORD midMessage(
    DWORD id,
    DWORD message,
    DWORD user,
    DWORD p1,
    DWORD p2
)
{
	DPRINT("midMessage %d %d %d %d %d\n", (int)id, (int)message, (int)user, (int)p1, (int)p2);

	switch(message)
	{
        case DRVM_INIT :
            DPRINT("MIDM_INIT\n");
            return AddMidiInDevice((WCHAR*) p2);

        case DRVM_EXIT :
            DPRINT("MIDM_EXIT\n");
            return RemoveMidiInDevice((WCHAR*) p2); /* FIXME */

        case MIDM_GETNUMDEVS :
            DPRINT("MIDM_GETNUMDEVS\n");
            return GetMidiInCount((WCHAR*) p1);
    
        case MIDM_GETDEVCAPS :
            DPRINT("MIDM_GETDEVCAPS\n");
            return GetMidiInCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);
	};

    DPRINT("* NOT IMPLEMENTED *\n");
	return MMSYSERR_NOTSUPPORTED;
}

APIENTRY DWORD modMessage(
    DWORD id,
    DWORD message,
    DWORD user,
    DWORD p1,
    DWORD p2
)
{
	DPRINT("modMessage %d %d %d %d %d\n", (int)id, (int)message, (int)user, (int)p1, (int)p2);

	switch(message)
	{
        case DRVM_INIT :
            DPRINT("MODM_INIT\n");
            return AddMidiOutDevice((WCHAR*) p2);

        case DRVM_EXIT :
            DPRINT("MODM_EXIT\n");
            return RemoveMidiOutDevice((WCHAR*) p2); /* FIXME */

        case MODM_GETNUMDEVS :
            DPRINT("MODM_GETNUMDEVS\n");
            return GetMidiOutCount((WCHAR*) p1);
    
        case MODM_GETDEVCAPS :
            DPRINT("MODM_GETDEVCAPS\n");
            return GetMidiOutCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);

        case MODM_OPEN :
            DPRINT("MODM_OPEN\n");
            return OpenMidiOutDevice(id,
                                     (LPMIDIOPENDESC) p1,
                                     p2,
                                     (PWDMAUD_DEVICE_INFO*) user);

        case MODM_CLOSE :
            DPRINT("MODM_CLOSE\n");
            return CloseMidiDevice((PWDMAUD_DEVICE_INFO) user);

        case MODM_DATA :
            DPRINT("MODM_DATA\n");
            return WriteMidiShort((PWDMAUD_DEVICE_INFO) user, p1);

        case MODM_LONGDATA :
            DPRINT("MODM_LONGDATA\n");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_RESET :
            DPRINT("MODM_RESET\n");
            return ResetMidiDevice((PWDMAUD_DEVICE_INFO) user);

        case MODM_SETVOLUME :
            DPRINT("MODM_SETVOLUME\n");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_GETVOLUME :
            DPRINT("MODM_GETVOLUME\n");
            return MMSYSERR_NOTSUPPORTED;

/* TODO: WINE's mmddk.h needs MODM_PREFERRED to be defined (value is ??) */
/*
        case MODM_PREFERRED :
            DPRINT("MODM_PREFERRED\n");
            return MMSYSERR_NOTSUPPORTED;
*/

    };

    DPRINT("* NOT IMPLEMENTED *\n");
    return MMSYSERR_NOTSUPPORTED;
}

APIENTRY DWORD mxdMessage(
    DWORD id,
    DWORD message,
    DWORD user,
    DWORD p1,
    DWORD p2
)
{
    DPRINT("mxdMessage %d %d %d %d %d\n", (int)id, (int)message, (int)user, (int)p1, (int)p2);

	switch(message)
	{
        case DRVM_INIT :
            DPRINT("MXDM_INIT\n");
            return AddMixerDevice((WCHAR*) p2);
            
        case DRVM_EXIT :
            DPRINT("MXDM_EXIT\n");
            return RemoveMixerDevice((WCHAR*) p2); /* FIXME */

        case MXDM_GETNUMDEVS :
            DPRINT("MXDM_GETNUMDEVS\n");
            return GetMixerCount((WCHAR*) p1);

        case MXDM_GETDEVCAPS :
            DPRINT("MXDM_GETDEVCAPS\n");
            return GetMixerCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);

        /* ... */
	};

    DPRINT("* NOT IMPLEMENTED *\n");
    return MMSYSERR_NOTSUPPORTED;
}

APIENTRY DWORD auxMessage(DWORD id, DWORD message, DWORD user, DWORD p1, DWORD p2)
{
	DPRINT("auxMessage %d %d %d %d %d\n", (int)id, (int)message, (int)user, (int)p1, (int)p2);

    switch(message)
    {
        case DRVM_INIT :
            DPRINT("AUXDM_INIT\n");
            return AddAuxDevice((WCHAR*) p2);
            
        case DRVM_EXIT :
            DPRINT("AUXDM_EXIT\n");
            return RemoveAuxDevice((WCHAR*) p2); /* FIXME */

        case AUXDM_GETNUMDEVS :
            DPRINT("AUXDM_GETNUMDEVS\n");
            return GetAuxCount((WCHAR*) p1);

        case AUXDM_GETDEVCAPS :
            DPRINT("AUXDM_GETDEVCAPS\n");
            return GetAuxCapabilities(id, (WCHAR*) p2, (LPMDEVICECAPSEX) p1);

        /* ... */
    };

    DPRINT("* NOT IMPLEMENTED *\n");
	return MMSYSERR_NOTSUPPORTED;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
    DPRINT("DllMain called!\n");

    if (Reason == DLL_PROCESS_ATTACH)
    {
		DisableThreadLibraryCalls(hInstance);
    }

    else if (Reason == DLL_PROCESS_DETACH)
    {
        DPRINT("*** wdmaud.drv is being closed ***\n");
        ReportMem();
    }

    return TRUE;
}

/* EOF */
